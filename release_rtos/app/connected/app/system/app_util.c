/**
 * @file app/connected/app/system/app_util.c
 *
 * Implemention of Demo application utility
 *
 * History:
 *    2013/08/16 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <apps/apps.h>
#include "app_util.h"
#include "app_pref.h"
#include "status.h"
#include <applib.h>
#include <wchar.h>
#include <AmbaRTC.h>
#include <usb/AmbaUSB_API.h>
#include <calibration/ApplibCalibMgr.h>
#include <AmbaGPIO.h>

static int usb_charge_check_running = 0;

/**
 *  @brief Get the status of USB charger mode.
 *
 *  Get the status of USB charger mode.
 *
 *  @return Status of USB charger mode.
 */
int AppUtil_UsbChargeCheckingGetStatus(void)
{
    return usb_charge_check_running;
}

void AppUtil_UsbChargeCheckingSetStatus(int status)
{
    usb_charge_check_running = status;
}

/**
 *  @brief To excute the functions that system block them when the Ready flag is not enabled.
 *
 *  To excute the functions that system block them when the Ready flag is not enabled.
 *
 *  @param [in] param Parameter
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_ReadyCheck(UINT32 param)
{
    int ReturnValue = 0;

    /* Check unsaved movie status */
    if (app_status.MvRecover == 1) {
        AppUtil_SwitchApp(APP_MISC_MVRECOVER);
        ReturnValue = 0;
        return ReturnValue;
    }

    /* Check firmware existence */
    if (app_status.FwUpdate == 1) {
        AppUtil_SwitchApp(APP_MISC_FWUPDATE);
        ReturnValue = 0;
        return ReturnValue;
    }

    /* Check Calibration Script */
    if (app_status.CalibRework == 1) {
        AppUtil_SwitchApp(APP_MISC_CALIBUPDATE);
        ReturnValue = 0;
        return ReturnValue;
    }

    /* Check card format parameter */
    if ((app_status.CardFmtParam == APP_CARD_FMT_NONOPTIMUM)) {
        AppUtil_SwitchApp(APP_MISC_FORMATCARD);
        ReturnValue = 0;
        return ReturnValue;
    }

    /* Check USB cable */
    if ((app_status.UsbPluginFlag == 1) &&
        (AppUtil_UsbChargeCheckingGetStatus() == 0) &&
        (app_status.UsbChargeMode == 0) &&
        (AmbaUser_CheckIsUsbSlave())) {
        APP_APP_s *CurApp;
        AppAppMgt_GetCurApp(&CurApp);
        CurApp->OnMessage(AMSG_CMD_USB_APP_START, 0, 0);
    }

    /* Enable the fchan device after finishing the mode switch */
    if (app_status.LockDecMode == 1) {
        ReturnValue =  AppLibDisp_GetDeviceID(DISP_CH_FCHAN);
        if (ReturnValue != AMP_DSIP_NONE) {
            AppLibDisp_SelectDevice(DISP_CH_FCHAN, DISP_ANY_DEV);
            AppLibDisp_ConfigMode(DISP_CH_FCHAN, AppUtil_GetVoutMode(0));
            AppLibDisp_SetupChan(DISP_CH_FCHAN);
            AppLibDisp_ChanStart(DISP_CH_FCHAN);
        }
    }

    /**Check vout */
    if ((app_status.HdmiPluginFlag == 1) &&
        (app_status.HdmiPluginFlag != AppLibSysVout_CheckJackHDMI())) {
        APP_APP_s *CurApp;
        AppAppMgt_GetCurApp(&CurApp);
        CurApp->OnMessage(HMSG_HDMI_INSERT_SET, 0, 0);
    } else if ((app_status.HdmiPluginFlag == 0) &&
        (app_status.HdmiPluginFlag != AppLibSysVout_CheckJackHDMI())) {
        APP_APP_s *CurApp;
        AppAppMgt_GetCurApp(&CurApp);
        CurApp->OnMessage(HMSG_HDMI_INSERT_CLR, 0, 0);
    }
    if ((app_status.CompositePluginFlag == 1) &&
        (app_status.CompositePluginFlag != AppLibSysVout_CheckJackCs())) {
        APP_APP_s *CurApp;
        AppAppMgt_GetCurApp(&CurApp);
        CurApp->OnMessage(HMSG_CS_INSERT_SET, 0, 0);
    } else if ((app_status.CompositePluginFlag == 0) &&
        (app_status.CompositePluginFlag != AppLibSysVout_CheckJackCs())) {
        APP_APP_s *CurApp;
        AppAppMgt_GetCurApp(&CurApp);
        CurApp->OnMessage(HMSG_CS_INSERT_CLR, 0, 0);
    }
#if 0
    /**Check audio input */
    if ((app_status.LineinPluginFlag == 1) &&
         (app_status.LineinPluginFlag != app_audio_check_jack_linein())) {
        appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_LINEIN_IN_SET, 0, 0);
    } else if ((app_status.LineinPluginFlag == 0) &&
         (app_status.LineinPluginFlag != app_audio_check_jack_linein())) {
        appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_LINEIN_IN_CLR, 0, 0);
    }

    /**Check audio output */
    if ((app_status.LineoutPluginFlag == 1) &&
         (app_status.LineoutPluginFlag != app_audio_check_jack_lineout())) {
             appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_LINEOUT_IN_SET, 0, 0);
    } else if ((app_status.LineoutPluginFlag == 0) &&
         (app_status.LineoutPluginFlag != app_audio_check_jack_lineout())) {
             appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_LINEOUT_IN_CLR, 0, 0);
    }
    if ((app_status.hp_plugin_flag == 1) &&
         (app_status.hp_plugin_flag != app_audio_check_jack_hp())) {
        appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_HP_IN_SET, 0, 0);
    } else if ((app_status.hp_plugin_flag == 0) &&
         (app_status.hp_plugin_flag != app_audio_check_jack_hp())) {
        appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_HP_IN_CLR, 0, 0);
    }
#endif
    /**Check app switch*/
    if (app_status.AppSwitchBlocked > 0) {
        AppUtil_SwitchApp(app_status.AppSwitchBlocked);
        app_status.AppSwitchBlocked = 0;
        ReturnValue = 0;
        return ReturnValue;
    }

    /**Check card insert block */
    {
        int i = 0;
        for (i = 0; i< CARD_NUM; i++) {
            if (AppLibCard_StatusCheckBlock(i) == 0) {
                if ((i < AppLibCard_GetActiveCardId()) && (AppLibCard_GetActiveCardId()!= -1)) {
                    APP_APP_s *CurApp;
                    AppAppMgt_GetCurApp(&CurApp);
                    CurApp->OnMessage(AMSG_CMD_CARD_UPDATE_ACTIVE_CARD, i, 0);
                } else {
                    AppLibCard_StatusSetBlock(i, 0);
                    AppLibComSvcAsyncOp_CardInsert(AppLibCard_GetSlot(i));
                }
                ReturnValue = 0;
                return ReturnValue;
            }
        }
    }

    return ReturnValue;
}

/**
 *  @brief To excute the functions that system block them when the Busy flag is enabled.
 *
 *  To excute the functions that system block them when the Busy flag is enabled.
 *
 *  @param [in] param Parameter
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_BusyCheck(UINT32 param)
{
    int ReturnValue = 0;

    /* Check unsaved movie status */
    if (app_status.MvRecover == 1) {
        AppUtil_SwitchApp(APP_MISC_MVRECOVER);
        ReturnValue = 0;
        return ReturnValue;
    }

    /* Check firmware existence */
    if (app_status.FwUpdate == 1) {
        AppUtil_SwitchApp(APP_MISC_FWUPDATE);
        ReturnValue = 0;
        return ReturnValue;
    }

    /* Check Calibration Script */
    if (app_status.CalibRework == 1) {
        AppUtil_SwitchApp(APP_MISC_CALIBUPDATE);
        ReturnValue = 0;
        return ReturnValue;
    }

    /* Check card format parameter */
    if ((app_status.CardFmtParam == APP_CARD_FMT_NONOPTIMUM)) {
        AppUtil_SwitchApp(APP_MISC_FORMATCARD);
        ReturnValue = 0;
        return ReturnValue;
    }

    /* Check USB cable */
    if ((app_status.UsbPluginFlag == 1) &&
        (AppUtil_UsbChargeCheckingGetStatus() == 0) &&
        (app_status.UsbChargeMode == 0) &&
        (AmbaUser_CheckIsUsbSlave())) {
        APP_APP_s *CurApp;
        AppAppMgt_GetCurApp(&CurApp);
        CurApp->OnMessage(AMSG_CMD_USB_APP_START, 0, 0);
    }

    /* Enable the fchan device after finishing the mode switch */
    if (app_status.LockDecMode == 1) {
        ReturnValue =  AppLibDisp_GetDeviceID(DISP_CH_FCHAN);
        if (ReturnValue != AMP_DSIP_NONE) {
            AppLibDisp_SelectDevice(DISP_CH_FCHAN, DISP_ANY_DEV);
            AppLibDisp_ConfigMode(DISP_CH_FCHAN, AppUtil_GetVoutMode(0));
            AppLibDisp_SetupChan(DISP_CH_FCHAN);
            AppLibDisp_ChanStart(DISP_CH_FCHAN);
        }
    }

    /**Check vout */
    if ((app_status.HdmiPluginFlag == 1) &&
        (app_status.HdmiPluginFlag != AppLibSysVout_CheckJackHDMI())) {
        APP_APP_s *CurApp;
        AppAppMgt_GetCurApp(&CurApp);
        CurApp->OnMessage(HMSG_HDMI_INSERT_SET, 0, 0);
    } else if ((app_status.HdmiPluginFlag == 0) &&
        (app_status.HdmiPluginFlag != AppLibSysVout_CheckJackHDMI())) {
        APP_APP_s *CurApp;
        AppAppMgt_GetCurApp(&CurApp);
        CurApp->OnMessage(HMSG_HDMI_INSERT_CLR, 0, 0);
    }
    if ((app_status.CompositePluginFlag == 1) &&
        (app_status.CompositePluginFlag != AppLibSysVout_CheckJackCs())) {
        APP_APP_s *CurApp;
        AppAppMgt_GetCurApp(&CurApp);
        CurApp->OnMessage(HMSG_CS_INSERT_SET, 0, 0);
    } else if ((app_status.CompositePluginFlag == 0) &&
        (app_status.CompositePluginFlag != AppLibSysVout_CheckJackCs())) {
        APP_APP_s *CurApp;
        AppAppMgt_GetCurApp(&CurApp);
        CurApp->OnMessage(HMSG_CS_INSERT_CLR, 0, 0);
    }
#if 0
    /**Check audio input */
    if ((app_status.LineinPluginFlag == 1) &&
         (app_status.LineinPluginFlag != app_audio_check_jack_linein())) {
        appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_LINEIN_IN_SET, 0, 0);
    } else if ((app_status.LineinPluginFlag == 0) &&
         (app_status.LineinPluginFlag != app_audio_check_jack_linein())) {
        appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_LINEIN_IN_CLR, 0, 0);
    }

    /**Check audio output */
    if ((app_status.LineoutPluginFlag == 1) &&
         (app_status.LineoutPluginFlag != app_audio_check_jack_lineout())) {
             appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_LINEOUT_IN_SET, 0, 0);
    } else if ((app_status.LineoutPluginFlag == 0) &&
         (app_status.LineoutPluginFlag != app_audio_check_jack_lineout())) {
             appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_LINEOUT_IN_CLR, 0, 0);
    }
    if ((app_status.hp_plugin_flag == 1) &&
         (app_status.hp_plugin_flag != app_audio_check_jack_hp())) {
        appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_HP_IN_SET, 0, 0);
    } else if ((app_status.hp_plugin_flag == 0) &&
         (app_status.hp_plugin_flag != app_audio_check_jack_hp())) {
        appmgt_get_curapp(&CurApp);
        CurApp->OnMessage(HMSG_HP_IN_CLR, 0, 0);
    }
#endif
    /**Check app switch*/
    if (app_status.AppSwitchBlocked > 0) {
        AppUtil_SwitchApp(app_status.AppSwitchBlocked);
        app_status.AppSwitchBlocked = 0;
        ReturnValue = 0;
        return ReturnValue;
    }

    /**Check card insert block */
    {
        int i = 0;
        for (i = 0; i< CARD_NUM; i++) {
            if (AppLibCard_StatusCheckBlock(i) == 0) {
                if ((i < AppLibCard_GetActiveCardId()) && (AppLibCard_GetActiveCardId()!= -1)) {
                    APP_APP_s *CurApp;
                    AppAppMgt_GetCurApp(&CurApp);
                    CurApp->OnMessage(AMSG_CMD_CARD_UPDATE_ACTIVE_CARD, i, 0);
                } else {
                    AppLibCard_StatusSetBlock(i, 0);
                    AppLibComSvcAsyncOp_CardInsert(AppLibCard_GetSlot(i));
                }
                ReturnValue = 0;
                return ReturnValue;
            }
        }
    }

    return ReturnValue;
}


/**
 *  @brief Switch the applicaton.
 *
 *  Switch the applicaton.
 *
 *  @param [in] appId Application Index
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_SwitchApp(int appId)
{
    int ReturnValue = 0;
    APP_APP_s *CurApp;
    APP_APP_s *NewApp;

    if (appId < 0) {
        AmbaPrintColor(RED, "[App Util] <SwitchApp> This app is not registered");
        return -1;
    }

    AppAppMgt_GetCurApp(&CurApp);
    AppAppMgt_GetApp(appId, &NewApp);

    if (AppAppMgt_CheckIdle() || // app is idle
        (CurApp->Tier < NewApp->Tier) || // new app is descendant
        APP_CHECKFLAGS(NewApp->GFlags, APP_AFLAGS_READY)) { // new app is ancestor
        AppAppMgt_SwitchApp(appId);
        if (CurApp->Tier >= NewApp->Tier) {
            AppPref_Save();
        }
    } else {
        AmbaPrint("[App Util] <SwitchApp> App switch target appId = %d is blocked", appId);
        app_status.AppSwitchBlocked = appId;
        ReturnValue = AppAppMgt_CheckBusy();
        if (ReturnValue) {
            AmbaPrint("[App Util] <SwitchApp> App switch is blocked by busy appId = %d", ReturnValue);
            CurApp->OnMessage(AMSG_CMD_SWITCH_APP, appId, 0);
        }
    }

    return ReturnValue;
}


/**
 *  @brief Switch application
 *
 *  Switch application
 *
 *  @param [in] param
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_SwitchMode(UINT32 param)
{
    int ReturnValue = 0;

    if (param != 0 ) {
        AmbaPrintColor(BLUE, "[App Util] <SwitchMode> Mode switch to %d ",param);
        AppUtil_SwitchApp(param);
    } else {
        if (app_status.Type == APP_TYPE_DV) {
            if (UserSetting->SystemPref.SystemMode == APP_MODE_DEC) {
                AppUtil_SwitchApp(APP_REC_CONNECTED_CAM);
            } else {
                AppUtil_SwitchApp(APP_THUMB_MOTION);
            }
        }
    }

    return ReturnValue;
}

/**
 *  @brief Get the first application.
 *
 *  Get the first application.
 *
 *  @param [in] param
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_GetStartApp(UINT32 param)
{
    int ReturnValue = 0;

    if (app_status.Type == APP_TYPE_DV) {
        if (UserSetting->SystemPref.SystemMode == APP_MODE_DEC) {
            ReturnValue = APP_THUMB_MOTION;
        } else {
            ReturnValue = APP_REC_CONNECTED_CAM;
        }
    }
#if defined (ENABLE_HDMI_TEST)
    UserSetting->SystemPref.SystemMode = APP_MODE_DEC;
    ReturnValue = APP_THUMB_MOTION;
#endif

    return ReturnValue;
}

char FirmwareName[64] = {'A','m','b','a','S','y','s','F','W','.','b','i','n','\0'};

/**
 *  @brief Check that the SD card have the firmware updated file.
 *
 *  Check that the SD card have the firmware updated file.
 *
 *  @param [in] drive Storage drive id
 *
 *  @return >=0 success, <0 failure
 */
static int AppUtil_CheckCardFw(char drive)
{
    char Firmware[64] = {0};
    AMBA_FS_STAT Fstat;
    int ReturnValue = -1;

    Firmware[0] = (char)drive;
    Firmware[1] = ':';
    Firmware[2] = '\\';
    strcat(Firmware, FirmwareName);
    ReturnValue = AmbaFS_Stat((const char *)Firmware, &Fstat);
    if ((ReturnValue == 0) && (Fstat.Size > 0)) {
        app_status.FwUpdate = 1;
        if (AppAppMgt_CheckIdle()) {
            if (app_status.MvRecover == 0) {
                ReturnValue = AppUtil_SwitchApp(APP_MISC_FWUPDATE);
            }
        }
    } else {
        /* when connect USB, insert SD with firmware.bin and remove firmware.bin in PC */
        app_status.FwUpdate = 0;
        ReturnValue = -1;
    }

    return ReturnValue;
}

char CalibScript[64] = CALIB_SCRIPT;
/**
 *  @brief Check that the SD card have the calibration script file.
 *
 *  Check that the SD card have the calibration script file.
 *
 *  @param [in] drive Storage drive id
 *
 *  @return >=0 success, <0 failure
 */
static int AppUtil_CheckCardScript(char drive)
{
    char Script[64] = {0};
    AMBA_FS_STAT Fstat;
    int ReturnValue = -1;

    Script[0] = (char)drive;
    Script[1] = ':';
    Script[2] = '\\';
    strcat(Script, CalibScript);

    //strcat(Script, CalibScript);
   // w_asc2uni(WScript, Script, strlen(WScript));
    ReturnValue = AmbaFS_Stat((const char *)Script, &Fstat);
    if ((ReturnValue == 0) && (Fstat.Size > 0)) {
        app_status.CalibRework = 1;
        if (AppAppMgt_CheckIdle()) {
            if (app_status.MvRecover == 0 && app_status.FwUpdate == 0) {
                ReturnValue = AppUtil_SwitchApp(APP_MISC_CALIBUPDATE);
            }
        }
    } else {
        /* when connect USB, insert SD with firmware.bin and remove firmware.bin in PC */
        app_status.CalibRework = 0;
        ReturnValue = -1;
    }

    return ReturnValue;
}

/**
 *  @brief Check card parameter
 *
 *  Check card parameter
 *
 *  @param [in] param Parameter
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_CheckCardParam(UINT32 param)
{
    int ReturnValue = -1;
    int CardId = 0;
    int Slot = 0;
    char Drive = 'A';
    APP_APP_s *CurApp;

    /* To check the movie recovery data. */
    if (UserSetting->VideoPref.UnsavingData != 0) {
        UserSetting->VideoPref.UnsavingData = 0;
        app_status.MvRecover = 1;
        if (AppAppMgt_CheckIdle()) {
            ReturnValue = AppUtil_SwitchApp(APP_MISC_MVRECOVER);
        } else {
            /*If status busy send msg to app to stop VF to do movie recover*/
            AppAppMgt_GetCurApp(&CurApp);
            CurApp->OnMessage(AMSG_CMD_CARD_MOVIE_RECOVER, 0, 0);
        }
    }

    CardId = AppLibCard_GetActiveCardId();
    Slot = AppLibCard_GetActiveSlot();
    Drive = AppLibCard_GetActiveDrive();
    /* to check firmware update file & Calib script*/
    if ((CardId == CARD_SD0) || (CardId == CARD_SD1)) {
        AppUtil_CheckCardFw(Drive);
        AppUtil_CheckCardScript(Drive);
    }

    if ((Slot == SCM_SLOT_SD0) || (Slot == SCM_SLOT_SD1)) {
        /* do format check only on SD Card */
        ReturnValue = AppLibCard_CheckFormatParam(Slot, Drive);
        if (ReturnValue == -1) {
            app_status.CardFmtParam = APP_CARD_FMT_NONOPTIMUM;
            if (AppAppMgt_CheckIdle()) {
                if ((app_status.MvRecover == 0) && (app_status.FwUpdate == 0)) {
                    ReturnValue = AppLibCard_CheckStatus(CARD_CHECK_DELETE);
                    if (ReturnValue == CARD_STATUS_NO_CARD) {
                        AmbaPrintColor(RED,"[App Util] <AppUtil_CheckCardParam> WARNING_NO_CARD");
                    } else if (ReturnValue == CARD_STATUS_WP_CARD) {
                        AmbaPrintColor(RED,"[App Util] <AppUtil_CheckCardParam> WARNING_CARD_PROTECTED");
                    } else {
                        ReturnValue = AppUtil_SwitchApp(APP_MISC_FORMATCARD);
                    }
                }
            }
             AppAppMgt_GetCurApp(&CurApp);
             CurApp->OnMessage(AMSG_CMD_CARD_FMT_NONOPTIMUM, 0, 0);
        }
    }

    return ReturnValue;
}


/**
 *  @brief Get the vout mode.
 *
 *  Get the vout mode.
 *
 *  @param [in] param Parameter
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_GetVoutMode(int param)
{
    int ReturnValue = 0;
    if (app_status.FixedVoutMode == VOUT_DISP_MODE_AUTO) {
        AmbaPrint("[App Util] <SwitchApp> Auto mode is not implemented. Use default display mode");
        ReturnValue = AppLibSysVout_GetVoutMode(VOUT_DISP_MODE_1080P);
    } else {
        ReturnValue = AppLibSysVout_GetVoutMode((VOUT_DISP_MODE_ID_e) app_status.FixedVoutMode);
    }
    return ReturnValue;
}


/**
 *  @brief Initialize the app utility
 *
 *  Initialize the app utility
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_Init(void)
{
    return 0;
}


/**
 *  @brief Initialize the status
 *
 *  Initialize the status
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_StatusInit(void)
{
    int ReturnValue = 0;

    /** Correct RTC time */
    {
        AMBA_RTC_TIME_SPEC_u TimeSpec = {0};
        AmbaRTC_GetSystemTime(AMBA_TIME_STD_TAI, &TimeSpec);
        if (ReturnValue < 0) {
            AmbaPrintColor(RED, "[App Util] AmbaRTC_GetSystemTime failure. ReturnValue = %d", ReturnValue);
        }
#if 0
        AmbaPrint("TimeSpec.Calendar.Year = %d",TimeSpec.Calendar.Year);
        AmbaPrint("TimeSpec.Calendar.Month = %d",TimeSpec.Calendar.Month);
        AmbaPrint("TimeSpec.Calendar.DayOfMonth = %d",TimeSpec.Calendar.DayOfMonth);
        AmbaPrint("TimeSpec.Calendar.DayOfWeek = %d",TimeSpec.Calendar.DayOfWeek);
        AmbaPrint("TimeSpec.Calendar.Hour = %d",TimeSpec.Calendar.Hour);
        AmbaPrint("TimeSpec.Calendar.Minute = %d",TimeSpec.Calendar.Minute);
        AmbaPrint("TimeSpec.Calendar.Second = %d",TimeSpec.Calendar.Second);
        AmbaPrint("AmbaRTC_IsDevValid() = %d",AmbaRTC_IsDevValid());
#endif
        if ((TimeSpec.Calendar.Year > 2037) || (TimeSpec.Calendar.Year < 2014)) {
            TimeSpec.Calendar.Year = 2014;
            TimeSpec.Calendar.Month = 1;
            TimeSpec.Calendar.Day = 1;
            TimeSpec.Calendar.DayOfWeek = WEEK_WEDNESDAY;
            TimeSpec.Calendar.Hour = 0;
            TimeSpec.Calendar.Minute = 0;
            TimeSpec.Calendar.Second = 0;
            ReturnValue =  AmbaRTC_SetSystemTime(AMBA_TIME_STD_TAI, &TimeSpec);
            if (ReturnValue < 0) {
                AmbaPrintColor(RED, "[App Util] AmbaRTC_SetSystemTime failure. ReturnValue = %d", ReturnValue);
            }
        }
    }

    /** Init status */
    AppLibSysVin_SetSystemType(UserSetting->SetupPref.VinSystem);
    AppLibSysVout_SetSystemType(UserSetting->SetupPref.VoutSystem);


    /** Init video settings */
    UserSetting->VideoPref.SensorVideoRes = AppLibSysSensor_CheckVideoRes(UserSetting->VideoPref.SensorVideoRes);
    AppLibVideoEnc_SetSensorVideoRes(UserSetting->VideoPref.SensorVideoRes);
    AppLibVideoEnc_SetQuality(UserSetting->VideoPref.VideoQuality);
    AppLibVideoEnc_SetPreRecord(UserSetting->VideoPref.PreRecord);
    AppLibVideoEnc_SetTimeLapse(UserSetting->VideoPref.TimeLapse);
    AppLibVideoEnc_SetDualStreams(UserSetting->VideoPref.DualStreams);
    AppLibVideoEnc_SetSplit(VIDEO_SPLIT_SIZE_AUTO | VIDEO_SPLIT_TIME_AUTO);
    AppLibVideoEnc_SetRecMode(REC_MODE_AV);

    /** Init photo settings */
    AppLibStillEnc_SetMultiCapMode(UserSetting->PhotoPref.PhotoMultiCap);
    AppLibStillEnc_SetNormCapMode(UserSetting->PhotoPref.PhotoCapMode);
    AppLibStillEnc_SetSizeID(UserSetting->PhotoPref.PhotoSize);
    AppLibStillEnc_SetQualityMode(UserSetting->PhotoPref.PhotoQuality);
    AppLibStillEnc_SetQuickview(PHOTO_QUICKVIEW_DUAL);
    AppLibStillEnc_SetQuickviewDelay(UserSetting->PhotoPref.QuickviewDelay);

    /** Init sound settings */
    AppLibAudioEnc_SetEncType(AUDIO_TYPE_AAC);

    return ReturnValue;
}


static int storage_slot_array[] = {
    SCM_SLOT_SD0,/**< CARD_SD0 */
    SCM_SLOT_SD1,/**< CARD_SD1 */
    SCM_SLOT_FL0,/**< CARD_NAND0 */
    SCM_SLOT_FL1,/**< CARD_NAND1 */
    SCM_SLOT_RD,/**< CARD_RD*/
    -1
};


/**
 *  @brief Polling all card slots
 *
 *  Polling all card slots
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_PollingAllSlots(void)
{
    int ReturnValue = 0;
    int i = 0;

    while (storage_slot_array[i] >= 0) {
        ReturnValue = AppLibCard_Polling(AppLibCard_GetCardId(storage_slot_array[i++]));
    }
    return ReturnValue;
}


/**
 *  @brief Convert the strings from ASCII to Unicode.
 *
 *  Convert the strings from ASCII to Unicode.
 *
 *  @param [in] ascStr strings of ASCII format
 *  @param [out] uniStr strings of Unicode format
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_AsciiToUnicode(char *ascStr, UINT16 *uniStr)
{
    int i, len;

    len = strlen(ascStr);
    for (i=0; i<len; i++) {
        uniStr[i] = ascStr[i];
    }
    uniStr[len] = 0;

    return len;
}

/**
 *  @brief To check the USB charger
 *
 *  To check the USB charger
 *
 *  @param [in] enable Enable flag
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_UsbChargeCheckingSet(UINT32 enable)
{
    if (enable) {
        if (AmbaUSB_System_ChargeDetection()) { // USB charge
            app_status.UsbChargeMode = 1;
        } else {
            ApplibUsbMsc_Start();
            AppUtil_UsbChargeCheckingSetStatus(1);
        }
    } else if (!enable ) {
        APP_APP_s *CurApp;

        ApplibUsbMsc_Stop();
        AppAppMgt_GetCurApp(&CurApp);
        CurApp->OnMessage(AMSG_CMD_USB_APP_STOP, 0, 0);
        AppUtil_UsbChargeCheckingSetStatus(0);
    }

    return 0;
}

/**
 *  @brief The jack handler of composite
 *
 *  The jack handler of composite
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_JackCompositeHandler(AMBA_GPIO_PIN_ID_e gpioPinID)
{
    int ReturnValue = 0;

#if defined (GPIO_COMPOSITE)
    if (gpioPinID == GPIO_COMPOSITE) {
        AMBA_GPIO_PIN_INFO_s pinInfo;
        AMBA_GPIO_PIN_LEVEL_e level;

        AmbaGPIO_GetPinInfo(gpioPinID, &pinInfo);
        level = pinInfo.Level;

        if (!level) {
            AppLibComSvcHcmgr_SendMsgNoWait(HMSG_CS_INSERT_CLR, 0, 0);
        } else {
            AppLibComSvcHcmgr_SendMsgNoWait(HMSG_CS_INSERT_SET, 0, 0);
        }
    }
#endif

    return ReturnValue;
}


/**
 *  @brief Initialize the jack monitor
 *
 *  Initialize the jack monitor
 *
 *  @return >=0 success, <0 failure
 */
int AppUtil_JackMonitor(void)
{
    int ReturnValue = 0;
    #ifndef CONFIG_BSP_TAROKO
    AMBA_GPIO_PIN_INFO_s pinInfo;
    #endif

#if defined (GPIO_COMPOSITE)
    AmbaGPIO_ConfigInput(GPIO_COMPOSITE);
    AmbaGPIO_IsrHook(GPIO_COMPOSITE, GPIO_INT_BOTH_EDGE_TRIGGER,  (AMBA_GPIO_ISR_f)AppUtil_JackCompositeHandler);
    AmbaGPIO_IntEnable(GPIO_COMPOSITE);

    AmbaGPIO_GetPinInfo(GPIO_COMPOSITE, &pinInfo);

    if (pinInfo.Level) {
        AppLibComSvcHcmgr_SendMsg(HMSG_CS_INSERT_SET, 0, 0);
    }
#elif defined (FORCE_COMPOSITE)
	AppLibComSvcHcmgr_SendMsg(HMSG_CS_INSERT_SET, 0, 0);
#endif



    return ReturnValue;
}


