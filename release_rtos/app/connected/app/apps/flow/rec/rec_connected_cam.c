/**
  * @file src/app/apps/flow/rec/rec_connected_cam.c
  *
  * Implementation of Sport Recorder (sensor) application
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

/*************************************************************************
 * Declarations (static)
 ************************************************************************/
/* App structure interfaces APIs */
static int app_rec_connected_cam_start(void);
static int app_rec_connected_cam_stop(void);
static int app_rec_connected_cam_on_message(UINT32 msg, UINT32 param1, UINT32 param2);

APP_APP_s app_rec_connected_cam = {
    0,  //Id
    1,  //Tier
    0,  //Parent
    0,  //Previous
    0,  //Child
    0,  //GFlags
    0,  //Flags
    app_rec_connected_cam_start,//start()
    app_rec_connected_cam_stop,    //stop()
    app_rec_connected_cam_on_message  //OnMessage()
};

/* App status */
REC_CONNECTED_CAM_s rec_connected_cam = {0};

/*************************************************************************
 * Definitions (static)
 ************************************************************************/
/* App structure interface APIs */

/**
 *  @brief The application's function that handle the message.
 *
 *  The application's function that handle the message.
 *
 *  @param [in] msg Message ID
 *  @param [in] param1 first parameter
 *  @param [in] param2 second parameter
 *
 *  @return >=0 success, <0 failure
 */
static int app_rec_connected_cam_on_message(UINT32 msg, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;
    DBGMSG("[app_rec_connected_cam] Received msg: 0x%X (param1 = 0x%X / param2 = 0x%X)", msg, param1, param2);

    ReturnValue = AppWidget_OnMessage(msg, param1, param2);
    if (ReturnValue != WIDGET_PASSED_MSG) {
        return ReturnValue;
    }
    switch (msg) {
    case HMSG_RECORDER_STATE_LIVEVIEW:
        DBGMSG("[app_rec_connected_cam] Received HMSG_RECORDER_STATE_LIVEVIEW");
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_LIVEVIEW_STATE, 0, 0);
        break;
    case HMSG_RECORDER_STATE_PHOTO_CAPTURE_COMPLETE:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CAPTURE_COMPLETE, 0, 0);
        break;
    case HMSG_RECORDER_STATE_PHOTO_BGPROC_COMPLETE:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CAPTURE_BG_PROCESS_DONE, 0, 0);
        break;
    case HMSG_USER_SNAP1_BUTTON:
    case HMSG_USER_IR_SNAP1_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonFocus();
        break;
    case HMSG_USER_SNAP1_BUTTON_CLR:
    case HMSG_USER_IR_SNAP1_BUTTON_CLR:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonFocusClr();
        break;
    case HMSG_USER_SNAP2_BUTTON:
    case HMSG_USER_IR_SNAP2_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonShutter();
        break;
    case HMSG_USER_SNAP2_BUTTON_CLR:
    case HMSG_USER_IR_SNAP2_BUTTON_CLR:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonShutterClr();
        break;
    case HMSG_USER_RECORD_BUTTON:
    case HMSG_USER_IR_RECORD_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonRecord();
        break;
    case HMSG_USER_ZOOM_IN_BUTTON:
    case HMSG_USER_IR_ZOOM_IN_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonZoomIn();
        break;
    case HMSG_USER_ZOOM_OUT_BUTTON:
    case HMSG_USER_IR_ZOOM_OUT_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonZoomOut();
        break;
    case HMSG_USER_ZOOM_IN_BUTTON_CLR:
    case HMSG_USER_IR_ZOOM_IN_BUTTON_CLR:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonZoomInClr();
        break;
    case HMSG_USER_ZOOM_OUT_BUTTON_CLR:
    case HMSG_USER_IR_ZOOM_OUT_BUTTON_CLR:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonZoomOutClr();
        break;
    case HMSG_USER_UP_BUTTON:
    case HMSG_USER_IR_UP_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonUp();
        break;
    case HMSG_USER_DOWN_BUTTON:
    case HMSG_USER_IR_DOWN_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonDown();
        break;
    case HMSG_USER_LEFT_BUTTON:
    case HMSG_USER_IR_LEFT_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonLeft();
        break;
    case HMSG_USER_RIGHT_BUTTON:
    case HMSG_USER_IR_RIGHT_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonRight();
        break;
    case HMSG_USER_SET_BUTTON:
    case HMSG_USER_IR_SET_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonSet();
        break;
    case HMSG_USER_F4_BUTTON:
    case HMSG_USER_IR_N4_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonF4();
        break;
    case HMSG_USER_MENU_BUTTON:
    case HMSG_USER_IR_MENU_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonMenu();
        break;
    case HMSG_USER_MODE_BUTTON:
    case HMSG_USER_IR_MODE_BUTTON:
        ReturnValue = rec_connected_cam.Op->ButtonMode();
        break;
    case HMSG_USER_DEL_BUTTON:
    case HMSG_USER_IR_DEL_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonDel();
        break;
    case HMSG_USER_POWER_BUTTON:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Op->ButtonPower();
        break;
    case AMSG_CMD_SWITCH_APP:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SWITCH_APP, param1, param2);
        break;
    case HMSG_MUXER_START:
        DBGMSG("[app_rec_connected_cam] Received HMSG_MUXER_START");
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_MUXER_START, param1, param2);
        break;
    case HMSG_MUXER_END:
        DBGMSG("[app_rec_connected_cam] Received HMSG_MUXER_END, Muxer type = %d",param1);
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_MUXER_END, param1, param2);
        break;
    case HMSG_MUXER_OPEN:
        DBGMSG("[app_rec_connected_cam] Received HMSG_MUXER_OPEN, Muxer type = %d",param1);
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_MUXER_OPEN, param1, param2);
        break;
    case HMSG_MUXER_END_EVENTRECORD:
        DBGMSG("[app_rec_connected_cam] Received HMSG_MUXER_END_EVENTRECORD");
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_MUXER_END_EVENTRECORD, param1, param2);
        break;
    case HMSG_MUXER_REACH_LIMIT:
        DBGMSG("[app_rec_connected_cam] Received HMSG_MUXER_REACH_LIMIT");
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_MUXER_REACH_LIMIT, param1, param2);
        break;
    case HMSG_MUXER_REACH_LIMIT_EVENTRECORD:
        DBGMSG("[app_rec_connected_cam] Received HMSG_MUXER_REACH_LIMIT_EVENTRECORD");
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_MUXER_REACH_LIMIT_EVENTRECORD, param1, param2);
        break;
    case HMSG_DCF_FILE_CLOSE:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_FILE_ID_UPDATE, param1, param2);
        DBGMSG("[app_rec_connected_cam] Received HMSG_DCF_FILE_CLOSE, file object ID = %d",param1);
        break;
    case HMSG_MEMORY_FIFO_BUFFER_RUNOUT:
        DBGMSG("[app_rec_connected_cam] Received HMSG_MEMORY_FIFO_BUFFER_RUNOUT");
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_ERROR_MEMORY_RUNOUT, param1, param2);
        break;
    case HMSG_MUXER_IO_ERROR:
        DBGMSG("[app_rec_connected_cam] Received HMSG_MUXER_IO_ERROR");
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_MUXER_STREAM_ERROR, param1, param2);
        break;
    case HMSG_MUXER_FIFO_ERROR:
        DBGMSG("[app_rec_connected_cam] Received AMP_MUXER_EVENT_FIFO_ERROR");
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_ERROR_MEMORY_RUNOUT, param1, param2);
        break;
    case AMSG_ERROR_CARD_REMOVED:
        if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_ERROR_REMOVED, param1, param2);
        }
        break;
    case AMSG_STATE_CARD_REMOVED:
        if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_REMOVED, param1, param2);
        }
        break;
    case HMSG_STORAGE_BUSY:
        if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_STORAGE_BUSY, param1, param2);
        }
        break;
    case HMSG_STORAGE_IDLE:
        if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_STORAGE_IDLE, param1, param2);
        }
        break;
    case HMSG_STORAGE_RUNOUT:
        DBGMSG("[app_rec_connected_cam] Received HMSG_STORAGE_RUNOUT");
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_ERROR_STORAGE_RUNOUT, param1, param2);
        break;
    case HMSG_STORAGE_REACH_FILE_LIMIT:
    case HMSG_STORAGE_REACH_FILE_NUMBER:
        break;
    case HMSG_LOOP_ENC_DONE:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_LOOP_ENC_DONE, 0, 0);
        break;
    case HMSG_LOOP_ENC_ERROR:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_ERROR_LOOP_ENC_ERR, param1, 0);
        break;
    case HMSG_EDTMGR_SUCCESS:
        AmbaPrintColor(5,"[app_rec_connected_cam]HMSG_EDTMGR_SUCCESS");
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_EDTMGR_DONE, 0, 0);
        break;
    case HMSG_EDTMGR_FAIL:
        AmbaPrintColor(5,"[app_rec_connected_cam]HMSG_EDTMGR_FAIL");
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_EDTMGR_FAIL, 0, 0);
        break;
    case HMSG_EM_RECORD_RETURN:
        AmbaPrintColor(5,"HMSG_EM_RECORD_RETURN");
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_EM_RECORD_RETURN, param1, 0);
        break;
    case HMSG_HDMI_INSERT_SET:
    case HMSG_HDMI_INSERT_CLR:
    case HMSG_CS_INSERT_SET:
    case HMSG_CS_INSERT_CLR:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            AmbaPrint("[app_rec_connected_cam] System is not ready. Jack event will be handled later");
        } else {
            ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_UPDATE_FCHAN_VOUT, msg, param1);
        }
        break;
    case HMSG_LINEIN_IN_SET:
    case HMSG_LINEIN_IN_CLR:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            AmbaPrint("[app_rec_connected_cam] System is not ready. Jack event will be handled later");
        } else {
            ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_AUDIO_INPUT, param1, param2);
        }
        break;
    case HMSG_HP_IN_SET:
    case HMSG_HP_IN_CLR:
    case HMSG_LINEOUT_IN_SET:
    case HMSG_LINEOUT_IN_CLR:
        if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            AmbaPrint("[app_rec_connected_cam] System is not ready. Jack event will be handled later");
        } else {
            ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_AUDIO_OUTPUT, param1, param2);
        }
        break;
    case HMSG_VA_FCAR_DEPARTURE:
    case HMSG_VA_LOW_LIGHT:
    case HMSG_VA_MD_Y:
    case HMSG_VA_MD_AE:
    case HMSG_VA_LDW:
    case HMSG_VA_FCW:
    case HMSG_VA_UPDATE:
    case HMSG_VA_CLIBRATION_DONE:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_ADAS_EVENT, msg, param1);
        break;
    case HMSG_USB_DETECT_SUSPEND:
        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_USB)) {
            APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_USB);
        } else {
            APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_USB);
        }
        break;
    case AMSG_CMD_USB_APP_START:
    case HMSG_USB_DETECT_CONNECT:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_USB_CONNECT, param1, param2);
        break;
    case AMSG_CMD_SET_VIDEO_RES:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SET_VIDEO_RES, param1, param2);
        break;
    case AMSG_CMD_SET_VIDEO_QUALITY:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SET_VIDEO_QUALITY, param1, param2);
        break;
    case AMSG_CMD_SET_VIDEO_PRE_RECORD:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SET_VIDEO_PRE_RECORD, param1, param2);
        break;
    case AMSG_CMD_SET_VIDEO_TIME_LAPSE:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SET_VIDEO_TIME_LAPSE, param1, param2);
        break;
    case AMSG_CMD_SET_VIDEO_DUAL_STREAMS:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SET_VIDEO_DUAL_STREAMS, param1, param2);
        break;
    case AMSG_CMD_SET_PHOTO_SIZE:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SET_PHOTO_SIZE, param1, param2);
        break;
    case AMSG_CMD_SET_PHOTO_QUALITY:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SET_PHOTO_QUALITY, param1, param2);
        break;
    case AMSG_CMD_SET_RECORD_MODE:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SET_ENC_MODE, param1, param2);
        break;
    case AMSG_CMD_SET_APP_ENC_MODE:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SET_ENC_MODE, param1, param2);
        break;
    case AMSG_CMD_SET_DMF_MODE:
        DBGMSG("[app_rec_connected_cam]Received AMSG_CMD_SET_DMF_MODE");
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_SET_DMF_MODE, param1, param2);
        break;
    case AMSG_CMD_CARD_FMT_NONOPTIMUM:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_FMT_NONOPTIMUM, param1, param2);
        break;
    case AMSG_CMD_CARD_MOVIE_RECOVER:
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
        /*Stop VF to do busy check, busy check will enable movie recover*/
        if (rec_connected_cam.RecCapState == REC_CAP_STATE_VF) {
            rec_connected_cam.Func(REC_CONNECTED_CAM_VF_STOP, 0, 0);
        }
#endif
        break;
    case AMSG_STATE_WIDGET_CLOSED:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_WIDGET_CLOSED, param1, param2);
        break;
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    case AMSG_EVENT_BOSS_BOOTED:
        if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
            /* Handling the case that HMSG_RECORDER_STATE_LIVEVIEW comes before AMSG_CMD_BOSS_BOOTED. */
            ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_BOSS_BOOTED, param1, param2);
        }
        break;
    case AMSG_NETFIFO_EVENT_START:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_NETFIFO_EVENT_START, 0, 0);
        break;
    case AMSG_NETFIFO_EVENT_STOP:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_NETFIFO_EVENT_STOP, 0, 0);
        break;
    case AMSG_NETCTRL_SESSION_START:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlRefreshPrefTable();
        break;
    case AMSG_NETCTRL_VF_RESET:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlVFReset();
        break;
    case AMSG_NETCTRL_VF_STOP:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlVFStop();
        break;
    case AMSG_NETCTRL_SESSION_STOP:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlStopSession();
        break;
    case AMSG_NETCTRL_VIDEO_RECORD_START:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlRecordStart();
        break;
    case AMSG_NETCTRL_VIDEO_RECORD_STOP:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlRecordStop();
        break;
    case AMSG_NETCTRL_VIDEO_GET_RECORD_TIME:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlGetRecordTime();
        break;
    case AMSG_NETCTRL_PHOTO_TAKE_PHOTO:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlCapture();
        break;
    case AMSG_NETCTRL_PHOTO_CONTINUE_CAPTURE_STOP:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlContinueCaptureStop();
        break;
    case AMSG_NETCTRL_SYS_GET_SETTING_ALL:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlGetAllCurSetting();
        break;
    case AMSG_NETCTRL_SYS_GET_SINGLE_SETTING_OPTION:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlGetSettingOptions(param1, param2);
        break;
    case AMSG_NETCTRL_SYS_GET_SETTING:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlGetSetting(param1, param2);
        break;
    case AMSG_NETCTRL_SYS_SET_SETTING:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlSetSetting(param1, param2);
        break;
    case AMSG_NETCTRL_SYS_GET_NUMB_FILES:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlGetNumbFiles(param1, param2);
        break;
    case AMSG_NETCTRL_SYS_GET_DEVICE_INFO:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlGetDeviceInfo();
        break;
    case AMSG_NETCTRL_MEDIA_GET_THUMB:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlGetThumb(param1, param2);
        break;
    case AMSG_NETCTRL_MEDIA_GET_MEDIAINFO:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlGetMediaInfo(param1, param2);
        break;
    case AMSG_NETCTRL_CUSTOM_CMD:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlCustomCmd(param1, param2);
        break;
    case AMSG_NETCTRL_SYS_FORMAT:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlFormat(param1, param2);
        break;
    case ASYNC_MGR_MSG_CARD_FORMAT_DONE:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlFormatDone(param1, param2);
        break;
    case AMSG_NETCTRL_SYS_GET_SPACE:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlGetSpace(param1, param2);
        break;
    case AMSG_NETCTRL_SYS_BURNIN_FW:
        ReturnValue = rec_connected_cam.NetCtrl->NetCtrlBurninFw(param1, param2);
        break;
    case HMSG_USER_VF_START:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_VF_START, msg, 0);
        break;
    case HMSG_USER_VF_STOP:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_VF_STOP, msg, 0);
        break;
    case HMSG_USER_VF_SWITCH_TO_RECORD:
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_VF_SWITCH_TO_RECORD, msg, 0);
        break;
    case HMSG_USER_PIRNT_REC_CAP_STATE:
        AmbaPrint("rec_connected_cam.RecCapState = %d",rec_connected_cam.RecCapState);
        break;
#endif
    default:
        break;
    }

    return ReturnValue;
}

/**
 *  @brief The start flow of recoder application.
 *
 *  The start flow of recoder application.
 *
 *  @return >=0 success, <0 failure
 */
static int app_rec_connected_cam_start(void)
{
    int ReturnValue = 0;

    /* Set app function and operate sets */
    rec_connected_cam.Func = rec_connected_cam_func;
    rec_connected_cam.Gui = gui_rec_connected_cam_func;
    rec_connected_cam.Op = &rec_connected_cam_op;

    #if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    rec_connected_cam.NetCtrlFlags = 0;
    rec_connected_cam.NetCtrl = &rec_connected_cam_netctrl_op;
    #endif

    if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_INIT)) {
        APP_ADDFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_INIT);
        rec_connected_cam.Func(REC_CONNECTED_CAM_INIT, 0, 0);
    }

    if (APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_START)) {
        rec_connected_cam.Func(REC_CONNECTED_CAM_START_FLAG_ON, 0, 0);
    } else {
        APP_ADDFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_START);

        rec_connected_cam.Func(REC_CONNECTED_CAM_SET_APP_ENV, 0, 0);

        rec_connected_cam.Func(REC_CONNECTED_CAM_START, 0, 0);
    }

    return ReturnValue;
}

/**
 *  @brief The stop flow of recoder application.
 *
 *  The stop flow of recoder application.
 *
 *  @return >=0 success, <0 failure
 */
static int app_rec_connected_cam_stop(void)
{
    int ReturnValue = 0;

    if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_READY)) {
        rec_connected_cam.Func(REC_CONNECTED_CAM_STOP, 0, 0);
    }

    return ReturnValue;
}
