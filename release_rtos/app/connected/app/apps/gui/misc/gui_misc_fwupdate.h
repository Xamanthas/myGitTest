/**
  * @file src/app/apps/gui/misc/gui_misc_fwupdate.h
  *
  *  Header of Firmware Update GUI display flows
  *
  * History:
  *    2014/03/20 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef APP_GUI_MISC_FWUPDATE_H_
#define APP_GUI_MISC_FWUPDATE_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaDataType.h>
#include <applib.h>

__BEGIN_C_PROTO__;

typedef enum _GUI_MISC_FWUPDATE_CMD_ID_e_ {
    GUI_FLUSH = 0,
    GUI_HIDE_ALL,
    GUI_SET_LAYOUT,
    GUI_APP_VIDEO_ICON_SHOW,
    GUI_APP_PHOTO_ICON_SHOW,
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
    GUI_FLASHLIGHT_SHOW,
    GUI_FWUPDATE_RATIO_SHOW,
    GUI_FWUPDATE_RATIO_UPDATE,
    GUI_FWUPDATE_RATIO_HIDE,
    GUI_FWUPDATE_STAGE_SHOW,
    GUI_FWUPDATE_STAGE_UPDATE,
    GUI_FWUPDATE_STAGE_HIDE
} GUI_MISC_FWUPDATE_CMD_ID_e;

extern int gui_misc_fwupdate_func(UINT32 guiCmd, UINT32 param1, UINT32 param2);

__END_C_PROTO__

#endif /* APP_GUI_MISC_FWUPDATE_H_ */
