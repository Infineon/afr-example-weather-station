/******************************************************************************
* File Name: common_resource.h
*
* Description: This file contains resources common to different operations.
*
*******************************************************************************
* Copyright (2018-2019), Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* (“Software”), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries (“Cypress”) and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software (“EULA”).
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
#ifndef SOURCE_COMMON_RESOURCE_H_
#define SOURCE_COMMON_RESOURCE_H_

#include "cyhal.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "iot_mqtt.h"

/***************************************
*            Defines
****************************************/
/*
 * Update this number for the number of the thing that you want to publish to.
 * The default is Thing_00
 */
#define MY_THING                                00

/*
 * The highest number thing on the MQTT Broker that you want to subscribe to.
 * If Things are from 0 to 39 then MAX_THING is 39
 */
#define MAX_THING                               39

/* Structure to hold data from an IoT device */
typedef struct {
    uint8_t thingNumber;
    char ip_str[16];
    bool alert;
    float temp;
    float humidity;
    float light;
} iot_data_t;

/* Publish commands */
typedef enum command {
    WEATHER_CMD,
    TEMPERATURE_CMD,
    HUMIDITY_CMD,
    LIGHT_CMD,
    ALERT_CMD,
    IP_CMD,
    GET_CMD,
} CMD;

/***************************************
*          External variables
****************************************/
extern SemaphoreHandle_t display_semaphore;
extern SemaphoreHandle_t i2c_mutex;
extern QueueHandle_t pub_queue;
extern iot_data_t iot_data[];
extern IotMqttConnection_t mqtt_connection;
extern volatile bool print_all;
extern volatile uint8_t disp_thing;

/***************************************
*          Function definition
****************************************/
void print_thing_info(uint8_t thingNumber);

#endif /* SOURCE_COMMON_RESOURCE_H_ */
