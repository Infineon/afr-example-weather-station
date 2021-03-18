/******************************************************************************
* File Name: i2c_portapi.c
*
* Description: This file contains the I2C port API functions that will be
*               used by emWin to communicate with the OLED display.
*
* Hardware Dependency: CY8CKIT-032 AFE Shield
*
*******************************************************************************
* $ Copyright 2020-2021 Cypress Semiconductor $
*******************************************************************************/

#include "../../cy8ckit_032_oled/driver/i2c_portapi.h"

#include "GUI.h"
#include "cyhal.h"
#include "cybsp.h"
#include "stdlib.h"

/*********************************************************************
*
*       Defines: Configuration
*
**********************************************************************
  Needs to be adapted to custom hardware.
*/
/* I2C port to communicate with the OLED display controller */
cyhal_i2c_t afe_shield_i2c_obj;

/* I2C slave address, Command and Data byte prefixes for the display controller */
#define SHIELD_OLED_I2C_ADDRESS     (0x3C)
#define OLED_CONTROL_BYTE_CMD       (0x00)
#define OLED_CONTROL_BYTE_DATA      (0x40)
#define DISPLAY_SDA                 (CYBSP_I2C_SDA)
#define DISPLAY_SCL                 (CYBSP_I2C_SCL)

/* I2C bus speed */
#define I2C_SPEED_HZ                (400000)

/* Configuration to initialize the I2C block */
cyhal_i2c_cfg_t i2c_config = {
    .is_slave = false,
    .address = SHIELD_OLED_I2C_ADDRESS,
    .frequencyhal_hz = I2C_SPEED_HZ
};

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*******************************************************************************
* Function Name: void I2C_Init(void)
********************************************************************************
*
* Summary: This function initializes I2C interface.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void I2C_Init(void) 
{
    cyhal_i2c_init(&afe_shield_i2c_obj, DISPLAY_SDA, DISPLAY_SCL, NULL);
    cyhal_i2c_configure(&afe_shield_i2c_obj, &i2c_config);
}

/*******************************************************************************
* Function Name: void I2C_WriteCommandByte(unsigned char c) 
********************************************************************************
*
* Summary: This function writes a command byte to the display controller with A0 = 0
*
* Parameters:
*  unsigned char c - command to be written to the display controller
*
* Return:
*  None
*
*******************************************************************************/
void I2C_WriteCommandByte(unsigned char c) 
{
    uint8_t buff[2];
    
    /* The first byte of the buffer tells the display that the following byte
        is a command */
    buff[0] = OLED_CONTROL_BYTE_CMD;
    buff[1] = (char)c;
    
    /* Write the buffer to display controller */
    cyhal_i2c_master_write(&afe_shield_i2c_obj, SHIELD_OLED_I2C_ADDRESS, buff, sizeof(buff), 0, true);
}

/*******************************************************************************
* Function Name: void I2C_WriteDataByte(unsigned char c) 
********************************************************************************
*
* Summary: This function writes a data byte to the display controller with A0 = 1
*
* Parameters:
*  unsigned char c - data to be written to the display controller
*
* Return:
*  None
*
*******************************************************************************/
void I2C_WriteDataByte(unsigned char c) 
{
    uint8_t buff[2];
    
    /* First byte of the buffer tells the display controller that the following byte 
        is a data byte */
    buff[0] = OLED_CONTROL_BYTE_DATA;
    buff[1] = c;

    /* Write the buffer to display controller */
    cyhal_i2c_master_write(&afe_shield_i2c_obj, SHIELD_OLED_I2C_ADDRESS, buff, sizeof(buff), 0, true);
}

/*******************************************************************************
* Function Name: void I2C_WriteDataStream(unsigned char * pData, int NumBytes) 
********************************************************************************
*
* Summary: This function writes multiple data bytes to the display controller with A0 = 1
*
* Parameters:
*  unsigned char *pData - Pointer to the buffer that has data
*  int numBytes - Number of bytes to be written to the display controller
*
* Return:
*  None
*
*******************************************************************************/
void I2C_WriteDataStream(unsigned char * pData, int numBytes) 
{   
    uint8_t* buff = (uint8_t*)malloc(numBytes + 1);
    
    /* Tell the display controller that the following bytes are data bytes */
    buff[0] = OLED_CONTROL_BYTE_DATA;

    /* Copy data bytes to buffer array */
    memcpy(&buff[1], pData, numBytes);

    /* Write all the data bytes to the display controller */
    cyhal_i2c_master_write(&afe_shield_i2c_obj, SHIELD_OLED_I2C_ADDRESS, buff, numBytes + 1, 0, true);
    free(buff);
}

/*******************************************************************************
* Function Name: void I2C_ReadDataStream(unsigned char * pData, int numBytes)  
********************************************************************************
*
* Summary: This function reads multiple data bytes from the display controller with A0 = 1
*
* Parameters:
*  unsigned char *pData - Pointer to the destination buffer
*  int numBytes - Number of bytes to be read from the display controller
*
* Return:
*  None
*
*******************************************************************************/
void I2C_ReadDataStream(unsigned char * pData, int numBytes) 
{
    /* SSD1306 is not readable through i2c. Using cache instead (LCDConf.c, GUIDRV_SPAGE_1C1)*/
}

/*************************** End of file ****************************/
