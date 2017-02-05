/**
  * @file src/app/apps/gui/pb/connectedcam/gui_utility.c
  *
  *  Implementation of GUI display utility
  *
  * History:
  *    2014/02/14 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#include <apps/gui/utility/gui_utility.h>
#include <apps/gui/resource/gui_resource.h>
#include <apps/gui/resource/gui_settle.h>
#include <wchar.h>

typedef struct _GUI_CTRL_FLAGS_s_ {
    UINT32 Power;
#define GUI_FLAGS_POWER_SHOWED    (0x01)
#define GUI_FLAGS_POWER_DC_IN    (0x02)
} GUI_CTRL_FLAGS_s;

static GUI_CTRL_FLAGS_s GuiCtrlFlags = {0};

int AppGuiUtil_PowerIconShow(UINT32 param1, UINT32 param2)
{
    if (param1 == GUI_POWER_ADAPTER) {
        APP_ADDFLAGS(GuiCtrlFlags.Power, GUI_FLAGS_POWER_DC_IN);
    } else {
        APP_REMOVEFLAGS(GuiCtrlFlags.Power, GUI_FLAGS_POWER_SHOWED);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_BATTERY);
    }
    if (APP_CHECKFLAGS(GuiCtrlFlags.Power, GUI_FLAGS_POWER_DC_IN)) {
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_DC_PLUG);
    }
    return 0;
}

int AppGuiUtil_PowerIconHide(UINT32 param1, UINT32 param2)
{
    if (!APP_CHECKFLAGS(param1, GUI_HIDE_POWER_EXCEPT_BAT)) {
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_BATTERY);
    }
    if (!APP_CHECKFLAGS(param1, GUI_HIDE_POWER_EXCEPT_DC)) {
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_DC_PLUG);
        APP_REMOVEFLAGS(GuiCtrlFlags.Power, GUI_FLAGS_POWER_DC_IN);
    }
    APP_REMOVEFLAGS(GuiCtrlFlags.Power, GUI_FLAGS_POWER_SHOWED);
    return 0;
}

int AppGuiUtil_PowerIconUpdate(int PowerType, int batteryState)
{
    switch (batteryState) {
    default:
    case GUI_BATTERY_NONE:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_BATTERY, BMP_0_NULL);
        break;
    case GUI_BATTERY_EMPTY:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_BATTERY, BMP_ICN_BATTERY_00);
        break;
    case GUI_BATTERY_STATE_0:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_BATTERY, BMP_ICN_BATTERY_25);
        break;
    case GUI_BATTERY_STATE_1:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_BATTERY, BMP_ICN_BATTERY_50);
        break;
    case GUI_BATTERY_STATE_2:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_BATTERY, BMP_ICN_BATTERY_75);
        break;
    case GUI_BATTERY_STATE_3:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_BATTERY, BMP_ICN_BATTERY_100);
        break;
    }

    if (PowerType == GUI_POWER_ADAPTER) {
        APP_ADDFLAGS(GuiCtrlFlags.Power, GUI_FLAGS_POWER_DC_IN);
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_DC_PLUG, BMP_ICN_PLUGGER_DC);
    } else if (PowerType == GUI_POWER_BATTERY) {
        APP_REMOVEFLAGS(GuiCtrlFlags.Power, GUI_FLAGS_POWER_DC_IN);
    }

    return 0;
}

int AppGuiUtil_CardIconShow(UINT32 param1, UINT32 param2)
{
    AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_CARD);
    return 0;
}

int AppGuiUtil_CardIconHide(UINT32 param1, UINT32 param2)
{
    AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_CARD);
    return 0;
}

int AppGuiUtil_CardIconUpdate(int state)
{
    switch (state) {
    case GUI_CARD_REFRESHING:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_CARD, BMP_ICN_CARD_1_INACTIVE);
        break;
    case GUI_CARD_READY:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_CARD, BMP_ICN_CARD_1);
        break;
    case GUI_NO_CARD:
    default:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_CARD, BMP_ICN_CARD_1_INACTIVE);
        break;
    }
    return 0;
}

UINT32 AppGuiUtil_GetVideoResolutionBitmapSizeId(int videoRes)
{
    int ReturnValue = 0;
    if (videoRes == SENSOR_VIDEO_RES_UHD_HALF) {
        ReturnValue = BMP_ICN_VIDEO_3840_2160;
    } else if (videoRes == SENSOR_VIDEO_RES_2560_1920P30) {
        ReturnValue = BMP_ICN_VIDEO_2560_1920;
    } else if (videoRes == SENSOR_VIDEO_RES_WQHD_FULL ||
        videoRes == SENSOR_VIDEO_RES_WQHDP50 ||
        videoRes == SENSOR_VIDEO_RES_WQHD_HALF ||
        videoRes == SENSOR_VIDEO_RES_WQHD_HALF_HDR) {
        ReturnValue = BMP_ICN_VIDEO_2560_1440;
    //} else if (videoRes == SENSOR_VIDEO_RES_WFHD_HALF) {
    //    ReturnValue = BMP_ICN_VIDEO_2560_1080;
    } else if (videoRes == SENSOR_VIDEO_RES_1296P30) {
        ReturnValue = BMP_ICN_VIDEO_2304_1296;
    } else if (videoRes == SENSOR_VIDEO_RES_1920_1440P60 ||
        videoRes == SENSOR_VIDEO_RES_1920_1440P30) {
        ReturnValue = BMP_ICN_VIDEO_1920_1440;
    } else if ((videoRes <= SENSOR_VIDEO_RES_COMP_1080I) ||
        (videoRes == SENSOR_VIDEO_RES_TRUE_1080P48) ||
        (videoRes == SENSOR_VIDEO_RES_TRUE_1080P24) ||
        (videoRes == SENSOR_VIDEO_RES_FHD_HFR_P120_P100) ||
        (videoRes == SENSOR_VIDEO_RES_FHD_HFR_P100_P100) ||
        (videoRes == SENSOR_VIDEO_RES_TRUE_1080P_HALF_HDR)) {
        ReturnValue = BMP_ICN_VIDEO_1920_1080;
    } else if ((videoRes == SENSOR_VIDEO_RES_1200P60) ||
        (videoRes == SENSOR_VIDEO_RES_1200P30) ||
        (videoRes == SENSOR_VIDEO_RES_1200P48) ||
        (videoRes == SENSOR_VIDEO_RES_1200P24)) {
        ReturnValue = BMP_0_NULL;//BMP_ICN_VIDEO_1600_1200;
    } else if ((videoRes == SENSOR_VIDEO_RES_HD_FULL) ||
        (videoRes == SENSOR_VIDEO_RES_HD_HALF) ||
        (videoRes == SENSOR_VIDEO_RES_HD_HALF_HDR) ||
        (videoRes == SENSOR_VIDEO_RES_HD_P48) ||
        (videoRes == SENSOR_VIDEO_RES_HD_P24) ||
        (videoRes == SENSOR_VIDEO_RES_HD_HFR_P240_P200) ||
        (videoRes == SENSOR_VIDEO_RES_HD_HFR_P200_P200) ||
        (videoRes == SENSOR_VIDEO_RES_HD_HFR_P120_P100)) {
        ReturnValue = BMP_ICN_VIDEO_1280_720;
    } else if ((videoRes == SENSOR_VIDEO_RES_WVGA_FULL) ||
        (videoRes == SENSOR_VIDEO_RES_WVGA_HALF) ||
        (videoRes == SENSOR_VIDEO_RES_WVGA_P48) ||
        (videoRes == SENSOR_VIDEO_RES_WVGA_P24) ||
        (videoRes == SENSOR_VIDEO_RES_WVGA_HFR_P240_P200) ||
        (videoRes == SENSOR_VIDEO_RES_WVGA_HFR_P120_P100)) {
        ReturnValue = BMP_ICN_VIDEO_848_480;
    } else if ((videoRes == SENSOR_VIDEO_RES_960P60) ||
        (videoRes == SENSOR_VIDEO_RES_960P30) ||
        (videoRes == SENSOR_VIDEO_RES_960P48) ||
        (videoRes == SENSOR_VIDEO_RES_960P24)) {
        ReturnValue = BMP_ICN_VIDEO_1280_960;
    } else if ((videoRes == SENSOR_VIDEO_RES_VGA_FULL) ||
        (videoRes == SENSOR_VIDEO_RES_VGA_HALF) ||
        (videoRes == SENSOR_VIDEO_RES_VGA_P48) ||
        (videoRes == SENSOR_VIDEO_RES_VGA_P24) ||
        (videoRes == SENSOR_VIDEO_RES_VGA_HFR_P240_P200) ||
        (videoRes == SENSOR_VIDEO_RES_VGA_HFR_P120_P100)) {
        ReturnValue = BMP_ICN_VIDEO_640_480;
    } else if (((videoRes >= SENSOR_VIDEO_RES_WQVGA_HFR_P240_P200) &&
        (videoRes <= SENSOR_VIDEO_RES_WQVGA_HFR_P120_P100)) ||
        (videoRes == SENSOR_VIDEO_RES_WQVGA_FULL) ||
        (videoRes == SENSOR_VIDEO_RES_WQVGA_HALF)) {
        ReturnValue = BMP_ICN_VIDEO_432_240;
    } else if ((videoRes == SENSOR_VIDEO_RES_QVGA_HFR_P120_P100) ||
        (videoRes == SENSOR_VIDEO_RES_QVGA)) {
        ReturnValue = BMP_ICN_VIDEO_320_240;
    } else {
        ReturnValue = BMP_0_NULL;
    }
    return ReturnValue;
}

UINT32 AppGuiUtil_GetVideoResolutionBitmapFrateARId(int videoRes)
{
    int ReturnValue = 0;
    int VinSysType = AppLibSysVin_GetSystemType();
    if (videoRes == SENSOR_VIDEO_RES_UHD_HALF ||
            videoRes == SENSOR_VIDEO_RES_WQHD_HALF ||
            videoRes == SENSOR_VIDEO_RES_WQHD_HALF_HDR ||
            videoRes == SENSOR_VIDEO_RES_1296P30 ||
            videoRes == SENSOR_VIDEO_RES_TRUE_1080P_HALF  ||
            videoRes == SENSOR_VIDEO_RES_TRUE_1080P_HALF_HDR ||
            videoRes == SENSOR_VIDEO_RES_HD_HALF ||
            videoRes == SENSOR_VIDEO_RES_HD_HALF_HDR ||
            videoRes == SENSOR_VIDEO_RES_WVGA_HALF ||
            videoRes == SENSOR_VIDEO_RES_WQVGA_HALF) {
        if (VinSysType == VIN_SYS_NTSC) {
            ReturnValue = BMP_ICN_16_9_30P;
        } else {
            ReturnValue = BMP_ICN_16_9_25P;
        }
    } else if ( videoRes == SENSOR_VIDEO_RES_2560_1920P30 ||
                videoRes == SENSOR_VIDEO_RES_1920_1440P30 ||
                videoRes == SENSOR_VIDEO_RES_1200P30 ||
                videoRes == SENSOR_VIDEO_RES_960P30) {
        if (VinSysType == VIN_SYS_NTSC) {
            ReturnValue = BMP_ICN_4_3_30P;
        } else {
            ReturnValue = BMP_ICN_4_3_25P;
        }
    } else if (videoRes == SENSOR_VIDEO_RES_1920_1440P60 ||
                videoRes == SENSOR_VIDEO_RES_1200P60 ||
                videoRes == SENSOR_VIDEO_RES_960P60 ||
                videoRes == SENSOR_VIDEO_RES_VGA_HALF ||
                videoRes == SENSOR_VIDEO_RES_QVGA) {
        if (VinSysType == VIN_SYS_NTSC) {
            ReturnValue = BMP_ICN_4_3_60P;
        } else {
            ReturnValue = BMP_ICN_4_3_60P;//Need to fix
        }
    } else if (videoRes == SENSOR_VIDEO_RES_WQHD_FULL ||
                videoRes == SENSOR_VIDEO_RES_TRUE_1080P_FULL ||
                videoRes == SENSOR_VIDEO_RES_TRUE_1080P_FULL_HDR ||
                videoRes == SENSOR_VIDEO_RES_HD_FULL ||
                videoRes == SENSOR_VIDEO_RES_WVGA_FULL ||
                videoRes == SENSOR_VIDEO_RES_VGA_FULL ||
                videoRes == SENSOR_VIDEO_RES_WQVGA_FULL) {
        if (VinSysType == VIN_SYS_NTSC) {
            ReturnValue = BMP_ICN_16_9_60P;
        } else {
            ReturnValue = BMP_ICN_16_9_50P;
        }
    } else if (videoRes == SENSOR_VIDEO_RES_COMP_1080I) {
        if (VinSysType == VIN_SYS_NTSC) {
            ReturnValue = BMP_ICN_16_9_60I;
        } else {
            ReturnValue = BMP_ICN_16_9_50I;
        }
    } else if (videoRes == SENSOR_VIDEO_RES_1200P48 ||
                videoRes == SENSOR_VIDEO_RES_960P48 ||
                videoRes == SENSOR_VIDEO_RES_VGA_P48) {
        ReturnValue = BMP_ICN_4_3_60P;//Need to fix
    } else if (videoRes == SENSOR_VIDEO_RES_1200P24 ||
                videoRes == SENSOR_VIDEO_RES_960P24 ||
                videoRes == SENSOR_VIDEO_RES_VGA_P24) {
        ReturnValue = BMP_ICN_4_3_30P;//Need to fix
    } else if (videoRes == SENSOR_VIDEO_RES_TRUE_1080P48 ||
                videoRes == SENSOR_VIDEO_RES_HD_P48 ||
                videoRes == SENSOR_VIDEO_RES_WVGA_P48) {
        ReturnValue = BMP_ICN_16_9_60P;//Need to fix
    } else if (videoRes == SENSOR_VIDEO_RES_TRUE_1080P24 ||
                videoRes == SENSOR_VIDEO_RES_HD_P24 ||
                videoRes == SENSOR_VIDEO_RES_WVGA_P24) {
        ReturnValue = BMP_ICN_16_9_30P;//Need to fix
    } else if (videoRes == SENSOR_VIDEO_RES_FHD_HFR_P120_P100 ||
                videoRes == SENSOR_VIDEO_RES_HD_HFR_P120_P100 ||
                videoRes == SENSOR_VIDEO_RES_WVGA_HFR_P120_P100 ||
                videoRes == SENSOR_VIDEO_RES_WQVGA_HFR_P120_P100 ||
                videoRes == SENSOR_VIDEO_RES_QVGA_HFR_P120_P100) {
        if (VinSysType == VIN_SYS_NTSC) {
            ReturnValue = BMP_ICN_16_9_120P;
        } else {
            ReturnValue = BMP_ICN_16_9_100P;
        }
    } else if (videoRes == SENSOR_VIDEO_RES_VGA_HFR_P120_P100) {
        if (VinSysType == VIN_SYS_NTSC) {
            ReturnValue = BMP_ICN_4_3_120P;
        } else {
            ReturnValue = BMP_ICN_4_3_100P;
        }
    } else if (videoRes == SENSOR_VIDEO_RES_HD_HFR_P240_P200 ||
                videoRes == SENSOR_VIDEO_RES_WVGA_HFR_P240_P200 ||
                videoRes == SENSOR_VIDEO_RES_WQVGA_HFR_P240_P200) {
        if (VinSysType == VIN_SYS_NTSC) {
            ReturnValue = BMP_ICN_16_9_240P;
        } else {
            ReturnValue = BMP_ICN_16_9_200P;
        }
    } else if (videoRes == SENSOR_VIDEO_RES_VGA_HFR_P240_P200) {
        if (VinSysType == VIN_SYS_NTSC) {
            ReturnValue = BMP_ICN_4_3_120P;//Need to fix
        } else {
            ReturnValue = BMP_ICN_4_3_100P;//Need to fix
        }
    } else if (videoRes == SENSOR_VIDEO_RES_WQHDP50) {
        ReturnValue = BMP_ICN_16_9_50P;
    } else if (videoRes == SENSOR_VIDEO_RES_FHD_HFR_P100_P100) {
        ReturnValue = BMP_ICN_16_9_100P;
    } else if (videoRes == SENSOR_VIDEO_RES_HD_HFR_P200_P200) {
        ReturnValue = BMP_ICN_16_9_200P;
    } else {
        ReturnValue = BMP_0_NULL;
    }
    return ReturnValue;
}

int AppGuiUtil_GetFilenameStrings(WCHAR *fnGui, WCHAR *fnDmf, GUI_UTILITY_FILENAME_STYLE_e style)
{
    int i = 0, Length = 0;
    WCHAR FilenameTmp[GUI_FILENAME_SIZE] = {0};
    WCHAR Delimiter[3] = {'\\','/','.'};
    WCHAR *FnTmp;

    switch (style) {
    case GUI_PB_FN_STYLE_HYPHEN: //display 140101100-10101000
        // C:\DCIM\140101100\10101000.XXX
        FnTmp = w_strrchr(fnDmf, Delimiter[2]);
        w_strncpy(FilenameTmp, (FnTmp-18), 9);
        FilenameTmp[9] = '-';
        w_strncpy(&FilenameTmp[10], (FnTmp - 8), 8);
        break;
    case GUI_PB_FN_STYLE_UNDERSCORE: //display 140101100-10101000
        // C:\DCIM\140101100\10101000.XXX
        FnTmp = w_strchr(fnDmf, Delimiter[2]);
        w_strncpy(FilenameTmp, (FnTmp-18), 9);
        FilenameTmp[9] = '_';
        w_strncpy(&FilenameTmp[10], (FnTmp - 8), 8);
        break;
    case GUI_PB_FN_STYLE_FULL:
    default:
        FnTmp = w_strchr(fnDmf, Delimiter[2]);
        w_strcpy(FilenameTmp, FnTmp-8);
        break;
    }

    Length = w_strlen(FilenameTmp);
    if (Length >= GUI_FILENAME_SIZE) {
        AmbaPrintColor(RED, "[Gui Utility] <GetFilenameStrings> ERROR! Length = %d", Length);
        return -1;
    }
    for (i=0; i<Length; i++) {
        fnGui[i] = 0x0000+((int)FilenameTmp[i]);
    }
    fnGui[i] = 0x0000;

    return 0;
}


