/******************************************************************************
* File Name: mqtt_operation.h
*
* Description: This file contains function declarations related to MQTT
* operations.
*
*******************************************************************************
* Copyright (2018-2019), Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* ("Software"), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries ("Cypress") and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, nonexclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress’s integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death (“High Risk Product”). By
* including Cypress’s product in a High Risk Product, the manufacturer of such
* system or application assumes all risk of such use and in doing so agrees to
* indemnify Cypress against all liability.
*******************************************************************************/
#ifndef SOURCE_MQTT_OPERATION_H_
#define SOURCE_MQTT_OPERATION_H_

#include "common_resource.h"

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
                         const uint32_t filterCount,
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
