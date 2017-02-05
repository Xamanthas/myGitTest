/**
 * @file app/connected/app/peripheral_mode/win/sensor/AmbaIQParamImx290_A12.c
 *
 * Implementation of SONY IMX290 related settings.
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

#ifndef __AMBA_IQPARAM_IMX290_A12_TABLE_H__
#define __AMBA_IQPARAM_IMX290_A12_TABLE_H__

#include <imgproc/AmbaImg_Adjustment_A12.h>
#include <imgproc/AmbaImg_AaaDef.h>
#include <imgproc/AmbaImg_Adjustment_Def.h>

extern IMG_PARAM_s AmbaIQParamImx290ImageParam;
extern AAA_PARAM_s AmbaIQParamImx290DefParams;
extern ADJ_TABLE_PARAM_s AmbaIQParamImx290AdjTableParam;
extern ADJ_VIDEO_PARAM_s AmbaIQParamImx290AdjVideoPc00;
extern ADJ_VIDEO_PARAM_s AmbaIQParamImx290AdjVideoPc01;
extern ADJ_VIDEO_PARAM_s AmbaIQParamImx290AdjVideoHIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamImx290AdjStillLIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamImx290AdjStillLIso01;
extern ADJ_PHOTO_PARAM_s AmbaIQParamImx290AdjPhotoPreview00;
extern ADJ_PHOTO_PARAM_s AmbaIQParamImx290AdjPhotoPreview01;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamImx290AdjStillHIso00;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamImx290AdjStillHIso01;
extern ADJ_STILL_IDX_INFO_s AmbaIQParamImx290StillParam;
extern ADJ_VIDEO_IDX_INFO_s AmbaIQParamImx290VideoParam;
extern SCENE_DATA_s SceneDataS01Imx290A12[8];
extern SCENE_DATA_s SceneDataS02Imx290A12[8];
extern SCENE_DATA_s SceneDataS03Imx290A12[8];
extern SCENE_DATA_s SceneDataS04Imx290A12[8];
extern SCENE_DATA_s SceneDataS05Imx290A12[8];
extern DE_PARAM_s DeVideoParamImx290A12;
extern DE_PARAM_s DeStillParamImx290A12;
//extern CALIBRATION_PARAM_s AmbaIQParamImx290CalibParams;

COLOR_TABLE_PATH_s GCcTableIMX290[10] = {
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


IP_TABLE_PATH_s GVideoAdjTablePathIMX290[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Imx290",
    "adj_video_default_01_Imx290",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathIMX290[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Imx290",
//    "adj_hiso_video_default_01_Imx290",
};


IP_TABLE_PATH_s GPhotoAdjTablePathIMX290[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Imx290",
    "adj_photo_default_01_Imx290",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathIMX290[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Imx290",
    "adj_still_default_01_Imx290",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathIMX290[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Imx290",
//    "adj_hiso_still_default_01_Imx290",
};

IP_TABLE_PATH_s GSceneDataTablePathIMX290[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Imx290",
    "scene_data_s02_Imx290",
    "scene_data_s03_Imx290",
    "scene_data_s04_Imx290",
    "scene_data_s05_Imx290",
};

IP_TABLE_PATH_s GImgAdjTablePathIMX290 ={
    "img_default_Imx290"
};

IP_TABLE_PATH_s GAaaAdjTablePathIMX290[AAA_TABLE_VALID_NO]  ={
    "aaa_default_00_Imx290",
    "aaa_default_01_Imx290"
};

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathIMX290 ={
    "adj_still_idx_Imx290"
};

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathIMX290 ={
    "adj_video_idx_Imx290"
};

IP_TABLE_PATH_s GDeVideoTablePathIMX290 ={
    "de_default_video_Imx290"
};

IP_TABLE_PATH_s GDeStillTablePathIMX290 ={
    "de_default_still_Imx290"
};
IP_TABLE_PATH_s GAdjTablePathIMX290 ={
    "adj_table_param_default_Imx290"
};


#endif
