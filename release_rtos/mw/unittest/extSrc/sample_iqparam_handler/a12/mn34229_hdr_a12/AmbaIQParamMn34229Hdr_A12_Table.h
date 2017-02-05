/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamMn34229hdr_A12_Table.h
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

#ifndef __AMBA_IQPARAM_MN34229HDR_A12_TABLE_H__
#define __AMBA_IQPARAM_MN34229HDR_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableMN34229hdr[10] = {
    { 0,
      "VideoCc0_Mn34229hdr",
      "VideoCc1_Mn34229hdr",
      "VideoCc2_Mn34229hdr",
      "VideoCc3_Mn34229hdr",
      "VideoCc4_Mn34229hdr",
      "StillCc0_Mn34229hdr",
      "StillCc1_Mn34229hdr",
      "StillCc2_Mn34229hdr",
      "StillCc3_Mn34229hdr",
      "StillCc4_Mn34229hdr",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Mn34229hdr",
      "cc3d_cc_bw_gamma_lin_video_Mn34229hdr",
      "cc3d_cc_bw_gamma_lin_video_Mn34229hdr",
      "cc3d_cc_bw_gamma_lin_video_Mn34229hdr",
      "cc3d_cc_bw_gamma_lin_video_Mn34229hdr",
      "cc3d_cc_bw_gamma_lin_still_Mn34229hdr",
      "cc3d_cc_bw_gamma_lin_still_Mn34229hdr",
      "cc3d_cc_bw_gamma_lin_still_Mn34229hdr",
      "cc3d_cc_bw_gamma_lin_still_Mn34229hdr",
      "cc3d_cc_bw_gamma_lin_still_Mn34229hdr",
    },
    { -1 },
};


IP_TABLE_PATH_s GVideoAdjTablePathMN34229hdr[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Mn34229hdr",
    "adj_video_default_01_Mn34229hdr",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathMN34229hdr[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Mn34229hdr",
//    "adj_hiso_video_default_01",
};

IP_TABLE_PATH_s GPhotoAdjTablePathMN34229hdr[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Mn34229hdr",
    "adj_photo_default_01_Mn34229hdr",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathMN34229hdr[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Mn34229hdr",
    "adj_still_default_01_Mn34229hdr",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathMN34229hdr[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Mn34229hdr",
//    "adj_hiso_still_default_01_Mn34229hdr",
};

IP_TABLE_PATH_s GSceneDataTablePathMN34229hdr[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Mn34229hdr",
    "scene_data_s02_Mn34229hdr",
    "scene_data_s03_Mn34229hdr",
    "scene_data_s04_Mn34229hdr",
    "scene_data_s05_Mn34229hdr",
};

IP_TABLE_PATH_s GImgAdjTablePathMN34229hdr ={
    "img_default_Mn34229hdr"
};    

IP_TABLE_PATH_s GAaaAdjTablePathMN34229hdr[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Mn34229hdr",
    "aaa_default_01_Mn34229hdr"
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathMN34229hdr ={
    "adj_still_idx_Mn34229hdr"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathMN34229hdr ={
    "adj_video_idx_Mn34229hdr"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathMN34229hdr ={
    "de_default_video_Mn34229hdr"
};   

IP_TABLE_PATH_s GDeStillTablePathMN34229hdr ={
    "de_default_still_Mn34229hdr"
};    
IP_TABLE_PATH_s GAdjTablePathMN34229hdr ={
    "adj_table_param_default_Mn34229hdr"
};        

IP_TABLE_ADDR_s GImgAddrMN34229hdr;
IP_TABLE_ADDR_s GAaaAddrMN34229hdr;
IP_TABLE_ADDR_s GAdjTableMN34229hdr;
IP_TABLE_ADDR_s GStillIdxInfoMN34229hdr;
IP_TABLE_ADDR_s GVideoIdxInfoMN34229hdr;
IP_TABLE_ADDR_s GDeVideoMN34229hdr;
IP_TABLE_ADDR_s GDeStillMN34229hdr;

#endif