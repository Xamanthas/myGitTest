/**
 * @file AmpUsb_Custom.c
 *
 *  USB custom device info
 *
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


// define MTP VENDOR REQUEST
#define MTP_VENDOR_REQUEST         0x54

/*-----------------------------------------------------------------------------------------------*\
   Multiple languages are supported on the device, to add
   a language besides English, the unicode language code must
   be appended to the LanguageIdFramework array and the length
   adjusted accordingly.
\*-----------------------------------------------------------------------------------------------*/

static UINT8 LangID[] = {

    /* English. */
    0x09, 0x04
};

/*-----------------------------------------------------------------------------------------------*\
 * Mass Storage Class Default Descriptor
\*-----------------------------------------------------------------------------------------------*/

static UINT8 __attribute__((aligned(32))) MscDescFs[] = {

    /* Device descriptor */
    0x12,       // this descriptor size
    0x01,       // device descriptor type
    0x10, 0x01, // Spec version
    0x00,       // class code
    0x00,       // subclass code
    0x00,       // procotol code
    0x40,       // max packet size
    0xec, 0x08, // VID
    0x10, 0x00, // PID
    0x00, 0x00, // Device release num
    0x01,       // Manufacturer string index
    0x02,       // Product string index
    0x03,       // device serial number index
    0x01,       // number of possible configuration

    /* Configuration descriptor */
    0x09,       // this descriptor size
    0x02,       // configuration descriptor type
    0x20, 0x00, // total length
    0x01,       // config number of interface
    0x01,       // config value
    0x00,       // config index
    0xc0,       // attribute
    0x32,       // max power unit=2mA

    /* Interface descriptor */
    0x09,       // this interface descriptor size
    0x04,       // interface descriptor type
    0x00,       // interface number
    0x00,       // alternative settings number
    0x02,       // endpoint number of this interface
    0x08,       // class code
    0x06,       // subclass code
    0x50,       // protocol
    0x00,       // interface index

    /* Endpoint descriptor (Bulk Out) */
    0x07,       // this endpoint descriptor size
    0x05,       // endpoint descriptor type
    0x01,       // EP1 address
    0x02,       // endpoint attribute = BULK
    0x40, 0x00, // max packet size
    0x00,       // interval

    /* Endpoint descriptor (Bulk In) */
    0x07,       // this descriptor size
    0x05,       // endpoint descriptor type
    0x82,       // EP2 address
    0x02,       // endpoint attribute = BULK
    0x40, 0x00, // max packet size
    0x00        // interval
};

static UINT8 __attribute__((aligned(32))) MscDescHs[] = {

    /* Device descriptor */
    0x12,       // this descriptor size
    0x01,       // device descriptor type
    0x00, 0x02, // Spec version
    0x00,       // class code
    0x00,       // subclass code
    0x00,       // procotol code
    0x40,       // max packet size
    0x0a, 0x07, // VID
    0x26, 0x40, // PID
    0x01, 0x00, // Device release num
    0x01,       // Manufacturer string index
    0x02,       // Product string index
    0x03,       // device serial number index
    0x01,       // number of possible configuration

    /* Device qualifier descriptor */
    0x0a,       // this descriptor size
    0x06,       // descriptor type
    0x00, 0x02, // Spec version
    0x00,       // class code
    0x00,       // subclass code
    0x00,       // procotol code
    0x40,       // max packet size
    0x01,       // configuration number
    0x00,       // reserved

    /* Configuration descriptor */
    0x09,       // this descriptor size
    0x02,       // descriptor type
    0x20, 0x00, // total length
    0x01,       // config number of interface
    0x01,       // config value
    0x00,       // config index
    0xc0,       // attribute
    0x32,       // max power unit=2mA

    /* Interface descriptor */
    0x09,       // this descriptor size
    0x04,       // descriptor type
    0x00,       // interface number
    0x00,       // alternative settings number
    0x02,       // endpoint number of this interface
    0x08,       // class code
    0x06,       // subclass code
    0x50,       // protocol
    0x00,       // interface index

    /* Endpoint descriptor (Bulk Out) */
    0x07,       // this descriptor size
    0x05,       // type
    0x01,       // EP1 address
    0x02,       // endpoint attribute = BULK
    0x00, 0x02, // max packet size
    0x00,       // interval

    /* Endpoint descriptor (Bulk In) */
    0x07,       // this descriptor size
    0x05,       // type
    0x82,       // EP2 address
    0x02,       // endpoint attribute = BULK
    0x00, 0x02, // max packet size
    0x00        // interval
};

/* String Device Framework :
 Byte 0 and 1 : Word containing the language ID : 0x0904 for US
 Byte 2       : Byte containing the index of the descriptor
 Byte 3       : Byte containing the length of the descriptor string
*/
static UINT8 __attribute__((aligned(32))) MscStr[] = {

    /* Manufacturer string descriptor : Index 1 */
    0x09, 0x04, 0x01, 0x0c,
    0x41, 0x6d, 0x62, 0x61,0x72, 0x65, 0x6c, 0x6c,
    0x61, 0x49, 0x6e, 0x63,

    /* Product string descriptor : Index 2 */
    0x09, 0x04, 0x02, 0x0c,
    0x41, 0x39, 0x20, 0x50, 0x6c, 0x61, 0x74, 0x66,
    0x6f, 0x72, 0x6d, 0x20,

    /* Serial Number string descriptor : Index 3 */
    0x09, 0x04, 0x03, 0x04,
    0x30, 0x30, 0x30, 0x31
};

/*-----------------------------------------------------------------------------------------------*\
 * MTP/PTP Class Default Descriptor
\*-----------------------------------------------------------------------------------------------*/
static UINT8 __attribute__((aligned(32))) MtpDescFs[] = {

    /* Device descriptor */
    0x12,           // this descriptor size
    0x01,           // device descriptor type
    0x10, 0x01,     // Spec version
    0x00,           // class code
    0x00,           // subclass code
    0x00,           // procotol code
    0x08,           // max packet size
    0xE8, 0x04,     // VID
    0xC5, 0x68,     // PID
    0x00, 0x00,     // Device release num
    0x00,           // Manufacturer string index
    0x00,           // Product string index
    0x00,           // device serial number index
    0x01,           // number of possible configuration

    /* Configuration descriptor */
    0x09,           // this descriptor size
    0x02,           // descriptor type
    0x27, 0x00,     // total length
    0x01,           // config number of interface
    0x01,           // config value
    0x00,           // config index
    0xc0,           // attribute
    0x32,           // max power unit=2mA

    /* Interface descriptor */
    0x09,           // this descriptor size
    0x04,           // descriptor type
    0x00,           // interface number
    0x00,           // alternative settings number
    0x03,           // endpoint number of this interface
    0x06,           // class code
    0x01,           // subclass code
    0x01,           // protocol
    0x00,           // interface index

    /* Endpoint descriptor (Bulk Out) */
    0x07,           // this descriptor size
    0x05,           // type
    0x02,           // EP2 address
    0x02,           // endpoint attribute = BULK
    0x40, 0x00,     // max packet size
    0x00,           // interval

    /* Endpoint descriptor (Bulk In) */
    0x07,           // this descriptor size
    0x05,           // type
    0x81,           // EP1 address
    0x02,           // endpoint attribute = BULK
    0x40, 0x00,     // max packet size
    0x00,           // interval

    /* Endpoint descriptor (Interrupt In) */
    0x07,           // this descriptor size
    0x05,           // type
    0x83,           // EP3 address
    0x03,           // endpoint attribute = INTERRUPT
    0x40, 0x00,     // max packet size
    0x04            // interval
};

static UINT8 __attribute__((aligned(32))) MtpDescHs[] = {

    /* Device descriptor */
    0x12,           // this descriptor size
    0x01,           // device descriptor type
    0x00, 0x02,     // Spec version
    0x00,           // class code
    0x00,           // subclass code
    0x00,           // procotol code
    0x40,           // max packet size
    0xE8, 0x04,     // VID
    0xC5, 0x68,     // PID
    0x01, 0x00,     // Device release num
    0x01,           // Manufacturer string index
    0x02,           // Product string index
    0x03,           // device serial number index
    0x01,           // number of possible configuration

    /* Device qualifier descriptor */
    0x0a,           // this descriptor size
    0x06,           // descriptor type
    0x00, 0x02,     // Spec version
    0x00,           // class code
    0x00,           // subclass code
    0x00,           // procotol code
    0x40,           // max packet size
    0x01,           // configuration number
    0x00,           // reserved

    /* Configuration descriptor */
    0x09,           // this descriptor size
    0x02,           // descriptor type
    0x27, 0x00,     // total length
    0x01,           // config number of interface
    0x01,           // config value
    0x00,           // config index
    0xc0,           // attribute
    0x32,           // max power unit=2mA

    /* Interface descriptor */
    0x09,           // this descriptor size
    0x04,           // descriptor type
    0x00,           // interface number
    0x00,           // alternative settings number
    0x03,           // endpoint number of this interface
    0x06,           // class code
    0x01,           // subclass code
    0x01,           // protocol
    0x00,           // interface index

    /* Endpoint descriptor (Bulk Out) */
    0x07,           // this descriptor size
    0x05,           // type
    0x01,           // EP1 address
    0x02,           // endpoint attribute = BULK
    0x00, 0x02,     // max packet size
    0x00,           // interval

    /* Endpoint descriptor (Bulk In) */
    0x07,           // this descriptor size
    0x05,           // type
    0x82,           // EP2 address
    0x02,           // endpoint attribute = BULK
    0x00, 0x02,     // max packet size
    0x00,           // interval

    /* Endpoint descriptor (Interrupt In) */
    0x07,           // this descriptor size
    0x05,           // type
    0x83,           // EP3 address
    0x03,           // endpoint attribute = INTERRUPT
    0x40, 0x00,     // max packet size
    0x04            // interval
};

/* String Device Framework :
 Byte 0 and 1 : Word containing the language ID : 0x0904 for US or 0x0000 for none.
 Byte 2       : Byte containing the index of the descriptor
 Byte 3       : Byte containing the length of the descriptor string

 The last string entry can be the optional Microsoft String descriptor.
*/

static UINT8 __attribute__((aligned(32))) MtpStr[] = {

    /* Manufacturer string descriptor : Index 1 */
    0x09, 0x04, 0x01, 0x0c,
    0x41, 0x6d, 0x62, 0x61,0x72, 0x65, 0x6c, 0x6c,
    0x61, 0x49, 0x6e, 0x63,

    /* Product string descriptor : Index 2 */
    0x09, 0x04, 0x02, 0x0c,
    0x41, 0x39, 0x20, 0x50, 0x6c, 0x61, 0x74, 0x66,
    0x6f, 0x72, 0x6d, 0x20,

    /* Serial Number string descriptor : Index 3 */
    0x09, 0x04, 0x03, 0x04,
    0x30, 0x30, 0x30, 0x31,

    /* Microsoft OS string descriptor : Index 0xEE. String is MSFT100.
       The last byte is the vendor code used to filter Vendor specific commands.
       The vendor commands will be executed in the class.
       This code can be anything but must not be 0x66 or 0x67 which are PIMA class commands.  */
    0x00, 0x00, 0xEE, 0x08,
    0x4D, 0x53, 0x46, 0x54,
    0x31, 0x30, 0x30,
    MTP_VENDOR_REQUEST

};

static CLASS_STACK_INIT_INFO_s UsbCustomInfo[AMP_USB_NUM_DEVICE_CLASS] = {
    [AMP_USB_DEVICE_CLASS_NONE] = {
        .DescFrameworkFs = NULL,
        .DescFrameworkHs = NULL,
        .StrFramework    = NULL,
        .LangIDFramework = NULL,
        .DescSizeFs      = 0,
        .DescSizeHs      = 0,
        .StrSize         = 0,
        .LangIDSize      = 0,
    },
    [AMP_USB_DEVICE_CLASS_MSC] = {
        .DescFrameworkFs = MscDescFs,
        .DescFrameworkHs = MscDescHs,
        .StrFramework    = MscStr,
        .LangIDFramework = LangID,
        .DescSizeFs      = sizeof(MscDescFs),
        .DescSizeHs      = sizeof(MscDescHs),
        .StrSize         = sizeof(MscStr),
        .LangIDSize      = sizeof(LangID),
    },
    [AMP_USB_DEVICE_CLASS_MTP] = {
        .DescFrameworkFs = MtpDescFs,
        .DescFrameworkHs = MtpDescHs,
        .StrFramework    = MtpStr,
        .LangIDFramework = LangID,
        .DescSizeFs      = sizeof(MtpDescFs),
        .DescSizeHs      = sizeof(MtpDescHs),
        .StrSize         = sizeof(MtpStr),
        .LangIDSize      = sizeof(LangID),
    },
#if 0
    [AMP_USB_DEVICE_CLASS_PICT] = {
        .DescFrameworkFs = PictBridgeDescFs,
        .DescFrameworkHs = PictBridgeDescHs,
        .StrFramework    = PictBridgeStr,
        .LangIDFramework = LangID,
        .DescSizeFs      = sizeof(PictBridgeDescFs),
        .DescSizeHs      = sizeof(PictBridgeDescHs),
        .StrSize         = sizeof(PictBridgeStr),
        .LangIDSize      = sizeof(LangID),
    },
    [AMP_USB_DEVICE_CLASS_CDC_ACM] = {
        .DescFrameworkFs = CdcAcmDescFs,
        .DescFrameworkHs = CdcAcmDescHs,
        .StrFramework    = CdcAcmStr,
        .LangIDFramework = LangID,
        .DescSizeFs      = sizeof(CdcAcmDescFs),
        .DescSizeHs      = sizeof(CdcAcmDescHs),
        .StrSize         = sizeof(CdcAcmStr),
        .LangIDSize      = sizeof(LangID),
    },
    [AMP_USB_DEVICE_CLASS_CUSTOM] = {
        .DescFrameworkFs = NULL,
        .DescFrameworkHs = NULL,
        .StrFramework    = NULL,
        .LangIDFramework = NULL,
        .DescSizeFs      = 0,
        .DescSizeHs      = 0,
        .StrSize         = 0,
        .LangIDSize      = 0,
    },
    [AMP_USB_DEVICE_CLASS_STREAM] = {
        .DescFrameworkFs = NULL,
        .DescFrameworkHs = NULL,
        .StrFramework    = NULL,
        .LangIDFramework = NULL,
        .DescSizeFs      = 0,
        .DescSizeHs      = 0,
        .StrSize         = 0,
    },
    [AMP_USB_DEVICE_CLASS_SIMPLE] = {
        .DescFrameworkFs = SimpleDescFs,
        .DescFrameworkHs = SimpleDescHs,
        .StrFramework    = SimpleStr,
        .LangIDFramework = LangID,
        .DescSizeFs      = sizeof(SimpleDescFs),
        .DescSizeHs      = sizeof(SimpleDescHs),
        .StrSize         = sizeof(SimpleStr),
        .LangIDSize      = sizeof(LangID),
    },
#endif
};
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaUSB_Custom_SetDevInfo
 *
 *  @Description:: Init USB custom device info
 *
 *  @Input      ::
 *          AMP_USB_CLASS_e:
 *
 *  @Output     :: none
 *
 *  @Return        ::
 *          UINT32 : UX_SUCCESS/UX_ERROR
\*-----------------------------------------------------------------------------------------------*/
UINT32 AmpUSB_Custom_SetDevInfo(AMP_USB_DEVICE_CLASS_e class)
{
    return AmbaUSB_Descriptor_Init(&UsbCustomInfo[class]);
}
