/**
  * @file src/app/apps/gui/usb/gui_usb_msc.h
  *
  *  Header of USB MSC GUI display flows
  *
  * History:
  *    2013/11/08 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaDataType.h>
#include <applib.h>

#ifndef APP_GUI_USB_MSC_H_
#define APP_GUI_USB_MSC_H_

__BEGIN_C_PROTO__

typedef enum _GUI_USB_MSC_GUI_CMD_e_ {
    GUI_FLUSH = 0,
    GUI_HIDE_ALL,
    GUI_SET_LAYOUT,
    GUI_APP_ICON_SHOW,
    GUI_APP_ICON_HIDE,
    GUI_POWER_STATE_SHOW,
    GUI_POWER_STATE_HIDE,
    GUI_POWER_STATE_UPDATE,
    GUI_CARD_SHOW,
    GUI_CARD_HIDE,
    GUI_CARD_UPDATE,
    GUI_PHOTO_SIZE_SHOW,
    GUI_PHOTO_SIZE_HIDE,
    GUI_PHOTO_SIZE_UPDATE,
    GUI_VIDEO_SENSOR_RES_SHOW,
    GUI_VIDEO_SENSOR_RES_HIDE,
    GUI_VIDEO_SENSOR_RES_UPDATE,
    GUI_ZOOMBAR_SHOW,
    GUI_ZOOMBAR_HIDE,
    GUI_ZOOMBAR_UPDATE,
    GUI_WARNING_SHOW,
    GUI_WARNING_HIDE,
    GUI_WARNING_UPDATE
} GUI_USB_MSC_GUI_CMD_e;

/* Battery and power status parameters */
#define GUI_HIDE_POWER_EXCEPT_BAT   (1)
#define GUI_HIDE_POWER_EXCEPT_DC    (2)

typedef enum _GUI_USB_MSC_BAT_STATE_ID_e_ {
    GUI_BATTERY_NONE = 0,
    GUI_BATTERY_EMPTY,
    GUI_BATTERY_STATE_0,
    GUI_BATTERY_STATE_1,
    GUI_BATTERY_STATE_2,
    GUI_BATTERY_STATE_3
} GUI_USB_MSC_BAT_STATE_ID_e;

typedef enum _GUI_USB_MSC_DC_STATE_ID_e_ {
    GUI_POWER_UNKNOWN = 0,
    GUI_POWER_ADAPTER,
    GUI_POWER_BATTERY
} GUI_USB_MSC_DC_STATE_ID_e;

/* Card state parameters */
typedef enum _GUI_USB_MSC_CARD_STATE_ID_e_ {
    GUI_NO_CARD = 0,
    GUI_CARD_REFRESHING,
    GUI_CARD_READY
} GUI_USB_MSC_CARD_STATE_ID_e;

extern int gui_usb_msc_func(UINT32 guiCmd, UINT32 param1, UINT32 param2);

__END_C_PROTO__

#endif /* APP_GUI_USB_MSC_H_ */
