/**
  * @file src/app/apps/gui/rec/demo/gui_rec_connected_cam.c
  *
  *  Implementation of Sport Recorder (sensor) GUI display flowsx
  *
  * History:
  *    2013/08/09 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#include <apps/gui/rec/gui_rec_connected_cam.h>
#include <apps/gui/resource/gui_resource.h>
#include <apps/gui/resource/gui_settle.h>
#include <wchar.h>

#ifdef ENABLE_VA_STAMP
#include <gps.h>
#endif

#define GOBJ_REC_SELFTIMER_CURSOR    (GOBJ_FV_ICON_UP_1_CURSOR)
#define GOBJ_REC_SELFTIMER            (GOBJ_FV_ICON_UP_1)
#define GOBJ_REC_MODE_CURSOR        (GOBJ_FV_ICON_UP_2_CURSOR)
#define GOBJ_REC_MODE                (GOBJ_FV_ICON_UP_2)
#define GOBJ_REC_CAP_MODE_CURSOR    (GOBJ_FV_ICON_UP_3_CURSOR)
#define GOBJ_REC_CAP_MODE            (GOBJ_FV_ICON_UP_3)
#define GOBJ_REC_VIDEO_RES_CURSOR    (GOBJ_FV_ICON_LEFT_1_CURSOR)
#define GOBJ_REC_VIDEO_RES            (GOBJ_FV_ICON_LEFT_1)
#define GOBJ_REC_VIDEO_FRAMERATE_AR_CURSOR    (GOBJ_FV_ICON_LEFT_2_CURSOR)
#define GOBJ_REC_VIDEO_FRAMERATE_AR        (GOBJ_FV_ICON_LEFT_2)
#define GOBJ_REC_VIDEO_QUALITY_CURSOR    (GOBJ_FV_ICON_LEFT_3_CURSOR)
#define GOBJ_REC_VIDEO_QUALITY        (GOBJ_FV_ICON_LEFT_3)
#define GOBJ_REC_PH_SIZE_CURSOR        (GOBJ_FV_ICON_LEFT_4_CURSOR)
#define GOBJ_REC_PH_SIZE            (GOBJ_FV_ICON_LEFT_4)
#define GOBJ_REC_PH_QUALITY_CURSOR    (GOBJ_FV_ICON_LEFT_5_CURSOR)
#define GOBJ_REC_PH_QUALITY            (GOBJ_FV_ICON_LEFT_5)

typedef struct gui_ctrl_flags_s {
    UINT8 Selftimer;
#define GUI_FLAGS_SELFTIMER_SHOWED    (0x01)
    UINT8 VideoRes;
#define GUI_FLAGS_VIDEO_RES_SHOWED    (0x01)
    UINT8 VideoQuality;
#define GUI_FLAGS_VIDEO_QUALITY_SHOWED    (0x01)
    UINT8 RecMode;
#define GUI_FLAGS_REC_MODE_SHOWED    (0x01)
    UINT8 PhotoSize;
#define GUI_FLAGS_PHOTO_SIZE_SHOWED    (0x01)
    UINT8 PhotoQuality;
#define GUI_FLAGS_PHOTO_QUALITY_SHOWED    (0x01)
    UINT8 CapMode;
#define GUI_FLAGS_CAP_MODE_SHOWED    (0x01)
} gui_ctrl_flags_t;

static gui_ctrl_flags_t gui_ctrl_flags = {0};

static int set_selftimer_state(int state)
{
#if 0
    switch (state) {
    case GUI_SELFTIMER_3S:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_SELFTIMER, BMP_ICN_SELF_TIMER_3);
        break;
    case GUI_SELFTIMER_5S:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_SELFTIMER, BMP_ICN_SELF_TIMER_5);
        break;
    case GUI_SELFTIMER_10S:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_SELFTIMER, BMP_ICN_SELF_TIMER_10);
        break;
    case GUI_SELFTIMER_OFF:
    default:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_SELFTIMER, BMP_ICN_SELF_TIMER_OFF);
        break;
    }
#endif
    return 0;
}

static int set_rec_state(int state)
{
    switch (state) {
    case GUI_REC_START:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_STATE, BMP_ICN_VIDEO_REC);
        break;
    case GUI_REC_PAUSED:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_STATE, BMP_ICN_VIDEO_REC_PAUSE);
        break;
    default:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_STATE, 0);
        break;
    }
    return 0;
}

static int set_rec_mode(int mode)
{
#if 0
    switch (mode) {
    case GUI_MODE_TIME_LAPSE_2S:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_MODE, BMP_ICN_VIDEO_TIME_LAPSE_2);
        break;
    case GUI_MODE_DEFAULT:
    default:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_MODE, BMP_ICN_VIDEO_FILM);
        break;
    }
#endif
    return 0;
}

static int set_video_quality(int quality)
{
    int ReturnValue = 0;

    switch (quality) {
    case GUI_SFINE:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_QUALITY, BMP_ICN_QUALITY_SF);
        break;
    case GUI_FINE:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_QUALITY, BMP_ICN_QUALITY_F);
        break;
    case GUI_NORMAL:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_QUALITY, BMP_ICN_QUALITY_N);
        break;
    default:
        ReturnValue = -1;
        break;
    }

    return ReturnValue;
}

static int set_photo_size(int sizeId)
{
    int ReturnValue = 0;
    switch (sizeId) {
    case GUI_PHOTO_SIZE_1:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE, BMP_ICN_PHOTO_SIZE_1);
        break;
    case GUI_PHOTO_SIZE_2:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE, BMP_ICN_PHOTO_SIZE_2);
        break;
    case GUI_PHOTO_SIZE_3:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE, BMP_ICN_PHOTO_SIZE_3);
        break;
    case GUI_PHOTO_SIZE_4:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE, BMP_ICN_PHOTO_SIZE_4);
        break;
    case GUI_PHOTO_SIZE_5:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE, BMP_ICN_PHOTO_SIZE_5);
        break;
    case GUI_PHOTO_SIZE_6:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE, BMP_ICN_PHOTO_SIZE_6);
        break;
    case GUI_PHOTO_SIZE_7:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE, BMP_ICN_PHOTO_SIZE_7);
        break;
    case GUI_PHOTO_SIZE_8:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE, BMP_ICN_PHOTO_SIZE_8);
        break;
    case GUI_PHOTO_SIZE_9:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE, BMP_ICN_PHOTO_SIZE_9);
        break;
    case GUI_PHOTO_SIZE_10:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE, BMP_ICN_PHOTO_SIZE_10);
        break;
    default:
        ReturnValue = -1;
        break;
    }
    return ReturnValue;
}

static int set_photo_quality(int quality)
{
    int ReturnValue = 0;
    switch (quality) {
    case GUI_SFINE:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_QUALITY, BMP_ICN_QUALITY_SF);
        break;
    case GUI_FINE:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_QUALITY, BMP_ICN_QUALITY_F);
        break;
    case GUI_NORMAL:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_PH_QUALITY, BMP_ICN_QUALITY_N);
        break;
    default:
        ReturnValue = -1;
        break;
    }
    return ReturnValue;
}

static int set_cap_mode(int mode_id, int multi_cap)
{
    int ReturnValue = 0;
#if 0
    if (multi_cap == PHOTO_MULTI_CAP_OFF) {
        switch (mode_id) {
        case GUI_PRECISE:
            AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_CAP_MODE, BMP_ICN_PRECISE_QUALITY);
            break;
        case GUI_PRECISE_CONT:
        case GUI_BURST:
            AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_CAP_MODE, BMP_ICN_CONT_SUPER_FAST);
            break;
        default:
            ReturnValue = -1;
            break;
        }
    }
#endif
    return ReturnValue;
}

static int set_cd_timer(int time)
{
    int seconds = 0;
    UINT16 str_timer[3];

    seconds = time;

    str_timer[0] = 0x0030+(seconds/10);
    str_timer[1] = 0x0030+(seconds%10);

    str_timer[2] = 0x0000;

    AppLibGraph_UpdateStringContext(0, STR_REC_TIME, str_timer);
    AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_REC_TIME, STR_REC_TIME);

    return 0;
}

static int set_rec_timer(UINT32 time)
{
    WCHAR RecStrTimer[10] = {'0','0','0',':','0','0',':','0','0','\0'};
    int Hours = 0, Minutes = 0, Seconds = 0;
    Hours = time/3600;
    Minutes = (time-(Hours*3600))/60;
    Seconds = time-((Hours*3600)+(Minutes*60));

    RecStrTimer[0] = 0x0030 + (Hours/100);
    RecStrTimer[1] = 0x0030 + ((Hours%100)/10);
    RecStrTimer[2] = 0x0030 + (Hours%10);

    RecStrTimer[3] = 0x003A;

    RecStrTimer[4] = 0x0030 + (Minutes/10);
    RecStrTimer[5] = 0x0030 + (Minutes%10);

    RecStrTimer[6] = 0x003A;

    RecStrTimer[7] = 0x0030 + (Seconds/10);
    RecStrTimer[8] = 0x0030 + (Seconds%10);

    RecStrTimer[9] = 0x0000;

    AppLibGraph_UpdateStringContext(0, STR_REC_TIME, (UINT16*)RecStrTimer);
    AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_REC_TIME, STR_REC_TIME);

    return 0;
}
#if defined(CONFIG_APP_CONNECTED_STAMP)
static int init_stamp(void)
{
    AppLibGraph_ResetStamp();
    return 0;
}

#ifdef ENABLE_VA_STAMP
static int set_stamp_date(UINT32 year, UINT32 month, UINT32 day)
{
    return 0;
}

static int set_stamp_time_h_m(UINT32 hour, UINT32 minute)
{
    return 0;
}

static int set_stamp_time_s(UINT32 second)
{
    return 0;
}

static void AdasStmpHandler(int eid)
{
    if (eid == TIMER_UNREGISTER) {
        return;
    }

    /* Distance to frontal car layer */
    {
        AMBA_ADAS_FCW_OUTPUT_s* FcwOutput = NULL;
        WCHAR StampStrTimeHandM[6] = {'0','0','0','m','/','\0'};
        int distance = 0;

        Amba_Adas_GetFcwOutput(&FcwOutput);
        distance = (int)FcwOutput->FrontVehicleDistance;
        if (distance > 0) {
            StampStrTimeHandM[0] = 0x0030 + distance/100;
            StampStrTimeHandM[1] = 0x0030 + (distance%100)/10;
            StampStrTimeHandM[2] = 0x0030 + (distance%10);
            StampStrTimeHandM[3] = 'm';
        } else {
            StampStrTimeHandM[0] = 'x';
            StampStrTimeHandM[1] = 'x';
            StampStrTimeHandM[2] = 'x';
            StampStrTimeHandM[3] = 'x';
        }
        AppLibGraph_UpdateStringContext(0, STR_OSD_BLEND_TIME_H_M, StampStrTimeHandM);
        AppLibGraph_UpdateString(GRAPH_CH_BLEND, BOBJ_TIME_H_M_STREAM_0, STR_OSD_BLEND_TIME_H_M);
    }

    /* Car speed layer */
    {
        WCHAR StampStrTimeHandS[4] = {'0','0','0','\0'};
        static gps_data_t* StampGpsData = NULL;
        int Speed, Status;
        StampGpsData = AppLibSysGps_GetData();
        Speed = StampGpsData->fix.speed*MPS_TO_KMPH;
        Status = StampGpsData->status;

        if (Status == 0) {
            StampStrTimeHandS[0] = 'x';
            StampStrTimeHandS[1] = 'x';
            StampStrTimeHandS[2] = 'x';
        } else {
            StampStrTimeHandS[0] = 0x0030 + Speed/100;
            StampStrTimeHandS[1] = 0x0030 + (Speed%100)/10;
            StampStrTimeHandS[2] = 0x0030 + (Speed%10);
        }
        AppLibGraph_UpdateStringContext(0, STR_OSD_BLEND_TIME_S, StampStrTimeHandS);
        AppLibGraph_UpdateString(GRAPH_CH_BLEND, BOBJ_TIME_S_STREAM_0, STR_OSD_BLEND_TIME_S);
    }
}


static void AdasStmpUpdate(void)
{
    /* ADAS event layer */
    {
        UINT32 EventFlag = AppLibVideoAnal_StmpHdlr_GetEventFlag();
        UINT16 StampStrDate[7] = {'-','-','-','-','-','-','\0'};

        if (EventFlag & VA_STMPHDLR_LDWS) {
            StampStrDate[0] = 'D';
        }

        if (EventFlag & VA_STMPHDLR_FCWS) {
            StampStrDate[1] = 'F';
        }

        if (EventFlag & VA_STMPHDLR_FCMD) {
            StampStrDate[2] = 'M';
        }

        if (EventFlag & VA_STMPHDLR_LLWS) {
            StampStrDate[3] = 'L';
        }

        if (EventFlag & VA_STMPHDLR_CALIB) {
            StampStrDate[4] = 'C';
        }

        AppLibGraph_UpdateStringContext(0, STR_OSD_BLEND_DATE, StampStrDate);
        AppLibGraph_UpdateString(GRAPH_CH_BLEND, BOBJ_DATE_STREAM_0, STR_OSD_BLEND_DATE);
    }

    AppLibGraph_Draw(GRAPH_CH_BLEND);
}
#else
static int set_stamp_date(UINT32 year, UINT32 month, UINT32 day)
{
    UINT16 StampStrDate[11] = {'0','0','0','0','/','0','0','/','0','0','\0'};
    UINT8 streamCount = 0;

    StampStrDate[0] = 0x0030 + (year / 1000);
    StampStrDate[1] = 0x0030 + ((year % 1000) / 100);
    StampStrDate[2] = 0x0030 + ((year % 100) / 10);
    StampStrDate[3] = 0x0030 + (year % 10);

    StampStrDate[4] = 0x002F;

    StampStrDate[5] = 0x0030 + (month / 10);
    StampStrDate[6] = 0x0030 + (month % 10);

    StampStrDate[7] = 0x002F;

    StampStrDate[8] = 0x0030 + (day / 10);
    StampStrDate[9] = 0x0030 + (day % 10);

    StampStrDate[10] = 0x0000;

    for (streamCount = 0; streamCount < 6; streamCount++) {
        AppLibGraph_UpdateStringContext(0, STR_OSD_BLEND_DATE, StampStrDate);
        AppLibGraph_UpdateString(GRAPH_CH_BLEND, BOBJ_DATE_STREAM_0 + streamCount, STR_OSD_BLEND_DATE);
    }

    return 0;
}

static int set_stamp_time_h_m(UINT32 hour, UINT32 minute)
{
    WCHAR StampStrTimeHandM[7] = {'0','0',':','0','0',':','\0'};
    UINT8 streamCount = 0;

    StampStrTimeHandM[0] = 0x0030 + (hour / 10);
    StampStrTimeHandM[1] = 0x0030 + (hour % 10);

    StampStrTimeHandM[2] = 0x003A;

    StampStrTimeHandM[3] = 0x0030 + (minute / 10);
    StampStrTimeHandM[4] = 0x0030 + (minute % 10);

    StampStrTimeHandM[5] = 0x003A;

    StampStrTimeHandM[6] = 0x0000;

    for (streamCount = 0; streamCount < 6; streamCount++) {
        AppLibGraph_UpdateStringContext(0, STR_OSD_BLEND_TIME_H_M, StampStrTimeHandM);
        AppLibGraph_UpdateString(GRAPH_CH_BLEND, BOBJ_TIME_H_M_STREAM_0 + streamCount, STR_OSD_BLEND_TIME_H_M);
    }

    return 0;
}

static int set_stamp_time_s(UINT32 second)
{
    WCHAR StampStrTimeHandM[3] = {'0','0','\0'};
    UINT8 streamCount = 0;

    StampStrTimeHandM[0] = 0x0030 + (second / 10);
    StampStrTimeHandM[1] = 0x0030 + (second % 10);

    StampStrTimeHandM[2] = 0x0000;

    for (streamCount = 0; streamCount < 6; streamCount++) {
        AppLibGraph_UpdateStringContext(0, STR_OSD_BLEND_TIME_S, StampStrTimeHandM);
        AppLibGraph_UpdateString(GRAPH_CH_BLEND, BOBJ_TIME_S_STREAM_0 + streamCount, STR_OSD_BLEND_TIME_S);
    }
    return 0;
}
#endif

#if 0
static UINT32 update_stamp_size(UINT32 guiId, AMP_AREA_s *areaInfo, UINT32 modifiedStrHeight)
{
    UINT32 ModifiedWidth = 0;
    UINT32 ModifiedHeight = 0;
    UINT8 streamCount = 0;

    ModifiedHeight = modifiedStrHeight;
    ModifiedWidth = (areaInfo->Width * ModifiedHeight) / (areaInfo->Height-2); // ToDo: should remove shift distance

    for (streamCount = 0; streamCount < 6; streamCount++) {
        AppLibGraph_UpdateSize(GRAPH_CH_BLEND, guiId + streamCount, ModifiedWidth, ModifiedHeight, modifiedStrHeight);
    }
    return ModifiedWidth;
}
#endif
static int update_stamp_position(UINT32 guiId, UINT32 left, UINT32 top)
{
    UINT8 streamCount = 0;
    for (streamCount = 0; streamCount < 6; streamCount++) {
        AppLibGraph_UpdatePosition(GRAPH_CH_BLEND, guiId + streamCount, left, top);
    }
    return 0;
}

static int show_stamp(UINT32 stampMode)
{
    UINT8 streamCount = 0;
    for (streamCount = 0; streamCount < 6; streamCount++) {
        if (stampMode == GUI_STAMP_DATE) { // date
            AppLibGraph_Show(GRAPH_CH_BLEND, BOBJ_DATE_STREAM_0 + streamCount);
        } else if (stampMode == GUI_STAMP_TIME) { // time
            AppLibGraph_Show(GRAPH_CH_BLEND, BOBJ_TIME_H_M_STREAM_0 + streamCount);
            AppLibGraph_Show(GRAPH_CH_BLEND, BOBJ_TIME_S_STREAM_0 + streamCount);
        } else { // date_time
            AppLibGraph_Show(GRAPH_CH_BLEND, BOBJ_DATE_STREAM_0 + streamCount);
            AppLibGraph_Show(GRAPH_CH_BLEND, BOBJ_TIME_H_M_STREAM_0 + streamCount);
            AppLibGraph_Show(GRAPH_CH_BLEND, BOBJ_TIME_S_STREAM_0 + streamCount);
        }
    }
    return 0;
}

static int hide_stamp(UINT32 stampMode)
{
    UINT8 streamCount = 0;
    for (streamCount = 0; streamCount < 6; streamCount++) {
        if (stampMode == GUI_STAMP_DATE) {
            AppLibGraph_Hide(GRAPH_CH_BLEND, BOBJ_DATE_STREAM_0 + streamCount);
        } else if (stampMode == GUI_STAMP_TIME) {
            AppLibGraph_Hide(GRAPH_CH_BLEND, BOBJ_TIME_H_M_STREAM_0 + streamCount);
            AppLibGraph_Hide(GRAPH_CH_BLEND, BOBJ_TIME_S_STREAM_0 + streamCount);
        } else {
            AppLibGraph_Hide(GRAPH_CH_BLEND, BOBJ_DATE_STREAM_0 + streamCount);
            AppLibGraph_Hide(GRAPH_CH_BLEND, BOBJ_TIME_H_M_STREAM_0 + streamCount);
            AppLibGraph_Hide(GRAPH_CH_BLEND, BOBJ_TIME_S_STREAM_0 + streamCount);
        }
    }
    return 0;
}

static int update_stamp(UINT8 stampType, GUI_REC_CONNECTED_CAM_STAMP_UPDATE_INFO_s* stampInfo)
{
    if (stampType == GUI_STAMP_OFF) {  // off
        return 0;
    }
    update_stamp_position(stampInfo->GuiObjId, stampInfo->Left, stampInfo->Top);
    return 0;
}
#endif
/**
 *  @brief The GUI functions of recorder application
 *
 *  The GUI functions of recorder application
 *
 *  @param [in] guiCmd Command ID
 *  @param [in] param1 First parameter
 *  @param [in] param2 Second parameter
 *
 *  @return >=0 success, <0 failure
 */
int gui_rec_connected_cam_func(UINT32 guiCmd, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;

    switch (guiCmd) {
    case GUI_FLUSH:
        AppLibGraph_Draw(GRAPH_CH_DUAL);
        AppLibGraph_Draw(GRAPH_CH_BLEND);
        break;
    case GUI_HIDE_ALL:
        AppLibGraph_HideAll(GRAPH_CH_DUAL);
        break;
    case GUI_SET_LAYOUT:
        AppLibGraph_SetGUILayout(GRAPH_CH_DCHAN, Gui_Resource_Dchan_Id, Gui_Table_Dchan, 0);
        AppLibGraph_SetGUILayout(GRAPH_CH_FCHAN, Gui_Resource_Fchan_Id, Gui_Table_Fchan, 0);
        AppLibGraph_SetGUILayout(GRAPH_CH_BLEND, Gui_Resource_Blend_Id, Gui_Table_Blend, 0);
        break;
    case GUI_APP_VIDEO_ICON_SHOW:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_APP_ICON, BMP_BTN_MODE_VIDEO);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_APP_ICON);
        break;
    case GUI_APP_PHOTO_ICON_SHOW:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_APP_ICON, BMP_BTN_MODE_PHOTO);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_APP_ICON);
        break;
    case GUI_APP_ICON_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_APP_ICON);
        break;
    case GUI_POWER_STATE_SHOW:
        AppGuiUtil_PowerIconShow(param1, param2);
        break;
    case GUI_POWER_STATE_HIDE:
        AppGuiUtil_PowerIconHide(param1, param2);
        break;
    case GUI_POWER_STATE_UPDATE:
        AppGuiUtil_PowerIconUpdate(param1, param2);
        break;
    case GUI_CARD_SHOW:
        AppGuiUtil_CardIconShow(param1, param2);
        break;
    case GUI_CARD_HIDE:
        AppGuiUtil_CardIconHide(param1, param2);
        break;
    case GUI_CARD_UPDATE:
        AppGuiUtil_CardIconUpdate(param1);
        break;
    case GUI_WARNING_SHOW:
        AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_WARNING);
        break;
    case GUI_WARNING_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_WARNING);
        break;
    case GUI_WARNING_UPDATE:
        AppLibGraph_UpdateString(GRAPH_CH_DCHAN, GOBJ_WARNING, GuiWarningTable[param1].str);
        break;
    case GUI_SELFTIMER_SHOW:
        APP_ADDFLAGS(gui_ctrl_flags.Selftimer, GUI_FLAGS_SELFTIMER_SHOWED);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_SELFTIMER);
        break;
    case GUI_SELFTIMER_HIDE:
        APP_REMOVEFLAGS(gui_ctrl_flags.Selftimer, GUI_FLAGS_SELFTIMER_SHOWED);
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_SELFTIMER);
        break;
    case GUI_SELFTIMER_HL:
        if (APP_CHECKFLAGS(gui_ctrl_flags.Selftimer, GUI_FLAGS_SELFTIMER_SHOWED) && param1) {
            AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_REC_SELFTIMER_CURSOR);
        } else {
            AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_REC_SELFTIMER_CURSOR);
        }
        break;
    case GUI_SELFTIMER_UPDATE:
        set_selftimer_state(param1);
        break;
    case GUI_PHOTO_SIZE_SHOW:
        APP_ADDFLAGS(gui_ctrl_flags.PhotoSize, GUI_FLAGS_PHOTO_SIZE_SHOWED);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE);
        break;
    case GUI_PHOTO_SIZE_HIDE:
        APP_REMOVEFLAGS(gui_ctrl_flags.PhotoSize, GUI_FLAGS_PHOTO_SIZE_SHOWED);
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_PH_SIZE);
        break;
    case GUI_PHOTO_SIZE_HL:
        if (APP_CHECKFLAGS(gui_ctrl_flags.PhotoSize, GUI_FLAGS_PHOTO_SIZE_SHOWED) && param1) {
            AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_REC_PH_SIZE_CURSOR);
        } else {
            AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_REC_PH_SIZE_CURSOR);
        }
        break;
    case GUI_PHOTO_SIZE_UPDATE:
        set_photo_size(param1);
        break;
    case GUI_PHOTO_QUALITY_SHOW:
        APP_ADDFLAGS(gui_ctrl_flags.PhotoQuality, GUI_FLAGS_PHOTO_QUALITY_SHOWED);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_PH_QUALITY);
        break;
    case GUI_PHOTO_QUALITY_HIDE:
        APP_REMOVEFLAGS(gui_ctrl_flags.PhotoQuality, GUI_FLAGS_PHOTO_QUALITY_SHOWED);
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_PH_QUALITY);
        break;
    case GUI_PHOTO_QUALITY_HL:
        if (APP_CHECKFLAGS(gui_ctrl_flags.PhotoQuality, GUI_FLAGS_PHOTO_QUALITY_SHOWED) && param1) {
            AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_REC_PH_QUALITY_CURSOR);
        } else {
            AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_REC_PH_QUALITY_CURSOR);
        }
        break;
    case GUI_PHOTO_QUALITY_UPDATE:
        set_photo_quality(param1);
        break;
    case GUI_CAP_MODE_SHOW:
        APP_ADDFLAGS(gui_ctrl_flags.CapMode, GUI_FLAGS_CAP_MODE_SHOWED);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_CAP_MODE);
        break;
    case GUI_CAP_MODE_HIDE:
        APP_REMOVEFLAGS(gui_ctrl_flags.CapMode, GUI_FLAGS_CAP_MODE_SHOWED);
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_CAP_MODE);
        break;
    case GUI_CAP_MODE_HL:
        if (APP_CHECKFLAGS(gui_ctrl_flags.CapMode, GUI_FLAGS_CAP_MODE_SHOWED) && param1) {
            AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_REC_CAP_MODE_CURSOR);
        } else {
            AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_REC_CAP_MODE_CURSOR);
        }
        break;
    case GUI_CAP_MODE_UPDATE:
        set_cap_mode(param1, param2);
        break;
    case GUI_VIDEO_SENSOR_RES_SHOW:
        APP_ADDFLAGS(gui_ctrl_flags.VideoRes, GUI_FLAGS_VIDEO_RES_SHOWED);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_RES);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_FRAMERATE_AR);
        break;
    case GUI_VIDEO_SENSOR_RES_HIDE:
        APP_ADDFLAGS(gui_ctrl_flags.VideoRes, GUI_FLAGS_VIDEO_RES_SHOWED);
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_RES);
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_FRAMERATE_AR);
        break;
    case GUI_VIDEO_SENSOR_RES_HL:
        if (APP_CHECKFLAGS(gui_ctrl_flags.VideoRes, GUI_FLAGS_VIDEO_RES_SHOWED) && param1) {
            AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_REC_VIDEO_RES_CURSOR);
        } else {
            AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_REC_VIDEO_RES_CURSOR);
        }
        break;
    case GUI_VIDEO_SENSOR_RES_UPDATE:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_RES, AppGuiUtil_GetVideoResolutionBitmapSizeId(param1));
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_FRAMERATE_AR, AppGuiUtil_GetVideoResolutionBitmapFrateARId(param1));
        break;
    case GUI_VIDEO_QUALITY_SHOW:
        APP_ADDFLAGS(gui_ctrl_flags.VideoQuality, GUI_FLAGS_VIDEO_QUALITY_SHOWED);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_QUALITY);
        break;
    case GUI_VIDEO_QUALITY_HIDE:
        APP_REMOVEFLAGS(gui_ctrl_flags.VideoQuality, GUI_FLAGS_VIDEO_QUALITY_SHOWED);
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_QUALITY);
        break;
    case GUI_VIDEO_QUALITY_HL:
        if (APP_CHECKFLAGS(gui_ctrl_flags.VideoQuality, GUI_FLAGS_VIDEO_QUALITY_SHOWED) && param1) {
            AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_QUALITY_CURSOR);
        } else {
            AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_VIDEO_QUALITY_CURSOR);
        }
        break;
    case GUI_VIDEO_QUALITY_UPDATE:
        set_video_quality(param1);
        break;
    case GUI_REC_MODE_SHOW:
        APP_ADDFLAGS(gui_ctrl_flags.RecMode, GUI_FLAGS_REC_MODE_SHOWED);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_MODE);
        break;
    case GUI_REC_MODE_HIDE:
        APP_REMOVEFLAGS(gui_ctrl_flags.RecMode, GUI_FLAGS_REC_MODE_SHOWED);
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_MODE);
        break;
    case GUI_REC_MODE_HL:
        if (APP_CHECKFLAGS(gui_ctrl_flags.RecMode, GUI_FLAGS_REC_MODE_SHOWED) && param1) {
            AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_MODE_CURSOR);
        } else {
            AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_MODE_CURSOR);
        }
        break;
    case GUI_REC_MODE_UPDATE:
        set_rec_mode(param1);
        break;
#if defined(CONFIG_APP_CONNECTED_STAMP)
    case GUI_STAMP_SET_MODE:
        //gui_rec_stamp_init(param2, param1);
        init_stamp();
        break;
    case GUI_STAMP_SHOW:
        show_stamp(param1);
        break;
    case GUI_STAMP_HIDE:
        hide_stamp(param1);
        break;
    case GUI_STAMP_UPDATE_SIZE:
        update_stamp(param1, (GUI_REC_CONNECTED_CAM_STAMP_UPDATE_INFO_s*)param2);
        break;
    case GUI_STAMP_UPDATE_DATE:
        {
            GUI_REC_CONNECTED_CAM_STAMP_DATE_s *date = (GUI_REC_CONNECTED_CAM_STAMP_DATE_s *)param1;
            set_stamp_date(date->Year, date->Month, date->Day);
        }
        break;
    case GUI_STAMP_UPDATE_TIME:
        {
            GUI_REC_CONNECTED_CAM_STAMP_TIME_s *time = (GUI_REC_CONNECTED_CAM_STAMP_TIME_s *)param1;
            //if(time->Update_h_m){
                set_stamp_time_h_m(time->Hour, time->Minute);
            //}

            set_stamp_time_s(time->Second);
        }
    break;
#ifdef ENABLE_VA_STAMP
    case GUI_ADAS_STAMP_SHOW:
        AppLibComSvcTimer_Register(TIMER_10HZ, AdasStmpHandler);
        break;
    case GUI_ADAS_STAMP_HIDE:
        AppLibComSvcTimer_Unregister(TIMER_10HZ, AdasStmpHandler);
        break;
    case GUI_ADAS_STAMP_UPDATE:
        AdasStmpUpdate();
        break;
#endif
#endif
    case GUI_SELFTIMER_COUNTDOWN_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_TIME);
        break;
    case GUI_SELFTIMER_COUNTDOWN_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_TIME);
        break;
    case GUI_SELFTIMER_COUNTDOWN_UPDATE:
        set_cd_timer(param1);
        break;
    case GUI_REC_STATE_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_STATE);
        break;
    case GUI_REC_STATE_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_STATE);
        break;
    case GUI_REC_STATE_UPDATE:
        set_rec_state(param1);
        break;
    case GUI_REC_TIMER_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_TIME);
        break;
    case GUI_REC_TIMER_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_TIME);
        break;
    case GUI_REC_TIMER_UPDATE:
        set_rec_timer(param1);
        break;
    case GUI_REC_EMERGENCY_SHOW:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_REC_EMERGENCY, BMP_ICN_VIDEO_REC_EMERGENCY);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_REC_EMERGENCY);
        break;
    case GUI_REC_EMERGENCY_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_REC_EMERGENCY);
        break;
    case GUI_SUR_COVER_SHOW:
        AmbaPrintColor(GREEN,"GUI_SUR_COVER_SHOW");
        AppLibGraph_Show(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_0);
        AppLibGraph_Show(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_1);
        AppLibGraph_Show(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_2);
        AppLibGraph_Show(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_3);
        AppLibGraph_Show(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_4);
        AppLibGraph_Show(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_5);
        break;
    case GUI_SUR_COVER_HIDE:
        AppLibGraph_Hide(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_0);
        AppLibGraph_Hide(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_1);
        AppLibGraph_Hide(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_2);
        AppLibGraph_Hide(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_3);
        AppLibGraph_Hide(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_4);
        AppLibGraph_Hide(GRAPH_CH_FCHAN, GOBJ_SURROUND_RECT_5);
        break;
    case GUI_SUR_COVER_UPDATE:
        break;
    default:
        ReturnValue = -1;
        break;
    }

    return ReturnValue;
}
