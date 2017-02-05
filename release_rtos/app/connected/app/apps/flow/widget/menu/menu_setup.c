/**
  * @file src/app/apps/flow/widget/menu/connectedcam/menu_setup.c
  *
  * Implementation of Setup-related Menu Items
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
//#pragma O0

#include <apps/apps.h>
#include <system/status.h>
#include <system/app_pref.h>
#include <system/app_util.h>
#include <apps/flow/widget/menu/widget.h>
#include <apps/gui/resource/gui_settle.h>
#include <apps/flow/widget/menu/widget.h>
#include <system/ApplibSys_Lcd.h>
#include <AmbaRTC.h>
/*************************************************************************
 * Declaration:
 ************************************************************************/
// tab
static int menu_setup_init(void);
static int menu_setup_start(void);
static int menu_setup_stop(void);
// item
static int menu_setup_system_type_init(void);
static int menu_setup_system_type_get_tab_str(void);
static int menu_setup_system_type_get_sel_str(int ref);
static int menu_setup_system_type_get_sel_bmp(int ref);
static int menu_setup_system_type_set(void);
static int menu_setup_system_type_sel_set(void);
static int menu_setup_dmf_mode_init(void);
static int menu_setup_dmf_mode_get_tab_str(void);
static int menu_setup_dmf_mode_get_sel_str(int ref);
static int menu_setup_dmf_mode_get_sel_bmp(int ref);
static int menu_setup_dmf_mode_set(void);
static int menu_setup_dmf_mode_sel_set(void);
static int menu_setup_time_init(void);
static int menu_setup_time_get_tab_str(void);
static int menu_setup_time_set(void);
static int menu_setup_date_time_disp_init(void);
static int menu_setup_date_time_disp_get_tab_str(void);
static int menu_setup_date_time_disp_get_sel_str(int ref);
static int menu_setup_date_time_disp_get_sel_bmp(int ref);
static int menu_setup_date_time_disp_set(void);
static int menu_setup_date_time_disp_sel_set(void);
static int menu_setup_format_init(void);
static int menu_setup_format_get_tab_str(void);
static int menu_setup_format_set(void);
static int menu_setup_default_init(void);
static int menu_setup_default_get_tab_str(void);
static int menu_setup_default_set(void);
static int menu_setup_lcd_ctrl_init(void);
static int menu_setup_lcd_ctrl_get_tab_str(void);
static int menu_setup_lcd_ctrl_get_sel_str(int ref);
static int menu_setup_lcd_ctrl_get_sel_bmp(int ref);
static int menu_setup_lcd_ctrl_set(void);
static int menu_setup_lcd_ctrl_sel_set(void);
static int menu_setup_lcd_brightness_get_cur_val(void);
static int menu_setup_lcd_brightness_get_val_str(int val);
static int menu_setup_lcd_brightness_val_set(int val);
static int menu_setup_lcd_contrast_get_cur_val(void);
static int menu_setup_lcd_contrast_get_val_str(int val);
static int menu_setup_lcd_contrast_val_set(int val);
static int menu_setup_lcd_colorbalance_get_cur_val(void);
static int menu_setup_lcd_colorbalance_get_val_str(int val);
static int menu_setup_lcd_colorbalance_val_set(int val);
static int menu_setup_poweroff_init(void);
static int menu_setup_poweroff_get_tab_str(void);
static int menu_setup_poweroff_get_sel_str(int ref);
static int menu_setup_poweroff_get_sel_bmp(int ref);
static int menu_setup_poweroff_set(void);
static int menu_setup_poweroff_sel_set(void);
static int menu_setup_powersaving_init(void);
static int menu_setup_powersaving_get_tab_str(void);
static int menu_setup_powersaving_get_sel_str(int ref);
static int menu_setup_powersaving_get_sel_bmp(int ref);
static int menu_setup_powersaving_set(void);
static int menu_setup_powersaving_sel_set(void);
static int menu_setup_hdmi_init(void);
static int menu_setup_hdmi_get_tab_str(void);
static int menu_setup_hdmi_get_sel_str(int ref);
static int menu_setup_hdmi_get_sel_bmp(int ref);
static int menu_setup_hdmi_set(void);
static int menu_setup_hdmi_sel_set(void);
static int menu_setup_flashlight_init(void);
static int menu_setup_flashlight_get_tab_str(void);
static int menu_setup_flashlight_get_sel_str(int ref);
static int menu_setup_flashlight_get_sel_bmp(int ref);
static int menu_setup_flashlight_set(void);
static int menu_setup_flashlight_sel_set(void);
static int menu_setup_usb_mode_init(void);
static int menu_setup_usb_mode_get_tab_str(void);
static int menu_setup_usb_mode_get_sel_str(int ref);
static int menu_setup_usb_mode_get_sel_bmp(int ref);
static int menu_setup_usb_mode_set(void);
static int menu_setup_usb_mode_sel_set(void);
static int menu_setup_input_dimension_mode_init(void);
static int menu_setup_input_dimension_mode_get_tab_str(void);
static int menu_setup_input_dimension_mode_get_sel_str(int ref);
static int menu_setup_input_dimension_mode_get_sel_bmp(int ref);
static int menu_setup_input_dimension_mode_set(void);
static int menu_setup_input_dimension_mode_sel_set(void);
static int menu_setup_output_dimension_mode_init(void);
static int menu_setup_output_dimension_mode_get_tab_str(void);
static int menu_setup_output_dimension_mode_get_sel_str(int ref);
static int menu_setup_output_dimension_mode_get_sel_bmp(int ref);
static int menu_setup_output_dimension_mode_set(void);
static int menu_setup_output_dimension_mode_sel_set(void);
static int menu_setup_wifi_init(void);
static int menu_setup_wifi_get_tab_str(void);
static int menu_setup_wifi_get_sel_str(int ref);
static int menu_setup_wifi_get_sel_bmp(int ref);
static int menu_setup_wifi_set(void);
static int menu_setup_wifi_sel_set(void);
static int menu_setup_wifi_config_init(void);
static int menu_setup_wifi_config_get_tab_str(void);
static int menu_setup_wifi_config_get_sel_str(int ref);
static int menu_setup_wifi_config_get_sel_bmp(int ref);
static int menu_setup_wifi_config_set(void);
static int menu_setup_wifi_config_sel_set(void);
static int menu_setup_version_init(void);
static int menu_setup_version_get_tab_str(void);
static int menu_setup_version_set(void);

// control
static MENU_TAB_s* menu_setup_get_tab(void);
static MENU_ITEM_s* menu_setup_get_item(UINT32 itemId);
static MENU_SEL_s* menu_setup_get_sel(UINT32 itemId, UINT32 selId);
static int menu_setup_set_sel_table(UINT32 itemId, MENU_SEL_s *selTbl);
static int menu_setup_lock_tab(void);
static int menu_setup_unlock_tab(void);
static int menu_setup_enable_item(UINT32 itemId);
static int menu_setup_disable_item(UINT32 itemId);
static int menu_setup_lock_item(UINT32 itemId);
static int menu_setup_unlock_item(UINT32 itemId);
static int menu_setup_enable_sel(UINT32 itemId, UINT32 selId);
static int menu_setup_disable_sel(UINT32 itemId, UINT32 selId);
static int menu_setup_lock_sel(UINT32 itemId, UINT32 selId);
static int menu_setup_unlock_sel(UINT32 itemId, UINT32 selId);

/*************************************************************************
 * Definition:
 ************************************************************************/
/** Selection */
static MENU_SEL_s menu_setup_system_type_sel_tbl[MENU_SETUP_SYSTEM_TYPE_SEL_NUM] = {
    {MENU_SETUP_SYSTEM_TYPE_NTSC, MENU_SEL_FLAGS_ENABLE,
        STR_SYSTEM_NTSC, BMP_ICN_SYSTEM_NTSC, 0,
        VOUT_SYS_NTSC, NULL},
    {MENU_SETUP_SYSTEM_TYPE_PAL, MENU_SEL_FLAGS_ENABLE,
        STR_SYSTEM_PAL, BMP_ICN_SYSTEM_PAL, 0,
        VOUT_SYS_PAL, NULL}
};

static MENU_SEL_s menu_setup_dmf_mode_sel_tbl[MENU_SETUP_DMF_MODE_SEL_NUM] = {
    {MENU_SETUP_DMF_MODE_RESET, MENU_SEL_FLAGS_ENABLE,
        STR_FILE_NUMBER_RESET, BMP_ICN_DMF_RESET, 0,
        MENU_SETUP_DMF_MODE_RESET, NULL},
    {MENU_SETUP_DMF_MODE_SERIAL, MENU_SEL_FLAGS_ENABLE,
        STR_FILE_NUMBER_SERIAL, BMP_ICN_DMF_SERIAL, 0,
        MENU_SETUP_DMF_MODE_SERIAL, NULL}
};

static MENU_SEL_s menu_setup_date_time_disp_sel_tbl[MENU_SETUP_DATE_TIME_DISPLAY_SEL_NUM] = {
    {MENU_SETUP_DATE_TIME_DISPLAY_OFF, MENU_SEL_FLAGS_ENABLE,
        0, 0, 0,
        MENU_SETUP_DATE_TIME_DISPLAY_OFF, NULL},
    {MENU_SETUP_DATE_TIME_DISPLAY_DATE, MENU_SEL_FLAGS_ENABLE,
        0, 0, 0,
        MENU_SETUP_DATE_TIME_DISPLAY_DATE, NULL},
    {MENU_SETUP_DATE_TIME_DISPLAY_TIME, MENU_SEL_FLAGS_ENABLE,
        0, 0, 0,
        MENU_SETUP_DATE_TIME_DISPLAY_TIME, NULL},
    {MENU_SETUP_DATE_TIME_DISPLAY_BOTH, MENU_SEL_FLAGS_ENABLE,
        0, 0, 0,
        MENU_SETUP_DATE_TIME_DISPLAY_BOTH, NULL}
};

static MENU_MAIN_ADJ_s menu_setup_lcd_brightness_adj = {
    5, 0, 255,
    menu_setup_lcd_brightness_get_cur_val,
    menu_setup_lcd_brightness_get_val_str,
    menu_setup_lcd_brightness_val_set
};

static MENU_MAIN_ADJ_s menu_setup_lcd_contrast_adj = {
    5, 0, 255,
    menu_setup_lcd_contrast_get_cur_val,
    menu_setup_lcd_contrast_get_val_str,
    menu_setup_lcd_contrast_val_set
};

static MENU_MAIN_ADJ_s menu_setup_lcd_colorbalance_adj = {
    5, 0, 255,
    menu_setup_lcd_colorbalance_get_cur_val,
    menu_setup_lcd_colorbalance_get_val_str,
    menu_setup_lcd_colorbalance_val_set
};

static MENU_SEL_s menu_setup_lcd_ctrl_sel_tbl[MENU_SETUP_LCD_CONTROL_SEL_NUM] = {
    {MENU_SETUP_LCD_CONTROL_BRIGHTNESS, MENU_SEL_FLAGS_ENABLE,
        STR_LCD_BRIGHTNESS, BMP_ICN_BRIGHTNESS_LOW, BMP_ICN_BRIGHTNESS_HIGH,
        65, &menu_setup_lcd_brightness_adj},
    {MENU_SETUP_LCD_CONTROL_CONTRAST, MENU_SEL_FLAGS_ENABLE,
        STR_LCD_CONTRAST, BMP_ICN_BRIGHTNESS_LOW, BMP_ICN_BRIGHTNESS_HIGH,
        65, &menu_setup_lcd_contrast_adj},
    {MENU_SETUP_LCD_CONTROL_COLOR_BALANCE, MENU_SEL_FLAGS_ENABLE,
        STR_LCD_COLORBALANCE, BMP_ICN_BRIGHTNESS_LOW, BMP_ICN_BRIGHTNESS_HIGH,
        125, &menu_setup_lcd_colorbalance_adj}
};

static MENU_SEL_s menu_setup_poweroff_sel_tbl[MENU_SETUP_POWEROFF_SEL_NUM] = {
    {MENU_SETUP_POWEROFF_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_POWEROFF_OFF, BMP_ICN_AUTO_POWER_OFF, 0,
        AUTO_POWEROFF_OFF, NULL},
    {MENU_SETUP_POWEROFF_3M, MENU_SEL_FLAGS_ENABLE,
        STR_POWEROFF_3_MIN, BMP_ICN_AUTO_POWER_OFF_3, 0,
        AUTO_POWEROFF_TIME_3M, NULL},
    {MENU_SETUP_POWEROFF_5M, MENU_SEL_FLAGS_ENABLE,
        STR_POWEROFF_5_MIN, BMP_ICN_AUTO_POWER_OFF_5, 0,
        AUTO_POWEROFF_TIME_5M, NULL}
};

static MENU_SEL_s menu_setup_powersaving_sel_tbl[MENU_SETUP_POWERSAVING_SEL_NUM] = {
    {MENU_SETUP_POWERSAVING_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_POWERSAVING_OFF, BMP_ICN_AUTO_POWER_OFF, 0,
        POWERSAVING_OFF, NULL},
    {MENU_SETUP_POWERSAVING_ON, MENU_SEL_FLAGS_ENABLE,
        STR_POWERSAVING_ON, BMP_ICN_AUTO_POWER_OFF, 0,
        POWERSAVING_ON, NULL}
};

static MENU_SEL_s menu_setup_hdmi_sel_tbl[MENU_SETUP_POWERSAVING_SEL_NUM] = {
    {MENU_SETUP_HDMI_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_HDMISUPPORT_OFF, BMP_ICN_HDMI_OFF, 0,
        MENU_SETUP_HDMI_OFF, NULL},
    {MENU_SETUP_HDMI_ON, MENU_SEL_FLAGS_ENABLE,
        STR_HDMISUPPORT_ON, BMP_ICN_HDMI_OFF, 0,
        MENU_SETUP_HDMI_ON, NULL}
};

static MENU_SEL_s menu_setup_flashlight_sel_tbl[MENU_SETUP_FLASHLIGHT_SEL_NUM] = {
    {MENU_SETUP_FLASHLIGHT_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_FLASHLIGHT_OFF, BMP_ICN_FLASHLIGHT_OFF, 0,
        MENU_SETUP_FLASHLIGHT_OFF, NULL},
    {MENU_SETUP_FLASHLIGHT_ON, MENU_SEL_FLAGS_ENABLE,
        STR_FLASHLIGHT_ON, BMP_ICN_FLASHLIGHT_ON, 0,
        MENU_SETUP_FLASHLIGHT_ON, NULL},
    {MENU_SETUP_FLASHLIGHT_AUTO, MENU_SEL_FLAGS_ENABLE,
        STR_FLASHLIGHT_AUTO, BMP_ICN_FLASHLIGHT_AUTO, 0,
        MENU_SETUP_FLASHLIGHT_AUTO, NULL}
};

static MENU_SEL_s menu_setup_usb_mode_sel_tbl[MENU_SETUP_USB_MODE_SEL_NUM] = {
    {MENU_SETUP_USB_MODE_MSC, MENU_SEL_FLAGS_ENABLE,
        STR_USB_MASS_STORAGE, BMP_ICN_USB_MSC, 0,
        USB_MODE_MSC, NULL},
    {MENU_SETUP_USB_MODE_AMAGE, MENU_SEL_FLAGS_ENABLE,  // disalbe AMAGE function until it's ready
        STR_USB_IMAGE, BMP_ICN_USB_IMAGE, 0,
        USB_MODE_AMAGE, NULL},
 #ifdef CONFIG_ASD_USB_RS232_ENABLE
    {MENU_SETUP_USB_MODE_RS232, MENU_SEL_FLAGS_ENABLE,
    	STR_USB_RS232, BMP_ICN_USB_IMAGE, 0,
    	USB_MODE_RS232, NULL},
 #endif
};

static MENU_SEL_s menu_setup_input_dimension_mode_sel_tbl[MENU_SETUP_INPUT_DIMENSION_MODE_SEL_NUM] = {
    {MENU_SETUP_INPUT_2D_R, MENU_SEL_FLAGS_ENABLE,
        STR_INPUT_DIMENSION_2D_R, BMP_ICN_2D_R, 0,
        MENU_SETUP_INPUT_2D_R, NULL},
    {MENU_SETUP_INPUT_2D_L, MENU_SEL_FLAGS_ENABLE,
        STR_INPUT_DIMENSION_2D_L, BMP_ICN_2D_L, 0,
        MENU_SETUP_INPUT_2D_L, NULL},
    {MENU_SETUP_INPUT_3D, MENU_SEL_FLAGS_ENABLE,
        STR_INPUT_DIMENSION_3D, BMP_ICN_3D, 0,
        MENU_SETUP_INPUT_3D, NULL}
};

static MENU_SEL_s menu_setup_output_dimension_mode_sel_tbl[MENU_SETUP_OUTPUT_DIMENSION_MODE_SEL_NUM] = {
    {MENU_SETUP_OUTPUT_2D, MENU_SEL_FLAGS_ENABLE,
        STR_OUTPUT_DIMENSION_2D, BMP_ICN_2D, 0,
        MENU_SETUP_OUTPUT_2D, NULL},
    {MENU_SETUP_OUTPUT_2D_RIGHT, MENU_SEL_FLAGS_ENABLE,
        STR_OUTPUT_DIMENSION_2D_RIGHT, BMP_ICN_2D_R, 0,
        MENU_SETUP_OUTPUT_2D_RIGHT, NULL},
    {MENU_SETUP_OUTPUT_2D_LEFT, MENU_SEL_FLAGS_ENABLE,
        STR_OUTPUT_DIMENSION_2D_LEFT, BMP_ICN_2D_L, 0,
        MENU_SETUP_OUTPUT_2D_LEFT, NULL},
    {MENU_SETUP_OUTPUT_3D, MENU_SEL_FLAGS_ENABLE,
        STR_OUTPUT_DIMENSION_3D, BMP_ICN_3D, 0,
        MENU_SETUP_OUTPUT_3D, NULL}
};

static MENU_SEL_s menu_setup_wifi_sel_tbl[MENU_SETUP_WIFI_SEL_NUM] = {
    {MENU_SETUP_WIFI_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_WIFI_OFF, BMP_ICN_WIFI_OFF, 0,
        MENU_SETUP_WIFI_OFF, NULL},
    {MENU_SETUP_WIFI_ON, MENU_SEL_FLAGS_ENABLE,
        STR_WIFI_ON, BMP_ICN_WIFI, 0,
        MENU_SETUP_WIFI_ON, NULL},
    {MENU_SETUP_WIFI_ON_AP, MENU_SEL_FLAGS_ENABLE,
        STR_WIFI_ON_AP, BMP_ICN_WIFI, 0,
        MENU_SETUP_WIFI_ON_AP, NULL}
};

static MENU_SEL_s menu_setup_wifi_config_sel_tbl[MENU_SETUP_WIFI_CONFIG_SEL_NUM] = {
    {MENU_SETUP_WIFI_CONFIG_0, MENU_SEL_FLAGS_LOCKED,
        STR_WIFI_AP_0, BMP_ICN_WIFI_AP_0, 0,
        0, NULL},
    {MENU_SETUP_WIFI_CONFIG_1, MENU_SEL_FLAGS_LOCKED,
        STR_WIFI_AP_1, BMP_ICN_WIFI_AP_1, 0,
        0, NULL},
    {MENU_SETUP_WIFI_CONFIG_2, MENU_SEL_FLAGS_LOCKED,
        STR_WIFI_AP_2, BMP_ICN_WIFI_AP_2, 0,
        0, NULL},
    {MENU_SETUP_WIFI_CONFIG_3, MENU_SEL_FLAGS_LOCKED,
        STR_WIFI_AP_3, BMP_ICN_WIFI_AP_3, 0,
        0, NULL},
    {MENU_SETUP_WIFI_CONFIG_4, MENU_SEL_FLAGS_LOCKED,
        STR_WIFI_AP_4, BMP_ICN_WIFI_AP_4, 0,
        0, NULL}
};

static MENU_SEL_s *menu_setup_item_sel_tbls[MENU_SETUP_ITEM_NUM] = {
    menu_setup_system_type_sel_tbl,
    menu_setup_dmf_mode_sel_tbl,
    NULL,    // time_init
    menu_setup_date_time_disp_sel_tbl,
    NULL,    // format
    NULL,    // default
    menu_setup_lcd_ctrl_sel_tbl,
    menu_setup_poweroff_sel_tbl,
    menu_setup_powersaving_sel_tbl,
    menu_setup_hdmi_sel_tbl,
    menu_setup_flashlight_sel_tbl,
    menu_setup_usb_mode_sel_tbl,
    menu_setup_input_dimension_mode_sel_tbl,
    menu_setup_output_dimension_mode_sel_tbl,
    menu_setup_wifi_sel_tbl,
    menu_setup_wifi_config_sel_tbl,
    NULL    // version
};

/*** Currently activated object id arrays ***/
static MENU_SEL_s *menu_setup_system_type_sels[MENU_SETUP_SYSTEM_TYPE_SEL_NUM];
static MENU_SEL_s *menu_setup_dmf_mode_sels[MENU_SETUP_DMF_MODE_SEL_NUM];
static MENU_SEL_s *menu_setup_date_time_disp_sels[MENU_SETUP_DATE_TIME_DISPLAY_SEL_NUM];
static MENU_SEL_s *menu_setup_lcd_ctrl_sels[MENU_SETUP_LCD_CONTROL_SEL_NUM];
static MENU_SEL_s *menu_setup_poweroff_sels[MENU_SETUP_POWEROFF_SEL_NUM];
static MENU_SEL_s *menu_setup_powersaving_sels[MENU_SETUP_POWERSAVING_SEL_NUM];
static MENU_SEL_s *menu_setup_hdmi_sels[MENU_SETUP_HDMI_SEL_NUM];
static MENU_SEL_s *menu_setup_flashlight_sels[MENU_SETUP_FLASHLIGHT_SEL_NUM];
static MENU_SEL_s *menu_setup_usb_mode_sels[MENU_SETUP_USB_MODE_SEL_NUM];
static MENU_SEL_s *menu_setup_input_dimension_mode_sels[MENU_SETUP_INPUT_DIMENSION_MODE_SEL_NUM];
static MENU_SEL_s *menu_setup_output_dimension_mode_sels[MENU_SETUP_OUTPUT_DIMENSION_MODE_SEL_NUM];
static MENU_SEL_s *menu_setup_wifi_sels[MENU_SETUP_WIFI_SEL_NUM];
static MENU_SEL_s *menu_setup_wifi_config_sels[MENU_SETUP_WIFI_CONFIG_SEL_NUM];

/*** Item ***/
static MENU_ITEM_s menu_setup_system_type = {
    MENU_SETUP_SYSTEM_TYPE, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_system_type_sels,
    menu_setup_system_type_init,
    menu_setup_system_type_get_tab_str,
    menu_setup_system_type_get_sel_str,
    menu_setup_system_type_get_sel_bmp,
    menu_setup_system_type_set,
    menu_setup_system_type_sel_set
};

static MENU_ITEM_s menu_setup_dmf_mode = {
    MENU_SETUP_DMF_MODE, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_dmf_mode_sels,
    menu_setup_dmf_mode_init,
    menu_setup_dmf_mode_get_tab_str,
    menu_setup_dmf_mode_get_sel_str,
    menu_setup_dmf_mode_get_sel_bmp,
    menu_setup_dmf_mode_set,
    menu_setup_dmf_mode_sel_set
};

static MENU_ITEM_s menu_setup_time = {
    MENU_SETUP_TIME, MENU_ITEM_FLAGS_ENABLE,
    0, 0,
    0, 0,
    0, 0, 0, NULL,
    menu_setup_time_init,
    menu_setup_time_get_tab_str,
    NULL,
    NULL,
    menu_setup_time_set,
    NULL
};

static MENU_ITEM_s menu_setup_date_time_disp = {
    MENU_SETUP_DATE_TIME_DISPLAY, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_date_time_disp_sels,
    menu_setup_date_time_disp_init,
    menu_setup_date_time_disp_get_tab_str,
    menu_setup_date_time_disp_get_sel_str,
    menu_setup_date_time_disp_get_sel_bmp,
    menu_setup_date_time_disp_set,
    menu_setup_date_time_disp_sel_set
};

static MENU_ITEM_s menu_setup_format = {
    MENU_SETUP_FORMAT, MENU_ITEM_FLAGS_ENABLE,
    0, 0,
    0, 0,
    0, 0, 0, NULL,
    menu_setup_format_init,
    menu_setup_format_get_tab_str,
    NULL,
    NULL,
    menu_setup_format_set,
    NULL
};

static MENU_ITEM_s menu_setup_default = {
    MENU_SETUP_DEFAULT, MENU_ITEM_FLAGS_ENABLE,
    0, 0,
    0, 0,
    0, 0, 0, NULL,
    menu_setup_default_init,
    menu_setup_default_get_tab_str,
    NULL,
    NULL,
    menu_setup_default_set,
    NULL
};

static MENU_ITEM_s menu_setup_lcd_ctrl = {
    MENU_SETUP_LCD_CONTROL, MENU_ITEM_FLAGS_ENABLE,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_lcd_ctrl_sels,
    menu_setup_lcd_ctrl_init,
    menu_setup_lcd_ctrl_get_tab_str,
    menu_setup_lcd_ctrl_get_sel_str,
    menu_setup_lcd_ctrl_get_sel_bmp,
    menu_setup_lcd_ctrl_set,
    menu_setup_lcd_ctrl_sel_set
};

static MENU_ITEM_s menu_setup_poweroff = {
    MENU_SETUP_POWEROFF, MENU_ITEM_FLAGS_ENABLE,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_poweroff_sels,
    menu_setup_poweroff_init,
    menu_setup_poweroff_get_tab_str,
    menu_setup_poweroff_get_sel_str,
    menu_setup_poweroff_get_sel_bmp,
    menu_setup_poweroff_set,
    menu_setup_poweroff_sel_set
};

static MENU_ITEM_s menu_setup_powersaving = {
    MENU_SETUP_POWERSAVING, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_powersaving_sels,
    menu_setup_powersaving_init,
    menu_setup_powersaving_get_tab_str,
    menu_setup_powersaving_get_sel_str,
    menu_setup_powersaving_get_sel_bmp,
    menu_setup_powersaving_set,
    menu_setup_powersaving_sel_set
};

static MENU_ITEM_s menu_setup_hdmi = {
    MENU_SETUP_HDMI, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_hdmi_sels,
    menu_setup_hdmi_init,
    menu_setup_hdmi_get_tab_str,
    menu_setup_hdmi_get_sel_str,
    menu_setup_hdmi_get_sel_bmp,
    menu_setup_hdmi_set,
    menu_setup_hdmi_sel_set
};

static MENU_ITEM_s menu_setup_flashlight = {
    MENU_SETUP_FLASHLIGHT, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_flashlight_sels,
    menu_setup_flashlight_init,
    menu_setup_flashlight_get_tab_str,
    menu_setup_flashlight_get_sel_str,
    menu_setup_flashlight_get_sel_bmp,
    menu_setup_flashlight_set,
    menu_setup_flashlight_sel_set
};

static MENU_ITEM_s menu_setup_usb_mode = {
    MENU_SETUP_USB_MODE, MENU_ITEM_FLAGS_ENABLE,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_usb_mode_sels,
    menu_setup_usb_mode_init,
    menu_setup_usb_mode_get_tab_str,
    menu_setup_usb_mode_get_sel_str,
    menu_setup_usb_mode_get_sel_bmp,
    menu_setup_usb_mode_set,
    menu_setup_usb_mode_sel_set
};

static MENU_ITEM_s menu_setup_input_dimension_mode = {
    MENU_SETUP_INPUT_DIMENSION_MODE, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_input_dimension_mode_sels,
    menu_setup_input_dimension_mode_init,
    menu_setup_input_dimension_mode_get_tab_str,
    menu_setup_input_dimension_mode_get_sel_str,
    menu_setup_input_dimension_mode_get_sel_bmp,
    menu_setup_input_dimension_mode_set,
    menu_setup_input_dimension_mode_sel_set
};

static MENU_ITEM_s menu_setup_output_dimension_mode = {
    MENU_SETUP_OUTPUT_DIMENSION_MODE, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_output_dimension_mode_sels,
    menu_setup_output_dimension_mode_init,
    menu_setup_output_dimension_mode_get_tab_str,
    menu_setup_output_dimension_mode_get_sel_str,
    menu_setup_output_dimension_mode_get_sel_bmp,
    menu_setup_output_dimension_mode_set,
    menu_setup_output_dimension_mode_sel_set
};

static MENU_ITEM_s menu_setup_wifi = {
    MENU_SETUP_WIFI, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_wifi_sels,
    menu_setup_wifi_init,
    menu_setup_wifi_get_tab_str,
    menu_setup_wifi_get_sel_str,
    menu_setup_wifi_get_sel_bmp,
    menu_setup_wifi_set,
    menu_setup_wifi_sel_set
};

static MENU_ITEM_s menu_setup_wifi_config = {
    MENU_SETUP_WIFI_CONFIG, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_setup_wifi_config_sels,
    menu_setup_wifi_config_init,
    menu_setup_wifi_config_get_tab_str,
    menu_setup_wifi_config_get_sel_str,
    menu_setup_wifi_config_get_sel_bmp,
    menu_setup_wifi_config_set,
    menu_setup_wifi_config_sel_set
};

/**
 * @brief Menu_setup item: version
 */
static MENU_ITEM_s menu_setup_version = {
    MENU_SETUP_VERSION, 0,
    0, 0,
    0, 0,
    0, 0, 0, NULL,
    menu_setup_version_init,
    menu_setup_version_get_tab_str,
    NULL,
    NULL,
    menu_setup_version_set,
    NULL
};

static MENU_ITEM_s *menu_setup_item_tbl[MENU_SETUP_ITEM_NUM] = {
    &menu_setup_system_type,
    &menu_setup_dmf_mode,
    &menu_setup_time,
    &menu_setup_date_time_disp,
    &menu_setup_format,
    &menu_setup_default,
    &menu_setup_lcd_ctrl,
    &menu_setup_poweroff,
    &menu_setup_powersaving,
    &menu_setup_hdmi,
    &menu_setup_flashlight,
    &menu_setup_usb_mode,
    &menu_setup_input_dimension_mode,
    &menu_setup_output_dimension_mode,
    &menu_setup_wifi,
    &menu_setup_wifi_config,
    &menu_setup_version
};

/*** Currently activated object id arrays ***/
static MENU_ITEM_s *menu_setup_items[MENU_SETUP_ITEM_NUM];

/*** Tab ***/
static MENU_TAB_s menu_setup = {
    MENU_SETUP, 1,
    0, 0,
    BMP_MENU_TAB_SETUP, BMP_MENU_TAB_SETUP_HL,
    menu_setup_items,
    menu_setup_init,
    menu_setup_start,
    menu_setup_stop
};

MENU_TAB_CTRL_s menu_setup_ctrl = {
    menu_setup_get_tab,
    menu_setup_get_item,
    menu_setup_get_sel,
    menu_setup_set_sel_table,
    menu_setup_lock_tab,
    menu_setup_unlock_tab,
    menu_setup_enable_item,
    menu_setup_disable_item,
    menu_setup_lock_item,
    menu_setup_unlock_item,
    menu_setup_enable_sel,
    menu_setup_disable_sel,
    menu_setup_lock_sel,
    menu_setup_unlock_sel
};

//static struct tm tm_bak;

/*** APIs ***/
// tab
static int menu_setup_init(void)
{
    int i = 0;
    UINT32 cur_item_id = 0;

#if defined (FLASHLIGHT)
    menu_setup_enable_item(MENU_SETUP_FLASHLIGHT);
#endif

    menu_setup_disable_item(MENU_SETUP_POWEROFF);
    menu_setup_disable_item(MENU_SETUP_LCD_CONTROL);

#if defined(APP_HDMI_TEST)
    menu_setup_enable_item(MENU_SETUP_SYSTEM_TYPE);
#endif

    APP_ADDFLAGS(menu_setup.Flags, MENU_TAB_FLAGS_INIT);
    if (menu_setup.ItemNum > 0) {
        cur_item_id = menu_setup_items[menu_setup.ItemCur]->Id;
    }
    menu_setup.ItemNum = 0;
    menu_setup.ItemCur = 0;
    for (i=0; i<MENU_SETUP_ITEM_NUM; i++) {
        if (APP_CHECKFLAGS(menu_setup_item_tbl[i]->Flags, MENU_ITEM_FLAGS_ENABLE)) {
            menu_setup_items[menu_setup.ItemNum] = menu_setup_item_tbl[i];
            if (cur_item_id == menu_setup_item_tbl[i]->Id) {
                menu_setup.ItemCur = menu_setup.ItemNum;
            }
            menu_setup.ItemNum++;
        }
    }

    return 0;
}

static int menu_setup_start(void)
{
    return 0;
}

static int menu_setup_stop(void)
{
    return 0;
}

// item
static int menu_setup_system_type_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_setup_system_type.Flags, MENU_ITEM_FLAGS_INIT);
    menu_setup_system_type.SelSaved = 0;
    menu_setup_system_type.SelNum = 0;
    menu_setup_system_type.SelCur = 0;
    for (i=0; i<MENU_SETUP_SYSTEM_TYPE_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_setup_item_sel_tbls[MENU_SETUP_SYSTEM_TYPE]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_setup_system_type_sels[menu_setup_system_type.SelNum] = menu_setup_item_sel_tbls[MENU_SETUP_SYSTEM_TYPE]+i;
            if (menu_setup_system_type_sels[menu_setup_system_type.SelNum]->Val == UserSetting->SetupPref.VoutSystem) {
                menu_setup_system_type.SelSaved = menu_setup_system_type.SelNum;
                menu_setup_system_type.SelCur = menu_setup_system_type.SelNum;
            }
            menu_setup_system_type.SelNum++;
        }
    }

    return 0;
}

static int menu_setup_system_type_get_tab_str(void)
{
    return menu_setup_system_type_sels[menu_setup_system_type.SelSaved]->Str;
}

static int menu_setup_system_type_get_sel_str(int ref)
{
    return menu_setup_system_type_sels[ref]->Str;
}

static int menu_setup_system_type_get_sel_bmp(int ref)
{
    return menu_setup_system_type_sels[ref]->Bmp;
}

static int menu_setup_system_type_set(void)
{
    AppMenuQuick_SetItem(MENU_SETUP, MENU_SETUP_SYSTEM_TYPE);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_setup_system_type_sel_set(void)
{
    if (menu_setup_system_type.SelSaved != menu_setup_system_type.SelCur) {
        menu_setup_system_type.SelSaved = menu_setup_system_type.SelCur;
        UserSetting->SetupPref.VinSystem = menu_setup_system_type_sels[menu_setup_system_type.SelCur]->Val;
        AppLibSysVin_SetSystemType(menu_setup_system_type_sels[menu_setup_system_type.SelCur]->Val);
        UserSetting->SetupPref.VoutSystem = menu_setup_system_type_sels[menu_setup_system_type.SelCur]->Val;
        AppLibSysVout_SetSystemType(menu_setup_system_type_sels[menu_setup_system_type.SelCur]->Val);
        {
        /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_SYSTEM_TYPE,
            menu_setup_system_type_sels[menu_setup_system_type.SelCur]->Val,
            menu_setup_system_type_sels[menu_setup_system_type.SelCur]->Val);
        }
    }

    return 0;
}

static int menu_setup_dmf_mode_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_setup_dmf_mode.Flags, MENU_ITEM_FLAGS_INIT);
    menu_setup_dmf_mode.SelSaved = 0;
    menu_setup_dmf_mode.SelNum = 0;
    menu_setup_dmf_mode.SelCur = 0;
    for (i=0; i<MENU_SETUP_DMF_MODE_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_setup_item_sel_tbls[MENU_SETUP_DMF_MODE]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_setup_dmf_mode_sels[menu_setup_dmf_mode.SelNum] = menu_setup_item_sel_tbls[MENU_SETUP_DMF_MODE]+i;
            if (menu_setup_dmf_mode_sels[menu_setup_dmf_mode.SelNum]->Val == UserSetting->SetupPref.DMFMode) {
                menu_setup_dmf_mode.SelSaved = menu_setup_dmf_mode.SelNum;
                menu_setup_dmf_mode.SelCur = menu_setup_dmf_mode.SelNum;
            }
            menu_setup_dmf_mode.SelNum++;
        }
    }

    return 0;
}

static int menu_setup_dmf_mode_get_tab_str(void)
{
    return menu_setup_dmf_mode_sels[menu_setup_dmf_mode.SelSaved]->Str;
}

static int menu_setup_dmf_mode_get_sel_str(int ref)
{
    return menu_setup_dmf_mode_sels[ref]->Str;
}

static int menu_setup_dmf_mode_get_sel_bmp(int ref)
{
    return menu_setup_dmf_mode_sels[ref]->Bmp;
}

static int menu_setup_dmf_mode_set(void)
{
    AppMenuQuick_SetItem(MENU_SETUP, MENU_SETUP_DMF_MODE);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_setup_dmf_mode_sel_set(void)
{
    if (menu_setup_dmf_mode.SelSaved != menu_setup_dmf_mode.SelCur) {
        menu_setup_dmf_mode.SelSaved = menu_setup_dmf_mode.SelCur;
        UserSetting->SetupPref.DMFMode = menu_setup_dmf_mode.Sels[menu_setup_dmf_mode.SelCur]->Val;
        if (UserSetting->SetupPref.DMFMode == DMF_MODE_RESET) {
            UserSetting->SetupPref.DmfMixLastIdx = 0;
        }
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_DMF_MODE, menu_setup_dmf_mode.Sels[menu_setup_dmf_mode.SelCur]->Val, 0);
        }
    }
    return 0;
}

static int menu_setup_time_init(void)
{
    menu_setup_time.SelNum = 5;
    return 0;
}

static int menu_setup_time_get_tab_str(void)
{
    char str_time[17] = {0};
    UINT16 str_time_uni[17] = {0};

    /** Correct RTC time */
    {
        AMBA_RTC_TIME_SPEC_u TimeSpec = {0};
        AmbaRTC_GetSystemTime(AMBA_TIME_STD_TAI, &TimeSpec);
#if 0
        AmbaPrint("TimeSpec.Calendar.Year = %d",TimeSpec.Calendar.Year);
        AmbaPrint("TimeSpec.Calendar.Month = %d",TimeSpec.Calendar.Month);
        AmbaPrint("TimeSpec.Calendar.DayOfMonth = %d",TimeSpec.Calendar.DayOfMonth);
        AmbaPrint("TimeSpec.Calendar.DayOfWeek = %d",TimeSpec.Calendar.DayOfWeek);
        AmbaPrint("TimeSpec.Calendar.Hour = %d",TimeSpec.Calendar.Hour);
        AmbaPrint("TimeSpec.Calendar.Minute = %d",TimeSpec.Calendar.Minute);
        AmbaPrint("TimeSpec.Calendar.Second = %d",TimeSpec.Calendar.Second);
#endif

        sprintf(str_time, "%04d/%02d/%02d %02d:%02d", TimeSpec.Calendar.Year, TimeSpec.Calendar.Month, TimeSpec.Calendar.Day, TimeSpec.Calendar.Hour, TimeSpec.Calendar.Minute);
    }

    AppUtil_AsciiToUnicode(str_time, str_time_uni);
    AppLibGraph_UpdateStringContext(0, STR_TIME_VALUE, str_time_uni);

    return STR_TIME_VALUE;
}

static int menu_setup_time_set(void)
{
    AppMenuTime_SetItem(MENU_SETUP, MENU_SETUP_TIME);
    AppWidget_On(WIDGET_MENU_TIME, 0);
    return 0;
}

static int menu_setup_date_time_disp_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_setup_date_time_disp.Flags, MENU_ITEM_FLAGS_INIT);
    menu_setup_date_time_disp.SelSaved = 0;
    menu_setup_date_time_disp.SelNum = 0;
    menu_setup_date_time_disp.SelCur = 0;
    for (i=0; i<MENU_SETUP_DATE_TIME_DISPLAY_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_setup_item_sel_tbls[MENU_SETUP_DATE_TIME_DISPLAY]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_setup_date_time_disp_sels[menu_setup_date_time_disp.SelNum] = menu_setup_item_sel_tbls[MENU_SETUP_DATE_TIME_DISPLAY]+i;
            if (menu_setup_date_time_disp_sels[menu_setup_date_time_disp.SelNum]->Val == 0/*UserSetting->date_time_display*/) {
                menu_setup_date_time_disp.SelSaved = menu_setup_date_time_disp.SelNum;
                menu_setup_date_time_disp.SelCur = menu_setup_date_time_disp.SelNum;
            }
            menu_setup_date_time_disp.SelNum++;
        }
    }

    return 0;
}

static int menu_setup_date_time_disp_get_tab_str(void)
{
    return menu_setup_date_time_disp_sels[menu_setup_date_time_disp.SelSaved]->Str;
}

static int menu_setup_date_time_disp_get_sel_str(int ref)
{
    return menu_setup_date_time_disp_sels[ref]->Str;
}

static int menu_setup_date_time_disp_get_sel_bmp(int ref)
{
    return menu_setup_date_time_disp_sels[ref]->Bmp;
}

static int menu_setup_date_time_disp_set(void)
{
    AppMenuQuick_SetItem(MENU_SETUP, MENU_SETUP_DATE_TIME_DISPLAY);
    return 0;
}

static int menu_setup_date_time_disp_sel_set(void)
{
    return 0;
}

static int menu_setup_format_init(void)
{
    return 0;
}

static int menu_setup_format_get_tab_str(void)
{
    return STR_FORMAT;
}

static int menu_setup_format_set(void)
{
    int rval = 0;

    rval = AppLibCard_CheckStatus(CARD_CHECK_DELETE);
    if (rval == CARD_STATUS_NO_CARD) {
        AmbaPrintColor(RED,"WARNING_NO_CARD");
    } else {
        rval = AppUtil_SwitchApp(APP_MISC_FORMATCARD);
    }

    return rval;
}

static int menu_setup_default_init(void)
{
    return 0;
}

static int menu_setup_default_get_tab_str(void)
{
    return STR_DEFAULT_SETTING;
}

static int menu_setup_default_set(void)
{
    int rval = 0;

    rval = AppUtil_SwitchApp(APP_MISC_DEFSETTING);

    return rval;
}

static int menu_setup_lcd_ctrl_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_setup_lcd_ctrl.Flags, MENU_ITEM_FLAGS_INIT);
    menu_setup_lcd_ctrl_sel_tbl[MENU_SETUP_LCD_CONTROL_BRIGHTNESS].Val = UserSetting->SetupPref.LcdBrightness;
    menu_setup_lcd_ctrl_sel_tbl[MENU_SETUP_LCD_CONTROL_CONTRAST].Val = UserSetting->SetupPref.LcdContrast;
    menu_setup_lcd_ctrl_sel_tbl[MENU_SETUP_LCD_CONTROL_COLOR_BALANCE].Val = UserSetting->SetupPref.LcdColorBalance;
    for (i=0; i<MENU_SETUP_LCD_CONTROL_SEL_NUM; i++) {
        menu_setup_lcd_ctrl_sels[i] = menu_setup_item_sel_tbls[MENU_SETUP_LCD_CONTROL]+i;
    }
    menu_setup_lcd_ctrl.SelNum = MENU_SETUP_LCD_CONTROL_SEL_NUM;
    return 0;
}

static int menu_setup_lcd_ctrl_get_tab_str(void)
{
    return STR_LCD_CONTROL;
}

static int menu_setup_lcd_ctrl_get_sel_str(int ref)
{
    return menu_setup_lcd_ctrl_sels[ref]->Str;
}

static int menu_setup_lcd_ctrl_get_sel_bmp(int ref)
{
    return menu_setup_lcd_ctrl_sels[ref]->Bmp;
}

static int menu_setup_lcd_ctrl_set(void)
{
    AppMenuQuick_SetItem(MENU_SETUP, MENU_SETUP_LCD_CONTROL);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_setup_lcd_ctrl_sel_set(void)
{
    menu_setup_lcd_ctrl.SelSaved = menu_setup_lcd_ctrl.SelCur;
    AppMenuAdj_SetSel(MENU_SETUP, MENU_SETUP_LCD_CONTROL, menu_setup_lcd_ctrl_sels[menu_setup_lcd_ctrl.SelCur]->Id);
    AppWidget_On(WIDGET_MENU_ADJ, 0);
    return 0;
}

static int menu_setup_lcd_brightness_get_cur_val(void)
{
    return UserSetting->SetupPref.LcdBrightness;
}

static int menu_setup_lcd_brightness_get_val_str(int val)
{
    char Str[4] = {0};
    UINT16 str_madj_value[4] = {0};

    sprintf(Str, "%3d", val);
    //gui_ascii_to_unicode(Str, str_madj_value);
    AppLibGraph_UpdateStringContext(0, STR_ADJUST_VALUE, str_madj_value);

    return STR_ADJUST_VALUE;
}

static int menu_setup_lcd_brightness_val_set(int val)
{
    UserSetting->SetupPref.LcdBrightness = val;
    return AppLibSysLcd_SetBrightness(LCD_CH_DCHAN, val, 0);
}

static int menu_setup_lcd_contrast_get_cur_val(void)
{
    return UserSetting->SetupPref.LcdContrast;
}

static int menu_setup_lcd_contrast_get_val_str(int val)
{
    char Str[4] = {0};
    UINT16 str_madj_value[4] = {0};

    sprintf(Str, "%3d", val);
    //gui_ascii_to_unicode(Str, str_madj_value);
    AppLibGraph_UpdateStringContext(0, STR_ADJUST_VALUE, str_madj_value);

    return STR_ADJUST_VALUE;
}

static int menu_setup_lcd_contrast_val_set(int val)
{
    UserSetting->SetupPref.LcdContrast = val;
    return AppLibSysLcd_SetContrast(LCD_CH_DCHAN, val, 0);
}

static int menu_setup_lcd_colorbalance_get_cur_val(void)
{
    return UserSetting->SetupPref.LcdColorBalance;
}

static int menu_setup_lcd_colorbalance_get_val_str(int val)
{
    char Str[4] = {0};
    UINT16 str_madj_value[4] = {0};

    sprintf(Str, "%3d", val);
    //gui_ascii_to_unicode(Str, str_madj_value);
    AppLibGraph_UpdateStringContext(0, STR_ADJUST_VALUE, str_madj_value);

    return STR_ADJUST_VALUE;
}

static int menu_setup_lcd_colorbalance_val_set(int Val)
{
    UserSetting->SetupPref.LcdColorBalance = Val;
    return 0;//AppLibSysLcd_SetColorBalance(LCD_CH_DCHAN, Val);
}

static int menu_setup_poweroff_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_setup_poweroff.Flags, MENU_ITEM_FLAGS_INIT);
    menu_setup_poweroff.SelSaved = 0;
    menu_setup_poweroff.SelNum = 0;
    menu_setup_poweroff.SelCur = 0;
    for (i=0; i<MENU_SETUP_POWEROFF_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_setup_item_sel_tbls[MENU_SETUP_POWEROFF]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_setup_poweroff_sels[menu_setup_poweroff.SelNum] = menu_setup_item_sel_tbls[MENU_SETUP_POWEROFF]+i;
            if (menu_setup_poweroff_sels[menu_setup_poweroff.SelNum]->Val == UserSetting->SetupPref.AutoPoweroff) {
                menu_setup_poweroff.SelSaved = menu_setup_poweroff.SelNum;
                menu_setup_poweroff.SelCur = menu_setup_poweroff.SelNum;
            }
            menu_setup_poweroff.SelNum++;
        }
    }

    return 0;
}

static int menu_setup_poweroff_get_tab_str(void)
{
    return menu_setup_poweroff_sels[menu_setup_poweroff.SelSaved]->Str;
}

static int menu_setup_poweroff_get_sel_str(int ref)
{
    return menu_setup_poweroff_sels[ref]->Str;
}

static int menu_setup_poweroff_get_sel_bmp(int ref)
{
    return menu_setup_poweroff_sels[ref]->Bmp;
}

static int menu_setup_poweroff_set(void)
{
    AppMenuQuick_SetItem(MENU_SETUP, MENU_SETUP_POWEROFF);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_setup_poweroff_sel_set(void)
{
    UserSetting->SetupPref.AutoPoweroff = (APP_PREF_AUTO_POWEROFF_e)menu_setup_poweroff_sels[menu_setup_poweroff.SelCur]->Val;
    menu_setup_poweroff.SelSaved = menu_setup_poweroff.SelCur;
    //app_util_auto_poweroff_init(0);
    //app_util_powersaving_init(0);
    return 0;
}

static int menu_setup_powersaving_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_setup_powersaving.Flags, MENU_ITEM_FLAGS_INIT);
    menu_setup_powersaving.SelSaved = 0;
    menu_setup_powersaving.SelNum = 0;
    menu_setup_powersaving.SelCur = 0;
    for (i=0; i<MENU_SETUP_POWERSAVING_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_setup_item_sel_tbls[MENU_SETUP_POWERSAVING]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_setup_powersaving_sels[menu_setup_powersaving.SelNum] = menu_setup_item_sel_tbls[MENU_SETUP_POWERSAVING]+i;
            if (menu_setup_powersaving_sels[menu_setup_powersaving.SelNum]->Val == UserSetting->SetupPref.PowerSaving) {
                menu_setup_powersaving.SelSaved = menu_setup_powersaving.SelNum;
                menu_setup_powersaving.SelCur = menu_setup_powersaving.SelNum;
            }
            menu_setup_powersaving.SelNum++;
        }
    }

    return 0;
}

static int menu_setup_powersaving_get_tab_str(void)
{
    return menu_setup_powersaving_sels[menu_setup_powersaving.SelSaved]->Str;
}

static int menu_setup_powersaving_get_sel_str(int ref)
{
    return menu_setup_powersaving_sels[ref]->Str;
}

static int menu_setup_powersaving_get_sel_bmp(int ref)
{
    return menu_setup_powersaving_sels[ref]->Bmp;
}

static int menu_setup_powersaving_set(void)
{
    AppMenuQuick_SetItem(MENU_SETUP, MENU_SETUP_POWERSAVING);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_setup_powersaving_sel_set(void)
{
    UserSetting->SetupPref.PowerSaving = (APP_PREF_POWER_SAVING_e)menu_setup_powersaving_sels[menu_setup_powersaving.SelCur]->Val;
    menu_setup_powersaving.SelSaved = menu_setup_powersaving.SelCur;
    //app_util_powersaving_init(0);
    return 0;
}

static int menu_setup_hdmi_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_setup_hdmi.Flags, MENU_ITEM_FLAGS_INIT);
    menu_setup_hdmi.SelSaved = 0;
    menu_setup_hdmi.SelNum = 0;
    menu_setup_hdmi.SelCur = 0;
    for (i=0; i<MENU_SETUP_HDMI_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_setup_item_sel_tbls[MENU_SETUP_HDMI]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_setup_hdmi_sels[menu_setup_hdmi.SelNum] = menu_setup_item_sel_tbls[MENU_SETUP_HDMI]+i;
            if (menu_setup_hdmi_sels[menu_setup_hdmi.SelNum]->Val == UserSetting->SetupPref.EnableHDMI) {
                menu_setup_hdmi.SelSaved = menu_setup_hdmi.SelNum;
                menu_setup_hdmi.SelCur = menu_setup_hdmi.SelNum;
            }
            menu_setup_hdmi.SelNum++;
        }
    }

    return 0;
}

static int menu_setup_hdmi_get_tab_str(void)
{
    return menu_setup_hdmi_sels[menu_setup_hdmi.SelSaved]->Str;
}

static int menu_setup_hdmi_get_sel_str(int ref)
{
    return menu_setup_hdmi_sels[ref]->Str;
}

static int menu_setup_hdmi_get_sel_bmp(int ref)
{
    return menu_setup_hdmi_sels[ref]->Bmp;
}

static int menu_setup_hdmi_set(void)
{
    AppMenuQuick_SetItem(MENU_SETUP, MENU_SETUP_HDMI);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_setup_hdmi_sel_set(void)
{
    menu_setup_hdmi.SelSaved = menu_setup_hdmi.SelCur;

    if ( UserSetting->SetupPref.EnableHDMI != menu_setup_hdmi_sels[menu_setup_hdmi.SelCur]->Val) {
        UserSetting->SetupPref.EnableHDMI = (APP_PREF_HDMI_SUPPORT_e)menu_setup_hdmi_sels[menu_setup_hdmi.SelCur]->Val;
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            //curapp->OnMessage(AMSG_CMD_RESET_HDMI_PREVIEW, UserSetting->SetupPref.EnableHDMI, 0);
        }

        if (app_status.HdmiPluginFlag == 1) {
            app_status.HdmiPluginFlag = 0;
            AppLibSysVout_SetJackHDMI(0);
            AppLibComSvcHcmgr_SendMsg(HMSG_HDMI_INSERT_SET, 0, 0);
        }
    }
    return 0;
}

#if defined (FLASHLIGHT)
static int menu_setup_flashlight_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_setup_flashlight.Flags, MENU_ITEM_FLAGS_INIT);
    menu_setup_flashlight.SelSaved = 0;
    menu_setup_flashlight.SelNum = 0;
    menu_setup_flashlight.SelCur = 0;
    for (i=0; i<MENU_SETUP_FLASHLIGHT_SEL_NUM; i++) {
    if (APP_CHECKFLAGS((menu_setup_item_sel_tbls[MENU_SETUP_FLASHLIGHT]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
        menu_setup_flashlight_sels[menu_setup_flashlight.SelNum] = menu_setup_item_sel_tbls[MENU_SETUP_FLASHLIGHT]+i;
        if (menu_setup_flashlight_sels[menu_setup_flashlight.SelNum]->Val == UserSetting->SetupPref.Flashlight) {
            menu_setup_flashlight.SelSaved = menu_setup_flashlight.SelNum;
            menu_setup_flashlight.SelCur = menu_setup_flashlight.SelNum;
        }
        menu_setup_flashlight.SelNum++;
        }
    }

    return 0;
}

static int menu_setup_flashlight_get_tab_str(void)
{
    return menu_setup_flashlight_sels[menu_setup_flashlight.SelSaved]->Str;
}

static int menu_setup_flashlight_get_sel_str(int ref)
{
    return menu_setup_flashlight_sels[ref]->Str;
}

static int menu_setup_flashlight_get_sel_bmp(int ref)
{
    return menu_setup_flashlight_sels[ref]->Bmp;
}

static int menu_setup_flashlight_set(void)
{
    AppMenuQuick_SetItem(MENU_SETUP, MENU_SETUP_FLASHLIGHT);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_setup_flashlight_sel_set(void)
{
    if (menu_setup_flashlight.SelSaved != menu_setup_flashlight.SelCur) {
        menu_setup_flashlight.SelSaved = menu_setup_flashlight.SelCur;
        UserSetting->SetupPref.Flashlight = menu_setup_flashlight_sels[menu_setup_flashlight.SelCur]->Val;
        app_image_setting_set_flashlight(menu_setup_flashlight_sels[menu_setup_flashlight.SelCur]->Val);
        app_image_setup_flashlight();
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_FLASHLIGHT, UserSetting->SetupPref.Flashlight, 0);
        }
    }
    return 0;
}
#else
static int menu_setup_flashlight_init(void)
{
    return 0;
}

static int menu_setup_flashlight_get_tab_str(void)
{
    return 0;
}

static int menu_setup_flashlight_get_sel_str(int ref)
{
    return 0;
}

static int menu_setup_flashlight_get_sel_bmp(int ref)
{
    return 0;
}

static int menu_setup_flashlight_set(void)
{
    return 0;
}

static int menu_setup_flashlight_sel_set(void)
{
    return 0;
}

#endif

static int menu_setup_usb_mode_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_setup_usb_mode.Flags, MENU_ITEM_FLAGS_INIT);
    menu_setup_usb_mode.SelSaved = 0;
    menu_setup_usb_mode.SelNum = 0;
    menu_setup_usb_mode.SelCur = 0;
    for (i=0; i<MENU_SETUP_USB_MODE_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_setup_item_sel_tbls[MENU_SETUP_USB_MODE]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_setup_usb_mode_sels[menu_setup_usb_mode.SelNum] = menu_setup_item_sel_tbls[MENU_SETUP_USB_MODE]+i;
            if (menu_setup_usb_mode_sels[menu_setup_usb_mode.SelNum]->Val == UserSetting->SetupPref.USBMode) {
                menu_setup_usb_mode.SelSaved = menu_setup_usb_mode.SelNum;
                menu_setup_usb_mode.SelCur = menu_setup_usb_mode.SelNum;
            }
            menu_setup_usb_mode.SelNum++;
        }
    }

    return 0;
}

static int menu_setup_usb_mode_get_tab_str(void)
{
    return menu_setup_usb_mode_sels[menu_setup_usb_mode.SelSaved]->Str;
}

static int menu_setup_usb_mode_get_sel_str(int ref)
{
    return menu_setup_usb_mode_sels[ref]->Str;
}

static int menu_setup_usb_mode_get_sel_bmp(int ref)
{
    return menu_setup_usb_mode_sels[ref]->Bmp;
}

static int menu_setup_usb_mode_set(void)
{
    AppMenuQuick_SetItem(MENU_SETUP, MENU_SETUP_USB_MODE);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_setup_usb_mode_sel_set(void)
{
    if (menu_setup_usb_mode.SelSaved != menu_setup_usb_mode.SelCur) {
        menu_setup_usb_mode.SelSaved = menu_setup_usb_mode.SelCur;
        UserSetting->SetupPref.USBMode = (APP_PREF_USB_MODE_e)menu_setup_usb_mode_sels[menu_setup_usb_mode.SelCur]->Val;

        AppLibComSvcHcmgr_SendMsg(AMSG_CMD_SET_USB_MODE, 0, 0);
    }
    return 0;
}

static int menu_setup_input_dimension_mode_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_setup_input_dimension_mode.Flags, MENU_ITEM_FLAGS_INIT);
    menu_setup_input_dimension_mode.SelSaved = 0;
    menu_setup_input_dimension_mode.SelNum = 0;
    menu_setup_input_dimension_mode.SelCur = 0;
    for (i=0; i<MENU_SETUP_INPUT_DIMENSION_MODE_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_setup_item_sel_tbls[MENU_SETUP_INPUT_DIMENSION_MODE]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_setup_input_dimension_mode_sels[menu_setup_input_dimension_mode.SelNum] = menu_setup_item_sel_tbls[MENU_SETUP_INPUT_DIMENSION_MODE]+i;
            if (menu_setup_input_dimension_mode_sels[menu_setup_input_dimension_mode.SelNum]->Val == UserSetting->SetupPref.InputDimension) {
                menu_setup_input_dimension_mode.SelSaved = menu_setup_input_dimension_mode.SelNum;
                menu_setup_input_dimension_mode.SelCur = menu_setup_input_dimension_mode.SelNum;
            }
            menu_setup_input_dimension_mode.SelNum++;
        }
    }

    return 0;
}
static int menu_setup_input_dimension_mode_get_tab_str(void)
{
    return menu_setup_input_dimension_mode_sels[menu_setup_input_dimension_mode.SelSaved]->Str;
}
static int menu_setup_input_dimension_mode_get_sel_str(int ref)
{
    return menu_setup_input_dimension_mode_sels[ref]->Str;
}
static int menu_setup_input_dimension_mode_get_sel_bmp(int ref)
{
    return menu_setup_input_dimension_mode_sels[ref]->Bmp;
}
static int menu_setup_input_dimension_mode_set(void)
{
    AppMenuQuick_SetItem(MENU_SETUP, MENU_SETUP_INPUT_DIMENSION_MODE);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}
static int menu_setup_input_dimension_mode_sel_set(void)
{
     return 0;
}

static int menu_setup_output_dimension_mode_init(void)
{
    return 0;
}
static int menu_setup_output_dimension_mode_get_tab_str(void)
{
    return menu_setup_output_dimension_mode_sels[menu_setup_output_dimension_mode.SelSaved]->Str;
}
static int menu_setup_output_dimension_mode_get_sel_str(int ref)
{
    return menu_setup_output_dimension_mode_sels[ref]->Str;
}
static int menu_setup_output_dimension_mode_get_sel_bmp(int ref)
{
    return menu_setup_output_dimension_mode_sels[ref]->Bmp;
}
static int menu_setup_output_dimension_mode_set(void)
{
    AppMenuQuick_SetItem(MENU_SETUP, MENU_SETUP_OUTPUT_DIMENSION_MODE);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}
static int menu_setup_output_dimension_mode_sel_set(void)
{
    if (menu_setup_output_dimension_mode.SelSaved != menu_setup_output_dimension_mode.SelCur) {
        menu_setup_output_dimension_mode.SelSaved = menu_setup_output_dimension_mode.SelCur;
        UserSetting->SetupPref.OutputDimension = menu_setup_output_dimension_mode_sels[menu_setup_output_dimension_mode.SelCur]->Val;
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            //curapp->OnMessage(AMSG_CMD_SET_OUTPUT_DIMENSION, 0, 0);
        }
    }
    return 0;
}

static int menu_setup_wifi_init(void)
{
    return 0;
}

static int menu_setup_wifi_get_tab_str(void)
{
    return 0;
}

static int menu_setup_wifi_get_sel_str(int ref)
{
    return 0;
}

static int menu_setup_wifi_get_sel_bmp(int ref)
{
    return 0;
}

static int menu_setup_wifi_set(void)
{
    return 0;
}

static int menu_setup_wifi_sel_set(void)
{
    return 0;
}

static int menu_setup_wifi_config_init(void)
{
    return 0;
}

static int menu_setup_wifi_config_get_tab_str(void)
{
    return 0;
}

static int menu_setup_wifi_config_get_sel_str(int ref)
{
    return 0;
}

static int menu_setup_wifi_config_get_sel_bmp(int ref)
{
    return 0;
}

static int menu_setup_wifi_config_set(void)
{
    return 0;
}

static int menu_setup_wifi_config_sel_set(void)
{
    return 0;
}

/**
 * @brief Initialization function of menu_setup_version
 * @return success or not
 */
static int menu_setup_version_init(void)
{
#if 0
    char *ver_br_name, *ver_br_svn;

    // output
    const int output_str_len = 64;
    UINT16 output_str[output_str_len] = {0};

    // branch version
    const int uni_ver_len = 14; // strlen(': SVN r') + strlen('0123456')
    UINT16 uni_ver[uni_ver_len];

    // branch name
    int uni_name_len = output_str_len - uni_ver_len;

    // tmp
    int tmp_len = 0;
    UINT16 ver_uni_tmp[16];

    APP_ADDFLAGS(menu_setup_version.Flags, MENU_ITEM_FLAGS_INIT);

    // Get SVN info
    ver_br_name = get_branch_name("prkapp");
    ver_br_svn = get_branch_ver("prkapp");

    // Get SVN number
    gui_ascii_to_unicode(": SVN r", uni_ver);
    gui_ascii_to_unicode(ver_br_svn, ver_uni_tmp);
    w_strcat(uni_ver, ver_uni_tmp);

    // Generate string
    tmp_len = gui_ascii_to_unicode(ver_br_name, ver_uni_tmp);

    if (tmp_len < uni_name_len) {
        gui_ascii_to_unicode(ver_br_name, output_str);
    } else {
        memcpy(output_str, ver_uni_tmp, (uni_name_len-1)*sizeof(UINT16));
        output_str[uni_name_len] = 0;
        AmbaPrint("Warning! SVN branch: %s", ver_br_name);
    }

    gui_ascii_to_unicode(": SVN r", ver_uni_tmp);
    w_strcat(output_str, ver_uni_tmp);
    gui_ascii_to_unicode(ver_br_svn, ver_uni_tmp);
    w_strcat(output_str, ver_uni_tmp);

    // Update string
    AppLibGraph_UpdateStringContext(0, STR_SVN_VERSION, output_str);
#endif
    return 0;
}

/**
 * @brief Function to get string Id to display on main menu
 * @return int String ID
 */
static int menu_setup_version_get_tab_str(void)
{
    return STR_SVN_VERSION;
}

/**
 * @brief Dummy function to handle SET operation
 * @return Don't care
 */
static int menu_setup_version_set(void)
{
    return 0;
}

// control

static MENU_TAB_s* menu_setup_get_tab(void)
{
    return &menu_setup;
}

static MENU_ITEM_s* menu_setup_get_item(UINT32 itemId)
{
    return menu_setup_item_tbl[itemId];
}

static MENU_SEL_s* menu_setup_get_sel(UINT32 itemId, UINT32 selId)
{
    return &menu_setup_item_sel_tbls[itemId][selId];
}

static int menu_setup_set_sel_table(UINT32 itemId, MENU_SEL_s *selTbl)
{
    menu_setup_item_sel_tbls[itemId] = selTbl;
    APP_REMOVEFLAGS(menu_setup_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_INIT);
    return 0;
}

static int menu_setup_lock_tab(void)
{
    APP_ADDFLAGS(menu_setup.Flags, MENU_TAB_FLAGS_LOCKED);
    return 0;
}

static int menu_setup_unlock_tab(void)
{
    APP_REMOVEFLAGS(menu_setup.Flags, MENU_TAB_FLAGS_LOCKED);
    return 0;
}

static int menu_setup_enable_item(UINT32 itemId)
{
    if (!APP_CHECKFLAGS(menu_setup_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_ENABLE)) {
        APP_ADDFLAGS(menu_setup_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_ENABLE);
        APP_REMOVEFLAGS(menu_setup.Flags, MENU_TAB_FLAGS_INIT);
    }
    return 0;
}

static int menu_setup_disable_item(UINT32 itemId)
{
    if (APP_CHECKFLAGS(menu_setup_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_ENABLE)) {
        APP_REMOVEFLAGS(menu_setup_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_ENABLE);
        APP_REMOVEFLAGS(menu_setup.Flags, MENU_TAB_FLAGS_INIT);
    }
    return 0;
}

static int menu_setup_lock_item(UINT32 itemId)
{
    APP_ADDFLAGS(menu_setup_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_LOCKED);
    return 0;
}

static int menu_setup_unlock_item(UINT32 itemId)
{
    APP_REMOVEFLAGS(menu_setup_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_LOCKED);
    return 0;
}

static int menu_setup_enable_sel(UINT32 itemId, UINT32 selId)
{
    if (!APP_CHECKFLAGS((menu_setup_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_ENABLE)) {
        APP_ADDFLAGS((menu_setup_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_ENABLE);
        APP_REMOVEFLAGS(menu_setup_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_INIT);
    }
    return 0;
}

static int menu_setup_disable_sel(UINT32 itemId, UINT32 selId)
{
    if (APP_CHECKFLAGS((menu_setup_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_ENABLE)) {
        APP_REMOVEFLAGS((menu_setup_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_ENABLE);
        APP_REMOVEFLAGS(menu_setup_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_INIT);
    }
    return 0;
}

static int menu_setup_lock_sel(UINT32 itemId, UINT32 selId)
{
    APP_ADDFLAGS((menu_setup_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_LOCKED);
    return 0;
}

static int menu_setup_unlock_sel(UINT32 itemId, UINT32 selId)
{
    APP_REMOVEFLAGS((menu_setup_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_LOCKED);
    return 0;
}
