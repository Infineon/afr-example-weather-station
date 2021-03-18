/******************************************************************************
* File Name: common_resource.h
*
* Description: This file contains resources common to different operations.
*
*******************************************************************************
* $ Copyright 2020-2021 Cypress Semiconductor $
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
