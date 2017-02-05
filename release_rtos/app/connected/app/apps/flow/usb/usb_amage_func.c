/**
  * @file src/app/apps/flow/usb/connectedcam/usb_amage_func.c
  *
  * Functions of USB MTP class for Amage
  *
  * History:
  *    2013/12/03 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */


#include <apps/flow/usb/usb_amage.h>
#include <system/status.h>
#include <system/app_pref.h>
#include <system/app_util.h>
#include <recorder/StillEnc.h>

extern AMBA_DSP_CHANNEL_ID_u AppEncChannel;
static int usb_amage_init(void)
{
    int ReturnValue = 0;
    /** Initialize the vin. */
    AppLibSysVin_Init();

    /** Initialize the video recorder. */
    AppLibVideoEnc_Init();

    /** Initialize the still recorder. */
    AppLibStillEnc_Init();

    /**Default open at video mode*/
    usb_amage.CurMode = USB_AMAGE_VIDEO_MODE;
    /**Set photo mode to 16:9 */
    {
        int Ar = 0;
        AppLibStillEnc_SetNormCapMode(PHOTO_CAP_MODE_PRECISE);
        Ar = AppLibSysSensor_GetCaptureModeAR(AppLibStillEnc_GetPhotoPjpegCapMode(), AppLibStillEnc_GetPhotoPjpegConfigId());
        while (Ar != VAR_16x9) {
            if (AppLibStillEnc_GetPhotoPjpegConfigId() < AppLibSysSensor_GetPjpegConfigNum(AppLibStillEnc_GetPhotoPjpegCapMode())) {
                AppLibStillEnc_SetSizeID(AppLibStillEnc_GetPhotoPjpegConfigId()+1);
            } else {
                AppLibStillEnc_SetSizeID(0);
            }
            Ar = AppLibSysSensor_GetCaptureModeAR(AppLibStillEnc_GetPhotoPjpegCapMode(), AppLibStillEnc_GetPhotoPjpegConfigId());
        }
    }
    return ReturnValue;
}

static int usb_amage_start(void)
{
    int ReturnValue = 0;

    if (app_status.UsbPluginFlag == 1) {
        AppLibUSB_Init();
        ApplibUsbAmage_Start();
    }

    AppLibVideoEnc_SetDualStreams(VIDEO_DUAL_STREAMS_OFF);

    /**Delete encode to disable second stream codec*/
    AppLibVideoEnc_PipeDelete();
    /* Video preview.*/
    AppLibVideoEnc_LiveViewSetup();
    usb_amage.Func(USB_AMAGE_CHANGE_DISPLAY, 0, 0);
    AppLibVideoEnc_LiveViewStart();

    AmbaPrintColor(5,"[app_usb_amage] USB AMAGE MODE");
    return ReturnValue;
}

static int usb_amage_stop(void)
{
    int ReturnValue = 0;

    /** Stop the liveview. */
    if (usb_amage.CurMode == USB_AMAGE_VIDEO_MODE) {
        AppLibVideoEnc_LiveViewStop();
    } else {
        AppLibStillEnc_LiveViewStop();
        AppLibStillEnc_LiveViewDeInit();
    }
    /**Delete encode to reset codec setting*/
    AppLibVideoEnc_PipeDelete();
    AppLibVideoEnc_SetDualStreams(UserSetting->VideoPref.DualStreams);
    AppLibStillEnc_SetNormCapMode(UserSetting->PhotoPref.PhotoCapMode);
    /* Disable the vout. */
    AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
    AppLibDisp_DeactivateWindow(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0));
    AppLibDisp_ChanStop(DISP_CH_FCHAN);

    return ReturnValue;
}

static int usb_amage_change_display(void)
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

        memset(&Window, 0, sizeof(AMP_DISP_WINDOW_CFG_s));

        if (usb_amage.CurMode == USB_AMAGE_VIDEO_MODE) {
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
            AmbaPrint("[app_usb_amage] FChan Disable. Disable the fchan window");
            AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
        } else {
            /** FCHAN window*/
            AppLibDisp_GetWindowConfig(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0), &Window);
            Window.Source = AMP_DISP_ENC;
            Window.SourceDesc.Enc.VinCh = AppEncChannel;
            Window.CropArea.Width = 0;
            Window.CropArea.Height = 0;
            Window.CropArea.X = 0;
            Window.CropArea.Y = 0;
            Window.TargetAreaOnPlane.Width = DispDev.DeviceInfo.VoutWidth;
            Window.TargetAreaOnPlane.Height = DispDev.DeviceInfo.VoutHeight;//  interlance should be consider in MW
            Window.TargetAreaOnPlane.X = 0;
            Window.TargetAreaOnPlane.Y = 0;
            AppLibDisp_SetWindowConfig(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0), &Window);
            AppLibDisp_ActivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
        }

        ReturnValue = AppLibDisp_GetDeviceInfo(DISP_CH_DCHAN, &DispDev);
        if ((ReturnValue < 0) || (DispDev.DeviceInfo.Enable == 0)) {
            AmbaPrint("[app_usb_amage] DChan Disable. Disable the Dchan window");
            AppLibDisp_DeactivateWindow(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0));
        } else {
            /** DCHAN window*/
            PreviewParam.ChanID = DISP_CH_DCHAN;
            AppLibDisp_CalcPreviewWindowSize(&PreviewParam);
            AppLibDisp_GetWindowConfig(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0), &Window);
            Window.Source = AMP_DISP_ENC;
            Window.SourceDesc.Enc.VinCh = AppEncChannel;
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
        }
        AppLibDisp_FlushWindow(DISP_CH_FCHAN | DISP_CH_DCHAN);
    }

    return ReturnValue;
}


static void usb_amage_switch_mode(void)
{
    int ReturnValue = 0;

    if (app_usb_amage.Parent != APP_MAIN) {
        AppUtil_SwitchApp(app_usb_amage.Parent);
    } else {
        /** Switch to decode mode if the Fchan will be enabled and the app_status.FchanDecModeOnly is on.*/
        ReturnValue = AppLibDisp_SelectDevice(DISP_CH_FCHAN, DISP_ANY_DEV);
        if (APP_CHECKFLAGS(ReturnValue, DISP_FCHAN_NO_DEVICE) || (app_status.FchanDecModeOnly == 0) ) {
            app_status.LockDecMode = 0;
            AppUtil_SwitchApp(AppUtil_GetStartApp(0));
        } else {
            ReturnValue = AppLibDisp_ConfigMode(DISP_CH_FCHAN, AppLibSysVout_GetVoutMode(VOUT_DISP_MODE_1080P));
            if (ReturnValue < 0) {
                app_status.LockDecMode = 0;
                AppUtil_SwitchApp(AppUtil_GetStartApp(0));
            } else {
                /*Switch to thumbnail mode if the fchan will be enabled.*/
                app_status.LockDecMode = 1;
                AppUtil_SwitchApp(APP_THUMB_MOTION);
            }
        }

    }

}

static void usb_amage_usb_connect(void)
{
    if (app_status.UsbPluginFlag == 1) {
        AppLibUSB_Init();
        ApplibUsbAmage_Start();
    }

}

static void usb_amage_change_enc_mode(void)
{

    /** Stop the liveview. */
    if (usb_amage.CurMode == USB_AMAGE_VIDEO_MODE) {
        AppLibVideoEnc_LiveViewStop();
        usb_amage.CurMode = USB_AMAGE_STILL_MODE;
        /* still preview. */
        AppLibStillEnc_LiveViewSetup();
        usb_amage.Func(USB_AMAGE_CHANGE_DISPLAY, 0, 0);
        AppLibStillEnc_LiveViewStart();
        AmbaPrintColor(5,"[app_usb_amage] STILL MODE");

    } else {
        AppLibStillEnc_LiveViewStop();
        AppLibStillEnc_LiveViewDeInit();
        usb_amage.CurMode = USB_AMAGE_VIDEO_MODE;

        /* Video preview. */
        AppLibVideoEnc_LiveViewSetup();
        usb_amage.Func(USB_AMAGE_CHANGE_DISPLAY, 0, 0);
        AppLibVideoEnc_LiveViewStart();
        AmbaPrintColor(5,"[app_usb_amage] VIDEO MODE");
    }

}

static int usb_amage_update_fchan_vout(UINT32 msg)
{
    int ReturnValue = 0;

    switch (msg) {
    case HMSG_HDMI_INSERT_SET:
    case HMSG_HDMI_INSERT_CLR:
        AppLibSysVout_SetJackHDMI(app_status.HdmiPluginFlag);
        break;
    default:
        return 0;
        break;
    }

    ReturnValue = AppLibDisp_SelectDevice(DISP_CH_FCHAN, DISP_ANY_DEV);
    if (APP_CHECKFLAGS(ReturnValue, DISP_FCHAN_NO_CHANGE)) {
        AmbaPrint("[app_usb_amage] Display FCHAN has no changed");
    } else {

        if (APP_CHECKFLAGS(ReturnValue, DISP_FCHAN_NO_DEVICE)) {
            AppLibDisp_ChanStop(DISP_CH_FCHAN);
            AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
            AppLibGraph_DeactivateWindow(GRAPH_CH_FCHAN);
            AppLibDisp_FlushWindow(DISP_CH_FCHAN);
        } else {
            AMP_DISP_WINDOW_CFG_s Window;
            AMP_DISP_INFO_s DispDev = {0};
            memset(&Window, 0, sizeof(AMP_DISP_WINDOW_CFG_s));

            /**Stop liveview*/
            if (usb_amage.CurMode == USB_AMAGE_VIDEO_MODE) {
                AppLibVideoEnc_LiveViewStop();
            } else {
                AppLibStillEnc_LiveViewStop();
                AppLibStillEnc_LiveViewDeInit();
            }


            AppLibDisp_ConfigMode(DISP_CH_FCHAN, AppLibSysVout_GetVoutMode(VOUT_DISP_MODE_1080P));
            AppLibDisp_SetupChan(DISP_CH_FCHAN);
            AppLibDisp_ChanStart(DISP_CH_FCHAN);

            ReturnValue = AppLibDisp_GetDeviceInfo(DISP_CH_FCHAN, &DispDev);
            if ((ReturnValue < 0) || (DispDev.DeviceInfo.Enable == 0)) {
                    AmbaPrint("[app_usb_amage] FChan Disable. Disable the fchan window");
                    AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
                    AppLibGraph_DeactivateWindow(GRAPH_CH_FCHAN);
                    AppLibDisp_FlushWindow(DISP_CH_FCHAN);
            } else {
                    /** FCHAN window*/
                    AppLibDisp_GetWindowConfig(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0), &Window);
                    Window.Source = AMP_DISP_ENC;
                    Window.SourceDesc.Enc.VinCh = AppEncChannel;
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
            }

            /**Start Liveview*/
            if (usb_amage.CurMode == USB_AMAGE_VIDEO_MODE) {
                AppLibVideoEnc_LiveViewSetup();
                AppLibVideoEnc_LiveViewStart();
            } else {
                AppLibStillEnc_LiveViewSetup();
                AppLibStillEnc_LiveViewStart();
            }
        }
    }



    return ReturnValue;
}

static int usb_amage_still_capture(void)
{
    int ReturnValue;
    if (usb_amage.CurMode != USB_AMAGE_STILL_MODE) {
        AmbaPrintColor(RED,"[app_usb_amage] It's not in still mode. Can not do capture.");
        return -1;
    }

    if (!APP_CHECKFLAGS(app_usb_amage.Flags, USB_AMAGE_FLAGS_SINGLE_CAPTURE)){
        ReturnValue = AppLibCard_CheckStatus(CARD_CHECK_WRITE);
        if (ReturnValue != CARD_STATUS_CHECK_PASS ){
            AmbaPrintColor(RED,"[app_usb_amage] Card error. Can not do capture.");
            return ReturnValue;
        }
        APP_ADDFLAGS(app_usb_amage.Flags, USB_AMAGE_FLAGS_SINGLE_CAPTURE);

        /* Initialize and start the muxer. */
        AppLibFormat_MuxerInit();
        AppLibFormatMuxExif_Start();
        AppLibImage_Lock3A();
        AppLibStillEnc_CaptureSingle();

    } else {
        AmbaPrintColor(RED,"[app_usb_amage] Previous capture is not finish. Can not do capture.");
    }
    return 0;
}

static int usb_amage_muxer_end(void)
{
        AppLibFormatMuxExif_Close();
        AppLibFormatMuxMgr_MuxEnd();
        AppLibFormatMuxExif_End();
        APP_REMOVEFLAGS(app_usb_amage.Flags, USB_AMAGE_FLAGS_SINGLE_CAPTURE);

        AppLibStillEnc_SingleCapFreeBuf();/**<free buff*/

        return 0;
}

static int usb_amage_bg_process_done(void)
{
    AppLibImage_UnLock3A();
    if (usb_amage.CurMode == USB_AMAGE_STILL_MODE) {
        AmpStillEnc_EnableLiveviewCapture(AMP_STILL_STOP_LIVEVIEW, 0);  // Still codec return to idle state
        /**Restart liveview*/
        AppLibStillEnc_LiveViewSetup();
        AppLibStillEnc_LiveViewStart();
    } else {
        AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_VIDEO_LIVEVIEW, 0);
        AppLibVideoEnc_LiveViewSetup();
        AppLibVideoEnc_LiveViewStart();
    }
    switch(app_usb_amage.Flags) {
        case USB_AMAGE_FLAGS_STILL_RAW_CAPTURE:
            AppLibStillEnc_RawCapFreeBuf();
            APP_REMOVEFLAGS(app_usb_amage.Flags, USB_AMAGE_FLAGS_STILL_RAW_CAPTURE);
            break;
        case USB_AMAGE_FLAGS_VIDEO_RAW_CAPTURE:
            if (AppLibVideoEnc_GetFastCapture()) {
                AppLibVideoEnc_DumpRaw();
            }
            AppLibVideoEnc_RawCapFreeBuf();
            APP_REMOVEFLAGS(app_usb_amage.Flags, USB_AMAGE_FLAGS_VIDEO_RAW_CAPTURE);
            break;
        default:
            break;
    }
    return 0;
}

void usb_amage_set_capture_type(UINT32 CapType)
{
    switch(CapType) {
        case USB_AMAGE_FLAGS_SINGLE_CAPTURE:
            APP_ADDFLAGS(app_usb_amage.Flags, USB_AMAGE_FLAGS_SINGLE_CAPTURE);
            break;
        case USB_AMAGE_FLAGS_STILL_RAW_CAPTURE:
            APP_ADDFLAGS(app_usb_amage.Flags, USB_AMAGE_FLAGS_STILL_RAW_CAPTURE);
            break;
        case USB_AMAGE_FLAGS_VIDEO_RAW_CAPTURE:
            APP_ADDFLAGS(app_usb_amage.Flags, USB_AMAGE_FLAGS_VIDEO_RAW_CAPTURE);
            break;
        default:
            break;
    }
}

/**
 *  @brief The functions of application
 *
 *  The functions of application
 *
 *  @param[in] funcId Function id
 *  @param[in] param1 first parameter
 *  @param[in] param2 second parameter
 *
 *  @return >=0 success, <0 failure
 */
int usb_amage_func(UINT32 funcId, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;

    switch (funcId) {
    case USB_AMAGE_INIT:
        usb_amage_init();
        break;
    case USB_AMAGE_START:
        usb_amage_start();
        break;
    case USB_AMAGE_STOP:
        usb_amage_stop();
        break;
    case USB_AMAGE_DETECT_REMOVE:
        usb_amage_switch_mode();
        break;
    case USB_AMAGE_DETECT_CONNECT:
        usb_amage_usb_connect();
        break;
    case USB_AMAGE_CHANGE_DISPLAY:
        usb_amage_change_display();
        break;
    case USB_AMAGE_CHANGE_ENC_MODE:
        usb_amage_change_enc_mode();
        break;
    case USB_AMAGE_UPDATE_FCHAN_VOUT:
        usb_amage_update_fchan_vout(param1);
        break;
    case USB_AMAGE_STILL_CAPTURE:
        usb_amage_still_capture();
        break;
    case USB_AMAGE_MUXER_END:
        usb_amage_muxer_end();
        break;
    case USB_AMAGE_BG_PROCESS_DONE:
        usb_amage_bg_process_done();
    default:
        break;
    }

    return ReturnValue;
}
