/**
  * @file src/app/peripheral_mod/vout/lcd/wdf9648w/lcd_wdf9648w.c
  *
  * Implementation of LCD Wdf9648w panel interface.
  *
  * History:
  *    2013/07/17 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#include <system/ApplibSys_Lcd.h>
#include <common/common.h>
#include <AmbaSPI.h>
#include <AmbaLCD_WDF9648W.h>
#include <wchar.h>

static int LcdWdf9648w_Init(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

static int LcdWdf9648w_GetDispMode(void)
{
    int ReturnValue = -1;
    int VoutSystem = 0;

    VoutSystem = AppLibSysVout_GetSystemType();

    if (VoutSystem == VOUT_SYS_PAL) {
        ReturnValue = AMBA_LCD_WDF9648W_960_480_50HZ;
    } else {    // VOUT_SYS_NTSC
        ReturnValue = AMBA_LCD_WDF9648W_960_480_60HZ;
    }

    return ReturnValue;
}

static int LcdWdf9648w_GetDispAr(void)
{
    return VAR_4x3;
}

static int LcdWdf9648w_SetMode(int mode)
{
    int ReturnValue = 0;
    return ReturnValue;
}

static int LcdWdf9648w_GetPipRectLineWidth(void)
{
    return 3;
}

int AppLcd_RegisterWdf9648w(UINT32 lcdChannelId)
{
    APPLIB_LCD_s Dev = {0};
    WCHAR DevName[] = {'w','d','f','9','6','4','8','w','\0'};
    AMBA_LCD_COLOR_BALANCE_s ColorBalance = {0, 0, 0, (0.015625 * 64), (0.015625 * 64), (0.015625 * 64)};
    w_strcpy(Dev.Name, DevName);
    Dev.Enable = 1;
    Dev.ThreeDCapacity = 0;
    Dev.FlipCapacity = 1;
    Dev.ColorbalanceCapacity = 1;
    Dev.BacklightCapacity = 1;//Temp
    Dev.Rotate = AMP_ROTATE_0;
    Dev.Width = 960;
    Dev.Height = 480;
    Dev.DefaultBrightness = 0;
    Dev.DefaultContrast = 0.015625 * 64;
    memcpy(&Dev.DefaultColorBalance, &ColorBalance, sizeof (AMBA_LCD_COLOR_BALANCE_s));
    Dev.LcdDelayTime = 150;

    Dev.Init = LcdWdf9648w_Init;
    Dev.GetDispMode = LcdWdf9648w_GetDispMode;
    Dev.GetDispAR = LcdWdf9648w_GetDispAr;
    Dev.SetLcdMode = LcdWdf9648w_SetMode;
    Dev.GetPipRectLineWidth = LcdWdf9648w_GetPipRectLineWidth;

    AppLibSysLcd_Attach(lcdChannelId, &Dev);

    return 0;
}
