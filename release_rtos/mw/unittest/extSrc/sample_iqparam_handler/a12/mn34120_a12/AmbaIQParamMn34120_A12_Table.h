/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamMn34120_A12_Table.h
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Mn34120 Image IQ parameters tables
 *
 *  @History        ::
 *      Date        Name        Comments
 *      01/08/2013  Eddie Chen  Created
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef __AMBA_IQPARAM_MN34120_A12_TABLE_H__
#define __AMBA_IQPARAM_MN34120_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableMN34120[10] = {
    { 0,
      "VideoCc0_Mn34120",
      "VideoCc1_Mn34120",
      "VideoCc2_Mn34120",
      "VideoCc3_Mn34120",
      "VideoCc4_Mn34120",
      "StillCc0_Mn34120",
      "StillCc1_Mn34120",
      "StillCc2_Mn34120",
      "StillCc3_Mn34120",
      "StillCc4_Mn34120",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Mn34120",
      "cc3d_cc_bw_gamma_lin_video_Mn34120",
      "cc3d_cc_bw_gamma_lin_video_Mn34120",
      "cc3d_cc_bw_gamma_lin_video_Mn34120",
      "cc3d_cc_bw_gamma_lin_video_Mn34120",
      "cc3d_cc_bw_gamma_lin_still_Mn34120",
      "cc3d_cc_bw_gamma_lin_still_Mn34120",
      "cc3d_cc_bw_gamma_lin_still_Mn34120",
      "cc3d_cc_bw_gamma_lin_still_Mn34120",
      "cc3d_cc_bw_gamma_lin_still_Mn34120",
    },
    { -1 },
};

IP_TABLE_PATH_s GVideoAdjTablePathMN34120[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Mn34120",
    "adj_video_default_01_Mn34120",
};


IP_TABLE_PATH_s GVideoHIsoAdjTablePathMN34120[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Mn34120",
//    "adj_hiso_video_default_01_Mn34120",
};

IP_TABLE_PATH_s GPhotoAdjTablePathMN34120[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Mn34120",
    "adj_photo_default_01_Mn34120",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathMN34120[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Mn34120",
    "adj_still_default_01_Mn34120",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathMN34120[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Mn34120",
//    "adj_hiso_still_default_01_Mn34120",
};

IP_TABLE_PATH_s GSceneDataTablePathMN34120[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Mn34120",
    "scene_data_s02_Mn34120",
    "scene_data_s03_Mn34120",
    "scene_data_s04_Mn34120",
    "scene_data_s05_Mn34120",
};

IP_TABLE_PATH_s GImgAdjTablePathMN34120 ={
    "img_default_Mn34120"
};    

IP_TABLE_PATH_s GAaaAdjTablePathMN34120[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Mn34120",
    "aaa_default_00_Mn34121",    
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathMN34120 ={
    "adj_still_idx_Mn34120"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathMN34120 ={
    "adj_video_idx_Mn34120"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathMN34120 ={
    "de_default_video_Mn34120"
};   

IP_TABLE_PATH_s GDeStillTablePathMN34120 ={
    "de_default_still_Mn34120"
};    
IP_TABLE_PATH_s GAdjTablePathMN34120 ={
    "adj_table_param_default_Mn34120"
};   

#endif
