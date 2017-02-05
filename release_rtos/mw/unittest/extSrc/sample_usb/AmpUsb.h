/**
 * @file AmpUsb.h
 *
 * Header of USB
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef _AMPUSB_H_
#define _AMPUSB_H_

#include "AmbaPrint.h"
#include "AmbaPrintk.h"


typedef enum _AMP_USB_DEVICE_CLASS_e_ {
    AMP_USB_DEVICE_CLASS_NONE = 0,
    AMP_USB_DEVICE_CLASS_MSC,      /* mass storage class */
    AMP_USB_DEVICE_CLASS_MTP,      /* mtp class */
    AMP_USB_NUM_DEVICE_CLASS
} AMP_USB_DEVICE_CLASS_e;


/**
 *  @brief Initialization USB module
 *
 *  Initialization USB module
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 AmpUSB_Init(void);

/**
 *  @brief Initialization USB jack
 *
 *  Initialization USB jack
 *
 *  @return >=0 success, <0 failure
 */
extern int AmpUSB_InitJack(void);

/**
 *  @brief Initialization USB MSC mount
 *
 *  Initialization USB MSC mount
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 AmpUsbMsc_MountInit(void);

/**
 *  @brief Initialization USB MSC DoMount
 *
 *  Initialization USB MSC DoMount
 *
 *
 *  @return >=0 success, <0 failure
 */
extern int AmpUsbMsc_DoMountInit(void);

/**
 *  @brief USB MSC Mount drive
 *
 *  USB MSC Mount drive
 *
 *  @param [in] slot
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 AmpUsbMsc_DoMount(UINT32 slot);

/**
 *  @brief USB MSC Do Un Mount Drive
 *
 *  USB MSC Do Un Mount Drive
 *
 *  @param [in] slot
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 AmpUsbMsc_DoUnMount(UINT32 slot);

/**
 *  @brief Initialization USB MSC Start
 *
 *  Initialization USB MSC Start
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 AmpUsbMsc_Start(void);

/**
 *  @brief Initialization USB MSC Stop
 *
 *  Initialization USB MSC Stop
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 AmpUsbMsc_Stop(void);

/**
 *  @brief Initialization USB AMAGE start
 *
 *  Initialization USB AMAGE start
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 AmpUsbAmage_Start(void);

/**
 *  @brief Init USB custom device info
 *
 *  Init USB custom device info
 *
 *  @param [in] class Usb class
 *
 *  @return >=0 success, <0 failure
 */
extern UINT32 AmpUSB_Custom_SetDevInfo(AMP_USB_DEVICE_CLASS_e class);

extern AMBA_KAL_BYTE_POOL_t G_MMPL;
extern AMBA_KAL_BYTE_POOL_t G_NC_MMPL;



#endif /* _AMPUSB_H_ */
