/**
 * @file AmpUsb_Msc.c
 *
 *  USB Mass Storage Class functions.
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <AmbaDataType.h>
#include <usb/AmbaUSB_API.h>
#include <AmbaCardManager.h>
#include "AmpUsb.h"

//#define DEBUG_AMP_USB_MSC
#if defined(DEBUG_AMP_USB_MSC)
#define DBGMSG AmbaPrint
#define DBGMSGc(x) AmbaPrintColor(GREEN,x)
#define DBGMSGc2 AmbaPrintColor
#else
#define DBGMSG(...)
#define DBGMSGc(...)
#define DBGMSGc2(...)
#endif

/*-----------------------------------------------------------------------------------------------*\
 * Define MSC FS structure.
\*-----------------------------------------------------------------------------------------------*/

/* Define Storage Class USB MEDIA types.  */
#define MSC_MEDIA_FAT_DISK                       0
#define MSC_MEDIA_CDROM                          5
#define MSC_MEDIA_OPTICAL_DISK                   7
#define MSC_MEDIA_IOMEGA_CLICK                   0x55

/*-----------------------------------------------------------------------------------------------*\
 * Global Variables/Functions
\*-----------------------------------------------------------------------------------------------*/

static UINT8   AmpStorageVendorId[]="Ambarella";               // 8 Bytes
static UINT8   AmpStorageProductId[]="A12 Platform";   // 16 Bytes
static UINT8   AmpStorageProductVer[]="1000";                 // 4 Bytes

static MSC_FSLIB_OP_s MscFsOp = {
    AmbaMSC_Read,
    AmbaMSC_Write,
    AmbaSCM_GetSlotStatus
};


int AmpUsbMsc_ClassInit(void)
{
    int ReturnValue = 0;
    DBGMSGc2(GREEN,"[Amp Usb MSC] AmpUsbMsc_DoMount ");
    /* Register File system Operation.  */
    ReturnValue = AmbaUSB_Class_Msc_Init(&MscFsOp);
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb MSC] AmbaUSB_Class_Msc_Init fail");
    }
    return ReturnValue;
}

/**
 *  @brief Mount drive.
 *
 *  Mount drive.
 *
 *  @return >=0 success, <0 failure
 */
#if 1
int AmpUsbMsc_DoMount_test(void)
{
    int ReturnValue = 0;
    DBGMSGc2(GREEN,"[Amp Usb MSC] AmpUsbMsc_DoMount ");
    AmpUsbMsc_ClassInit();
    /* Mount SCM_SLOT_SD00 */
    ReturnValue = AmbaUSB_Class_Msc_SetInfo(AmpStorageVendorId, AmpStorageProductId, AmpStorageProductVer);
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb MSC] AmbaUSB_Class_Msc_SetInfo fail");
    }
    ReturnValue = AmbaUSB_Class_Msc_SetProp(SCM_SLOT_SD0, 0x80, 0, MSC_MEDIA_FAT_DISK);
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb MSC] AmbaUSB_Class_Msc_SetProp fail");
    }
    ReturnValue = AmbaUSB_Class_Msc_Mount(SCM_SLOT_SD0);
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb MSC] AmbaUSB_Class_Msc_Mount fail");
    }
    return ReturnValue;
}
#endif
int AmpUsbMsc_DoMountInit(void)
{
    int ReturnValue = 0;
    DBGMSGc2(GREEN,"[Amp Usb MSC] AmpUsbMsc_DoMount ");
    AmpUsbMsc_ClassInit();

    ReturnValue = AmbaUSB_Class_Msc_SetInfo(AmpStorageVendorId, AmpStorageProductId, AmpStorageProductVer);
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb MSC] AmbaUSB_Class_Msc_SetInfo fail");
    }

    return ReturnValue;
}



/**
 *  @brief UnMount drive.
 *
 *  UnMount drive.
 *
 *  @return >=0 success, <0 failure
 */
INT32 AmpUsbMsc_DoMount(UINT32 slot)
{
    int ReturnValue = 0;
    DBGMSGc2(GREEN,"[Amp Usb MSC] AmpUsbMsc_UnMount ");

    ReturnValue = AmbaUSB_Class_Msc_SetProp(slot, 0x80, 0, MSC_MEDIA_FAT_DISK);
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb MSC] AmbaUSB_Class_Msc_SetProp fail");
    }

    ReturnValue = AmbaUSB_Class_Msc_Mount(slot);

    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb MSC] AmbaUSB_Class_Msc_Mount fail Slot = %d", slot);
    }

    return ReturnValue;
}

/**
 *  @brief UnMount drive.
 *
 *  UnMount drive.
 *
 *  @return >=0 success, <0 failure
 */
INT32 AmpUsbMsc_DoUnMount(UINT32 slot)
{
    int ReturnValue = 0;
    DBGMSGc2(GREEN,"[Amp Usb MSC] AmpUsbMsc_UnMount ");

    ReturnValue = AmbaUSB_Class_Msc_UnMount(slot);

    return ReturnValue;
}

#define AMP_USB_MSC_TASK_STACK_SIZE  (8 * 1024)
#define AMP_USB_MSC_TASK_STACK_PRIORITY  (70)

int AmpUsbMsc_Start(void)
{
    int ReturnValue = 0;
    USB_CLASS_INIT_s ClassConfig = {UDC_CLASS_NONE, 0, 0};

    DBGMSGc2(GREEN,"[Amp Usb MSC] Start ");

    /* Init USB custom device info.*/
    ReturnValue = AmpUSB_Custom_SetDevInfo(AMP_USB_DEVICE_CLASS_MSC);
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb MSC] AmpUSB_Custom_SetDevInfo fail");
    }

    /* Hook USB Class*/
    ClassConfig.classID = UDC_CLASS_MSC;
    ClassConfig.ClassTaskPriority = AMP_USB_MSC_TASK_STACK_PRIORITY;
    ClassConfig.ClassTaskStackSize = AMP_USB_MSC_TASK_STACK_SIZE;
    ReturnValue = AmbaUSB_System_ClassHook(&ClassConfig);
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb MSC] AmbaUSB_System_ClassHook fail");
    }

    // pollo - 2014/06/16 - Fix issue: [Jira][Amba168] USB: support for delayed initialization, Item 2:
    // Application will not initialize the USB class immediately after the vbus detect.
    // It will wait for switching to MTP mode.
    // The lower layer should keep the host happy till the application initializes the USB class.
    // 1. Add AmbaUSB_System_SetDeviceDataConn() for Applications to enable/disable USB device data connect.
    // 2. Add AmbaUSB_System_SetDeviceDataConnWithVbus() for Applications to setup USB device data connect status when VBUS is detected.
    //    1: Data connect will be enabled when VBUS is detected. PC will recognize it immediately.
    //    0: Data connect will NOT be enabled when VBUS is detected. PC will not recognize it until AmbaUSB_System_SetDeviceDataConn(1) is called.
    AmbaUSB_System_SetDeviceDataConn(1);

    return ReturnValue;
}

int AmpUsbMsc_Stop(void)
{
    int ReturnValue = 0;
    USB_CLASS_INIT_s ClassConfig = {UDC_CLASS_NONE, 0, 0};

    DBGMSGc2(GREEN,"[Amp Usb MSC] Stop ");
    ClassConfig.classID = UDC_CLASS_MSC;
    ClassConfig.ClassTaskPriority = AMP_USB_MSC_TASK_STACK_PRIORITY;
    ClassConfig.ClassTaskStackSize = AMP_USB_MSC_TASK_STACK_SIZE;
    AmbaUSB_System_ClassUnHook(&ClassConfig);
    if (ReturnValue < 0) {
        AmbaPrintColor(RED,"[Amp Usb MSC] AmbaUSB_System_ClassHook fail");
    }

    return ReturnValue;
}

