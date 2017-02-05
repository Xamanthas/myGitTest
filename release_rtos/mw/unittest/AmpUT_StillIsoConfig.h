 /**
  * @file src/app/sample/unittest/AmpUT_StillIsoConfig.h
  *
  * still ISO config setting header
  *
  * History:
  *    2013/07/05 - [Edgar Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */


#ifndef _AMPUT_STILL_ISO_CONFIG_H_
#define _AMPUT_STILL_ISO_CONFIG_H_

#include "AmbaDSP_ImgFilter.h"
#include "AmbaDSP_Img3aStatistics.h"
#include "AmbaDSP_ImgUtility.h"

/* The follow structure is for low iso configure */
typedef struct _STILL_ISO_CONFIG_s_ {
    struct {
        UINT32  Enable;
        AMBA_DSP_IMG_AE_STAT_INFO_s Data;
    } AeStatInfo;

    struct {
        UINT32  Enable;
        AMBA_DSP_IMG_AF_STAT_INFO_s Data;
    } AfStatInfo;

    struct {
        UINT32  Enable;
        AMBA_DSP_IMG_AWB_STAT_INFO_s Data;
    } AwbStatInfo;

    struct {
        UINT32  Enable;
        AMBA_DSP_IMG_SENSOR_INFO_s Data;
    } SensorInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SBP_CORRECTION_s Data;
    } SbpCorr;  /* Statistic Bad Pixel Correction */

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_VIGNETTE_CALC_INFO_s Data;
    } VignetteCalcInfo;

    struct {
        UINT32  Enable;
        AMBA_DSP_IMG_WARP_CALC_INFO_s Data;
    } WarpCalcInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CAWARP_CALC_INFO_s Data;
    } CACalInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_BLACK_CORRECTION_s Data;
    } StaticBlackLevelInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DBP_CORRECTION_s Data;
    } BadPixelCorrectionInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s Data;
    } CfaLeakageFilterInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CFA_NOISE_FILTER_s Data;
    } CfaNoiseFilterInfo;

    struct {
        UINT32 Enable;
    #ifdef CONFIG_SOC_A9
        AMBA_DSP_IMG_ANTI_ALIASING_FILTER_s  Data;
    #else
        AMBA_DSP_IMG_ANTI_ALIASING_s  Data;
    #endif
    } AntiAliasingInfo;

    struct {
        UINT32 Enable;
        UINT32 GlobalGain;
        AMBA_DSP_IMG_WB_GAIN_s Data;
    } WbGainInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DGAIN_SATURATION_s Data;
    } DgainSaturationLevelInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LOCAL_EXPOSURE_s Data;
    } LocalExposureInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_COLOR_CORRECTION_REG_s Data;
    } ColorCorrectionRegInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_COLOR_CORRECTION_s Data;
    } ColorCorrectionInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_TONE_CURVE_s Data;
    } ToneCurveInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_RGB_TO_YUV_s Data;
    } Rgb2YuvInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_SCALE_s Data;
    } ChromaScaleInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s Data;
    } ChromaMedianFilterInfor;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DEMOSAIC_s Data;
    } DemosaicInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_GBGR_MISMATCH_s Data;
    } GbGrMismatch;

#ifdef CONFIG_SOC_A9
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHP_A_SELECT_e Data;
    } SharpenAOrSpatialFilterInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CDNR_INFO_s Data;
    } CdnrInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_ASF_INFO_s Data;
    } AsfInfo;
#endif

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_FILTER_s Data;
    } ChromaFilterInfo;

#ifdef CONFIG_SOC_A9
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_BOTH_s Data;
    } SharpenBoth;
#endif

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_NOISE_s Data;
    } SharpenNoise;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_FIR_s Data;
    } SharpenBFir;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CORING_s Data;
    } SharpenBCoringInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } SharpBLevelOverallInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } SharpBLevelMinInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } SharpenBScaleCoring;

} STILL_ISO_CONFIG_s;

/* The follow structure is for High iso normal filter configure only */
typedef struct _STILL_HISO_NORMAL_CONFIG_s_ {
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_BLACK_CORRECTION_s Data;
    } StaticBlackLevelInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DBP_CORRECTION_s Data;
    } BadPixelCorrectionInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s Data;
    } CfaLeakageFilterInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CFA_NOISE_FILTER_s Data;
    } CfaNoiseFilterInfo;

    struct {
        UINT32 Enable;
    #ifdef CONFIG_SOC_A9
        AMBA_DSP_IMG_ANTI_ALIASING_FILTER_s  Data;
    #else
        AMBA_DSP_IMG_ANTI_ALIASING_s  Data;
    #endif
    } AntiAliasingInfo;

    struct {
        UINT32 Enable;
        UINT32 GlobalGain;
        AMBA_DSP_IMG_WB_GAIN_s Data;
    } WbGainInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DGAIN_SATURATION_s Data;
    } DgainSaturationLevelInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LOCAL_EXPOSURE_s Data;
    } LocalExposureInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_COLOR_CORRECTION_REG_s Data;
    } ColorCorrectionRegInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_COLOR_CORRECTION_s Data;
    } ColorCorrectionInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_TONE_CURVE_s Data;
    } ToneCurveInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_RGB_TO_YUV_s Data;
    } Rgb2YuvInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_SCALE_s Data;
    } ChromaScaleInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s Data;
    } ChromaMedianFilterInfor;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DEMOSAIC_s Data;
    } DemosaicInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_GBGR_MISMATCH_s Data;
    } GbGrMismatch;

#ifdef CONFIG_SOC_A9
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHP_A_SELECT_e Data;
    } SharpenAOrSpatialFilterInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CDNR_INFO_s Data;
    } CdnrInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_ASF_INFO_s Data;
    } AsfInfo;
#endif

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_FILTER_s Data;
    } ChromaFilterInfo;

} STILL_HISO_NORMAL_CONFIG_s;

/* The follow structure is for High iso configure only */
typedef struct _STILL_HISO_CONFIG_s_ {
    struct {
        UINT32 Enable;
    #ifdef CONFIG_SOC_A9
        AMBA_DSP_IMG_ANTI_ALIASING_FILTER_s Data;
    #else
        AMBA_DSP_IMG_ANTI_ALIASING_s Data;
    #endif
    } AntiAliasing;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s Data;
    } CfaLeakageFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DBP_CORRECTION_s Data;
    } DynamicBadPixelCorrection;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CFA_NOISE_FILTER_s Data;
    } CfaNoiseFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_GBGR_MISMATCH_s Data;
    } GbGrMismatch;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DEMOSAIC_s Data;
    } Demosaic;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s Data;
    } ChromaMedianFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CDNR_INFO_s Data;
    } ColorDependentNoiseReduction;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s Data;
    } DeferColorCorrection;

#ifdef CONFIG_SOC_A9
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_ASF_INFO_s Data;
    } AdvanceSpatialFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_ASF_INFO_s Data;
    } HighAdvanceSpatialFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_ASF_INFO_s Data;
    } LowAdvanceSpatialFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_ASF_INFO_s Data;
    } Med1AdvanceSpatialFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_ASF_INFO_s Data;
    } Med2AdvanceSpatialFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_ASF_INFO_s Data;
    } Li2ndAdvanceSpatialFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_ASF_INFO_s Data;
    } ChromaAdvanceSpatialFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_BOTH_s Data;
    } HighSharpenNoiseBoth;
#endif

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_NOISE_s Data;
    } HighSharpenNoiseNoise;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_FIR_s Data;
    } HighSharpenNoiseSharpenFir;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CORING_s Data;
    } HighSharpenNoiseSharpenCoring;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } HighSharpenNoiseSharpenCoringIndexScale;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } HighSharpenNoiseSharpenMinCoringResult;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } HighSharpenNoiseSharpenScaleCoring;

#ifdef CONFIG_SOC_A9
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_BOTH_s Data;
    } MedSharpenNoiseBoth;
#endif

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_NOISE_s Data;
    } MedSharpenNoiseNoise;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_FIR_s Data;
    } MedSharpenNoiseSharpenFir;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CORING_s Data;
    } MedSharpenNoiseSharpenCoring;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } MedSharpenNoiseSharpenCoringIndexScale;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } MedSharpenNoiseSharpenMinCoringResult;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } MedSharpenNoiseSharpenScaleCoring;

#ifdef CONFIG_SOC_A9
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_BOTH_s Data;
    } Liso1SharpenNoiseBoth;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_NOISE_s Data;
    } Liso1SharpenNoiseNoise;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_FIR_s Data;
    } Liso1SharpenNoiseSharpenFir;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CORING_s Data;
    } Liso1SharpenNoiseSharpenCoring;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } Liso1SharpenNoiseSharpenCoringIndexScale;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } Liso1SharpenNoiseSharpenMinCoringResult;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } Liso1SharpenNoiseSharpenScaleCoring;
#endif

#ifdef CONFIG_SOC_A9
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_BOTH_s Data;
    } Liso2SharpenNoiseBoth;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_NOISE_s Data;
    } Liso2SharpenNoiseNoise;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_FIR_s Data;
    } Liso2SharpenNoiseSharpenFir;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CORING_s Data;
    } Liso2SharpenNoiseSharpenCoring;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } Liso2SharpenNoiseSharpenCoringIndexScale;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } Liso2SharpenNoiseSharpenMinCoringResult;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } Liso2SharpenNoiseSharpenScaleCoring;
#endif

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_FILTER_s Data;
    } ChromaFilterHigh;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s Data;
    } ChromaFilterLowVeryLow;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_CHROMA_FILTER_s Data;
    } ChromaFilterPre;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_CHROMA_FILTER_s Data;
    } ChromaFilterMed;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_CHROMA_FILTER_s Data;
    } ChromaFilterLow;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_CHROMA_FILTER_s Data;
    } ChromaFilterVeryLow;

#ifdef CONFIG_SOC_A9
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s Data;
    } ChromaFilterMedCombine;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s Data;
    } ChromaFilterLowCombine;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s Data;
    } ChromaFilterVeryLowCombine;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s Data;
    } LumaNoiseCombine;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s Data;
    } LowASFCombine;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_COMBINE_s Data;
    } HighIsoCombine;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_FREQ_RECOVER_s Data;
    } HighIsoFreqRecover;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_LUMA_BLEND_s Data;
    } HighIsoLumaBlend;
#endif

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_BLEND_s Data;
    } HighIsoBlendLumaLevel;

} STILL_HISO_CONFIG_s;

#endif /* _AMPUT_STILL_ISO_CONFIG_H_ */
