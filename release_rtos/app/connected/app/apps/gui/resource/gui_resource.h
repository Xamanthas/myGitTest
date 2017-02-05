/**
  * @file src/app/apps/gui/resource/connectedcam/gui_resource.h
  *
  * Header for GUI resource
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mw.h>
#include <AmbaDataType.h>
#include "clut.h"
#include "clut_ayuv4444.h"
#include "clut_ayuv8888.h"
#include "clut_argb8888.h"
#include "strings.h"
#include "bitmaps.h"
#include <applib.h>

#ifndef APP_GUI_RESOURCE_H_
#define APP_GUI_RESOURCE_H_


__BEGIN_C_PROTO__

/*************************************************************************
 * GUI resource pixel type
 ************************************************************************/
extern int gui_pixel_type;
#define GUI_RESOURCE_8BIT    (0)
#define GUI_RESOURCE_16BIT    (1)
#define GUI_RESOURCE_32BIT_AYUV    (2)
#define GUI_RESOURCE_32BIT_ARGB    (3)
#define GUI_RESOURCE_TYPE_NUM    (4)
extern int gui_tv_layout;

/*************************************************************************
 * GUI font resource
 ************************************************************************/
#if    defined(ENABLE_ARPHIC_LIB) || defined(ENABLE_FREETYPE_LIB)
#define GUI_VECTOR_FONT        (1)
#endif

#if defined(GUI_VECTOR_FONT)
#define GUI_FONT_H_DEFAULT    (36)
#else
#define GUI_FONT_H_DEFAULT    (54)
#endif


/*************************************************************************
 * GUI string resource
 ************************************************************************/


/*************************************************************************
 * GUI bitmap resource
 ************************************************************************/

/*************************************************************************
 * GUI default colors
 ************************************************************************/
extern UINT32 COLOR_BLACK;
extern UINT32 COLOR_RED;
extern UINT32 COLOR_GREEN;
extern UINT32 COLOR_BLUE;
extern UINT32 COLOR_MAGENTA;
extern UINT32 COLOR_LIGHTGRAY;
extern UINT32 COLOR_DARKGRAY;
extern UINT32 COLOR_YELLOW;
extern UINT32 COLOR_WHITE;
extern UINT32 COLOR_THUMB_BLUE;
extern UINT32 COLOR_THUMB_GRAY;
extern UINT32 COLOR_TEXT_BORDER;
extern UINT32 COLOR_MENU_BG;
extern UINT32 COLOR_WARNING;
extern UINT32 COLOR_CLR;

/*************************************************************************
 * GUI table resource
 ************************************************************************/
#define GUI_TABLE_STATIC    (0)    // Static GUI tables
#define GUI_TABLE_ROMFS        (1)    // GUI tables from ROMFS
#define GUI_TABLE_SOURCE    GUI_TABLE_STATIC

__END_C_PROTO__

#endif /* APP_GUI_RESOURCE_H__ */
