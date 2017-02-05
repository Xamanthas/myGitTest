/**
  * @file src/app/apps/gui/misc/gui_misc_calibupdate.h
  *
  *  Header of Firmware Update GUI display flows
  *
  * History:
  *    2014/06/26 - [Annie Ting] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef APP_GUI_MISC_CALIBUPDATE_H_
#define APP_GUI_MISC_CALIBUPDATE_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaDataType.h>
#include <applib.h>

__BEGIN_C_PROTO__;

typedef enum _GUI_MISC_CALIBUPDATE_CMD_ID_e_ {
    GUI_FLUSH = 0,
    GUI_HIDE_ALL,
    GUI_SET_LAYOUT,
    GUI_WARNING_SHOW,
    GUI_WARNING_HIDE,
    GUI_WARNING_UPDATE
} GUI_MISC_CALIBUPDATE_CMD_ID_e;

extern int gui_misc_calibupdate_func(UINT32 guiCmd, UINT32 param1, UINT32 param2);

__END_C_PROTO__

#endif /* APP_GUI_MISC_CALIBUPDATE_H_ */
