/**
  * @file src/app/peripheral_mod/vout/lcd/eg020theg1/lcd_eg020theg1.c
  *
  * Implementation of LCD EG020THEG1 panel interface.
  *
  * History:
  *    2015/07/21 - [Keith Wu] created file
  *
  * Copyright (C) 2015, ASD Tech LTD.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#include <system/ApplibSys_Lcd.h>
#include <common/common.h>
#include <AmbaSPI.h>
#include <AmbaLCD_EG020THEG1.h>
#include <wchar.h>

static int LcdEG020THEG1_Init(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

static int LcdEG020THEG1_GetDispMode(void)
{
    int ReturnValue = -1;
    int VoutSystem = 0;

    VoutSystem = AppLibSysVout_GetSystemType();

    if (VoutSystem == VOUT_SYS_PAL) {
        ReturnValue = AMBA_LCD_EG020THEG1_960_240_50HZ;
    } else {    // VOUT_SYS_NTSC
        ReturnValue = AMBA_LCD_EG020THEG1_960_240_60HZ;
    }

    return ReturnValue;
}

static int LcdEG020THEG1_GetDispAr(void)
{
    return VAR_4x3;
}

static int LcdEG020THEG1_SetMode(int mode)
{
    int ReturnValue = 0;
    return ReturnValue;
}

static int LcdEG020THEG1_GetPipRectLineWidth(void)
{
    return 3;
}

int AppLcd_RegisterEG020THEG1(UINT32 lcdChannelId)
{
    APPLIB_LCD_s Dev = {0};
    WCHAR DevName[] = {'e','g','0','2','0','t','h','e','g','1','\0'};
    AMBA_LCD_COLOR_BALANCE_s ColorBalance = {0, 0, 0, (0.015625 * 64), (0.015625 * 64), (0.015625 * 64)};
    w_strcpy(Dev.Name, DevName);
    Dev.Enable = 1;
    Dev.ThreeDCapacity = 0;
    Dev.FlipCapacity = 1;
    Dev.ColorbalanceCapacity = 1;
    Dev.BacklightCapacity = 1;// Enable the backlight function
    Dev.Rotate = AMP_ROTATE_0;
    Dev.Width = 640;
    Dev.Height = 240;
    Dev.DefaultBrightness = 0;
    Dev.DefaultContrast = 0.015625 * 64;
    memcpy(&Dev.DefaultColorBalance, &ColorBalance, sizeof (AMBA_LCD_COLOR_BALANCE_s));
    Dev.LcdDelayTime = 100;

    Dev.Init = LcdEG020THEG1_Init;
    Dev.GetDispMode = LcdEG020THEG1_GetDispMode;
    Dev.GetDispAR = LcdEG020THEG1_GetDispAr;
    Dev.SetLcdMode = LcdEG020THEG1_SetMode;
    Dev.GetPipRectLineWidth = LcdEG020THEG1_GetPipRectLineWidth;

    AppLibSysLcd_Attach(lcdChannelId, &Dev);

    return 0;
}
