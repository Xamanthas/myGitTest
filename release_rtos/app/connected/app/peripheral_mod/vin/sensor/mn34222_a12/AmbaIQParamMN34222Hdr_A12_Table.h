/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamMn34222hdr_A12_Table.h
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: MN34222 Image IQ parameters tables
 *
 *  @History        ::
 *      Date        Name        Comments
 *      07/29/2015  Ming Gao    Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef __AMBA_IQPARAM_MN34222HDR_A12_TABLE_H__
#define __AMBA_IQPARAM_MN34222HDR_A12_TABLE_H__
#include <imgproc/AmbaImg_AaaDef.h>
#include <imgproc/AmbaImg_Adjustment_Def.h>
#include <3a/iqparam/ApplibIQParamHandler.h>


COLOR_TABLE_PATH_s GCcTableMN34222hdr[10] = {
    { 0,
      "VideoCc0_Mn34222hdr",
      "VideoCc1_Mn34222hdr",
      "VideoCc2_Mn34222hdr",
      "VideoCc3_Mn34222hdr",
      "VideoCc4_Mn34222hdr",
      "StillCc0_Mn34222hdr",
      "StillCc1_Mn34222hdr",
      "StillCc2_Mn34222hdr",
      "StillCc3_Mn34222hdr",
      "StillCc4_Mn34222hdr",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Mn34222hdr",
      "cc3d_cc_bw_gamma_lin_video_Mn34222hdr",
      "cc3d_cc_bw_gamma_lin_video_Mn34222hdr",
      "cc3d_cc_bw_gamma_lin_video_Mn34222hdr",
      "cc3d_cc_bw_gamma_lin_video_Mn34222hdr",
      "cc3d_cc_bw_gamma_lin_still_Mn34222hdr",
      "cc3d_cc_bw_gamma_lin_still_Mn34222hdr",
      "cc3d_cc_bw_gamma_lin_still_Mn34222hdr",
      "cc3d_cc_bw_gamma_lin_still_Mn34222hdr",
      "cc3d_cc_bw_gamma_lin_still_Mn34222hdr",
    },
    { -1 },
};


IP_TABLE_PATH_s GVideoAdjTablePathMN34222hdr[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Mn34222hdr",
    "adj_video_default_01_Mn34222hdr",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathMN34222hdr[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Mn34222hdr",
//    "adj_hiso_video_default_01",
};

IP_TABLE_PATH_s GPhotoAdjTablePathMN34222hdr[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Mn34222hdr",
    "adj_photo_default_01_Mn34222hdr",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathMN34222hdr[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Mn34222hdr",
    "adj_still_default_01_Mn34222hdr",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathMN34222hdr[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Mn34222hdr",
//    "adj_hiso_still_default_01_Mn34222hdr",
};

IP_TABLE_PATH_s GSceneDataTablePathMN34222hdr[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Mn34222hdr",
    "scene_data_s02_Mn34222hdr",
    "scene_data_s03_Mn34222hdr",
    "scene_data_s04_Mn34222hdr",
    "scene_data_s05_Mn34222hdr",
};

IP_TABLE_PATH_s GImgAdjTablePathMN34222hdr ={
    "img_default_Mn34222hdr"
};

IP_TABLE_PATH_s GAaaAdjTablePathMN34222hdr[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Mn34222hdr",
    "aaa_default_01_Mn34222hdr"
};

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathMN34222hdr ={
    "adj_still_idx_Mn34222hdr"
};

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathMN34222hdr ={
    "adj_video_idx_Mn34222hdr"
};

IP_TABLE_PATH_s GDeVideoTablePathMN34222hdr ={
    "de_default_video_Mn34222hdr"
};

IP_TABLE_PATH_s GDeStillTablePathMN34222hdr ={
    "de_default_still_Mn34222hdr"
};
IP_TABLE_PATH_s GAdjTablePathMN34222hdr ={
    "adj_table_param_default_Mn34222hdr"
};

IP_TABLE_ADDR_s GImgAddrMN34222hdr;
IP_TABLE_ADDR_s GAaaAddrMN34222hdr;
IP_TABLE_ADDR_s GAdjTableMN34222hdr;
IP_TABLE_ADDR_s GStillIdxInfoMN34222hdr;
IP_TABLE_ADDR_s GVideoIdxInfoMN34222hdr;
IP_TABLE_ADDR_s GDeVideoMN34222hdr;
IP_TABLE_ADDR_s GDeStillMN34222hdr;

#endif