/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaLCD_EG020THEG1.h
 *
 *  @Copyright      :: Copyright (C) 2015 ASD Tech LTD. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions & Constants for Wintek 4:3 LCD panel WD-F9648W APIs.
 *
 *  @History        ::
 *      Date        Name        Comments
 *      21/07/2015  Keith Wu    Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_LCD_EG020THEG1_H_
#define _AMBA_LCD_EG020THEG1_H_

/*-----------------------------------------------------------------------------------------------*\
 * Defintions for all the screen modes.
\*-----------------------------------------------------------------------------------------------*/
typedef enum _AMBA_LCD_EG020THEG1_SCREEN_MODE_e_ {
    AMBA_LCD_EG020THEG1_SCREEN_MODE_WIDE = 0,         /* Wide Screen Mode */
    AMBA_LCD_EG020THEG1_SCREEN_MODE_NARROW            /* Narrow Screen Mode */
} AMBA_LCD_EG020THEG1_SCREEN_MODE_e;

typedef enum _AMBA_LCD_EG020THEG1_MODE_e_ {
    AMBA_LCD_EG020THEG1_960_240_60HZ = 0,             /* 960x480@60Hz */
    AMBA_LCD_EG020THEG1_960_240_50HZ,                 /* 960x480@50Hz */

    AMBA_LCD_EG020THEG1_NUM_MODE                      /* Number of LCD mode */
} AMBA_LCD_EG020THEG1_MODE_e;

/*-----------------------------------------------------------------------------------------------*\
 * LCD DISPLAY CONFIG
\*-----------------------------------------------------------------------------------------------*/
typedef struct _AMBA_LCD_EG020THEG1_CONFIG_s_ {
    UINT32  Width;          /* Horizontal display resolution of LCD panel */
    UINT32  Height;         /* Vertical display resolution of LCD panel */
    AMBA_DSP_FRAME_RATE_s                   FrameRate;      /* Frame rate */
    AMBA_LCD_EG020THEG1_SCREEN_MODE_e         ScreenMode;     /* Wide screen mode or Narrow screen mode */

    AMBA_DSP_VOUT_PIXEL_CLOCK_OUTPUT_e      DeviceClock;    /* Source clock frequency of LCD device */
    AMBA_DSP_VOUT_DIGITAL_OUTPUT_MODE_e     OutputMode;     /* Digital Output Mode Register */
    AMBA_DSP_VOUT_LCD_COLOR_ORDER_e         EvenLineColor;  /* RGB color sequence of even lines */
    AMBA_DSP_VOUT_LCD_COLOR_ORDER_e         OddLineColor;   /* RGB color sequence of odd lines */

    AMBA_LCD_TIMING_s       VideoTiming;
} AMBA_LCD_EG020THEG1_CONFIG_s;


/*-----------------------------------------------------------------------------------------------*\
 * WD-F9648W LCD Panel Management Structure
\*-----------------------------------------------------------------------------------------------*/
typedef struct _AMBA_LCD_EG020THEG1_CTRL_s_ {
    AMBA_SPI_CONFIG_s                       SpiConfig;  /* SPI control interface */
    AMBA_LCD_EG020THEG1_CONFIG_s              *pDispConfig;
    AMBA_DSP_VOUT_DISPLAY_DIGITAL_CONFIG_s  PixelFormat;
    AMBA_DSP_VOUT_DISPLAY_TIMING_CONFIG_s   DisplayTiming;
} AMBA_LCD_EG020THEG1_CTRL_s;

/*-----------------------------------------------------------------------------------------------*\
 * Defined in AmbaLCD_EG020THEG1.c
\*-----------------------------------------------------------------------------------------------*/

#endif /* _AMBA_LCD_EG020THEG1_H_ */
