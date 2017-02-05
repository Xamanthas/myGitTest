/**
  * @file src/app/apps/gui/rec/gui_hdmi_test.h
  *
  *  Header of HDMI test GUI display flows
  *
  * History:
  *    2015/05/25 - [James Wang] created file
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
#include <apps/apps.h>
#include <apps/gui/utility/gui_utility.h>

#ifndef APP_GUI_HDMI_TEST_H_
#define APP_GUI_HDMI_TEST_H_

__BEGIN_C_PROTO__

typedef enum _GUI_HDMI_TEST_GUI_CMD_e_ {
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
    GUI_WARNING_SHOW,
    GUI_WARNING_HIDE,
    GUI_WARNING_UPDATE,
    GUI_PLAY_STATE_SHOW,
    GUI_PLAY_STATE_HIDE,
    GUI_PLAY_STATE_UPDATE,
    GUI_PLAY_TIMER_SHOW,
    GUI_PLAY_TIMER_HIDE,
    GUI_PLAY_TIMER_UPDATE,
    GUI_PLAY_SPEED_SHOW,
    GUI_PLAY_SPEED_HIDE,
    GUI_FILENAME_SHOW,
    GUI_FILENAME_HIDE,
    GUI_FILENAME_UPDATE,
    GUI_MEDIA_INFO_SHOW,
    GUI_MEDIA_INFO_HIDE,
    GUI_MEDIA_INFO_UPDATE,
    GUI_VIDEO_SENSOR_RES_SHOW,
    GUI_VIDEO_SENSOR_RES_HIDE,
    GUI_VIDEO_SENSOR_RES_UPDATE,
    GUI_ZOOMBAR_SHOW,
    GUI_ZOOMBAR_HIDE,
    GUI_ZOOMBAR_UPDATE
} GUI_HDMI_TEST_GUI_CMD_e;

/* Battery and power status parameters */
#define GUI_HIDE_POWER_EXCEPT_BAT   (1)
#define GUI_HIDE_POWER_EXCEPT_DC    (2)

/** Video Play state parameters */
typedef enum _GUI_HDMI_TEST_PLAY_STATE_ID_e_ {
    GUI_PAUSE = 0,
    GUI_FWD_NORMAL,
    GUI_FWD_FAST,
    GUI_FWD_FAST_END,
    GUI_FWD_SLOW,
    GUI_FWD_STEP,
    GUI_REW_NORMAL,
    GUI_REW_FAST,
    GUI_REW_FAST_END,
    GUI_REW_SLOW,
    GUI_REW_STEP
} GUI_HDMI_TEST_PLAY_STATE_ID_e;

extern int gui_hdmi_test_func(UINT32 guiCmd, UINT32 param1, UINT32 param2);

__END_C_PROTO__

#endif /* APP_GUI_HDMI_TEST_H_ */
