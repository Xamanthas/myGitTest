/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamAr1335_A12_Table.h
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

#ifndef __AMBA_IQPARAM_AR1335_A12_TABLE_H__
#define __AMBA_IQPARAM_AR1335_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableAR1335[10] = {
    { 0,
      "VideoCc0_Ar1335",
      "VideoCc1_Ar1335",
      "VideoCc2_Ar1335",
      "VideoCc3_Ar1335",
      "VideoCc4_Ar1335",
      "StillCc0_Ar1335",
      "StillCc1_Ar1335",
      "StillCc2_Ar1335",
      "StillCc3_Ar1335",
      "StillCc4_Ar1335",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Ar1335",
      "cc3d_cc_bw_gamma_lin_video_Ar1335",
      "cc3d_cc_bw_gamma_lin_video_Ar1335",
      "cc3d_cc_bw_gamma_lin_video_Ar1335",
      "cc3d_cc_bw_gamma_lin_video_Ar1335",
      "cc3d_cc_bw_gamma_lin_still_Ar1335",
      "cc3d_cc_bw_gamma_lin_still_Ar1335",
      "cc3d_cc_bw_gamma_lin_still_Ar1335",
      "cc3d_cc_bw_gamma_lin_still_Ar1335",
      "cc3d_cc_bw_gamma_lin_still_Ar1335",
    },
    { -1 },
};


IP_TABLE_PATH_s GVideoAdjTablePathAR1335[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Ar1335",
    "adj_video_default_01_Ar1335",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathAR1335[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Ar1335",
//    "adj_hiso_video_default_01",
};

IP_TABLE_PATH_s GPhotoAdjTablePathAR1335[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Ar1335",
    "adj_photo_default_01_Ar1335",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathAR1335[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Ar1335",
    "adj_still_default_01_Ar1335",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathAR1335[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Ar1335",
//    "adj_hiso_still_default_01_Ar1335",
};

IP_TABLE_PATH_s GSceneDataTablePathAR1335[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Ar1335",
    "scene_data_s02_Ar1335",
    "scene_data_s03_Ar1335",
    "scene_data_s04_Ar1335",
    "scene_data_s05_Ar1335",
};

IP_TABLE_PATH_s GImgAdjTablePathAR1335 ={
    "img_default_Ar1335"
};    

IP_TABLE_PATH_s GAaaAdjTablePathAR1335[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Ar1335",
    "aaa_default_01_Ar1335"
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathAR1335 ={
    "adj_still_idx_Ar1335"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathAR1335 ={
    "adj_video_idx_Ar1335"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathAR1335 ={
    "de_default_video_Ar1335"
};   

IP_TABLE_PATH_s GDeStillTablePathAR1335 ={
    "de_default_still_Ar1335"
};    
IP_TABLE_PATH_s GAdjTablePathAR1335 ={
    "adj_table_param_default_Ar1335"
};        

IP_TABLE_ADDR_s GImgAddrAR1335;
IP_TABLE_ADDR_s GAaaAddrAR1335;
IP_TABLE_ADDR_s GAdjTableAR1335;
IP_TABLE_ADDR_s GStillIdxInfoAR1335;
IP_TABLE_ADDR_s GVideoIdxInfoAR1335;
IP_TABLE_ADDR_s GDeVideoAR1335;
IP_TABLE_ADDR_s GDeStillAR1335;

#endif