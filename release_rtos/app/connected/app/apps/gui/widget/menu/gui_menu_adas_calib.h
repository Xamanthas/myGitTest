/**
  * @file src/app/apps/gui/widget/menu/sportcam/gui_menu_adas_calib.h
  *
  * Header for ADAS Calibration Menu GUI flow
  *
  * History:
  *    2013/11/22 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef APP_GUI_WIDGET_MENU_ADAS_CALIB_H_
#define APP_GUI_WIDGET_MENU_ADAS_CALIB_H_

__BEGIN_C_PROTO__

/*************************************************************************
 * ADAS calibration menu GUI definitions
 ************************************************************************/
typedef enum _GUI_MENU_ADAS_CALIB_CMD_ID_e_ {
    GUI_FLUSH = 0,
    GUI_HIDE_ALL,
    GUI_HIDE_ADAS_CALIB_SKY,
    GUI_HIDE_ADAS_CALIB_HOOD,
    GUI_SHOW_ADAS_CALIB_SKY,
    GUI_SHOW_ADAS_CALIB_HOOD,
    GUI_UPDATE_ADAS_CALIB_SKY,
    GUI_UPDATE_ADAS_CALIB_HOOD
} GUI_MENU_ADAS_CALIB_CMD_ID_e;


/*************************************************************************
 * ADAS calibration menu Widget GUI functions
 ************************************************************************/
extern int gui_menu_adas_calib_func(UINT32 guiCmd, UINT32 param1, UINT32 param2);

__END_C_PROTO__

#endif /* APP_GUI_WIDGET_MENU_ADAS_CALIB_H_ */
