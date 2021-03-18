/******************************************************************************
* File Name: mqtt_operation.c
*
* Description: This file contains threads, functions, and other resources related
* to MQTT operations.
*
*******************************************************************************
* $ Copyright 2020-2021 Cypress Semiconductor $
*******************************************************************************/
#include <inttypes.h>
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "iot_mqtt.h"
#include "cJSON.h"
#include "mqtt_operation.h"

/* Set up logging for this demo. */
#include "iot_demo_logging.h"

/***************************************
*            Defines
****************************************/
#define TOPIC_HEAD  "$aws/things/Thing_"

/* Thread delays */
#define PUBLISH_THREAD_LOOP_DELAY_MS            (100)

/* MQTT Broker info */
#define KEEP_ALIVE_SECONDS                      (60)
#define MQTT_TIMEOUT_MS                         (5000)
#define PUBLISH_RETRY_LIMIT                     (10)
#define PUBLISH_RETRY_MS                        (1000)
#define MAX_JSON_MESSAGE_LENGTH                 (100)
#define MAX_TOPIC_LENGTH                        (50)

/* IP String length to copy from JSON message */
#define IP_STR_LEN                              (16)

/* Publish command size */
#define PUBLISH_CMD_SIZE_BYTES                  (4)

/*************** Publish Thread ***************/
/*
 * Summary: Thread to publish data to the cloud
 *
 *  @param[in] arg argument for the thread
 *
 */
void publishThread(void* arg)
{
    ( void )arg; /* Suppress compiler warning */

    uint8_t thingNumber;

    /* json message to send */
    char json[MAX_JSON_MESSAGE_LENGTH];

    /* Command pushed onto the queue to determine what to publish */
    uint8_t pubCmd[PUBLISH_CMD_SIZE_BYTES];

    /* Value popped from the queue to determine what to publish */
    uint8_t command[PUBLISH_CMD_SIZE_BYTES];

    char topic[MAX_TOPIC_LENGTH];       /* Buffer for topic name */
    uint16_t topicLength = 0;           /* Length of topic name */
    uint16_t messageLength = 0;         /* Length of JSON message */

    /* Publish the IP address to the server one time */
    pubCmd[0] = IP_CMD;
    xQueueSend(pub_queue, pubCmd, portMAX_DELAY);   /* Push value onto queue*/

    /* Push a shadow/get command to the publish queue for all things to get initial state */
    for(thingNumber = 0; thingNumber <= MAX_THING; thingNumber++)
    {
        pubCmd[0] = GET_CMD;
        pubCmd[1] = thingNumber; /* 2nd byte of the message will be the thing number */
        xQueueSend(pub_queue, pubCmd, portMAX_DELAY); /* Push value onto queue*/
    }

    while ( 1 )
    {
        /* Wait until a publish is requested */
        xQueueReceive(pub_queue, command, portMAX_DELAY);

        /* Set the topic for an update of my thing */
        topicLength = snprintf(topic, sizeof(topic), "%s%02d/shadow/update", TOPIC_HEAD, MY_THING);

        /* Setup the JSON message based on the command */
        switch(command[0])
        {
            case WEATHER_CMD:     /* publish temperature and humidity */
                messageLength = snprintf(json, sizeof(json), "{\"state\" : {\"reported\" : {\"temperature\":%.1f,\"humidity\":%.1f,\"light\":%.0f,\"weatherAlert\":%s}}}", iot_data[MY_THING].temp, iot_data[MY_THING].humidity, iot_data[MY_THING].light, iot_data[MY_THING].alert ? "true" : "false");
                break;
            case TEMPERATURE_CMD:     /* publish temperature */
                messageLength = snprintf(json, sizeof(json), "{\"state\" : {\"reported\" : {\"temperature\":%.1f} } }", iot_data[MY_THING].temp);
                break;
            case HUMIDITY_CMD:     /* publish humidity */
                messageLength = snprintf(json, sizeof(json), "{\"state\" : {\"reported\" : {\"humidity\":%.1f} } }", iot_data[MY_THING].humidity);
                break;
            case LIGHT_CMD:  /* publish light value */
                messageLength = snprintf(json, sizeof(json), "{\"state\" : {\"reported\" : {\"light\":%.1f} } }", iot_data[MY_THING].light);
                break;
            case ALERT_CMD: /* weather alert */
                if(iot_data[MY_THING].alert)
                {
                    messageLength = snprintf(json, sizeof(json), "{\"state\" : {\"reported\" : {\"weatherAlert\":true} } }");
                }
                else
                {
                    messageLength = snprintf(json, sizeof(json), "{\"state\" : {\"reported\" : {\"weatherAlert\":false} } }");
                }
                break;
            case IP_CMD:    /* IP address */
                messageLength = snprintf(json, sizeof(json), "{\"state\" : {\"reported\" : {\"IPAddress\":\"%s\"} } }", iot_data[MY_THING].ip_str);
                break;
            case GET_CMD:   /* Get starting state of other things */
                messageLength = snprintf(json, sizeof(json), "{}");
                /* Override the topic to do a get of the specified thing's shadow */
                topicLength = snprintf(topic, sizeof(topic), "%s%02d/shadow/get", TOPIC_HEAD, command[1]);
                break;
        }

        /* PUBLISH (and wait) for all messages. */
        PublishMessage( mqtt_connection,
                        topic,
                        topicLength,
                        json,
                        messageLength);

        vTaskDelay(pdMS_TO_TICKS(PUBLISH_THREAD_LOOP_DELAY_MS));
    }
}

/***************  MQTT Subscription callback ***************/
/*
 * Summary: Called by the MQTT library when an incoming PUBLISH message is received.
 *
 * The demo uses this callback to handle incoming PUBLISH messages.
 * @param[in] param1 Counts the total number of received PUBLISH messages. This
 * callback will increment this counter.
 * @param[in] pPublish Information about the incoming PUBLISH message passed by
 * the MQTT library.
 */
void MqttSubscriptionCallback( void * param1,
                               IotMqttCallbackParam_t * const pPublish )
{
    ( void )param1;             /* Suppress compiler warning */
    char topicStr[MAX_TOPIC_LENGTH] = {0};    /* String to copy the topic into */
    char pubType[20] =  {0};    /* String to compare to the publish type */
    uint32_t thingNumber;           /* The number of the thing that published a message */

    /* Copy the topic name to a null terminated string */
    memcpy(topicStr, pPublish->u.message.info.pTopicName, pPublish->u.message.info.topicNameLength);
    topicStr[pPublish->u.message.info.topicNameLength] = 0; /* Add termination */

    /* Copy the message to a null terminated string */
    char *pPayload = (char*)pvPortMalloc(pPublish->u.message.info.payloadLength + 1);
    memcpy(pPayload, pPublish->u.message.info.pPayload, pPublish->u.message.info.payloadLength);
    topicStr[pPublish->u.message.info.topicNameLength] = 0; /* Add termination */

    /* Scan the topic to see if it is one of the things we are interested in */
    sscanf(topicStr, "$aws/things/Thing_%2"PRIu32"/shadow/%19s", &thingNumber, pubType);

    /* Check to see if it is an initial get of the values of other things */
    if(strcmp(pubType,"get/accepted") == 0)
    {
        if(thingNumber != MY_THING) /* Only do the rest if it isn't the local thing */
        {
            /* Parse JSON message for the weather station data */
            cJSON *root = cJSON_Parse(pPayload);
            cJSON *state = cJSON_GetObjectItem(root,"state");
            cJSON *reported = cJSON_GetObjectItem(state,"reported");
            cJSON *ipValue = cJSON_GetObjectItem(reported,"IPAddress");
            if(ipValue->type == cJSON_String) /* Make sure we have a string */
            {
                strncpy(iot_data[thingNumber].ip_str, ipValue->valuestring, IP_STR_LEN);
            }
            iot_data[thingNumber].temp = (float) cJSON_GetObjectItem(reported,"temperature")->valuedouble;
            iot_data[thingNumber].humidity = (float) cJSON_GetObjectItem(reported,"humidity")->valuedouble;
            iot_data[thingNumber].light = (float) cJSON_GetObjectItem(reported,"light")->valuedouble;
            iot_data[thingNumber].alert = (bool) cJSON_GetObjectItem(reported,"weatherAlert")->valueint;
            cJSON_Delete(root);
        }
    }
    /* Check to see if it is an update published by another thing */
    if(strcmp(pubType,"update/documents") == 0)
    {
        if(thingNumber != MY_THING) /* Only do the rest if it isn't the local thing */
        {
            /* Parse JSON message for the weather station data */
            cJSON *root = cJSON_Parse(pPayload);
            cJSON *current = cJSON_GetObjectItem(root,"current");
            cJSON *state = cJSON_GetObjectItem(current,"state");
            cJSON *reported = cJSON_GetObjectItem(state,"reported");
            cJSON *ipValue = cJSON_GetObjectItem(reported,"IPAddress");
            if(ipValue->type == cJSON_String) /* Make sure we have a string */
            {
                strncpy(iot_data[thingNumber].ip_str, ipValue->valuestring, IP_STR_LEN);
            }
            iot_data[thingNumber].temp = (float) cJSON_GetObjectItem(reported,"temperature")->valuedouble;
            iot_data[thingNumber].humidity = (float) cJSON_GetObjectItem(reported,"humidity")->valuedouble;
            iot_data[thingNumber].light = (float) cJSON_GetObjectItem(reported,"light")->valuedouble;
            iot_data[thingNumber].alert = (bool) cJSON_GetObjectItem(reported,"weatherAlert")->valueint;
            cJSON_Delete(root);
            if(print_all)
            {
                print_thing_info(thingNumber);
            }
            /* Update the display if we are displaying this thing's data */
            if(thingNumber == disp_thing)
            {
                xSemaphoreGive(display_semaphore);
            }
        }
    }
    vPortFree(pPayload);
}

/*************** Initialize MQTT library***************/
/* @return `EXIT_SUCCESS` if all libraries were successfully initialized; EXIT_FAILURE` otherwise. */
int InitializeMqtt( void )
{
    int status = EXIT_SUCCESS;
    IotMqttError_t mqttInitStatus = IOT_MQTT_SUCCESS;

    mqttInitStatus = IotMqtt_Init();

    if( mqttInitStatus != IOT_MQTT_SUCCESS )
    {
        /* Failed to initialize MQTT library. */
        configPRINTF(("Failed to initialize MQTT library\r\n"));
        status = EXIT_FAILURE;
    }
    return status;
}

/*************** Establish a new connection to the MQTT server ***************/
/*
 * Summary: Establish a new connection to the MQTT server.
 *
 * @param[in] awsIotMqttMode Specify if this demo is running with the AWS IoT
 * MQTT server. Set this to `false` if using another MQTT server.
 * @param[in] pIdentifier NULL-terminated MQTT client identifier.
 * @param[in] pNetworkServerInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkCredentialInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkInterface The network interface to use for this demo.
 * @param[out] pmqtt_connection Set to the handle to the new MQTT connection.
 *
 * @return `EXIT_SUCCESS` if the connection is successfully established; `EXIT_FAILURE`
 * otherwise.
 */
int EstablishMqttConnection( bool awsIotMqttMode,
                             const char * pIdentifier,
                             void * pNetworkServerInfo,
                             void * pNetworkCredentialInfo,
                             const IotNetworkInterface_t * pNetworkInterface,
                             IotMqttConnection_t * pmqtt_connection )
{
    int status = EXIT_SUCCESS;
    IotMqttError_t connectStatus = IOT_MQTT_STATUS_PENDING;
    IotMqttNetworkInfo_t networkInfo = IOT_MQTT_NETWORK_INFO_INITIALIZER;
    IotMqttConnectInfo_t connectInfo = IOT_MQTT_CONNECT_INFO_INITIALIZER;

    /*
     * Set the members of the network info not set by the initializer. This
     * struct provided information on the transport layer to the MQTT connection.
     */
    networkInfo.createNetworkConnection = true;
    networkInfo.u.setup.pNetworkServerInfo = pNetworkServerInfo;
    networkInfo.u.setup.pNetworkCredentialInfo = pNetworkCredentialInfo;
    networkInfo.pNetworkInterface = pNetworkInterface;

    #if ( IOT_MQTT_ENABLE_SERIALIZER_OVERRIDES == 1 ) && defined( IOT_DEMO_MQTT_SERIALIZER )
        networkInfo.pMqttSerializer = IOT_DEMO_MQTT_SERIALIZER;
    #endif

    /* Set the members of the connection info not set by the initializer. */
    connectInfo.awsIotMqttMode = awsIotMqttMode;
    connectInfo.cleanSession = true;
    connectInfo.keepAliveSeconds = KEEP_ALIVE_SECONDS;
    connectInfo.pWillInfo = NULL;

    /* Use the parameter client identifier provided. */
    if( pIdentifier != NULL )
    {
        connectInfo.pClientIdentifier = pIdentifier;
        connectInfo.clientIdentifierLength = ( uint16_t ) strlen( pIdentifier );
    }

    connectStatus = IotMqtt_Connect( &networkInfo,
                                     &connectInfo,
                                     MQTT_TIMEOUT_MS,
                                     pmqtt_connection );

    if( connectStatus != IOT_MQTT_SUCCESS )
    {
        status = EXIT_FAILURE;
    }

    return status;
}

/*************** Modify MQTT Subscription ***************/
/*
 * Summary: Add or remove subscriptions by either subscribing or unsubscribing.
 *
 * @param[in] mqtt_connection The MQTT connection to use for subscriptions.
 * @param[in] operation Either #IOT_MQTT_SUBSCRIBE or #IOT_MQTT_UNSUBSCRIBE.
 * @param[in] pTopicFilters Array of topic filters for subscriptions.
 * @param[in] pTopicFilterLength Array of length of topic filters.
 * @param[in] filterCount Number of topic filters.
 * @param[in] pCallbackParameter The parameter to pass to the subscription
 * callback.
 *
 * @return `EXIT_SUCCESS` if the subscription operation succeeded; `EXIT_FAILURE`
 * otherwise.
 */
int ModifySubscriptions( IotMqttConnection_t mqtt_connection,
                         IotMqttOperationType_t operation,
                         const char ** pTopicFilters,
                         const uint16_t *pTopicFilterLength,
                         void * pCallbackParameter )
{
    int status = EXIT_SUCCESS;
    uint32_t i = 0;
    IotMqttError_t subscriptionStatus = IOT_MQTT_STATUS_PENDING;
    IotMqttSubscription_t pSubscriptions[ TOPIC_FILTER_COUNT ] = { IOT_MQTT_SUBSCRIPTION_INITIALIZER };

    /* Set the members of the subscription list. */
    for( i = 0; i < TOPIC_FILTER_COUNT; i++ )
    {
        pSubscriptions[ i ].qos = IOT_MQTT_QOS_1;
        pSubscriptions[ i ].pTopicFilter = pTopicFilters[ i ];
        pSubscriptions[ i ].topicFilterLength = pTopicFilterLength[ i ];
        pSubscriptions[ i ].callback.pCallbackContext = pCallbackParameter;
        pSubscriptions[ i ].callback.function = MqttSubscriptionCallback;
    }

    /* Modify subscriptions by either subscribing or unsubscribing. */
    if( operation == IOT_MQTT_SUBSCRIBE )
    {
        subscriptionStatus = IotMqtt_TimedSubscribe( mqtt_connection,
                                                     pSubscriptions,
                                                     TOPIC_FILTER_COUNT,
                                                     0,
                                                     MQTT_TIMEOUT_MS );

        /* Check the status of SUBSCRIBE. */
        switch( subscriptionStatus )
        {
            case IOT_MQTT_SUCCESS:
                IotLogInfo( "All demo topic filter subscriptions accepted.\r\n");
                break;

            case IOT_MQTT_SERVER_REFUSED:

                /* Check which subscriptions were rejected before exiting the demo. */
                for( i = 0; i < TOPIC_FILTER_COUNT; i++ )
                {
                    if( IotMqtt_IsSubscribed( mqtt_connection,
                                              pSubscriptions[ i ].pTopicFilter,
                                              pSubscriptions[ i ].topicFilterLength,
                                              NULL ) == true )
                    {
                        IotLogInfo("Topic filter %.*s was accepted.\r\n",
                                    pSubscriptions[ i ].topicFilterLength,
                                    pSubscriptions[ i ].pTopicFilter);
                    }
                    else
                    {
                        IotLogInfo("Topic filter %.*s was rejected.\r\n",
                                    pSubscriptions[ i ].topicFilterLength,
                                    pSubscriptions[ i ].pTopicFilter);
                    }
                }

                status = EXIT_FAILURE;
                break;

            default:

                status = EXIT_FAILURE;
                break;
        }
    }
    else if( operation == IOT_MQTT_UNSUBSCRIBE )
    {
        subscriptionStatus = IotMqtt_TimedUnsubscribe( mqtt_connection,
                                                       pSubscriptions,
                                                       TOPIC_FILTER_COUNT,
                                                       0,
                                                       MQTT_TIMEOUT_MS );

        /* Check the status of UNSUBSCRIBE. */
        if( subscriptionStatus != IOT_MQTT_SUCCESS )
        {
            status = EXIT_FAILURE;
        }
    }
    else
    {
        /* Only SUBSCRIBE and UNSUBSCRIBE are valid for modifying subscriptions. */
        IotLogInfo("MQTT operation %s is not valid for modifying subscriptions.\r\n",
                    IotMqtt_OperationType( operation ) );

        status = EXIT_FAILURE;
    }

    return status;
}

/*************** Publish MQTT Message ***************/
/*
 * Summary: Publish MQTT Message
 *
 * @param[in] mqtt_connection The MQTT connection to use for publishing.
 * @param[in] Topic name for publishing.
 * @param[in] Topic name length.
 * @param[in] Message for publishing.
 * @param[in] Message length.
 *
 * @return `EXIT_SUCCESS` if all messages are published and received; `EXIT_FAILURE`
 * otherwise.
 */
int PublishMessage( IotMqttConnection_t mqtt_connection,
                    char* topic,
                    uint16_t topicLength,
                    char* mqttMessage,
                    uint16_t messageLength)
{
    int status = EXIT_SUCCESS;
    IotMqttError_t publishStatus = IOT_MQTT_STATUS_PENDING;
    IotMqttPublishInfo_t publishInfo = IOT_MQTT_PUBLISH_INFO_INITIALIZER;
    IotMqttCallbackInfo_t publishComplete = IOT_MQTT_CALLBACK_INFO_INITIALIZER;

    /* The MQTT library should invoke this callback when a PUBLISH message
     * is successfully transmitted. Assign NULL to not attach a callback */
    publishComplete.function = NULL;

    /* Pass the PUBLISH number to the operation complete callback.
       Assigned NULL since callback is not attached */
    publishComplete.pCallbackContext = NULL;

    /* Set the common members of the publish info. */
    publishInfo.qos = IOT_MQTT_QOS_1;
    publishInfo.topicNameLength = topicLength;
    publishInfo.pPayload = mqttMessage;
    publishInfo.retryMs = PUBLISH_RETRY_MS;
    publishInfo.retryLimit = PUBLISH_RETRY_LIMIT;
    publishInfo.payloadLength = ( size_t ) messageLength;
    publishInfo.pTopicName = topic;

    /* PUBLISH a message */
    publishStatus = IotMqtt_Publish( mqtt_connection,
                                     &publishInfo,
                                     0,
                                     &publishComplete,
                                     NULL );

    if( publishStatus != IOT_MQTT_STATUS_PENDING )
    {
        status = EXIT_FAILURE;
    }
    return status;
}
