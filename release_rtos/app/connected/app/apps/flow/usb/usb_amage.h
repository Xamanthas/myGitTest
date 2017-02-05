/**
  * @file src/app/apps/flow/usb/usb_amage.h
  *
  * Header of USB MTP class for Amage
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
#ifndef APP_USB_AMAGE_H__
#define APP_USB_AMAGE_H__

#include <apps/apps.h>

__BEGIN_C_PROTO__

//#define USB_AMAGE_DEBUG
#if defined(USB_AMAGE_DEBUG)
#define DBGMSG  AmbaPrint
#define DBGMSGc2    AmbaPrintColor
#else
#define DBGMSG(...)
#define DBGMSGc2(...)
#endif


/*************************************************************************
 * App Flag Definitions
 ************************************************************************/


/*************************************************************************
 * App Function Definitions
 ************************************************************************/
typedef enum _USB_AMAGE_FUNC_ID_e_ {
    USB_AMAGE_INIT = 0,
    USB_AMAGE_START,
    USB_AMAGE_START_FLG_ON,
    USB_AMAGE_STOP,
    USB_AMAGE_DETECT_REMOVE,
    USB_AMAGE_DETECT_CONNECT,
    USB_AMAGE_SET_APP_ENV,
    USB_AMAGE_CHANGE_DISPLAY,
    USB_AMAGE_CHANGE_ENC_MODE,
    USB_AMAGE_UPDATE_FCHAN_VOUT,
    USB_AMAGE_STILL_CAPTURE,
    USB_AMAGE_MUXER_END,
    USB_AMAGE_BG_PROCESS_DONE
} USB_AMAGE_FUNC_ID_e;

extern int usb_amage_func(UINT32 funcId, UINT32 param1, UINT32 param2);

#define USB_AMAGE_FLAGS_SINGLE_CAPTURE            (1<<0)
#define USB_AMAGE_FLAGS_STILL_RAW_CAPTURE            (1<<1)
#define USB_AMAGE_FLAGS_VIDEO_RAW_CAPTURE            (1<<2)

/*************************************************************************
 * App Status Definitions
 ************************************************************************/
typedef struct _USB_AMAGE_s_ {
    UINT32 CurMode;
#define USB_AMAGE_VIDEO_MODE   0
#define USB_AMAGE_STILL_MODE    1
    int (*Func)(UINT32 funcId, UINT32 param1, UINT32 param2);
    int (*Gui)(UINT32 guiCmd, UINT32 param1, UINT32 param2);
} USB_AMAGE_s;

extern USB_AMAGE_s usb_amage;

__END_C_PROTO__

#endif /* APP_USB_AMAGE_H__ */
