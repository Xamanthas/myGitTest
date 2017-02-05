/**
  * @file src/app/apps/flow/usb/connectedcam/usb_msc_func.c
  *
  * Functions of USB MSC class
  *
  * History:
  *    2013/12/02 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */


#include <apps/flow/usb/usb_msc.h>
#include <system/status.h>
#include <system/app_pref.h>
#include <system/app_util.h>


#define VALID_SLOT_IN_MSC    0x0000000F

static int usb_msc_mount_devices(void)
{
    int ReturnValue = 0;
    AMBA_SCM_STATUS_s info;

    /** Mount SD0 slot if it exists */
    if (APP_CHECKFLAGS(VALID_SLOT_IN_MSC, (0x1<<SCM_SLOT_SD0))) {
        ReturnValue = AmbaSCM_GetSlotStatus(SCM_SLOT_SD0, &info);
        if (ReturnValue != -1) {
            ApplibUsbMsc_DoMount(SCM_SLOT_SD0);
        }
    }

    /** Mount SD1 slot if it exists */
    if (APP_CHECKFLAGS(VALID_SLOT_IN_MSC, (0x1<<SCM_SLOT_SD1))) {
        ReturnValue = AmbaSCM_GetSlotStatus(SCM_SLOT_SD1, &info);
        if (ReturnValue != -1) {
            ApplibUsbMsc_DoMount(SCM_SLOT_SD1);
        }
    }

    /** Mount FL0 slot if it exists */
    if (APP_CHECKFLAGS(VALID_SLOT_IN_MSC, (0x1<<SCM_SLOT_FL0))) {
        if (AppLibCard_CheckNandStorage(AppLibCard_GetCardId(SCM_SLOT_FL0))) {
            ReturnValue = AmbaSCM_GetSlotStatus(SCM_SLOT_FL0, &info);
            if (ReturnValue != -1) {
                ApplibUsbMsc_DoMount(SCM_SLOT_FL0);
            }
        }
    }

    /** Mount FL1 slot if it exists **/
    if (APP_CHECKFLAGS(VALID_SLOT_IN_MSC, (0x1<<SCM_SLOT_FL1))) {
        if (AppLibCard_CheckNandStorage(AppLibCard_GetCardId(SCM_SLOT_FL1))) {
            ReturnValue = AmbaSCM_GetSlotStatus(SCM_SLOT_FL1, &info);
            if (ReturnValue != -1) {
                ApplibUsbMsc_DoMount(SCM_SLOT_FL1);
            }
        }
    }

    app_status.UsbStorageUnmount = 1;

    return ReturnValue;
}

static int usb_msc_unmount_devices(void)
{
    int ReturnValue = 0;

    ApplibUsbMsc_DoUnMount(SCM_SLOT_SD0);
    ApplibUsbMsc_DoUnMount(SCM_SLOT_SD1);
    ApplibUsbMsc_DoUnMount(SCM_SLOT_FL0);
    ApplibUsbMsc_DoUnMount(SCM_SLOT_FL1);

    AppUtil_PollingAllSlots();

    return ReturnValue;
}

static int usb_msc_start(void)
{
    int ReturnValue = 0;

    AppLibUSB_Init();
    /**Set data connection = 0 before remount device*/
    AmbaUSB_System_SetDeviceDataConn(0);
    /* Initialize the USB MSC. */
    ApplibUsbMsc_DoMountInit();
    /* Unmount the storage. */
    AppLibCard_Remove(AppLibCard_GetActiveSlot());
    /* Mount the storage on USB. */
    usb_msc_mount_devices();
    /* Start the USB class. */
    ApplibUsbMsc_Start();
    /* Init decoder. Decode black screen rather than use background source. */
    AppLibThmBasic_Init();

    return ReturnValue;
}

static int usb_msc_stop(void)
{
    int ReturnValue = 0;

    /* Deinit decoder. */
    AppLibThmBasic_Deinit();

    /* Stop the USB class. */
    ApplibUsbMsc_Stop();
    /* Unmount the storage. */
    usb_msc_unmount_devices();
    app_status.UsbStorageUnmount = 0;

    AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
    AppLibDisp_DeactivateWindow(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0));
    AppLibDisp_ChanStop(DISP_CH_FCHAN);
    /* Hide GUI */
    usb_msc.Gui(GUI_HIDE_ALL, 0, 0);
    usb_msc.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}


static int usb_msc_app_ready(void)
{
    int ReturnValue = 0;

    if (!APP_CHECKFLAGS(app_usb_msc.GFlags, APP_AFLAGS_READY)) {
        APP_ADDFLAGS(app_usb_msc.GFlags, APP_AFLAGS_READY);

        // ToDo: need to remove to handler when iav completes the dsp cmd queue mechanism
        AppLibGraph_Init();
        usb_msc.Func(USB_MSC_CHANGE_OSD, 0, 0);

        AppUtil_ReadyCheck(0);
        if (!APP_CHECKFLAGS(app_usb_msc.GFlags, APP_AFLAGS_READY)) {
            /* The system could switch the current app to other in the function "AppUtil_ReadyCheck". */
            return ReturnValue;
        }
    }

    /* Clean vout buffer */
    AppLibThmBasic_ClearScreen();
    usb_msc.Func(USB_MSC_GUI_INIT_SHOW, 0, 0);

    return ReturnValue;
}

static int usb_msc_detect_remove(void)
{
    int ReturnValue = 0;

    /** Switch to decode mode if the Fchan will be enabled and the app_status.jack_to_pb_mode is on.*/
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

    return ReturnValue;
}

static int usb_msc_update_fchan_vout(UINT32 msg)
{
    int ReturnValue = 0;

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
        AmbaPrint("[app_usb_msc] Vout no changed");
        return 0;
        break;
    }
    ReturnValue = AppLibDisp_SelectDevice(DISP_CH_FCHAN, DISP_ANY_DEV);
    if (APP_CHECKFLAGS(ReturnValue, DISP_FCHAN_NO_CHANGE)) {
        AmbaPrint("[app_usb_msc] Display FCHAN has no changed");
    } else {
        if (APP_CHECKFLAGS(ReturnValue, DISP_FCHAN_NO_DEVICE)) {
            AppLibGraph_DisableDraw(GRAPH_CH_FCHAN);
            AppLibDisp_ChanStop(DISP_CH_FCHAN);
            AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
            AppLibDisp_FlushWindow(DISP_CH_FCHAN);
            app_status.LockDecMode = 0;
        } else {
            AppLibDisp_ConfigMode(DISP_CH_FCHAN, AppLibSysVout_GetVoutMode(VOUT_DISP_MODE_1080P));
            AppLibDisp_SetupChan(DISP_CH_FCHAN);
            AppLibDisp_ChanStart(DISP_CH_FCHAN);
            {
                AMP_DISP_WINDOW_CFG_s Window;
                AMP_DISP_INFO_s DispDev = {0};

                memset(&Window, 0, sizeof(AMP_DISP_WINDOW_CFG_s));

                ReturnValue = AppLibDisp_GetDeviceInfo(DISP_CH_FCHAN, &DispDev);
                if ((ReturnValue < 0) || (DispDev.DeviceInfo.Enable == 0)) {
                    DBGMSG("[app_usb_msc] FChan Disable. Disable the fchan Window");
                    AppLibGraph_DisableDraw(GRAPH_CH_FCHAN);
                    AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
                    AppLibDisp_FlushWindow(DISP_CH_FCHAN);
                    app_status.LockDecMode = 0;
                } else {
                    /** FCHAN Window*/
                    AppLibGraph_EnableDraw(GRAPH_CH_FCHAN);
                    AppLibDisp_GetWindowConfig(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0), &Window);
                    Window.Source = AMP_DISP_DEC;
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
                    AppLibDisp_FlushWindow(DISP_CH_FCHAN);
                }
            }
            AppLibGraph_SetWindowConfig(GRAPH_CH_FCHAN);
            AppLibGraph_ActivateWindow(GRAPH_CH_FCHAN);
            AppLibGraph_FlushWindow(GRAPH_CH_FCHAN);
            usb_msc.Gui(GUI_SET_LAYOUT, 0, 0);
            usb_msc.Gui(GUI_FLUSH, 0, 0);
        }
    }

    return ReturnValue;
}

static int usb_msc_change_display(void)
{
    int ReturnValue = 0;

    AppLibDisp_SelectDevice(DISP_CH_FCHAN | DISP_CH_DCHAN, DISP_ANY_DEV);
    AppLibDisp_ConfigMode(DISP_CH_FCHAN | DISP_CH_DCHAN, AppLibSysVout_GetVoutMode(VOUT_DISP_MODE_1080P));
    AppLibDisp_SetupChan(DISP_CH_FCHAN | DISP_CH_DCHAN);
    AppLibDisp_ChanStart(DISP_CH_FCHAN | DISP_CH_DCHAN);
    {
        AMP_DISP_WINDOW_CFG_s Window;
        AMP_DISP_INFO_s DispDev = {0};

        memset(&Window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));

        ReturnValue = AppLibDisp_GetDeviceInfo(DISP_CH_FCHAN, &DispDev);
        if ((ReturnValue < 0) || (DispDev.DeviceInfo.Enable == 0)) {
            DBGMSG("[app_usb_msc] FChan Disable. Disable the fchan Window");
            AppLibDisp_DeactivateWindow(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0));
            AppLibGraph_DisableDraw(GRAPH_CH_FCHAN);
        } else {
            /** FCHAN Window*/
            AppLibDisp_GetWindowConfig(DISP_CH_FCHAN, AppLibDisp_GetWindowId(DISP_CH_FCHAN, 0), &Window);
            Window.Source = AMP_DISP_BACKGROUND_COLOR;
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
            AppLibGraph_EnableDraw(GRAPH_CH_FCHAN);
        }

        ReturnValue = AppLibDisp_GetDeviceInfo(DISP_CH_DCHAN, &DispDev);
        if ((ReturnValue < 0) || (DispDev.DeviceInfo.Enable == 0)) {
            DBGMSG("[app_usb_msc] DChan Disable. Disable the Dchan Window");
            AppLibDisp_DeactivateWindow(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0));
            AppLibGraph_DisableDraw(GRAPH_CH_DCHAN);
        } else {
            /** DCHAN Window*/
            AppLibDisp_GetWindowConfig(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0), &Window);
            Window.Source = AMP_DISP_BACKGROUND_COLOR;
            Window.SourceDesc.Dec.DecHdlr = 0;
            Window.CropArea.Width = 0;
            Window.CropArea.Height = 0;
            Window.CropArea.X = 0;
            Window.CropArea.Y = 0;
            Window.TargetAreaOnPlane.Width = DispDev.DeviceInfo.VoutWidth;
            Window.TargetAreaOnPlane.Height = DispDev.DeviceInfo.VoutHeight;
            Window.TargetAreaOnPlane.X = 0;
            Window.TargetAreaOnPlane.Y = 0;
            AppLibDisp_SetWindowConfig(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0), &Window);
            AppLibDisp_ActivateWindow(DISP_CH_DCHAN, AppLibDisp_GetWindowId(DISP_CH_DCHAN, 0));
            AppLibGraph_EnableDraw(GRAPH_CH_DCHAN);
        }
        AppLibDisp_FlushWindow(DISP_CH_FCHAN | DISP_CH_DCHAN);
    }

    return ReturnValue;
}

static int usb_msc_change_osd(void)
{
    int ReturnValue = 0;

    AppLibGraph_SetWindowConfig(GRAPH_CH_FCHAN | GRAPH_CH_DCHAN);
    AppLibGraph_ActivateWindow(GRAPH_CH_FCHAN | GRAPH_CH_DCHAN);
    AppLibGraph_FlushWindow(GRAPH_CH_FCHAN | GRAPH_CH_DCHAN);
    usb_msc.Gui(GUI_SET_LAYOUT, 0, 0);
    usb_msc.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int usb_msc_start_show_gui(void)
{
    int ReturnValue = 0;
    usb_msc.Gui(GUI_APP_ICON_SHOW, 0, 0);
    usb_msc.Gui(GUI_FLUSH, 0, 0);
    return ReturnValue;
}

static int usb_msc_card_insert(void)
{
    int ReturnValue = 0;
    /* Mount the storage on USB. */
    usb_msc_mount_devices();
    return ReturnValue;
}

static int usb_msc_card_remove(void)
{
    int ReturnValue = 0;
    /* Unmount the storage. */
    usb_msc_unmount_devices();
    return ReturnValue;
}

/**
 *  @brief The functions of video playback application
 *
 *  The functions of video playback application
 *
 *  @param[in] funcId Function id
 *  @param[in] param1 first parameter
 *  @param[in] param2 second parameter
 *
 *  @return >=0 success, <0 failure
 */
int usb_msc_func(UINT32 funcId, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;

    switch (funcId) {
    case USB_MSC_INIT:
        break;
    case USB_MSC_START:
        usb_msc_start();
        break;
    case USB_MSC_STOP:
        usb_msc_stop();
        break;
    case USB_MSC_APP_READY:
        usb_msc_app_ready();
        break;
    case USB_MSC_DETECT_REMOVE:
        usb_msc_detect_remove();
        break;
    case USB_MSC_UPDATE_FCHAN_VOUT:
        usb_msc_update_fchan_vout(param1);
        break;
    case USB_MSC_CHANGE_DISPLAY:
        usb_msc_change_display();
        break;
    case USB_MSC_CHANGE_OSD:
        usb_msc_change_osd();
        break;
    case USB_MSC_GUI_INIT_SHOW:
        usb_msc_start_show_gui();
        break;
    case USB_MSC_CARD_INSERT:
        usb_msc_card_insert();
        break;
    case USB_MSC_CARD_REMOVE:
        usb_msc_card_remove();
        break;
    default:
        break;
    }

    return ReturnValue;
}
