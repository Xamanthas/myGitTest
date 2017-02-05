 /**
  * @file src/app/sample/unittest/AmpUT_VideoIsoConfig.h
  *
  * Video ISO config setting header
  *
  * History:
  *    2014/11/07 - [Edgar Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */


#ifndef _AMPUT_VIDEO_ISO_CONFIG_H_
#define _AMPUT_VIDEO_ISO_CONFIG_H_

#include "AmbaDSP_ImgFilter.h"
#include "AmbaDSP_Img3aStatistics.h"
#include "AmbaDSP_ImgUtility.h"

/* The follow structure is for low iso configure */
typedef struct _VIDEO_ISO_CONFIG_s_ {
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_RGB_TO_YUV_s Data;
    } Rgb2YuvInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_BLACK_CORRECTION_s Data;
    } StaticBlackLevelInfo;

	struct {
        UINT32 Enable;
        AMBA_DSP_IMG_ANTI_ALIASING_s  Data;
    } AntiAliasingInfo;
	
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DBP_CORRECTION_s Data;
    } BadPixelCorrectionInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CFA_NOISE_FILTER_s Data;
    } CfaNoiseFilterInfo;
	
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LOCAL_EXPOSURE_s Data;
    } LocalExposureInfo;
	
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s Data;
    } CfaLeakageFilterInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DEMOSAIC_s Data;
    } DemosaicInfo;

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
        AMBA_DSP_IMG_CHROMA_SCALE_s Data;
    } ChromaScaleInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s Data;
    } ChromaMedianFilterInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CDNR_INFO_s Data;
    } CDNR;

	struct {
        UINT32 Enable;
#ifdef CONFIG_SOC_A12
        AMBA_DSP_IMG_LISO_PROCESS_SELECT_s Data;
#endif
    } LumaProcess;

	struct {
        UINT32 Enable;
        AMBA_DSP_IMG_ASF_INFO_s Data;
    } Asf;

	struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_BOTH_s Data;
    } SharpenABoth;

	struct {
        UINT32 Enable;
        AMBA_DSP_IMG_SHARPEN_NOISE_s Data;
    } SharpenNoise;

	struct {
        UINT32 Enable;
        AMBA_DSP_IMG_FIR_s Data;
    } SharpenAFir;

	struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CORING_s Data;
    } SharpenCoring;
	
	struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } SharpenCoringIdxScale;

	struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } SharpenMinCoring;
	
	struct {
        UINT32 Enable;
        AMBA_DSP_IMG_LEVEL_s Data;
    } SharpenScaleCoring;	

	struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CDNR_INFO_s Data;
    } LiCDNR;	

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_FILTER_s Data;
    } ChromaFilterInfo;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s Data;
    } LiWideChromaFilterComb;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_GBGR_MISMATCH_s Data;
    } GbGrMismatch;

    /* HISO only */
    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s Data;
    } CfaLeakageFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_ANTI_ALIASING_s Data;
    } HiAntiAliasing;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DBP_CORRECTION_s Data;
    } HiDynamicBadPixelCorrection;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CFA_NOISE_FILTER_s Data;
    } HiCfaNoiseFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_GBGR_MISMATCH_s Data;
    } HiGbGrMismatch;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DEMOSAIC_s Data;
    } HiDemosaic;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s Data;
    } HiChromaMedianFilter;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s Data;
    } HiDeferColorCorrection;

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

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_CHROMA_FILTER_s Data;
    } ChromaFilterPre;

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_CHROMA_FILTER_s Data;
    } ChromaFilterHigh;

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

    struct {
        UINT32 Enable;
        AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s Data;
    } ChromaFilterLowVeryLow;

} VIDEO_ISO_CONFIG_s;
#endif /* _AMPUT_VIDEO_ISO_CONFIG_H_ */
