/**
 * @file app/connected/app/peripheral_mode/win/sensor/AmbaIQParamMn34222_A12.c
 *
 * Implementation of SONY MN34222 related settings.
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

#ifndef __AMBA_IQPARAM_MN34222_A12_TABLE_H__
#define __AMBA_IQPARAM_MN34222_A12_TABLE_H__

#include <imgproc/AmbaImg_Adjustment_A12.h>
#include <imgproc/AmbaImg_AaaDef.h>
#include <imgproc/AmbaImg_Adjustment_Def.h>

extern IMG_PARAM_s AmbaIQParamMn34222ImageParam;
extern AAA_PARAM_s AmbaIQParamMn34222DefParams;
extern ADJ_TABLE_PARAM_s AmbaIQParamMn34222AdjTableParam;
extern ADJ_VIDEO_PARAM_s AmbaIQParamMn34222AdjVideoPc00;
extern ADJ_VIDEO_PARAM_s AmbaIQParamMn34222AdjVideoPc01;
extern ADJ_VIDEO_PARAM_s AmbaIQParamMn34222AdjVideoHIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamMn34222AdjStillLIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamMn34222AdjStillLIso01;
extern ADJ_PHOTO_PARAM_s AmbaIQParamMn34222AdjPhotoPreview00;
extern ADJ_PHOTO_PARAM_s AmbaIQParamMn34222AdjPhotoPreview01;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamMn34222AdjStillHIso00;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamMn34222AdjStillHIso01;
extern ADJ_STILL_IDX_INFO_s AmbaIQParamMn34222StillParam;
extern ADJ_VIDEO_IDX_INFO_s AmbaIQParamMn34222VideoParam;
extern SCENE_DATA_s SceneDataS01Mn34222A12[8];
extern SCENE_DATA_s SceneDataS02Mn34222A12[8];
extern SCENE_DATA_s SceneDataS03Mn34222A12[8];
extern SCENE_DATA_s SceneDataS04Mn34222A12[8];
extern SCENE_DATA_s SceneDataS05Mn34222A12[8];
extern DE_PARAM_s DeVideoParamMn34222A12;
extern DE_PARAM_s DeStillParamMn34222A12;
//extern CALIBRATION_PARAM_s AmbaIQParamMn34222CalibParams;

COLOR_TABLE_PATH_s GCcTableMN34222[10] = {
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


IP_TABLE_PATH_s GVideoAdjTablePathMN34222[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Mn34222",
    "adj_video_default_01_Mn34222",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathMN34222[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Mn34222",
//    "adj_hiso_video_default_01_Mn34222",
};


IP_TABLE_PATH_s GPhotoAdjTablePathMN34222[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Mn34222",
    "adj_photo_default_01_Mn34222",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathMN34222[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Mn34222",
    "adj_still_default_01_Mn34222",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathMN34222[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Mn34222",
//    "adj_hiso_still_default_01_Mn34222",
};

IP_TABLE_PATH_s GSceneDataTablePathMN34222[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Mn34222",
    "scene_data_s02_Mn34222",
    "scene_data_s03_Mn34222",
    "scene_data_s04_Mn34222",
    "scene_data_s05_Mn34222",
};

IP_TABLE_PATH_s GImgAdjTablePathMN34222 ={
    "img_default_Mn34222"
};

IP_TABLE_PATH_s GAaaAdjTablePathMN34222[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Mn34222",
    "aaa_default_01_Mn34222"
};

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathMN34222 ={
    "adj_still_idx_Mn34222"
};

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathMN34222 ={
    "adj_video_idx_Mn34222"
};

IP_TABLE_PATH_s GDeVideoTablePathMN34222 ={
    "de_default_video_Mn34222"
};

IP_TABLE_PATH_s GDeStillTablePathMN34222 ={
    "de_default_still_Mn34222"
};
IP_TABLE_PATH_s GAdjTablePathMN34222 ={
    "adj_table_param_default_Mn34222"
};


#endif
