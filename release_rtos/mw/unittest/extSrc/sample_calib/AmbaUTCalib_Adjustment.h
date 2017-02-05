/**
 * @file mw/unittest/extSrc/sample_calib/AmbaUTCalib_Adjustment.h
 *
 * header file for adjust calibration parameters
 *
 * History:
 *    02/16/2015  Allen Chiu Created
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "AmbaDSP_ImgFilter.h"

/**
* @defgroup ApplibCalib_Adjustment
* @brief Header file for Calibration control
*
*/

/**
 * @addtogroup ApplibCalib_Adjustment
 * @{
 */

#define CALIB_PARAM_VER     (0x14031400)
#define LV_TABLE_NUM        (21)
#define WB_GAIN_TAB_NUM     (9)

/**
 * Vignette Luma Strength table
 */
typedef struct _VIG_LUMA_TABLE_s_ {
	UINT16 VideoWeightTable[LV_TABLE_NUM];  /**< luma strength weightting for video */
	UINT16 StillWeightTable[LV_TABLE_NUM];  /**< luma strength weightting for still */
} VIG_LUMA_TABLE_s;

/**
 * Vignette WB Strength table
 */
typedef struct _VIG_WB_GAIN_TABLE_s_ {
	AMBA_DSP_IMG_WB_GAIN_s WbGain;  /**< WB weightting */
	int WbIdx;                      /**< WB index */
} VIG_WB_GAIN_TABLE_s;

/**
 * Vignette WB blend table
 */
typedef struct _WB_BLEND_CURVE_s_ {
	VIG_WB_GAIN_TABLE_s     *Start; /**< start of blend region */
	VIG_WB_GAIN_TABLE_s     *End;   /**< end of blend region */
	UINT16                  VideoWeightTab[WB_GAIN_TAB_NUM]; /**< video weighting  */
	UINT16                  StillWeightTab[WB_GAIN_TAB_NUM]; /**< still weighting */
} WB_BLEND_CURVE_s;

/**
 * calibration control parameters
 */
typedef struct _CALIBRATION_ADJUST_PARAM_s_ {
    UINT32                   FWVersionNumber;       /**< FW version number */
    UINT32                   ParamVersionNum;       /**< parameter version number */
    UINT32                   VigLumaStrCtlEnable;   /**< vignette luma control flag */
    UINT32                   VigChromaStrCtlEnable; /**< vignette chroma control flag */    
    UINT32                   VigBlendOzoomEnable;   /**< vignette OZ blend control flag */
    UINT32                   VigBlendWBEnable;      /**< vignette WB blend control flag */
    UINT32                   CheckFrameNum;         /**< frame period to control vignette luma strength */
    UINT32                   AverageNum;            /**< frame NO. to average the vignette strength (1*new_gain+(AverageNum-1)*old)/AverageNum */
    UINT32                   LumaThreshold;         /**< LV Threshold to change the luma strength*/
    UINT32                   ChromaThreshold;     /**< LV Threshold to change the chroma strength*/
    VIG_LUMA_TABLE_s         VigLumaTable[4];       /**< vignette luma table */
    VIG_LUMA_TABLE_s         VigChromaTable[4];     /**< vignette chroma table */    
    WB_BLEND_CURVE_s         VigWbBlendTable[4];    /**< vignette WB table */
} CALIBRATION_ADJUST_PARAM_s;

/**
 * @}
 */


