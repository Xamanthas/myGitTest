/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamImx144_A9_Table.h
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
#ifdef CONFIG_SOC_A9
#ifndef __AMBA_IQPARAM_IMX144_A9_TABLE_H__
#define __AMBA_IQPARAM_IMX144_A9_TABLE_H__

#include "AmbaImg_Adjustment_A9.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_Def.h"

extern IMG_PARAM_s AmbaIQParamImx144ImageParam;
extern AAA_PARAM_s AmbaIQParamImx144DefParams;
extern ADJ_TABLE_PARAM_s AmbaIQParamImx144AdjTableParam;
extern ADJ_VIDEO_PARAM_s AmbaIQParamImx144AdjVideoPc00;
extern ADJ_VIDEO_PARAM_s AmbaIQParamImx144AdjVideoPc01;0;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamImx144AdjStillLIso00;
extern ADJ_STILL_FAST_LISO_PARAM_S AmbaIQParamImx144AdjStillLIso01;
extern ADJ_PHOTO_PARAM_s AmbaIQParamImx144AdjPhotoPreview00;
extern ADJ_PHOTO_PARAM_s AmbaIQParamImx144AdjPhotoPreview01;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamImx144AdjStillHIso00;
extern ADJ_STILL_HISO_PARAM_s AmbaIQParamImx144AdjStillHIso01;
extern ADJ_STILL_IDX_INFO_s AmbaIQParamImx144StillParam;
extern SCENE_DATA_s SceneDataS01Imx144A9[8];
extern SCENE_DATA_s SceneDataS02Imx144A9[8];
extern SCENE_DATA_s SceneDataS03Imx144A9[8];
extern SCENE_DATA_s SceneDataS04Imx144A9[8];
extern SCENE_DATA_s SceneDataS05Imx144A9[8];
extern DE_PARAM_s DeVideoParamImx144A9;
extern DE_PARAM_s DeStillParamImx144A9;
//extern CALIBRATION_PARAM_s AmbaIQParamImx144CalibParams;

COLOR_TABLE_PATH_s GCcTableIMX144[10] = {
    { 0,
      "VideoCc0_Imx144",
      "VideoCc1_Imx144",
      "VideoCc2_Imx144",
      "VideoCc3_Imx144",
      "VideoCc4_Imx144",
      "StillCc0_Imx144",
      "StillCc1_Imx144",
      "StillCc2_Imx144",
      "StillCc3_Imx144",
      "StillCc4_Imx144",
    },
    { 1,
      "cc3d_cc_bw_gamma_lin_video_Imx144",
      "cc3d_cc_bw_gamma_lin_video_Imx144",
      "cc3d_cc_bw_gamma_lin_video_Imx144",
      "cc3d_cc_bw_gamma_lin_video_Imx144",
      "cc3d_cc_bw_gamma_lin_video_Imx144",
      "cc3d_cc_bw_gamma_lin_still_Imx144",
      "cc3d_cc_bw_gamma_lin_still_Imx144",
      "cc3d_cc_bw_gamma_lin_still_Imx144",
      "cc3d_cc_bw_gamma_lin_still_Imx144",
      "cc3d_cc_bw_gamma_lin_still_Imx144",
    },
    { -1 },
};

IP_TABLE_PATH_s GVideoAdjTablePathIMX144[ADJ_VIDEO_TABLE_VALID_NO] = {
    "adj_video_default_00_Imx144",
    "adj_video_default_01_Imx144",
};

IP_TABLE_PATH_s GPhotoAdjTablePathIMX144[ADJ_PHOTO_TABLE_VALID_NO] = {
    "adj_photo_default_00_Imx144",
    "adj_photo_default_01_Imx144",
};

IP_TABLE_PATH_s GStillLIsoAdjTablePathIMX144[ADJ_STILL_LISO_TABLE_VALID_NO] = {
    "adj_still_default_00_Imx144",
    "adj_still_default_01_Imx144",
};

IP_TABLE_PATH_s GStillHIsoAdjTablePathIMX144[ADJ_STILL_HISO_TABLE_VALID_NO] = {
    "adj_hiso_still_default_00_Imx144",
//    "adj_hiso_still_default_01_Imx144",
};

IP_TABLE_PATH_s GSceneDataTablePathIMX144[SCENE_TABLE_VALID_NO] = {
    "scene_data_s01_Imx144",
    "scene_data_s02_Imx144",
    "scene_data_s03_Imx144",
    "scene_data_s04_Imx144",
    "scene_data_s05_Imx144",
};

IP_TABLE_PATH_s GImgAdjTablePathIMX144 ={
    "img_default_Imx144"
};    

IP_TABLE_PATH_s GAaaAdjTablePathIMX144 ={
    "aaa_default_Imx144"
};    

IP_TABLE_PATH_s GStillIdxInfoAdjTablePathIMX144 ={
    "adj_still_idx_Imx144"
};    
    
IP_TABLE_PATH_s GDeVideoTablePathIMX144 ={
    "de_default_video_Imx144"
};   

IP_TABLE_PATH_s GDeStillTablePathIMX144 ={
    "de_default_still_Imx144"
};    
IP_TABLE_PATH_s GAdjTablePathIMX144 ={
    "adj_table_param_default_Imx144"
};   

#endif
#endif
