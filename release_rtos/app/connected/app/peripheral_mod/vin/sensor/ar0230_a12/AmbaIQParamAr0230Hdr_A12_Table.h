/**
 * @file app/connected/app/peripheral_mode/win/sensor/AmbaIQParamAr0230Hdr_A12_Table.h
 *
 * Implementation of SONY Ar0230Hdr related settings.
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

#ifndef __AMBA_IQPARAM_AR0230HDR_A12_TABLE_H__
#define __AMBA_IQPARAM_AR0230HDR_A12_TABLE_H__


#include <imgproc/AmbaImg_AaaDef.h>
#include <imgproc/AmbaImg_Adjustment_Def.h>
#include <3a/iqparam/ApplibIQParamHandler.h>
extern IMG_PARAM_s AmbaIQParamAr0230hdrImageParam;
extern AAA_PARAM_s AmbaIQParamAr0230hdrDefParams;
extern ADJ_TABLE_PARAM_s AmbaIQParamAr0230hdrAdjTableParam;
extern ADJ_VIDEO_PARAM_s AmbaIQParamAr0230hdrAdjVideoPc00;
extern ADJ_VIDEO_PARAM_s AmbaIQParamAr0230hdrAdjVideoPc01;
extern ADJ_VIDEO_HISO_PARAM_s AmbaIQParamAr0230hdrAdjVideoHIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamAr0230hdrAdjStillLIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamAr0230hdrAdjStillLIso01;
extern ADJ_PHOTO_PARAM_s AmbaIQParamAr0230hdrAdjPhotoPreview00;
extern ADJ_PHOTO_PARAM_s AmbaIQParamAr0230hdrAdjPhotoPreview01;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamAr0230hdrAdjStillHIso00;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamAr0230hdrAdjStillHIso01;
extern ADJ_STILL_IDX_INFO_s AmbaIQParamAr0230hdrStillParam;
extern ADJ_VIDEO_IDX_INFO_s AmbaIQParamAr0230hdrVideoParam;
extern SCENE_DATA_s SceneDataS01Ar0230hdrA12[8];
extern SCENE_DATA_s SceneDataS02Ar0230hdrA12[8];
extern SCENE_DATA_s SceneDataS03Ar0230hdrA12[8];
extern SCENE_DATA_s SceneDataS04Ar0230hdrA12[8];
extern SCENE_DATA_s SceneDataS05Ar0230hdrA12[8];
extern DE_PARAM_s DeVideoParamAr0230hdrA12;
extern DE_PARAM_s DeStillParamAr0230hdrA12;
//extern CALIBRATION_PARAM_s AmbaIQParamAr0230hdrCalibParams;

COLOR_TABLE_PATH_s GCcTableAR0230hdr[10] = {
    { 0,
      "VideoCc0_Ar0230hdr",
      "VideoCc1_Ar0230hdr",
      "VideoCc2_Ar0230hdr",
      "VideoCc3_Ar0230hdr",
      "VideoCc4_Ar0230hdr",
      "StillCc0_Ar0230hdr",
      "StillCc1_Ar0230hdr",
      "StillCc2_Ar0230hdr",
      "StillCc3_Ar0230hdr",
      "StillCc4_Ar0230hdr",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Ar0230hdr",
      "cc3d_cc_bw_gamma_lin_video_Ar0230hdr",
      "cc3d_cc_bw_gamma_lin_video_Ar0230hdr",
      "cc3d_cc_bw_gamma_lin_video_Ar0230hdr",
      "cc3d_cc_bw_gamma_lin_video_Ar0230hdr",
      "cc3d_cc_bw_gamma_lin_still_Ar0230hdr",
      "cc3d_cc_bw_gamma_lin_still_Ar0230hdr",
      "cc3d_cc_bw_gamma_lin_still_Ar0230hdr",
      "cc3d_cc_bw_gamma_lin_still_Ar0230hdr",
      "cc3d_cc_bw_gamma_lin_still_Ar0230hdr",
    },
    { -1 },
};


IP_TABLE_PATH_s GVideoAdjTablePathAR0230hdr[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Ar0230hdr",
    "adj_video_default_01_Ar0230hdr",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathAR0230hdr[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Ar0230hdr",
//    "adj_hiso_video_default_01",
};

IP_TABLE_PATH_s GPhotoAdjTablePathAR0230hdr[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Ar0230hdr",
    "adj_photo_default_01_Ar0230hdr",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathAR0230hdr[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Ar0230hdr",
    "adj_still_default_01_Ar0230hdr",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathAR0230hdr[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Ar0230hdr",
//    "adj_hiso_still_default_01_Ar0230hdr",
};

IP_TABLE_PATH_s GSceneDataTablePathAR0230hdr[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Ar0230hdr",
    "scene_data_s02_Ar0230hdr",
    "scene_data_s03_Ar0230hdr",
    "scene_data_s04_Ar0230hdr",
    "scene_data_s05_Ar0230hdr",
};

IP_TABLE_PATH_s GImgAdjTablePathAR0230hdr ={
    "img_default_Ar0230hdr"
};

IP_TABLE_PATH_s GAaaAdjTablePathAR0230hdr[AAA_TABLE_VALID_NO]  ={
    "aaa_default_00_Ar0230hdr",
    "aaa_default_01_Ar0230hdr"
};

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathAR0230hdr ={
    "adj_still_idx_Ar0230hdr"
};

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathAR0230hdr ={
    "adj_video_idx_Ar0230hdr"
};

IP_TABLE_PATH_s GDeVideoTablePathAR0230hdr ={
    "de_default_video_Ar0230hdr"
};

IP_TABLE_PATH_s GDeStillTablePathAR0230hdr ={
    "de_default_still_Ar0230hdr"
};
IP_TABLE_PATH_s GAdjTablePathAR0230hdr ={
    "adj_table_param_default_Ar0230hdr"
};

IP_TABLE_ADDR_s GImgAddrAR0230hdr;
IP_TABLE_ADDR_s GAaaAddrAR0230hdr;
IP_TABLE_ADDR_s GAdjTableAR0230hdr;
IP_TABLE_ADDR_s GStillIdxInfoAR0230hdr;
IP_TABLE_ADDR_s GVideoIdxInfoAR0230hdr;
IP_TABLE_ADDR_s GDeVideoAR0230hdr;
IP_TABLE_ADDR_s GDeStillAR0230hdr;

#endif
