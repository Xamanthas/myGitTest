/**
 * @file app/connected/app/peripheral_mode/win/sensor/AmbaIQParamMn34229_A12.c
 *
 * Implementation of SONY MN34229 related settings.
 *
 * History:
 *    2013/01/08 - [Eddie Chen] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBA_IQPARAM_MN34229_A12_TABLE_H__
#define __AMBA_IQPARAM_MN34229_A12_TABLE_H__

#include <imgproc/AmbaImg_Adjustment_A12.h>
#include <imgproc/AmbaImg_AaaDef.h>
#include <imgproc/AmbaImg_Adjustment_Def.h>

extern IMG_PARAM_s AmbaIQParamMn34229ImageParam;
extern AAA_PARAM_s AmbaIQParamMn34229DefParams;
extern ADJ_TABLE_PARAM_s AmbaIQParamMn34229AdjTableParam;
extern ADJ_VIDEO_PARAM_s AmbaIQParamMn34229AdjVideoPc00;
extern ADJ_VIDEO_PARAM_s AmbaIQParamMn34229AdjVideoPc01;
extern ADJ_VIDEO_PARAM_s AmbaIQParamMn34229AdjVideoHIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamMn34229AdjStillLIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamMn34229AdjStillLIso01;
extern ADJ_PHOTO_PARAM_s AmbaIQParamMn34229AdjPhotoPreview00;
extern ADJ_PHOTO_PARAM_s AmbaIQParamMn34229AdjPhotoPreview01;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamMn34229AdjStillHIso00;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamMn34229AdjStillHIso01;
extern ADJ_STILL_IDX_INFO_s AmbaIQParamMn34229StillParam;
extern ADJ_VIDEO_IDX_INFO_s AmbaIQParamMn34229VideoParam;
extern SCENE_DATA_s SceneDataS01Mn34229A12[8];
extern SCENE_DATA_s SceneDataS02Mn34229A12[8];
extern SCENE_DATA_s SceneDataS03Mn34229A12[8];
extern SCENE_DATA_s SceneDataS04Mn34229A12[8];
extern SCENE_DATA_s SceneDataS05Mn34229A12[8];
extern DE_PARAM_s DeVideoParamMn34229A12;
extern DE_PARAM_s DeStillParamMn34229A12;
//extern CALIBRATION_PARAM_s AmbaIQParamMn34229CalibParams;

COLOR_TABLE_PATH_s GCcTableMN34229[10] = {
    { 0,
      "VideoCc0",
      "VideoCc1",
      "VideoCc2",
      "VideoCc3",
      "VideoCc4",
      "StillCc0",
      "StillCc1",
      "StillCc2",
      "StillCc3",
      "StillCc4",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video.bin",
      "cc3d_cc_bw_gamma_lin_video.bin",
      "cc3d_cc_bw_gamma_lin_video.bin",
      "cc3d_cc_bw_gamma_lin_video.bin",
      "cc3d_cc_bw_gamma_lin_video.bin",
      "cc3d_cc_bw_gamma_lin_still.bin",
      "cc3d_cc_bw_gamma_lin_still.bin",
      "cc3d_cc_bw_gamma_lin_still.bin",
      "cc3d_cc_bw_gamma_lin_still.bin",
      "cc3d_cc_bw_gamma_lin_still.bin",
    },
    { -1 },
};


IP_TABLE_PATH_s GVideoAdjTablePathMN34229[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Mn34229",
    "adj_video_default_01_Mn34229",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathMN34229[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Mn34229",
//    "adj_hiso_video_default_01_Mn34229",
};


IP_TABLE_PATH_s GPhotoAdjTablePathMN34229[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Mn34229",
    "adj_photo_default_01_Mn34229",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathMN34229[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Mn34229",
    "adj_still_default_01_Mn34229",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathMN34229[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Mn34229",
//    "adj_hiso_still_default_01_Mn34229",
};

IP_TABLE_PATH_s GSceneDataTablePathMN34229[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Mn34229",
    "scene_data_s02_Mn34229",
    "scene_data_s03_Mn34229",
    "scene_data_s04_Mn34229",
    "scene_data_s05_Mn34229",
};

IP_TABLE_PATH_s GImgAdjTablePathMN34229 ={
    "img_default_Mn34229"
};

IP_TABLE_PATH_s GAaaAdjTablePathMN34229[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Mn34229",
    "aaa_default_01_Mn34229"
};

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathMN34229 ={
    "adj_still_idx_Mn34229"
};

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathMN34229 ={
    "adj_video_idx_Mn34229"
};

IP_TABLE_PATH_s GDeVideoTablePathMN34229 ={
    "de_default_video_Mn34229"
};

IP_TABLE_PATH_s GDeStillTablePathMN34229 ={
    "de_default_still_Mn34229"
};
IP_TABLE_PATH_s GAdjTablePathMN34229 ={
    "adj_table_param_default_Mn34229"
};


#endif
