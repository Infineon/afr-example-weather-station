/******************************************************************************
* File Name: afe_shield_operation.c
*
* Description: This file contains threads, functions, and other resources related
* to AFE shield operations.
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
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "GUI.h"
#include "afe_shield_operation.h"
#include "display_interface.h"

/***************************************
*            Defines
****************************************/
/* PSoC I2C device addresses */
#define SHIELD_PSOC_I2C_ADDRESS                 (0x42)

/* I2C Offset registers for CapSense buttons and start of weather data */
#define WEATHER_DATA_OFFSET_REG                 (0x07)
#define TOUCH_BUTTON_OFFSET_REG                 (0x06)

/* CapSense Buttons */
#define TOUCH_BTN0_MASK                         (0x01)
#define TOUCH_BTN1_MASK                         (0x02)
#define TOUCH_BTN2_MASK                         (0x04)
#define TOUCH_BTN3_MASK                         (0x08)
#define ALL_MASK                                (0x0F)
#define CHANGE_IN_THING_NUM_ON_BTN3_PRESS                 (10)

/* Thread delays */
#define WEATHER_DATA_POLLING_PERIOD_MS          (500)
#define CAPSENSE_DATA_POLLING_PERIOD_MS         (100)

/* Strings size to hold the results to print */
#define RESULT_STRING_SIZE                      (30)

/***************************************
*          Global Variables
****************************************/
volatile uint8_t disp_thing = MY_THING;  /* Which thing to display data for, on the OLED */

/*************** Weather Data Acquisition Thread ***************/
/*
 * Summary: Thread to read temperature, humidity, and light from
 * the PSoC analog Co-processor
 *
 *  @param[in] arg argument for the thread
 *
 */
void getWeatherDataThread(void* arg)
{
    ( void )arg; /* Suppress compiler warning */

    /* Weather data from the PSoC Analog Co-processor  */
    struct {
        float temp;
        float humidity;
        float light;
    } __attribute__((packed)) weather_data;

    /* Variables to remember previous values */
    static float tempPrev = 0;
    static float humPrev = 0;
    static float lightPrev = 0;

    /* Buffer to set the offset */
    uint8_t offset = WEATHER_DATA_OFFSET_REG;

    while(1)
    {
        /* Get I2C data - use a Mutex to prevent conflicts */
        xSemaphoreTake( i2c_mutex, portMAX_DELAY);

        /* Set the offset */
        cyhal_i2c_master_write(&afe_shield_i2c_obj,
                               SHIELD_PSOC_I2C_ADDRESS,
                               &offset, sizeof(offset),
                               0,
                               true);

        /* Get data */
        cyhal_i2c_master_read(&afe_shield_i2c_obj,
                              SHIELD_PSOC_I2C_ADDRESS,
                              (uint8_t *)&weather_data,
                              sizeof(weather_data),
                              0,
                              true);

        xSemaphoreGive(i2c_mutex);

        /* Copy weather data into my thing's data structure */
        iot_data[MY_THING].temp =     weather_data.temp;
        iot_data[MY_THING].humidity = weather_data.humidity;
        iot_data[MY_THING].light =    weather_data.light;

        /* Look at weather data - only update display if a value has changed*/
        if((tempPrev != iot_data[MY_THING].temp)    ||
           (humPrev != iot_data[MY_THING].humidity) ||
           (lightPrev != iot_data[MY_THING].light))
        {
            /* Save the new values as previous for next time around */
            tempPrev  = iot_data[MY_THING].temp;
            humPrev   = iot_data[MY_THING].humidity;
            lightPrev = iot_data[MY_THING].light;

            /* Set a semaphore for the OLED to update the display */
            xSemaphoreGive(display_semaphore);
        }

        vTaskDelay(pdMS_TO_TICKS(WEATHER_DATA_POLLING_PERIOD_MS));
    }
}

/*************** CapSense Button Monitor Thread ***************/
/* Summary: Thread to read CapSense button values
 *
 *  @param[in] arg argument for the thread
 *
 */
void getCapSenseThread(void* arg)
{
    ( void )arg; /* Suppress compiler warning */

    uint8_t capSenseValues = 0;
    bool buttonPressed = false;

    /* Buffer to set the offset */
    uint8_t offset = TOUCH_BUTTON_OFFSET_REG;

    while(1)
    {
        /* Get I2C data - use a Mutex to prevent conflicts */
        xSemaphoreTake( i2c_mutex, portMAX_DELAY);

        /* Set the offset */
        cyhal_i2c_master_write(&afe_shield_i2c_obj,
                               SHIELD_PSOC_I2C_ADDRESS,
                               &offset,
                               sizeof(offset),
                               0,
                               true);

        /* Get data */
        cyhal_i2c_master_read(&afe_shield_i2c_obj,
                              SHIELD_PSOC_I2C_ADDRESS,
                              &capSenseValues,
                              sizeof(capSenseValues),
                              0,
                              true);

        xSemaphoreGive(i2c_mutex);

        /* Look for CapSense button presses */
        if(buttonPressed == false) /* Only look for new button presses */
        {
            /* Button 0 goes to the local thing's screen */
            if((capSenseValues & TOUCH_BTN0_MASK) == TOUCH_BTN0_MASK)
            {
                buttonPressed = true;
                disp_thing = MY_THING;
                xSemaphoreGive(display_semaphore);
            }
            /* Button 1 goes to next thing's screen */
            if((capSenseValues & TOUCH_BTN1_MASK) == TOUCH_BTN1_MASK)
            {
                buttonPressed = true;
                if(disp_thing == 0) /* Handle wrap-around case */
                {
                    disp_thing = MAX_THING;
                }
                else
                {
                    disp_thing--;
                }
                xSemaphoreGive(display_semaphore);
            }
            /* Button 2 goes to previous thing's screen */
            if((capSenseValues & TOUCH_BTN2_MASK) == TOUCH_BTN2_MASK)
            {
                buttonPressed = true;
                disp_thing++;
                if(disp_thing > MAX_THING) /* Handle wrap-around case */
                {
                    disp_thing = 0;
                }
                xSemaphoreGive(display_semaphore);
            }
            /* Button 3 increments by 10 things */
            if((capSenseValues & TOUCH_BTN3_MASK) == TOUCH_BTN3_MASK)
            {
                buttonPressed = true;
                disp_thing += CHANGE_IN_THING_NUM_ON_BTN3_PRESS;
                if(disp_thing > MAX_THING) /* Handle wrap-around case */
                {
                    disp_thing -= (MAX_THING + 1);
                }
                xSemaphoreGive(display_semaphore);
            }
        }
        if((capSenseValues & ALL_MASK) == 0) /* All buttons released */
        {
            buttonPressed = false;
        }

         vTaskDelay(pdMS_TO_TICKS(CAPSENSE_DATA_POLLING_PERIOD_MS));
    }
}

/*************** OLED Display Thread ***************/
/*
 * Summary: Thread to display data on the OLED
 *
 *  @param[in] arg argument for the thread
 *
 */
void displayThread(void* arg)
{
    ( void )arg; /* Suppress compiler warning */

    /* Strings to hold the results to print */
    char thing_str[RESULT_STRING_SIZE];
    char temp_str[RESULT_STRING_SIZE];
    char humidity_str[RESULT_STRING_SIZE];
    char light_str[RESULT_STRING_SIZE];

    /* Clear screen, set font size, background color, and text mode */
    GUI_Clear();
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetBkColor(GUI_BLACK);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextMode(GUI_TM_NORMAL);

    while(1)
    {
        /* Wait until new data is ready to display */
        xSemaphoreTake( display_semaphore, portMAX_DELAY);

        /* Set UTF8 character display */
        GUI_UC_SetEncodeUTF8();

        /* Setup Display Strings */
        if(iot_data[disp_thing].alert)
        {
            snprintf(thing_str, sizeof(thing_str),    "Thing_%02d  *ALERT*\n", disp_thing);
        } else {
            snprintf(thing_str, sizeof(thing_str),    "Thing_%02d                \n", disp_thing);
        }
        snprintf(temp_str,      sizeof(temp_str),     "Temp:        %.1f °C  \n", iot_data[disp_thing].temp);
        snprintf(humidity_str,  sizeof(humidity_str), "Humidity:   %.1f %%  \n", iot_data[disp_thing].humidity);
        snprintf(light_str,     sizeof(light_str),    "Light:         %03.0f lx  \n", iot_data[disp_thing].light);

        /* Print data on display - use a mutex to prevent conflict*/
        xSemaphoreTake( i2c_mutex, portMAX_DELAY);
        GUI_GotoXY(0, 0);
        GUI_DispString(thing_str);
        GUI_DispString(iot_data[disp_thing].ip_str);
        GUI_DispString("            \n");
        GUI_DispString(temp_str);
        GUI_DispString(humidity_str);
        GUI_DispString(light_str);
        xSemaphoreGive(i2c_mutex);
    }
}
