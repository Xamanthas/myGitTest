/**
  * @file src/app/apps/flow/rec/connectedcam/rec_connected_cam_func.c
  *
  *  Functions of Sport Recorder (sensor) application
  *
  * History:
  *    2013/03/24 - [Annie Ting] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */


#include <apps/flow/rec/rec_connected_cam.h>
#include <system/status.h>
#include <system/app_pref.h>
#include <system/app_util.h>
#include <system/ApplibSys_Sensor.h>
#include <imgproc/AmbaImg_Impl_Cmd.h>
//#include <storage/ApplibStorage_Dmf.h>
#include <apps/flow/widget/menu/menu.h>
#include <apps/gui/resource/gui_settle.h>
#include <apps/flow/disp_param/disp_param_rec.h>
#include <AmbaROM.h>
#include <AmbaRTC.h>
#include <graphics/stamp/ApplibGraphics_Stamp.h>
#include <apps/flow/widget/menu/menu_video.h>
#include <framework/appmaintask.h>
#include <recorder/StillEnc.h>
#include <recorder/ApplibRecorder_ExtendEnc.h>
#include <FirmwareLoader.h>

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
#include <AmbaUtility.h>
#include <net/ApplibNet_JsonUtility.h>

static int NotifyNetFifoOfAppState(int State)
{
    int ReturnValue = 0;

    if (UserSetting->VideoPref.StreamType == STREAM_TYPE_RTSP) {
        ReturnValue = AppLibNetFifo_NotifyAppStateChange(State);
        if (ReturnValue == 0){
            if (State == AMP_NETFIFO_NOTIFY_STARTENC) {
                APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_NETFIFO_BUSY);
            }
        }
    }

    return ReturnValue;
}


/* return 0: system idle,  return 1: system busy */
static int rec_connected_cam_system_is_busy(void)
{
    int Status = 1;

    if((rec_connected_cam.RecCapState == REC_CAP_STATE_PREVIEW) &&
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
        (!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_NETFIFO_BUSY))&&
#endif
        (!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY)) ) {
        Status = 0;

        if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
            AmbaPrint("[%s] AppLibVideoEnc_EraseFifo",__func__);
            AppLibVideoEnc_EraseFifo();
            AppLibExtendEnc_UnInit();
        } else if (app_status.CurrEncMode == APP_STILL_ENC_MODE){
            AmbaPrint("[%s] AppLibStillEnc_EraseFifo",__func__);
            AppLibFormatMuxExif_End();
            AppLibStillEnc_EraseFifo();
        }
    }

    return Status;
}

static int SendJsonString(APPLIB_JSON_OBJECT *JsonObject)
{
    APPLIB_JSON_STRING *JsonString = NULL;
    char *ReplyString = NULL;

    AppLibNetJsonUtility_JsonObjectToJsonString(JsonObject, &JsonString);
    AppLibNetJsonUtility_GetString(JsonString, &ReplyString);
    if (ReplyString) {
        AppLibNetControl_ReplyToLnx(ReplyString, strlen(ReplyString));
    }
    AppLibNetJsonUtility_FreeJsonString(JsonString);

    return 0;
}

#else /* CONFIG_APP_CONNECTED_AMBA_LINK */

/* return 0: system idle,  return 1: system busy */
static int rec_connected_cam_system_is_busy(void)
{
    int Status = 1;

    if((rec_connected_cam.RecCapState == REC_CAP_STATE_PREVIEW) &&
        (!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY)) ) {
        Status = 0;

        if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
            AmbaPrint("[%s] AppLibVideoEnc_EraseFifo",__func__);
            AppLibVideoEnc_EraseFifo();
            AppLibExtendEnc_UnInit();
        } else if (app_status.CurrEncMode == APP_STILL_ENC_MODE){
            AmbaPrint("[%s] AppLibStillEnc_EraseFifo",__func__);
            AppLibFormatMuxExif_End();
            AppLibStillEnc_EraseFifo();
        }
    }

    return Status;
}

#endif /* CONFIG_APP_CONNECTED_AMBA_LINK */

#if defined(CONFIG_APP_CONNECTED_STAMP)
static UINT8 stampAreaId = 0;
static AMP_AREA_s stampArea = {0};  // for encodec
static AMP_AREA_s stampAreaSec = {0};  // for encodec
static AMP_AREA_s stampOsd = {0};   // for osd

static int rec_connected_cam_calculate_stamp(APP_PREF_TIME_STAMP_e stampType, UINT32 encWidth, UINT32 encHeight)
{
    UINT32 BlankLeavingW = 0, BlankLeavingH = 0;
    UINT32 StringHeight = 0;
    GUI_REC_CONNECTED_CAM_STAMP_UPDATE_INFO_s stampUpdateInfo = {0};

    /* Initialize */
    BlankLeavingW = encWidth / 15;
    BlankLeavingH = encHeight / 15;
    StringHeight = 54;// encWidth / 20; // TBD
    //ModifiedStringW // TBD

    /* Date */
    if (stampType == STAMP_DATE) {
        UINT32 DateWidth = AppLibGraph_GetStringWidth(GRAPH_CH_BLEND, BOBJ_DATE_STREAM_0, StringHeight);

        stampUpdateInfo.GuiObjId = BOBJ_DATE_STREAM_0;
        stampUpdateInfo.Left = 0;
        stampUpdateInfo.Top = 230;
        stampUpdateInfo.Width = DateWidth;
        stampUpdateInfo.Height = StringHeight;
        rec_connected_cam.Gui(GUI_STAMP_UPDATE_SIZE, (UINT32)stampType, (UINT32)&stampUpdateInfo);

        /* Update position and size */
        stampArea.X = encWidth - BlankLeavingW - DateWidth;
        stampArea.Y = encHeight - BlankLeavingH - StringHeight;
        stampArea.Width = (DateWidth%32) ? (((DateWidth>>5)+1)<<5) : DateWidth;
        stampArea.Height = StringHeight;

        stampAreaSec.X = AppLibVideoEnc_GetSecStreamW() - BlankLeavingW - DateWidth;
        stampAreaSec.Y = AppLibVideoEnc_GetSecStreamH() - BlankLeavingH - StringHeight;
        stampAreaSec.Width = stampArea.Width;
        stampAreaSec.Height = stampArea.Height;

        stampOsd.X = 0;
        stampOsd.Y = 258;
        stampOsd.Width = stampArea.Width;
        stampOsd.Height = StringHeight;
    } else if (stampType == STAMP_TIME){
        UINT32 HrMinWidth = AppLibGraph_GetStringWidth(GRAPH_CH_BLEND, BOBJ_TIME_H_M_STREAM_0, StringHeight);
        UINT32 SecWidth = AppLibGraph_GetStringWidth(GRAPH_CH_BLEND, BOBJ_TIME_S_STREAM_0, StringHeight);
        UINT32 TotalWidth = 0;

        stampUpdateInfo.GuiObjId = BOBJ_TIME_H_M_STREAM_0;
        stampUpdateInfo.Left = 0;
        stampUpdateInfo.Top = 230;
        stampUpdateInfo.Width = HrMinWidth;
        stampUpdateInfo.Height = StringHeight;
        rec_connected_cam.Gui(GUI_STAMP_UPDATE_SIZE, (UINT32)stampType, (UINT32)&stampUpdateInfo);
        stampUpdateInfo.GuiObjId = BOBJ_TIME_S_STREAM_0;
        stampUpdateInfo.Left = HrMinWidth;
        stampUpdateInfo.Top = 230;
        stampUpdateInfo.Width = SecWidth;
        stampUpdateInfo.Height = StringHeight;
        rec_connected_cam.Gui(GUI_STAMP_UPDATE_SIZE, (UINT32)stampType, (UINT32)&stampUpdateInfo);

        /* Update position and size */
        stampArea.X = encWidth - BlankLeavingW - SecWidth - HrMinWidth;
        stampArea.Y = encHeight - BlankLeavingH - StringHeight;
        TotalWidth = HrMinWidth + SecWidth;
        stampArea.Width = (TotalWidth%32) ? (((TotalWidth>>5)+1)<<5) : TotalWidth;
        stampArea.Height = StringHeight;

        stampAreaSec.X = AppLibVideoEnc_GetSecStreamW() - BlankLeavingW - SecWidth - HrMinWidth;
        stampAreaSec.Y = AppLibVideoEnc_GetSecStreamH() - BlankLeavingH - StringHeight;
        stampAreaSec.Width = stampArea.Width;
        stampAreaSec.Height = stampArea.Height;

        stampOsd.X = 0;
        stampOsd.Y = 258;
        stampOsd.Width = stampArea.Width;
        stampOsd.Height = StringHeight;
    } else {
        UINT32 DateWidth = AppLibGraph_GetStringWidth(GRAPH_CH_BLEND, BOBJ_DATE_STREAM_0, StringHeight);
        UINT32 HrMinWidth = AppLibGraph_GetStringWidth(GRAPH_CH_BLEND, BOBJ_TIME_H_M_STREAM_0, StringHeight);
        UINT32 SecWidth = AppLibGraph_GetStringWidth(GRAPH_CH_BLEND, BOBJ_TIME_S_STREAM_0, StringHeight);
        UINT32 TotalWidhth = HrMinWidth + SecWidth;
        UINT32 MaxWidth = (DateWidth >= TotalWidhth) ? DateWidth : TotalWidhth;

        stampUpdateInfo.GuiObjId = BOBJ_DATE_STREAM_0;
        stampUpdateInfo.Left = 0;
        stampUpdateInfo.Top = 230;
        stampUpdateInfo.Width = DateWidth;
        stampUpdateInfo.Height = StringHeight;
        rec_connected_cam.Gui(GUI_STAMP_UPDATE_SIZE, stampType, (UINT32)&stampUpdateInfo);
        stampUpdateInfo.GuiObjId = BOBJ_TIME_H_M_STREAM_0;
        stampUpdateInfo.Left = MaxWidth - TotalWidhth;
        stampUpdateInfo.Top = 230 + StringHeight;
        stampUpdateInfo.Width = HrMinWidth;
        stampUpdateInfo.Height = StringHeight;
        rec_connected_cam.Gui(GUI_STAMP_UPDATE_SIZE, stampType, (UINT32)&stampUpdateInfo);
        stampUpdateInfo.GuiObjId = BOBJ_TIME_S_STREAM_0;
        stampUpdateInfo.Left = MaxWidth - SecWidth;
        stampUpdateInfo.Top = 230 + StringHeight;
        stampUpdateInfo.Width = SecWidth;
        stampUpdateInfo.Height = StringHeight;
        rec_connected_cam.Gui(GUI_STAMP_UPDATE_SIZE, stampType, (UINT32)&stampUpdateInfo);

        /* Update position and size */
        stampArea.X = encWidth - BlankLeavingW - MaxWidth;
        stampArea.Y = encHeight - BlankLeavingH - (StringHeight<<1);
        stampArea.Width = (MaxWidth%32) ? (((MaxWidth>>5)+1)<<5) : MaxWidth;
        stampArea.Height = (StringHeight<<1);

        stampAreaSec.X = AppLibVideoEnc_GetSecStreamW() - BlankLeavingW - MaxWidth;
        stampAreaSec.Y = AppLibVideoEnc_GetSecStreamH() - BlankLeavingH - (StringHeight<<1);
        stampAreaSec.Width = stampArea.Width;
        stampAreaSec.Height = stampArea.Height;

        stampOsd.X = 0;
        stampOsd.Y = 258;
        stampOsd.Width = stampArea.Width;
        stampOsd.Height = StringHeight<<1;
    }

    return 0;
}

static int rec_connected_cam_encode_stamp(UINT8 updateFlag)
{
    APPLIB_GRAPHIC_STAMP_BUF_CONFIG_s stampConfig = {0};
    /* Add a new stamp area*/
    if (app_status.CurrEncMode == APP_STILL_ENC_MODE) {
        stampAreaId = AppLibGraph_AddStampArea(stampOsd, ENCODE_FORMAT_YUV422);
        if(stampAreaId == 0xFF){
            return -1;
        }
    } else {
        if (!updateFlag) {
            stampAreaId = AppLibGraph_AddStampArea(stampOsd, ENCODE_FORMAT_YUV422);
            if(stampAreaId == 0xFF){
            return -1;
            }
        } else {
            AppLibGraph_UpdateStampArea(stampAreaId, stampOsd, ENCODE_FORMAT_YUV422);
        }
    }

    AppLibStamp_GetBlendBuf(stampAreaId, &stampConfig);

    /* Encode */
    if (app_status.CurrEncMode == APP_STILL_ENC_MODE) {
        APPLIB_GRAPHIC_STAMP_BUF_CONFIG_s stampConfig = {0};
        APPLIB_STILLENC_STAMP_SETTING_s stillStamp = {0};

        /* Blending */
        stampAreaId = AppLibGraph_AddStampArea(stampOsd, ENCODE_FORMAT_YUV422);
        if(stampAreaId == 0xFF){
            return -1;
        }
        AppLibStamp_GetBlendBuf(stampAreaId, &stampConfig);
        stampConfig.OffsetX = stampArea.X;
        stampConfig.OffsetY = stampArea.Y;
        memcpy((void*)&stillStamp, (void*)&stampConfig, sizeof(APPLIB_STILLENC_STAMP_SETTING_s));
        switch (UserSetting->PhotoPref.PhotoCapMode) {
            case PHOTO_CAP_MODE_PES:
                AppLibStillEnc_SingleContCapRegisterStampCB(stillStamp);
                break;
            case PHOTO_CAP_MODE_BURST:
                AppLibStillEnc_BurstCapRegisterStampCB(stillStamp);
                break;
            case PHOTO_CAP_MODE_PRECISE:
            default:
                AppLibStillEnc_SingleCapRegisterStampCB(stillStamp);
                break;
        }
    } else {
        AMP_VIDEOENC_BLEND_INFO_s BlendInfo;
        BlendInfo.Enable = 1;
        BlendInfo.BufferID = stampAreaId;  // Unit Test only use one blend area, we choose ID = 0
        BlendInfo.OffsetX = stampArea.X;
        BlendInfo.OffsetY = stampArea.Y;
        BlendInfo.Pitch = ((stampArea.Width%32) ? (((stampArea.Width>>5)+1)<<5) : (stampArea.Width));
        BlendInfo.Width = ((stampArea.Width%32) ? (((stampArea.Width>>5)+1)<<5) : (stampArea.Width));
        BlendInfo.Height = stampArea.Height;
        BlendInfo.YAddr = stampConfig.YAddr;
        BlendInfo.UVAddr = stampConfig.UVAddr;
        BlendInfo.AlphaYAddr = stampConfig.AlphaYAddr;
        BlendInfo.AlphaUVAddr = stampConfig.AlphaUVAddr;
        AppLibVideoEnc_EncodeStamp(0, &BlendInfo);
        if (AppLibVideoEnc_GetDualStreams() && AppLibFormat_GetDualFileSaving() ) {
            UINT8 HdrEnable;
            AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_HDR_ENABLE, 0, (UINT32)&HdrEnable, 0);

            BlendInfo.OffsetX = stampAreaSec.X;
            BlendInfo.OffsetY = stampAreaSec.Y;
            BlendInfo.Pitch = ((stampAreaSec.Width%32) ? (((stampAreaSec.Width>>5)+1)<<5) : (stampAreaSec.Width));
            BlendInfo.Width = ((stampAreaSec.Width%32) ? (((stampAreaSec.Width>>5)+1)<<5) : (stampAreaSec.Width));
            BlendInfo.Height = stampAreaSec.Height;
            if (HdrEnable) {
                /**Set stamp info at hybrid mode*/
                AppLibVideoEnc_EncodeStamp(1, &BlendInfo);
            }
        }
    }
    return 0;
}

static int rec_connected_cam_setup_stamp(void)
{
    int ReturnValue = 0;

    APP_PREF_TIME_STAMP_e TimeStampType = STAMP_OFF;
    AMBA_RTC_TIME_SPEC_u TimeSpec = {0};

    /* Function Check */
    if (app_status.CurrEncMode == APP_STILL_ENC_MODE) {
        TimeStampType = UserSetting->PhotoPref.PhotoTimeStamp;
    } else if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        TimeStampType = UserSetting->VideoPref.VideoDateTimeStamp;
    }
    if (TimeStampType == STAMP_OFF) {
        AmbaPrint("<rec_connected_cam_setup_stamp> stamp off!");
        return ReturnValue;
    }

    /* Initialize */
    rec_connected_cam.Gui(GUI_STAMP_SET_MODE, 0, 0);
    rec_connected_cam.Gui(GUI_STAMP_HIDE, TimeStampType, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    /* Get time setting */
    AmbaRTC_GetSystemTime(AMBA_TIME_STD_TAI, &TimeSpec);

    /* Update date */
    if ((TimeStampType == STAMP_DATE_TIME) || (TimeStampType == STAMP_DATE)) {
        GUI_REC_CONNECTED_CAM_STAMP_DATE_s stampDate = {0};
        stampDate.Year = (TimeSpec.Calendar.Year > 2044) ? 2014 : TimeSpec.Calendar.Year;
        stampDate.Month = TimeSpec.Calendar.Month;
        stampDate.Day = TimeSpec.Calendar.Day;
        rec_connected_cam.Gui(GUI_STAMP_UPDATE_DATE, (UINT32)&stampDate, 0);
    }

    /* Update time */
    if ((TimeStampType == STAMP_DATE_TIME) || (TimeStampType == STAMP_TIME)) {
        GUI_REC_CONNECTED_CAM_STAMP_TIME_s stampTime = {0};
        stampTime.Hour = TimeSpec.Calendar.Hour;
        stampTime.Minute = TimeSpec.Calendar.Minute;
        stampTime.Second = TimeSpec.Calendar.Second;
        rec_connected_cam.Gui(GUI_STAMP_UPDATE_TIME, (UINT32)&stampTime, 0);
    }

    /* Enable stamp on OSD buffer */
    rec_connected_cam.Gui(GUI_STAMP_SHOW, TimeStampType, 0);

    /* Modify position and size */
    if (app_status.CurrEncMode == APP_STILL_ENC_MODE) {
        APPLIB_SENSOR_STILLCAP_CONFIG_s *StillCapConfigData;
        StillCapConfigData = AppLibSysSensor_GetPjpegConfig(AppLibStillEnc_GetPhotoPjpegCapMode(), AppLibStillEnc_GetPhotoPjpegConfigId());
        rec_connected_cam_calculate_stamp(TimeStampType, StillCapConfigData->FullviewWidth, StillCapConfigData->FullviewHeight);
    } else {
        APPLIB_SENSOR_VIDEO_ENC_CONFIG_s *VideoEncConfigData;
        VideoEncConfigData = AppLibSysSensor_GetVideoConfig(AppLibVideoEnc_GetSensorVideoRes());
        rec_connected_cam_calculate_stamp(TimeStampType, VideoEncConfigData->EncodeWidth, VideoEncConfigData->EncodeHeight);
    }
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    /* Encode */
    rec_connected_cam_encode_stamp(0);

    return ReturnValue;
}

static int rec_connected_cam_update_stamp(void)
{
    int ReturnValue = 0;

    APP_PREF_TIME_STAMP_e TimeStampType = STAMP_OFF;
    AMBA_RTC_TIME_SPEC_u TimeSpec = {0};

    /* Function Check */
    if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        TimeStampType = UserSetting->VideoPref.VideoDateTimeStamp;
    }

    if (TimeStampType == STAMP_OFF) {
        AmbaPrint("<rec_connected_cam_update_stamp> stamp off!");
        return ReturnValue;
    }

    /* Get time setting */
    AmbaRTC_GetSystemTime(AMBA_TIME_STD_TAI, &TimeSpec);

    /* Update date */
    if ((TimeStampType == STAMP_DATE_TIME) || (TimeStampType == STAMP_DATE)) {
        GUI_REC_CONNECTED_CAM_STAMP_DATE_s stampDate = {0};
        stampDate.Year = (TimeSpec.Calendar.Year > 2044) ? 2014 : TimeSpec.Calendar.Year;
        stampDate.Month = TimeSpec.Calendar.Month;
        stampDate.Day = TimeSpec.Calendar.Day;
        rec_connected_cam.Gui(GUI_STAMP_UPDATE_DATE, (UINT32)&stampDate, 0);
    }

    /* Update time */
    if ((TimeStampType == STAMP_DATE_TIME) || (TimeStampType == STAMP_TIME)) {
        GUI_REC_CONNECTED_CAM_STAMP_TIME_s stampTime = {0};
        stampTime.Hour = TimeSpec.Calendar.Hour;
        stampTime.Minute = TimeSpec.Calendar.Minute;
        stampTime.Second = TimeSpec.Calendar.Second;
        rec_connected_cam.Gui(GUI_STAMP_UPDATE_TIME, (UINT32)&stampTime, 0);
    }

    /* Enable stamp on OSD buffer */
    rec_connected_cam.Gui(GUI_STAMP_SHOW, TimeStampType, 0);

    /* Modify position and size */
    if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        APPLIB_SENSOR_VIDEO_ENC_CONFIG_s *VideoEncConfigData;
        VideoEncConfigData = AppLibSysSensor_GetVideoConfig(AppLibVideoEnc_GetSensorVideoRes());
        rec_connected_cam_calculate_stamp(TimeStampType, VideoEncConfigData->EncodeWidth, VideoEncConfigData->EncodeHeight);
    }

    return 0;
}

static int rec_connected_cam_stop_stamp(void)
{
    int ReturnValue = 0;

    APP_PREF_TIME_STAMP_e TimeStampType = STAMP_OFF;
    AMP_VIDEOENC_BLEND_INFO_s BlendInfo;
    if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        TimeStampType = UserSetting->VideoPref.VideoDateTimeStamp;
    }

    /* Function Check */
    if (TimeStampType == STAMP_OFF) {
        AmbaPrint("<rec_connected_cam_stop_stamp> stamp off!");
        return ReturnValue;
    }

    BlendInfo.Enable = 0;
    BlendInfo.BufferID = stampAreaId;  // Unit Test only use one blend area, we choose ID = 0
    AppLibVideoEnc_EncodeStamp(0, &BlendInfo);
    if (AppLibVideoEnc_GetDualStreams() && AppLibFormat_GetDualFileSaving() ) {
        UINT8 HdrEnable;
        AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_HDR_ENABLE, 0, (UINT32)&HdrEnable, 0);
        if (HdrEnable) {
            AppLibVideoEnc_EncodeStamp(1, &BlendInfo);
        }
    }

    rec_connected_cam.Gui(GUI_STAMP_HIDE, TimeStampType, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
    return 0;
}
#endif

static int rec_connected_cam_init(void)
{
    int ReturnValue = 0;

    /** Initialize the vin. */
    AppLibSysVin_Init();

    /** Initialize the video recorder. */
    AppLibVideoEnc_Init();

    /** Initialize the still recorder. */
    AppLibStillEnc_Init();

    /** Initialize the audio recorder. */
    AppLibAudioEnc_Init();

    /** Initialize the extend recorder. */
    AppLibExtendEnc_Init();

    rec_connected_cam.QuickViewFileType = MEDIA_TYPE_UNKNOWN;
    rec_connected_cam.RecCapState = REC_CAP_STATE_RESET;
    #if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    NotifyNetFifoOfAppState(AMP_NETFIFO_NOTIFY_RELEASE);
    #endif

    return ReturnValue;
}

static int rec_connected_cam_muxer_reach_limit_eventrecord(int param1)
{
    int ReturnValue = 0;
    AppLibFormatMuxMp4_Event_End();
    return ReturnValue;
}

static int rec_connected_cam_eventrecord_start(void)
{
    int ReturnValue = 0;
    int PreEventTime = 10;//to set how many seconds before event to record
    AppLibFormat_SetSplit_EventRecord(30000);//30 secs
    if (AppLibFormat_GetEventStatus()==0){
        rec_connected_cam.MuxerNum ++;
    }
    AppLibFormatMuxMp4_StartOnRecording_EventRecord(PreEventTime);
    return ReturnValue;
}

static int rec_connected_cam_start(void)
{
    int ReturnValue = 0;

    UserSetting->SystemPref.SystemMode = APP_MODE_ENC;

    /** Set menus */
    AppMenu_Reset();
    AppMenu_RegisterTab(MENU_SETUP);
    //AppMenu_RegisterTab(MENU_IMAGE);
    AppMenu_RegisterTab(MENU_PHOTO);
    AppMenu_RegisterTab(MENU_VIDEO);

    #if defined (CONFIG_APP_CONNECTED_AMBA_LINK)
    //AppMenu_EnableItem(MENU_VIDEO, MENU_VIDEO_STREAMS_TYPE);
    #endif

    #ifdef APP_ADAS
    AppLibVideoEnc_SetSecStreamW(576);
    AppLibVideoEnc_SetSecStreamH(320);
    #endif

    // ToDo: need to remove to handler when iav completes the dsp cmd queue mechanism
    AppLibGraph_Init();

    /** Start the liveview. */
    if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        /**Calculate second stream timescale by main stream time scale*/
        APPLIB_SENSOR_VIDEO_ENC_CONFIG_s *VideoEncConfigData;
        VideoEncConfigData = AppLibSysSensor_GetVideoConfig(AppLibVideoEnc_GetSensorVideoRes());
        if ((VideoEncConfigData->EncNumerator % 25000) ==0) {
            AppLibVideoEnc_SetSecStreamTimeScale(25000);
            AppLibVideoEnc_SetSecStreamTick(1000);
        } else {
            AppLibVideoEnc_SetSecStreamTimeScale(30000);
            AppLibVideoEnc_SetSecStreamTick(1001);
        }

        /* Video preview. */
        AppLibVideoEnc_LiveViewSetup();
        rec_connected_cam.Func(REC_CONNECTED_CAM_CHANGE_DISPLAY, 0, 0);
        AppLibVideoEnc_LiveViewStart();
    } else {
        /* Photo preview. */
        AppLibStillEnc_LiveViewSetup();
        rec_connected_cam.Func(REC_CONNECTED_CAM_CHANGE_DISPLAY, 0, 0);
        AppLibStillEnc_LiveViewStart();
    }

    /** set free space threshold*/
    AppLibCard_SetThreahold(FREESPACE_THRESHOLD); /**<set card check threshold*/
    AppLibMonitorStorage_SetThreshold(FREESPACE_THRESHOLD);/**<set storage monitor threshold*/
    /** inital storage async op task*/
    AppLibStorageAsyncOp_Init();

    /*register loop enc handler*/
    AppLibLoopEnc_Init();

    /** init frame handler task for ADAS */
    AppLibVideoAnal_FrmHdlr_Init();
    AppLibVideoAnal_TriAHdlr_Init();

    /** set split setting to 60 sec.*/
    AppLibVideoEnc_SetSplit(VIDEO_SPLIT_TIME_60_SECONDS);

    return ReturnValue;
}

static int rec_connected_cam_stop(void)
{
    int ReturnValue = 0;

    if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_SELFTIMER_RUN)) {
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SELFTIMER_STOP, 0, 0);
    }

    /* Stop the warning message, because the warning could need to be updated. */
    rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_STOP, 0, 0);

    APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_ERROR);

    /** Stop the liveview. */
    if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        AppLibVideoEnc_LiveViewStop();
    } else {
        AppLibStillEnc_LiveViewStop();
        AppLibStillEnc_LiveViewDeInit();
    }

    /* Close the menu or dialog. */
    AppWidget_Off(WIDGET_ALL, WIDGET_HIDE_SILENT);
    APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_POPUP);

    /* Disable the vout. */
    AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
    AppLibDisp_DeactivateWindow(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0));
    AppLibDisp_ChanStop(DISP_CH_FCHAN);

    /** Hide GUI */
    rec_connected_cam.Gui(GUI_HIDE_ALL, 0, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_do_liveview_state(void)
{
    int ReturnValue = 0;
#ifdef CONFIG_APP_CONNECTED_PARTIAL_LOAD
    static int PartialLoadRunOnce = 0;
#endif
    rec_connected_cam.RecCapState = REC_CAP_STATE_PREVIEW;
    if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
        /* Application is ready. */
        if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY)) {
            /* Application is busy. */
            if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_STILL_CAPTURE)) {
                /* Capture complete. */
                APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_STILL_CAPTURE);
                /* The application is busy, if the muxer is not idle mode.*/
                if (!rec_connected_cam_system_is_busy()) {
                    APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);
                    APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_MENU);

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
                    if (APP_CHECKFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_FROM_NETCTRL)) {
                        rec_connected_cam.Func(REC_CONNECTED_CAM_NETCTRL_CAPTURE_DONE, 0, 0);
                    }

                    if (APP_CHECKFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF)) {
                        DBGMSGc2(CYAN,"[rec_connected_cam] <do_liveview_state> clear REC_CAR_DV_FLAGS_CAPTURE_ON_VF");
                        APP_REMOVEFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF);
                        app_status.CurrEncMode = APP_VIDEO_ENC_MODE;
                    }
#endif
                    /* To excute the functions that system block them when the Busy flag is enabled. */
                    AppUtil_BusyCheck(0);
                    if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
                        return ReturnValue;/**<  App switched out*/
                    }
                }
            } else {
                /* Stop recording clip. */
                if (!rec_connected_cam_system_is_busy()) {
                    APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
                    if (APP_CHECKFLAGS(rec_connected_cam.NetCtrlFlags, APP_NETCTRL_FLAGS_VF_STOP_DONE)) {
                        APP_REMOVEFLAGS(rec_connected_cam.NetCtrlFlags, APP_NETCTRL_FLAGS_VF_STOP_DONE);
                        AppLibNetControl_ReplyErrorCode(AMBA_STOP_VF, 0);
                    }
#endif
                    /* To excute the functions that system block them when the Busy flag is enabled. */
                    AppUtil_BusyCheck(0);
                    /**Resend usb connect is usb connect is blocked by recording*/
                    if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_USB)) {
                        APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_USB);
                        AppLibComSvcHcmgr_SendMsg(HMSG_USB_DETECT_CONNECT, 0, 0);
                    }
                    if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
                        return ReturnValue;/**<  App switched out*/
                    }
                }
            }
        }
    } else {
#ifdef CONFIG_APP_CONNECTED_PARTIAL_LOAD
        if ((PartialLoadRunOnce == 0) && (AmpFwLoader_GetEnableSuspend())) {
            extern int AppHandler_AppPostInit(void);

            PartialLoadRunOnce++;
            // Resume the firmware partial loader.
            AmpFwLoader_Resume();
            AppHandler_AppPostInit();
        }
#endif
        APP_ADDFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY);
        /* Enable Anti flicker. */
        AppLibImage_EnableAntiFlicker(1);

        rec_connected_cam.Func(REC_CONNECTED_CAM_CHANGE_OSD, 0, 0);

        /* To excute the functions that system block them when the Ready flag is not enabled. */
        AppUtil_ReadyCheck(0);
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            return ReturnValue;/**<  App switched out*/
        }
        /* To show the gui of current application. */
        rec_connected_cam.Func(REC_CONNECTED_CAM_GUI_INIT_SHOW, 0, 0);
    }
    rec_connected_cam.Func(REC_CONNECTED_CAM_LIVEVIEW_POST_ACTION, 0, 0);

    return ReturnValue;
}

/**
 *  @brief do actions which need to be call at liveview state
 *  call auto record if encode mode is video
 *
 *  @return =0 success
 */
static int rec_connected_cam_liveview_post_action(void)
{
    int ReturnValue = 0;

    rec_connected_cam.Func(REC_CONNECTED_CAM_ADAS_FUNCTION_INIT, 0, 0);

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    if (AppLibNetBase_GetBootStatus() == 0) {
        //DBGMSGc2(GREEN,"[rec_connected_cam] <liveview_post_action> AMBA Link not booted yet!");
        return 0;
    }

    if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY)) {
        return 0;
    }

    if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF)) {
            DBGMSGc2(CYAN, "[rec_connected_cam] <liveview_post_action> REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF");
            /**Wait photo preview to enter preview state*/
            rec_connected_cam.RecCapState = REC_CAP_STATE_RESET;
            AppLibVideoEnc_LiveViewStop();
            AppLibStillEnc_LiveViewSetup();
            AppLibStillEnc_LiveViewStart();
            app_status.CurrEncMode = APP_STILL_ENC_MODE;
        } else {
            if (UserSetting->VideoPref.StreamType == STREAM_TYPE_RTSP) {
                DBGMSGc2(GREEN, "[rec_car_dv] <liveview_post_action> REC_CAR_DV_VF_START");
                rec_connected_cam.Func(REC_CONNECTED_CAM_VF_START, 0, 0);
            }
        }
    } else {
        /* APP_STILL_ENC_MODE */
        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF)) {
            DBGMSGc2(CYAN, "[rec_connected_cam] <liveview_post_action> REC_CONNECTED_CAM_CAPTURE");
            rec_connected_cam.Func(REC_CONNECTED_CAM_CAPTURE, 0, 0);
        }
    }

#else
    if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        if (!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY)) {
            rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_AUTO_START, 0, 0);
        }
    }
#endif /* CONFIG_APP_CONNECTED_AMBA_LINK */

    return ReturnValue;
}

/**
 *  @brief The timer handler of selftimer
 *
 *  To countdown and show the gui.
 *
 *  @param [in] eid Event id
 *
 *  @return >=0 success, <0 failure
 */
static void rec_connected_cam_selftimer_handler(int eid)
{
    if (eid == TIMER_UNREGISTER) {
        return;
    }

    if (--rec_connected_cam.SelfTimerTime > 0) {
        rec_connected_cam.Gui(GUI_SELFTIMER_COUNTDOWN_UPDATE, rec_connected_cam.SelfTimerTime, 0);
        rec_connected_cam.Gui(GUI_SELFTIMER_COUNTDOWN_SHOW, 0, 0);
    } else {
        AppLibComSvcTimer_Unregister(TIMER_1HZ, rec_connected_cam_selftimer_handler);
        rec_connected_cam.Gui(GUI_SELFTIMER_COUNTDOWN_HIDE, 0, 0);
        if (rec_connected_cam.SelfTimerType == SELF_TIMER_TYPE_PHOTO) {
            rec_connected_cam.Func(REC_CONNECTED_CAM_CAPTURE, 0, 0);
        } else if (rec_connected_cam.SelfTimerType == SELF_TIMER_TYPE_VIDEO) {
            rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_START, 0, 0);
        }
    }
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
}


static int rec_connected_cam_selftimer(int enable, int param1, int param2)
{
    int ReturnValue = 0;

    if (enable) {
        rec_connected_cam.SelfTimerTime = param1;
        rec_connected_cam.SelfTimerType = param2;
        APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_SELFTIMER_RUN);

        /* Register the timer for self timer. */
        AppLibComSvcTimer_Register(TIMER_1HZ, rec_connected_cam_selftimer_handler);

        /* Update the gui. */
        rec_connected_cam.Gui(GUI_SELFTIMER_COUNTDOWN_UPDATE, rec_connected_cam.SelfTimerTime, 0);
        rec_connected_cam.Gui(GUI_SELFTIMER_COUNTDOWN_SHOW, 0, 0);
        rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
    } else {
        if(APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_SELFTIMER_RUN)) {
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_SELFTIMER_RUN);
            /* Unregister the timer for self timer. */
            AppLibComSvcTimer_Unregister(TIMER_1HZ, rec_connected_cam_selftimer_handler);

            /* Update the gui. */
            rec_connected_cam.Gui(GUI_SELFTIMER_COUNTDOWN_HIDE, 0, 0);
            rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
        }
    }
    return ReturnValue;
}

#define Single_JPEG_SIZE    (5*1024)    /**< 5MB */
static UINT32  SingleCapContCount = 0;/**<Count single capture number for 3A*/
/**
 *  @brief The timer handler that can show the record time.
 *
 *  The timer handler that can show the record time.
 *
 *  @param[in] eid timer id.
 */
static void rec_connected_cam_capture_cont_timer_handler(int eid)
{
    static int CapCount = 0;
    static UINT32 Cap_num = 0;
    int PhotoAmount = 0;

    if (eid == TIMER_UNREGISTER) {
        return;
    }
    PhotoAmount = AppLibStorageDmf_GetFileAmount(APPLIB_DCF_MEDIA_IMAGE,DCIM_HDLR) + AppLibStillEnc_GetCaptureNum() + \
        rec_connected_cam.MuxerNum;
    if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_OPEN)) {
        /**if muxer open flag is on, represent there is a dcf object is created but file is writing.
             The fileamount function and muxernum both count this file, so minus one to correct the file amount*/
        PhotoAmount --;
    }
    if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_SHUTTER_PRESSED) &&
        (AppLibCard_GetFreeSpace(AppLibCard_GetActiveDrive()) > (rec_connected_cam.MuxerNum * Single_JPEG_SIZE))
        && PhotoAmount <= MAX_PHOTO_COUNT) {
        if ( CapCount == (UserSetting->PhotoPref.TimeLapse - 1)) {
            if (!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_BG_PROCESS)) {
                /* Enable stamp */
#if defined(CONFIG_APP_CONNECTED_STAMP)
                rec_connected_cam_setup_stamp();
#endif
                AppLibStillEnc_CaptureSingleCont();
                APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_BG_PROCESS);
                Cap_num ++;
                DBGMSGc2(GREEN, "[app_rec_connected_cam] cont_cap: %d", Cap_num);
                rec_connected_cam.MuxerNum ++;
            } else {
                APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_PES_DELAY);
                AppLibComSvcTimer_Unregister(TIMER_2HZ, rec_connected_cam_capture_cont_timer_handler);
                Cap_num = 0;
                AmbaPrintColor(GREEN, "[app_rec_connected_cam] Previous capture not finish yet, wait until it finish");
            }
            CapCount = 0;
        } else {
            CapCount++;
        }
    } else {
        if (PhotoAmount > MAX_PHOTO_COUNT) {
            rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_PHOTO_LIMIT, 0);
            AmbaPrintColor(CYAN,"[rec_connected_cam] Timer handler: Photo count reach limit, can not do capture (%d)",PhotoAmount-1);
        }
        AppLibComSvcTimer_Unregister(TIMER_2HZ, rec_connected_cam_capture_cont_timer_handler);
        DBGMSGc2(GREEN, "[app_rec_connected_cam] Stop cont_cap: %d", Cap_num);
        Cap_num = 0;
        CapCount = 0;
        if (!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_BG_PROCESS)) {
            SingleCapContCount = 0;/**<reset capture count*/
            AppLibImage_UnLock3A();
            AmpStillEnc_EnableLiveviewCapture(AMP_STILL_STOP_LIVEVIEW, 0);  // Still codec return to idle state
            AppLibStillEnc_SingleCapContFreeBuf();/**<free buff*/
            /*Start the liveview after stoping capturing photo.*/
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
            if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF)) {
                AppLibStillEnc_LiveViewDeInit();
                AppLibVideoEnc_LiveViewSetup();
                AppLibVideoEnc_LiveViewStart();
            } else
#endif
            {
                AppLibStillEnc_LiveViewSetup();
                AppLibStillEnc_LiveViewStart();
            }
        } else {
            APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_PES_DELAY);
        }
    }
}

static int rec_connected_cam_capture(void)
{
    int ReturnValue = 0;

    if (app_status.CurrEncMode != APP_STILL_ENC_MODE) {
        DBGMSG("[rec_connected_cam] <capture> It is the photo preview mode now.");
        return -1;
    }
    DBGMSGc2(GREEN,"[rec_connected_cam] <capture> REC_CONNECTED_CAM_CAPTURE");
    if (rec_connected_cam.RecCapState != REC_CAP_STATE_PREVIEW)
        return ReturnValue;

    if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY)) {
        AmbaPrintColor(GREEN,"[app_rec_connected_cam] Capture block due to previous capture is not finish");
        return ReturnValue;
    }

    /* Initialize and start the muxer. */
    AppLibFormat_MuxerInit();
    AppLibFormatMuxExif_Start();
    AppLibImage_Lock3A();

    /* Enable stamp */
    #if defined(CONFIG_APP_CONNECTED_STAMP)
    rec_connected_cam_setup_stamp();
    #endif
    switch (UserSetting->PhotoPref.PhotoCapMode) {
    case PHOTO_CAP_MODE_PES:
        DBGMSGc2(GREEN,"[app_rec_connected_cam] CAPTURE: PHOTO_CAP_MODE_PES");
        if (UserSetting->PhotoPref.TimeLapse != PHOTO_TIME_LAPSE_OFF) {
            AppLibComSvcTimer_Register(TIMER_2HZ, rec_connected_cam_capture_cont_timer_handler);
        }
        AppLibStillEnc_CaptureSingleCont();
        rec_connected_cam.MuxerNum += AppLibStillEnc_GetCaptureNum();
        break;
    case PHOTO_CAP_MODE_BURST:
        DBGMSGc2(GREEN,"[app_rec_connected_cam] CAPTURE: PHOTO_CAP_MODE_BURST");
        AppLibStillEnc_CaptureBurst();
        rec_connected_cam.MuxerNum += AppLibStillEnc_GetCaptureNum();
        break;
    case PHOTO_CAP_MODE_PRECISE:
    default:
        DBGMSGc2(GREEN,"[app_rec_connected_cam] CAPTURE: PHOTO_CAP_MODE_PRECISE");
        AppLibStillEnc_CaptureSingle();
        rec_connected_cam.MuxerNum += AppLibStillEnc_GetCaptureNum();
        break;
    }
    APP_ADDFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);
    APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY);
    APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_STILL_CAPTURE);
    APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_BG_PROCESS);
    rec_connected_cam.RecCapState = REC_CAP_STATE_CAPTURE;

    return ReturnValue;
}

static int rec_connected_cam_capture_piv(void)
{
    int ReturnValue = 0;
     /* Initialize and start the muxer. */
    AppLibFormat_MuxerInit();
    AppLibVideoEnc_PIVInit();
    if (!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_PIV)) {
        AppLibFormatMuxExifPIV_Start();//Testing
        ReturnValue = AppLibVideoEnc_CapturePIV();
        rec_connected_cam.MuxerNum += 1;
    } else {
        AmbaPrintColor(GREEN,"[app_rec_connected_cam] PIV Capture block due to previous capture is not finish");
        return -1;
    }
    APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_BG_PROCESS);
    APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_PIV);

    return ReturnValue;
}

static int rec_connected_cam_capture_complete(void)
{
    int ReturnValue = 0;

    switch (UserSetting->PhotoPref.PhotoCapMode) {
    case PHOTO_CAP_MODE_PES:
        AmbaPrintColor(GREEN,"[app_rec_connected_cam] CAPTURE_COMPLETE: PHOTO_CAP_MODE_PES");
        break;
    case PHOTO_CAP_MODE_BURST:
        AmbaPrintColor(GREEN,"[app_rec_connected_cam] CAPTURE_COMPLETE: PHOTO_CAP_MODE_BURST");
        break;
    case PHOTO_CAP_MODE_PRECISE:
    default:
        AmbaPrintColor(GREEN,"[app_rec_connected_cam] CAPTURE_COMPLETE: PHOTO_CAP_MODE_PRECISE");
        break;
    }

    return ReturnValue;
}

static int rec_connected_cam_capture_bg_process_done(void)
{
    int ReturnValue = 0;
    int PhotoAmount = 0;
    APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_BG_PROCESS);

    if (app_status.CurrEncMode != APP_STILL_ENC_MODE) {
        /** if not in still mode, do not start liveview*/
        return ReturnValue;
    }
    PhotoAmount = AppLibStorageDmf_GetFileAmount(APPLIB_DCF_MEDIA_IMAGE,DCIM_HDLR) + AppLibStillEnc_GetCaptureNum() + \
        rec_connected_cam.MuxerNum;

    if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_OPEN)) {
        /**if muxer open flag is on, represent there is a dcf object is created but file is writing.
             The fileamount function and muxernum both count this file, so minus one to correct the file amount*/
        PhotoAmount --;
    }

    switch (UserSetting->PhotoPref.PhotoCapMode) {
    case PHOTO_CAP_MODE_PES:
        AmbaPrintColor(GREEN,"[app_rec_connected_cam] BG_PROCESS_DONE: PHOTO_CAP_MODE_PES");
        /**Restart liveview when timer unregister to avoid the bg prosess done come before shutter release will not restart live view*/
        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_SHUTTER_PRESSED) && PhotoAmount <= MAX_PHOTO_COUNT) {
            SingleCapContCount++;
            AppLibStillEnc_SetShotCount(SingleCapContCount);
            /* Enable stamp */
#if defined(CONFIG_APP_CONNECTED_STAMP)
            rec_connected_cam_setup_stamp();
#endif
            if (UserSetting->PhotoPref.TimeLapse == PHOTO_TIME_LAPSE_OFF) {
                AppLibStillEnc_CaptureSingleCont();
                APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_BG_PROCESS);
                rec_connected_cam.MuxerNum ++;
            } else if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_PES_DELAY)){
                AppLibComSvcTimer_Register(TIMER_2HZ, rec_connected_cam_capture_cont_timer_handler);
                AppLibStillEnc_CaptureSingleCont();
                APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_BG_PROCESS);
                APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_PES_DELAY);
                rec_connected_cam.MuxerNum ++;
            }
        } else if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_PES_DELAY) || UserSetting->PhotoPref.TimeLapse == PHOTO_TIME_LAPSE_OFF) {
            SingleCapContCount = 0;/**<reset capture count*/
            AppLibImage_UnLock3A();
            AmpStillEnc_EnableLiveviewCapture(AMP_STILL_STOP_LIVEVIEW, 0);  // Still codec return to idle state
            AppLibStillEnc_SingleCapContFreeBuf();/**<free buff*/
            /*Start the liveview after stoping capturing photo.*/
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
            if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF)) {
                AppLibStillEnc_LiveViewDeInit();
                AppLibVideoEnc_LiveViewSetup();
                AppLibVideoEnc_LiveViewStart();
            } else
#endif
            {
                AppLibStillEnc_LiveViewSetup();
                AppLibStillEnc_LiveViewStart();
            }
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_PES_DELAY);
        } else if (PhotoAmount >= MAX_PHOTO_COUNT) {
            rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_PHOTO_LIMIT, 0);
            AmbaPrintColor(CYAN,"[rec_connected_cam] BG_PROCESS_DONE: Photo count reach limit, can not do capture (%d)",PhotoAmount-1);
        }
        break;
    case PHOTO_CAP_MODE_BURST:
        AmbaPrintColor(GREEN,"[app_rec_connected_cam] BG_PROCESS_DONE: PHOTO_CAP_MODE_BURST");
        AppLibImage_UnLock3A();
        AmpStillEnc_EnableLiveviewCapture(AMP_STILL_STOP_LIVEVIEW, 0);  // Still codec return to idle state
        AppLibStillEnc_BurstCapFreeBuf();/**<free buff*/
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF)) {
            AppLibStillEnc_LiveViewDeInit();
            AppLibVideoEnc_LiveViewSetup();
            AppLibVideoEnc_LiveViewStart();
        } else
#endif
        {
            AppLibStillEnc_LiveViewSetup();
            AppLibStillEnc_LiveViewStart();
        }
        break;
    case PHOTO_CAP_MODE_PRECISE:
    default:
        AmbaPrintColor(GREEN,"[app_rec_connected_cam] BG_PROCESS_DONE: PHOTO_CAP_MODE_PRECISE");
        AppLibImage_UnLock3A();
        AmpStillEnc_EnableLiveviewCapture(AMP_STILL_STOP_LIVEVIEW, 0);  // Still codec return to idle state
        AppLibStillEnc_SingleCapFreeBuf();/**<free buff*/
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF)) {
            AppLibStillEnc_LiveViewDeInit();
            AppLibVideoEnc_LiveViewSetup();
            AppLibVideoEnc_LiveViewStart();
        } else
#endif
        {
            AppLibStillEnc_LiveViewSetup();
            AppLibStillEnc_LiveViewStart();
        }
        break;
    }

    return ReturnValue;
}

static void rec_connected_cam_rec_check_event_folder(void)
{
    int number = AppLibStorageDmf_GetFileAmount(APPLIB_DCF_MEDIA_VIDEO, EVENTRECORD_HDLR);
    if (number>10){
        rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_FULL_HANDLE_EVENT, 0, 0);
    }
}

/**
 *  @brief The timer handler that can show the record time.
 *
 *  The timer handler that can show the record time.
 *
 *  @param[in] eid timer id.
 *
 */
static void rec_connected_cam_rec_timer_handler(int eid)
{
    if (eid == TIMER_UNREGISTER) {
        return;
    }

    if (rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) {
        rec_connected_cam.RecTime++;
        if (AppLibVideoEnc_GetTimeLapse() == VIDEO_TIME_LAPSE_OFF) {
            rec_connected_cam.Gui(GUI_REC_TIMER_UPDATE, rec_connected_cam.RecTime, 0);
        } else {
            rec_connected_cam.TimeLapseTime--;
            if (rec_connected_cam.TimeLapseTime == 0) {
                AppLibVideoEnc_EncodeTimeLapse();
                rec_connected_cam.TimeLapseTime = AppLibVideoEnc_GetTimeLapse();
                rec_connected_cam.Gui(GUI_REC_TIMER_UPDATE, rec_connected_cam.RecTime, 0);
            }
        }
        #if defined(CONFIG_APP_CONNECTED_STAMP)
        rec_connected_cam_update_stamp();
        #endif

        rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

        #if defined(CONFIG_APP_CONNECTED_STAMP)
        rec_connected_cam_encode_stamp(1);
        #endif
    }
    #if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    else if (rec_connected_cam.RecCapState == REC_CAP_STATE_VF) {
        rec_connected_cam.RecTime++;
        rec_connected_cam.Gui(GUI_REC_TIMER_UPDATE, rec_connected_cam.RecTime, 0);

        #if defined(CONFIG_APP_CONNECTED_STAMP)
        rec_connected_cam_update_stamp();
        #endif

        rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

        #if defined(CONFIG_APP_CONNECTED_STAMP)
        rec_connected_cam_encode_stamp(1);
        #endif
    }
    #endif
    if (rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) {
        rec_connected_cam_rec_check_event_folder();
    }
}

static int rec_connected_cam_record_start(void)
{
    int ReturnValue = 0;
    if (app_status.CurrEncMode != APP_VIDEO_ENC_MODE) {
        DBGMSG("[app_rec_connected_cam] It is the photo preview mode now.");
        return ReturnValue;
    }
    if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY))
        return ReturnValue;

    DBGMSG("[app_rec_connected_cam] REC_CONNECTED_CAM_RECORD_START");

    /* Set time lapse. */
    rec_connected_cam.TimeLapseTime = AppLibVideoEnc_GetTimeLapse();

    /* Register the timer to show the record time. */
    rec_connected_cam.RecTime = 0;
    AppLibComSvcTimer_Register(TIMER_1HZ, rec_connected_cam_rec_timer_handler);


    /* Setup the encode setting. */
    AppLibVideoEnc_EncodeSetup();

    /* Initialize and start the muxer. */
    AppLibFormat_MuxerInit();
    AppLibFormatMuxMp4_Start();
    AppLibFormatMuxMp4_SetAutoSplitFileType(1);/**set the auto split file type*/

    /* Start video encoding. */
    AppLibVideoEnc_EncodeStart();

    /* Enable the storage monitor.*/
    AppLibMonitorStorage_Enable(1);
    AppLibMonitorStorage_EnableMsg(1);

    /* Enable stamp */
    #if defined(CONFIG_APP_CONNECTED_STAMP)
    rec_connected_cam_setup_stamp();
    #endif

    if (UserSetting->VAPref.AdasDetection == ADAS_ON) {
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_ADAS_UPDATE_PARAM, 0, 0);
	    AppLibVideoAnal_FCMD_Enable();
        AppLibVideoAnal_LLWS_Enable();
        AppLibVideoAnal_ADAS_Enable();
        rec_connected_cam.Gui(GUI_ADAS_STAMP_UPDATE, 0, 0);
        rec_connected_cam.Gui(GUI_ADAS_STAMP_SHOW, 0, 0);
    }

    rec_connected_cam.RecCapState = REC_CAP_STATE_RECORD;
    #if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    NotifyNetFifoOfAppState(AMP_NETFIFO_NOTIFY_STARTENC);
    #endif

    APP_ADDFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);

    /* Update the gui. */
    rec_connected_cam.Gui(GUI_REC_STATE_UPDATE, GUI_REC_START, 0);
    rec_connected_cam.Gui(GUI_REC_STATE_SHOW, 0, 0);
    rec_connected_cam.Gui(GUI_REC_TIMER_UPDATE, rec_connected_cam.RecTime, 0);
    rec_connected_cam.Gui(GUI_REC_TIMER_SHOW, 0, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_record_pause(void)
{
    int ReturnValue = 0;

    /* Pause encoding */
    AppLibVideoEnc_EncodePause();
    APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_PAUSED);

    /* Stop the timer that show the gui of recording time because of recording pause. */
    AppLibComSvcTimer_Unregister(TIMER_1HZ, rec_connected_cam_rec_timer_handler);

    /* Update the gui. */
    rec_connected_cam.Gui(GUI_REC_STATE_UPDATE, GUI_REC_PAUSED, 0);
    rec_connected_cam.Gui(GUI_REC_TIMER_UPDATE, rec_connected_cam.RecTime, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_record_resume(void)
{
    int ReturnValue = 0;

    /* Resume encoding */
    AppLibVideoEnc_EncodeResume();
    APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_PAUSED);
    /* Start the timer that show the gui of recording time because the flow resume the recording. */
    AppLibComSvcTimer_Register(TIMER_1HZ, rec_connected_cam_rec_timer_handler);

    /* Update the gui. */
    rec_connected_cam.Gui(GUI_REC_STATE_UPDATE, GUI_REC_START, 0);
    rec_connected_cam.Gui(GUI_REC_TIMER_UPDATE, rec_connected_cam.RecTime, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_record_stop(void)
{
    int ReturnValue = 0;

    if ((rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) || (rec_connected_cam.RecCapState == REC_CAP_STATE_PRE_RECORD)) {
        DBGMSG("[app_rec_connected_cam] REC_CONNECTED_CAM_RECORD_STOP");
        if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_POPUP)) {
            AppWidget_Off(WIDGET_ALL, 0);
        }
        /* Stop the storage monitor.*/
        AppLibMonitorStorage_EnableMsg(0);
        AppLibMonitorStorage_Enable(0);

        #if defined(CONFIG_APP_CONNECTED_STAMP)
        rec_connected_cam_stop_stamp();
        #endif
		#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
        NotifyNetFifoOfAppState(AMP_NETFIFO_NOTIFY_SWITCHENCSESSION);
		#endif
        /* Stop encoding. */
        AppLibVideoEnc_EncodeStop();

        if (UserSetting->VAPref.AdasDetection == ADAS_ON) {
            AppLibVideoAnal_FCMD_Disable();
            AppLibVideoAnal_LLWS_Disable();
            AppLibVideoAnal_ADAS_Disable();
            rec_connected_cam.Gui(GUI_ADAS_STAMP_HIDE, 0, 0);
        }

        rec_connected_cam.RecCapState = REC_CAP_STATE_RESET;

        /* Stop the timer that show the gui of recording time because of stopping recording. */
        AppLibComSvcTimer_Unregister(TIMER_1HZ, rec_connected_cam_rec_timer_handler);
        /* Update the gui. */
        rec_connected_cam.Gui(GUI_REC_STATE_HIDE, 0, 0);
        rec_connected_cam.Gui(GUI_REC_TIMER_HIDE, 0, 0);
        rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
    }

    return ReturnValue;
}

/**
*  @brief:add auto record function to check card status and start record
*
*record stop after memory runout, mode change, menu open, the record should be auto restart after
*before start record, card status should be recheck
*
*  @return =0 success
*/
static int rec_connected_cam_record_auto_start(void)
{
    int ReturnValue = 0;
#if 0
    if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_POPUP)) {
        /* Don't start VF when menu is on. */
        DBGMSGc2(GREEN, "[rec_connected_cam] <record_auto_start> APP_AFLAGS_POPUP");
        return -1;
    }

    if (rec_connected_cam.RecCapState == REC_CAP_STATE_PREVIEW && app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        /** Check the card's status. */
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_CHECK_STATUS, 0, 0);
        if (ReturnValue == 0) {
            /** To record the clip if the card is ready. */
            rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_START, 0, 0);
        }
    }
#endif
    return ReturnValue;
}
static int rec_connected_cam_muxer_open(APPLIB_MUXER_TYPE_ID_e MuxerType)
{
    int ReturnValue = 0;

    switch(MuxerType) {
        case APPLIB_MUXER_TYPE_MP4:
            /**video muxer open add muxer busy flag and turn on unsaving data flag*/
            APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY);
            APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_OPEN);
            APP_ADDFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_IO);
            UserSetting->VideoPref.UnsavingData = 1;
            AppPref_Save();
            break;
        case APPLIB_MUXER_TYPE_EXIF:
            /**exif muxer open add muxer open flag for photo amount counting*/
            APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_OPEN);
            break;
        default:
            break;
    }

    return ReturnValue;
}

static int rec_connected_cam_muxer_end(APPLIB_MUXER_TYPE_ID_e MuxerType)
{
    int ReturnValue = 0;

    switch (MuxerType) {
        case APPLIB_MUXER_TYPE_MP4:
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_OPEN);
            APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_IO);
            /* Close the mp4 muxer. */
            AppLibFormatMuxMp4_Close();
            /* Clear the movie recovery flag. */
            if (!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_ERROR)) {
                APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_ERROR);
                UserSetting->VideoPref.UnsavingData = 0;
                AppPref_Save();
            }
            /* Remove the flag.*/
            if (rec_connected_cam.MuxerNum == 0) {
                APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY);
            }
            if (!rec_connected_cam_system_is_busy()) {
                /* The system should be idle if the muxer is idle and the state is preview state. */
                APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);

                /* To excute the functions that system block them when the Busy flag is enabled. */
                AppUtil_BusyCheck(0);
                /**Resend usb connect if usb connect is blocked by recording*/
                if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_USB)) {
                    APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_USB);
                    AppLibComSvcHcmgr_SendMsg(HMSG_USB_DETECT_CONNECT, 0, 0);
                }
                if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
                    return ReturnValue;/**<  App switched out and break current application's flow. */
                }

                #if defined (CONFIG_APP_CONNECTED_AMBA_LINK)
                if (UserSetting->VideoPref.StreamType == STREAM_TYPE_RTSP) {
                    DBGMSGc2(GREEN, "[rec_connected_cam] <muxer_end> REC_CONNECTED_CAM_VF_START");
                    rec_connected_cam.Func(REC_CONNECTED_CAM_VF_START, 0, 0);
                }
                #else
                rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_AUTO_START, 0, 0);
                #endif
            }
            break;
        case APPLIB_MUXER_TYPE_EVENT:
            AppLibFormatMuxMp4_Close_EventRecord();
            rec_connected_cam.MuxerNum --;
            if ((rec_connected_cam.MuxerNum == 0)&&(!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_OPEN))) {
                APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY);
                if (!rec_connected_cam_system_is_busy()) {
                    /* The system should be idle if the muxer is idle and the state is preview state. */
                    APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);

                    /* To excute the functions that system block them when the Busy flag is enabled. */
                    AppUtil_BusyCheck(0);
                    if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
                        return ReturnValue;/**<  App switched out and break current application's flow. */
                    }

                    #if defined (CONFIG_APP_CONNECTED_AMBA_LINK)
                    if (UserSetting->VideoPref.StreamType == STREAM_TYPE_RTSP) {
                        DBGMSGc2(GREEN, "[rec_connected_cam] <muxer_end> REC_CONNECTED_CAM_VF_START");
                        rec_connected_cam.Func(REC_CONNECTED_CAM_VF_START, 0, 0);
                    }
                    #else
                    rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_AUTO_START, 0, 0);
                    #endif
                }
            }
            break;
        case APPLIB_MUXER_TYPE_EXIF:
            AppLibFormatMuxExif_Close();
            AppLibFormatMuxMgr_MuxEnd();
            rec_connected_cam.MuxerNum --;
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_OPEN);
            if (rec_connected_cam.MuxerNum == 0) {
                APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY);
                if (!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_STILL_CAPTURE)) {
                    if (!rec_connected_cam_system_is_busy()) {
                        /* The application is idle, if the muxer is idle after caputuring completed.*/
                        APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);
                        APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_MENU);

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
                        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_FROM_NETCTRL)) {
                            rec_connected_cam.Func(REC_CONNECTED_CAM_NETCTRL_CAPTURE_DONE, 0, 0);
                        }
                        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF)) {
                            APP_REMOVEFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF);
                            app_status.CurrEncMode = APP_VIDEO_ENC_MODE;
                            if (UserSetting->VideoPref.StreamType == STREAM_TYPE_RTSP) {
                                DBGMSGc2(GREEN, "[rec_connected_cam] <muxer_end> REC_CONNECTED_CAM_VF_START 2");
                                rec_connected_cam.Func(REC_CONNECTED_CAM_VF_START, 0, 0);
                            }
                        }
#endif

                        /* To excute the functions that system block them when the Busy flag is enabled. */
                        AppUtil_BusyCheck(0);
                        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
                            return ReturnValue;/**<  App switched out and break current application's flow. */
                        }
                    }
                }
            }
            break;
        case APPLIB_MUXER_TYPE_EXIF_PIV:
            /*PIV muxer end*/
            AppLibFormatMuxExifPIV_Close();
            AppLibFormatMuxMgr_MuxEnd();
            AppLibVideoEnc_PIVFreeBuf();
            rec_connected_cam.MuxerNum --;
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_OPEN);
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_CAPTURE_PIV);
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_MENU);
            #if defined (CONFIG_APP_CONNECTED_AMBA_LINK)
            if (APP_CHECKFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_FROM_NETCTRL)) {
                rec_connected_cam.Func(REC_CONNECTED_CAM_NETCTRL_PIV_DONE, 0, 0);
            }
            #endif
            DBGMSGc2(GREEN, "[rec_connected_cam] <muxer_end> PIV Done");
            break;
        default:
            break;
    }

    AmbaPrintColor(GREEN,"[app_rec_connected_cam] REC_CONNECTED_CAM_MUXER_END mux.num%d",rec_connected_cam.MuxerNum);

    return ReturnValue;
}

static int rec_connected_cam_muxer_reach_limit(int param1)
{
    int ReturnValue = 0;

    if (param1) {
        /* Reach the limitation of file, but the setting of split file is off. Stop recording. */
        if ((rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) || (rec_connected_cam.RecCapState == REC_CAP_STATE_PRE_RECORD)) {
            /* Stop recording if the audio and video data buffer is full. */
            rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_STOP, 0, 0);
        }
    }

    return ReturnValue;
}

static int rec_connected_cam_muxer_stream_error(void)
{
    int ReturnValue = 0;

    if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {


        if (rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) {
            rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_STOP, 0, 0);
        }

        /* Close the mp4 muxer. */
        AppLibFormatMuxMp4_StreamError();

        /* Remove the flag.*/
        APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY);
        if (!rec_connected_cam_system_is_busy()) {
            /* The system should be idle if the muxer is idle and the state is preview state. */
            APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);

            /* To excute the functions that system block them when the Busy flag is enabled. */
            AppUtil_BusyCheck(0);
            /**Resend usb connect if usb connect is blocked by recording*/
            if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_USB)) {
                APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_USB);
                AppLibComSvcHcmgr_SendMsg(HMSG_USB_DETECT_CONNECT, 0, 0);
            }
            if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
                return ReturnValue;/**<  App switched out and break current application's flow. */
            }

#if defined (CONFIG_APP_CONNECTED_AMBA_LINK)
            if (UserSetting->VideoPref.StreamType == STREAM_TYPE_RTSP) {
                DBGMSGc2(GREEN, "[rec_connected_cam] <muxer_end> REC_CONNECTED_CAM_VF_START");
                rec_connected_cam.Func(REC_CONNECTED_CAM_VF_START, 0, 0);
            }
#else
                rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_AUTO_START, 0, 0);
#endif

        }
    } else {
        rec_connected_cam.MuxerNum --;
        if (rec_connected_cam.MuxerNum == 0) {
            /* The system will close the EXIF muxer automatically. The application flow does not need to close it. */
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY);
            if (!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_STILL_CAPTURE)) {

                if (!rec_connected_cam_system_is_busy()) {
                    /* The application is idle, if the muxer is idle after caputuring completed.*/
                    APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);
                    APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_MENU);

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
                    if (APP_CHECKFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF)) {
                        APP_REMOVEFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF);
                        app_status.CurrEncMode = APP_VIDEO_ENC_MODE;
                        if (UserSetting->VideoPref.StreamType == STREAM_TYPE_RTSP) {
                            DBGMSGc2(GREEN, "[rec_connected_cam] <muxer_end> REC_CONNECTED_CAM_VF_START 2");
                            rec_connected_cam.Func(REC_CONNECTED_CAM_VF_START, 0, 0);
                        }
                    }
#endif

                    /* To excute the functions that system block them when the Busy flag is enabled. */
                    AppUtil_BusyCheck(0);
                    if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
                        return ReturnValue;/**<  App switched out and break current application's flow. */
                    }
                }
            }
        }
    }
    AmbaPrintColor(GREEN,"[app_rec_connected_cam] REC_CONNECTED_CAM_MUXER_END mux.num%d",rec_connected_cam.MuxerNum);

    return ReturnValue;
}


static int rec_connected_cam_error_memory_runout(void)
{
    int ReturnValue = 0;
    if ((rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) || (rec_connected_cam.RecCapState == REC_CAP_STATE_PRE_RECORD)) {

        /* Stop/Pause recording if the audio and video data buffer is full. */
#if defined(REC_CONNECTED_CAM_MEM_RUNOUT_PAUSE)
        AmbaPrintColor(GREEN,"[app_rec_connected_cam] REC_CONNECTED_CAM_MEM_RUNOUT_PAUSE");
        rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_PAUSE, 0, 0);
#else
        APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MEM_RUNOUT);
        rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_STOP, 0, 0);
#endif
    }

    return ReturnValue;
}

static int rec_connected_cam_error_storage_runout(void)
{
    int ReturnValue = 0;

    if ((rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) || (rec_connected_cam.RecCapState == REC_CAP_STATE_PRE_RECORD)) {
        /**call card full handle to do loop enc*/
        rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_FULL_HANDLE, 0, 0);
    }
    return ReturnValue;
}


static int rec_connected_cam_error_storage_io(void)
{
    int ReturnValue = 0;

    if ((rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) || (rec_connected_cam.RecCapState == REC_CAP_STATE_PRE_RECORD)) {
        /* Stop recording if the audio and video data buffer is full. */
        rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_STOP, 0, 0);
    }

    return ReturnValue;
}

static int rec_connected_cam_error_loop_enc_err(int err_type)
{
    int ReturnValue = 0;
    /**start send storage runout msg after loop enc function*/
    AppLibMonitorStorage_EnableMsg(1);
    if (err_type) {
        AmbaPrint("[app_rec_connected_cam] Loop Enc Delete File Fail ");
    } else {
        if (rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD ) {
            AmbaPrint("[app_rec_connected_cam] Loop Enc Search First File Fail Stop Record ");
            rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_STOP, 0, 0);
        } else {
            AmbaPrint("[app_rec_connected_cam] Loop Enc Search First File Fail");
        }
    }
    return ReturnValue;
}

static int rec_connected_cam_loop_enc_done(void)
{
    int ReturnValue = 0;
    AmbaPrintColor(GREEN,"[app_rec_connected_cam] Loop Enc Done");
    /**start send storage runout msg after loop enc function*/
    AppLibMonitorStorage_EnableMsg(1);
    if (rec_connected_cam.RecCapState == REC_CAP_STATE_PREVIEW ) {
        rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_AUTO_START, 0, 0);
    }
    #if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    else if (rec_connected_cam.RecCapState == REC_CAP_STATE_VF) {
        rec_connected_cam.Func(REC_CONNECTED_CAM_VF_SWITCH_TO_RECORD, 0, 0);
    }
    #endif

    return ReturnValue;
}
static int rec_connected_cam_switch_app(void)
{
    int ReturnValue = 0;

    if ((rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) || (rec_connected_cam.RecCapState == REC_CAP_STATE_PRE_RECORD)) {
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_STOP, 0, 0);
    }
    #if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    else if (rec_connected_cam.RecCapState == REC_CAP_STATE_VF) {
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_VF_STOP, 0, 0);
    }
    #endif

    return ReturnValue;
}


/**
 *  @brief To update the video encoding resolution.
 *
 *  To update the video encoding resolution.
 *
 *  @param [in] videoRes Video resolution id
 *  @param [in] guiFlush The flag that update the gui.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_set_videoRes(UINT32 videoRes, UINT32 guiFlush)
{
    int ReturnValue = 0;

    AppLibVideoEnc_SetSensorVideoRes(videoRes);
    /** do not change to video only at HFR mode
    if (videoRes >= SENSOR_VIDEO_RES_FHD_HFR_P120_P100 && AppLibVideoEnc_GetRecMode() == REC_MODE_AV) {
        AppLibVideoEnc_SetRecMode(REC_MODE_VIDEO_ONLY);
        ReturnValue = AppLibVideoEnc_PipeChange();
    } else if (videoRes < SENSOR_VIDEO_RES_FHD_HFR_P120_P100 && AppLibVideoEnc_GetRecMode() == REC_MODE_VIDEO_ONLY) {
        AppLibVideoEnc_SetRecMode(REC_MODE_AV);
        ReturnValue = AppLibVideoEnc_PipeChange();
    }
    */
    if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        /**Calculate second stream timescale by main stream time scale*/
        APPLIB_SENSOR_VIDEO_ENC_CONFIG_s *VideoEncConfigData;
        VideoEncConfigData = AppLibSysSensor_GetVideoConfig(AppLibVideoEnc_GetSensorVideoRes());
        if ((VideoEncConfigData->EncNumerator % 25000) ==0) {
            AppLibVideoEnc_SetSecStreamTimeScale(25000);
            AppLibVideoEnc_SetSecStreamTick(1000);
        } else {
            AppLibVideoEnc_SetSecStreamTimeScale(30000);
            AppLibVideoEnc_SetSecStreamTick(1001);
        }
        AppLibVideoEnc_LiveViewSetup();
    }

    /** reflush the menu */
    if (AppWidget_GetCur() == WIDGET_MENU) {
        AppMenu_ReflushItem();
    }

    rec_connected_cam.Gui(GUI_VIDEO_SENSOR_RES_UPDATE, UserSetting->VideoPref.SensorVideoRes, 0);
    if (guiFlush) {
        rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
    }

    return ReturnValue;
}

/**
 *  @brief To update the video encoding quality.
 *
 *  To update the video encoding quality.
 *
 *  @param [in] videoRes Quality id
 *  @param [in] guiFlush The flag that update the gui.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_set_video_quality(UINT32 quality, UINT32 guiFlush)
{
    int ReturnValue = 0;

    AppLibVideoEnc_SetQuality(quality);
    switch (quality) {
    case VIDEO_QUALITY_SFINE:
        ReturnValue = GUI_SFINE;
        break;
    case VIDEO_QUALITY_FINE:
        ReturnValue = GUI_FINE;
        break;
    case VIDEO_QUALITY_NORMAL:
    default:
        ReturnValue = GUI_NORMAL;
        break;
    }
    rec_connected_cam.Gui(GUI_VIDEO_QUALITY_UPDATE, ReturnValue, 0);
    if (guiFlush) {
        rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
    }

    return ReturnValue;
}

/**
 *  @brief To update the photo caputre size.
 *
 *  To update the photo caputre size.
 *
 *  @param [in] size Photo size id
 *  @param [in] guiFlush The flag that update the gui.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_set_photo_size(UINT32 size, UINT32 guiFlush)
{
    int ReturnValue = 0;

    AppLibStillEnc_SetSizeID(size);

    if (app_status.CurrEncMode == APP_STILL_ENC_MODE) {
        AppLibStillEnc_LiveViewSetup();
    }

    rec_connected_cam.Gui(GUI_PHOTO_SIZE_UPDATE, size, 0);
    if (guiFlush) {
        rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
    }
    return ReturnValue;
}

/**
 *  @brief To update the photo quality.
 *
 *  To update the photo quality.
 *
 *  @param [in] videoRes Quality id
 *  @param [in] guiFlush The flag that update the gui.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_set_photo_quality(UINT32 quality, UINT32 guiFlush)
{
    int ReturnValue = 0;

    AppLibStillEnc_SetQualityMode(quality);

    /* Update gui. */
    switch (quality) {
    case PHOTO_QUALITY_SFINE:
        ReturnValue = GUI_SFINE;
        break;
    case PHOTO_QUALITY_FINE:
        ReturnValue = GUI_FINE;
        break;
    case PHOTO_QUALITY_NORMAL:
        default:
        ReturnValue = GUI_NORMAL;
        break;
    }
    rec_connected_cam.Gui(GUI_PHOTO_QUALITY_UPDATE, ReturnValue, 0);
    if (guiFlush) {
        rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
    }

    return ReturnValue;
}

/**
 *  @brief Update the gui of record mode.
 *
 *  Update the gui of record mode.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_rec_mode_gui_update(void)
{
    int ReturnValue = 0;

    if (UserSetting->VideoPref.TimeLapse == VIDEO_TIME_LAPSE_OFF) {
        if (UserSetting->VideoPref.DualStreams) {
            rec_connected_cam.Gui(GUI_REC_MODE_UPDATE, GUI_MODE_DUAL_STREAMS, 0);
        } else {
            rec_connected_cam.Gui(GUI_REC_MODE_UPDATE, GUI_MODE_DEFAULT, 0);
        }
    } else {
        switch (UserSetting->VideoPref.TimeLapse) {
        case VIDEO_TIME_LAPSE_2S:
            rec_connected_cam.Gui(GUI_REC_MODE_UPDATE, GUI_MODE_TIME_LAPSE_2S, 0);
            break;
        default:
        case VIDEO_TIME_LAPSE_OFF:
            rec_connected_cam.Gui(GUI_REC_MODE_UPDATE, GUI_MODE_DEFAULT, 0);
            break;
        }
    }

    return ReturnValue;
}


/**
 *  @brief To update the time lapse setting.
 *
 *  To update the time lapse setting.
 *
 *  @param [in] timeLapse Time Lapse setting
 *  @param [in] guiFlush The flag that update the gui.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_set_time_lapse(UINT32 timeLapse, UINT32 guiFlush)
{
    int ReturnValue = 0;

    ReturnValue = AppLibVideoEnc_GetSensorVideoRes();
    if ((ReturnValue > SENSOR_VIDEO_RES_PHOTO) && (ReturnValue < SENSOR_VIDEO_RES_NUM)) {
        /* HFR mode doesn't support timeLapse. */
        AmbaPrint("[app_rec_connected_cam] HFR mode doesn't support timeLapse.");
        timeLapse = VIDEO_TIME_LAPSE_OFF;
    }

    switch (timeLapse) {
    case VIDEO_TIME_LAPSE_2S:
        ReturnValue = AppLibVideoEnc_SetTimeLapse(timeLapse);
        break;
    case VIDEO_TIME_LAPSE_OFF:
        ReturnValue = AppLibVideoEnc_SetTimeLapse(timeLapse);
        break;
    default:
        break;
    }

    /* Update gui. */
    rec_connected_cam_rec_mode_gui_update();
    if (guiFlush) {
        rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
    }

    return ReturnValue;
}

/**
 *  @brief To update the self timer setting.
 *
 *  To update the self timer setting.
 *
 *  @param [in] selftimer Self timer setting
 *  @param [in] guiFlush The flag that update the gui.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_set_selftimer(UINT32 selftimer, UINT32 guiFlush)
{
    int ReturnValue = 0;

    /* Update gui. */
    switch (selftimer) {
    case SELF_TIMER_3S:
        ReturnValue = GUI_SELFTIMER_3S;
        break;
    case SELF_TIMER_5S:
        ReturnValue = GUI_SELFTIMER_5S;
        break;
    case SELF_TIMER_10S:
        ReturnValue = GUI_SELFTIMER_10S;
        break;
    case SELF_TIMER_OFF:
    default:
        ReturnValue = GUI_SELFTIMER_OFF;
        break;
    }
    rec_connected_cam.Gui(GUI_SELFTIMER_UPDATE, ReturnValue, 0);
    if (guiFlush) {
        rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
    }

    return ReturnValue;
}

static int rec_connected_cam_set_enc_mode(int param1)
{
    int ReturnValue = 0;

    if (param1 == app_status.CurrEncMode)
        return ReturnValue;/**<  Break the flow if the mode is the same as current mode. */

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    if (rec_connected_cam.RecCapState == REC_CAP_STATE_TRANSIT_TO_VF) {
        /**notify net fifo stop and change state to reset to suspend vf start*/
        NotifyNetFifoOfAppState(AMP_NETFIFO_NOTIFY_STOPENC);
        rec_connected_cam.RecCapState = REC_CAP_STATE_RESET;
    }
#endif

    /** Start the liveview. */
    if (param1 == APP_STILL_ENC_MODE) {
        /* Stop video preivew. */
        AppLibVideoEnc_LiveViewStop();
        /* Start still preivew. */
        AppLibStillEnc_LiveViewSetup();
        AppLibStillEnc_LiveViewStart();
        app_status.CurrEncMode = APP_STILL_ENC_MODE;
    } else {
        /* Stop still preivew. */
        AppLibStillEnc_LiveViewStop();
        AppLibStillEnc_LiveViewDeInit();
        /**delete still encode pipe when change to video mode*/
        AppLibStillEnc_DeletePipe();
        /* Start video preivew. */
        AppLibVideoEnc_LiveViewSetup();
        AppLibVideoEnc_LiveViewStart();
        app_status.CurrEncMode = APP_VIDEO_ENC_MODE;
    }

    /** Update the GUI. */
    if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        rec_connected_cam.Gui(GUI_APP_VIDEO_ICON_SHOW, 0, 0);
    } else {
        rec_connected_cam.Gui(GUI_APP_PHOTO_ICON_SHOW, 0, 0);
    }
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}
static int rec_connected_cam_set_dmf_mode(int param1)
{
    int ReturnValue = 0;
    AppLibStorageDmf_SetFileNumberMode(APPLIB_DCF_MEDIA_VIDEO, (APPLIB_DCF_NUMBER_MODE_e)param1, DCIM_HDLR);
    return ReturnValue;
}

static int rec_connected_cam_card_removed(void)
{
    int ReturnValue = 0;

    if (rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) {
        /* Stop recording when the card be removed during recording. */
        rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_STOP, 0, 0);
    }

    /* Stop the warning message, because the warning could need to be updated.. */
    rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_STOP, 0, 0);

    /* Stop the self timer because the system can not capture or record. */
    rec_connected_cam.Func(REC_CONNECTED_CAM_SELFTIMER_STOP, 0, 0);

    /* Reset the file type of quick view function. */
    rec_connected_cam.QuickViewFileType = MEDIA_TYPE_UNKNOWN;

    /* Update the gui of card's status. */
    rec_connected_cam.Gui(GUI_CARD_UPDATE, GUI_NO_CARD, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_card_error_removed(void)
{
    int ReturnValue = 0;

    if (rec_connected_cam.RecCapState == REC_CAP_STATE_VF) {
        // do nothing
    } else if (rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD){
        /* Update the flags */
        APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);
        APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_ERROR);

        /* Stop recording when the card be removed during recording. */
        rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_STOP, 0, 0);

        /* Stop the warning message, because the warning could need to be updated.. */
        rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_STOP, 0, 0);

        /* Stop the self timer because the system can not capture or record. */
        rec_connected_cam.Func(REC_CONNECTED_CAM_SELFTIMER_STOP, 0, 0);

        /* Reset the file type of quick view function. */
        rec_connected_cam.QuickViewFileType = MEDIA_TYPE_UNKNOWN;

    }

    /* Update the gui of card's status. */
    rec_connected_cam.Gui(GUI_CARD_UPDATE, GUI_NO_CARD, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_card_new_insert(void)
{
    int ReturnValue = 0;

    /* Stop the warning message, because the warning could need to be updated.. */
    rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_STOP, 0, 0);

    /* Stop the self timer because the system can not capture or record. */
    rec_connected_cam.Func(REC_CONNECTED_CAM_SELFTIMER_STOP, 0, 0);

    /* Reset the file type of quick view function. */
    rec_connected_cam.QuickViewFileType = MEDIA_TYPE_UNKNOWN;

    /* Update the gui of card's status. */
    rec_connected_cam.Gui(GUI_CARD_UPDATE, GUI_NO_CARD, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_card_storage_idle(void)
{
    int ReturnValue = 0;

    rec_connected_cam.Func(REC_CONNECTED_CAM_SET_FILE_INDEX, 0, 0);

    AppUtil_CheckCardParam(0);
    if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
        return ReturnValue;/**<  App switched out*/
    }

    /* Update the gui of card's status. */
    rec_connected_cam.Gui(GUI_CARD_UPDATE, GUI_CARD_READY, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    /** card ready call auto record to start record*/
    rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_AUTO_START, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_card_storage_busy(void)
{
    int ReturnValue = 0;

    /* Update the gui of card's status. */
    rec_connected_cam.Gui(GUI_CARD_UPDATE, GUI_CARD_REFRESHING, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_card_check_status(int param1)
{
    int ReturnValue = 0;

    ReturnValue = AppLibCard_CheckStatus(CARD_CHECK_WRITE);
    if (ReturnValue == CARD_STATUS_NO_CARD) {
        rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_NO_CARD, 0);
        AmbaPrintColor(RED,"[app_rec_connected_cam] Card error = %d  No Card", ReturnValue);
    } else if (ReturnValue == CARD_STATUS_WP_CARD) {
        rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_CARD_PROTECTED, 0);
        AmbaPrintColor(RED,"[app_rec_connected_cam] Card error = %d Write Protection Card", ReturnValue);
    } else if (ReturnValue == CARD_STATUS_NOT_ENOUGH_SPACE) {
        rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_CARD_FULL, 0);
        if (param1 == 0) {/**< video card caheck, do loop encode , photo do noting*/
            rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_FULL_HANDLE, 0, 0);
        }
        AmbaPrintColor(RED,"[app_rec_connected_cam] Card error = %d CARD_STATUS_NOT_ENOUGH_SPACE", ReturnValue);
    } else {
        if (ReturnValue != CARD_STATUS_CHECK_PASS ) {
            AmbaPrintColor(RED,"[app_rec_connected_cam] Card error = %d", ReturnValue);
        }
    }

    return ReturnValue;
}

static int rec_connected_cam_card_full_handle(void)
{
    int ReturnValue = 0;
    /**disable storage runout msg send*/
    AppLibMonitorStorage_EnableMsg(0);
    AmbaPrintColor(GREEN,"[app_rec_connected_cam] SEND MSG APPLIB_LOOP_ENC_START");
    /**send msg to start loop enc*/
    ReturnValue = AppLibStorageAsyncOp_SndMsg(HMSG_LOOP_ENC_START, DCIM_HDLR, 0);
    return ReturnValue;

}

static int rec_connected_cam_card_full_handle_event(void)
{
    int ReturnValue = 0;
    /**disable storage runout msg send*/
    AppLibMonitorStorage_EnableMsg(0);
    AmbaPrintColor(GREEN,"[app_rec_connected_cam] SEND MSG APPLIB_LOOP_ENC_START for Event Record");
    /**send msg to start loop enc*/
    ReturnValue = AppLibStorageAsyncOp_SndMsg(HMSG_LOOP_ENC_START, EVENTRECORD_HDLR, 0);
    return ReturnValue;

}

static int rec_connected_cam_file_id_update(UINT32 FileID)
{
    int ReturnValue = 0;
    /**update last id for serial mode if new filw close*/
    if (FileID > UserSetting->SetupPref.DmfMixLastIdx || UserSetting->SetupPref.DMFMode == DMF_MODE_RESET) {
        UserSetting->SetupPref.DmfMixLastIdx = FileID;
    }
    return ReturnValue;
}


/**
 *  @brief handle widget close msg
 *
 *  remove pop up flag and call auto record function to start record
 *
 *  @return =0 success
 *
 */
static int rec_connected_cam_widget_closed(void)
{
    int ReturnValue = 0;

    if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_POPUP)) {
        APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_POPUP);
        /** after menu close call auto record function to start record*/
        #if defined (CONFIG_APP_CONNECTED_AMBA_LINK)
        if (UserSetting->VideoPref.StreamType == STREAM_TYPE_RTSP) {
            if ((app_status.CurrEncMode == APP_VIDEO_ENC_MODE) &&
                ((!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY)))) {
                DBGMSGc2(GREEN, "[rec_connected_cam] <widget_closed> REC_CONNECTED_CAM_VF_START");
                rec_connected_cam.Func(REC_CONNECTED_CAM_VF_START, 0, 0);
            }
        }
        #else
        AmbaPrintColor(YELLOW,"[rec_connected_cam] <widget_closed> record auto start");
        rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_AUTO_START, 0, 0);
        #endif
    }
    return ReturnValue;
}

/**
 *  @brief Switch NTSC and PAL mode
 *
 *  Switch NTSC and PAL mode
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_set_system_type(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief Update the Vout setting of FCHAN
 *
 *  Update the Vout setting of FCHAN
 *
 *  @param [in] msg Message id
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_update_fchan_vout(UINT32 msg)
{
    int ReturnValue = 0;
	//AmbaPrintColor(YELLOW,"[rec_connected_cam] rec_connected_cam_update_fchan_vout");
    switch (msg) {
    case HMSG_HDMI_INSERT_SET:
    case HMSG_HDMI_INSERT_CLR:
        AppLibSysVout_SetJackHDMI(app_status.HdmiPluginFlag);
        break;
    case HMSG_CS_INSERT_SET:
    case HMSG_CS_INSERT_CLR:
        AppLibSysVout_SetJackCs(app_status.CompositePluginFlag);
        break;
    default:
        return 0;
        break;
    }
    ReturnValue = AppLibDisp_SelectDevice(DISP_CH_FCHAN, DISP_ANY_DEV);
    if (APP_CHECKFLAGS(ReturnValue, DISP_FCHAN_NO_CHANGE)) {
        AmbaPrint("[app_rec_connected_cam] Display FCHAN has no changed");
    } else {
        if (APP_CHECKFLAGS(ReturnValue, DISP_FCHAN_NO_DEVICE)) {
            AppLibGraph_DisableDraw(GRAPH_CH_FCHAN);
            AppLibDisp_ChanStop(DISP_CH_FCHAN);
            AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
            AppLibDisp_FlushWindow(DISP_CH_FCHAN);
            app_status.LockDecMode = 0;
        } else {
            AppLibGraph_EnableDraw(GRAPH_CH_FCHAN);
            if (app_status.FchanDecModeOnly == 1) {
                app_status.LockDecMode = 1;
                AppUtil_SwitchApp(APP_THUMB_MOTION);
                return ReturnValue;
            } else {
                app_status.LockDecMode = 0;
#ifdef CONFIG_ASD_HDMI_PREVIEW				
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
				if (rec_connected_cam.RecCapState == REC_CAP_STATE_VF) {
					/** stop view finder at menu open*/
					rec_connected_cam.Func(REC_CONNECTED_CAM_VF_STOP, 0, 0);
				} else if (rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD)
#endif
				{
					/** record stop at menu open*/
					rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_STOP, 0, 0);
				}
				
					AppLibDisp_ConfigMode(DISP_CH_FCHAN, AppLibSysVout_GetVoutMode(VOUT_DISP_MODE_2160P_HALF));
					AppLibDisp_SetupChan(DISP_CH_FCHAN);
					AppLibDisp_ChanStart(DISP_CH_FCHAN);
					{
						AMP_DISP_WINDOW_CFG_s Window;
						AMP_DISP_INFO_s DispDev = {0};
				
						memset(&Window, 0, sizeof(AMP_DISP_WINDOW_CFG_s));
				
						ReturnValue = AppLibDisp_GetDeviceInfo(DISP_CH_FCHAN, &DispDev);
						if ((ReturnValue < 0) || (DispDev.DeviceInfo.Enable == 0)) {
							DBGMSG("[rec_cam] FChan Disable. Disable the fchan window");
							AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
							AppLibGraph_DeactivateWindow(GRAPH_CH_FCHAN);
							AppLibDisp_FlushWindow(DISP_CH_FCHAN);
							app_status.LockDecMode = 0;
							AppLibGraph_DisableDraw(GRAPH_CH_FCHAN);					
						} else {
							/** FCHAN window*/
							AppLibDisp_GetWindowConfig(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0), &Window);
							Window.Source = AMP_DISP_ENC;
							Window.SourceDesc.Dec.DecHdlr = 0;
							Window.CropArea.Width = 0;
							Window.CropArea.Height = 0;
							Window.CropArea.X = 0;
							Window.CropArea.Y = 0;
							Window.TargetAreaOnPlane.Width = DispDev.DeviceInfo.VoutWidth;
							Window.TargetAreaOnPlane.Height = DispDev.DeviceInfo.VoutHeight;
							Window.TargetAreaOnPlane.X = 0;
							Window.TargetAreaOnPlane.Y = 0;
							AppLibDisp_SetWindowConfig(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0), &Window);
							AppLibDisp_ActivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
							AppLibGraph_SetWindowConfig(GRAPH_CH_FCHAN);
							AppLibGraph_ActivateWindow(GRAPH_CH_FCHAN);
							AppLibDisp_FlushWindow(DISP_CH_FCHAN);
				
							rec_connected_cam.Gui(GUI_SET_LAYOUT, 0, 0);
							AppLibGraph_EnableDraw(GRAPH_CH_DCHAN); 				
							rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
				
						}
						AppLibVideoEnc_LiveViewSetup(); 
					}
#endif //CONFIG_ASD_HDMI_PREVIEW				
            }
        }
    }

    return ReturnValue;
}

static int rec_connected_cam_change_display(void)
{
    int ReturnValue = 0;

    /** Setup the display. */
    AppLibDisp_SelectDevice(DISP_CH_FCHAN | DISP_CH_DCHAN, DISP_ANY_DEV);
    AppLibDisp_ConfigMode(DISP_CH_FCHAN | DISP_CH_DCHAN, AppLibSysVout_GetVoutMode(VOUT_DISP_MODE_1080P));
    AppLibDisp_SetupChan(DISP_CH_FCHAN | DISP_CH_DCHAN);
    AppLibDisp_ChanStart(DISP_CH_FCHAN | DISP_CH_DCHAN);
    /** Setup the Window. */
    {
        AMP_DISP_WINDOW_CFG_s Window;
        APPLIB_VOUT_PREVIEW_PARAM_s PreviewParam = {0};
        AMP_DISP_INFO_s DispDev = {0};

        memset(&Window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));

        if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
            APPLIB_SENSOR_VIDEO_ENC_CONFIG_s *VideoEncConfigData = NULL;
            VideoEncConfigData = AppLibSysSensor_GetVideoConfig(AppLibVideoEnc_GetSensorVideoRes());
            PreviewParam.AspectRatio = VideoEncConfigData->VAR;
        } else {
            APPLIB_SENSOR_STILLPREV_CONFIG_s *StillLiveViewConfigData = NULL;
            StillLiveViewConfigData = (APPLIB_SENSOR_STILLPREV_CONFIG_s *)AppLibSysSensor_GetPhotoLiveviewConfig(AppLibStillEnc_GetPhotoPjpegCapMode(), AppLibStillEnc_GetPhotoPjpegConfigId());
            PreviewParam.AspectRatio = StillLiveViewConfigData->VAR;
        }

        ReturnValue = AppLibDisp_GetDeviceInfo(DISP_CH_FCHAN, &DispDev);
        if ((ReturnValue < 0) || (DispDev.DeviceInfo.Enable == 0)) {
            DBGMSG("[app_rec_connected_cam] FChan Disable. Disable the fchan Window");
            AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
            AppLibGraph_DisableDraw(GRAPH_CH_FCHAN);
        } else {
            /** FCHAN Window*/
            PreviewParam.ChanID = DISP_CH_FCHAN;
            AppLibDisp_CalcPreviewWindowSize(&PreviewParam);
            AppLibDisp_GetWindowConfig(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0), &Window);
            Window.Source = AMP_DISP_ENC;
            Window.CropArea.Width = 0;
            Window.CropArea.Height = 0;
            Window.CropArea.X = 0;
            Window.CropArea.Y = 0;
            Window.TargetAreaOnPlane.Width = PreviewParam.Preview.Width;
            Window.TargetAreaOnPlane.Height = PreviewParam.Preview.Height;
            Window.TargetAreaOnPlane.X = PreviewParam.Preview.X;
            Window.TargetAreaOnPlane.Y = PreviewParam.Preview.Y;
            AppLibDisp_SetWindowConfig(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0), &Window);
            AppLibDisp_ActivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
            AppLibGraph_EnableDraw(GRAPH_CH_FCHAN);
        }

        ReturnValue = AppLibDisp_GetDeviceInfo(DISP_CH_DCHAN, &DispDev);
        if ((ReturnValue < 0) || (DispDev.DeviceInfo.Enable == 0)) {
            DBGMSG("[app_rec_connected_cam] DChan Disable. Disable the Dchan Window");
            AppLibDisp_DeactivateWindow(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0));
            AppLibGraph_DisableDraw(GRAPH_CH_DCHAN);
        } else {
            /** DCHAN Window*/
            PreviewParam.ChanID = DISP_CH_DCHAN;
            AppLibDisp_CalcPreviewWindowSize(&PreviewParam);
            AppLibDisp_GetWindowConfig(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0), &Window);
            Window.Source = AMP_DISP_ENC;
            Window.CropArea.Width = 0;
            Window.CropArea.Height = 0;
            Window.CropArea.X = 0;
            Window.CropArea.Y = 0;
            Window.TargetAreaOnPlane.Width = PreviewParam.Preview.Width;
            Window.TargetAreaOnPlane.Height = PreviewParam.Preview.Height;
            Window.TargetAreaOnPlane.X = PreviewParam.Preview.X;
            Window.TargetAreaOnPlane.Y = PreviewParam.Preview.Y;
            AppLibDisp_SetWindowConfig(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0), &Window);
            AppLibDisp_ActivateWindow(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0));
            AppLibGraph_EnableDraw(GRAPH_CH_DCHAN);
        }
        AppLibDisp_FlushWindow(DISP_CH_FCHAN | DISP_CH_DCHAN);
    }

    return ReturnValue;
}

static int rec_connected_cam_change_osd(void)
{
    int ReturnValue = 0;

    /* Update graphic window*/
    AppLibGraph_SetWindowConfig(GRAPH_CH_FCHAN | GRAPH_CH_DCHAN);
    AppLibGraph_ActivateWindow(GRAPH_CH_FCHAN | GRAPH_CH_DCHAN);
    AppLibGraph_FlushWindow(GRAPH_CH_FCHAN | GRAPH_CH_DCHAN);
    rec_connected_cam.Gui(GUI_SET_LAYOUT, 0, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_usb_connect(void)
{
    int ReturnValue = 0;

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    if (rec_connected_cam.RecCapState == REC_CAP_STATE_VF) {
        rec_connected_cam.Func(REC_CONNECTED_CAM_VF_STOP, 0, 0);
    }
#endif


    if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY) ||
        APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY)) {
        return ReturnValue;
    }

    /* The flow after connecting the USB cable. */
    switch (UserSetting->SetupPref.USBMode) {
    case USB_MODE_AMAGE:
        AppAppMgt_SwitchApp(APP_USB_AMAGE);
        break;
	case USB_MODE_RS232:
		break;
    case USB_MODE_MSC:
    default:
        AppAppMgt_SwitchApp(APP_USB_MSC);
        break;
    }

    return ReturnValue;
}

/**
 *  @brief To show the gui of current application
 *
 *  To show the gui of current application
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_start_show_gui(void)
{
    int ReturnValue = 0;
    int GuiParam = 0;

    // check encode status
    if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
        rec_connected_cam.Gui(GUI_APP_VIDEO_ICON_SHOW, 0, 0);
    } else {
        rec_connected_cam.Gui(GUI_APP_PHOTO_ICON_SHOW, 0, 0);
    }
    // show power status
    rec_connected_cam.Gui(GUI_POWER_STATE_UPDATE, app_status.PowerType, app_status.BatteryState);
    rec_connected_cam.Gui(GUI_POWER_STATE_SHOW, app_status.PowerType, app_status.BatteryState);

    // show SD card status
    ReturnValue = AppLibCard_CheckStatus(0);
    if (ReturnValue == CARD_STATUS_NO_CARD) {
        GuiParam = GUI_NO_CARD;
    } else {
        GuiParam = GUI_CARD_READY;
    }
    rec_connected_cam.Gui(GUI_CARD_UPDATE, GuiParam, 0);
    rec_connected_cam.Gui(GUI_CARD_SHOW, 0, 0);

    // show selftimer
    rec_connected_cam.Gui(GUI_SELFTIMER_UPDATE, UserSetting->VideoPref.VideoSelftimer, 0);
    rec_connected_cam.Gui(GUI_SELFTIMER_SHOW, 0, 0);

    // show video resolution
    rec_connected_cam.Gui(GUI_VIDEO_SENSOR_RES_UPDATE, UserSetting->VideoPref.SensorVideoRes, 0);
    rec_connected_cam.Gui(GUI_VIDEO_SENSOR_RES_SHOW, 0, 0);

    // show quality
    rec_connected_cam.Gui(GUI_VIDEO_QUALITY_UPDATE, UserSetting->VideoPref.VideoQuality, 0);
    rec_connected_cam.Gui(GUI_VIDEO_QUALITY_SHOW, 0, 0);

    // show mode
    rec_connected_cam.Gui(GUI_REC_MODE_UPDATE, 0, 0);
    rec_connected_cam.Gui(GUI_REC_MODE_SHOW, 0, 0);

    // show photo resolution
    rec_connected_cam.Gui(GUI_PHOTO_SIZE_UPDATE, UserSetting->PhotoPref.PhotoSize, 0);
    rec_connected_cam.Gui(GUI_PHOTO_SIZE_SHOW, 0, 0);
    // show quality
    rec_connected_cam.Gui(GUI_PHOTO_QUALITY_UPDATE, UserSetting->PhotoPref.PhotoQuality, 0);
    rec_connected_cam.Gui(GUI_PHOTO_QUALITY_SHOW, 0, 0);
    rec_connected_cam.Gui(GUI_CAP_MODE_UPDATE, UserSetting->PhotoPref.PhotoCapMode, 0);
    rec_connected_cam.Gui(GUI_CAP_MODE_SHOW, 0, 0);

    // draw
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);
    return ReturnValue;
}

static int rec_connected_cam_update_bat_power_status(int param1)
{
    int ReturnValue = 0;

    /* Update the gui of power's status. */
    if (param1 == 0) {
        /*Hide the battery gui.*/
        rec_connected_cam.Gui(GUI_POWER_STATE_HIDE, GUI_HIDE_POWER_EXCEPT_DC, 0);
    } else if (param1 == 1) {
        /*Update the battery gui.*/
        rec_connected_cam.Gui(GUI_POWER_STATE_UPDATE, app_status.PowerType, app_status.BatteryState);
        rec_connected_cam.Gui(GUI_POWER_STATE_SHOW, app_status.PowerType, app_status.BatteryState);
    } else if (param1 == 2) {
        /*Reset the battery and power gui.*/
        rec_connected_cam.Gui(GUI_POWER_STATE_HIDE, 0, 0);
        rec_connected_cam.Gui(GUI_POWER_STATE_UPDATE, app_status.PowerType, app_status.BatteryState);
        rec_connected_cam.Gui(GUI_POWER_STATE_SHOW, app_status.PowerType, app_status.BatteryState);
    }
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

/**
 *  @brief The timer handler of warning message.
 *
 *  To show and hide the warning message.
 *
 *  @param [in] eid Event id
 *
 *  @return >=0 success, <0 failure
 */
static void rec_connected_cam_warning_timer_handler(int eid)
{
    static int blink_count = 0;

    if (eid == TIMER_UNREGISTER) {
        blink_count = 0;
        return;
    }

    blink_count++;

    if (blink_count & 0x01) {
        if (blink_count >= 5) {
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_WARNING_MSG_RUN);
            AppLibComSvcTimer_Unregister(TIMER_2HZ, rec_connected_cam_warning_timer_handler);
        }
        rec_connected_cam.Gui(GUI_WARNING_HIDE, 0, 0);
    } else {
        rec_connected_cam.Gui(GUI_WARNING_SHOW, 0, 0);
    }

    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

}


static int rec_connected_cam_warning_msg_show(int enable, int param1, int param2)
{
    int ReturnValue = 0;

    if (enable) {
        /* To show the warning message. */
        if (param2) {
            rec_connected_cam.Gui(GUI_WARNING_UPDATE, param1, 0);
            rec_connected_cam.Gui(GUI_WARNING_SHOW, 0, 0);
        } else if (!APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_WARNING_MSG_RUN)) {
            APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_WARNING_MSG_RUN);
            rec_connected_cam.Gui(GUI_WARNING_UPDATE, param1, 0);
            rec_connected_cam.Gui(GUI_WARNING_SHOW, 0, 0);

            AppLibComSvcTimer_Register(TIMER_2HZ, rec_connected_cam_warning_timer_handler);
        }
    } else {
        /* To disable the warning message. */
        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_WARNING_MSG_RUN)) {
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_WARNING_MSG_RUN);
            AppLibComSvcTimer_Unregister(TIMER_2HZ, rec_connected_cam_warning_timer_handler);
        }
        rec_connected_cam.Gui(GUI_WARNING_HIDE, 0, 0);
    }
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_card_fmt_nonoptimum(void)
{
    int ReturnValue = 0;

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    if (rec_connected_cam.RecCapState == REC_CAP_STATE_VF) {
        rec_connected_cam.Func(REC_CONNECTED_CAM_VF_STOP, 0, 0);
    }
    APP_ADDFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_POPUP);
#endif

    return ReturnValue;
}

static int rec_connected_cam_adas_event_handler(UINT32 msg)
{
    int ReturnValue = 0;

    switch (msg) {
        case HMSG_VA_FCAR_DEPARTURE:
            rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_FCMD_EVENT, 0);
            break;
        case HMSG_VA_LOW_LIGHT:
            rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_LLWS_EVENT, 0);
            break;
        case HMSG_VA_MD_Y:
        case HMSG_VA_MD_AE:
            rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_MD_EVENT, 0);
            break;
        case HMSG_VA_LDW:
            rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_LDWS_EVENT, 0);
            break;
        case HMSG_VA_FCW:
            rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_FCWS_EVENT, 0);
            break;
        case HMSG_VA_CLIBRATION_DONE:
            rec_connected_cam.Func(REC_CONNECTED_CAM_ADAS_UPDATE_PARAM, 0, 0);
            break;
        default:
            break;
    }

    rec_connected_cam.Gui(GUI_ADAS_STAMP_UPDATE, 0, 0);

    return ReturnValue;
}

static int rec_connected_cam_adas_function_init(void)
{
    int ReturnValue = 0;
    APPLIB_FCMD_CFG_t FCMD_Config = {0};
    APPLIB_FCMD_PAR_t FCMD_Params = {0};
    APPLIB_LLWS_CFG_t LLWS_Config = {0};
    APPLIB_LLWS_PAR_t LLWS_Params = {0};
    APPLIB_LDWS_CFG_t LDWS_Config = {0};
    APPLIB_FCWS_CFG_t FCWS_Config = {0};
    APPLIB_ADAS_PAR_t ADAS_Params = {0};

    /** FCMD init */
    ReturnValue = AppLibVideoAnal_FCMD_GetDef_Setting(&FCMD_Config, &FCMD_Params);
    FCMD_Config.FCMDSensitivity = ADAS_SL_HIGH;
    ReturnValue = AppLibVideoAnal_FCMD_Init(APPLIB_FRM_HDLR_2ND_YUV, FCMD_Config, FCMD_Params);
    DBGMSGc2(YELLOW, "--------AppLibVideoAnal_FCMD_Init return = %d", ReturnValue);

    /** LLWS init */
    AppLibVideoAnal_LLWS_GetDef_Setting(&LLWS_Config, &LLWS_Params);
    LLWS_Config.LLWSSensitivity = ADAS_SL_HIGH;
    ReturnValue = AppLibVideoAnal_LLWS_Init(LLWS_Config, LLWS_Params);
    DBGMSGc2(YELLOW, "--------AppLibVideoAnal_LLWS_Init return = %d", ReturnValue);

    /** LDWS, FCWS init */
    AppLibVideoAnal_ADAS_GetDef_Setting(&LDWS_Config, &FCWS_Config, &ADAS_Params);
    LDWS_Config.LDWSSensitivity = ADAS_SL_HIGH;
    FCWS_Config.FCWSSensitivity = ADAS_SL_HIGH;
    ReturnValue = AppLibVideoAnal_ADAS_Init(APPLIB_FRM_HDLR_2ND_YUV, LDWS_Config, FCWS_Config, ADAS_Params);
    DBGMSGc2(YELLOW, "--------AppLibVideoAnal_ADAS_Init return = %d", ReturnValue);

    return ReturnValue;
}

static int rec_connected_cam_adas_update_param(void)
{
    int ReturnValue = 0;
    APPLIB_FCMD_PAR_t FCMD_Params = {0};
    APPLIB_LLWS_PAR_t LLWS_Params = {0};
    APPLIB_ADAS_PAR_t ADAS_Params = {0};
    AMBA_ADAS_SCENE_STATUS_s* pSsceneStatus = NULL;
    int hood_line_position;
    int sky_line_position;
    Amba_Adas_GetSceneStatus(&pSsceneStatus);

    hood_line_position = pSsceneStatus->HoodLevel;
    sky_line_position = pSsceneStatus->HorizonLevel;

    FCMD_Params.HoodLevel = hood_line_position;
    FCMD_Params.HorizonLevel = sky_line_position;
    AppLibVideoAnal_FCMD_SetPar(&FCMD_Params);

    LLWS_Params.HoodLevel = hood_line_position;
    LLWS_Params.HorizonLevel = sky_line_position;
    AppLibVideoAnal_LLWS_SetPar(&LLWS_Params);

    ADAS_Params.HoodLevel = hood_line_position;
    ADAS_Params.HorizonLevel = sky_line_position;
    AppLibVideoAnal_ADAS_SetPar(&ADAS_Params);

    return ReturnValue;
}

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
static int rec_connected_cam_boos_booted(void)
{
    if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_POPUP)) {
        /* Don't start VF when menu is on. */
        DBGMSGc2(GREEN, "[rec_connected_cam] <boos_booted> APP_AFLAGS_POPUP");
        return 0;
    }

    if (UserSetting->VideoPref.StreamType != STREAM_TYPE_RTSP){
        return 0;
    }

    if (rec_connected_cam.RecCapState == REC_CAP_STATE_PREVIEW) {
        if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
            /* start view finder */
            if (UserSetting->VideoPref.StreamType == STREAM_TYPE_RTSP) {
                rec_connected_cam.Func(REC_CONNECTED_CAM_VF_START, 0, 0);
            }
        }
    } else if (rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) {
        NotifyNetFifoOfAppState(AMP_NETFIFO_NOTIFY_STARTENC);
    } else {
        ;
    }

    return 0;
}

static int rec_connected_cam_vf_start(void)
{
    int ReturnValue = 0;

    if (AppLibNetBase_GetBootStatus() == 0) {
        DBGMSGc2(GREEN,"[rec_connected_cam] <vf_start> AMBA Link not booted yet! Do nothing!");
        return 0;
    }

    if (rec_connected_cam.RecCapState != REC_CAP_STATE_PREVIEW) {
        DBGMSGc2(GREEN,"[rec_connected_cam] <vf_start> RecCapState is not REC_CAP_STATE_PREVIEW");
        return -1;
    }

    if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_POPUP)) {
        /* Don't start VF when menu is on. */
        DBGMSGc2(GREEN, "[rec_connected_cam] <vf_start> APP_AFLAGS_POPUP");
        return -1;
    }

    /* This check condition is for the case that view finder is stopped through netCtrol command AMSG_NETCTRL_VF_STOP.
          REC_CONNECTED_CAM_FLAGS_VF_DISABLE flag will be set when receiving AMSG_NETCTRL_VF_STOP. */
    if (APP_CHECKFLAGS(rec_connected_cam.NetCtrlFlags, APP_NETCTRL_FLAGS_VF_DISABLE)) {
        DBGMSGc2(GREEN,"[rec_connected_cam] <vf_start> VF function is disabled through netCtrl");
        return -1;
    }

    /* Setup the encode setting. */
    AppLibVideoEnc_EncodeSetup();

    ReturnValue = NotifyNetFifoOfAppState(AMP_NETFIFO_NOTIFY_STARTENC);
    if (ReturnValue == 0) {
        rec_connected_cam.RecCapState = REC_CAP_STATE_TRANSIT_TO_VF;

        /* AppLibVideoEnc_EncodeStart() should be postponed until
            message 'AMSG_NETFIFO_EVENT_START' is received. */
    }

    return 0;
}

static int rec_connected_cam_vf_stop(void)
{
    if (AppLibNetBase_GetBootStatus() == 0) {
        DBGMSGc2(GREEN,"[rec_connected_cam] <vf_stop> AMBA Link not booted yet! Do nothing!");
        return 0;
    }

    if (rec_connected_cam.RecCapState != REC_CAP_STATE_VF) {
        DBGMSGc2(RED,"[rec_connected_cam] <vf_stop> RecCapState is not REC_CAP_STATE_VF");
        return -1;
    }
    DBGMSGc2(GREEN,"[rec_connected_cam] <vf_stop> EncodeStop");

    #if defined(CONFIG_APP_CONNECTED_STAMP)
    rec_connected_cam_stop_stamp();
    #endif

    /* Start video encoding. */
    AppLibVideoEnc_EncodeStop();
    rec_connected_cam.RecCapState = REC_CAP_STATE_RESET;

    /* Stop the timer when stopping view finder. */
    AppLibComSvcTimer_Unregister(TIMER_1HZ, rec_connected_cam_rec_timer_handler);

    /* Update the gui. */
    rec_connected_cam.Gui(GUI_REC_STATE_HIDE, 0, 0);
    rec_connected_cam.Gui(GUI_REC_TIMER_HIDE, 0, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return 0;
}

static int rec_connected_cam_vf_switch_to_record(void)
{
    int ReturnValue = 0;

    if (AppLibNetBase_GetBootStatus() == 0) {
        DBGMSGc2(GREEN,"[rec_connected_cam] <vf_switch_to_record> AMBA Link not booted yet! Do nothing!");
        return -1;
    }

    if (rec_connected_cam.RecCapState != REC_CAP_STATE_VF) {
        DBGMSGc2(RED,"[rec_connected_cam] <vf_switch_to_record> RecCapState is not REC_CAP_STATE_VF");
        return -1;
    }

    /* Check the card's status. */
    ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_CHECK_STATUS, 0, 0);
    if (ReturnValue != 0) {
        return -1;
    }

    /* Set time lapse. */
    rec_connected_cam.TimeLapseTime = AppLibVideoEnc_GetTimeLapse();

    /* Register the timer to show the record time. */
    rec_connected_cam.RecTime = 0;

    /* Initialize and start the muxer. */
    AppLibFormat_MuxerInit();
    AppLibFormatMuxMp4_StartOnRecording();
    AppLibFormatMuxMp4_SetAutoSplitFileType(1);/**set the auto split file type*/

    /* Enable the storage monitor.*/
    AppLibMonitorStorage_Enable(1);
    AppLibMonitorStorage_EnableMsg(1);

    if (UserSetting->VAPref.AdasDetection == ADAS_ON) {
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_ADAS_UPDATE_PARAM, 0, 0);
	    AppLibVideoAnal_FCMD_Enable();
        AppLibVideoAnal_LLWS_Enable();
        AppLibVideoAnal_ADAS_Enable();
        rec_connected_cam.Gui(GUI_ADAS_STAMP_UPDATE, 0, 0);
        rec_connected_cam.Gui(GUI_ADAS_STAMP_SHOW, 0, 0);
    }

    rec_connected_cam.RecCapState = REC_CAP_STATE_RECORD;

    APP_ADDFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);

    /* Update the gui. */
    rec_connected_cam.Gui(GUI_REC_STATE_UPDATE, GUI_REC_START, 0);
    rec_connected_cam.Gui(GUI_REC_STATE_SHOW, 0, 0);
    rec_connected_cam.Gui(GUI_REC_TIMER_UPDATE, rec_connected_cam.RecTime, 0);
    rec_connected_cam.Gui(GUI_REC_TIMER_SHOW, 0, 0);
    rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

    return 0;
}

static int rec_connected_cam_capture_on_vf(void)
{
    if (rec_connected_cam.RecCapState != REC_CAP_STATE_VF) {
        return -1;
    }

    APP_ADDFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF);
    rec_connected_cam.Func(REC_CONNECTED_CAM_VF_STOP, 0, 0);

    return 0;
}

static int rec_connected_cam_netfifo_event_start(void)
{
    int ReturnValue = 0;

    AmbaPrintColor(GREEN,"[rec_connected_cam] <netfifo_event_start> AMSG_NETFIFO_EVENT_START");

    if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_POPUP)) {
        /* Don't start VF when menu is on. */
        NotifyNetFifoOfAppState(AMP_NETFIFO_NOTIFY_STOPENC);
        rec_connected_cam.RecCapState = REC_CAP_STATE_PREVIEW;
        DBGMSGc2(GREEN, "[rec_connected_cam] <vf_start> APP_AFLAGS_POPUP");
        return -1;
    }

    /* Virtual fifo of net stream has been created when AMSG_NETFIFO_EVENT_START is received. */
    if (rec_connected_cam.RecCapState == REC_CAP_STATE_TRANSIT_TO_VF) {
        AmbaPrintColor(GREEN,"[rec_connected_cam] <netfifo_event_start> EncodeStart");

        /* Set time lapse. */
        rec_connected_cam.TimeLapseTime = AppLibVideoEnc_GetTimeLapse();

        /* Register the timer to show the view finder time. */
        rec_connected_cam.RecTime = 0;

        AppLibComSvcTimer_Register(TIMER_1HZ, rec_connected_cam_rec_timer_handler);

        /* Start video encoding. */
        AppLibVideoEnc_EncodeStart();

        APP_ADDFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);
        rec_connected_cam.RecCapState = REC_CAP_STATE_VF;

        if (APP_CHECKFLAGS(rec_connected_cam.NetCtrlFlags, APP_NETCTRL_FLAGS_VF_RESET_DONE)) {
            APP_REMOVEFLAGS(rec_connected_cam.NetCtrlFlags, APP_NETCTRL_FLAGS_VF_RESET_DONE);
            AppLibNetControl_ReplyErrorCode(AMBA_RESET_VF, 0);
        }

        /* Update the gui. */
        rec_connected_cam.Gui(GUI_REC_STATE_UPDATE, GUI_REC_PRE_RECORD, 0);
        rec_connected_cam.Gui(GUI_REC_STATE_SHOW, 0, 0);
        rec_connected_cam.Gui(GUI_REC_TIMER_UPDATE, rec_connected_cam.RecTime, 0);
        rec_connected_cam.Gui(GUI_REC_TIMER_SHOW, 0, 0);
        rec_connected_cam.Gui(GUI_FLUSH, 0, 0);

        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MEM_RUNOUT)) {
        AmbaPrintColor(YELLOW, "[rec_connected_cam] <record> VF -> REC auto due to memory runout");
        rec_connected_cam.Func(REC_CONNECTED_CAM_VF_SWITCH_TO_RECORD, 0, 0);
        APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MEM_RUNOUT);
        }

        /* Enable stamp */
        #if defined(CONFIG_APP_CONNECTED_STAMP)
        rec_connected_cam_setup_stamp();
        #endif

    }

    return ReturnValue;
}

static int rec_connected_cam_netfifo_event_stop(void)
{
    int ReturnValue = 0;

    AmbaPrintColor(GREEN,"[rec_connected_cam] <netfifo_event_stop> AMSG_NETFIFO_EVENT_STOP");

    APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_NETFIFO_BUSY);
    if (!rec_connected_cam_system_is_busy()) {
        APP_REMOVEFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_BUSY);
        if (APP_CHECKFLAGS(rec_connected_cam.NetCtrlFlags, APP_NETCTRL_FLAGS_VF_STOP_DONE)) {
            APP_REMOVEFLAGS(rec_connected_cam.NetCtrlFlags, APP_NETCTRL_FLAGS_VF_STOP_DONE);
            AppLibNetControl_ReplyErrorCode(AMBA_STOP_VF, 0);
        }

        /* To excute the functions that system block them when the Busy flag is enabled. */
        AppUtil_BusyCheck(0);
        /**Resend usb connect if usb connect is blocked by recording*/
        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_USB)) {
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_USB);
            AppLibComSvcHcmgr_SendMsg(HMSG_USB_DETECT_CONNECT, 0, 0);
        }
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            return ReturnValue;/**<  App switched out*/
        }

        if (app_status.CurrEncMode == APP_VIDEO_ENC_MODE) {
            if (APP_CHECKFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF)) {
                DBGMSGc2(CYAN, "[rec_connected_cam] <netfifo_event_stop> switch liveivew");
                AppLibVideoEnc_LiveViewStop();
                AppLibStillEnc_LiveViewSetup();
                AppLibStillEnc_LiveViewStart();
                app_status.CurrEncMode = APP_STILL_ENC_MODE;
            } else {
                if (UserSetting->VideoPref.StreamType == STREAM_TYPE_RTSP) {
                    DBGMSGc2(GREEN, "[rec_connected_cam] <netfifo_event_stop> REC_CONNECTED_CAM_VF_START");
                    rec_connected_cam.Func(REC_CONNECTED_CAM_VF_START, 0, 0);
                }
            }
        }
    }

    return ReturnValue;
}

static int rec_connected_cam_netctrl_capture_done(void)
{
    UINT64 FileObjID = 0;
    char CurFn[APP_MAX_FN_SIZE] = {0};
    APPLIB_JSON_OBJECT *JsonObject = NULL;
    int ErrorCode = 0;
    int ReturnValue = 0;

    APP_REMOVEFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_FROM_NETCTRL);
    if (UserSetting->PhotoPref.PhotoCapMode == PHOTO_CAP_MODE_PRECISE) {
        ErrorCode = ERROR_NETCTRL_UNKNOWN_ERROR;
        FileObjID = AppLibStorageDmf_GetLastFilePos(APPLIB_DCF_MEDIA_IMAGE, DCIM_HDLR);
        if (FileObjID > 0) {
            ReturnValue = AppLibStorageDmf_GetFileName(APPLIB_DCF_MEDIA_IMAGE, ".JPG", APPLIB_DCF_EXT_OBJECT_SPLIT_FILE, DCIM_HDLR, 0, CUR_OBJ(FileObjID), CurFn);
            if (ReturnValue == 0) {
                AmbaPrint("[rec_connected_cam] <netctrl_capture_done> CurFn: %s", CurFn);
                ErrorCode = 0;
            }
        }

        if (AppLibNetJsonUtility_CreateObject(&JsonObject) == 0) {
            AppLibNetJsonUtility_AddIntegerObject(JsonObject,"rval", ErrorCode);
            AppLibNetJsonUtility_AddIntegerObject(JsonObject,"msg_id", AMBA_TAKE_PHOTO);
            if (ErrorCode == 0) {
                AppLibNetJsonUtility_AddStringObject(JsonObject,"param", CurFn);
            }

            SendJsonString(JsonObject);
            AppLibNetJsonUtility_FreeJsonObject(JsonObject);
        } else {
            AppLibNetControl_ReplyErrorCode(AMBA_TAKE_PHOTO, ERROR_NETCTRL_UNKNOWN_ERROR);
        }

    }
    else if (UserSetting->PhotoPref.PhotoCapMode == PHOTO_CAP_MODE_BURST) {
        ErrorCode = ERROR_NETCTRL_UNKNOWN_ERROR;
        FileObjID = AppLibStorageDmf_GetLastFilePos(APPLIB_DCF_MEDIA_IMAGE, DCIM_HDLR);
        if (FileObjID > 0) {
            ReturnValue = AppLibStorageDmf_GetFileName(APPLIB_DCF_MEDIA_IMAGE, ".JPG", APPLIB_DCF_EXT_OBJECT_SPLIT_FILE, DCIM_HDLR, 0, CUR_OBJ(FileObjID), CurFn);
            if (ReturnValue == 0) {
                AmbaPrint("[rec_connected_cam] <netctrl_capture_done> CurFn: %s", CurFn);
                ErrorCode = 0;
            }
        }

        if (AppLibNetJsonUtility_CreateObject(&JsonObject) == 0) {
            AppLibNetJsonUtility_AddIntegerObject(JsonObject,"msg_id",AMBA_NOTIFICATION);
            AppLibNetJsonUtility_AddStringObject(JsonObject,"type", "continue_burst_complete");

            if (ErrorCode == 0) {
                AppLibNetJsonUtility_AddStringObject(JsonObject,"param", CurFn);
            }

            SendJsonString(JsonObject);
            AppLibNetJsonUtility_FreeJsonObject(JsonObject);
        } else {
            AppLibNetControl_ReplyErrorCode(AMBA_TAKE_PHOTO, ERROR_NETCTRL_UNKNOWN_ERROR);
        }
    } else if (UserSetting->PhotoPref.PhotoCapMode == PHOTO_CAP_MODE_PES) {
        AppLibNetControl_ReplyErrorCode(AMBA_CONTINUE_CAPTURE_STOP, 0);
    } else {
        return -1;
    }

    return ReturnValue;
}

static int rec_connected_cam_netctrl_piv_done(void)
{
    UINT64 FileObjID = 0;
    char CurFn[APP_MAX_FN_SIZE] = {0};
    APPLIB_JSON_OBJECT *JsonObject = NULL;
    int ErrorCode = ERROR_NETCTRL_UNKNOWN_ERROR;
    int ReturnValue = 0;

    AmbaPrint("[rec_connected_cam] <netctrl_piv_done>");
    APP_REMOVEFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_FROM_NETCTRL);

    FileObjID = AppLibStorageDmf_GetLastFilePos(APPLIB_DCF_MEDIA_IMAGE, DCIM_HDLR);
    if (FileObjID > 0) {
        ReturnValue = AppLibStorageDmf_GetFileName(APPLIB_DCF_MEDIA_IMAGE, ".JPG", APPLIB_DCF_EXT_OBJECT_SPLIT_FILE, DCIM_HDLR, 0, CUR_OBJ(FileObjID), CurFn);
        if (ReturnValue == 0) {
            AmbaPrint("[rec_connected_cam] <netctrl_piv_done> CurFn: %s", CurFn);
            ErrorCode = 0;
        }
    }

    if (AppLibNetJsonUtility_CreateObject(&JsonObject) == 0) {
        AppLibNetJsonUtility_AddIntegerObject(JsonObject,"rval", ErrorCode);
        AppLibNetJsonUtility_AddIntegerObject(JsonObject,"msg_id", AMBA_TAKE_PHOTO);
        if (ErrorCode == 0) {
            AppLibNetJsonUtility_AddStringObject(JsonObject,"param", CurFn);
        }

        SendJsonString(JsonObject);
        AppLibNetJsonUtility_FreeJsonObject(JsonObject);
    } else {
        AppLibNetControl_ReplyErrorCode(AMBA_TAKE_PHOTO, ERROR_NETCTRL_UNKNOWN_ERROR);
    }

    return ReturnValue;
}

#endif

/**
 *  @brief The functions of recorder application
 *
 *  The functions of recorder application
 *
 *  @param[in] funcId Function id
 *  @param[in] param1 First parameter
 *  @param[in] param2 Second parameter
 *
 *  @return >=0 success, <0 failure
 */
int rec_connected_cam_func(UINT32 funcId, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;

    switch (funcId) {
    case REC_CONNECTED_CAM_INIT:
        ReturnValue = rec_connected_cam_init();
        break;
    case REC_CONNECTED_CAM_START:
        ReturnValue = rec_connected_cam_start();
        break;
    case REC_CONNECTED_CAM_STOP:
        ReturnValue = rec_connected_cam_stop();
        break;
    case REC_CONNECTED_CAM_LIVEVIEW_STATE:
        ReturnValue = rec_connected_cam_do_liveview_state();
        break;
    case REC_CONNECTED_CAM_LIVEVIEW_POST_ACTION:
        ReturnValue = rec_connected_cam_liveview_post_action();
        break;
    case REC_CONNECTED_CAM_SELFTIMER_START:
        ReturnValue = rec_connected_cam_selftimer( 1, param1, param2);
        break;
    case REC_CONNECTED_CAM_SELFTIMER_STOP:
        ReturnValue = rec_connected_cam_selftimer( 0, param1, param2);
        break;
    case REC_CONNECTED_CAM_CAPTURE:
        ReturnValue = rec_connected_cam_capture();
        break;
    case REC_CONNECTED_CAM_CAPTURE_PIV:
        ReturnValue = rec_connected_cam_capture_piv();
        break;
    case REC_CONNECTED_CAM_CAPTURE_COMPLETE:
        ReturnValue = rec_connected_cam_capture_complete();
        break;
    case REC_CONNECTED_CAM_CAPTURE_BG_PROCESS_DONE:
        ReturnValue = rec_connected_cam_capture_bg_process_done();
        break;
    case REC_CONNECTED_CAM_RECORD_START:
		if(app_status.HdmiPluginFlag == 0) //keith, avoid enter record in HDMI
        	ReturnValue = rec_connected_cam_record_start();
        break;
    case REC_CONNECTED_CAM_RECORD_PAUSE:
        ReturnValue = rec_connected_cam_record_pause();
        break;
    case REC_CONNECTED_CAM_RECORD_RESUME:
        ReturnValue = rec_connected_cam_record_resume();
        break;
    case REC_CONNECTED_CAM_RECORD_STOP:
        ReturnValue = rec_connected_cam_record_stop();
        break;
    case REC_CONNECTED_CAM_RECORD_AUTO_START:
        ReturnValue = rec_connected_cam_record_auto_start();
        break;
    case REC_CONNECTED_CAM_MUXER_OPEN:
        ReturnValue = rec_connected_cam_muxer_open(param1);
        break;
    case REC_CONNECTED_CAM_MUXER_END:
        ReturnValue = rec_connected_cam_muxer_end(param1);
        break;
    case REC_CONNECTED_CAM_MUXER_REACH_LIMIT:
        ReturnValue = rec_connected_cam_muxer_reach_limit(param1);
        break;
    case REC_CONNECTED_CAM_MUXER_REACH_LIMIT_EVENTRECORD:
        ReturnValue = rec_connected_cam_muxer_reach_limit_eventrecord(param1);
        break;
    case REC_CONNECTED_CAM_EVENTRECORD_START:
        ReturnValue = rec_connected_cam_eventrecord_start();
        break;
    case REC_CONNECTED_CAM_MUXER_STREAM_ERROR:
        ReturnValue = rec_connected_cam_muxer_stream_error();
        break;
    case REC_CONNECTED_CAM_ERROR_MEMORY_RUNOUT:
        ReturnValue = rec_connected_cam_error_memory_runout();
        break;
    case REC_CONNECTED_CAM_ERROR_STORAGE_RUNOUT:
        ReturnValue = rec_connected_cam_error_storage_runout();
        break;
    case REC_CONNECTED_CAM_ERROR_STORAGE_IO:
        ReturnValue = rec_connected_cam_error_storage_io();
        break;
    case REC_CONNECTED_CAM_ERROR_LOOP_ENC_ERR:
        ReturnValue = rec_connected_cam_error_loop_enc_err(param1);
        break;
    case REC_CONNECTED_CAM_LOOP_ENC_DONE:
        ReturnValue = rec_connected_cam_loop_enc_done();
        break;
    case REC_CONNECTED_CAM_SWITCH_APP:
        ReturnValue = rec_connected_cam_switch_app();
        break;
    case REC_CONNECTED_CAM_SET_VIDEO_RES:
        ReturnValue = rec_connected_cam_set_videoRes(param1, 1);
        break;
    case REC_CONNECTED_CAM_SET_VIDEO_QUALITY:
        ReturnValue = rec_connected_cam_set_video_quality(param1, 1);
        break;
    case REC_CONNECTED_CAM_SET_VIDEO_PRE_RECORD:
        break;
    case REC_CONNECTED_CAM_SET_VIDEO_TIME_LAPSE:
        ReturnValue = rec_connected_cam_set_time_lapse(param1, 1);
        break;
    case REC_CONNECTED_CAM_SET_VIDEO_DUAL_STREAMS:
        break;
    case REC_CONNECTED_CAM_SET_VIDEO_RECORD_MODE:
        break;
    case REC_CONNECTED_CAM_SET_PHOTO_SIZE:
        ReturnValue = rec_connected_cam_set_photo_size(param1, param2);
        break;
    case REC_CONNECTED_CAM_SET_PHOTO_QUALITY:
        ReturnValue = rec_connected_cam_set_photo_quality(param1, 1);
        break;
    case REC_CONNECTED_CAM_SET_SELFTIMER:
        ReturnValue = rec_connected_cam_set_selftimer(param1, 1);
        break;
    case REC_CONNECTED_CAM_SET_ENC_MODE:
        ReturnValue = rec_connected_cam_set_enc_mode(param1);
        break;
    case REC_CONNECTED_CAM_SET_DMF_MODE:
        ReturnValue = rec_connected_cam_set_dmf_mode(param1);
        break;
    case REC_CONNECTED_CAM_CARD_REMOVED:
        ReturnValue = rec_connected_cam_card_removed();
        break;
    case REC_CONNECTED_CAM_CARD_ERROR_REMOVED:
        ReturnValue = rec_connected_cam_card_error_removed();
        break;
    case REC_CONNECTED_CAM_CARD_NEW_INSERT:
        ReturnValue = rec_connected_cam_card_new_insert();
        break;
    case REC_CONNECTED_CAM_CARD_STORAGE_IDLE:
        ReturnValue = rec_connected_cam_card_storage_idle();
        break;
    case REC_CONNECTED_CAM_CARD_STORAGE_BUSY:
        ReturnValue = rec_connected_cam_card_storage_busy();
        break;
    case REC_CONNECTED_CAM_CARD_CHECK_STATUS:
        /**param1 0: video if space not enough will do loop encode 1: photo if space not enough do nothing*/
        ReturnValue = rec_connected_cam_card_check_status(param1);
        break;
    case REC_CONNECTED_CAM_CARD_FULL_HANDLE:
        ReturnValue = rec_connected_cam_card_full_handle();
        break;
    case REC_CONNECTED_CAM_CARD_FULL_HANDLE_EVENT:
        ReturnValue = rec_connected_cam_card_full_handle_event();
        break;
    case REC_CONNECTED_CAM_FILE_ID_UPDATE:
        ReturnValue = rec_connected_cam_file_id_update(param1);
        break;
    case REC_CONNECTED_CAM_WIDGET_CLOSED:
        ReturnValue = rec_connected_cam_widget_closed();
        break;
    case REC_CONNECTED_CAM_SET_SYSTEM_TYPE:
        ReturnValue = rec_connected_cam_set_system_type();
        break;
    case REC_CONNECTED_CAM_UPDATE_FCHAN_VOUT:
        ReturnValue = rec_connected_cam_update_fchan_vout(param1);
        break;
    case REC_CONNECTED_CAM_UPDATE_DCHAN_VOUT:
        break;
    case REC_CONNECTED_CAM_CHANGE_DISPLAY:
        ReturnValue = rec_connected_cam_change_display();
        break;
    case REC_CONNECTED_CAM_CHANGE_OSD:
        ReturnValue = rec_connected_cam_change_osd();
        break;
    case REC_CONNECTED_CAM_USB_CONNECT:
        ReturnValue = rec_connected_cam_usb_connect();
        break;
    case REC_CONNECTED_CAM_GUI_INIT_SHOW:
        ReturnValue = rec_connected_cam_start_show_gui();
        break;
    case REC_CONNECTED_CAM_UPDATE_BAT_POWER_STATUS:
        ReturnValue = rec_connected_cam_update_bat_power_status(param1);
        break;
    case REC_CONNECTED_CAM_WARNING_MSG_SHOW_START:
        ReturnValue = rec_connected_cam_warning_msg_show( 1, param1, param2);
        break;
    case REC_CONNECTED_CAM_WARNING_MSG_SHOW_STOP:
        ReturnValue = rec_connected_cam_warning_msg_show( 0, param1, param2);
        break;
    case REC_CONNECTED_CAM_CARD_FMT_NONOPTIMUM:
        ReturnValue = rec_connected_cam_card_fmt_nonoptimum();
        break;
    case REC_CONNECTED_CAM_ADAS_EVENT:
        ReturnValue = rec_connected_cam_adas_event_handler(param1);
        break;
    case REC_CONNECTED_CAM_ADAS_FUNCTION_INIT:
        ReturnValue = rec_connected_cam_adas_function_init();
        break;
    case REC_CONNECTED_CAM_ADAS_UPDATE_PARAM:
        ReturnValue = rec_connected_cam_adas_update_param();
        break;
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    case REC_CONNECTED_CAM_BOSS_BOOTED:
        ReturnValue = rec_connected_cam_boos_booted();
        break;
    case REC_CONNECTED_CAM_VF_START:
        ReturnValue = rec_connected_cam_vf_start();
        break;
    case REC_CONNECTED_CAM_VF_STOP:
        ReturnValue = rec_connected_cam_vf_stop();
        break;
    case REC_CONNECTED_CAM_CAPTURE_ON_VF:
        ReturnValue = rec_connected_cam_capture_on_vf();
        break;
    case REC_CONNECTED_CAM_VF_SWITCH_TO_RECORD:
        ReturnValue = rec_connected_cam_vf_switch_to_record();
        break;
    case REC_CONNECTED_CAM_NETFIFO_EVENT_START:
        ReturnValue = rec_connected_cam_netfifo_event_start();
        break;
    case REC_CONNECTED_CAM_NETFIFO_EVENT_STOP:
        ReturnValue = rec_connected_cam_netfifo_event_stop();
        break;
    case REC_CONNECTED_CAM_NETCTRL_CAPTURE_DONE:
        ReturnValue = rec_connected_cam_netctrl_capture_done();
        break;
    case REC_CONNECTED_CAM_NETCTRL_PIV_DONE:
        ReturnValue = rec_connected_cam_netctrl_piv_done();
        break;
#endif
    default:
        break;
    }

    return ReturnValue;
}
