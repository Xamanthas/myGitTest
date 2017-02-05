/**
  * @file src/app/apps/flow/rec/connectedcam/rec_connected_cam_op.c
  *
  * Operations of Sport Recorder (sensor) application
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
#include <system/app_util.h>
#include <apps/gui/resource/gui_settle.h>

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
#include <system/app_pref.h>
#endif

/*************************************************************************
 * App Function Declarations (static)
 ************************************************************************/
static int rec_connected_cam_button_record(void);
static int rec_connected_cam_button_focus(void);
static int rec_connected_cam_button_focus_clr(void);
static int rec_connected_cam_button_shutter(void);
static int rec_connected_cam_button_shutter_clr(void);
static int rec_connected_cam_button_zoom_in(void);
static int rec_connected_cam_button_zoom_in_clr(void);
static int rec_connected_cam_button_zoom_out(void);
static int rec_connected_cam_button_zoom_out_clr(void);
static int rec_connected_cam_button_up(void);
static int rec_connected_cam_button_down(void);
static int rec_connected_cam_button_left(void);
static int rec_connected_cam_button_right(void);
static int rec_connected_cam_button_set(void);
static int rec_connected_cam_button_menu(void);
static int rec_connected_cam_button_mode(void);
static int rec_connected_cam_button_del(void);
static int rec_connected_cam_button_power(void);
static int rec_connected_cam_button_f4(void);

REC_CONNECTED_CAM_OP_s rec_connected_cam_op = {
    rec_connected_cam_button_record,
    rec_connected_cam_button_focus,
    rec_connected_cam_button_focus_clr,
    rec_connected_cam_button_shutter,
    rec_connected_cam_button_shutter_clr,
    rec_connected_cam_button_zoom_in,
    rec_connected_cam_button_zoom_in_clr,
    rec_connected_cam_button_zoom_out,
    rec_connected_cam_button_zoom_out_clr,
    rec_connected_cam_button_up,
    rec_connected_cam_button_down,
    rec_connected_cam_button_left,
    rec_connected_cam_button_right,
    rec_connected_cam_button_set,
    rec_connected_cam_button_menu,
    rec_connected_cam_button_mode,
    rec_connected_cam_button_del,
    rec_connected_cam_button_power,
    rec_connected_cam_button_f4
};

/**
 *  @brief The operation of Record button.
 *
 *  The operation of Record button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_record(void)
{
    int ReturnValue = 0;

    /* Close the menu or dialog. */
    AppWidget_Off(WIDGET_ALL, 0);
    if (rec_connected_cam.RecCapState == REC_CAP_STATE_PREVIEW) {
        /* Check the card's status. */
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_CHECK_STATUS, 0, 0);
        if (ReturnValue == 0) {
            /* To record the clip if the card is ready. */
            rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_START, 0, 0);
        }
    } else if (rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) {

        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_MUXER_BUSY)) {
            /* Stop recording. */
            rec_connected_cam.Func(REC_CONNECTED_CAM_RECORD_STOP, 0, 0);
        } else {
            AmbaPrintColor(YELLOW, "[rec_connected_cam] <button_record> Record not actually start, block record stop");
        }
    }
    #if defined (CONFIG_APP_CONNECTED_AMBA_LINK)
    else if (rec_connected_cam.RecCapState == REC_CAP_STATE_VF) {
        AmbaPrintColor(YELLOW, "[rec_connected_cam] <button_record> VF -> REC");
        rec_connected_cam.Func(REC_CONNECTED_CAM_VF_SWITCH_TO_RECORD, 0, 0);
    }
    #endif

    return ReturnValue;
}

/**
 *  @brief The operation of Focus button.
 *
 *  The operation of Focus button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_focus(void)
{
    int ReturnValue = 0;
    if (app_rec_connected_cam.Child != 0) {
        AppUtil_SwitchApp(APP_REC_CONNECTED_CAM);    // shrink from Child apps
    }
    return ReturnValue;
}

/**
 *  @brief The operation of Focus button release.
 *
 *  The operation of Focus button release.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_focus_clr(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Shutter button.
 *
 *  The operation of Shutter button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_shutter(void)
    {
    int ReturnValue = 0;
    int PhotoAmount = 0;
    APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_SHUTTER_PRESSED);
    if (app_rec_connected_cam.Child != 0) {
        AppUtil_SwitchApp(APP_REC_CONNECTED_CAM);    // shrink from Child apps
    }
    /* Close the menu or dialog. */
    AppWidget_Off(WIDGET_ALL, 0);
    /**PIV can not use GetCaptureNum function*/
    PhotoAmount = AppLibStorageDmf_GetFileAmount(APPLIB_DCF_MEDIA_IMAGE,DCIM_HDLR) + rec_connected_cam.MuxerNum;
    AmbaPrintColor(5,"[%s] Photo amount before capture %d",__func__,PhotoAmount);
    if (rec_connected_cam.RecCapState == REC_CAP_STATE_PREVIEW) {
        /* Check the card's status. */
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_CHECK_STATUS, 1, 0);
        if (ReturnValue == 0) {
            /* Check the photo count. */
            PhotoAmount = PhotoAmount + AppLibStillEnc_GetCaptureNum();
            if (PhotoAmount <= MAX_PHOTO_COUNT) {

                /* Capture the photo if the card is ready. */
                ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CAPTURE, 0, 0);
            } else {
                ReturnValue = -1;
                rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_PHOTO_LIMIT, 0);
                AmbaPrintColor(CYAN,"[rec_connected_cam] <button_shutter> Photo count reach limit, can not do capture (%d)",PhotoAmount );
            }
        }
    } else if (rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) {
        /* Check the photo count. */
        PhotoAmount = PhotoAmount + 1;
        if (PhotoAmount <= MAX_PHOTO_COUNT) {
            rec_connected_cam.Func(REC_CONNECTED_CAM_CAPTURE_PIV, 0, 0);
        } else {
            ReturnValue = -1;
            rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_PHOTO_LIMIT, 0);
            AmbaPrintColor(CYAN,"[rec_connected_cam] <button_shutter> Photo count reach limit, can not do capture (%d)",PhotoAmount);
        }
    }
#if 0 // Block VF mode capture
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    else if (rec_connected_cam.RecCapState == REC_CAP_STATE_VF) {
        /* Check the card's status. */
        ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CARD_CHECK_STATUS, 1, 0);
        if (ReturnValue == 0) {
            /* Check the photo count. */
            PhotoAmount = PhotoAmount + AppLibStillEnc_GetCaptureNum();
            if (PhotoAmount <= MAX_PHOTO_COUNT) {
                AmbaPrintColor(CYAN,"[rec_connected_cam] <button_shutter> REC_CONNECTED_CAM_CAPTURE_ON_VF");
                ReturnValue = rec_connected_cam.Func(REC_CONNECTED_CAM_CAPTURE_ON_VF, 0, 0);
            } else {
                ReturnValue = -1;
                rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_START, GUI_WARNING_PHOTO_LIMIT, 0);
                AmbaPrintColor(CYAN,"[rec_connected_cam] <button_shutter> Photo count reach limit, can not do capture (%d)",PhotoAmount);
            }
        }
    }
#endif
#endif
    else {
        AmbaPrintColor(CYAN,"[rec_connected_cam] <button_shutter> This state does not support capture ");
        ReturnValue = -1;
    }

    if (ReturnValue == 0) {
        /* Flag 'REC_CONNECTED_CAM_FLAGS_BLOCK_MENU' is used to block menu while capturing.
               Set this flag when cpature starts. And this flag should be cleared when capture is done,
               including muxer finished. */
        APP_ADDFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_MENU);
    }

    return ReturnValue;
}

/**
 *  @brief The operation of Shutter button release.
 *
 *  The operation of Shutter button release.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_shutter_clr(void)
{
    int ReturnValue = 0;

    DBGMSG("[rec_connected_cam] Release the shutter button. ");

    APP_REMOVEFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_SHUTTER_PRESSED);

    return ReturnValue;
}

/**
 *  @brief The operation of Zoom_in button.
 *
 *  The operation of Zoom_in button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_zoom_in(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Zoom_in button release.
 *
 *  The operation of Zoom_in button release.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_zoom_in_clr(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Zoom_out button.
 *
 *  The operation of Zoom_out button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_zoom_out(void)
{
    int ReturnValue = 0;
    return ReturnValue;
}

/**
 *  @brief The operation of Zoom_out button release.
 *
 *  The operation of Zoom_out button release.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_zoom_out_clr(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Up button.
 *
 *  The operation of Up button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_up(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Down button.
 *
 *  The operation of Down button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_down(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Left button.
 *
 *  Left button operation
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_left(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Right button.
 *
 *  The operation of Right button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_right(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Set button.
 *
 *  The operation of Set button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_set(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Menu button.
 *
 *  The operation of Menu button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_menu(void)
{
    int ReturnValue = 0;

    if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_BLOCK_MENU)) {
        DBGMSG("[rec_connected_cam] <button_menu> Block menu.");
        return -1;
    }

    if (!APP_CHECKFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_POPUP)) {
        if (APP_CHECKFLAGS(app_rec_connected_cam.Flags, REC_CONNECTED_CAM_FLAGS_SELFTIMER_RUN)) {
            rec_connected_cam.Func(REC_CONNECTED_CAM_SELFTIMER_STOP, 0, 0);
            rec_connected_cam.Func(REC_CONNECTED_CAM_PREVIEW, 0, 0);
        }

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

        rec_connected_cam.Func(REC_CONNECTED_CAM_WARNING_MSG_SHOW_STOP, 0, 0);
        APP_ADDFLAGS(app_rec_connected_cam.GFlags, APP_AFLAGS_POPUP);
        ReturnValue = AppWidget_On(WIDGET_MENU, 0);
    }

    return ReturnValue;
}

/**
 *  @brief The operation of Mode button.
 *
 *  The operation of Mode button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_mode(void)
{
    int ReturnValue = 0;


    /*block change mode at capture on vf ,due to capture on vf will remove busy flag at change video to stil
    so flow will be interrupt if allow switch model*/
    if (!APP_CHECKFLAGS(app_rec_connected_cam.Flags,REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF)) {
        /* Switch mode. */
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    if (rec_connected_cam.RecCapState == REC_CAP_STATE_TRANSIT_TO_VF &&
        UserSetting->VideoPref.StreamType == STREAM_TYPE_RTSP) {
        /**notify net fifo stop and change state to reset to suspend vf start*/
        AppLibNetFifo_NotifyAppStateChange(AMP_NETFIFO_NOTIFY_STOPENC);
    }
#endif
        AppUtil_SwitchMode(0);
    } else {
        AmbaPrint("[app_rec_connected_cam] Block switch mode at capture on VF");
    }

    return ReturnValue;
}

/**
 *  @brief The operation of Delete button.
 *
 *  The operation of Delete button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_del(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Power button.
 *
 *  The operation of Power button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_power(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of F4 button.
 *
 *  The operation of F4 button.
 *
 *  @return >=0 success, <0 failure
 */
static int rec_connected_cam_button_f4(void)
{
    int ReturnValue = 0;

    if (rec_connected_cam.RecCapState == REC_CAP_STATE_RECORD) {
        rec_connected_cam.Func(REC_CONNECTED_CAM_EVENTRECORD_START, 0, 0);
    }
    else{
        AmbaPrint("[app_rec_connected_cam] Block eventrecord when system is not in recording");
        return 0;
    }
    return ReturnValue;
}

