/**
 * @file src/app/connected/applib/inc/usb/ApplibUsb.h
 *
 * Header of USB
 *
 * History:
 *    2013/11/29 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_USB_H_
#define APPLIB_USB_H_
/**
 * @defgroup USB
 * @brief USB related function implementation
 *
 * Implementation of
 * USB related function
 *
 */

/**
* @defgroup ApplibUsb
* @brief USB utility
*
*
*/

/**
 * @addtogroup ApplibUsb
 * @ingroup USB
 * @{
 */
#include <applib.h>
#include <AmbaUART.h>

__BEGIN_C_PROTO__

typedef enum _APPLIB_USB_DEVICE_CLASS_e_ {
    APPLIB_USB_DEVICE_CLASS_NONE = 0,
    APPLIB_USB_DEVICE_CLASS_MSC,      /* mass storage class */
    APPLIB_USB_DEVICE_CLASS_MTP,      /* mtp class */
    APPLIB_USB_DEVICE_CLASS_CDC_ACM_MULTI, /* cdc-acm class */
    APPLIB_USB_NUM_DEVICE_CLASS
} APPLIB_USB_DEVICE_CLASS_e;

typedef void (*CdcAcmNotify)(int running);


/**
 *  @brief Initialization USB module
 *
 *  Initialization USB module
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 AppLibUSB_Init(void);

/**
 *  @brief Initialization USB jack
 *
 *  Initialization USB jack
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibUSB_InitJack(void);

/**
 *  @brief Initialization USB MSC mount
 *
 *  Initialization USB MSC mount
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 ApplibUsbMsc_MountInit(void);

/**
 *  @brief Initialization USB MSC DoMount
 *
 *  Initialization USB MSC DoMount
 *
 *
 *  @return >=0 success, <0 failure
 */
extern int ApplibUsbMsc_DoMountInit(void);

/**
 *  @brief USB MSC Mount drive
 *
 *  USB MSC Mount drive
 *
 *  @param [in] slot
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 ApplibUsbMsc_DoMount(UINT32 slot);

/**
 *  @brief USB MSC Do Un Mount Drive
 *
 *  USB MSC Do Un Mount Drive
 *
 *  @param [in] slot
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 ApplibUsbMsc_DoUnMount(UINT32 slot);

/**
 *  @brief Initialization USB MSC Start
 *
 *  Initialization USB MSC Start
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 ApplibUsbMsc_Start(void);

/**
 *  @brief Initialization USB MSC Stop
 *
 *  Initialization USB MSC Stop
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 ApplibUsbMsc_Stop(void);

/**
 *  @brief Initialization USB AMAGE start
 *
 *  Initialization USB AMAGE start
 *
 *  @return >=0 success, <0 failure
 */
extern INT32 ApplibUsbAmage_Start(void);

/**
 *  @brief Init USB custom device info
 *
 *  Init USB custom device info
 *
 *  @param [in] class Usb class
 *
 *  @return >=0 success, <0 failure
 */
extern UINT32 AppLibUSB_Custom_SetDevInfo(APPLIB_USB_DEVICE_CLASS_e class);

/*-----------------------------------------------------------------------------------------------*\
 * Defined in ApplibUsb_CdcAcmMulti.c
\*-----------------------------------------------------------------------------------------------*/
/**
 *  @brief USB CDC-ACM milti start
 *
 *  USB CDC-ACM milti start
 *
 *  @return >=0 success, <0 failure
 */
extern int ApplibUsbCdcAcmMulti_Start(void);

/**
 *  @brief USB CDC-ACM milti stop
 *
 *  USB CDC-ACM milti stop
 *
 *  @return >=0 success, <0 failure
 */
extern int ApplibUsbCdcAcmMulti_Stop(void);

/**
 *  @brief USB CDC-ACM milti write
 *
 *  USB CDC-ACM milti write
 *
 *  @return bytes are writen
 */
extern int ApplibUsbCdcAcmMulti_Write(AMBA_UART_CHANNEL_e UartChanNo, int StringSize, char *StringBuf, UINT32 TimeOut);

/**
 *  @brief USB CDC-ACM milti read
 *
 *  USB CDC-ACM milti read
 *
 *  @return bytes are read
 */
extern int ApplibUsbCdcAcmMulti_Read(AMBA_UART_CHANNEL_e UartChanNo, int StringSize, char *StringBuf, UINT32 TimeOut);


__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_USB_H_ */
