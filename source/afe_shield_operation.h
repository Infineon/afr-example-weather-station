/******************************************************************************
* File Name: afe_shield_operation.h
*
* Description: This file contains function declarations related to AFE shield
* operations.
*
*******************************************************************************
* $ Copyright 2020-2021 Cypress Semiconductor $
*******************************************************************************/
#ifndef SOURCE_AFE_SHIELD_OPERATION_H_
#define SOURCE_AFE_SHIELD_OPERATION_H_

#include "common_resource.h"

/* Shield interaction threads */
void getWeatherDataThread(void* arg);
void getCapSenseThread(void* arg);
void displayThread(void* arg);

#endif /* SOURCE_AFE_SHIELD_OPERATION_H_ */
