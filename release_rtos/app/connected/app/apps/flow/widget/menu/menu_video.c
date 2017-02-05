/**
  * @file src/app/apps/flow/widget/menu/connectedcam/menu_video.c
  *
  * Implementation of Video-related Menu Items
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
#include <apps/apps.h>
#include <system/status.h>
#include <system/app_pref.h>
#include <system/app_util.h>
#include <apps/flow/widget/menu/widget.h>
#include <apps/gui/resource/gui_settle.h>
#include <apps/flow/widget/menu/widget.h>
#include <apps/gui/utility/gui_utility.h>

/*************************************************************************
 * Declaration:
 ************************************************************************/
// tab
static int menu_video_init(void);
static int menu_video_start(void);
static int menu_video_stop(void);
// item
static int menu_video_sensor_res_init(void);
static int menu_video_sensor_res_get_tab_str(void);
static int menu_video_sensor_res_get_sel_str(int ref);
static int menu_video_sensor_res_get_sel_bmp(int ref);
static int menu_video_sensor_res_set(void);
static int menu_video_sensor_res_sel_set(void);
static int menu_video_yuv_res_init(void);
static int menu_video_yuv_res_get_tab_str(void);
static int menu_video_yuv_res_get_sel_str(int ref);
static int menu_video_yuv_res_get_sel_bmp(int ref);
static int menu_video_yuv_res_set(void);
static int menu_video_yuv_res_sel_set(void);
static int menu_video_quality_init(void);
static int menu_video_quality_get_tab_str(void);
static int menu_video_quality_get_sel_str(int ref);
static int menu_video_quality_get_sel_bmp(int ref);
static int menu_video_quality_set(void);
static int menu_video_quality_sel_set(void);
static int menu_video_selftimer_init(void);
static int menu_video_selftimer_get_tab_str(void);
static int menu_video_selftimer_get_sel_str(int ref);
static int menu_video_selftimer_get_sel_bmp(int ref);
static int menu_video_selftimer_set(void);
static int menu_video_selftimer_sel_set(void);
static int menu_video_pre_record_init(void);
static int menu_video_pre_record_get_tab_str(void);
static int menu_video_pre_record_get_sel_str(int ref);
static int menu_video_pre_record_get_sel_bmp(int ref);
static int menu_video_pre_record_set(void);
static int menu_video_pre_record_sel_set(void);
static int menu_video_time_lapse_init(void);
static int menu_video_time_lapse_get_tab_str(void);
static int menu_video_time_lapse_get_sel_str(int ref);
static int menu_video_time_lapse_get_sel_bmp(int ref);
static int menu_video_time_lapse_set(void);
static int menu_video_time_lapse_sel_set(void);
static int menu_video_dual_streams_init(void);
static int menu_video_dual_streams_get_tab_str(void);
static int menu_video_dual_streams_get_sel_str(int ref);
static int menu_video_dual_streams_get_sel_bmp(int ref);
static int menu_video_dual_streams_set(void);
static int menu_video_dual_streams_sel_set(void);

static int menu_video_streams_type_init(void);
static int menu_video_streams_type_get_tab_str(void);
static int menu_video_streams_type_get_sel_str(int ref);
static int menu_video_streams_type_get_sel_bmp(int ref);
static int menu_video_streams_type_set(void);
static int menu_video_streams_type_sel_set(void);

static int menu_video_streaming_init(void);
static int menu_video_streaming_get_tab_str(void);
static int menu_video_streaming_get_sel_str(int ref);
static int menu_video_streaming_get_sel_bmp(int ref);
static int menu_video_streaming_set(void);
static int menu_video_streaming_sel_set(void);

static int menu_video_digital_zoom_init(void);
static int menu_video_digital_zoom_get_tab_str(void);
static int menu_video_digital_zoom_get_sel_str(int ref);
static int menu_video_digital_zoom_get_sel_bmp(int ref);
static int menu_video_digital_zoom_set(void);
static int menu_video_digital_zoom_sel_set(void);
static int menu_video_stamp_init(void);
static int menu_video_stamp_get_tab_str(void);
static int menu_video_stamp_get_sel_str(int ref);
static int menu_video_stamp_get_sel_bmp(int ref);
static int menu_video_stamp_set(void);
static int menu_video_stamp_sel_set(void);
static int menu_video_rec_mode_init(void);
static int menu_video_rec_mode_get_tab_str(void);
static int menu_video_rec_mode_get_sel_str(int ref);
static int menu_video_rec_mode_get_sel_bmp(int ref);
static int menu_video_rec_mode_set(void);
static int menu_video_rec_mode_sel_set(void);
static int menu_video_adas_init(void);
static int menu_video_adas_get_tab_str(void);
static int menu_video_adas_get_sel_str(int ref);
static int menu_video_adas_get_sel_bmp(int ref);
static int menu_video_adas_set(void);
static int menu_video_adas_sel_set(void);
static int menu_video_adas_calibration_init(void);
static int menu_video_adas_calibration_get_tab_str(void);
static int menu_video_adas_calibration_get_sel_str(int ref);
static int menu_video_adas_calibration_get_sel_bmp(int ref);
static int menu_video_adas_calibration_set(void);
static int menu_video_adas_calibration_sel_set(void);
// control
static MENU_TAB_s* menu_video_get_tab(void);
static MENU_ITEM_s* menu_video_get_item(UINT32 itemId);
static MENU_SEL_s* menu_video_get_sel(UINT32 itemId, UINT32 selId);
static int menu_video_set_sel_table(UINT32 itemId, MENU_SEL_s *selTbl);
static int menu_video_lock_tab(void);
static int menu_video_unlock_tab(void);
static int menu_video_enable_item(UINT32 itemId);
static int menu_video_disable_item(UINT32 itemId);
static int menu_video_lock_item(UINT32 itemId);
static int menu_video_unlock_item(UINT32 itemId);
static int menu_video_enable_sel(UINT32 itemId, UINT32 selId);
static int menu_video_disable_sel(UINT32 itemId, UINT32 selId);
static int menu_video_lock_sel(UINT32 itemId, UINT32 selId);
static int menu_video_unlock_sel(UINT32 itemId, UINT32 selId);

/*************************************************************************
 * Definition:
 ************************************************************************/
/*** Selection ***/
/**
* video mres selection table is in sensor config
**/
static MENU_SEL_s menu_video_sensor_res_sel_tbl[MENU_VIDEO_SENSOR_RES_SEL_NUM] = {
    {MENU_VIDEO_SENSOR_RES_1, 0,
        STR_VIDEO_RES_1, 0, 0,
        MENU_VIDEO_SENSOR_RES_1, NULL},
    {MENU_VIDEO_SENSOR_RES_2, 0,
        STR_VIDEO_RES_2, 0, 0,
        MENU_VIDEO_SENSOR_RES_2, NULL},
    {MENU_VIDEO_SENSOR_RES_3, 0,
        STR_VIDEO_RES_3, 0, 0,
        MENU_VIDEO_SENSOR_RES_3, NULL},
    {MENU_VIDEO_SENSOR_RES_4, 0,
        STR_VIDEO_RES_4, 0, 0,
        MENU_VIDEO_SENSOR_RES_4, NULL},
    {MENU_VIDEO_SENSOR_RES_5, 0,
        STR_VIDEO_RES_5, 0, 0,
        MENU_VIDEO_SENSOR_RES_5, NULL},
    {MENU_VIDEO_SENSOR_RES_6, 0,
        STR_VIDEO_RES_6, 0, 0,
        MENU_VIDEO_SENSOR_RES_6, NULL},
    {MENU_VIDEO_SENSOR_RES_7, 0,
        STR_VIDEO_RES_7, 0, 0,
        MENU_VIDEO_SENSOR_RES_7, NULL},
    {MENU_VIDEO_SENSOR_RES_8, 0,
        STR_VIDEO_RES_8, 0, 0,
        MENU_VIDEO_SENSOR_RES_8, NULL},
    {MENU_VIDEO_SENSOR_RES_9, 0,
        STR_VIDEO_RES_9, 0, 0,
        MENU_VIDEO_SENSOR_RES_9, NULL},
    {MENU_VIDEO_SENSOR_RES_10, 0,
        STR_VIDEO_RES_10, 0, 0,
        MENU_VIDEO_SENSOR_RES_10, NULL},
    {MENU_VIDEO_SENSOR_RES_11, 0,
        STR_VIDEO_RES_11, 0, 0,
        MENU_VIDEO_SENSOR_RES_11, NULL},
    {MENU_VIDEO_SENSOR_RES_12, 0,
        STR_VIDEO_RES_12, 0, 0,
        MENU_VIDEO_SENSOR_RES_12, NULL},
    {MENU_VIDEO_SENSOR_RES_13, 0,
        STR_VIDEO_RES_13, 0, 0,
        MENU_VIDEO_SENSOR_RES_13, NULL},
    {MENU_VIDEO_SENSOR_RES_14, 0,
        STR_VIDEO_RES_14, 0, 0,
        MENU_VIDEO_SENSOR_RES_14, NULL},
    {MENU_VIDEO_SENSOR_RES_15, 0,
        STR_VIDEO_RES_15, 0, 0,
        MENU_VIDEO_SENSOR_RES_15, NULL},
    {MENU_VIDEO_SENSOR_RES_16, 0,
        STR_VIDEO_RES_16, 0, 0,
        MENU_VIDEO_SENSOR_RES_16, NULL},
    {MENU_VIDEO_SENSOR_RES_17, 0,
        STR_VIDEO_RES_17, 0, 0,
        MENU_VIDEO_SENSOR_RES_17, NULL},
    {MENU_VIDEO_SENSOR_RES_18, 0,
        STR_VIDEO_RES_18, 0, 0,
        MENU_VIDEO_SENSOR_RES_18, NULL},
    {MENU_VIDEO_SENSOR_RES_19, 0,
        STR_VIDEO_RES_19, 0, 0,
        MENU_VIDEO_SENSOR_RES_19, NULL},
    {MENU_VIDEO_SENSOR_RES_20, 0,
        STR_VIDEO_RES_20, 0, 0,
        MENU_VIDEO_SENSOR_RES_20, NULL},
    {MENU_VIDEO_SENSOR_RES_21, 0,
        STR_VIDEO_RES_21, 0, 0,
        MENU_VIDEO_SENSOR_RES_21, NULL},
    {MENU_VIDEO_SENSOR_RES_22, 0,
        STR_VIDEO_RES_22, 0, 0,
        MENU_VIDEO_SENSOR_RES_22, NULL},
    {MENU_VIDEO_SENSOR_RES_23, 0,
        STR_VIDEO_RES_23, 0, 0,
        MENU_VIDEO_SENSOR_RES_23, NULL},
    {MENU_VIDEO_SENSOR_RES_24, 0,
        STR_VIDEO_RES_24, 0, 0,
        MENU_VIDEO_SENSOR_RES_24, NULL},
    {MENU_VIDEO_SENSOR_RES_25, 0,
        STR_VIDEO_RES_25, 0, 0,
        MENU_VIDEO_SENSOR_RES_25, NULL},
    {MENU_VIDEO_SENSOR_RES_26, 0,
        STR_VIDEO_RES_26, 0, 0,
        MENU_VIDEO_SENSOR_RES_26, NULL},
    {MENU_VIDEO_SENSOR_RES_27, 0,
        STR_VIDEO_RES_27, 0, 0,
        MENU_VIDEO_SENSOR_RES_27, NULL},
    {MENU_VIDEO_SENSOR_RES_28, 0,
        STR_VIDEO_RES_28, 0, 0,
        MENU_VIDEO_SENSOR_RES_28, NULL},
    {MENU_VIDEO_SENSOR_RES_29, 0,
        STR_VIDEO_RES_29, 0, 0,
        MENU_VIDEO_SENSOR_RES_29, NULL},
    {MENU_VIDEO_SENSOR_RES_30, 0,
        STR_VIDEO_RES_30, 0, 0,
        MENU_VIDEO_SENSOR_RES_30, NULL},
    {MENU_VIDEO_SENSOR_RES_31, 0,
        STR_VIDEO_RES_31, 0, 0,
        MENU_VIDEO_SENSOR_RES_31, NULL},
    {MENU_VIDEO_SENSOR_RES_32, 0,
        STR_VIDEO_RES_32, 0, 0,
        MENU_VIDEO_SENSOR_RES_32, NULL}
};

static MENU_SEL_s menu_video_yuv_res_sel_tbl[MENU_VIDEO_YUV_RES_SEL_NUM] = {
    {MENU_VIDEO_YUV_RES_1, 0,
        STR_VIDEO_RES_1, 0, 0,
        MENU_VIDEO_YUV_RES_1, NULL},
    {MENU_VIDEO_YUV_RES_2, 0,
        STR_VIDEO_RES_2, 0, 0,
        MENU_VIDEO_YUV_RES_2, NULL},
    {MENU_VIDEO_YUV_RES_3, 0,
        STR_VIDEO_RES_3, 0, 0,
        MENU_VIDEO_YUV_RES_3, NULL},
    {MENU_VIDEO_YUV_RES_4, 0,
        STR_VIDEO_RES_4, 0, 0,
        MENU_VIDEO_YUV_RES_4, NULL},
    {MENU_VIDEO_YUV_RES_5, 0,
        STR_VIDEO_RES_5, 0, 0,
        MENU_VIDEO_YUV_RES_5, NULL},
    {MENU_VIDEO_YUV_RES_6, 0,
        STR_VIDEO_RES_6, 0, 0,
        MENU_VIDEO_YUV_RES_6, NULL},
    {MENU_VIDEO_YUV_RES_7, 0,
        STR_VIDEO_RES_7, 0, 0,
        MENU_VIDEO_YUV_RES_7, NULL},
    {MENU_VIDEO_YUV_RES_8, 0,
        STR_VIDEO_RES_8, 0, 0,
        MENU_VIDEO_YUV_RES_8, NULL},
    {MENU_VIDEO_YUV_RES_9, 0,
        STR_VIDEO_RES_9, 0, 0,
        MENU_VIDEO_YUV_RES_9, NULL},
    {MENU_VIDEO_YUV_RES_10, 0,
        STR_VIDEO_RES_10, 0, 0,
        MENU_VIDEO_YUV_RES_10, NULL},
    {MENU_VIDEO_YUV_RES_11, 0,
        STR_VIDEO_RES_11, 0, 0,
        MENU_VIDEO_YUV_RES_11, NULL},
    {MENU_VIDEO_YUV_RES_12, 0,
        STR_VIDEO_RES_12, 0, 0,
        MENU_VIDEO_YUV_RES_12, NULL},
    {MENU_VIDEO_YUV_RES_13, 0,
        STR_VIDEO_RES_13, 0, 0,
        MENU_VIDEO_YUV_RES_13, NULL},
    {MENU_VIDEO_YUV_RES_14, 0,
        STR_VIDEO_RES_14, 0, 0,
        MENU_VIDEO_YUV_RES_14, NULL},
    {MENU_VIDEO_YUV_RES_15, 0,
        STR_VIDEO_RES_15, 0, 0,
        MENU_VIDEO_YUV_RES_15, NULL},
    {MENU_VIDEO_YUV_RES_16, 0,
        STR_VIDEO_RES_16, 0, 0,
        MENU_VIDEO_YUV_RES_16, NULL},
    {MENU_VIDEO_YUV_RES_17, 0,
        STR_VIDEO_RES_17, 0, 0,
        MENU_VIDEO_YUV_RES_17, NULL},
    {MENU_VIDEO_YUV_RES_18, 0,
        STR_VIDEO_RES_18, 0, 0,
        MENU_VIDEO_YUV_RES_18, NULL},
    {MENU_VIDEO_YUV_RES_19, 0,
        STR_VIDEO_RES_19, 0, 0,
        MENU_VIDEO_YUV_RES_19, NULL},
    {MENU_VIDEO_YUV_RES_20, 0,
        STR_VIDEO_RES_20, 0, 0,
        MENU_VIDEO_YUV_RES_20, NULL},
    {MENU_VIDEO_YUV_RES_21, 0,
        STR_VIDEO_RES_21, 0, 0,
        MENU_VIDEO_YUV_RES_21, NULL},
    {MENU_VIDEO_YUV_RES_22, 0,
        STR_VIDEO_RES_22, 0, 0,
        MENU_VIDEO_YUV_RES_22, NULL},
    {MENU_VIDEO_YUV_RES_23, 0,
        STR_VIDEO_RES_23, 0, 0,
        MENU_VIDEO_YUV_RES_23, NULL},
    {MENU_VIDEO_YUV_RES_24, 0,
        STR_VIDEO_RES_24, 0, 0,
        MENU_VIDEO_YUV_RES_24, NULL},
    {MENU_VIDEO_YUV_RES_25, 0,
        STR_VIDEO_RES_25, 0, 0,
        MENU_VIDEO_YUV_RES_25, NULL},
    {MENU_VIDEO_YUV_RES_26, 0,
        STR_VIDEO_RES_26, 0, 0,
        MENU_VIDEO_YUV_RES_26, NULL},
    {MENU_VIDEO_YUV_RES_27, 0,
        STR_VIDEO_RES_27, 0, 0,
        MENU_VIDEO_YUV_RES_27, NULL},
    {MENU_VIDEO_YUV_RES_28, 0,
        STR_VIDEO_RES_28, 0, 0,
        MENU_VIDEO_YUV_RES_28, NULL},
    {MENU_VIDEO_YUV_RES_29, 0,
        STR_VIDEO_RES_29, 0, 0,
        MENU_VIDEO_YUV_RES_29, NULL},
    {MENU_VIDEO_YUV_RES_30, 0,
        STR_VIDEO_RES_30, 0, 0,
        MENU_VIDEO_YUV_RES_30, NULL},
    {MENU_VIDEO_YUV_RES_31, 0,
        STR_VIDEO_RES_31, 0, 0,
        MENU_VIDEO_YUV_RES_31, NULL},
    {MENU_VIDEO_YUV_RES_32, 0,
        STR_VIDEO_RES_32, 0, 0,
        MENU_VIDEO_YUV_RES_32, NULL}
};

static MENU_SEL_s menu_video_quality_sel_tbl[MENU_VIDEO_QUALITY_SEL_NUM] = {
    {MENU_VIDEO_QUALITY_SFINE, MENU_SEL_FLAGS_ENABLE,
        STR_QUALITY_SFINE, BMP_ICN_QUALITY_SF, 0,
        VIDEO_QUALITY_SFINE, NULL},
    {MENU_VIDEO_QUALITY_FINE, MENU_SEL_FLAGS_ENABLE,
        STR_QUALITY_FINE, BMP_ICN_QUALITY_F, 0,
        VIDEO_QUALITY_FINE, NULL},
    {MENU_VIDEO_QUALITY_NORMAL, MENU_SEL_FLAGS_ENABLE,
        STR_QUALITY_NORMAL, BMP_ICN_QUALITY_N, 0,
        VIDEO_QUALITY_NORMAL, NULL}
};

#ifdef VIDEO_TIMELAPES_ENABLE
static MENU_SEL_s menu_video_selftimer_sel_tbl[MENU_VIDEO_SELFTIMER_SEL_NUM] = {
    {MENU_VIDEO_SELFTIMER, MENU_SEL_FLAGS_ENABLE,
        STR_SELF_TIMER_OFF, BMP_ICN_SELF_TIMER_OFF, 0,
        SELF_TIMER_OFF, NULL},
    {MENU_VIDEO_SELFTIMER, MENU_SEL_FLAGS_ENABLE,
        STR_SELF_TIMER_3S, BMP_ICN_SELF_TIMER_3, 0,
        SELF_TIMER_3S, NULL},
    {MENU_VIDEO_SELFTIMER, MENU_SEL_FLAGS_ENABLE,
        STR_SELF_TIMER_5S, BMP_ICN_SELF_TIMER_5, 0,
        SELF_TIMER_5S, NULL},
    {MENU_VIDEO_SELFTIMER, MENU_SEL_FLAGS_ENABLE,
        STR_SELF_TIMER_10S, BMP_ICN_SELF_TIMER_10, 0,
        SELF_TIMER_10S, NULL}
};
#else
static MENU_SEL_s menu_video_selftimer_sel_tbl[MENU_VIDEO_SELFTIMER_SEL_NUM] = {
    {MENU_VIDEO_SELFTIMER, MENU_SEL_FLAGS_ENABLE,
        STR_SELF_TIMER_OFF, 0, 0,
        SELF_TIMER_OFF, NULL},
    {MENU_VIDEO_SELFTIMER, MENU_SEL_FLAGS_ENABLE,
        STR_SELF_TIMER_3S, 0, 0,
        SELF_TIMER_3S, NULL},
    {MENU_VIDEO_SELFTIMER, MENU_SEL_FLAGS_ENABLE,
        STR_SELF_TIMER_5S, 0, 0,
        SELF_TIMER_5S, NULL},
    {MENU_VIDEO_SELFTIMER, MENU_SEL_FLAGS_ENABLE,
        STR_SELF_TIMER_10S, 0, 0,
        SELF_TIMER_10S, NULL}
};
#endif

static MENU_SEL_s menu_video_pre_record_sel_tbl[MENU_VIDEO_PRE_RECORD_SEL_NUM] = {
    {MENU_VIDEO_PRE_RECORD_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_PRE_RECORD_OFF, BMP_ICN_VIDEO_FILM, 0,
        VIDEO_PRE_RECORD_OFF, NULL},
    {MENU_VIDEO_PRE_RECORD_ON, MENU_SEL_FLAGS_ENABLE,
        STR_PRE_RECORD_ON, BMP_ICN_VIDEO_PRE_RECORD, 0,
        VIDEO_PRE_RECORD_ON, NULL}
};

#ifdef VIDEO_TIMELAPES_ENABLE
static MENU_SEL_s menu_video_time_lapse_sel_tbl[MENU_VIDEO_TIME_LAPSE_SEL_NUM] = {
    {MENU_VIDEO_TIME_LAPSE_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_TIME_LAPSE_OFF, BMP_ICN_VIDEO_FILM, 0,
        VIDEO_TIME_LAPSE_OFF, NULL},
    {MENU_VIDEO_TIME_LAPSE_2S, MENU_SEL_FLAGS_ENABLE,
        STR_TIME_LAPSE_2S, BMP_ICN_VIDEO_TIME_LAPSE_2, 0,
        VIDEO_TIME_LAPSE_2S, NULL}
};
#else
static MENU_SEL_s menu_video_time_lapse_sel_tbl[MENU_VIDEO_TIME_LAPSE_SEL_NUM] = {
    {MENU_VIDEO_TIME_LAPSE_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_TIME_LAPSE_OFF, BMP_ICN_VIDEO_FILM, 0,
        VIDEO_TIME_LAPSE_OFF, NULL},
    {MENU_VIDEO_TIME_LAPSE_2S, MENU_SEL_FLAGS_ENABLE,
        STR_TIME_LAPSE_2S, 0, 0,
        VIDEO_TIME_LAPSE_2S, NULL}
};
#endif

static MENU_SEL_s menu_video_dual_streams_sel_tbl[MENU_VIDEO_DUAL_STREAMS_SEL_NUM] = {
    {MENU_VIDEO_DUAL_STREAMS_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_DUAL_STREAMS_OFF, BMP_ICN_VIDEO_FILM, 0,
        VIDEO_DUAL_STREAMS_OFF, NULL},
    {MENU_VIDEO_DUAL_STREAMS_ON, MENU_SEL_FLAGS_ENABLE,
        STR_DUAL_STREAMS_ON, BMP_ICN_VIDEO_DUAL, 0,
        VIDEO_DUAL_STREAMS_ON, NULL}
};

static MENU_SEL_s menu_video_streams_type_sel_tbl[MENU_VIDEO_STREAMS_TYPE_SEL_NUM] = {
    {MENU_VIDEO_STREAMS_TYPE_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_STREAM_TYPE_OFF, BMP_ICN_STREAMING_OFF, 0,
        STREAM_TYPE_OFF, NULL},
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    {MENU_VIDEO_STREAMS_TYPE_RTSP, MENU_SEL_FLAGS_ENABLE,
        STR_STREAM_TYPE_RTSP, BMP_ICN_STREAMING_RTSP, 0,
        STREAM_TYPE_RTSP, NULL},
#endif
#if defined(CONFIG_AMBA_STREAMING)
    {MENU_VIDEO_STREAMS_TYPE_MJPG, MENU_SEL_FLAGS_ENABLE,
        STR_STREAM_TYPE_MJPEG, BMP_ICN_STREAMING_MJPG, 0,
        STREAM_TYPE_MJPEG, NULL},
    {MENU_VIDEO_STREAMS_TYPE_UVC_MJPG, MENU_SEL_FLAGS_ENABLE,
        STR_STREAM_TYPE_UVC, BMP_ICN_STREAMING_MJPG, 0,
        STREAM_TYPE_UVC, NULL},
    {MENU_VIDEO_STREAMS_TYPE_HLS, MENU_SEL_FLAGS_ENABLE,
        STR_STREAM_TYPE_HLS, BMP_ICN_STREAMING_HLS, 0,
        STREAM_TYPE_HLS, NULL}
#endif
};

static MENU_SEL_s menu_video_streaming_sel_tbl[MENU_VIDEO_STREAMING_SEL_NUM] = {
    {MENU_VIDEO_STREAMING_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_STREAMING_OFF, BMP_ICN_VIDEO_FILM, 0,
        STREAMING_OFF, NULL},
    {MENU_VIDEO_STREAMING_ON, MENU_SEL_FLAGS_ENABLE,
        STR_STREAMING_ON, BMP_ICN_VIDEO_DUAL, 0,
        STREAMING_ON, NULL}
};

static MENU_SEL_s menu_video_digital_zoom_sel_tbl[MENU_VIDEO_DIGITAL_ZOOM_SEL_NUM] = {
    {MENU_VIDEO_DIGITAL_ZOOM_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_DIGITAL_ZOOM_OFF, BMP_ICN_DIGITAL_ZOOM_OFF, 0,
        DZOOM_OFF, NULL},
    {MENU_VIDEO_DIGITAL_ZOOM_ON, MENU_SEL_FLAGS_ENABLE,
        STR_DIGITAL_ZOOM_ON, BMP_ICN_DIGITAL_ZOOM_4X, 0,
        DZOOM_4X, NULL},
    {MENU_VIDEO_DIGITAL_ZOOM_NUMX, 0,
        STR_DIGITAL_ZOOM_120X, BMP_ICN_DIGITAL_ZOOM_120X, 0,
        DZOOM_120X, NULL}
};

static MENU_SEL_s menu_video_stamp_sel_tbl[MENU_VIDEO_STAMP_SEL_NUM] = {
    {MENU_VIDEO_STAMP_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_STAMP_OFF, BMP_ICN_STAMP_OFF, 0,
        STAMP_OFF, NULL},
    {MENU_VIDEO_STAMP_DATE, MENU_SEL_FLAGS_ENABLE,
        STR_STAMP_DATE, BMP_ICN_STAMP_DATE, 0,
        STAMP_DATE, NULL},
    {MENU_VIDEO_STAMP_TIME, MENU_SEL_FLAGS_ENABLE,
        STR_STAMP_TIME, BMP_ICN_STAMP_TIME, 0,
        STAMP_TIME, NULL},
    {MENU_VIDEO_STAMP_BOTH, MENU_SEL_FLAGS_ENABLE,
        STR_STAMP_DATE_TIME, BMP_ICN_STAMP_DATE_TIME, 0,
        STAMP_DATE_TIME, NULL}
};

static MENU_SEL_s menu_video_rec_mode_sel_tbl[MENU_VIDEO_REC_MODE_SEL_NUM] = {
    {MENU_VIDEO_REC_MODE_VIDEO_MODE, MENU_SEL_FLAGS_ENABLE,
        STR_VIDEO_MODE, BMP_ICN_VIDEO_FILM, 0,
        MENU_VIDEO_REC_MODE_VIDEO_MODE, NULL},
    {MENU_VIDEO_REC_MODE_STILL_MODE, MENU_SEL_FLAGS_ENABLE,
        STR_PHOTO_MODE, BMP_ICN_PRECISE_QUALITY, 0,
        MENU_VIDEO_REC_MODE_STILL_MODE, NULL}
};

static MENU_SEL_s menu_video_adas_sel_tbl[MENU_VIDEO_ADAS_SEL_NUM] = {
    {MENU_VIDEO_ADAS_OFF, MENU_SEL_FLAGS_ENABLE,
        STR_ADAS_OFF, BMP_ICN_AUTO_POWER_OFF, 0,
        ADAS_OFF, NULL},
    {MENU_VIDEO_ADAS_ON, MENU_SEL_FLAGS_ENABLE,
        STR_ADAS_ON, BMP_ICN_AUTO_POWER_OFF, 0,
        ADAS_ON, NULL}
};

static MENU_SEL_s menu_video_adas_calibration_sel_tbl[MENU_VIDEO_ADAS_CALIBRATION_SEL_NUM] = {
    {MENU_VIDEO_ADAS_CALIBRATION_SET, MENU_SEL_FLAGS_ENABLE,
        STR_ADAS_CALIBRATION, BMP_ICN_AUTO_POWER_OFF, 0,
        ADAS_ON, NULL}
};

static MENU_SEL_s *menu_video_item_sel_tbls[MENU_VIDEO_ITEM_NUM] = {
    menu_video_sensor_res_sel_tbl,
    menu_video_yuv_res_sel_tbl,
    menu_video_quality_sel_tbl,
    menu_video_selftimer_sel_tbl,
    menu_video_pre_record_sel_tbl,
    menu_video_time_lapse_sel_tbl,
    menu_video_dual_streams_sel_tbl,
    menu_video_streams_type_sel_tbl,
    menu_video_streaming_sel_tbl,
    menu_video_digital_zoom_sel_tbl,
    menu_video_stamp_sel_tbl,
    menu_video_rec_mode_sel_tbl,
    menu_video_adas_sel_tbl,
    menu_video_adas_calibration_sel_tbl
};

/*** Currently activated object id arrays ***/
static MENU_SEL_s *menu_video_sensor_res_sels[MENU_VIDEO_SENSOR_RES_SEL_NUM];
static MENU_SEL_s *menu_video_yuv_res_sels[MENU_VIDEO_YUV_RES_SEL_NUM];
static MENU_SEL_s *menu_video_quality_sels[MENU_VIDEO_QUALITY_SEL_NUM];
static MENU_SEL_s *menu_video_selftimer_sels[MENU_VIDEO_SELFTIMER_SEL_NUM];
static MENU_SEL_s *menu_video_pre_record_sels[MENU_VIDEO_PRE_RECORD_SEL_NUM];
static MENU_SEL_s *menu_video_time_lapse_sels[MENU_VIDEO_TIME_LAPSE_SEL_NUM];
static MENU_SEL_s *menu_video_dual_streams_sels[MENU_VIDEO_DUAL_STREAMS_SEL_NUM];
static MENU_SEL_s *menu_video_streams_type_sels[MENU_VIDEO_STREAMS_TYPE_SEL_NUM];
static MENU_SEL_s *menu_video_streaming_sels[MENU_VIDEO_STREAMING_SEL_NUM];
static MENU_SEL_s *menu_video_digital_zoom_sels[MENU_VIDEO_DIGITAL_ZOOM_SEL_NUM];
static MENU_SEL_s *menu_video_stamp_sels[MENU_VIDEO_STAMP_SEL_NUM];
static MENU_SEL_s *menu_video_rec_mode_sels[MENU_VIDEO_REC_MODE_SEL_NUM];
static MENU_SEL_s *menu_video_adas_sels[MENU_VIDEO_ADAS_SEL_NUM];
static MENU_SEL_s *menu_video_adas_calibration_sels[MENU_VIDEO_ADAS_CALIBRATION_SEL_NUM];


/*** Item ***/
static MENU_ITEM_s menu_video_sensor_res = {
    MENU_VIDEO_SENSOR_RES, MENU_ITEM_FLAGS_ENABLE,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_sensor_res_sels,
    menu_video_sensor_res_init,
    menu_video_sensor_res_get_tab_str,
    menu_video_sensor_res_get_sel_str,
    menu_video_sensor_res_get_sel_bmp,
    menu_video_sensor_res_set,
    menu_video_sensor_res_sel_set
};

static MENU_ITEM_s menu_video_yuv_res = {
    MENU_VIDEO_YUV_RES, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_yuv_res_sels,
    menu_video_yuv_res_init,
    menu_video_yuv_res_get_tab_str,
    menu_video_yuv_res_get_sel_str,
    menu_video_yuv_res_get_sel_bmp,
    menu_video_yuv_res_set,
    menu_video_yuv_res_sel_set
};

static MENU_ITEM_s menu_video_quality = {
    MENU_VIDEO_QUALITY, MENU_ITEM_FLAGS_ENABLE,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_quality_sels,
    menu_video_quality_init,
    menu_video_quality_get_tab_str,
    menu_video_quality_get_sel_str,
    menu_video_quality_get_sel_bmp,
    menu_video_quality_set,
    menu_video_quality_sel_set
};

static MENU_ITEM_s menu_video_selftimer = {
    MENU_VIDEO_SELFTIMER, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_selftimer_sels,
    menu_video_selftimer_init,
    menu_video_selftimer_get_tab_str,
    menu_video_selftimer_get_sel_str,
    menu_video_selftimer_get_sel_bmp,
    menu_video_selftimer_set,
    menu_video_selftimer_sel_set
};

static MENU_ITEM_s menu_video_pre_record = {
    MENU_VIDEO_PRE_RECORD, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_pre_record_sels,
    menu_video_pre_record_init,
    menu_video_pre_record_get_tab_str,
    menu_video_pre_record_get_sel_str,
    menu_video_pre_record_get_sel_bmp,
    menu_video_pre_record_set,
    menu_video_pre_record_sel_set
};

static MENU_ITEM_s menu_video_time_lapse = {
    MENU_VIDEO_TIME_LAPSE, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_time_lapse_sels,
    menu_video_time_lapse_init,
    menu_video_time_lapse_get_tab_str,
    menu_video_time_lapse_get_sel_str,
    menu_video_time_lapse_get_sel_bmp,
    menu_video_time_lapse_set,
    menu_video_time_lapse_sel_set
};

static MENU_ITEM_s menu_video_dual_streams = {
    MENU_VIDEO_DUAL_STREAMS, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_dual_streams_sels,
    menu_video_dual_streams_init,
    menu_video_dual_streams_get_tab_str,
    menu_video_dual_streams_get_sel_str,
    menu_video_dual_streams_get_sel_bmp,
    menu_video_dual_streams_set,
    menu_video_dual_streams_sel_set
};

static MENU_ITEM_s menu_video_streams_type = {
    MENU_VIDEO_STREAMS_TYPE, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_streams_type_sels,
    menu_video_streams_type_init,
    menu_video_streams_type_get_tab_str,
    menu_video_streams_type_get_sel_str,
    menu_video_streams_type_get_sel_bmp,
    menu_video_streams_type_set,
    menu_video_streams_type_sel_set
};

static MENU_ITEM_s menu_video_streaming = {
    MENU_VIDEO_STREAMING, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_streaming_sels,
    menu_video_streaming_init,
    menu_video_streaming_get_tab_str,
    menu_video_streaming_get_sel_str,
    menu_video_streaming_get_sel_bmp,
    menu_video_streaming_set,
    menu_video_streaming_sel_set
};

static MENU_ITEM_s menu_video_digital_zoom = {
    MENU_VIDEO_DIGITAL_ZOOM, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_digital_zoom_sels,
    menu_video_digital_zoom_init,
    menu_video_digital_zoom_get_tab_str,
    menu_video_digital_zoom_get_sel_str,
    menu_video_digital_zoom_get_sel_bmp,
    menu_video_digital_zoom_set,
    menu_video_digital_zoom_sel_set
};

static MENU_ITEM_s menu_video_stamp = {
    MENU_VIDEO_STAMP, 0,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_stamp_sels,
    menu_video_stamp_init,
    menu_video_stamp_get_tab_str,
    menu_video_stamp_get_sel_str,
    menu_video_stamp_get_sel_bmp,
    menu_video_stamp_set,
    menu_video_stamp_sel_set
};

static MENU_ITEM_s menu_video_rec_mode = {
    MENU_VIDEO_REC_MODE, MENU_ITEM_FLAGS_ENABLE,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_rec_mode_sels,
    menu_video_rec_mode_init,
    menu_video_rec_mode_get_tab_str,
    menu_video_rec_mode_get_sel_str,
    menu_video_rec_mode_get_sel_bmp,
    menu_video_rec_mode_set,
    menu_video_rec_mode_sel_set
};

static MENU_ITEM_s menu_video_adas = {
    MENU_VIDEO_ADAS, MENU_ITEM_FLAGS_ENABLE,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_adas_sels,
    menu_video_adas_init,
    menu_video_adas_get_tab_str,
    menu_video_adas_get_sel_str,
    menu_video_adas_get_sel_bmp,
    menu_video_adas_set,
    menu_video_adas_sel_set
};

static MENU_ITEM_s menu_video_adas_calibration = {
    MENU_VIDEO_ADAS_CALIB, MENU_ITEM_FLAGS_ENABLE,
    0, 0,
    0, 0,
    0, 0, 0, menu_video_adas_calibration_sels,
    menu_video_adas_calibration_init,
    menu_video_adas_calibration_get_tab_str,
    menu_video_adas_calibration_get_sel_str,
    menu_video_adas_calibration_get_sel_bmp,
    menu_video_adas_calibration_set,
    menu_video_adas_calibration_sel_set
};

static MENU_ITEM_s *menu_video_item_tbl[MENU_VIDEO_ITEM_NUM] = {
    &menu_video_sensor_res,
    &menu_video_yuv_res,
    &menu_video_quality,
    &menu_video_selftimer,
    &menu_video_pre_record,
    &menu_video_time_lapse,
    &menu_video_dual_streams,
    &menu_video_streams_type,
    &menu_video_streaming,
    &menu_video_digital_zoom,
    &menu_video_stamp,
    &menu_video_rec_mode,
    &menu_video_adas,
    &menu_video_adas_calibration
};

/*** Currently activated object id arrays ***/
static MENU_ITEM_s *menu_video_items[MENU_VIDEO_ITEM_NUM];

/*** Tab ***/
static MENU_TAB_s menu_video = {
    MENU_VIDEO, MENU_TAB_FLAGS_ENABLE,
    0, 0,
    BMP_MENU_TAB_VIDEO, BMP_MENU_TAB_VIDEO_HL,
    menu_video_items,
    menu_video_init,
    menu_video_start,
    menu_video_stop
};

MENU_TAB_CTRL_s menu_video_ctrl = {
    menu_video_get_tab,
    menu_video_get_item,
    menu_video_get_sel,
    menu_video_set_sel_table,
    menu_video_lock_tab,
    menu_video_unlock_tab,
    menu_video_enable_item,
    menu_video_disable_item,
    menu_video_lock_item,
    menu_video_unlock_item,
    menu_video_enable_sel,
    menu_video_disable_sel,
    menu_video_lock_sel,
    menu_video_unlock_sel
};

/*** APIs ***/

// tab
static int menu_video_init(void)
{
    int i = 0;
    UINT32 cur_item_id = 0;

//#if defined(CONFIG_APP_CONNECTED_STAMP)
//    menu_video_enable_item(MENU_VIDEO_STAMP);
//#endif

#if defined APP_ADAS
    menu_video_enable_item(MENU_VIDEO_ADAS);
    menu_video_enable_item(MENU_VIDEO_ADAS_CALIB);
#else
    menu_video_disable_item(MENU_VIDEO_ADAS);
    menu_video_disable_item(MENU_VIDEO_ADAS_CALIB);
    UserSetting->VAPref.AdasDetection = ADAS_OFF;
#endif

    APP_ADDFLAGS(menu_video.Flags, MENU_TAB_FLAGS_INIT);

    if (menu_video.ItemNum > 0) {
        cur_item_id = menu_video_items[menu_video.ItemCur]->Id;
    }
    menu_video.ItemNum = 0;
    menu_video.ItemCur = 0;
    for (i=0; i<MENU_VIDEO_ITEM_NUM; i++) {
        if (APP_CHECKFLAGS(menu_video_item_tbl[i]->Flags, MENU_ITEM_FLAGS_ENABLE)) {
            menu_video_items[menu_video.ItemNum] = menu_video_item_tbl[i];
            if (cur_item_id == menu_video_item_tbl[i]->Id) {
                menu_video.ItemCur = menu_video.ItemNum;
            }
            menu_video.ItemNum++;
        }
    }

    return 0;
}

static int menu_video_start(void)
{
    return 0;
}

static int menu_video_stop(void)
{
    return 0;
}

// item
static int menu_video_sensor_res_init(void)
{
    int i = 0;
    int res_num = 0;
    UINT16 *Str;

    APP_ADDFLAGS(menu_video_sensor_res.Flags, MENU_ITEM_FLAGS_INIT);

    res_num = AppLibSysSensor_GetVideoResNum();
    for (i=0; i<MENU_VIDEO_SENSOR_RES_SEL_NUM; i++) {
        if (i < res_num) {
            APP_ADDFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_SENSOR_RES]+i)->Flags, MENU_SEL_FLAGS_ENABLE);
            (menu_video_item_sel_tbls[MENU_VIDEO_SENSOR_RES]+i)->Val = AppLibSysSensor_GetVideoResID(i);
            Str = AppLibSysSensor_GetVideoResStr((menu_video_item_sel_tbls[MENU_VIDEO_SENSOR_RES]+i)->Val);
            AppLibGraph_UpdateStringContext(0, (menu_video_item_sel_tbls[MENU_VIDEO_SENSOR_RES]+i)->Str, Str);
            (menu_video_item_sel_tbls[MENU_VIDEO_SENSOR_RES]+i)->Bmp = AppGuiUtil_GetVideoResolutionBitmapSizeId((menu_video_item_sel_tbls[MENU_VIDEO_SENSOR_RES]+i)->Val);
            if ((menu_video_item_sel_tbls[MENU_VIDEO_SENSOR_RES]+i)->Val == SENSOR_VIDEO_RES_PHOTO) {
                (menu_video_item_sel_tbls[MENU_VIDEO_SENSOR_RES]+i)->Bmp = BMP_ICN_PRECISE_QUALITY_CONT;
            }

        } else {
            APP_REMOVEFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_SENSOR_RES]+i)->Flags, MENU_SEL_FLAGS_ENABLE);
        }

    }

    menu_video_sensor_res.SelSaved = 0;
    menu_video_sensor_res.SelNum = 0;
    menu_video_sensor_res.SelCur = 0;
    for (i=0; i<MENU_VIDEO_SENSOR_RES_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_SENSOR_RES]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_sensor_res_sels[menu_video_sensor_res.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_SENSOR_RES]+i;
            if (menu_video_sensor_res_sels[menu_video_sensor_res.SelNum]->Val == UserSetting->VideoPref.SensorVideoRes) {
                menu_video_sensor_res.SelSaved = menu_video_sensor_res.SelNum;
                menu_video_sensor_res.SelCur = menu_video_sensor_res.SelNum;
            }
            menu_video_sensor_res.SelNum++;
        }
    }
    return 0;
}

static int menu_video_sensor_res_get_tab_str(void)
{
    return menu_video_sensor_res_sels[menu_video_sensor_res.SelSaved]->Str;
}

static int menu_video_sensor_res_get_sel_str(int ref)
{
    return menu_video_sensor_res_sels[ref]->Str;
}

static int menu_video_sensor_res_get_sel_bmp(int ref)
{
    return menu_video_sensor_res_sels[ref]->Bmp;
}

static int menu_video_sensor_res_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_SENSOR_RES);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_sensor_res_sel_set(void)
{
    if (menu_video_sensor_res.SelSaved != menu_video_sensor_res.SelCur) {
        menu_video_sensor_res.SelSaved = menu_video_sensor_res.SelCur;
        UserSetting->VideoPref.SensorVideoRes = menu_video_sensor_res.Sels[menu_video_sensor_res.SelCur]->Val;
        //app_util_video_sensor_res_pref_reset(0);
        //app_util_menu_lock_item_dual_stream_prerecord(0);
        //app_util_menu_lock_item_stamp(0);
        //app_util_menu_lock_item_time_lapse(0);
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_VIDEO_RES, menu_video_sensor_res.Sels[menu_video_sensor_res.SelCur]->Val, 0);
        }
    }
    return 0;
}

static int menu_video_yuv_res_init(void)
{
    int i = 0;
    int res_num = 1;
    //WCHAR *Str;

    APP_ADDFLAGS(menu_video_yuv_res.Flags, MENU_ITEM_FLAGS_INIT);

    //res_num = app_yuv_get_video_res_num();
    for (i=0; i<MENU_VIDEO_YUV_RES_SEL_NUM; i++) {
        if (i < res_num) {
            APP_ADDFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_YUV_RES]+i)->Flags, MENU_SEL_FLAGS_ENABLE);
            //(menu_video_item_sel_tbls[MENU_VIDEO_YUV_RES]+i)->Val = app_yuv_get_video_res_id(i);
            //Str = app_yuv_get_video_res_str((menu_video_item_sel_tbls[MENU_VIDEO_YUV_RES]+i)->Val);
            /*{
                char str_t[YUV_VIDEO_RES_STR_LEN];
                uni_to_asc(Str, str_t);
                AmbaPrint("Update STR_VIDEO_YUV_RES_%d with %s", i, str_t);
            }*/
        } else {
            APP_REMOVEFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_YUV_RES]+i)->Flags, MENU_SEL_FLAGS_ENABLE);
        }
    }

    menu_video_yuv_res.SelSaved = 0;
    menu_video_yuv_res.SelNum = 0;
    menu_video_yuv_res.SelCur = 0;
    for (i=0; i<MENU_VIDEO_YUV_RES_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_YUV_RES]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_yuv_res_sels[menu_video_yuv_res.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_YUV_RES]+i;
            if (menu_video_yuv_res_sels[menu_video_yuv_res.SelNum]->Val == UserSetting->VideoPref.YUVVideoRes) {
                menu_video_yuv_res.SelSaved = menu_video_yuv_res.SelNum;
                menu_video_yuv_res.SelCur = menu_video_yuv_res.SelNum;
            }
            menu_video_yuv_res.SelNum++;
        }
    }
    return 0;
}

static int menu_video_yuv_res_get_tab_str(void)
{
    return menu_video_yuv_res_sels[menu_video_yuv_res.SelSaved]->Str;
}

static int menu_video_yuv_res_get_sel_str(int ref)
{
    return menu_video_yuv_res_sels[ref]->Str;
}

static int menu_video_yuv_res_get_sel_bmp(int ref)
{
    return menu_video_yuv_res_sels[ref]->Bmp;
}

static int menu_video_yuv_res_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_YUV_RES);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_yuv_res_sel_set(void)
{
    if (menu_video_yuv_res.SelSaved != menu_video_yuv_res.SelCur) {
        menu_video_yuv_res.SelSaved = menu_video_yuv_res.SelCur;
        UserSetting->VideoPref.YUVVideoRes = menu_video_yuv_res.Sels[menu_video_yuv_res.SelCur]->Val;
        AppLibVideoEnc_SetYuvVideoRes(menu_video_yuv_res.Sels[menu_video_yuv_res.SelCur]->Val);
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_VIDEO_RES, menu_video_yuv_res.Sels[menu_video_yuv_res.SelCur]->Val, 0);
        }
    }
    return 0;
}

static int menu_video_quality_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_quality.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_quality.SelSaved = 0;
    menu_video_quality.SelNum = 0;
    menu_video_quality.SelCur = 0;
    for (i=0; i<MENU_VIDEO_QUALITY_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_QUALITY]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_quality_sels[menu_video_quality.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_QUALITY]+i;
            if (menu_video_quality_sels[menu_video_quality.SelNum]->Val == UserSetting->VideoPref.VideoQuality) {
                menu_video_quality.SelSaved = menu_video_quality.SelNum;
                menu_video_quality.SelCur = menu_video_quality.SelNum;
            }
            menu_video_quality.SelNum++;
        }
    }

    return 0;
}

static int menu_video_quality_get_tab_str(void)
{
    return menu_video_quality_sels[menu_video_quality.SelSaved]->Str;
}

static int menu_video_quality_get_sel_str(int ref)
{
    return menu_video_quality_sels[ref]->Str;
}

static int menu_video_quality_get_sel_bmp(int ref)
{
    return menu_video_quality_sels[ref]->Bmp;
}

static int menu_video_quality_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_QUALITY);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_quality_sel_set(void)
{
    if (menu_video_quality.SelSaved != menu_video_quality.SelCur) {
        menu_video_quality.SelSaved = menu_video_quality.SelCur;
        UserSetting->VideoPref.VideoQuality = menu_video_quality.Sels[menu_video_quality.SelCur]->Val;
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_VIDEO_QUALITY, menu_video_quality.Sels[menu_video_quality.SelCur]->Val, 0);
        }
    }
    return 0;
}

static int menu_video_selftimer_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_selftimer.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_selftimer.SelSaved = 0;
    menu_video_selftimer.SelNum = 0;
    menu_video_selftimer.SelCur = 0;
    for (i=0; i<MENU_VIDEO_SELFTIMER_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_SELFTIMER]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_selftimer_sels[menu_video_selftimer.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_SELFTIMER]+i;
            if (menu_video_selftimer_sels[menu_video_selftimer.SelNum]->Val == UserSetting->VideoPref.VideoSelftimer) {
                menu_video_selftimer.SelSaved = menu_video_selftimer.SelNum;
                menu_video_selftimer.SelCur = menu_video_selftimer.SelNum;
            }
            menu_video_selftimer.SelNum++;
        }
    }

    return 0;
}

static int menu_video_selftimer_get_tab_str(void)
{
    return menu_video_selftimer_sels[menu_video_selftimer.SelSaved]->Str;
}

static int menu_video_selftimer_get_sel_str(int ref)
{
    return menu_video_selftimer_sels[ref]->Str;
}

static int menu_video_selftimer_get_sel_bmp(int ref)
{
    return menu_video_selftimer_sels[ref]->Bmp;
}

static int menu_video_selftimer_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_SELFTIMER);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_selftimer_sel_set(void)
{
    if (menu_video_selftimer.SelSaved != menu_video_selftimer.SelCur) {
        menu_video_selftimer.SelSaved = menu_video_selftimer.SelCur;
        UserSetting->VideoPref.VideoSelftimer = (APP_PREF_SELF_TIMER_e)menu_video_selftimer.Sels[menu_video_selftimer.SelCur]->Val;
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_SELFTIMER, menu_video_selftimer.Sels[menu_video_selftimer.SelCur]->Val, 0);
        }
    }
    return 0;
}

static int menu_video_pre_record_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_pre_record.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_pre_record.SelSaved = 0;
    menu_video_pre_record.SelNum = 0;
    menu_video_pre_record.SelCur = 0;
    for (i=0; i<MENU_VIDEO_PRE_RECORD_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_PRE_RECORD]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_pre_record_sels[menu_video_pre_record.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_PRE_RECORD]+i;
            if (menu_video_pre_record_sels[menu_video_pre_record.SelNum]->Val == UserSetting->VideoPref.PreRecord) {
                menu_video_pre_record.SelSaved = menu_video_pre_record.SelNum;
                menu_video_pre_record.SelCur = menu_video_pre_record.SelNum;
            }
            menu_video_pre_record.SelNum++;
        }
    }

    return 0;
}

static int menu_video_pre_record_get_tab_str(void)
{
    return menu_video_pre_record_sels[menu_video_pre_record.SelSaved]->Str;
}

static int menu_video_pre_record_get_sel_str(int ref)
{
    return menu_video_pre_record_sels[ref]->Str;
}

static int menu_video_pre_record_get_sel_bmp(int ref)
{
    return menu_video_pre_record_sels[ref]->Bmp;
}

static int menu_video_pre_record_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_PRE_RECORD);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_pre_record_sel_set(void)
{
    if (menu_video_pre_record.SelSaved != menu_video_pre_record.SelCur) {
        menu_video_pre_record.SelSaved = menu_video_pre_record.SelCur;
        UserSetting->VideoPref.PreRecord = menu_video_pre_record.Sels[menu_video_pre_record.SelCur]->Val;
        //app_util_menu_lock_item_time_lapse(0);
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_VIDEO_PRE_RECORD, menu_video_pre_record.Sels[menu_video_pre_record.SelCur]->Val, 0);
        }
    }
    return 0;
}

static int menu_video_time_lapse_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_time_lapse.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_time_lapse.SelSaved = 0;
    menu_video_time_lapse.SelNum = 0;
    menu_video_time_lapse.SelCur = 0;
    for (i=0; i<MENU_VIDEO_TIME_LAPSE_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_TIME_LAPSE]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_time_lapse_sels[menu_video_time_lapse.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_TIME_LAPSE]+i;
            if (menu_video_time_lapse_sels[menu_video_time_lapse.SelNum]->Val == UserSetting->VideoPref.TimeLapse) {
                menu_video_time_lapse.SelSaved = menu_video_time_lapse.SelNum;
                menu_video_time_lapse.SelCur = menu_video_time_lapse.SelNum;
            }
            menu_video_time_lapse.SelNum++;
        }
    }

    return 0;
}

static int menu_video_time_lapse_get_tab_str(void)
{
    return menu_video_time_lapse_sels[menu_video_time_lapse.SelSaved]->Str;
}

static int menu_video_time_lapse_get_sel_str(int ref)
{
    return menu_video_time_lapse_sels[ref]->Str;
}

static int menu_video_time_lapse_get_sel_bmp(int ref)
{
    return menu_video_time_lapse_sels[ref]->Bmp;
}

static int menu_video_time_lapse_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_TIME_LAPSE);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_time_lapse_sel_set(void)
{
    if (menu_video_time_lapse.SelSaved != menu_video_time_lapse.SelCur) {
        menu_video_time_lapse.SelSaved = menu_video_time_lapse.SelCur;
        UserSetting->VideoPref.TimeLapse = menu_video_time_lapse.Sels[menu_video_time_lapse.SelCur]->Val;
        //app_util_menu_lock_item_dual_stream_prerecord(0);
        //app_util_menu_lock_item_scene_sshutter(0);
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_VIDEO_TIME_LAPSE, menu_video_time_lapse.Sels[menu_video_time_lapse.SelCur]->Val, 0);
        }
    }
    return 0;
}

static int menu_video_dual_streams_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_dual_streams.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_dual_streams.SelSaved = 0;
    menu_video_dual_streams.SelNum = 0;
    menu_video_dual_streams.SelCur = 0;
    for (i=0; i<MENU_VIDEO_DUAL_STREAMS_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_DUAL_STREAMS]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_dual_streams_sels[menu_video_dual_streams.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_DUAL_STREAMS]+i;
            if (menu_video_dual_streams_sels[menu_video_dual_streams.SelNum]->Val == UserSetting->VideoPref.DualStreams) {
                menu_video_dual_streams.SelSaved = menu_video_dual_streams.SelNum;
                menu_video_dual_streams.SelCur = menu_video_dual_streams.SelNum;
            }
            menu_video_dual_streams.SelNum++;
        }
    }

    return 0;
}

static int menu_video_dual_streams_get_tab_str(void)
{
    if (APP_CHECKFLAGS(menu_video_dual_streams.Flags, MENU_ITEM_FLAGS_LOCKED)) {
        return STR_DUAL_STREAMS_OFF;
    } else {
        return menu_video_dual_streams_sels[menu_video_dual_streams.SelSaved]->Str;
    }
}

static int menu_video_dual_streams_get_sel_str(int ref)
{
    return menu_video_dual_streams_sels[ref]->Str;
}

static int menu_video_dual_streams_get_sel_bmp(int ref)
{
    return menu_video_dual_streams_sels[ref]->Bmp;
}

static int menu_video_dual_streams_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_DUAL_STREAMS);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_dual_streams_sel_set(void)
{
    if (menu_video_dual_streams.SelSaved != menu_video_dual_streams.SelCur) {
        menu_video_dual_streams.SelSaved = menu_video_dual_streams.SelCur;
        UserSetting->VideoPref.DualStreams = menu_video_dual_streams.Sels[menu_video_dual_streams.SelCur]->Val;
        //app_util_video_sensor_res_pref_reset(0);
        //app_util_menu_lock_item_time_lapse(0);
        //app_util_menu_lock_item_stamp(0);
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_VIDEO_DUAL_STREAMS, menu_video_dual_streams.Sels[menu_video_dual_streams.SelCur]->Val, 0);
        }
    }
    return 0;
}

static int menu_video_streams_type_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_streams_type.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_streams_type.SelSaved = 0;
    menu_video_streams_type.SelNum = 0;
    menu_video_streams_type.SelCur = 0;
    for (i=0; i<MENU_VIDEO_STREAMS_TYPE_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_STREAMS_TYPE]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_streams_type_sels[menu_video_streams_type.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_STREAMS_TYPE]+i;
            if (menu_video_streams_type_sels[menu_video_streams_type.SelNum]->Val == UserSetting->VideoPref.StreamType) {
                menu_video_streams_type.SelSaved = menu_video_streams_type.SelNum;
                menu_video_streams_type.SelCur = menu_video_streams_type.SelNum;
            }
            menu_video_streams_type.SelNum++;
        }
    }

    return 0;
}

static int menu_video_streams_type_get_tab_str(void)
{
#if defined(CONFIG_AMBA_STREAMING)
    if (APP_CHECKFLAGS(menu_video_streams_type.Flags, MENU_ITEM_FLAGS_LOCKED) &&
        (UserSetting->SetupPref.USBMode == USB_MODE_MSC)) {
        return STR_STREAM_TYPE_OFF;
    } else
#endif
    return menu_video_streams_type_sels[menu_video_streams_type.SelSaved]->Str;
}

static int menu_video_streams_type_get_sel_str(int ref)
{
    return menu_video_streams_type_sels[ref]->Str;
}

static int menu_video_streams_type_get_sel_bmp(int ref)
{
    return menu_video_streams_type_sels[ref]->Bmp;
}

static int menu_video_streams_type_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_STREAMS_TYPE);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_streams_type_sel_set(void)
{
    if (menu_video_streams_type.SelSaved != menu_video_streams_type.SelCur) {
        menu_video_streams_type.SelSaved = menu_video_streams_type.SelCur;
        UserSetting->VideoPref.StreamType = (APP_PREF_STREAM_TYPE_e)menu_video_streams_type.Sels[menu_video_streams_type.SelCur]->Val;
        //app_rec_setting_set_streaming_type(UserSetting->VideoPref.StreamType);
#if defined(CONFIG_AMBA_STREAMING)
        if (UserSetting->VideoPref.StreamType == SPORT_DV_STREAM_OFF) {
            app_video_set_streaming_mode(VIDEO_STREAMING_OFF);
            if (app_video_setting_get_sensor_video_res() != UserSetting->VideoPref.SensorVideoRes) {
                /* Send the message to the current app. */
                APP_APP_s *curapp;
                AppAppMgt_GetCurApp(&curapp);
                curapp->OnMessage(AMSG_CMD_SET_VIDEO_RES, UserSetting->VideoPref.SensorVideoRes, 0);
            }
        }
#endif
        //app_util_video_sensor_res_pref_reset(0);
        //app_util_menu_lock_item_dual_stream_prerecord(0);
        //app_util_menu_lock_item_time_lapse(0);

    }
    return 0;
}

static int menu_video_streaming_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_streaming.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_streaming.SelSaved = 0;
    menu_video_streaming.SelNum = 0;
    menu_video_streaming.SelCur = 0;
    for (i=0; i<MENU_VIDEO_STREAMING_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_STREAMING]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_streaming_sels[menu_video_streaming.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_STREAMING]+i;
            if (menu_video_streaming_sels[menu_video_streaming.SelNum]->Val == UserSetting->VideoPref.Streaming) {
                menu_video_streaming.SelSaved = menu_video_streaming.SelNum;
                menu_video_streaming.SelCur = menu_video_streaming.SelNum;
            }
            menu_video_streaming.SelNum++;
        }
    }

    return 0;
}

static int menu_video_streaming_get_tab_str(void)
{
    return menu_video_streaming_sels[menu_video_streaming.SelSaved]->Str;
}

static int menu_video_streaming_get_sel_str(int ref)
{
    return menu_video_streaming_sels[ref]->Str;
}

static int menu_video_streaming_get_sel_bmp(int ref)
{
    return menu_video_streaming_sels[ref]->Bmp;
}

static int menu_video_streaming_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_STREAMING);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_streaming_sel_set(void)
{
    if (menu_video_streaming.SelSaved != menu_video_streaming.SelCur) {
        menu_video_streaming.SelSaved = menu_video_streaming.SelCur;
        UserSetting->VideoPref.Streaming = (APP_PREF_STREAMING_STATUS_e)menu_video_streaming.Sels[menu_video_streaming.SelCur]->Val;
        //app_util_video_sensor_res_pref_reset(0);
        //app_util_menu_lock_item_time_lapse(0);
        //app_util_menu_lock_item_stamp(0);
#if defined(CONFIG_AMBA_STREAMING)
        if (UserSetting->VideoPref.Streaming) {
            app_video_set_streaming_mode(VIDEO_STREAMING_OUT_3RD);
        } else {
            app_video_set_streaming_mode(VIDEO_STREAMING_OFF);
        }
        app_rec_setup_file_format(0);
        //AmbaPrintColor(RED,"UserSetting->streaming = %d",UserSetting->streaming);
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_STREAMING, menu_video_streaming.Sels[menu_video_streaming.SelCur]->Val, 0);
        }
#endif
    }
    return 0;
}

static int menu_video_digital_zoom_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_digital_zoom.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_digital_zoom.SelSaved = 0;
    menu_video_digital_zoom.SelNum = 0;
    menu_video_digital_zoom.SelCur = 0;
    for (i=0; i<MENU_VIDEO_DIGITAL_ZOOM_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_DIGITAL_ZOOM]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_digital_zoom_sels[menu_video_digital_zoom.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_DIGITAL_ZOOM]+i;
            if (menu_video_digital_zoom_sels[menu_video_digital_zoom.SelNum]->Val == UserSetting->VideoPref.VideoDZoom) {
                menu_video_digital_zoom.SelSaved = menu_video_digital_zoom.SelNum;
                menu_video_digital_zoom.SelCur = menu_video_digital_zoom.SelNum;
            }
            menu_video_digital_zoom.SelNum++;
        }
    }

    return 0;
}

static int menu_video_digital_zoom_get_tab_str(void)
{
    return menu_video_digital_zoom_sels[menu_video_digital_zoom.SelSaved]->Str;
}

static int menu_video_digital_zoom_get_sel_str(int ref)
{
    return menu_video_digital_zoom_sels[ref]->Str;
}

static int menu_video_digital_zoom_get_sel_bmp(int ref)
{
    return menu_video_digital_zoom_sels[ref]->Bmp;
}

static int menu_video_digital_zoom_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_DIGITAL_ZOOM);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_digital_zoom_sel_set(void)
{
    if (menu_video_digital_zoom.SelSaved != menu_video_digital_zoom.SelCur) {
        menu_video_digital_zoom.SelSaved = menu_video_digital_zoom.SelCur;
        UserSetting->VideoPref.VideoDZoom = (APP_PREF_DZOOM_e)menu_video_digital_zoom.Sels[menu_video_digital_zoom.SelCur]->Val;
        //app_dzoom_setting_set_mode(menu_video_digital_zoom.Sels[menu_video_digital_zoom.SelCur]->Val);
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_DIGITAL_ZOOM, menu_video_digital_zoom.Sels[menu_video_digital_zoom.SelCur]->Val, 0);
        }
    }
    return 0;
}

static int menu_video_stamp_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_stamp.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_stamp.SelSaved = 0;
    menu_video_stamp.SelNum = 0;
    menu_video_stamp.SelCur = 0;
    for (i=0; i<MENU_VIDEO_STAMP_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_STAMP]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_stamp_sels[menu_video_stamp.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_STAMP]+i;
            if (menu_video_stamp_sels[menu_video_stamp.SelNum]->Val == UserSetting->VideoPref.VideoDateTimeStamp) {
                menu_video_stamp.SelSaved = menu_video_stamp.SelNum;
                menu_video_stamp.SelCur = menu_video_stamp.SelNum;
            }
            menu_video_stamp.SelNum++;
        }
    }

    return 0;
}

static int menu_video_stamp_get_tab_str(void)
{
    return menu_video_stamp_sels[menu_video_stamp.SelSaved]->Str;
}

static int menu_video_stamp_get_sel_str(int ref)
{
    return menu_video_stamp_sels[ref]->Str;
}

static int menu_video_stamp_get_sel_bmp(int ref)
{
    return menu_video_stamp_sels[ref]->Bmp;
}

static int menu_video_stamp_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_STAMP);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_stamp_sel_set(void)
{
    if (menu_video_stamp.SelSaved != menu_video_stamp.SelCur) {
        menu_video_stamp.SelSaved = menu_video_stamp.SelCur;
        UserSetting->VideoPref.VideoDateTimeStamp = (APP_PREF_TIME_STAMP_e)menu_video_stamp_sels[menu_video_stamp.SelCur]->Val;
    }
    //app_util_menu_lock_item_dual_stream_prerecord(0);

    return 0;
}

static int menu_video_rec_mode_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_rec_mode.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_rec_mode.SelSaved = 0;
    menu_video_rec_mode.SelNum = 0;
    menu_video_rec_mode.SelCur = 0;


    for (i=0; i<MENU_VIDEO_REC_MODE_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_REC_MODE]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_rec_mode_sels[menu_video_rec_mode.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_REC_MODE]+i;
            if (menu_video_rec_mode_sels[menu_video_rec_mode.SelNum]->Val == app_status.CurrEncMode) {
                menu_video_rec_mode.SelSaved = menu_video_rec_mode.SelNum;
                menu_video_rec_mode.SelCur = menu_video_rec_mode.SelNum;
            }
            menu_video_rec_mode.SelNum++;
        }
    }

    return 0;
}

static int menu_video_rec_mode_get_tab_str(void)
{
    return menu_video_rec_mode_sels[menu_video_rec_mode.SelSaved]->Str;
}

static int menu_video_rec_mode_get_sel_str(int ref)
{
    return menu_video_rec_mode_sels[ref]->Str;
}

static int menu_video_rec_mode_get_sel_bmp(int ref)
{
    return menu_video_rec_mode_sels[ref]->Bmp;
}

static int menu_video_rec_mode_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_REC_MODE);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_rec_mode_sel_set(void)
{
    if (menu_video_rec_mode.SelSaved != menu_video_rec_mode.SelCur) {
        menu_video_rec_mode.SelSaved = menu_video_rec_mode.SelCur;
        {
            /* Send the message to the current app. */
            APP_APP_s *curapp;
            AppAppMgt_GetCurApp(&curapp);
            curapp->OnMessage(AMSG_CMD_SET_RECORD_MODE, menu_video_rec_mode.Sels[menu_video_rec_mode.SelCur]->Val, 0);
        }
    }
    return 0;
}

static int menu_video_adas_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_adas.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_adas.SelSaved = 0;
    menu_video_adas.SelNum = 0;
    menu_video_adas.SelCur = 0;


    for (i=0; i<MENU_VIDEO_ADAS_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_ADAS]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_adas_sels[menu_video_adas.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_ADAS]+i;
            if (menu_video_adas_sels[menu_video_adas.SelNum]->Val == UserSetting->VAPref.AdasDetection) {
                menu_video_adas.SelSaved = menu_video_adas.SelNum;
                menu_video_adas.SelCur = menu_video_adas.SelNum;
            }
            menu_video_adas.SelNum++;
        }
    }

    return 0;
}

static int menu_video_adas_get_tab_str(void)
{
    return menu_video_adas_sels[menu_video_adas.SelSaved]->Str;
}

static int menu_video_adas_get_sel_str(int ref)
{
    return menu_video_adas_sels[ref]->Str;
}

static int menu_video_adas_get_sel_bmp(int ref)
{
    return menu_video_adas_sels[ref]->Bmp;
}

static int menu_video_adas_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_ADAS);
    AppWidget_On(WIDGET_MENU_QUICK, 0);
    return 0;
}

static int menu_video_adas_sel_set(void)
{
    if (menu_video_adas.SelSaved != menu_video_adas.SelCur) {
        menu_video_adas.SelSaved = menu_video_adas.SelCur;
        UserSetting->VAPref.AdasDetection = menu_video_adas.Sels[menu_video_adas.SelCur]->Val;

    }
    return 0;
}

static int menu_video_adas_calibration_init(void)
{
    int i = 0;

    APP_ADDFLAGS(menu_video_adas_calibration.Flags, MENU_ITEM_FLAGS_INIT);
    menu_video_adas_calibration.SelSaved = 0;
    menu_video_adas_calibration.SelNum = 0;
    menu_video_adas_calibration.SelCur = 0;

    for (i=0; i<MENU_VIDEO_ADAS_CALIBRATION_SEL_NUM; i++) {
        if (APP_CHECKFLAGS((menu_video_item_sel_tbls[MENU_VIDEO_ADAS_CALIB]+i)->Flags, MENU_SEL_FLAGS_ENABLE)) {
            menu_video_adas_calibration_sels[menu_video_adas_calibration.SelNum] = menu_video_item_sel_tbls[MENU_VIDEO_ADAS_CALIB]+i;
            menu_video_adas_calibration.SelNum++;
        }
    }

    return 0;
}

static int menu_video_adas_calibration_get_tab_str(void)
{
    return menu_video_adas_calibration_sels[menu_video_adas_calibration.SelSaved]->Str;
}

static int menu_video_adas_calibration_get_sel_str(int ref)
{
    return menu_video_adas_calibration_sels[ref]->Str;
}

static int menu_video_adas_calibration_get_sel_bmp(int ref)
{
    return menu_video_adas_calibration_sels[ref]->Bmp;
}

static int menu_video_adas_calibration_set(void)
{
    AppMenuQuick_SetItem(MENU_VIDEO, MENU_VIDEO_ADAS_CALIB);
    AppWidget_On(WIDGET_MENU_ADAS_CALIB, 0);
    return 0;
}

static int menu_video_adas_calibration_sel_set(void)
{
    return 0;
}

// control
static MENU_TAB_s* menu_video_get_tab(void)
{
    return &menu_video;
}

static MENU_ITEM_s* menu_video_get_item(UINT32 itemId)
{
    return menu_video_item_tbl[itemId];
}

static MENU_SEL_s* menu_video_get_sel(UINT32 itemId, UINT32 selId)
{
    return &menu_video_item_sel_tbls[itemId][selId];
}

static int menu_video_set_sel_table(UINT32 itemId, MENU_SEL_s *selTbl)
{
    menu_video_item_sel_tbls[itemId] = selTbl;
    APP_REMOVEFLAGS(menu_video_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_INIT);
    return 0;
}

static int menu_video_lock_tab(void)
{
    APP_ADDFLAGS(menu_video.Flags, MENU_TAB_FLAGS_LOCKED);
    return 0;
}

static int menu_video_unlock_tab(void)
{
    APP_REMOVEFLAGS(menu_video.Flags, MENU_TAB_FLAGS_LOCKED);
    return 0;
}

static int menu_video_enable_item(UINT32 itemId)
{
    if (!APP_CHECKFLAGS(menu_video_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_ENABLE)) {
        APP_ADDFLAGS(menu_video_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_ENABLE);
        APP_REMOVEFLAGS(menu_video.Flags, MENU_TAB_FLAGS_INIT);
    }
    return 0;
}

static int menu_video_disable_item(UINT32 itemId)
{
    if (APP_CHECKFLAGS(menu_video_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_ENABLE)) {
        APP_REMOVEFLAGS(menu_video_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_ENABLE);
        APP_REMOVEFLAGS(menu_video.Flags, MENU_TAB_FLAGS_INIT);
    }
    return 0;
}

static int menu_video_lock_item(UINT32 itemId)
{
    APP_ADDFLAGS(menu_video_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_LOCKED);
    return 0;
}

static int menu_video_unlock_item(UINT32 itemId)
{
    APP_REMOVEFLAGS(menu_video_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_LOCKED);
    return 0;
}

static int menu_video_enable_sel(UINT32 itemId, UINT32 selId)
{
    if (!APP_CHECKFLAGS((menu_video_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_ENABLE)) {
        APP_ADDFLAGS((menu_video_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_ENABLE);
        APP_REMOVEFLAGS(menu_video_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_INIT);
    }
    return 0;
}

static int menu_video_disable_sel(UINT32 itemId, UINT32 selId)
{
    if (APP_CHECKFLAGS((menu_video_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_ENABLE)) {
        APP_REMOVEFLAGS((menu_video_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_ENABLE);
        APP_REMOVEFLAGS(menu_video_item_tbl[itemId]->Flags, MENU_ITEM_FLAGS_INIT);
    }
    return 0;
}

static int menu_video_lock_sel(UINT32 itemId, UINT32 selId)
{
    APP_ADDFLAGS((menu_video_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_LOCKED);
    return 0;
}

static int menu_video_unlock_sel(UINT32 itemId, UINT32 selId)
{
    APP_REMOVEFLAGS((menu_video_item_sel_tbls[itemId]+selId)->Flags, MENU_SEL_FLAGS_LOCKED);
    return 0;
}
