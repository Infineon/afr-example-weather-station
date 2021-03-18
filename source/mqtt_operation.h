/******************************************************************************
* File Name: mqtt_operation.h
*
* Description: This file contains function declarations related to MQTT
* operations.
*
*******************************************************************************
* $ Copyright 2020-2021 Cypress Semiconductor $
*******************************************************************************/
#ifndef SOURCE_MQTT_OPERATION_H_
#define SOURCE_MQTT_OPERATION_H_

#include "common_resource.h"

/* MQTT Broker info */
#define TOPIC_FILTER_COUNT                      (2)

/***************************************
*      Function Declarations
****************************************/

/* MQTT functions */
int InitializeMqtt(void);
int EstablishMqttConnection( bool awsIotMqttMode,
                             const char * pIdentifier,
                             void * pNetworkServerInfo,
                             void * pNetworkCredentialInfo,
                             const IotNetworkInterface_t * pNetworkInterface,
                             IotMqttConnection_t * pMqttConnection );
int ModifySubscriptions( IotMqttConnection_t mqttConnection,
                         IotMqttOperationType_t operation,
                         const char ** pTopicFilters,
                         const uint16_t *pTopicFilterLength,
                         void * pCallbackParameter );
int PublishMessage( IotMqttConnection_t mqttConnection,
                    char* topic,
                    uint16_t topicLength,
                    char* mqttMessage,
                    uint16_t messageLength);
void MqttSubscriptionCallback( void * param1,
                               IotMqttCallbackParam_t * const pPublish );

/* MQTT Thread */
void publishThread(void* arg);

#endif /* SOURCE_MQTT_OPERATION_H_ */
