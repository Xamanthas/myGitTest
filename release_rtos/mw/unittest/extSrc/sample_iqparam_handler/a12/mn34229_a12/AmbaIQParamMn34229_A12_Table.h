/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamMn34229_A12_Table.h
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Mn34229 Image IQ parameters tables
 *
 *  @History        ::
 *      Date        Name        Comments
 *      01/08/2013  Eddie Chen  Created
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef __AMBA_IQPARAM_MN34229_A12_TABLE_H__
#define __AMBA_IQPARAM_MN34229_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableMN34229[10] = {
    { 0,
      "VideoCc0_Mn34229",
      "VideoCc1_Mn34229",
      "VideoCc2_Mn34229",
      "VideoCc3_Mn34229",
      "VideoCc4_Mn34229",
      "StillCc0_Mn34229",
      "StillCc1_Mn34229",
      "StillCc2_Mn34229",
      "StillCc3_Mn34229",
      "StillCc4_Mn34229",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Mn34229",
      "cc3d_cc_bw_gamma_lin_video_Mn34229",
      "cc3d_cc_bw_gamma_lin_video_Mn34229",
      "cc3d_cc_bw_gamma_lin_video_Mn34229",
      "cc3d_cc_bw_gamma_lin_video_Mn34229",
      "cc3d_cc_bw_gamma_lin_still_Mn34229",
      "cc3d_cc_bw_gamma_lin_still_Mn34229",
      "cc3d_cc_bw_gamma_lin_still_Mn34229",
      "cc3d_cc_bw_gamma_lin_still_Mn34229",
      "cc3d_cc_bw_gamma_lin_still_Mn34229",
    },
    { -1 },
};

IP_TABLE_PATH_s GVideoAdjTablePathMN34229[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Mn34229",
    "adj_video_default_01_Mn34229",
};


IP_TABLE_PATH_s GVideoHIsoAdjTablePathMN34229[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Mn34229",
//    "adj_hiso_video_default_01_Mn34229",
};

IP_TABLE_PATH_s GPhotoAdjTablePathMN34229[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Mn34229",
    "adj_photo_default_01_Mn34229",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathMN34229[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Mn34229",
    "adj_still_default_01_Mn34229",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathMN34229[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Mn34229",
//    "adj_hiso_still_default_01_Mn34229",
};

IP_TABLE_PATH_s GSceneDataTablePathMN34229[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Mn34229",
    "scene_data_s02_Mn34229",
    "scene_data_s03_Mn34229",
    "scene_data_s04_Mn34229",
    "scene_data_s05_Mn34229",
};

IP_TABLE_PATH_s GImgAdjTablePathMN34229 ={
    "img_default_Mn34229"
};    

IP_TABLE_PATH_s GAaaAdjTablePathMN34229[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Mn34229",
    "aaa_default_00_Mn34121",    
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathMN34229 ={
    "adj_still_idx_Mn34229"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathMN34229 ={
    "adj_video_idx_Mn34229"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathMN34229 ={
    "de_default_video_Mn34229"
};   

IP_TABLE_PATH_s GDeStillTablePathMN34229 ={
    "de_default_still_Mn34229"
};    
IP_TABLE_PATH_s GAdjTablePathMN34229 ={
    "adj_table_param_default_Mn34229"
};   

#endif
