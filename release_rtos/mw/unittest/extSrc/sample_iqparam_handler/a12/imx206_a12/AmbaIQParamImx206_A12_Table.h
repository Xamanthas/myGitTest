/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamImx206_A12_Table.h
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
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef __AMBA_IQPARAM_IMX206_A12_TABLE_H__
#define __AMBA_IQPARAM_IMX206_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableIMX206[10] = {
    { 0,
      "VideoCc0_Imx206",
      "VideoCc1_Imx206",
      "VideoCc2_Imx206",
      "VideoCc3_Imx206",
      "VideoCc4_Imx206",
      "StillCc0_Imx206",
      "StillCc1_Imx206",
      "StillCc2_Imx206",
      "StillCc3_Imx206",
      "StillCc4_Imx206",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Imx206",
      "cc3d_cc_bw_gamma_lin_video_Imx206",
      "cc3d_cc_bw_gamma_lin_video_Imx206",
      "cc3d_cc_bw_gamma_lin_video_Imx206",
      "cc3d_cc_bw_gamma_lin_video_Imx206",
      "cc3d_cc_bw_gamma_lin_still_Imx206",
      "cc3d_cc_bw_gamma_lin_still_Imx206",
      "cc3d_cc_bw_gamma_lin_still_Imx206",
      "cc3d_cc_bw_gamma_lin_still_Imx206",
      "cc3d_cc_bw_gamma_lin_still_Imx206",
    },
    { -1 },
};

IP_TABLE_PATH_s GVideoAdjTablePathIMX206[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Imx206",
    "adj_video_default_01_Imx206",
};


IP_TABLE_PATH_s GVideoHIsoAdjTablePathIMX206[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Imx206",
//    "adj_hiso_video_default_01_Imx206",
};

IP_TABLE_PATH_s GPhotoAdjTablePathIMX206[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Imx206",
    "adj_photo_default_01_Imx206",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathIMX206[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Imx206",
    "adj_still_default_01_Imx206",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathIMX206[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Imx206",
//    "adj_hiso_still_default_01_Imx206",
};

IP_TABLE_PATH_s GSceneDataTablePathIMX206[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Imx206",
    "scene_data_s02_Imx206",
    "scene_data_s03_Imx206",
    "scene_data_s04_Imx206",
    "scene_data_s05_Imx206",
};

IP_TABLE_PATH_s GImgAdjTablePathIMX206 ={
    "img_default_Imx206"
};    

IP_TABLE_PATH_s GAaaAdjTablePathIMX206[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Imx206",
    "aaa_default_01_Imx206"    
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathIMX206 ={
    "adj_still_idx_Imx206"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathIMX206 ={
    "adj_video_idx_Imx206"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathIMX206 ={
    "de_default_video_Imx206"
};   

IP_TABLE_PATH_s GDeStillTablePathIMX206 ={
    "de_default_still_Imx206"
};    
IP_TABLE_PATH_s GAdjTablePathIMX206 ={
    "adj_table_param_default_Imx206"
};   

#endif
