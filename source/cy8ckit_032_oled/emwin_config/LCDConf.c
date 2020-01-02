/*********************************************************************
*                SEGGER Microcontroller GmbH                         *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2018  SEGGER Microcontroller GmbH                *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V5.48 - Graphical user interface for embedded applications **
All  Intellectual Property rights  in the Software belongs to  SEGGER.
emWin is protected by  international copyright laws.  Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with the following terms:

The software  has been licensed to  Cypress Semiconductor Corporation,
whose registered  office is situated  at 198 Champion Ct. San Jose, CA 
95134 USA  solely for the  purposes of creating  libraries for Cypress
PSoC3 and  PSoC5 processor-based devices,  sublicensed and distributed
under  the  terms  and  conditions  of  the  Cypress  End User License
Agreement.
Full source code is available at: www.segger.com

We appreciate your understanding and fairness.
----------------------------------------------------------------------
Licensing information
Licensor:                 SEGGER Microcontroller Systems LLC
Licensed to:              Cypress Semiconductor Corp, 198 Champion Ct., San Jose, CA 95134, USA
Licensed SEGGER software: emWin
License number:           GUI-00319
License model:            Services and License Agreement, signed June 10th, 2009
Licensed platform:        Any Cypress platform (Initial targets are: PSoC3, PSoC5)
----------------------------------------------------------------------
Support and Update Agreement (SUA)
SUA period:               2009-06-12 - 2022-07-27
Contact to extend SUA:    sales@segger.com
----------------------------------------------------------------------
File        : LCDConf.c
Purpose     : Display controller configuration (single layer)
---------------------------END-OF-HEADER------------------------------
*/

#include "i2c_portapi.h"
#include "GUI.h"
#include "GUIDRV_SPage.h"


/*********************************************************************
*
*       Layer configuration
*
**********************************************************************
*/
//
// Physical display size
//
#define XSIZE_PHYS 128
#define YSIZE_PHYS 64

//
// Color conversion
//
#define COLOR_CONVERSION GUICC_1

//
// Display driver, display data cache enabled
//
#define DISPLAY_DRIVER GUIDRV_SPAGE_1C1

/*********************************************************************
*
*       Configuration checking
*
**********************************************************************
*/
#ifndef   VXSIZE_PHYS
  #define VXSIZE_PHYS XSIZE_PHYS
#endif
#ifndef   VYSIZE_PHYS
  #define VYSIZE_PHYS YSIZE_PHYS
#endif
#ifndef   XSIZE_PHYS
  #error Physical X size of display is not defined!
#endif
#ifndef   YSIZE_PHYS
  #error Physical Y size of display is not defined!
#endif
#ifndef   COLOR_CONVERSION
  #error Color conversion not defined!
#endif
#ifndef   DISPLAY_DRIVER
  #error No display driver defined!
#endif

/*********************************************************************
*
*       Defines: Configuration
*
**********************************************************************/
/* OLED display controller register addresses */
#define OLED_SETCONTRAST             0x81
#define OLED_DISPLAYALLON_RESUME     0xA4
#define OLED_DISPLAYALLON            0xA5
#define OLED_NORMALDISPLAY           0xA6
#define OLED_INVERTDISPLAY           0xA7
#define OLED_DISPLAYOFF              0xAE
#define OLED_DISPLAYON               0xAF
#define OLED_SETDISPLAYOFFSET        0xD3
#define OLED_SETCOMPINS              0xDA
#define OLED_SETVCOMDETECT           0xDB
#define OLED_SETDISPLAYCLOCKDIV      0xD5
#define OLED_SETPRECHARGE            0xD9
#define OLED_SETMULTIPLEX            0xA8
#define OLED_SETLOWCOLUMN            0x00
#define OLED_SETHIGHCOLUMN           0x10
#define OLED_SETSTARTLINE            0x40
#define OLED_MEMORYMODE              0x20
#define OLED_COLUMNADDR              0x21
#define OLED_PAGEADDR                0x22
#define OLED_COMSCANINC              0xC0
#define OLED_COMSCANDEC              0xC8
#define OLED_SEGREMAP                0xA0
#define OLED_CHARGEPUMP              0x8D
#define OLED_EXTERNALVCC             0x01
#define OLED_SWITCHCAPVCC            0x02

/* Scrolling #defines */ 
#define OLED_ACTIVATE_SCROLL                         0x2F
#define OLED_DEACTIVATE_SCROLL                       0x2E
#define OLED_SET_VERTICAL_SCROLL_AREA                0xA3
#define OLED_RIGHT_HORIZONTAL_SCROLL                 0x26
#define OLED_LEFT_HORIZONTAL_SCROLL                  0x27
#define OLED_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL    0x29
#define OLED_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL     0x2A


/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _InitController
*
* Purpose:
*   Initializes the display controller
*/
static void _InitController(void) 
{
    I2C_Init();
  
    I2C_WriteCommandByte(OLED_DISPLAYOFF);            // 0xAE
    I2C_WriteCommandByte(OLED_SETDISPLAYCLOCKDIV);    // 0xD5
    I2C_WriteCommandByte(0xF0);                       // the suggested ratio 0x80

    I2C_WriteCommandByte(OLED_SETMULTIPLEX);          // 0xA8
    I2C_WriteCommandByte(YSIZE_PHYS - 1);

    I2C_WriteCommandByte(OLED_SETDISPLAYOFFSET);      // 0xD3
    I2C_WriteCommandByte(0x00);                       // no offset
    I2C_WriteCommandByte(OLED_SETSTARTLINE | 0x0);    // line #0
    I2C_WriteCommandByte(OLED_CHARGEPUMP);            // 0x8D
    I2C_WriteCommandByte(0x14);
    I2C_WriteCommandByte(OLED_MEMORYMODE);            // 0x20
    I2C_WriteCommandByte(0x02);                       // Horizontal addressing mode (A[1:0]=00b) <-
                                             // Vertical addressing mode: (A[1:0]=01b)
                                             // Page addressing mode (A[1:0]=10b)
    I2C_WriteCommandByte(OLED_SEGREMAP | 0x1);
    I2C_WriteCommandByte(OLED_COMSCANDEC);

    I2C_WriteCommandByte(OLED_SETCOMPINS);            // 0xDA
    
    #if YSIZE_PHYS == 64
        I2C_WriteCommandByte(0x12);
    #elif YSIZE_PHYS == 32
        I2C_WriteCommandByte(0x02);
    #endif
    
    I2C_WriteCommandByte(OLED_SETCONTRAST);           // 0x81
    I2C_WriteCommandByte(0x8F);

    I2C_WriteCommandByte(OLED_SETPRECHARGE);          // 0xd9
    I2C_WriteCommandByte(0xF1);
    I2C_WriteCommandByte(OLED_SETVCOMDETECT);         // 0xDB
    I2C_WriteCommandByte(0x20);
    I2C_WriteCommandByte(OLED_DISPLAYALLON_RESUME);   // 0xA4
    I2C_WriteCommandByte(OLED_NORMALDISPLAY);         // 0xA6

    I2C_WriteCommandByte(OLED_DEACTIVATE_SCROLL);

    I2C_WriteCommandByte(OLED_DISPLAYON);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       LCD_X_Config
*
* Purpose:
*   Called during the initialization process in order to set up the
*   display driver configuration.
*   
*/
void LCD_X_Config(void) 
{
    CONFIG_SPAGE Config = {0};
    GUI_DEVICE * pDevice;
    GUI_PORT_API PortAPI = {0};

    //
    // Set display driver and color conversion for 1st layer
    //
    pDevice = GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER, COLOR_CONVERSION, 0, 0);
    //
    // Display size configuration
    //
    LCD_SetSizeEx (0, XSIZE_PHYS,   YSIZE_PHYS);
    LCD_SetVSizeEx(0, VXSIZE_PHYS,  VYSIZE_PHYS);
    //
    // Driver configuration
    //
    Config.FirstSEG = 0;
    Config.FirstCOM = 0;
    GUIDRV_SPage_Config(pDevice, &Config);
    //
    // Configure hardware routines
    //
    PortAPI.pfWrite8_A0  = I2C_WriteCommandByte;
    PortAPI.pfWrite8_A1  = I2C_WriteDataByte;
    PortAPI.pfWriteM8_A1 = I2C_WriteDataStream;
    
    /* SSD1306 is not readable through i2c. Cache is enabled 
    to use display without data read operations*/
    
    GUIDRV_SPage_SetBus8(pDevice, &PortAPI);
    //
    // Controller configuration
    //
    GUIDRV_SPage_Set1510(pDevice);
}

/*********************************************************************
*
*       LCD_X_DisplayDriver
*
* Purpose:
*   This function is called by the display driver for several purposes.
*   To support the according task the routine needs to be adapted to
*   the display controller. Please note that the commands marked with
*   'optional' are not cogently required and should only be adapted if 
*   the display controller supports these features.
*
* Parameter:
*   LayerIndex - Index of layer to be configured
*   Cmd        - Please refer to the details in the switch statement below
*   pData      - Pointer to a LCD_X_DATA structure
*/
int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * pData) {
  int r;

  GUI_USE_PARA(LayerIndex);
  GUI_USE_PARA(pData);
  switch (Cmd) 
    {
    case LCD_X_INITCONTROLLER:
        //
        // Called during the initialization process in order to set up the
        // display controller and put it into operation. If the display
        // controller is not initialized by any external routine this needs
        // to be adapted by the customer...
        //
        _InitController();
        r = 0;
        break;

    case LCD_X_ON: 
        I2C_WriteCommandByte(OLED_DISPLAYON);
        r = 0;
        break;
        
    case LCD_X_OFF: 
        I2C_WriteCommandByte(OLED_DISPLAYOFF);
        r = 0;
        break;

    default:
        r = -1;
        break;
  }
  return r;
}

/*************************** End of file ****************************/
