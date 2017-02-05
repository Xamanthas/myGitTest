/**
  * @file src/app/apps/gui/pb/gui_pb_photo.h
  *
  *  Header of photo playback GUI display flows
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
#include <apps/gui/utility/gui_utility.h>

#ifndef APP_GUI_PB_PHOTO_H_
#define APP_GUI_PB_PHOTO_H_

__BEGIN_C_PROTO__

typedef enum _GUI_PB_PHOTO_GUI_CMD_e_ {
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
    GUI_FILENAME_SHOW,
    GUI_FILENAME_HIDE,
    GUI_FILENAME_UPDATE,
    GUI_MEDIA_INFO_SHOW,
    GUI_MEDIA_INFO_HIDE,
    GUI_MEDIA_INFO_UPDATE,
    GUI_PHOTO_SIZE_SHOW,
    GUI_PHOTO_SIZE_HIDE,
    GUI_PHOTO_SIZE_UPDATE,
    GUI_ZOOMBAR_SHOW,
    GUI_ZOOMBAR_HIDE,
    GUI_ZOOMBAR_UPDATE
} GUI_PB_PHOTO_GUI_CMD_e;

extern int gui_pb_photo_func(UINT32 guiCmd, UINT32 param1, UINT32 param2);

__END_C_PROTO__

#endif /* APP_GUI_PB_PHOTO_H_ */
