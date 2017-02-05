/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamOv4689_A12_Table.h
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

#ifndef __AMBA_IQPARAM_OV4689_A12_TABLE_H__
#define __AMBA_IQPARAM_OV4689_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableOV4689[10] = {
    { 0,
      "VideoCc0_Ov4689",
      "VideoCc1_Ov4689",
      "VideoCc2_Ov4689",
      "VideoCc3_Ov4689",
      "VideoCc4_Ov4689",
      "StillCc0_Ov4689",
      "StillCc1_Ov4689",
      "StillCc2_Ov4689",
      "StillCc3_Ov4689",
      "StillCc4_Ov4689",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Ov4689",
      "cc3d_cc_bw_gamma_lin_video_Ov4689",
      "cc3d_cc_bw_gamma_lin_video_Ov4689",
      "cc3d_cc_bw_gamma_lin_video_Ov4689",
      "cc3d_cc_bw_gamma_lin_video_Ov4689",
      "cc3d_cc_bw_gamma_lin_still_Ov4689",
      "cc3d_cc_bw_gamma_lin_still_Ov4689",
      "cc3d_cc_bw_gamma_lin_still_Ov4689",
      "cc3d_cc_bw_gamma_lin_still_Ov4689",
      "cc3d_cc_bw_gamma_lin_still_Ov4689",
    },
    { -1 },
};


IP_TABLE_PATH_s GVideoAdjTablePathOV4689[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Ov4689",
    "adj_video_default_01_Ov4689",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathOV4689[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Ov4689",
//    "adj_hiso_video_default_01",
};

IP_TABLE_PATH_s GPhotoAdjTablePathOV4689[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Ov4689",
    "adj_photo_default_01_Ov4689",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathOV4689[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Ov4689",
    "adj_still_default_01_Ov4689",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathOV4689[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Ov4689",
//    "adj_hiso_still_default_01_Ov4689",
};

IP_TABLE_PATH_s GSceneDataTablePathOV4689[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Ov4689",
    "scene_data_s02_Ov4689",
    "scene_data_s03_Ov4689",
    "scene_data_s04_Ov4689",
    "scene_data_s05_Ov4689",
};

IP_TABLE_PATH_s GImgAdjTablePathOV4689 ={
    "img_default_Ov4689"
};    

IP_TABLE_PATH_s GAaaAdjTablePathOV4689[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Ov4689",
    "aaa_default_01_Ov4689"
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathOV4689 ={
    "adj_still_idx_Ov4689"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathOV4689 ={
    "adj_video_idx_Ov4689"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathOV4689 ={
    "de_default_video_Ov4689"
};   

IP_TABLE_PATH_s GDeStillTablePathOV4689 ={
    "de_default_still_Ov4689"
};    
IP_TABLE_PATH_s GAdjTablePathOV4689 ={
    "adj_table_param_default_Ov4689"
};        

IP_TABLE_ADDR_s GImgAddrOV4689;
IP_TABLE_ADDR_s GAaaAddrOV4689;
IP_TABLE_ADDR_s GAdjTableOV4689;
IP_TABLE_ADDR_s GStillIdxInfoOV4689;
IP_TABLE_ADDR_s GVideoIdxInfoOV4689;
IP_TABLE_ADDR_s GDeVideoOV4689;
IP_TABLE_ADDR_s GDeStillOV4689;

#endif