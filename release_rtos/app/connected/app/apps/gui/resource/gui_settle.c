 /**
  * @file src/app/apps/gui/resource/connectedcam/gui_settle.c
  *
  * GUI objects
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
#include "gui_settle.h"
#include <applib.h>
#include <display/Osd.h>


GUI_WARNING_DATA_s GuiWarningTable[GUI_WARNING_MSG_NUM] = {
    {GUI_WARNING_PROCESSING, STR_PROCESSING, BMP_0_NULL},
    {GUI_WARNING_LOADING, STR_LOADING, BMP_0_NULL},
    {GUI_WARNING_RECOVERING, STR_RECOVERING, BMP_0_NULL},
    {GUI_WARNING_NO_FILES, STR_NO_FILES, BMP_0_NULL},
    {GUI_WARNING_MEM_RUNOUT, STR_MEM_RUNOUT, BMP_0_NULL},
    {GUI_WARNING_NO_CARD, STR_NO_CARD, BMP_0_NULL},
    {GUI_WARNING_CARD_PROTECTED, STR_CARD_PROTECTED, BMP_0_NULL},
    {GUI_WARNING_CARD_FULL, STR_CARD_FULL, BMP_0_NULL},
    {GUI_WARNING_CARD_VOLUME_LIMIT, STR_CARD_VOLUME_LIMIT, BMP_0_NULL},
    {GUI_WARNING_FILE_LIMIT, STR_FILE_LIMIT, BMP_0_NULL},
    {GUI_WARNING_PHOTO_LIMIT, STR_PHOTO_AMOUNT_LIMIT, BMP_0_NULL},
    {GUI_WARNING_INDEX_FILE_LIMIT, STR_INDEX_FILE_LIMIT, BMP_0_NULL},
    {GUI_WARNING_FILE_INDEX_LIMIT, STR_DMF_FILE_INDEX_LIMIT, BMP_0_NULL},
    {GUI_WARNING_FILE_AMOUNT_LIMIT, STR_DMF_FILE_AMOUNT_LIMIT, BMP_0_NULL},
    {GUI_WARNING_STORAGE_IO_ERROR, STR_STORAGE_IO_ERROR, BMP_0_NULL},
    {GUI_WARNING_PRE_RECORDING_NOT_PERMIT, STR_PRE_RECORDING_NOT_PERMIT, BMP_0_NULL},
    {GUI_WARNING_CANT_GET_AP, STR_CANT_GET_WIFI_AP, BMP_0_NULL},
    {GUI_WARNING_PIV_BUSY, STR_PIV_BUSY, BMP_0_NULL},
    {GUI_WARNING_PIV_BUSY, STR_PIV_DISALLOWED, BMP_0_NULL},
    {GUI_WARNING_PIV_ERROR, STR_PIV_ERROR, BMP_0_NULL},
    {GUI_WARNING_LDWS_EVENT, STR_LDWS_EVENT, BMP_0_NULL},
    {GUI_WARNING_FCWS_EVENT, STR_FCWS_EVENT, BMP_0_NULL},
    {GUI_WARNING_FCMD_EVENT, STR_FCMD_EVENT, BMP_0_NULL},
    {GUI_WARNING_LLWS_EVENT, STR_LLWS_EVENT, BMP_0_NULL},
    {GUI_WARNING_MD_EVENT, STR_MD_EVENT, BMP_0_NULL}
};


/*************************************************************************
 * GUI pixel format setting
 ************************************************************************/
int gui_pixel_type = 0;
int gui_tv_layout = -1;

/*************************************************************************
 * GUI preset colors
 ************************************************************************/
UINT32 COLOR_BLACK = 0;
UINT32 COLOR_RED = 0;
UINT32 COLOR_GREEN = 0;
UINT32 COLOR_BLUE = 0;
UINT32 COLOR_MAGENTA = 0;
UINT32 COLOR_LIGHTGRAY = 0;
UINT32 COLOR_DARKGRAY = 0;
UINT32 COLOR_YELLOW = 0;
UINT32 COLOR_WHITE = 0;
UINT32 COLOR_THUMB_BLUE = 0;
UINT32 COLOR_THUMB_GRAY = 0;
UINT32 COLOR_TEXT_BORDER = 0;
UINT32 COLOR_MENU_BG = 0;
UINT32 COLOR_WARNING = 0;
UINT32 COLOR_CLR = 0;

/*************************************************************************
 * GUI layout settings
 ************************************************************************/
#define OSD_FCHAN_WIDTH     (960)
#define OSD_FCHAN_HEIGHT    (540)
#define OSD_DCHAN_WIDTH     (960)
#define OSD_DCHAN_HEIGHT    (480)
#define OSD_BLEND_WIDTH     (960)
#define OSD_BLEND_HEIGHT    (480)

int Gui_Resource_Fchan_Id = -1;
int Gui_Resource_Dchan_Id = -1;
int Gui_Resource_Blend_Id = -1;
APPLIB_GRAPHIC_UIOBJ_s **Gui_Table_Fchan = NULL;
APPLIB_GRAPHIC_UIOBJ_s **Gui_Table_Dchan = NULL;
APPLIB_GRAPHIC_UIOBJ_s **Gui_Table_Blend = NULL;

int AppGui_Init(void)
{
    gui_pixel_type = GUI_RESOURCE_8BIT;
    switch (gui_pixel_type) {
    case GUI_RESOURCE_8BIT:
        COLOR_BLACK = COLOR_8BIT_BLACK;
        COLOR_RED = COLOR_8BIT_RED;
        COLOR_GREEN = COLOR_8BIT_GREEN;
        COLOR_BLUE = COLOR_8BIT_BLUE;
        COLOR_MAGENTA = COLOR_8BIT_MAGENTA;
        COLOR_LIGHTGRAY = COLOR_8BIT_LIGHTGRAY;
        COLOR_DARKGRAY = COLOR_8BIT_DARKGRAY;
        COLOR_YELLOW = COLOR_8BIT_YELLOW;
        COLOR_WHITE = COLOR_8BIT_WHITE;
        COLOR_THUMB_BLUE = COLOR_8BIT_THUMB_BLUE;
        COLOR_THUMB_GRAY = COLOR_8BIT_THUMB_GRAY;
        COLOR_TEXT_BORDER = COLOR_8BIT_TEXT_BORDER;
        COLOR_MENU_BG = COLOR_8BIT_MENU_BG;
        COLOR_WARNING = COLOR_8BIT_WARNING;
        COLOR_CLR = COLOR_8BIT_CLR;
        AppLibGraph_SetPixelFormat(GRAPH_CH_DCHAN, AMP_OSD_8BIT_CLUT_MODE);
        AppLibGraph_SetPixelFormat(GRAPH_CH_FCHAN, AMP_OSD_8BIT_CLUT_MODE);
        Gui_Table_Fchan = gui_table_960x540_tv_8bit;
        Gui_Table_Dchan = gui_table_960x480_8bit;
        Gui_Table_Blend = gui_table_blend_8bit;
        break;
    case GUI_RESOURCE_16BIT:
        COLOR_BLACK = COLOR_16BIT_BLACK;
        COLOR_RED = COLOR_16BIT_RED;
        COLOR_GREEN = COLOR_16BIT_GREEN;
        COLOR_BLUE = COLOR_16BIT_BLUE;
        COLOR_MAGENTA = COLOR_16BIT_MAGENTA;
        COLOR_LIGHTGRAY = COLOR_16BIT_LIGHTGRAY;
        COLOR_DARKGRAY = COLOR_16BIT_DARKGRAY;
        COLOR_YELLOW = COLOR_16BIT_YELLOW;
        COLOR_WHITE = COLOR_16BIT_WHITE;
        COLOR_THUMB_BLUE = COLOR_16BIT_THUMB_BLUE;
        COLOR_THUMB_GRAY = COLOR_16BIT_THUMB_GRAY;
        COLOR_TEXT_BORDER = COLOR_16BIT_TEXT_BORDER;
        COLOR_MENU_BG = COLOR_16BIT_MENU_BG;
        COLOR_WARNING = COLOR_16BIT_WARNING;
        COLOR_CLR = COLOR_16BIT_CLR;
        AppLibGraph_SetPixelFormat(GRAPH_CH_DCHAN, AMP_OSD_16BIT_AYUV_4444);
        AppLibGraph_SetPixelFormat(GRAPH_CH_FCHAN, AMP_OSD_16BIT_AYUV_4444);
        break;
    case GUI_RESOURCE_32BIT_AYUV:
        COLOR_BLACK = COLOR_AYUV32BIT_BLACK;
        COLOR_RED = COLOR_AYUV32BIT_RED;
        COLOR_GREEN = COLOR_AYUV32BIT_GREEN;
        COLOR_BLUE = COLOR_AYUV32BIT_BLUE;
        COLOR_MAGENTA = COLOR_AYUV32BIT_MAGENTA;
        COLOR_LIGHTGRAY = COLOR_AYUV32BIT_LIGHTGRAY;
        COLOR_DARKGRAY = COLOR_AYUV32BIT_DARKGRAY;
        COLOR_YELLOW = COLOR_AYUV32BIT_YELLOW;
        COLOR_WHITE = COLOR_AYUV32BIT_WHITE;
        COLOR_THUMB_BLUE = COLOR_AYUV32BIT_THUMB_BLUE;
        COLOR_THUMB_GRAY = COLOR_AYUV32BIT_THUMB_GRAY;
        COLOR_TEXT_BORDER = COLOR_AYUV32BIT_TEXT_BORDER;
        COLOR_MENU_BG = COLOR_AYUV32BIT_MENU_BG;
        COLOR_WARNING = COLOR_AYUV32BIT_WARNING;
        COLOR_CLR = COLOR_AYUV32BIT_CLR;
        AppLibGraph_SetPixelFormat(GRAPH_CH_DCHAN, AMP_OSD_32BIT_AYUV_8888);
        AppLibGraph_SetPixelFormat(GRAPH_CH_FCHAN, AMP_OSD_32BIT_AYUV_8888);
        Gui_Table_Fchan = gui_table_960x540_tv_32bit;
        Gui_Table_Dchan = gui_table_960x480_32bit;
        Gui_Table_Blend = gui_table_blend_32bit;
        break;
    case GUI_RESOURCE_32BIT_ARGB:
    default:
        COLOR_BLACK = COLOR_ARGB32BIT_BLACK;
        COLOR_RED = COLOR_ARGB32BIT_RED;
        COLOR_GREEN = COLOR_ARGB32BIT_GREEN;
        COLOR_BLUE = COLOR_ARGB32BIT_BLUE;
        COLOR_MAGENTA = COLOR_ARGB32BIT_MAGENTA;
        COLOR_LIGHTGRAY = COLOR_ARGB32BIT_LIGHTGRAY;
        COLOR_DARKGRAY = COLOR_ARGB32BIT_DARKGRAY;
        COLOR_YELLOW = COLOR_ARGB32BIT_YELLOW;
        COLOR_WHITE = COLOR_ARGB32BIT_WHITE;
        COLOR_THUMB_BLUE = COLOR_ARGB32BIT_THUMB_BLUE;
        COLOR_THUMB_GRAY = COLOR_ARGB32BIT_THUMB_GRAY;
        COLOR_TEXT_BORDER = COLOR_ARGB32BIT_TEXT_BORDER;
        COLOR_MENU_BG = COLOR_ARGB32BIT_MENU_BG;
        COLOR_WARNING = COLOR_ARGB32BIT_WARNING;
        COLOR_CLR = COLOR_ARGB32BIT_CLR;
        AppLibGraph_SetPixelFormat(GRAPH_CH_DCHAN, AMP_OSD_32BIT_ARGB_8888);
        AppLibGraph_SetPixelFormat(GRAPH_CH_FCHAN, AMP_OSD_32BIT_ARGB_8888);
        Gui_Table_Fchan = gui_table_960x540_tv_32bit;
        Gui_Table_Dchan = gui_table_960x480_32bit;
        Gui_Table_Blend = gui_table_blend_32bit;
        break;
    }
    AppLibGraph_SetOsdSize(GRAPH_CH_DCHAN, OSD_DCHAN_WIDTH, OSD_DCHAN_HEIGHT);
    AppLibGraph_SetOsdSize(GRAPH_CH_FCHAN, OSD_FCHAN_WIDTH, OSD_FCHAN_HEIGHT);
    AppLibGraph_SetOsdSize(GRAPH_CH_BLEND, OSD_BLEND_WIDTH, OSD_BLEND_HEIGHT);

    Gui_Resource_Fchan_Id = 0;
    Gui_Resource_Dchan_Id = 0;
    Gui_Resource_Blend_Id = 0;


    AppLibGraph_SetMaxObjectNum(GRAPH_CH_DUAL, GOBJ_END);
    AppLibGraph_SetMaxObjectNum(GRAPH_CH_BLEND, GOBJ_END);
    {
        APPLIB_GRAPH_INIT_CONFIG_s initConfig;
        initConfig.Font.FontFileName = "fonts.bin";
        initConfig.Font.FontType = FONT_TYPE_BMP;
        //initConfig.FontFileName = "cour.ttf";
        //initConfig.FontType = FONT_TYPE_CUSTOMIZED;
        initConfig.ClutFileName = "clut.bin";
        initConfig.BMPFileName = "bitmaps.bin";
        initConfig.StringFileName = "strings.bin";
        initConfig.DchanEnable = 1;
        initConfig.FchanEnable = 1;
        initConfig.BlendEnable = 1;
        AppLibGraph_SetDefaultConfig(initConfig);
    }
    return 0;
}

