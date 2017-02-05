/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamOv4689hdr_A12_Table.h
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

#ifndef __AMBA_IQPARAM_OV4689HDR_A12_TABLE_H__
#define __AMBA_IQPARAM_OV4689HDR_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableOV4689hdr[10] = {
    { 0,
      "VideoCc0_Ov4689hdr",
      "VideoCc1_Ov4689hdr",
      "VideoCc2_Ov4689hdr",
      "VideoCc3_Ov4689hdr",
      "VideoCc4_Ov4689hdr",
      "StillCc0_Ov4689hdr",
      "StillCc1_Ov4689hdr",
      "StillCc2_Ov4689hdr",
      "StillCc3_Ov4689hdr",
      "StillCc4_Ov4689hdr",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Ov4689hdr",
      "cc3d_cc_bw_gamma_lin_video_Ov4689hdr",
      "cc3d_cc_bw_gamma_lin_video_Ov4689hdr",
      "cc3d_cc_bw_gamma_lin_video_Ov4689hdr",
      "cc3d_cc_bw_gamma_lin_video_Ov4689hdr",
      "cc3d_cc_bw_gamma_lin_still_Ov4689hdr",
      "cc3d_cc_bw_gamma_lin_still_Ov4689hdr",
      "cc3d_cc_bw_gamma_lin_still_Ov4689hdr",
      "cc3d_cc_bw_gamma_lin_still_Ov4689hdr",
      "cc3d_cc_bw_gamma_lin_still_Ov4689hdr",
    },
    { -1 },
};


IP_TABLE_PATH_s GVideoAdjTablePathOV4689hdr[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Ov4689hdr",
    "adj_video_default_01_Ov4689hdr",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathOV4689hdr[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Ov4689hdr",
//    "adj_hiso_video_default_01",
};

IP_TABLE_PATH_s GPhotoAdjTablePathOV4689hdr[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Ov4689hdr",
    "adj_photo_default_01_Ov4689hdr",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathOV4689hdr[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Ov4689hdr",
    "adj_still_default_01_Ov4689hdr",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathOV4689hdr[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Ov4689hdr",
//    "adj_hiso_still_default_01_Ov4689hdr",
};

IP_TABLE_PATH_s GSceneDataTablePathOV4689hdr[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Ov4689hdr",
    "scene_data_s02_Ov4689hdr",
    "scene_data_s03_Ov4689hdr",
    "scene_data_s04_Ov4689hdr",
    "scene_data_s05_Ov4689hdr",
};

IP_TABLE_PATH_s GImgAdjTablePathOV4689hdr ={
    "img_default_Ov4689hdr"
};    

IP_TABLE_PATH_s GAaaAdjTablePathOV4689hdr[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Ov4689hdr",
    "aaa_default_01_Ov4689hdr"
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathOV4689hdr ={
    "adj_still_idx_Ov4689hdr"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathOV4689hdr ={
    "adj_video_idx_Ov4689hdr"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathOV4689hdr ={
    "de_default_video_Ov4689hdr"
};   

IP_TABLE_PATH_s GDeStillTablePathOV4689hdr ={
    "de_default_still_Ov4689hdr"
};    
IP_TABLE_PATH_s GAdjTablePathOV4689hdr ={
    "adj_table_param_default_Ov4689hdr"
};        

IP_TABLE_ADDR_s GImgAddrOV4689hdr;
IP_TABLE_ADDR_s GAaaAddrOV4689hdr;
IP_TABLE_ADDR_s GAdjTableOV4689hdr;
IP_TABLE_ADDR_s GStillIdxInfoOV4689hdr;
IP_TABLE_ADDR_s GVideoIdxInfoOV4689hdr;
IP_TABLE_ADDR_s GDeVideoOV4689hdr;
IP_TABLE_ADDR_s GDeStillOV4689hdr;

#endif