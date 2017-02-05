/**
  * @file src/app/apps/gui/resource/connectedcam/gui_settle.h
  *
  * Header for GUI object
  *
  * History:
  *    2013/09/23 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#ifndef APP_GUI_SETTLE_H_
#define APP_GUI_SETTLE_H_

#include "gui_resource.h"
#include "gui_table.h"

__BEGIN_C_PROTO__


/*************************************************************************
 * GUI layout size define
 ************************************************************************/


/*************************************************************************
 * GUI layout settings
 ************************************************************************/
typedef enum _GUI_LAYOUT_ID_e_ {
    GUI_LAYOUT_960x480 = 0,
    GUI_LAYOUT_480x960,
    GUI_LAYOUT_1920x135_TV,
    GUI_LAYOUT_960x540_TV,
    GUI_LAYOUT_BLEND,
    GUI_LAYOUT_NUM
} GUI_LAYOUT_ID_e;

/* Warning message id */
typedef enum _GUI_WARNING_ID_e_ {
    GUI_WARNING_PROCESSING = 0,
    GUI_WARNING_LOADING,
    GUI_WARNING_RECOVERING,
    GUI_WARNING_NO_FILES,
    GUI_WARNING_MEM_RUNOUT,
    GUI_WARNING_NO_CARD,
    GUI_WARNING_CARD_PROTECTED,
    GUI_WARNING_CARD_FULL,
    GUI_WARNING_CARD_VOLUME_LIMIT,
    GUI_WARNING_FILE_LIMIT,
    GUI_WARNING_PHOTO_LIMIT,
    GUI_WARNING_INDEX_FILE_LIMIT,
    GUI_WARNING_FILE_INDEX_LIMIT,
    GUI_WARNING_FILE_AMOUNT_LIMIT,
    GUI_WARNING_STORAGE_IO_ERROR,
    GUI_WARNING_PRE_RECORDING_NOT_PERMIT,
    GUI_WARNING_CANT_GET_AP,
    GUI_WARNING_PIV_BUSY,
    GUI_WARNING_PIV_DISALLOWED,
    GUI_WARNING_PIV_ERROR,
    GUI_WARNING_LDWS_EVENT,
    GUI_WARNING_FCWS_EVENT,
    GUI_WARNING_FCMD_EVENT,
    GUI_WARNING_LLWS_EVENT,
    GUI_WARNING_MD_EVENT,
    GUI_WARNING_MSG_NUM
} GUI_WARNING_ID_e;

typedef struct _GUI_WARNING_DATA_s_ {
    UINT32 id;
    UINT32 str;
    UINT32 bmp;
} GUI_WARNING_DATA_s;

extern GUI_WARNING_DATA_s GuiWarningTable[];

extern int Gui_Resource_Fchan_Id;
extern int Gui_Resource_Dchan_Id;
extern int Gui_Resource_Blend_Id;
extern APPLIB_GRAPHIC_UIOBJ_s **Gui_Table_Fchan;
extern APPLIB_GRAPHIC_UIOBJ_s **Gui_Table_Dchan;
extern APPLIB_GRAPHIC_UIOBJ_s **Gui_Table_Blend;

extern int AppGui_Init(void);

__END_C_PROTO__

#endif /* APP_GUI_SETTLE_H_ */
