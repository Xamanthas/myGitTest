/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamImx290hdr_A12_Table.h
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Imx144 Image IQ parameters tables
 *
 *  @History        ::
 *      Date        Name        Comments
 *      01/08/2013  Eddie Chen  Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef __AMBA_IQPARAM_IMX290HDR_A12_TABLE_H__
#define __AMBA_IQPARAM_IMX290HDR_A12_TABLE_H__
#include <imgproc/AmbaImg_AaaDef.h>
#include <imgproc/AmbaImg_Adjustment_Def.h>
#include <3a/iqparam/ApplibIQParamHandler.h>


COLOR_TABLE_PATH_s GCcTableIMX290hdr[10] = {
    { 0,
      "VideoCc0_Imx290hdr",
      "VideoCc1_Imx290hdr",
      "VideoCc2_Imx290hdr",
      "VideoCc3_Imx290hdr",
      "VideoCc4_Imx290hdr",
      "StillCc0_Imx290hdr",
      "StillCc1_Imx290hdr",
      "StillCc2_Imx290hdr",
      "StillCc3_Imx290hdr",
      "StillCc4_Imx290hdr",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Imx290hdr",
      "cc3d_cc_bw_gamma_lin_video_Imx290hdr",
      "cc3d_cc_bw_gamma_lin_video_Imx290hdr",
      "cc3d_cc_bw_gamma_lin_video_Imx290hdr",
      "cc3d_cc_bw_gamma_lin_video_Imx290hdr",
      "cc3d_cc_bw_gamma_lin_still_Imx290hdr",
      "cc3d_cc_bw_gamma_lin_still_Imx290hdr",
      "cc3d_cc_bw_gamma_lin_still_Imx290hdr",
      "cc3d_cc_bw_gamma_lin_still_Imx290hdr",
      "cc3d_cc_bw_gamma_lin_still_Imx290hdr",
    },
    { -1 },
};


IP_TABLE_PATH_s GVideoAdjTablePathIMX290hdr[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Imx290hdr",
    "adj_video_default_01_Imx290hdr",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathIMX290hdr[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Imx290hdr",
//    "adj_hiso_video_default_01",
};

IP_TABLE_PATH_s GPhotoAdjTablePathIMX290hdr[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Imx290hdr",
    "adj_photo_default_01_Imx290hdr",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathIMX290hdr[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Imx290hdr",
    "adj_still_default_01_Imx290hdr",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathIMX290hdr[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Imx290hdr",
//    "adj_hiso_still_default_01_Imx290hdr",
};

IP_TABLE_PATH_s GSceneDataTablePathIMX290hdr[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Imx290hdr",
    "scene_data_s02_Imx290hdr",
    "scene_data_s03_Imx290hdr",
    "scene_data_s04_Imx290hdr",
    "scene_data_s05_Imx290hdr",
};

IP_TABLE_PATH_s GImgAdjTablePathIMX290hdr ={
    "img_default_Imx290hdr"
};

IP_TABLE_PATH_s GAaaAdjTablePathIMX290hdr[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Imx290hdr",
    "aaa_default_01_Imx290hdr"
};

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathIMX290hdr ={
    "adj_still_idx_Imx290hdr"
};

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathIMX290hdr ={
    "adj_video_idx_Imx290hdr"
};

IP_TABLE_PATH_s GDeVideoTablePathIMX290hdr ={
    "de_default_video_Imx290hdr"
};

IP_TABLE_PATH_s GDeStillTablePathIMX290hdr ={
    "de_default_still_Imx290hdr"
};
IP_TABLE_PATH_s GAdjTablePathIMX290hdr ={
    "adj_table_param_default_Imx290hdr"
};

IP_TABLE_ADDR_s GImgAddrIMX290hdr;
IP_TABLE_ADDR_s GAaaAddrIMX290hdr;
IP_TABLE_ADDR_s GAdjTableIMX290hdr;
IP_TABLE_ADDR_s GStillIdxInfoIMX290hdr;
IP_TABLE_ADDR_s GVideoIdxInfoIMX290hdr;
IP_TABLE_ADDR_s GDeVideoIMX290hdr;
IP_TABLE_ADDR_s GDeStillIMX290hdr;

#endif