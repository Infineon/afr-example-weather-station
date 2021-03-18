/******************************************************************************
* File Name: console_operation.c
*
* Description: This file contains threads, functions, and other resources related
* to UART console operations.
*
*******************************************************************************
* $ Copyright 2020-2021 Cypress Semiconductor $
*******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "cy_retarget_io.h"
#include "console_operation.h"

/***************************************
*            Defines
****************************************/
/* Publish command size */
#define PUBLISH_CMD_SIZE_BYTES                  (4)

/* UART callback event priority */
#define UART_EVENT_CALLBACK_PRIORITY            (3)

/* Delay between printing info of all things */
#define DELAY_BETWEEN_PRINT_MS                  (20)

/***************************************
*          Global Variables
****************************************/
static SemaphoreHandle_t command_semaphore;

/* Buffer to store command received from UART terminal */
static volatile uint8_t receive_command;

/* Flag to print updates from all things to UART when true */
volatile bool print_all = false;

/***************************************
*          Forward Declaration
****************************************/
void command_thread_callback(void *callback_arg, cyhal_uart_event_t event);
void print_banner(void);
/*************** UART Command Interface Thread ***************/
/*
 * Summary: Thread to handle UART command input/output
 *
 *  @param[in] arg argument for the thread
 *
 */
void commandThread(void* arg)
{
    ( void )arg; /* Suppress compiler warning */

    uint8_t loop;

    /* Command pushed onto the queue to determine what to publish */
    char pubCmd[PUBLISH_CMD_SIZE_BYTES];

    /* Setup Thread Control entities */
    command_semaphore = xSemaphoreCreateBinary();

    /* Register callback to trigger when RX FIFO is not empty */
    cyhal_uart_register_callback(&cy_retarget_io_uart_obj, command_thread_callback, NULL);
    cyhal_uart_enable_event(&cy_retarget_io_uart_obj,
                            CYHAL_UART_IRQ_RX_NOT_EMPTY,
                            UART_EVENT_CALLBACK_PRIORITY,
                            true);

    print_banner();

    while(1)
    {
        /* Wait for semaphore to be given from UART ISR */
        xSemaphoreTake(command_semaphore, portMAX_DELAY);

        /* If we get here then a character has been received */
        switch(receive_command)
        {
        case '?':
            configPRINTF(("Commands:\r\n"));
            configPRINTF(("\tt - Print temperature and publish\r\n"));
            configPRINTF(("\th - Print humidity and publish\r\n"));
            configPRINTF(("\tl - Print light value and publish\r\n"));
            configPRINTF(("\tA - Publish weather alert ON\r\n"));
            configPRINTF(("\ta - Publish weather alert OFF\r\n"));
            configPRINTF(("\tP - Turn printing of messages from all things ON\r\n"));
            configPRINTF(("\tp - Turn printing of messages from all things OFF\r\n"));
            configPRINTF(("\tx - Print the current known state of the data from all things\r\n"));
            configPRINTF(("\tc - Clear the terminal and set the cursor to the upper left corner\r\n"));
            configPRINTF(("\t? - Print the list of commands\r\n"));
            break;
        case 't': /* Print temperature to terminal and publish */
            configPRINTF(("Temperature: %.1f\r\n", iot_data[MY_THING].temp));
            /* Publish temperature to the cloud */
            pubCmd[0] = TEMPERATURE_CMD;
            xQueueSend(pub_queue, pubCmd, portMAX_DELAY); /* Push value onto queue*/
            break;
        case 'h': /* Print humidity to terminal and publish */
            configPRINTF(("Humidity: %.1f\t\r\n", iot_data[MY_THING].humidity));
            /* Publish humidity to the cloud */
            pubCmd[0] = HUMIDITY_CMD;
            xQueueSend(pub_queue, pubCmd, portMAX_DELAY); /* Push value onto queue*/
            break;
        case 'l': /* Print light value to terminal and publish */
            configPRINTF(("Light: %.1f\t\r\n", iot_data[MY_THING].light));
            /* Publish light value to the cloud */
            pubCmd[0] = LIGHT_CMD;
            xQueueSend(pub_queue, pubCmd, portMAX_DELAY); /* Push value onto queue*/
            break;
        case 'A': /* Publish Weather Alert ON */
            configPRINTF(("Weather Alert ON\r\n"));
            iot_data[MY_THING].alert = true;
            xSemaphoreGive(display_semaphore); /* Update display */
            pubCmd[0] = ALERT_CMD;
            xQueueSend(pub_queue, pubCmd, portMAX_DELAY); /* Push value onto queue*/
            break;
        case 'a': /* Publish Weather Alert OFF */
            configPRINTF(("Weather Alert OFF\r\n"));
            iot_data[MY_THING].alert = false;
            xSemaphoreGive(display_semaphore); /* Update display */
            pubCmd[0] = ALERT_CMD;
            xQueueSend(pub_queue, pubCmd, portMAX_DELAY); /* Push value onto queue*/
            break;
        case 'P': /* Turn on printing of updates to all things */
            configPRINTF(("Thing Updates ON\r\n"));
            print_all = true;
            break;
        case 'p': /* Turn off printing of updates to all things */
            configPRINTF(("Thing Updates OFF\r\n"));
            print_all = false;
            break;
        case 'x': /* Print current state of all things */
            for(loop = 0; loop <= MAX_THING; loop++)
            {
                print_thing_info(loop);
            }
            break;
        case 'c':
            print_banner();
            break;
        }
    }
}

/*************** UART Event Callback ***************/
/*
 * Summary: Callback to handle UART event
 *
 *  @param[in] callback_arg argument for the ISR.
 *  @param[in] event Event that triggered the ISR.
 *
 */
void command_thread_callback(void *callback_arg, cyhal_uart_event_t event)
{
    ( void )callback_arg; /* Suppress compiler warning */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if(0UL != (CYHAL_UART_IRQ_RX_NOT_EMPTY & event))
    {
        cyhal_uart_getc(&cy_retarget_io_uart_obj, (uint8_t*)&receive_command, 0);
        xSemaphoreGiveFromISR(command_semaphore,&xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*************** Print Thing Info ***************/
/*
 * Summary: Print information for the given thing
 *
 *  @param[in] thingNumber The number of the Thing
 *  whose details are to be printed.
 */
void print_thing_info(uint8_t thingNumber)
{
    configPRINTF(("\tThing: Thing_%02d\tIP: %15s\tAlert: %d\tTemperature: %4.1f\tHumidity: %4.1f\tLight: %5.0f\r\n",
                   thingNumber,
                   iot_data[thingNumber].ip_str,
                   iot_data[thingNumber].alert,
                   iot_data[thingNumber].temp,
                   iot_data[thingNumber].humidity,
                   iot_data[thingNumber].light));

    /* Delay to avoid the overflow of the print queue */
    vTaskDelay(pdMS_TO_TICKS(DELAY_BETWEEN_PRINT_MS));
}

/*************** Print Banner ***************/
/*
 * Summary: Prints a banner for the command thread.
 */
void print_banner(void)
{
    /* Send VT100 clear screen code ESC[2J and move cursor to upper left corner with ESC[H */
    configPRINTF(("\x1b[2J\x1b[H"));
    configPRINTF(("******************************************\r\n"));
    configPRINTF(("Enter '?' for a list of available commands\r\n"));
    configPRINTF(("******************************************\r\n"));
}
