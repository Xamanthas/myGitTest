/**
  * @file app/peripheral_mod/vout/lcd/t30p61/lcd_t30p61.c
  *
  * Implementation of LCD T30P61 panel interface.
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
#include <AmbaLCD_T30P61.h>
#include <wchar.h>

static int LcdT30P61_Init(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

static int LcdT30P61_GetDispMode(void)
{
    int ReturnValue = -1;
    int VoutSystem = 0;

    VoutSystem = AppLibSysVout_GetSystemType();

    if (VoutSystem == VOUT_SYS_PAL) {
        ReturnValue = AMBA_LCD_T30P61_960_240_50HZ;
    } else {    // VOUT_SYS_NTSC
        ReturnValue = AMBA_LCD_T30P61_960_240_60HZ;
    }

    return ReturnValue;
}

static int LcdT30P61_GetDispAr(void)
{
    return VAR_16x9;
}

static int LcdT30P61_SetMode(int mode)
{
    int ReturnValue = 0;
    return ReturnValue;
}

static int LcdT30P61_GetPipRectLineWidth(void)
{
    return 3;
}

int AppLcd_RegisterT30P61(UINT32 lcdChannelId)
{
    APPLIB_LCD_s Dev = {0};
    WCHAR DevName[] = {'t','3','0','p','6','1','\0'};
    AMBA_LCD_COLOR_BALANCE_s ColorBalance = {0, 0, 0, (0.015625 * 64), (0.015625 * 64), (0.015625 * 64)};
    w_strcpy(Dev.Name, DevName);
    Dev.Enable = 1;
    Dev.ThreeDCapacity = 0;
    Dev.FlipCapacity = 1;
    Dev.ColorbalanceCapacity = 1;
    Dev.BacklightCapacity = 1;//Enable back light setting
    Dev.Rotate = AMP_ROTATE_0;
    Dev.Width = 960;
    Dev.Height = 240;
    Dev.DefaultBrightness = 0;
    Dev.DefaultContrast = 0.015625 * 64;
    memcpy(&Dev.DefaultColorBalance, &ColorBalance, sizeof (AMBA_LCD_COLOR_BALANCE_s));
    Dev.LcdDelayTime = 150;

    Dev.Init = LcdT30P61_Init;
    Dev.GetDispMode = LcdT30P61_GetDispMode;
    Dev.GetDispAR = LcdT30P61_GetDispAr;
    Dev.SetLcdMode = LcdT30P61_SetMode;
    Dev.GetPipRectLineWidth = LcdT30P61_GetPipRectLineWidth;

    AppLibSysLcd_Attach(lcdChannelId, &Dev);

    return 0;
}
