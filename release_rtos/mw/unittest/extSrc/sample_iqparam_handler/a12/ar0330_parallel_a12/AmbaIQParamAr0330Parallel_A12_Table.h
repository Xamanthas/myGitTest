/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamAr0330Parallel_A12_Table.h
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

#ifndef __AMBA_IQPARAM_AR0330PARALLEL_A12_TABLE_H__
#define __AMBA_IQPARAM_AR0330PARALLEL_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableAR0330Parallel[10] = {
    { 0,
      "VideoCc0_Ar0330Parallel",
      "VideoCc1_Ar0330Parallel",
      "VideoCc2_Ar0330Parallel",
      "VideoCc3_Ar0330Parallel",
      "VideoCc4_Ar0330Parallel",
      "StillCc0_Ar0330Parallel",
      "StillCc1_Ar0330Parallel",
      "StillCc2_Ar0330Parallel",
      "StillCc3_Ar0330Parallel",
      "StillCc4_Ar0330Parallel",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Ar0330Parallel",
      "cc3d_cc_bw_gamma_lin_video_Ar0330Parallel",
      "cc3d_cc_bw_gamma_lin_video_Ar0330Parallel",
      "cc3d_cc_bw_gamma_lin_video_Ar0330Parallel",
      "cc3d_cc_bw_gamma_lin_video_Ar0330Parallel",
      "cc3d_cc_bw_gamma_lin_still_Ar0330Parallel",
      "cc3d_cc_bw_gamma_lin_still_Ar0330Parallel",
      "cc3d_cc_bw_gamma_lin_still_Ar0330Parallel",
      "cc3d_cc_bw_gamma_lin_still_Ar0330Parallel",
      "cc3d_cc_bw_gamma_lin_still_Ar0330Parallel",
    },
    { -1 },
};


IP_TABLE_PATH_s GVideoAdjTablePathAR0330Parallel[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Ar0330Parallel",
    "adj_video_default_01_Ar0330Parallel",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathAR0330Parallel[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Ar0330Parallel",
//    "adj_hiso_video_default_01",
};

IP_TABLE_PATH_s GPhotoAdjTablePathAR0330Parallel[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Ar0330Parallel",
    "adj_photo_default_01_Ar0330Parallel",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathAR0330Parallel[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Ar0330Parallel",
    "adj_still_default_01_Ar0330Parallel",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathAR0330Parallel[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Ar0330Parallel",
//    "adj_hiso_still_default_01_Ar0330Parallel",
};

IP_TABLE_PATH_s GSceneDataTablePathAR0330Parallel[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Ar0330Parallel",
    "scene_data_s02_Ar0330Parallel",
    "scene_data_s03_Ar0330Parallel",
    "scene_data_s04_Ar0330Parallel",
    "scene_data_s05_Ar0330Parallel",
};

IP_TABLE_PATH_s GImgAdjTablePathAR0330Parallel ={
    "img_default_Ar0330Parallel"
};    

IP_TABLE_PATH_s GAaaAdjTablePathAR0330Parallel[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Ar0330Parallel",
    "aaa_default_01_Ar0330Parallel"    
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathAR0330Parallel ={
    "adj_still_idx_Ar0330Parallel"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathAR0330Parallel ={
    "adj_video_idx_Ar0330Parallel"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathAR0330Parallel ={
    "de_default_video_Ar0330Parallel"
};   

IP_TABLE_PATH_s GDeStillTablePathAR0330Parallel ={
    "de_default_still_Ar0330Parallel"
};    
IP_TABLE_PATH_s GAdjTablePathAR0330Parallel ={
    "adj_table_param_default_Ar0330Parallel"
};        

IP_TABLE_ADDR_s GImgAddrAR0330Parallel;
IP_TABLE_ADDR_s GAaaAddrAR0330Parallel;
IP_TABLE_ADDR_s GAdjTableAR0330Parallel;
IP_TABLE_ADDR_s GStillIdxInfoAR0330Parallel;
IP_TABLE_ADDR_s GVideoIdxInfoAR0330Parallel;
IP_TABLE_ADDR_s GDeVideoAR0330Parallel;
IP_TABLE_ADDR_s GDeStillAR0330Parallel;

#endif