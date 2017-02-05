/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamMn34222_A12_Table.h
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Mn34222 Image IQ parameters tables
 *
 *  @History        ::
 *      Date        Name        Comments
 *      01/08/2013  Eddie Chen  Created
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef __AMBA_IQPARAM_MN34222_A12_TABLE_H__
#define __AMBA_IQPARAM_MN34222_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableMN34222[10] = {
    { 0,
      "VideoCc0_Mn34222",
      "VideoCc1_Mn34222",
      "VideoCc2_Mn34222",
      "VideoCc3_Mn34222",
      "VideoCc4_Mn34222",
      "StillCc0_Mn34222",
      "StillCc1_Mn34222",
      "StillCc2_Mn34222",
      "StillCc3_Mn34222",
      "StillCc4_Mn34222",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Mn34222",
      "cc3d_cc_bw_gamma_lin_video_Mn34222",
      "cc3d_cc_bw_gamma_lin_video_Mn34222",
      "cc3d_cc_bw_gamma_lin_video_Mn34222",
      "cc3d_cc_bw_gamma_lin_video_Mn34222",
      "cc3d_cc_bw_gamma_lin_still_Mn34222",
      "cc3d_cc_bw_gamma_lin_still_Mn34222",
      "cc3d_cc_bw_gamma_lin_still_Mn34222",
      "cc3d_cc_bw_gamma_lin_still_Mn34222",
      "cc3d_cc_bw_gamma_lin_still_Mn34222",
    },
    { -1 },
};

IP_TABLE_PATH_s GVideoAdjTablePathMN34222[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Mn34222",
    "adj_video_default_01_Mn34222",
};


IP_TABLE_PATH_s GVideoHIsoAdjTablePathMN34222[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Mn34222",
//    "adj_hiso_video_default_01_Mn34222",
};

IP_TABLE_PATH_s GPhotoAdjTablePathMN34222[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Mn34222",
    "adj_photo_default_01_Mn34222",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathMN34222[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Mn34222",
    "adj_still_default_01_Mn34222",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathMN34222[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Mn34222",
//    "adj_hiso_still_default_01_Mn34222",
};

IP_TABLE_PATH_s GSceneDataTablePathMN34222[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Mn34222",
    "scene_data_s02_Mn34222",
    "scene_data_s03_Mn34222",
    "scene_data_s04_Mn34222",
    "scene_data_s05_Mn34222",
};

IP_TABLE_PATH_s GImgAdjTablePathMN34222 ={
    "img_default_Mn34222"
};    

IP_TABLE_PATH_s GAaaAdjTablePathMN34222[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Mn34222",
    "aaa_default_01_Mn34222",    
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathMN34222 ={
    "adj_still_idx_Mn34222"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathMN34222 ={
    "adj_video_idx_Mn34222"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathMN34222 ={
    "de_default_video_Mn34222"
};   

IP_TABLE_PATH_s GDeStillTablePathMN34222 ={
    "de_default_still_Mn34222"
};    
IP_TABLE_PATH_s GAdjTablePathMN34222 ={
    "adj_table_param_default_Mn34222"
};   

#endif
