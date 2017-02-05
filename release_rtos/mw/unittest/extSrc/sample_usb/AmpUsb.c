/**
 * @file AmpUsb.c
 *
 *  USB gerenal API.
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <AmbaDataType.h>
#include <AmbaPrintk.h>
#include <usb/AmbaUSB_API.h>
#include <usb/AmbaUSB_Host_API.h>
#include "AmpUsb.h"
#include "util.h"

//#define DEBUG_AMP_USB
#if defined(DEBUG_AMP_USB)
#define DBGMSG AmbaPrint
#define DBGMSGc(x) AmbaPrintColor(GREEN,x)
#define DBGMSGc2 AmbaPrintColor
#else
#define DBGMSG(...)
#define DBGMSGc(...)
#define DBGMSGc2(...)
#endif

#define AMP_USB_MEM_BUF_SIZE       (640*1024)
UINT32 MspVbusConnect = 0;

static UINT32 CurClass = AMP_USB_DEVICE_CLASS_NONE;


/**
 *  @brief Callback function about USB connected
 *
 *  Callback function about USB connected
 */
static void AmpUSB_VbusConnect(void)
{
    DBGMSGc2(GREEN,"[Amp Usb] Vbus Connect!!");
    MspVbusConnect = 1;
}

/**
 *  @brief Callback function about USB disconnected
 *
 *  Callback function about USB disconnected
 */
static void AmpUSB_VbusDisConnect(void)
{
    DBGMSGc2(GREEN,"[Amp Usb] Vbus Disconnect!!");
    MspVbusConnect = 0;
}

static void AmpUSB_DeviceSystemInit(void)
{
    int ReturnValue = 0;
    /* Initialization of USB Class*/
    USB_SYSTEM_INIT_s Sysconfig = {0};
    UINT8 *Buf = NULL, *BufRaw = NULL;

    ReturnValue = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&Buf, (void **)&BufRaw, AMP_USB_MEM_BUF_SIZE, 32);
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp USB] <Init> %s:%u", __FUNCTION__, __LINE__);
    }
    Sysconfig.MemPoolPtr = Buf;
    Sysconfig.TotalMemSize = AMP_USB_MEM_BUF_SIZE;
    ReturnValue = AmbaUSB_System_DeviceSystemSetup(&Sysconfig);
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb] AmbaUSB_System_Init fail");
    }
}

static void AmpUSB_DeviceSystemStart(void)
{
    AmbaPrint("[Amp Usb] DeviceSystemStart");
    AmpUSB_DeviceSystemInit();
}

static void AmpUSB_DeviceSystemRelease(void)
{
    AmbaPrint("[Amp Usb] DeviceSystemRelease");
//    AmbaUSB_DeviceSystemDestroy();
//    if (AmbaUSB_ClassCtrl[AmbaUsbDeviceClass].RelFunc) {
//        AmbaUSB_ClassCtrl[AmbaUsbDeviceClass].RelFunc();
//    }
}

static void AmpUSB_DeviceSystemConfigured(UINT16 index)
{
    AmbaPrint("[Amp Usb] Device System Configured, Index = %d", index);
}

static void AmpUSB_DeviceSystemSuspended(void)
{
    AmbaPrint("[Amp Usb] USB Device Suspended");
}

static void AmpUSB_DeviceSystemResumed(void)
{
    AmbaPrint("[Amp Usb] USB Device Resumed");
}

static void AmpUSB_DeviceSystemReset(void)
{
    AmbaPrint("[Amp Usb] USB Device Reset");
}

USB_DEV_VBUS_CB_s UsbVbusCb = {
    AmpUSB_VbusConnect,
    AmpUSB_VbusDisConnect,
    AmpUSB_DeviceSystemStart,
    AmpUSB_DeviceSystemRelease,
    AmpUSB_DeviceSystemConfigured,
    AmpUSB_DeviceSystemSuspended,
    AmpUSB_DeviceSystemResumed,
    AmpUSB_DeviceSystemReset
};


/**
 *  @brief Register the callback function about USB connected
 *
 *  Register the callback function about USB connected
 *
 *  @return >=0 success, <0 failure
 */
static int USB_RegisterVbusCallback(void)
{
    return AmbaUSB_System_RegisterVbusCallback(&UsbVbusCb);
}

static int AmpUsbInit = -1;

/**
 *  @brief Initialization USB module
 *
 *  Initialization USB module
 *
 *  @return >=0 success, <0 failure
 */
int AmpUSB_Init(void)
{
    int ReturnValue = 0;
    if (AmpUsbInit == 0) {
        return 0;
    }

    DBGMSGc2(GREEN,"[Amp Usb] AmpUSB_Init start.");

    /* Initialization of USB Module. */
    AmbaUSB_System_SetMemoryPool(&G_MMPL, &G_NC_MMPL);

    /* Swicth the IRQ owner to RTOS. */
    AmbaUSB_System_SetUsbOwner(USB_IRQ_RTOS, 0);

    /* Switch Usb Phy0 owner to device. */
    AmbaUSB_Host_Init_SwitchUsbOwner(UDC_OWN_PORT);

    // Disconnect USB till USB class is hooked.
    // Otherwise host would find unknown device.
    AmbaUSB_System_SetDeviceDataConn(0);

    DBGMSGc2(GREEN,"[Amp Usb] AmpUSB_Init done.");
    AmpUsbInit = 0;

    return ReturnValue;
}

/**
 *  @brief Initialization USB jack
 *
 *  Initialization USB jack
 *
 *  @return >=0 success, <0 failure
 */
int AmpUSB_InitJack(void)
{
    int ReturnValue = 0;

    ReturnValue = USB_RegisterVbusCallback();
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb] USB_RegisterVbusCallback fail");
    }

    return ReturnValue;
}



/**
 *  @brief Simulate USB cable plug/unplug.
 *
 *  Simulate USB cable plug/unplug.
 *
 *  @param [in] enable Enable
 *
 *  @return >=0 success, <0 failure
 */
int AmpUSB_SetConnection(UINT32 enable)
{
    AmbaUSB_System_SetConnect(enable);
    return 0;
}

/**
 *  @brief Get current active USB class
 *
 *  Get current active USB class
 *
 *  @return Class
 */
int AmpUSB_GetCurClass(void)
{
    return CurClass;
}

