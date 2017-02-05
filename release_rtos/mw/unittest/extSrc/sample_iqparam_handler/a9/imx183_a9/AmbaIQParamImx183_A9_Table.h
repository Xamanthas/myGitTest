/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamImx183_A9_Table.h
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Imx183 Image IQ parameters tables
 *
 *  @History        ::
 *      Date        Name        Comments
 *      01/08/2013  Eddie Chen  Created
\*-------------------------------------------------------------------------------------------------------------------*/
#ifdef CONFIG_SOC_A9
#ifndef __AMBA_IQPARAM_IMX183_A9_TABLE_H__
#define __AMBA_IQPARAM_IMX183_A9_TABLE_H__

#include "AmbaImg_Adjustment_A9.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"

extern IMG_PARAM_s AmbaIQParamImx183ImageParam;
extern AAA_PARAM_s AmbaIQParamImx183DefParams;
extern ADJ_TABLE_PARAM_s AmbaIQParamImx183AdjTableParam;
extern ADJ_VIDEO_PARAM_s AmbaIQParamImx183AdjVideoPc00;
extern ADJ_VIDEO_PARAM_s AmbaIQParamImx183AdjVideoPc01;0;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamImx183AdjStillLIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamImx183AdjStillLIso01;
extern ADJ_PHOTO_PARAM_s AmbaIQParamImx183AdjPhotoPreview00;
extern ADJ_PHOTO_PARAM_s AmbaIQParamImx183AdjPhotoPreview01;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamImx183AdjStillHIso00;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamImx183AdjStillHIso01;
extern ADJ_STILL_IDX_INFO_s AmbaIQParamImx183StillParam;
extern SCENE_DATA_s SceneDataS01Imx183A9[8];
extern SCENE_DATA_s SceneDataS02Imx183A9[8];
extern SCENE_DATA_s SceneDataS03Imx183A9[8];
extern SCENE_DATA_s SceneDataS04Imx183A9[8];
extern SCENE_DATA_s SceneDataS05Imx183A9[8];
extern DE_PARAM_s DeVideoParamImx183A9;
extern DE_PARAM_s DeStillParamImx183A9;
//extern CALIBRATION_PARAM_s AmbaIQParamImx183CalibParams;

COLOR_TABLE_PATH_s GCcTableIMX183[10] = {
    { 0,
      "VideoCc0_Imx183",
      "VideoCc1_Imx183",
      "VideoCc2_Imx183",
      "VideoCc3_Imx183",
      "VideoCc4_Imx183",
      "StillCc0_Imx183",
      "StillCc1_Imx183",
      "StillCc2_Imx183",
      "StillCc3_Imx183",
      "StillCc4_Imx183",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Imx183",
      "cc3d_cc_bw_gamma_lin_video_Imx183",
      "cc3d_cc_bw_gamma_lin_video_Imx183",
      "cc3d_cc_bw_gamma_lin_video_Imx183",
      "cc3d_cc_bw_gamma_lin_video_Imx183",
      "cc3d_cc_bw_gamma_lin_still_Imx183",
      "cc3d_cc_bw_gamma_lin_still_Imx183",
      "cc3d_cc_bw_gamma_lin_still_Imx183",
      "cc3d_cc_bw_gamma_lin_still_Imx183",
      "cc3d_cc_bw_gamma_lin_still_Imx183",
    },
    { -1 },
};

IP_TABLE_PATH_s GVideoAdjTablePathIMX183[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Imx183",
    "adj_video_default_01_Imx183",
};

IP_TABLE_PATH_s GPhotoAdjTablePathIMX183[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Imx183",
    "adj_photo_default_01_Imx183",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathIMX183[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Imx183",
    "adj_still_default_01_Imx183",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathIMX183[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Imx183",
//    "adj_hiso_still_default_01_Imx183",
};

IP_TABLE_PATH_s GSceneDataTablePathIMX183[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Imx183",
    "scene_data_s02_Imx183",
    "scene_data_s03_Imx183",
    "scene_data_s04_Imx183",
    "scene_data_s05_Imx183",
};

IP_TABLE_PATH_s GImgAdjTablePathIMX183 ={
    "img_default_Imx183"
};    

IP_TABLE_PATH_s GAaaAdjTablePathIMX183 ={
    "aaa_default_Imx183"
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathIMX183 ={
    "adj_still_idx_Imx183"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathIMX183 ={
    "de_default_video_Imx183"
};   

IP_TABLE_PATH_s GDeStillTablePathIMX183 ={
    "de_default_still_Imx183"
};    
IP_TABLE_PATH_s GAdjTablePathIMX183 ={
    "adj_table_param_default_Imx183"
};   

#endif
#endif
