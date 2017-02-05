/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamOv9750_A12_Table.h
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

#ifndef __AMBA_IQPARAM_OV9750_A12_TABLE_H__
#define __AMBA_IQPARAM_OV9750_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableOV9750[10] = {
    { 0,
      "VideoCc0_Ov9750",
      "VideoCc1_Ov9750",
      "VideoCc2_Ov9750",
      "VideoCc3_Ov9750",
      "VideoCc4_Ov9750",
      "StillCc0_Ov9750",
      "StillCc1_Ov9750",
      "StillCc2_Ov9750",
      "StillCc3_Ov9750",
      "StillCc4_Ov9750",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Ov9750",
      "cc3d_cc_bw_gamma_lin_video_Ov9750",
      "cc3d_cc_bw_gamma_lin_video_Ov9750",
      "cc3d_cc_bw_gamma_lin_video_Ov9750",
      "cc3d_cc_bw_gamma_lin_video_Ov9750",
      "cc3d_cc_bw_gamma_lin_still_Ov9750",
      "cc3d_cc_bw_gamma_lin_still_Ov9750",
      "cc3d_cc_bw_gamma_lin_still_Ov9750",
      "cc3d_cc_bw_gamma_lin_still_Ov9750",
      "cc3d_cc_bw_gamma_lin_still_Ov9750",
    },
    { -1 },
};


IP_TABLE_PATH_s GVideoAdjTablePathOV9750[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Ov9750",
    "adj_video_default_01_Ov9750",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathOV9750[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Ov9750",
//    "adj_hiso_video_default_01",
};

IP_TABLE_PATH_s GPhotoAdjTablePathOV9750[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Ov9750",
    "adj_photo_default_01_Ov9750",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathOV9750[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Ov9750",
    "adj_still_default_01_Ov9750",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathOV9750[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Ov9750",
//    "adj_hiso_still_default_01_Ov9750",
};

IP_TABLE_PATH_s GSceneDataTablePathOV9750[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Ov9750",
    "scene_data_s02_Ov9750",
    "scene_data_s03_Ov9750",
    "scene_data_s04_Ov9750",
    "scene_data_s05_Ov9750",
};

IP_TABLE_PATH_s GImgAdjTablePathOV9750 ={
    "img_default_Ov9750"
};    

IP_TABLE_PATH_s GAaaAdjTablePathOV9750[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Ov9750",
    "aaa_default_01_Ov9750"
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathOV9750 ={
    "adj_still_idx_Ov9750"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathOV9750 ={
    "adj_video_idx_Ov9750"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathOV9750 ={
    "de_default_video_Ov9750"
};   

IP_TABLE_PATH_s GDeStillTablePathOV9750 ={
    "de_default_still_Ov9750"
};    
IP_TABLE_PATH_s GAdjTablePathOV9750 ={
    "adj_table_param_default_Ov9750"
};        

IP_TABLE_ADDR_s GImgAddrOV9750;
IP_TABLE_ADDR_s GAaaAddrOV9750;
IP_TABLE_ADDR_s GAdjTableOV9750;
IP_TABLE_ADDR_s GStillIdxInfoOV9750;
IP_TABLE_ADDR_s GVideoIdxInfoOV9750;
IP_TABLE_ADDR_s GDeVideoOV9750;
IP_TABLE_ADDR_s GDeStillOV9750;

#endif