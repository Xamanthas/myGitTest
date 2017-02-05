/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamAr0230_A12_Table.h
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

#ifndef __AMBA_IQPARAM_AR0230_A12_TABLE_H__
#define __AMBA_IQPARAM_AR0230_A12_TABLE_H__

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"


COLOR_TABLE_PATH_s GCcTableAR0230[10] = {
    { 0,
      "VideoCc0_Ar0230",
      "VideoCc1_Ar0230",
      "VideoCc2_Ar0230",
      "VideoCc3_Ar0230",
      "VideoCc4_Ar0230",
      "StillCc0_Ar0230",
      "StillCc1_Ar0230",
      "StillCc2_Ar0230",
      "StillCc3_Ar0230",
      "StillCc4_Ar0230",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Ar0230",
      "cc3d_cc_bw_gamma_lin_video_Ar0230",
      "cc3d_cc_bw_gamma_lin_video_Ar0230",
      "cc3d_cc_bw_gamma_lin_video_Ar0230",
      "cc3d_cc_bw_gamma_lin_video_Ar0230",
      "cc3d_cc_bw_gamma_lin_still_Ar0230",
      "cc3d_cc_bw_gamma_lin_still_Ar0230",
      "cc3d_cc_bw_gamma_lin_still_Ar0230",
      "cc3d_cc_bw_gamma_lin_still_Ar0230",
      "cc3d_cc_bw_gamma_lin_still_Ar0230",
    },
    { -1 },
};


IP_TABLE_PATH_s GVideoAdjTablePathAR0230[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Ar0230",
    "adj_video_default_01_Ar0230",
};

IP_TABLE_PATH_s GVideoHIsoAdjTablePathAR0230[ADJ_VIDEO_HISO_TABLE_VALID_NO] = {
    "adj_hiso_video_default_00_Ar0230",
//    "adj_hiso_video_default_01",
};

IP_TABLE_PATH_s GPhotoAdjTablePathAR0230[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Ar0230",
    "adj_photo_default_01_Ar0230",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathAR0230[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Ar0230",
    "adj_still_default_01_Ar0230",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathAR0230[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Ar0230",
//    "adj_hiso_still_default_01_Ar0230",
};

IP_TABLE_PATH_s GSceneDataTablePathAR0230[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Ar0230",
    "scene_data_s02_Ar0230",
    "scene_data_s03_Ar0230",
    "scene_data_s04_Ar0230",
    "scene_data_s05_Ar0230",
};

IP_TABLE_PATH_s GImgAdjTablePathAR0230 ={
    "img_default_Ar0230"
};    

IP_TABLE_PATH_s GAaaAdjTablePathAR0230[AAA_TABLE_VALID_NO] ={
    "aaa_default_00_Ar0230",
    "aaa_default_01_Ar0230"
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathAR0230 ={
    "adj_still_idx_Ar0230"
};    

IP_TABLE_PATH_s GVideoIdxInfoAdjTablePathAR0230 ={
    "adj_video_idx_Ar0230"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathAR0230 ={
    "de_default_video_Ar0230"
};   

IP_TABLE_PATH_s GDeStillTablePathAR0230 ={
    "de_default_still_Ar0230"
};    
IP_TABLE_PATH_s GAdjTablePathAR0230 ={
    "adj_table_param_default_Ar0230"
};        

IP_TABLE_ADDR_s GImgAddrAR0230;
IP_TABLE_ADDR_s GAaaAddrAR0230;
IP_TABLE_ADDR_s GAdjTableAR0230;
IP_TABLE_ADDR_s GStillIdxInfoAR0230;
IP_TABLE_ADDR_s GVideoIdxInfoAR0230;
IP_TABLE_ADDR_s GDeVideoAR0230;
IP_TABLE_ADDR_s GDeStillAR0230;

#endif