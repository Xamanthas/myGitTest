/**
  * @file src/app/apps/flow/usb/usb_msc.h
  *
  * Implementation of USB MSC class
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
#ifndef APP_USB_MSC_H__
#define APP_USB_MSC_H__

#include <apps/apps.h>
#include <apps/gui/usb/gui_usb_msc.h>

__BEGIN_C_PROTO__

//#define USB_MSC_DEBUG
#if defined(USB_MSC_DEBUG)
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
typedef enum _USB_MSC_FUNC_ID_e_ {
    USB_MSC_INIT = 0,
    USB_MSC_START,
    USB_MSC_START_FLG_ON,
    USB_MSC_STOP,
    USB_MSC_APP_READY,
    USB_MSC_CHANGE_DISPLAY,
    USB_MSC_CHANGE_OSD,
    USB_MSC_SET_APP_ENV,
    USB_MSC_DETECT_REMOVE,
    USB_MSC_UPDATE_FCHAN_VOUT,
    USB_MSC_UPDATE_DCHAN_VOUT,
    USB_MSC_AUDIO_INPUT,
    USB_MSC_AUDIO_OUTPUT,
    USB_MSC_GUI_INIT_SHOW,
    USB_MSC_CARD_INSERT,
    USB_MSC_CARD_REMOVE
} USB_MSC_FUNC_ID_e;

extern int usb_msc_func(UINT32 funcId, UINT32 param1, UINT32 param2);


/*************************************************************************
 * App Status Definitions
 ************************************************************************/
typedef struct _USB_MSC_s_ {
    int (*Func)(UINT32 funcId, UINT32 param1, UINT32 param2);
    int (*Gui)(UINT32 guiCmd, UINT32 param1, UINT32 param2);
} USB_MSC_s;

extern USB_MSC_s usb_msc;

__END_C_PROTO__

#endif /* APP_USB_MSC_H__ */
