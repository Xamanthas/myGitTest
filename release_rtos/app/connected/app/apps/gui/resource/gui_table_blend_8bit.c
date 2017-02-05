/**
  * @file src/app/apps/gui/resource/connectedcam/gui_table_blend_8bit.c
  *
  *   Blending GUI objects
  *
  * History:
  *    2013/11/28 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include "gui_resource.h"
#include "gui_table.h"

#define BLEND_TABLE_FONT_HEIGHT   (54)

/*************************************************************************
 * Blending objects
 ************************************************************************/

APPLIB_GRAPHIC_UIOBJ_s bobj_date_stream_0_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_DATE, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_date_stream_1_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_DATE, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_date_stream_2_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_DATE, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_date_stream_3_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_DATE, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_date_stream_4_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_DATE, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_date_stream_5_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_DATE, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_h_m_stream_0_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_H_M, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_h_m_stream_1_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_H_M, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_h_m_stream_2_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_H_M, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_h_m_stream_3_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_H_M, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_h_m_stream_4_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_H_M, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_h_m_stream_5_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_H_M, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_s_stream_0_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_S, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_s_stream_1_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_S, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_s_stream_2_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_S, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_s_stream_3_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_S, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_s_stream_4_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_S, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

APPLIB_GRAPHIC_UIOBJ_s bobj_time_s_stream_5_8bit = {
    {0}, 0, GP_MAIN_1, 0, 0, APPLIB_GRAPHIC_UIOBJ_STRING,
    {.Str = {0, 230, 960, 110, BLEND_TABLE_FONT_HEIGHT, COLOR_8BIT_TEXT_BORDER, COLOR_8BIT_LIGHTGRAY,
                0x0, APPLIB_GRAPHIC_STRING_ALIGN_ML, 0, STR_OSD_BLEND_TIME_S, {1, APPLIB_GRAPHIC_STRING_SHADOW_BR, 2, COLOR_8BIT_DARKGRAY}, 0x0, 0x0}}
};

/*************************************************************************
 * Blending object table
 ************************************************************************/
APPLIB_GRAPHIC_UIOBJ_s *gui_table_blend_8bit[BOBJ_NUM] = {
    &bobj_date_stream_0_8bit,
    &bobj_date_stream_1_8bit,
    &bobj_date_stream_2_8bit,
    &bobj_date_stream_3_8bit,
    &bobj_date_stream_4_8bit,
    &bobj_date_stream_5_8bit,
    &bobj_time_h_m_stream_0_8bit,
    &bobj_time_h_m_stream_1_8bit,
    &bobj_time_h_m_stream_2_8bit,
    &bobj_time_h_m_stream_3_8bit,
    &bobj_time_h_m_stream_4_8bit,
    &bobj_time_h_m_stream_5_8bit,
    &bobj_time_s_stream_0_8bit,
    &bobj_time_s_stream_1_8bit,
    &bobj_time_s_stream_2_8bit,
    &bobj_time_s_stream_3_8bit,
    &bobj_time_s_stream_4_8bit,
    &bobj_time_s_stream_5_8bit,

    NULL    /** BOBJ_END */
};
