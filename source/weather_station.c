/******************************************************************************
* File Name: iot_demo_weather_station.c
*
* Related Document: See Readme.md
*
* Description: This example demonstrates an AWS IoT Weather Station using
* PSoC 6 MCU. The station connects to AWS IoT cloud service using MQTT protocol
* and publishes weather updates. Also, the station subscribes to updates from other
* weather stations.
*
* Hardware Dependency: CY8CKIT-032 AFE Shield
*                      CY8CKIT-062-WiFi-BT Pioneer kit
*
*******************************************************************************
* $ Copyright 2020-2021 Cypress Semiconductor $
*******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "iot_mqtt.h"
#include "iot_wifi.h"
#include "aws_demo.h"
#include "GUI.h"
#include "mqtt_operation.h"
#include "console_operation.h"
#include "afe_shield_operation.h"

/***************************************
*            Defines
****************************************/
/* Thread stack size and priorities */
#define CAPSENSE_THREAD_STACK_SIZE              (1024*2)
#define CAPSENSE_THREAD_PRIORITY                (tskIDLE_PRIORITY + 2)
#define WEATHER_DATA_THREAD_STACK_SIZE          (1024)
#define WEATHER_DATA_THREAD_PRIORITY            (tskIDLE_PRIORITY + 2)
#define DISPLAY_THREAD_STACK_SIZE               (1024*4)
#define DISPLAY_THREAD_PRIORITY                 (tskIDLE_PRIORITY + 3)
#define PUBLISH_THREAD_STACK_SIZE               (1024*2)
#define PUBLISH_THREAD_PRIORITY                 (tskIDLE_PRIORITY + 1)
#define COMMAND_THREAD_STACK_SIZE               (1024*2)
#define COMMAND_THREAD_PRIORITY                 (tskIDLE_PRIORITY + 3)

/* Publish weather data every 30 seconds */
#define MESSAGE_PUBLISH_INTERVAL_MS             (30000)

/* Queue details */
#define PUBLISH_CMD_SIZE_BYTES                  (4)
#define QUEUE_SIZE                              (50)

/* Mechanical Buttons */
#define MECH_BTN1                               (CYBSP_D4)
#define MECH_BTN2                               (CYBSP_D12)

/***************************************
*          Global Variables
****************************************/
iot_data_t iot_data[MAX_THING + 1];  /* Array to hold data from all IoT things */
/* Handle of the MQTT connection used in this demo. */
IotMqttConnection_t mqtt_connection = IOT_MQTT_CONNECTION_INITIALIZER;

/* RTOS constructs */
SemaphoreHandle_t display_semaphore;
SemaphoreHandle_t i2c_mutex;
QueueHandle_t pub_queue;
TimerHandle_t message_timer;

/***************************************
*          Forward Declaration
****************************************/
/* ISRs */
void publish_button_isr(void *callback_arg, cyhal_gpio_event_t event);
void alert_button_isr(void *callback_arg, cyhal_gpio_event_t event);

/* Callback functions */
void publish30sec(TimerHandle_t xTimer);

int RunApplication(bool awsIotMqttMode,
                   const char * pIdentifier,
                   void * pNetworkServerInfo,
                   void * pNetworkCredentialInfo,
                   const IotNetworkInterface_t * pNetworkInterface );
/*******************************************************************************
* Function Name: InitApplication
********************************************************************************
* Summary:
*  Initializes the tasks required to run the application.
*
*******************************************************************************/
void InitApplication(void)
{
    static demoContext_t appContext =
    {
        .networkTypes                = AWSIOT_NETWORK_TYPE_WIFI,
        .demoFunction                = RunApplication,
        .networkConnectedCallback    = NULL,
        .networkDisconnectedCallback = NULL
    };

    Iot_CreateDetachedThread( runDemoTask,
                              &appContext,
                              tskIDLE_PRIORITY,
                              (configMINIMAL_STACK_SIZE * 8) );
}

 /********************** Demo starter **************************/
 /*
 * Summary: This thread starts all the necessary threads and does the required
 * configurations for the demo.
 *
 * @param[in] awsIotMqttMode Specify if this demo is running with the AWS IoT
 * MQTT server. Set this to `false` if using another MQTT server.
 * @param[in] pIdentifier NULL-terminated MQTT client identifier.
 * @param[in] pNetworkServerInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkCredentialInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkInterface The network interface to use for this demo.
 *
 */
int RunApplication(bool awsIotMqttMode,
                   const char * pIdentifier,
                   void * pNetworkServerInfo,
                   void * pNetworkCredentialInfo,
                   const IotNetworkInterface_t * pNetworkInterface )
{
    /* variable to store IP address */
    uint8_t ucTempIp[4] = { 0 };

    /* variable to control loop statements */
    int loop;

    /* Topics used as both topic filters and topic names in this demo. */
    const char * pTopics[ TOPIC_FILTER_COUNT ] =
    {
        "$aws/things/+/shadow/update/documents",
        "$aws/things/+/shadow/get/accepted"
    };

    /* Length of topic names as per pTopics array */
    const uint16_t pTopicsSize[ TOPIC_FILTER_COUNT ] =
    {
        (uint16_t)sizeof("$aws/things/+/shadow/update/documents") - 1,
        (uint16_t)sizeof("$aws/things/+/shadow/get/accepted") - 1
    };

    /* Setup Thread Control entities */
    display_semaphore = xSemaphoreCreateBinary();
    pub_queue = xQueueCreate( QUEUE_SIZE, PUBLISH_CMD_SIZE_BYTES );

    /* Mutex to secure I2C object use */
    i2c_mutex = xSemaphoreCreateMutex();

    /* Initialize all of the thing data structures */
    for(loop = 0; loop <= MAX_THING; loop++)
    {
        iot_data[loop].thingNumber = loop;
        snprintf(iot_data[loop].ip_str, sizeof(iot_data[loop].ip_str), "0.0.0.0");
        iot_data[loop].alert = false;
        iot_data[loop].temp = 0.0;
        iot_data[loop].humidity = 0.0;
        iot_data[loop].light = 0.0;
    }

    /* Get IP Address for MyThing and save in the Thing data structure */
    WIFI_GetIP( ucTempIp );
    snprintf(iot_data[MY_THING].ip_str,
             sizeof(iot_data[MY_THING].ip_str),
             "%d.%d.%d.%d",
             ucTempIp[ 0 ],
             ucTempIp[ 1 ],
             ucTempIp[ 2 ],
             ucTempIp[ 3 ]);

    /* Initialize emWin GUI */
    GUI_Init();

    /* Start threads that interact with the shield (PSoC and OLED) */
    /* Start threads that interact with the shield (PSoC and OLED) */
    xTaskCreate( getCapSenseThread,
                 "CapSense Thread",
                 CAPSENSE_THREAD_STACK_SIZE,
                 0,
                 CAPSENSE_THREAD_PRIORITY,
                 0);
    xTaskCreate( getWeatherDataThread,
                 "Weather Data Thread",
                 WEATHER_DATA_THREAD_STACK_SIZE,
                 0,
                 WEATHER_DATA_THREAD_PRIORITY,
                 0);
    xTaskCreate( displayThread,
                 "Display Thread",
                 DISPLAY_THREAD_STACK_SIZE,
                 0,
                 DISPLAY_THREAD_PRIORITY,
                 0);

     /* Initialize the MQTT libraries required for this demo. */
    InitializeMqtt();

    /* Establish a new MQTT connection. */
    EstablishMqttConnection( awsIotMqttMode,
                             pIdentifier,
                             pNetworkServerInfo,
                             pNetworkCredentialInfo,
                             pNetworkInterface,
                             &mqtt_connection );

    /* Add the topic filter subscriptions used in this demo. */
    ModifySubscriptions( mqtt_connection,
                         IOT_MQTT_SUBSCRIBE,
                         pTopics,
                         pTopicsSize,
                         NULL );

    /* Start the publish thread */
   xTaskCreate( publishThread,
                "Publish Thread",
                PUBLISH_THREAD_STACK_SIZE,
                0,
                PUBLISH_THREAD_PRIORITY,
                0);

     /* Create and start timer to publish weather data every 30 seconds */
    message_timer = xTimerCreate("Timer", MESSAGE_PUBLISH_INTERVAL_MS, true, NULL, publish30sec);
    xTimerStart(message_timer, 0);

    /*Start command thread to display help info on terminal window */
    xTaskCreate( commandThread,
                 "Command Thread",
                 COMMAND_THREAD_STACK_SIZE,
                 0,
                 COMMAND_THREAD_PRIORITY,
                 0);

    /* Configure and setup interrupts for the 2 mechanical buttons */
    cyhal_gpio_init(MECH_BTN1, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    cyhal_gpio_register_callback(MECH_BTN1, alert_button_isr, NULL);
    cyhal_gpio_enable_event(MECH_BTN1, CYHAL_GPIO_IRQ_FALL, 3, true);

    cyhal_gpio_init(MECH_BTN2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    cyhal_gpio_register_callback(MECH_BTN2, publish_button_isr, NULL);
    cyhal_gpio_enable_event(MECH_BTN2, CYHAL_GPIO_IRQ_FALL, 3, true);

    /* Suspend this task to prevent network connection resources from being cleaned up */
    vTaskSuspend( NULL );

    return 0;
}

/*************** Weather Publish Button ISR ***************/
/*
 * Summary: ISR called when publish button is pressed.
 * Current weather state is published.
 *
 *  @param[in] callback_arg argument for the ISR.
 *  @param[in] event Event that triggered the ISR.
 *
 */
void publish_button_isr(void *callback_arg, cyhal_gpio_event_t event)
{
    ( void )callback_arg; /* Suppress compiler warning */
    /* Command pushed onto the queue to determine what to publish */
    char pubCmd[PUBLISH_CMD_SIZE_BYTES];
    pubCmd[0] = WEATHER_CMD;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(0UL != ( CYHAL_GPIO_IRQ_FALL & event))
    {
        /* Push value onto queue*/
        xQueueSendFromISR(pub_queue, pubCmd, &xHigherPriorityTaskWoken);
    }

    /* If xHigherPriorityTaskWoken was set to true you we should yield. */
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*************** Weather Alert Button ISR ***************/
/*
 * Summary: ISR called when alert button is pressed.
 * Weather alert is toggled and published.
 *
 *  @param[in] callback_arg argument for the ISR.
 *  @param[in] event Event that triggered the ISR.
 *
 */
void alert_button_isr(void *callback_arg, cyhal_gpio_event_t event)
{
    ( void )callback_arg; /* Suppress compiler warning */
    /* Command pushed onto the queue to determine what to publish */
    char pubCmd[PUBLISH_CMD_SIZE_BYTES];
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(0UL != ( CYHAL_GPIO_IRQ_FALL & event))
    {
        if(iot_data[MY_THING].alert == true)
         {
            iot_data[MY_THING].alert = false;
         }
         else
         {
             iot_data[MY_THING].alert = true;
         }

        pubCmd[0] = ALERT_CMD;

        /* Publish the alert */
        xQueueSendFromISR(pub_queue, pubCmd, &xHigherPriorityTaskWoken);

        /* Set a semaphore for the OLED to update the display */
        xSemaphoreGiveFromISR(display_semaphore, &xHigherPriorityTaskWoken);
    }

    /* If xHigherPriorityTaskWoken was set to true you we should yield. */
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*************** Timer to publish weather data every 30sec ***************/
/*
 * Summary: Timer callback to publish weather data every 30 secs
 *
 *  @param[in] callback_arg argument for the ISR.
 *  @param[in] event Event that triggered the ISR.
 *
 */
void publish30sec(TimerHandle_t xTimer)
{
    ( void )xTimer; /* Suppress compiler warning */
    /* Command pushed onto the queue to determine what to publish */
    char pubCmd[PUBLISH_CMD_SIZE_BYTES];

    pubCmd[0] = WEATHER_CMD;
    xQueueSend(pub_queue, pubCmd, portMAX_DELAY); /* Push value onto queue*/
}
