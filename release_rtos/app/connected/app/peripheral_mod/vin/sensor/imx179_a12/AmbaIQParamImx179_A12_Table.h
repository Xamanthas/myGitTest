/**
 * @file app/connected/app/peripheral_mode/win/sensor/AmbaIQParamImx179_A12.c
 *
 * Implementation of SONY IMX179 related settings.
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

#ifndef __AMBA_IQPARAM_IMX179_A12_TABLE_H__
#define __AMBA_IQPARAM_IMX179_A12_TABLE_H__

#include <imgproc/AmbaImg_Adjustment_A12.h>
#include <imgproc/AmbaImg_AaaDef.h>
#include <imgproc/AmbaImg_Adjustment_Def.h>

extern IMG_PARAM_s AmbaIQParamImx179ImageParam;
extern AAA_PARAM_s AmbaIQParamImx179DefParams;
extern ADJ_TABLE_PARAM_s AmbaIQParamImx179AdjTableParam;
extern ADJ_VIDEO_PARAM_s AmbaIQParamImx179AdjVideoPc00;
extern ADJ_VIDEO_PARAM_s AmbaIQParamImx179AdjVideoPc01;
extern ADJ_VIDEO_PARAM_s AmbaIQParamImx179AdjVideoHIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamImx179AdjStillLIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamImx179AdjStillLIso01;
extern ADJ_PHOTO_PARAM_s AmbaIQParamImx179AdjPhotoPreview00;
extern ADJ_PHOTO_PARAM_s AmbaIQParamImx179AdjPhotoPreview01;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamImx179AdjStillHIso00;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamImx179AdjStillHIso01;
extern ADJ_STILL_IDX_INFO_s AmbaIQParamImx179StillParam;
extern ADJ_VIDEO_IDX_INFO_s AmbaIQParamImx179VideoParam;
extern SCENE_DATA_s SceneDataS01Imx179A12[8];
extern SCENE_DATA_s SceneDataS02Imx179A12[8];
extern SCENE_DATA_s SceneDataS03Imx179A12[8];
extern SCENE_DATA_s SceneDataS04Imx179A12[8];
extern SCENE_DATA_s SceneDataS05Imx179A12[8];
extern DE_PARAM_s DeVideoParamImx179A12;
extern DE_PARAM_s DeStillParamImx179A12;
//extern CALIBRATION_PARAM_s AmbaIQParamImx179CalibParams;

COLOR_TABLE_PATH_s GCcTableIMX179[10] = {
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


IP_TABLE_PATH_s GVideoAdjTablePathIMX179[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Imx179",
    "adj_video_default_01_Imx179",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathIMX179[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Imx179",
//    "adj_hiso_video_default_01_Imx179",
};


IP_TABLE_PATH_s GPhotoAdjTablePathIMX179[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Imx179",
    "adj_photo_default_01_Imx179",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathIMX179[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Imx179",
    "adj_still_default_01_Imx179",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathIMX179[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Imx179",
//    "adj_hiso_still_default_01_Imx179",
};

IP_TABLE_PATH_s GSceneDataTablePathIMX179[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Imx179",
    "scene_data_s02_Imx179",
    "scene_data_s03_Imx179",
    "scene_data_s04_Imx179",
    "scene_data_s05_Imx179",
};

IP_TABLE_PATH_s GImgAdjTablePathIMX179 ={
    "img_default_Imx179"
};

IP_TABLE_PATH_s GAaaAdjTablePathIMX179[AAA_TABLE_VALID_NO]  ={
    "aaa_default_Imx179",
    "aaa_default_01_Imx179"
};

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathIMX179 ={
    "adj_still_idx_Imx179"
};

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathIMX179 ={
    "adj_video_idx_Imx179"
};

IP_TABLE_PATH_s GDeVideoTablePathIMX179 ={
    "de_default_video_Imx179"
};

IP_TABLE_PATH_s GDeStillTablePathIMX179 ={
    "de_default_still_Imx179"
};
IP_TABLE_PATH_s GAdjTablePathIMX179 ={
    "adj_table_param_default_Imx179"
};


#endif
