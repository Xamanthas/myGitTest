/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaLCD_EG020THEG1.c
 *
 *  @Copyright      :: Copyright (C) 2015 ASD Tech LTD. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of Wintek 4:3 LCD panel WD-F9648W
 *
 *  @History        ::
 *      Date        Name        Comments
 *      21/07/2015  Keith Wu    Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaKAL.h"
#include "AmbaPrint.h"

#include "AmbaPLL.h"
#include "AmbaSPI.h"
#include "AmbaLCD.h"

#include "AmbaGPIO_Def.h"
#include "AmbaDSP_VOUT.h"
#include "AmbaLCD_EG020THEG1.h"
#include "AmbaPrintk.h"


#define LCD_REVERT 1

/*-----------------------------------------------------------------------------------------------*\
 * Global instance for LCD info
\*-----------------------------------------------------------------------------------------------*/
static AMBA_LCD_EG020THEG1_CTRL_s EG020THEG1_Ctrl = {
    .SpiConfig = {
        .SlaveID       = 1,                                 /* Slave ID */
        .ProtocolMode  = AMBA_SPI_CPOL_LOW_CPHA_LOW,      /* SPI Protocol mode */
        .CsPolarity    = AMBA_SPI_CHIP_SELECT_ACTIVE_LOW,   /* Slave select polarity */
        .DataFrameSize = 16,                                /* Data Frame Size in Bit */
        .BaudRate      = 500000                             /* Transfer BaudRate in Hz */
    },
};

/*-----------------------------------------------------------------------------------------------*\
 * EG020THEG1 video mode info
\*-----------------------------------------------------------------------------------------------*/
static AMBA_LCD_EG020THEG1_CONFIG_s EG020THEG1_Config[AMBA_LCD_EG020THEG1_NUM_MODE] = {
    [AMBA_LCD_EG020THEG1_960_240_60HZ] = {
        .Width          = 640,//960,
        .Height         = 240, //240
        .FrameRate      = {
            .Interlace  = 0,
            .TimeScale  = 60000,
            .NumUnitsInTick = 1001,
        },
        .ScreenMode     = AMBA_LCD_EG020THEG1_SCREEN_MODE_WIDE,
        .DeviceClock    = AMBA_DSP_VOUT_PIXEL_CLOCK_FULL_DCLK,  /* clock frequency to LCD */
        .OutputMode     = AMBA_DSP_VOUT_LCD_1COLOR_PER_DOT,     /* pixel format */
#if (LCD_REVERT==1)
        .EvenLineColor  = AMBA_DSP_VOUT_LCD_COLOR_GRB,//AMBA_DSP_VOUT_LCD_COLOR_BGR,//AMBA_DSP_VOUT_LCD_COLOR_BGR,
        .OddLineColor   = AMBA_DSP_VOUT_LCD_COLOR_BGR,//AMBA_DSP_VOUT_LCD_COLOR_GRB,//AMBA_DSP_VOUT_LCD_COLOR_BGR,          /* odd line color order */
#else
        .EvenLineColor  = AMBA_DSP_VOUT_LCD_COLOR_BRG,//AMBA_DSP_VOUT_LCD_COLOR_GBR,//AMBA_DSP_VOUT_LCD_COLOR_BGR,//AMBA_DSP_VOUT_LCD_COLOR_RBG,          /* even line color order */
        .OddLineColor   = AMBA_DSP_VOUT_LCD_COLOR_RGB,//AMBA_DSP_VOUT_LCD_COLOR_BGR,          /* odd line color order */
#endif
        .VideoTiming    = {
            .PixelClock         = 12890400,
            .PixelRepetition    = 1,
            .Htotal             = 820,
            .Vtotal             = 262,
            .HsyncColStart      = 0,
            .HsyncColEnd        = 2,
            .VsyncColStart      = 0,
            .VsyncColEnd        = 0,
            .VsyncRowStart      = 0,
            .VsyncRowEnd        = 5,
            .ActiveColStart     = 116,//98,
            .ActiveColWidth     = 640,//960,
            .ActiveRowStart     = 14,//21,
            .ActiveRowHeight    = 240 //240
        }
    },
    [AMBA_LCD_EG020THEG1_960_240_50HZ] = {
        .Width          = 960,
        .Height         = 240,
        .FrameRate      = {
            .Interlace  = 0,
            .TimeScale  = 50,
            .NumUnitsInTick = 1,
        },
        .ScreenMode     = AMBA_LCD_EG020THEG1_SCREEN_MODE_WIDE,
        .DeviceClock    = AMBA_DSP_VOUT_PIXEL_CLOCK_FULL_DCLK,  /* clock frequency to LCD */
        .OutputMode     = AMBA_DSP_VOUT_LCD_1COLOR_PER_DOT,     /* pixel format */
        .EvenLineColor  = AMBA_DSP_VOUT_LCD_COLOR_RGB,          /* even line color order */
        .OddLineColor   = AMBA_DSP_VOUT_LCD_COLOR_RGB,          /* odd line color order */
        .VideoTiming    = {
            .PixelClock         = 12890400,
            .PixelRepetition    = 1,
            .Htotal             = 820,
            .Vtotal             = 262,
            .HsyncColStart      = 0,
            .HsyncColEnd        = 2,
            .VsyncColStart      = 0,
            .VsyncColEnd        = 0,
            .VsyncRowStart      = 0,
            .VsyncRowEnd        = 5,
            .ActiveColStart     = 116,
            .ActiveColWidth     = 640,
            .ActiveRowStart     = 14,
            .ActiveRowHeight    = 240
        }
    }
};

/*-----------------------------------------------------------------------------------------------*\
 * EG020THEG1 video mode configuration
\*-----------------------------------------------------------------------------------------------*/
static AMBA_DSP_VOUT_ROTATE_TYPE_e EG020THEG1_RotationTable[2][2] = {
    [0] = {
        [0] = AMBA_DSP_VOUT_ROTATE_NORMAL,
        [1] = AMBA_DSP_VOUT_ROTATE_MIRROR_HORIZONTAL
    },
    [1] = {
        [0] = AMBA_DSP_VOUT_ROTATE_FLIP_VERTICAL,
        [1] = AMBA_DSP_VOUT_ROTATE_180_DEGREE
    },
};

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: EG020THEG1_Write
 *
 *  @Description:: Write WD-F9648W register
 *
 *  @Input      ::
 *      Offset:     Register offset
 *      Data:       Register data
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int EG020THEG1_Write(UINT16 Offset, UINT16 Data)
{
    UINT16 SpiCmd = (Offset << 9) | (Data & 0xFF);

    return AmbaSPI_Transfer(AMBA_SPI_MASTER, &EG020THEG1_Ctrl.SpiConfig, 1, &SpiCmd, NULL, 500);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: EG020THEG1_SPI_Transfer
 *
 *  @Description:: Transfer WD-F9648W commands via SPI
 *
 *  @Input      ::
 *      NumSpiCmd:    number of SPI commands
 *      pSpiCmdBuf:   pointer to SPI command buffer
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int EG020THEG1_SPI_Transfer(int NumSpiCmd, void *pSpiCmdBuf)
{
    int RetVal = 0;

    /* Note that each serial command must consist of 16 bits of data */
    RetVal = AmbaSPI_Transfer(AMBA_SPI_MASTER, &EG020THEG1_Ctrl.SpiConfig, NumSpiCmd, pSpiCmdBuf, NULL, 3000);

    if (RetVal == TX_NO_EVENTS) {
        AmbaPrintColor(RED,"EG020THEG1_SPI_Transfer timed out!");
        return NG;
    } else if (RetVal == NG) {
        AmbaPrintColor(RED,"EG020THEG1_SPI_Transfer failed!");
        return NG;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: EG020THEG1_GetContrast
 *
 *  @Description:: Adjust LCD panel contrast
 *
 *  @Input      ::
 *      Gain:   D_contrast = D_in * Gain, 0x40 = 1x gain, (1/256)x per step
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          UINT16: Register value
\*-----------------------------------------------------------------------------------------------*/
static UINT16 EG020THEG1_GetContrast(float Gain)
{
    INT16 RegVal = 0;
#if 0
    /*-----------------------------------------------------------------------*\
     * 00h: 0.75x
     * 40h: 1.0x
     * 7Fh: 1.24609375 = (1 + 63/256)x
    \*-----------------------------------------------------------------------*/
    if (Gain < 0.751953125)
        RegVal = 0x00;
    else if (Gain >= 1.244140625)
        RegVal = 0x7F;
    else
        RegVal = (INT16) (((Gain - 1) + 0.001953125) / 0.00390625) + 0x40;
#endif
    return RegVal;  /* RGB gain of contrast */
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: EG020THEG1_GetBrightness
 *
 *  @Description:: Adjust LCD panel brightness (black level)
 *
 *  @Input      ::
 *      Brightness: D_brightness = D_contrast + Offset, 0x40 = zero offset
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          UINT16: Register value
\*-----------------------------------------------------------------------------------------------*/
static UINT16 EG020THEG1_GetBrightness(INT32 Offset)
{
    INT16 RegVal = 0;
#if 0
    /*-----------------------------------------------------------------------*\
     * 00h: Dark    (-64)
     * 40h: Default (0)
     * 7Fh: Bright  (+63)
    \*-----------------------------------------------------------------------*/
    if (Offset <= -64)
        RegVal = 0x00;
    else if (Offset >= 63)
        RegVal = 0x7F;
    else
        RegVal = (INT16) (0x40 + Offset);
#endif
    return RegVal;  /* Offset of brightness RGB */
}

void AmbaUserGPIO_LCD_SPI_CS(UINT32 LcdCS);
static int EG020THEG1_SPI_Transfer_Data(const UINT16 data)
{
    int RetVal = 0;
    UINT16 SpiCmd = data;

    /* Note that each serial command must consist of 16 bits of data */
		AmbaUserGPIO_LCD_SPI_CS(1);
    RetVal = AmbaSPI_Transfer(AMBA_SPI_MASTER, &EG020THEG1_Ctrl.SpiConfig, 1, &SpiCmd, NULL, 3000);

    if (RetVal == TX_NO_EVENTS) {
        AmbaPrintColor(RED,"EG020THEG1_SPI_Transfer timed out!");
        return NG;
    } else if (RetVal == NG) {
        AmbaPrintColor(RED,"EG020THEG1_SPI_Transfer failed!");
        return NG;
    }
	AmbaPrint("Nick retVal = %d data = %x",RetVal,SpiCmd);
	AmbaUserGPIO_LCD_SPI_CS(0);
    return OK;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: LCD_EG020THEG1Enable
 *
 *  @Description:: Enable LCD panel device
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int LCD_EG020THEG1Enable(void)
{
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: LCD_EG020THEG1Enable
 *
 *  @Description:: Enable LCD panel device
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
void AmbaUserGPIO_LCD_STANDBY(UINT32 LcdStandby);
void AmbaUserGPIO_LCD_Reset(UINT32 LcdReset);
int LCD_EG020THEG1EnableConfig(void)
{
    int RetVal1 = 0, RetVal2 = 0;

    /*-----------------------------------------------------------------------*\
     * Configure LCD registers by SPI control interface.
    \*-----------------------------------------------------------------------*/
	
		AmbaUserGPIO_LCD_STANDBY(1);
	 	AmbaKAL_TaskSleep(20);
		AmbaUserGPIO_LCD_Reset(AMBA_GPIO_LEVEL_HIGH);
	  AmbaKAL_TaskSleep(20);
	  AmbaUserGPIO_LCD_Reset(AMBA_GPIO_LEVEL_LOW);
	  AmbaKAL_TaskSleep(10);
	 	AmbaUserGPIO_LCD_Reset(AMBA_GPIO_LEVEL_HIGH);
		AmbaKAL_TaskSleep(50);

#if (LCD_REVERT==1)
		EG020THEG1_SPI_Transfer_Data(0x100b);
		EG020THEG1_SPI_Transfer_Data(0x080a);
		EG020THEG1_SPI_Transfer_Data(0x0c01);
		EG020THEG1_SPI_Transfer_Data(0x403f);
		EG020THEG1_SPI_Transfer_Data(0x1420);
		EG020THEG1_SPI_Transfer_Data(0x403e);
		EG020THEG1_SPI_Transfer_Data(0x481d);
		EG020THEG1_SPI_Transfer_Data(0x5078);
		EG020THEG1_SPI_Transfer_Data(0x5456);
		EG020THEG1_SPI_Transfer_Data(0x5866);
		EG020THEG1_SPI_Transfer_Data(0x5C67);
		EG020THEG1_SPI_Transfer_Data(0x6008);
		EG020THEG1_SPI_Transfer_Data(0x100f);
		AmbaKAL_TaskSleep(200);

#else	
	EG020THEG1_SPI_Transfer_Data(0x100b);
	EG020THEG1_SPI_Transfer_Data(0x080a);
	EG020THEG1_SPI_Transfer_Data(0x0c01);
	EG020THEG1_SPI_Transfer_Data(0x403f);
	EG020THEG1_SPI_Transfer_Data(0x481d);
	EG020THEG1_SPI_Transfer_Data(0x5078);
	EG020THEG1_SPI_Transfer_Data(0x5456);
	EG020THEG1_SPI_Transfer_Data(0x5866);
	EG020THEG1_SPI_Transfer_Data(0x5C67);
	EG020THEG1_SPI_Transfer_Data(0x6008);
	EG020THEG1_SPI_Transfer_Data(0x100f);
	AmbaKAL_TaskSleep(200);
#endif
	
    if (RetVal1 == NG || RetVal2 == NG)
        return NG;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: LCD_EG020THEG1Disable
 *
 *  @Description:: Disable LCD panel device
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int LCD_EG020THEG1Disable(void)
{
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: LCD_EG020THEG1GetInfo
 *
 *  @Description:: Get vout configuration for current LCD display mode
 *
 *  @Input      :: none
 *
 *  @Output     ::
 *      pInfo:      pointer to LCD display mode info
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int LCD_EG020THEG1GetInfo(AMBA_LCD_INFO_s *pInfo)
{
    if (EG020THEG1_Ctrl.pDispConfig == NULL)
        return NG;

    pInfo->Width = EG020THEG1_Ctrl.pDispConfig->Width;
    pInfo->Height = EG020THEG1_Ctrl.pDispConfig->Height;
    pInfo->AspectRatio.X = 4;
    pInfo->AspectRatio.Y = 3;
    memcpy(&pInfo->FrameRate, &EG020THEG1_Ctrl.pDispConfig->FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: LCD_EG020THEG1Config
 *
 *  @Description:: Configure LCD display mode
 *
 *  @Input      ::
 *      pLcdConfig: configuration of LCD display mode
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int LCD_EG020THEG1Config(AMBA_LCD_MODE_ID_u Mode)
{
    AMBA_DSP_VOUT_DISPLAY_DIGITAL_CONFIG_s *pPixelFormat = &EG020THEG1_Ctrl.PixelFormat;
    AMBA_DSP_VOUT_DISPLAY_TIMING_CONFIG_s *pDisplayTiming = &EG020THEG1_Ctrl.DisplayTiming;
    AMBA_LCD_EG020THEG1_CONFIG_s *pDispConfig;

    if (Mode.Bits.Mode >= AMBA_LCD_EG020THEG1_NUM_MODE)
        return NG;


    pDispConfig = EG020THEG1_Ctrl.pDispConfig = &EG020THEG1_Config[Mode.Bits.Mode];

    pPixelFormat->DeviceClock = pDispConfig->DeviceClock;
    pPixelFormat->OutputMode = pDispConfig->OutputMode;
    pPixelFormat->ClkSampleEdge = AMBA_DSP_VOUT_ACTIVE_HIGH;
    pPixelFormat->FrameValidPolarity = AMBA_DSP_VOUT_ACTIVE_HIGH;
    pPixelFormat->LineValidPolarity = AMBA_DSP_VOUT_ACTIVE_HIGH;
    pPixelFormat->FrameSyncPolarity = AMBA_DSP_VOUT_ACTIVE_LOW;
    pPixelFormat->LineSyncPolarity = AMBA_DSP_VOUT_ACTIVE_LOW;

    pPixelFormat->SyncCtrl.HSyncColStart = pDispConfig->VideoTiming.HsyncColStart;
    pPixelFormat->SyncCtrl.HSyncColEnd = pDispConfig->VideoTiming.HsyncColEnd;
    pPixelFormat->SyncCtrl.VSyncColStart = pDispConfig->VideoTiming.VsyncColStart;
    pPixelFormat->SyncCtrl.VSyncColEnd = pDispConfig->VideoTiming.VsyncColEnd;
    pPixelFormat->SyncCtrl.VSyncRowStart = pDispConfig->VideoTiming.VsyncRowStart;
    pPixelFormat->SyncCtrl.VSyncRowEnd = pDispConfig->VideoTiming.VsyncRowEnd;
    pPixelFormat->Interlace = 0;
    pPixelFormat->RowTime = pDispConfig->VideoTiming.Htotal;

    if (pDispConfig->OutputMode == AMBA_DSP_VOUT_LCD_3COLORS_DUMMY_PER_DOT) {
        pPixelFormat->EvenLineColor   = pDispConfig->EvenLineColor;
        pPixelFormat->OddLineColor    = pDispConfig->OddLineColor;
    } else if (Mode.Bits.FlipHorizontal) {
        pPixelFormat->EvenLineColor   = (AMBA_DSP_VOUT_LCD_COLOR_ORDER_e)(pDispConfig->OddLineColor ^ 0x1);
        pPixelFormat->OddLineColor    = (AMBA_DSP_VOUT_LCD_COLOR_ORDER_e)(pDispConfig->EvenLineColor ^ 0x1);
    } else {
        pPixelFormat->EvenLineColor   = pDispConfig->OddLineColor;
        pPixelFormat->OddLineColor    = pDispConfig->EvenLineColor;
    }

    pDisplayTiming->Rotation = EG020THEG1_RotationTable[Mode.Bits.FlipVertical][Mode.Bits.FlipHorizontal];
    pDisplayTiming->PixelClock = pDispConfig->VideoTiming.PixelClock;
    pDisplayTiming->Interlace = 0;
    pDisplayTiming->FrameWidth = pDispConfig->VideoTiming.Htotal * pDispConfig->VideoTiming.PixelRepetition;
    pDisplayTiming->FrameHeight = pDispConfig->VideoTiming.Vtotal;
    pDisplayTiming->FrameActiveColStart = pDispConfig->VideoTiming.ActiveColStart;
    pDisplayTiming->FrameActiveColWidth = pDispConfig->VideoTiming.ActiveColWidth;
    pDisplayTiming->FrameActiveRowStart = pDispConfig->VideoTiming.ActiveRowStart;
    pDisplayTiming->FrameActiveRowHeight = pDispConfig->VideoTiming.ActiveRowHeight;

    AmbaPLL_SetVoutLcdClk(pDispConfig->VideoTiming.PixelClock);
    AmbaKAL_TaskSleep(10);
    LCD_EG020THEG1EnableConfig();
    AmbaKAL_TaskSleep(100);
    AmbaDSP_VoutDisplayTimingSetup(AMBA_DSP_VOUT_LCD, pDisplayTiming);
    AmbaDSP_VoutDisplayDigitalSetup(pPixelFormat);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: LCD_EG020THEG1SetBacklight
 *
 *  @Description:: Turn LCD Backlight On/Off
 *
 *  @Input      ::
 *      EnableFlag: 1 = Backlight On, 0 = Backlight Off
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int LCD_EG020THEG1SetBacklight(INT32 EnableFlag)
{
    extern void AmbaUserGPIO_LcdCtrl(UINT32 LcdFlag);
    AmbaUserGPIO_LcdCtrl(EnableFlag);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: LCD_EG020THEG1SetBrightness
 *
 *  @Description:: Adjust LCD panel brightness (black level)
 *
 *  @Input      ::
 *      Offset: D_brightness = D_contrast + Offset, 0x40 = zero offset
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int LCD_EG020THEG1SetBrightness(INT32 Offset)
{
#if 0
    UINT16 RegVal;

    /*-----------------------------------------------------------------------*\
     * 00h: Dark    (-64)
     * 40h: Default (0)
     * 80h: Bright  (+64)
     * C0h: Bright  (+128)
     * FFh: Bright  (+191)
    \*-----------------------------------------------------------------------*/
    if (Offset <= -64)
        RegVal = 0x00;
    else if (Offset >= 191)
        RegVal = 0xFF;
    else
        RegVal = (UINT16) (0x40 + Offset);

    return EG020THEG1_Write(EG020THEG1_REG_R0AH, RegVal);    /* Offset of brightness RGB */
#else
    return 0;
#endif
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: LCD_EG020THEG1SetContrast
 *
 *  @Description:: Adjust LCD panel contrast
 *
 *  @Input      ::
 *      Gain:   D_contrast = D_in * Gain, 0x40 = 1x gain, (1/64)x per step
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int LCD_EG020THEG1SetContrast(float Gain)
{
#if 0
    UINT16 RegVal;

    /*-----------------------------------------------------------------------*\
     * 00h: 0.0x
     * 40h: 1.0x
     * 80h: 2.0x
     * C0h: 3.0x
     * FFh: 3.984375x = (255/64)x
    \*-----------------------------------------------------------------------*/
    if (Gain < 0.0078125)
        RegVal = 0x00;
    else if (Gain > 3.9765625)
        RegVal = 0xFF;
    else
        RegVal = (UINT16) ((Gain + 0.0078125) / 0.015625);

    return EG020THEG1_Write(EG020THEG1_REG_R0BH, RegVal);    /* RGB gain of contrast */
#else
    return 0;
#endif
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: LCD_EG020THEG1SetColorBalance
 *
 *  @Description:: Adjust LCD color balance
 *
 *  @Input      ::
 *      ColorBalance:   Gains and Offsets of each color channel
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int LCD_EG020THEG1SetColorBalance(AMBA_LCD_COLOR_BALANCE_s *pColorBalance)
{
#if 0
    UINT16 RegVal;

    if (pColorBalance == NULL)
        return NG;

    RegVal = EG020THEG1_GetContrast(pColorBalance->GainRed);
    EG020THEG1_Write(EG020THEG1_REG_R0CH, RegVal);    /* R gain of contrast */
    RegVal = EG020THEG1_GetBrightness(pColorBalance->OffsetRed);
    EG020THEG1_Write(EG020THEG1_REG_R0DH, RegVal);    /* R gain of brightness */

    RegVal = EG020THEG1_GetContrast(pColorBalance->GainBlue);
    EG020THEG1_Write(EG020THEG1_REG_R0EH, RegVal);    /* B gain of contrast */
    RegVal = EG020THEG1_GetBrightness(pColorBalance->OffsetBlue);
    EG020THEG1_Write(EG020THEG1_REG_R0FH, RegVal);    /* B gain of brightness */

    return OK;
#else
    return 0;
#endif
}

/*-----------------------------------------------------------------------------------------------*\
 * LCD driver object
\*-----------------------------------------------------------------------------------------------*/
AMBA_LCD_OBJECT_s AmbaLCD_EG020THEG1Obj = {
    .LcdEnable = LCD_EG020THEG1Enable,
    .LcdDisable = LCD_EG020THEG1Disable,
    .LcdGetInfo = LCD_EG020THEG1GetInfo,
    .LcdConfig = LCD_EG020THEG1Config,
    .LcdSetBacklight = LCD_EG020THEG1SetBacklight,
    .LcdSetBrightness = LCD_EG020THEG1SetBrightness,
    .LcdSetContrast = LCD_EG020THEG1SetContrast,
    .LcdSetColorBalance = LCD_EG020THEG1SetColorBalance,

    .pName = "EG020THEG1"
};
