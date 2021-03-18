/******************************************************************************
* File Name: i2c_portapi.h
*
* Description: This is the header file for i2c_portapi.h.
*
* Hardware Dependency: CY8CKIT-032 AFE Shield
*
*******************************************************************************
* $ Copyright 2020-2021 Cypress Semiconductor $
*******************************************************************************/

#ifndef __I2C_H
#define __I2C_H


/*********************************************************************
*
*       Public routines
*/
void          I2C_Init(void);
void          I2C_WriteCommandByte(unsigned char c);
void          I2C_WriteDataByte(unsigned char c);
void          I2C_WriteDataStream(unsigned char * pData, int NumBytes);
void          I2C_ReadDataStream(unsigned char * pData, int NumBytes);

#endif /* __I2C_H */

/*************************** End of file ****************************/
