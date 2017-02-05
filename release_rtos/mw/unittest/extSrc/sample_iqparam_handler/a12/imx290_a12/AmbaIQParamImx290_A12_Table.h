/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamImx290_A12_Table.h
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

#ifndef __AMBA_IQPARAM_IMX290_A12_TABLE_H__
#define __AMBA_IQPARAM_IMX290_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableIMX290[10] = {
    { 0,
      "VideoCc0_Imx290",
      "VideoCc1_Imx290",
      "VideoCc2_Imx290",
      "VideoCc3_Imx290",
      "VideoCc4_Imx290",
      "StillCc0_Imx290",
      "StillCc1_Imx290",
      "StillCc2_Imx290",
      "StillCc3_Imx290",
      "StillCc4_Imx290",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Imx290",
      "cc3d_cc_bw_gamma_lin_video_Imx290",
      "cc3d_cc_bw_gamma_lin_video_Imx290",
      "cc3d_cc_bw_gamma_lin_video_Imx290",
      "cc3d_cc_bw_gamma_lin_video_Imx290",
      "cc3d_cc_bw_gamma_lin_still_Imx290",
      "cc3d_cc_bw_gamma_lin_still_Imx290",
      "cc3d_cc_bw_gamma_lin_still_Imx290",
      "cc3d_cc_bw_gamma_lin_still_Imx290",
      "cc3d_cc_bw_gamma_lin_still_Imx290",
    },
    { -1 },
};

IP_TABLE_PATH_s GVideoAdjTablePathIMX290[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Imx290",
    "adj_video_default_01_Imx290",
};


IP_TABLE_PATH_s GVideoHIsoAdjTablePathIMX290[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Imx290",
//    "adj_hiso_video_default_01_Imx290",
};

IP_TABLE_PATH_s GPhotoAdjTablePathIMX290[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Imx290",
    "adj_photo_default_01_Imx290",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathIMX290[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Imx290",
    "adj_still_default_01_Imx290",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathIMX290[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Imx290",
//    "adj_hiso_still_default_01_Imx290",
};

IP_TABLE_PATH_s GSceneDataTablePathIMX290[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Imx290",
    "scene_data_s02_Imx290",
    "scene_data_s03_Imx290",
    "scene_data_s04_Imx290",
    "scene_data_s05_Imx290",
};

IP_TABLE_PATH_s GImgAdjTablePathIMX290 ={
    "img_default_Imx290"
};    

IP_TABLE_PATH_s GAaaAdjTablePathIMX290[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Imx290",
    "aaa_default_01_Imx290"    
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathIMX290 ={
    "adj_still_idx_Imx290"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathIMX290 ={
    "adj_video_idx_Imx290"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathIMX290 ={
    "de_default_video_Imx290"
};   

IP_TABLE_PATH_s GDeStillTablePathIMX290 ={
    "de_default_still_Imx290"
};    
IP_TABLE_PATH_s GAdjTablePathIMX290 ={
    "adj_table_param_default_Imx290"
};   

#endif
