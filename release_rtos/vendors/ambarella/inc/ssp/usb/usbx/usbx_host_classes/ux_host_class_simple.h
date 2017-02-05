/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: ux_host_class_simle.h
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: USB Host Simple Class headers.
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef UX_HOST_CLASS_SIMPLE_H
#define UX_HOST_CLASS_SIMPLE_H


/* Include the FileX API or the FileX stub API in cases where FileX is not
   available.  */


/* Define User configurable simple Class constants.  */

#ifndef UX_MAX_HOST_LUN
#define UX_MAX_HOST_LUN                                     16
#endif

#define USB_DEV_PRODUCT_ID_SIMPLE               0x0008  /* product id      */
#define USB_DEV_VENDOR_ID_SIMPLE                0x4055  /* vendor id            */

#ifndef UX_HOST_CLASS_SIMPLE_THREAD_STACK_SIZE
#define UX_HOST_CLASS_SIMPLE_THREAD_STACK_SIZE             UX_THREAD_STACK_SIZE
#endif

#define UX_HOST_CLASS_SIMPLE_THREAD_PRIORITY_CLASS         70

#define UX_HOST_CLASS_SIMPLE_THREAD_SLEEP_TIME             (10 * UX_PERIODIC_RATE)
#define UX_HOST_CLASS_SIMPLE_TRANSFER_TIMEOUT              100

#define UX_HOST_CLASS_SIMPLE_MAX_TRANSFER_SIZE             (1024 * 512)

typedef struct UX_HOST_CLASS_SIMPLE_STRUCT
{
    struct UX_HOST_CLASS_SIMPLE_STRUCT
                    *ux_host_class_simple_next_instance;
    UX_HOST_CLASS   *ux_host_class_simple_class;
    UX_DEVICE       *ux_host_class_simple_device;
    UX_INTERFACE    *ux_host_class_simple_interface;
    UX_ENDPOINT     *ux_host_class_simple_bulk_in_endpoint;
    UX_ENDPOINT     *ux_host_class_simple_bulk_out_endpoint;
    UINT            ux_host_class_simple_state;
    UINT            (*ux_host_class_simple_transport) (struct UX_HOST_CLASS_SIMPLE_STRUCT *simple, UCHAR * data_pointer);
    UCHAR *         ux_host_class_simple_memory;
    TX_SEMAPHORE    ux_host_class_simple_semaphore;
} UX_HOST_CLASS_SIMPLE;

UINT  _ux_host_class_simple_entry(UX_HOST_CLASS_COMMAND *command);
VOID  _ux_host_class_simple_thread_entry(ULONG class_address);
UINT  _ux_host_class_simple_configure(UX_HOST_CLASS_SIMPLE *simple);
UINT  _ux_host_class_simple_endpoints_get(UX_HOST_CLASS_SIMPLE *simple);
UINT  _ux_host_class_simple_activate(UX_HOST_CLASS_COMMAND *command);
UINT  _ux_host_class_simple_deactivate(UX_HOST_CLASS_COMMAND *command);
UINT  _ux_host_class_simple_control_request(UX_HOST_CLASS_SIMPLE *simple, ULONG request, ULONG type, ULONG value, ULONG index, UCHAR *buf, ULONG length);
UINT  _ux_host_class_simple_bulk_read(UX_HOST_CLASS_SIMPLE *simple, ULONG ep_addr, UCHAR *buf, ULONG length, ULONG *actual_len, ULONG timeout);
UINT  _ux_host_class_simple_bulk_write(UX_HOST_CLASS_SIMPLE *simple, ULONG ep_addr, UCHAR *buf, ULONG length, ULONG *actual_len, ULONG timeout);
// 2014/06/06 - remove compile warnings.
// It's not perfect to use external function in APP layer from driver.
// But at least it works.
extern UINT  AmbaUSB_Debug_WriteData(UINT Data0, UINT Data1, UINT Data2, UINT Data3, UINT Data4);

#endif


