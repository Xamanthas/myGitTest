/**
 * @file app/connected/app/system/handler.c
 *
 * Application Handler file.
 *
 * History:
 *    2013/08/13 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <framework/appmaintask.h>
#include <framework/appdefines.h>
#include <framework/apphmi.h>
#include <framework/appmgt.h>
#include "AmbaPWC.h"
#include <apps/apps.h>
#include <apps/flow/widget/widgetmgt.h>
#include <applib.h>
#include <system/ApplibSys_Lcd.h>
#include <peripheral_mod/ui/button/button_op.h>
#include <peripheral_mod/ui/ir/irbutton_op.h>
#include <apps/gui/resource/gui_settle.h>
#include <apps/flow/widget/dialog/dialog.h>
#include <apps/flow/widget/dialog/dialog_table.h>
#include "status.h"
#include "app_pref.h"
#include "app_util.h"
#include <calibration/ApplibCalibMgr.h>
#include <mw/net/NetUtility.h>
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
#include <net/ApplibNet_Base.h>
#include <net/ApplibNet_IPCTest.h>
#include <net/ApplibNet_JsonUtility.h>
#include "onlinePlayback.h"
#endif
#include <imgproc/AmbaImg_Proc.h>
#include <imgproc/AmbaImg_Impl_Cmd.h>

#include <FirmwareLoader.h>

//#define CONNECTED_CAM_DEBUG
#if defined(CONNECTED_CAM_DEBUG)
#define DBGMSG  AmbaPrint
#define DBGMSGc2    AmbaPrintColor
#else
#define DBGMSG(...)
#define DBGMSGc2(...)
#endif
#include <AmbaShell.h>

#ifdef CONFIG_BUILD_ASD_LIB //temp, need to add inc and link file later
int AsdDebugPrintMsg(char* pszDiscript, UINT32 MsgId, UINT32 MsgParam, UINT32 MsgParam2);

#endif
static APP_APP_s *CarCamApps[APPS_NUM];


/**
 *  @brief Register the sensor
 *
 *  Register the sensor
 *
 *  @return >=0 success, <0 failure
 */
static int AppHandler_RegisterSensor(void)
{
    extern UINT32 AppLibIQ_ImageInit(UINT32 chNo);
    int ReturnValue = 0;

#ifdef CONFIG_SENSOR_SONY_IMX117
    {
        extern int AppSensor_register_imx117_a12(void);
        extern int AppIQParamImx117_A12_Register(UINT32 chNo);
        extern CALIBRATION_ADJUST_PARAM_s AmbaCalibParamsImx117;

        ReturnValue = AppSensor_register_imx117_a12();
        AppLibSysSensor_SetIQChannelCount(1);

        AppLibIQ_ImageInit(1);
        AppIQParamImx117_A12_Register(0);

        AppLibCalibTableSet(&AmbaCalibParamsImx117);
    }
#endif

#ifdef CONFIG_SENSOR_SONY_IMX206
    {
        extern int AppSensor_register_imx206_a12(void);
        extern int AppIQParamImx206_A12_Register(UINT32 chNo);
        extern CALIBRATION_ADJUST_PARAM_s AmbaCalibParamsImx206;

        ReturnValue = AppSensor_register_imx206_a12();
        AppLibSysSensor_SetIQChannelCount(1);

        AppLibIQ_ImageInit(1);
        AppIQParamImx206_A12_Register(0);

        AppLibCalibTableSet(&AmbaCalibParamsImx206);
    }
#endif

#ifdef CONFIG_SENSOR_SONY_IMX290
    {
        extern int AppSensor_register_imx290_a12(void);
        extern int AmbaIQParamImx290Hdr_A12_Register(UINT32 chNo);
        extern int AppIQParamImx290_A12_Register(UINT32 chNo);
        extern CALIBRATION_ADJUST_PARAM_s AmbaCalibParamsImx290;

        ReturnValue = AppSensor_register_imx290_a12();
        AppLibSysSensor_SetIQChannelCount(2);

        AppLibIQ_ImageInit(2);/**Has HDR Mode, need 2 channel*/
        AppIQParamImx290_A12_Register(0);
        AmbaIQParamImx290Hdr_A12_Register(1);
        AppLibCalibTableSet(&AmbaCalibParamsImx290);
    }
#endif


#ifdef CONFIG_SENSOR_OV4689
    {
        extern int AppSensor_register_ov4689_a12(void);
        extern int AppIQParamOv4689_A12_Register(UINT32 chNo);
        extern int AmbaIQParamOv4689Hdr_A12_Register(UINT32 chNo);
        extern CALIBRATION_ADJUST_PARAM_s AmbaCalibParamsOv4689;

        ReturnValue = AppSensor_register_ov4689_a12();
        AppLibSysSensor_SetIQChannelCount(2);

        AppLibIQ_ImageInit(2);/**Ov4689 has HDR Mode, need 2 channel*/
        AppIQParamOv4689_A12_Register(0);
        AmbaIQParamOv4689Hdr_A12_Register(1);

        AppLibCalibTableSet(&AmbaCalibParamsOv4689);
    }
#endif

#ifdef CONFIG_SENSOR_AR0330_PARALLEL
    {
        extern int AppSensor_register_ar0330_parallel_a12(void);
        extern int AppIQParamAr0330_Parallel_A12_Register(UINT32 chNo);
        extern CALIBRATION_ADJUST_PARAM_s AmbaCalibParamsAr0330Parallel;

        ReturnValue = AppSensor_register_ar0330_parallel_a12();
        AppLibSysSensor_SetIQChannelCount(1);

        AppLibIQ_ImageInit(1);
        AppIQParamAr0330_Parallel_A12_Register(0);

        AppLibCalibTableSet(&AmbaCalibParamsAr0330Parallel);
    }
#endif

#ifdef CONFIG_SENSOR_AR0230
    {
        extern int AppSensor_register_ar0230_a12(void);
        extern int AppIQParamAr0230_A12_Register(UINT32 chNo);
        extern int AppIQParamAr0230Hdr_A12_Register(UINT32 chNo);
        extern CALIBRATION_ADJUST_PARAM_s AmbaCalibParamsAr0230;

        ReturnValue = AppSensor_register_ar0230_a12();
        AppLibSysSensor_SetIQChannelCount(2);

        AppLibIQ_ImageInit(2);/**AR0230 has HDR Mode, need 2 channel*/
        AppIQParamAr0230_A12_Register(0);
        AppIQParamAr0230Hdr_A12_Register(1);

        AppLibCalibTableSet(&AmbaCalibParamsAr0230);
    }
#endif

#ifdef CONFIG_SENSOR_MN34120
    {
        extern int AppSensor_register_mn34120_a12(void);
        extern int AppIQParamMn34120_A12_Register(UINT32 chNo);
        extern CALIBRATION_ADJUST_PARAM_s AmbaCalibParamsMn34120;

        ReturnValue = AppSensor_register_mn34120_a12();
        AppLibSysSensor_SetIQChannelCount(1);

        AppLibIQ_ImageInit(1);
        AppIQParamMn34120_A12_Register(0);

        AppLibCalibTableSet(&AmbaCalibParamsMn34120);
    }
#endif

#ifdef CONFIG_SENSOR_MN34229
        {
            extern int AppSensor_register_mn34229_a12(void);
            extern int AppIQParamMn34229_A12_Register(UINT32 chNo);
            extern CALIBRATION_ADJUST_PARAM_s AmbaCalibParamsMn34229;
        ReturnValue = AppSensor_register_mn34229_a12();
        AppLibSysSensor_SetIQChannelCount(1);

        AppLibIQ_ImageInit(1);
            AppIQParamMn34229_A12_Register(0);
            AppLibCalibTableSet(&AmbaCalibParamsMn34229);
        }
#endif

#ifdef CONFIG_SENSOR_MN34222
    {
        extern int AppSensor_register_mn34222_a12(void);
        extern int AmbaIQParamMn34222Hdr_A12_Register(UINT32 chNo);
        extern int AppIQParamMn34222_A12_Register(UINT32 chNo);
        extern CALIBRATION_ADJUST_PARAM_s AmbaCalibParamsMn34222;

        ReturnValue = AppSensor_register_mn34222_a12();
        AppLibSysSensor_SetIQChannelCount(2);

        AppLibIQ_ImageInit(2);
        AppIQParamMn34222_A12_Register(0);
        AmbaIQParamMn34222Hdr_A12_Register(1);
        AppLibCalibTableSet(&AmbaCalibParamsMn34222);
    }
#endif

    return ReturnValue;
}

/**
 *  @brief Register Lens
 *
 *  Register Lens
 *
 *  @return >=0 success, <0 failure
 */
static int AppHandler_RegisterLens(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief Register Gyro
 *
 *  Register Gyro
 *
 *  @return >=0 success, <0 failure
 */
static int AppHandler_RegisterGyro(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief Register Gyro
 *
 *  Register Gyro
 *
 *  @return >=0 success, <0 failure
 */
static int AppHandler_RegisterGps(void)
{
    int ReturnValue = 0;

#ifdef CONFIG_GPS_UBLOX6_GM6XX
    {
        extern int AppGps_RegisterGM6xx(void);
        AppGps_RegisterGM6xx();
    }
    AppLibSysGps_Init();
#endif

    return ReturnValue;
}

/**
 *  @brief Register Lcd
 *
 *  Register Lcd
 *
 *  @param [in] lcdChanID Lcd channel id
 *
 *  @return >=0 success, <0 failure
 */
static int AppRegisterLcd(UINT32 lcdChanID)
{
    int ReturnValue = 0;

#ifdef CONFIG_LCD_T20P52
    AmbaPrint("[App-LCD] LCD panel selected: t20p52");
    {
        extern int AppLcd_RegisterT20P52(UINT32 lcdChanID);
        ReturnValue = AppLcd_RegisterT20P52(lcdChanID);
    }
#endif
#ifdef CONFIG_LCD_T30P61
        AmbaPrint("[App-LCD] LCD panel selected: t30p61");
        {
            extern int AppLcd_RegisterT30P61(UINT32 lcdChanID);
            ReturnValue = AppLcd_RegisterT30P61(lcdChanID);
        }
#endif
#ifdef CONFIG_LCD_WDF9648W
        AmbaPrint("[App-LCD] LCD panel selected: wdf9648w");
        {
            extern int AppLcd_RegisterWdf9648w(UINT32 lcdChanID);
            ReturnValue = AppLcd_RegisterWdf9648w(lcdChanID);
        }
#endif
#ifdef CONFIG_LCD_EG020THEG1
		AmbaPrint("[App-LCD] LCD panel selected: EG020THEG1");
		{
			extern int AppLcd_RegisterEG020THEG1(UINT32 lcdChannelId);
			ReturnValue = AppLcd_RegisterEG020THEG1(lcdChanID);
		}
#endif

    return ReturnValue;
}

static int AppHandler_DcfInit(void)
{
    APPLIB_DCF_INIT_CFG_s InitCfg;
    void *DcfBufRaw = NULL;
    extern AMBA_KAL_BYTE_POOL_t G_MMPL;

    /**< Get default config */
    AmbaPrint("[AppHandler] DCF init start!");
    AppLibDCF_GetDefaultInitCfg(&InitCfg);
    InitCfg.CfsCfg.TaskInfo.Priority = APP_CFS_SCHDLR_PRIORITY;
    InitCfg.DcfInitCfg.TaskInfo.Priority = APP_DCF_PRIORITY;

    InitCfg.DcfInitCfg.DefTblCfg.MaxFile = 10000 * InitCfg.DcfInitCfg.DefTblCfg.MaxHdlr;
    InitCfg.BufferSize = AppLibDCF_GetRequiredBufSize(&InitCfg.CfsCfg, &InitCfg.DcfInitCfg, InitCfg.IntObjAmount);

    /**< Allocate memory */
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **) &InitCfg.Buffer, &DcfBufRaw, InitCfg.BufferSize, 32) == OK) {
        /**< Init AppLibDCF */
        if (AppLibDCF_Init(&InitCfg) >= 0) {
            AmbaPrint("[AppMainTask] DCF init ok!");
            return AMP_OK; /**< Every thing is OK! */
        }
        AmbaPrintColor(RED, "[AppHandler] AppLib DCF init fail!");
    } else AmbaPrintColor(RED, "[AppHandler] Allocate memory fail");
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 *  @brief Pre-initialization flows for booting
 *
 *  Pre-initialization flows for booting
 *
 *  @return >=0 success, <0 failure
 */
static int AppHandler_AppPreInit(void)
{
    int ReturnValue = 0;

    /**
    * Application status and perference should be initialized
    * before initialize app library which may actually config
    * system states
    **/
/** Load the user's perference. and initialize the status of app.*/
    AppPref_InitPref();
    AppPref_Load();

#if 1
    /** Register applications' flow. */
	app_status.Type = APP_TYPE_DV;

    DBGMSG("#### AppAppMgt_Init: call");
    AppAppMgt_Init(CarCamApps, APPS_NUM);
    APP_MAIN = AppAppMgt_Register(&app_main);
    APP_REC_CONNECTED_CAM = AppAppMgt_Register(&app_rec_connected_cam);
    APP_PB_VIDEO = AppAppMgt_Register(&app_pb_video);
    APP_PB_PHOTO = AppAppMgt_Register(&app_pb_photo);
    APP_PB_MULTI = AppAppMgt_Register(&app_pb_multi);
    APP_THUMB_MOTION = AppAppMgt_Register(&app_thumb_motion);
    APP_USB_MSC = AppAppMgt_Register(&app_usb_msc);
    APP_USB_AMAGE = AppAppMgt_Register(&app_usb_amage);
    APP_MISC_FWUPDATE = AppAppMgt_Register(&app_misc_fwupdate);
    APP_MISC_MVRECOVER = AppAppMgt_Register(&app_misc_mvrecover);
    APP_MISC_FORMATCARD = AppAppMgt_Register(&app_misc_formatcard);
    APP_MISC_CALIBUPDATE = AppAppMgt_Register(&app_misc_calibupdate);
    APP_MISC_DEFSETTING = AppAppMgt_Register(&app_misc_defsetting);
#if defined (ENABLE_HDMI_TEST)
    APP_HDMI_TEST = AppAppMgt_Register(&app_hdmi_test);
#endif
    DBGMSG("#### AppAppMgt_Init: end");
#endif
    return ReturnValue;
}

/**
 *  @brief Initializations can be initialized before pipe initialization
 *
 *  Initializations can be initialized before pipe initialization
 *
 *  @return >=0 success, <0 failure
 */
static int AppHandler_AppInit(void)
{
    int ReturnValue = 0;

    /** Initialize 3A . */
    /** Register and initialize sensor.*/
    AppLibSysSensor_PreInit();
    AppHandler_RegisterSensor();
    AppLibSysSensor_Init();

    /** Load the user's perference. and initialize the status of app.*/
    //AppPref_InitPref();
    //AppPref_Load();

    UserSetting->VideoPref.DualStreams = VIDEO_DUAL_STREAMS_ON;
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    /** Enable net stream by default.*/
    UserSetting->VideoPref.StreamType = STREAM_TYPE_RTSP;
#endif
    /**default set time lapse of PES = 500MS*/
    UserSetting->PhotoPref.TimeLapse = PHOTO_TIME_LAPSE_500MS;
    /* default stamp as STAMP_DATE_TIME */
    UserSetting->VideoPref.VideoDateTimeStamp = STAMP_DATE_TIME;
    UserSetting->PhotoPref.PhotoTimeStamp = STAMP_DATE_TIME;
	#ifdef CONFIG_ASD_USB_RS232_ENABLE
	UserSetting->SetupPref.USBMode = USB_MODE_RS232;
	#endif
    /** Register and initialize lens.*/
    AppLibSysLens_PreInit();
    AppHandler_RegisterLens();
    AppLibSysLens_Init();

    /** Register and initialize gyro sensor.*/
    AppLibSysGyro_PreInit();
    AppHandler_RegisterGyro();
    AppLibSysGyro_Init();

     /** Initialize calibration.*/
    AppLibCalib_Load();

    /** Initialize image algo.*/
    AppLibImage_Init();
    /* Enable 3A. */
    AppLibImage_Set3A(1);

    /** Register and initialize LCD. */
    AppLibSysLcd_PreInit();
    AppRegisterLcd(LCD_CH_DCHAN);
    AppLibSysLcd_Init(LCD_CH_DCHAN);
    AppLibSysLcd_SetSeamless(LCD_CH_DCHAN);
    AppLibSysLcd_RegVoutIRQ();

    /** Initial display. */
    AppLibDisp_Init();
    AppLibDisp_InitWindow();
    AppLibDisp_EnableChan(DISP_CH_DUAL);

    /** Initilalize GPS */
    AppLibSysGps_PreInit();
    AppHandler_RegisterGps();

    /** Initialize jack detection.*/
    //AppUtil_JackDetectInit();

    /** Check vout status.*/
    //AppUtil_CheckVoutJack();
    AppLibDisp_SelectDevice(DISP_CH_FCHAN | DISP_CH_DCHAN, DISP_ANY_DEV);

    /* Initialize the USB.*/
    AppLibUSB_Init();
    ReturnValue = AppLibUSB_InitJack();

    AppUtil_StatusInit();

    /** Initial graphic. */
    AppGui_Init();
    app_status.Type = APP_TYPE_DV;

    /*For testing.*/
    app_status.PowerType = APP_POWER_ADAPTER;
#ifdef CONFIG_ASD_HDMI_PREVIEW
    app_status.FchanDecModeOnly = 0;
#else
	app_status.FchanDecModeOnly = 1;
#endif

    //AppLibVideoEnc_SetRecMode(REC_MODE_VIDEO_ONLY);

#ifdef CONFIG_APP_CONNECTED_PARTIAL_LOAD
	if (AppUtil_GetStartApp(0) != APP_REC_CONNECTED_CAM)
		AmpFwLoader_WaitComplete(5000);
#endif

    /** Switch the application. */
    AppUtil_SwitchApp(AppUtil_GetStartApp(0));

    return ReturnValue;
}

/**
 *  @brief Initializations can be initialized after pipe initialization
 *
 *  Initializations can be initialized after pipe initialization
 *
 *  @return >=0 success, <0 failure
 */
int AppHandler_AppPostInit(void)
{
    /** Initial muxer and demuxer. */
    AppLibFormat_Init();

    /** Set the storage type for index files */
    #ifdef CONFIG_ENABLE_EMMC_BOOT
    AppLibIndex_SetStorageType(APPLIB_INDEX_FILE_STROAGE_TYPE_MEMORY);
    #else
    AppLibIndex_SetStorageType(APPLIB_INDEX_FILE_STROAGE_TYPE_NAND);
    #endif

#ifdef CONFIG_APP_CONNECTED_PARTIAL_LOAD
    AppHandler_DcfInit();

    /* Initialize the audio codec. */
    {
        extern void AmbaUserAudioCodec_Init(void);
        AmbaUserAudioCodec_Init();
    }
#endif

    AppLibFormat_SetDualFileSaving(VIDEO_DUAL_SAVING_ON);
    AppLibFormat_SetPriStreamFileSizeAlignment(20<<20);//20MB
    AppLibFormat_SetSecStreamFileSizeAlignment(20<<20);//20MB

    /** Initial storage DMF. */
    AppLibStorageDmf_Init(0);

    /** Inital DCF config*/
    {
        APPLIB_DCF_CFG_s InitConfig = {0};

        InitConfig.NumberMode = UserSetting->SetupPref.DMFMode;
        InitConfig.BrowseMode = APPLIB_DCF_MEDIA_DCIM;
        InitConfig.PhotoLastIdx = UserSetting->SetupPref.DmfPhotoLastIdx;
        InitConfig.SoundLastIdx= UserSetting->SetupPref.DmfSoundLastIdx;
        InitConfig.VideoLastIdx = UserSetting->SetupPref.DmfVideoLastIdx;
        InitConfig.MixLastIdx = UserSetting->SetupPref.DmfMixLastIdx;
        AppLibDCF_SetDefaultCfg(&InitConfig);
    }

    /** Initial card. */
    app_status.CardBusySlot = NO_BUSY_SLOT;
    AppLibCard_Init();
    AppUtil_PollingAllSlots();

    /** Initialize Buttons.*/
    AppButtonOp_Init();
    AppIRButtonOp_Init();

    /** Initialize asynchronous operations */
    AppLibComSvcAsyncOp_Init();

    /** Initialize Timer.*/
    AppLibComSvcTimer_Init();

    /** Initialize Widget.*/
    AppWidget_Init();
    AppDialog_SetDialogTable(dialog_subject_str_tbl);

    /** Initialize Jack monitor. */
    AppUtil_JackMonitor();

    /* Add the test commands of application. */
    {
        extern int App_TestAdd(void);
        App_TestAdd();
    }

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    /* The second param of  AppLibNetBase_InitAmbaLink() is used to indicate that
          hibernation is enabled or not. '1' means enabled, otherwise disabled.
          This setting should be corresponded with linux kenel image's configuration. */
    AppLibNetBase_InitAmbaLink(APPLIB_G_MMPL, 1);
    app_status.NetCtrlSessionOn = 0;

    AppLibNetIPCTest_Add(APPLIB_G_MMPL);
#endif

    return 0;
}

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
static int AppHandler_StartNetService(void)
{
    /* net services should be intiailized or started after linux booted. */
    AppLibNetControl_Init(APPLIB_G_MMPL);

    AppLibNetFifo_PlaybackRegisterApp(OnlinePlayback);

    AppLibNetFifo_Init();
    AppLibNetFifo_EnablePlayback();

    /* stream mode should be set before starting RTSP server. */
    AppLibNetFifo_SetStreamMode(NET_STREAM_MODE_VIDEO_ONLY);
    AppLibNetFifo_StartRTSPServer();

	/* Register 'pt' service and execute linux 'amba_pt' command to enable linux pseudo terminal. */
	#ifdef CONFIG_ASD_USB_RS232_ENABLE
	AppLibNetIPC_RegisterService("pt");
	AppLibNetIPC_ClientExec2("amba_pt");
	#endif
    return 0;
}

/* param 'Token' is only meaningful when SessionStart is 1. */
static int AppHandler_NetControlSession(UINT8 SessionStart, UINT32 Token)
{
    int MsgId = 0;
    int ReturnValue = 0;
    APPLIB_JSON_OBJECT *JsonObject = NULL;
    APPLIB_JSON_STRING *JsonString = NULL;
    char *ReplyString = NULL;

    MsgId = (SessionStart == 1) ? AMBA_START_SESSION : AMBA_STOP_SESSION;

    ReturnValue = AppLibNetJsonUtility_CreateObject(&JsonObject);
    if (ReturnValue == 0) {
        if (SessionStart == 1) {
            AmbaPrintColor(GREEN,"[App Handler] AMSG_NETCTRL_SESSION_START");
            app_status.NetCtrlSessionOn = 1;

            AppLibNetJsonUtility_AddIntegerObject(JsonObject,"rval", 0);
            AppLibNetJsonUtility_AddIntegerObject(JsonObject,"msg_id", MsgId);
            AppLibNetJsonUtility_AddIntegerObject(JsonObject,"param", Token);
        } else {
            AmbaPrintColor(GREEN,"[App Handler] AMSG_NETCTRL_SESSION_STOP");
            app_status.NetCtrlSessionOn = 0;

            AppLibNetJsonUtility_AddIntegerObject(JsonObject,"rval", 0);
            AppLibNetJsonUtility_AddIntegerObject(JsonObject,"msg_id", MsgId);
        }

        AppLibNetJsonUtility_JsonObjectToJsonString(JsonObject, &JsonString);
        AppLibNetJsonUtility_GetString(JsonString, &ReplyString);
        if (ReplyString) {
            AppLibNetControl_ReplyToLnx(ReplyString, strlen(ReplyString));
        }

        AppLibNetJsonUtility_FreeJsonString(JsonString);
        AppLibNetJsonUtility_FreeJsonObject(JsonObject);
    }else {
        ReturnValue = -1;
        AppLibNetControl_ReplyErrorCode(MsgId, ERROR_NETCTRL_JSON_PACKAGE_ERROR);
    }

    return ReturnValue;
}
#endif /* CONFIG_APP_CONNECTED_AMBA_LINK */

#ifdef CONFIG_ASD_USB_RS232_ENABLE
static int AppHandler_UsbRs232Start(void)
{
    /* start RS232 over USB */
    ApplibUsbCdcAcmMulti_Start();

    /* switch console read/write function */
    AppUserConsole_SetWriteFunc(ApplibUsbCdcAcmMulti_Write);
    AppUserConsole_SetReadFunc(ApplibUsbCdcAcmMulti_Read);

    AmbaShellEnable();

   return 0;
}
#endif


/**
 *  @brief To handle the message of app.
 *
 *  To handle the message.
 *
 *  @return >=0 success, <0 failure
 */
static void AppHandler_Entry(void)
{
    int AppExit = 0;
    UINT32 Param1 = 0, Param2 = 0;
    APP_APP_s *CurApp;
    APP_MESSAGE_s Msg = {0};
    int ReturnValue = 0;

    AppHandler_AppPreInit();

#ifdef CONFIG_APP_CONNECTED_PARTIAL_LOAD
    {
        void *FwldStack;
        extern AMBA_KAL_BYTE_POOL_t G_MMPL;

        /* Allocate needed buffer */
        if (AmbaKAL_BytePoolAllocate(&G_MMPL, &FwldStack, 0x8000, AMBA_KAL_WAIT_FOREVER) != OK) {
        	AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        }

        /* Initial and start to load */
        AmpFwLoader_Init(FwldStack, 0x8000);
        if (AppUtil_GetStartApp(0) == APP_REC_CONNECTED_CAM) {
        	AmpFwLoader_SetEnableSuspend();
        }
    }
#endif
    AppHandler_AppInit();

#ifndef CONFIG_APP_CONNECTED_PARTIAL_LOAD
    AppHandler_AppPostInit();
#else
	if (AppUtil_GetStartApp(0) != APP_REC_CONNECTED_CAM) {
		AppHandler_AppPostInit();
	}
#endif

    while (AppExit == 0) {
        AppLibComSvcHcmgr_RcvMsg(&Msg, AMBA_KAL_WAIT_FOREVER);
        Param1 = Msg.MessageData[0];
        Param2 = Msg.MessageData[1];
#ifdef CONFIG_BUILD_ASD_LIB
		AsdDebugPrintMsg("ASD DEBUG AppHandler_Entry: ", Msg.MessageID, Param1, Param2);
#else
        DBGMSG("[App Handler] Received msg: 0x%X (Param1 = 0x%X / Param2 = 0x%X)", Msg.MessageID, Param1, Param2);
#endif
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
        /* button and t_app operation should be blocked while there is a net control connection. */
        if (app_status.NetCtrlSessionOn) {
            if ((MSG_MDL_ID(Msg.MessageID) == MDL_APP_KEY_ID) ||
                ((MSG_MDL_ID(Msg.MessageID) == MDL_APP_FLOW_ID) && (MSG_APP_FLOW_TYPE(Msg.MessageID) == HMSG_APPFLOW_ID_TEST))) {
                AmbaPrint("[App Handler] button/t_app operation is blocked!");
                continue;
            }
        }
#endif

        /* Message handling. */
        switch (Msg.MessageID) {
        case HMSG_TIMER_CHECK:
        case HMSG_TIMER_1HZ:
        case HMSG_TIMER_2HZ:
        case HMSG_TIMER_4HZ:
        case HMSG_TIMER_10HZ:
        case HMSG_TIMER_20HZ:
        case HMSG_TIMER_5S:
        case HMSG_TIMER_30S:
            AppLibComSvcTimer_Handler(Param1);
            break;
        case HMSG_HDMI_INSERT_SET:
            if (app_status.HdmiPluginFlag == 0) {
                app_status.HdmiPluginFlag = 1;
                AppAppMgt_GetCurApp(&CurApp);
                CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            }
            break;
        case HMSG_HDMI_INSERT_CLR:
            if (app_status.HdmiPluginFlag == 1) {
                app_status.HdmiPluginFlag = 0;
                AppAppMgt_GetCurApp(&CurApp);
                CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            }
            break;
        case HMSG_CS_INSERT_SET:
            if (app_status.CompositePluginFlag == 0) {
                app_status.CompositePluginFlag = 1;
                AppAppMgt_GetCurApp(&CurApp);
                CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            }
            break;
        case HMSG_CS_INSERT_CLR:
            if (app_status.CompositePluginFlag == 1) {
                app_status.CompositePluginFlag = 0;
                AppAppMgt_GetCurApp(&CurApp);
                CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            }
            break;
        case HMSG_USB_DETECT_CONNECT:
            AmbaPrintColor(GREEN,"HMSG_USB_DETECT_CONNECT");
#if 1
            if (!AppAppMgt_CheckIo()) {/**< Block USB connect/remove if it's recording*/
                if (app_status.UsbPluginFlag == 0) {
                    app_status.UsbPluginFlag = 1;
                    app_status.UsbChargeMode = 0;
                    if (UserSetting->SetupPref.USBMode == USB_MODE_MSC) {
                        AppUtil_UsbChargeCheckingSet(1);
                    } else if (UserSetting->SetupPref.USBMode == USB_MODE_AMAGE) {
                        AppAppMgt_GetCurApp(&CurApp);
                        CurApp->OnMessage(AMSG_CMD_USB_APP_START, Param1, Param2);
                    } else {
                    	AmbaPrint("<%s> connect: usb RS232",__FUNCTION__);
						#ifdef CONFIG_ASD_USB_RS232_ENABLE
						AppHandler_UsbRs232Start();
						#endif
                    }
                }
            } else {
                    /**if app is recording, notify app usb insert is suspend*/
                    AppLibComSvcHcmgr_SendMsg(HMSG_USB_DETECT_SUSPEND, 0, 0);
            }
#else
            if (app_status.UsbPluginFlag == 0) {
                app_status.UsbPluginFlag = 1;
                if (AmbaUser_CheckIsUsbSlave()) {
                    AppAppMgt_GetCurApp(&CurApp);
                    CurApp->OnMessage(AMSG_CMD_USB_APP_START, Param1, Param2);
                }
            }
#endif
            break;
        case HMSG_USB_DETECT_CONFIGURE:
            AmbaPrintColor(GREEN,"HMSG_USB_DETECT_CONFIGURE 0x%x",Param1);
            AppUtil_UsbChargeCheckingSetStatus(0);
            Param1 = Param1 & 0xFF;/**The original return value is UINT16*/
            if (Param1 == 0xFF) {
                /**Charging mode*/
                app_status.UsbChargeMode = 1;
            } else {
                AppAppMgt_GetCurApp(&CurApp);
                CurApp->OnMessage(AMSG_CMD_USB_APP_START, 0, 0);
            }
            break;
        case HMSG_USB_DETECT_REMOVE:
            AmbaPrintColor(GREEN,"HMSG_USB_DETECT_REMOVE");
#if 1
            if (!AppAppMgt_CheckIo()) {/**< Block USB connect/remove if it's recording*/
                if (app_status.UsbPluginFlag == 1) {
                    app_status.UsbPluginFlag = 0;
                    app_status.UsbChargeMode = 0;
                    if (UserSetting->SetupPref.USBMode == USB_MODE_MSC) {
                        AppUtil_UsbChargeCheckingSet(0);
                    } else if (UserSetting->SetupPref.USBMode == USB_MODE_AMAGE) {
                        AppAppMgt_GetCurApp(&CurApp);
                        CurApp->OnMessage(AMSG_CMD_USB_APP_STOP, Param1, Param2);
                    } else {
                    	 AmbaPrint("<%s> remove: usb RS232",__FUNCTION__);
                    }
                }
            } else {
                    /**if app is recording, notify app usb is removed*/
                    AppLibComSvcHcmgr_SendMsg(HMSG_USB_DETECT_SUSPEND, 0, 0);
            }

#else
            if (app_status.UsbPluginFlag == 1) {
                app_status.UsbPluginFlag = 0;
                if (AmbaUser_CheckIsUsbSlave()) {
                    AppAppMgt_GetCurApp(&CurApp);
                    CurApp->OnMessage(AMSG_CMD_USB_APP_STOP, Param1, Param2);
                }
            }
#endif
            break;
        case HMSG_NAND0_CARD_INSERT:
        case HMSG_NAND1_CARD_INSERT:
        case HMSG_SD0_CARD_INSERT:
        case HMSG_SD1_CARD_INSERT:
            if (app_status.UsbStorageUnmount == 0) {
                ReturnValue = AppLibCard_GetCardId(Param1);
                if ((AppLibCard_GetActiveSlot() == NULL_SLOT || Param1 > AppLibCard_GetActiveSlot()) && (app_status.CardBusySlot == NO_BUSY_SLOT)) {
                    AppLibComSvcAsyncOp_CardInsert(Param1);
                } else if (AppAppMgt_CheckBusy() || AppAppMgt_CheckIo() || app_status.CardBusySlot != NO_BUSY_SLOT) {
                    /** set card insert blocked flag */
                    AppLibCard_StatusSetBlock(ReturnValue, 1);
                } else {
                    /** Remove card insert blocked flag */
                    AppLibCard_StatusSetBlock(ReturnValue, 0);
                    AppLibComSvcAsyncOp_CardInsert(Param1);
                }
            } else {
                AppAppMgt_GetCurApp(&CurApp);
                CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            }
            break;
        case HMSG_NAND0_CARD_REMOVE:
        case HMSG_NAND1_CARD_REMOVE:
        case HMSG_SD0_CARD_REMOVE:
        case HMSG_SD1_CARD_REMOVE:
            if (app_status.UsbStorageUnmount == 0) {
                int slot = 0;
                ReturnValue = AppLibCard_Remove(Param1);
                if (app_status.CardBusySlot == Param1) {
                    AmbaPrint("[App Handler] Remove set-rooting card, reset app_status.busy_slot !");
                    app_status.CardBusySlot = NO_BUSY_SLOT;
                    /** Check card insert block */
                    ReturnValue = AppLibCard_StatusCheckHighProrityBlock();
                    if (ReturnValue >= 0 && (ReturnValue < AppLibCard_GetActiveCardId())) {
                        /** send the blocked card insert if the card is higher prority than current ative card */
                        AppLibCard_SndCardInsertMsg(ReturnValue);
                    }
                } else if (ReturnValue == CARD_REMOVE_ACTIVE) {
                    slot = AppLibCard_GetPrimarySlot();
                    if (AppAppMgt_CheckBusy() || AppAppMgt_CheckIo() || app_status.CardBusySlot != NO_BUSY_SLOT) {
                        if (slot != -1) {
                            /** set card insert blocked flag */
                            ReturnValue = AppLibCard_GetCardId(slot);
                            AppLibCard_StatusSetBlock(ReturnValue, 1);
                        }
                        Msg.MessageID = AMSG_ERROR_CARD_REMOVED;
                    } else {
                        if (slot != -1) {
                            /** remove card insert blocked flag */
                            ReturnValue = AppLibCard_GetCardId(slot);
                            AppLibCard_StatusSetBlock(ReturnValue, 0);
                            AppLibComSvcAsyncOp_CardInsert(slot);
                        }
                        Msg.MessageID = AMSG_STATE_CARD_REMOVED;
                    }
                    AppAppMgt_GetCurApp(&CurApp);
                    CurApp->OnMessage(Msg.MessageID, Param1, Param2);
                }
            } else {
                AppAppMgt_GetCurApp(&CurApp);
                CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            }
            break;
        case HMSG_STORAGE_BUSY:
            {
                int slot = AppLibCard_GetSlotFromChar(Param1);
                app_status.CardBusySlot = slot;
                AppLibCard_StatusSetRefreshing(app_status.CardBusySlot, 1);
                AppAppMgt_GetCurApp(&CurApp);
                CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            }
            break;
        case HMSG_STORAGE_IDLE:
            if (Param2 == 1) {
                /** Resend from current app */
                AppAppMgt_GetCurApp(&CurApp);
                CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            } else if (AppLibCard_CheckInsertingCard() == 0) {
                int slot = AppLibCard_GetSlotFromChar(Param1);
                if (app_status.CardBusySlot == slot) {
                    app_status.CardBusySlot = NO_BUSY_SLOT;
                    AppLibCard_SetInsertingSlot(0xFF);
                    AppLibCard_StatusSetRefreshing(slot, 0);

                    /**Check card insert block */
                    ReturnValue = AppLibCard_StatusCheckHighProrityBlock();
                    if (ReturnValue >= 0 && (ReturnValue < AppLibCard_GetActiveCardId())) {
                    /** send the blocked card insert if the card is higher prority than current ative card */
                        AppLibCard_SndCardInsertMsg(ReturnValue);
                    } else if (slot == AppLibCard_GetPrimarySlot()) {
                        /** send the storage idle when there are no higher prority blocked slot */

                        /** Send the storage idle to the current app */
                        AppAppMgt_GetCurApp(&CurApp);
                        CurApp->OnMessage(Msg.MessageID, Param1, Param2);
                    }
                } else {
                    AmbaPrint("[App Handler] The storage idle slot %d is not the same as busy slot %d!Something wrong!", slot, app_status.CardBusySlot);
                    app_status.CardBusySlot = NO_BUSY_SLOT;
                }
            } else {
                AppLibComSvcHcmgr_SendMsgNoWait(HMSG_STORAGE_IDLE, Param1, 0);
            }
            break;
        case HMSG_STOP_APP_COMPLETE:
            AmbaPrintColor(BLUE, "[App Handler] HMSG_STOP_APP_COMPLETE");
            AppLibComSvcHcmgr_DetachHandler();
            break;
        case HMSG_EXIT_HDL:
            AmbaPrint("[App Handler] Exit handler application!");
            AppExit = 1;
            break;
        case HMSG_USER_POWER_BUTTON:
            // TODO: Power saving mode
            AmbaPrint("[App Handler] Power off");
            AmbaPWC_Init();
            AmbaPWC_ForcePowerDownSequence();
            break;
        case DISP_CMD_LCD_IRQ:
            {
                int LcdDelayTime = 0;
                LcdDelayTime = AppLibSysLcd_GetLcdDelayTime(LCD_CH_DCHAN);
                AmbaKAL_TaskSleep(LcdDelayTime);
                AppLibSysLcd_SetBrightness(LCD_CH_DCHAN, 0, 0);
                AppLibSysLcd_SetBacklight(LCD_CH_DCHAN, 1);
                break;
            }
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
        case AMSG_EVENT_BOSS_BOOTED:
            {
                extern int AmbaBoss_ChangePriority(int NewPriority);
                extern void AmbaBoss_AdjustSchedulePeriod(UINT32 Period);

                AmbaPrintColor(MAGENTA, "===> [App Handler] AMSG_EVENT_BOSS_BOOTED");
                #ifdef CONFIG_BSP_TAROKO
                // TODO: use switch control wifi on/off
                AmpNetUtility_luExecNoResponse("/usr/local/share/script/wifi_start.sh fast");
                #endif
                /* Linux booted. */
                AppHandler_StartNetService();
                AmbaBoss_ChangePriority(APP_AMBA_BOSS_PRIORITY);
                AmbaBoss_AdjustSchedulePeriod(10);

                AppAppMgt_GetCurApp(&CurApp);
                CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            }
            break;
        case AMSG_NETCTRL_SESSION_START:
            AppHandler_NetControlSession(1, Param1);
            AppAppMgt_GetCurApp(&CurApp);
            CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            break;
        case AMSG_NETCTRL_SESSION_STOP:
            AppHandler_NetControlSession(0, 0);
            AppAppMgt_GetCurApp(&CurApp);
            CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            break;
#endif //CONFIG_APP_CONNECTED_AMBA_LINK
        default:
            AppAppMgt_GetCurApp(&CurApp);
            CurApp->OnMessage(Msg.MessageID, Param1, Param2);
            break;
        }
    }
}

/**
 *  @brief Handler Exit Implementation
 *
 *  Handler Exit Implementation
 *
 *  @return >=0 success, <0 failure
 */
static int AppHandler_Exit(void)
{
    return AppLibComSvcHcmgr_SendMsg(HMSG_EXIT_HDL, 0, 0);
}

/**
 * @brief Car Cam Handler Object
 */
APPLIB_HCMGR_HANDLER_s Handler = {
    AppHandler_Entry,
    AppHandler_Exit
};

