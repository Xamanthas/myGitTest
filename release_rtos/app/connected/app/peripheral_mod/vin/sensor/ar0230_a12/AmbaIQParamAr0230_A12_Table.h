/**
 * @file app/connected/app/peripheral_mode/win/sensor/AmbaIQParamAr0230_A12_Table.h
 *
 * Implementation of SONY AR0230 related settings.
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

#ifndef __AMBA_IQPARAM_AR0230_A12_TABLE_H__
#define __AMBA_IQPARAM_AR0230_A12_TABLE_H__


#include <imgproc/AmbaImg_AaaDef.h>
#include <imgproc/AmbaImg_Adjustment_Def.h>
#include <3a/iqparam/ApplibIQParamHandler.h>
extern IMG_PARAM_s AmbaIQParamAr0230ImageParam;
extern AAA_PARAM_s AmbaIQParamAr0230DefParams;
extern ADJ_TABLE_PARAM_s AmbaIQParamAr0230AdjTableParam;
extern ADJ_VIDEO_PARAM_s AmbaIQParamAr0230AdjVideoPc00;
extern ADJ_VIDEO_PARAM_s AmbaIQParamAr0230AdjVideoPc01;
extern ADJ_VIDEO_HISO_PARAM_s AmbaIQParamAr0230AdjVideoHIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamAr0230AdjStillLIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamAr0230AdjStillLIso01;
extern ADJ_PHOTO_PARAM_s AmbaIQParamAr0230AdjPhotoPreview00;
extern ADJ_PHOTO_PARAM_s AmbaIQParamAr0230AdjPhotoPreview01;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamAr0230AdjStillHIso00;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamAr0230AdjStillHIso01;
extern ADJ_STILL_IDX_INFO_s AmbaIQParamAr0230StillParam;
extern ADJ_VIDEO_IDX_INFO_s AmbaIQParamAr0230VideoParam;
extern SCENE_DATA_s SceneDataS01Ar0230A12[8];
extern SCENE_DATA_s SceneDataS02Ar0230A12[8];
extern SCENE_DATA_s SceneDataS03Ar0230A12[8];
extern SCENE_DATA_s SceneDataS04Ar0230A12[8];
extern SCENE_DATA_s SceneDataS05Ar0230A12[8];
extern DE_PARAM_s DeVideoParamAr0230A12;
extern DE_PARAM_s DeStillParamAr0230A12;
//extern CALIBRATION_PARAM_s AmbaIQParamAr0230CalibParams;

COLOR_TABLE_PATH_s GCcTableAR0230[10] = {
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


IP_TABLE_PATH_s GVideoAdjTablePathAR0230[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Ar0230",
    "adj_video_default_01_Ar0230",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathAR0230[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Ar0230",
//    "adj_hiso_video_default_01",
};

IP_TABLE_PATH_s GPhotoAdjTablePathAR0230[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Ar0230",
    "adj_photo_default_01_Ar0230",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathAR0230[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Ar0230",
    "adj_still_default_01_Ar0230",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathAR0230[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Ar0230",
//    "adj_hiso_still_default_01_Ar0230",
};

IP_TABLE_PATH_s GSceneDataTablePathAR0230[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Ar0230",
    "scene_data_s02_Ar0230",
    "scene_data_s03_Ar0230",
    "scene_data_s04_Ar0230",
    "scene_data_s05_Ar0230",
};

IP_TABLE_PATH_s GImgAdjTablePathAR0230 ={
    "img_default_Ar0230"
};

IP_TABLE_PATH_s GAaaAdjTablePathAR0230[AAA_TABLE_VALID_NO]  ={
    "aaa_default_00_Ar0230",
    "aaa_default_01_Ar0230"
};

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathAR0230 ={
    "adj_still_idx_Ar0230"
};

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathAR0230 ={
    "adj_video_idx_Ar0230"
};

IP_TABLE_PATH_s GDeVideoTablePathAR0230 ={
    "de_default_video_Ar0230"
};

IP_TABLE_PATH_s GDeStillTablePathAR0230 ={
    "de_default_still_Ar0230"
};
IP_TABLE_PATH_s GAdjTablePathAR0230 ={
    "adj_table_param_default_Ar0230"
};

IP_TABLE_ADDR_s GImgAddrAR0230;
IP_TABLE_ADDR_s GAaaAddrAR0230;
IP_TABLE_ADDR_s GAdjTableAR0230;
IP_TABLE_ADDR_s GStillIdxInfoAR0230;
IP_TABLE_ADDR_s GVideoIdxInfoAR0230;
IP_TABLE_ADDR_s GDeVideoAR0230;
IP_TABLE_ADDR_s GDeStillAR0230;

#endif
