/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaImgCalibItuner.h
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions & Constants for Ambarella DSP Image Kernel ituner APIs
\*-------------------------------------------------------------------------------------------------------------------*/
/*!
 * @addtogroup Ituner
 * @{
 */
#ifndef _AMBA_IMG_CALIB_ITUNER_H_
#define _AMBA_IMG_CALIB_ITUNER_H_

#include "AmbaDSP_ImgDef.h"
#include "AmbaDSP.h"
#include "AmbaDSP_Img3aStatistics.h" // AMBA_DSP_IMG_AE_STAT_INFO_s need
#include "AmbaDSP_ImgFilter.h" // AMBA_DSP_IMG_BLACK_CORRECTION_s need
#include "AmbaKAL.h"

__BEGIN_C_PROTO__
#define ITUNER_VER_MAJOR (1)
#define ITUNER_VER_MINOR (0)
#define MAX_PARAMS (16)
#define APP_MAX_DIR_SIZE (64)
#define APP_MAX_FN_SIZE (64)
#define APP_MAX_TITLE_SIZE (20)
#define ITUNER_MAX_FPN_MAP_SIZE (4000*3000)
#define ITUNER_VIGNETTE_GAIN_MAP_SIZE (2*65*49)
#define ITUNER_BYPASS_VIGNETTE_GAIN_MAP_SIZE (33*33*2)
/*!@brief Use to transfer TUNING_MODE_e to String for text file*/
#define TUING_MODE_TO_STR(TUING_MODE) #TUING_MODE
/*!@brief Use to transfer TUNING_MODE_EXT_e to String for text file*/
#define TUING_MODE_EXT_TO_STR(TUING_MODE_EXT) #TUING_MODE_EXT
//#define POSTPROC_MAX_RAW_PICTURE    32
//#define POSTPROC_MAX_YUV_PICTURE    32

//#define ITUNER_CAL_FPN_MAP_SIZE(width, height) (width * height >> 3)
#define PITCH_ALIGN 32
#define ITUNER_CAL_FPN_MAP_SIZE(width, height) (((width >> 3)+(PITCH_ALIGN-1))/PITCH_ALIGN * height * PITCH_ALIGN)

/*!@note: Bypass FPN table size is referenced to A7L, fpn_size = G_data.static_bad_pixel_correction.pixel_map_pitch * G_data.static_bad_pixel_correction.pixel_map_height;*/
#define ITUNER_CAL_BYPASS_FPN_MAP_SIZE(width, height) (width * height)
#define ITUNER_CAL_WARP_TABLE_SIZE(width, height) (width * height * sizeof(AMBA_DSP_IMG_GRID_POINT_s))
#define ITUNER_CAL_CA_TABLE_SIZE(width, height) (width * height * sizeof(AMBA_DSP_IMG_GRID_POINT_s))
/*!@brief Specified the tuning mode*/
typedef enum {
    IMG_MODE_VIDEO = 0, //!< Video Tuning
    IMG_MODE_STILL, //!< Still Tuning
    IMG_MODE_LOW_ISO_STILL, //!< Low Iso Still Tuning
    IMG_MODE_MID_ISO_STILL, //!< Reserved
    IMG_MODE_HIGH_ISO_STILL,//!< High Iso Still Tuning
    IMG_MODE_NIGHT_PORTRAIT_STILL, //!< Reserved
    IMG_MODE_PREVIEW, //!< Preview Tuning, This tuning mode will skip ituner check
    IMG_MODE_HIGH_ISO_PREVIEW, //!< High Iso Preview Tuning, This tuning mode will skip ituner check
    IMG_MODE_HIGH_ISO_VIDEO, //!< High Iso Video Tuning
    IMG_MODE_NUMBER,
} TUNING_MODE_e;

/*!@brief Specified the tuning mode ext*/
typedef enum {
    SINGLE_SHOT = 0, //!< Single Shot Mode
    MULTI_SHOT_HDR, //!< Reserved
    MULTI_SHOT_MF_HISO, //!< Reserved
    MULTI_SHOT_MAHDR, //!< Reserved
    SINGLE_SHOT_MULTI_EXPOSURE_HDR,
    DYNAMIC_OB,
    TUNING_MODE_EXT_NUMBER,
} TUNING_MODE_EXT_e;

/*!@brief Specified the tuning mode info, that capture flow need*/
typedef struct {
    AMBA_DSP_IMG_MODE_CFG_s TuningAlgoMode;
    TUNING_MODE_EXT_e TuningModeExt;
} ITUNER_INFO_s;

typedef struct _AAA_FUNC_s_{
    UINT8  AeOp;
    UINT8  AwbOp;
    UINT8  AfOp;
    UINT8  AdjOp;
    UINT8  Reserved1;
    UINT8  Reserved2;
    UINT8  Reserved3;
    UINT8  Reserved4;
} AAA_FUNC_s;

typedef struct _ITUNER_AE_INFO_s_ {
    UINT16    EvIndex;
    UINT16    NfIndex;
    INT16     ShutterIndex;
    INT16     AgcIndex;
    INT16     IrisIndex;
    INT32     Dgain;
    UINT16    IsoValue;
    UINT16    Flash;
    UINT16    Mode;
    INT32     ShutterTime;
    INT32     AgcGain;
    UINT16    Target;
    UINT16    LumaStat;
    INT16     LimitStatus;
    UINT32    Multiplier;   //1024*1024
} ITUNER_AE_INFO_s;

typedef struct _ITUNER_WB_SIM_INFO_s_ {
    UINT16  LumaIdx;
    UINT16  OutDoorIdx;
    UINT16  HighLightIdx;
    UINT16  LowLightIdx;
    UINT32  AwbRatio[2];
    UINT32  Reserved;
    UINT32  Reserved1;
    UINT32  Reserved2;
    UINT32  Reserved3;
} ITUNER_WB_SIM_INFO_s;

typedef struct _IMG_EXP_s_{
    int ShutterIndex;
    int AgcIndex;
    int IrisIndex;
    int Dgain;
} IMG_EXP_s;

typedef struct _ITUNER_WARP_s_ {
    UINT8   Enable;
    UINT32  CalibVersion;
    int     HorizontalGridNumber;
    int     VerticalGridNumber;
    int     TileWidthExponent;
    int     TileHeightExponent;
    UINT32  VinSensorStartX;
    UINT32  VinSensorStartY;
    UINT32  VinSensorWidth;
    UINT32  VinSensorHeight;
    UINT8   VinSensorHSubSampleFactorNum;
    UINT8   VinSensorHSubSampleFactorDen;
    UINT8   VinSensorVSubSampleFactorNum;
    UINT8   VinSensorVSubSampleFactorDen;
    char    WarpGridTablePath[64];
} ITUNER_WARP_s;

typedef struct _ITUNER_DZOOM_s_ {
    UINT8  Enable;
    UINT32 ActualLeftTopX;
    UINT32 ActualLeftTopY;
    UINT32 ActualRightBotX;
    UINT32 ActualRightBotY;
    int    HorSkewPhaseInc;
    UINT16 DummyWindowLeftX;
    UINT16 DummyWindowTopY;
    UINT16 DummyWindowWidth;
    UINT16 DummyWindowHeight;
    UINT16 CfaOutputWidth;
    UINT16 CfaOutputHeight;
    UINT16 R2rOutputWidth;
    UINT16 R2rOutputHeigh;
} ITUNER_DZOOM_s;

typedef struct _ITUNER_WARP_DZOOM_BYPASS_s_ {

    // Warp part
    UINT32  WarpControl;
    UINT8   GridArrayWidth;
    UINT8   GridArrayHeight;
    UINT8   HorzGridSpacingExponent;
    UINT8   VertGridSpacingExponent;
    UINT8   VertWarpEnable;
    UINT8   VertWarpGridArrayWidth;
    UINT8   VertWarpGridArrayHeight;
    UINT8   VertWarpHorzGridSpacingExponent;
    UINT8   VertWarpVertGridSpacingExponent;
    char    WarpHorizontalTablePath[64];
    char    WarpVerticalTablePath[64];

    // Dzoom part
    UINT32  ActualLeftTopX;
    UINT32  ActualLeftTopY;
    UINT32  ActualRightBotX;
    UINT32  ActualRightBotY;
    UINT32  ZoomX;
    UINT32  ZoomY;
    UINT32  XCenterOffset;
    UINT32  YCenterOffset;
    INT32   HorSkewPhaseInc;
    UINT16  DummyWindowXLeft;
    UINT16  DummyWindowYTop;
    UINT16  DummyWindowWidth;
    UINT16  DummyWindowHeight;
    UINT16  CfaOutputWidth;
    UINT16  CfaOutputHeight;

} ITUNER_WARP_DZOOM_BYPASS_s;

typedef struct _ITUNER_VIGNETTE_s_ {
    UINT8    Enable;
    UINT8    GainShift;
    UINT8    StrengthEffectMode;
    UINT32   ChromaRatio;
    UINT32   Strength;
    UINT32   CalibVersion;
    int      CalibTableWidth;
    int      CalibTableHeight;
    UINT32   CalibVinStartX;
    UINT32   CalibVinStartY;
    UINT32   CalibVinWidth;
    UINT32   CalibVinHeight;
    UINT8    CailbVinHSubSampleFactorNum;
    UINT8    CailbVinHSubSampleFactorDen;
    UINT8    CailbVinVSubSampleFactorNum;
    UINT8    CailbVinVSubSampleFactorDen;
    char     CalibTablePath[64];
} ITUNER_VIGNETTE_s;

typedef struct _ITUNER_VIGNETTE_BYPASS_s_ {
    UINT8    Enable;
    UINT8    GainShift;
    char     GainPath[64];
} ITUNER_VIGNETTE_BYPASS_s;

typedef struct _ITUNER_FPN_s_ {
    UINT8  Enable;
    UINT32 CalibVersion;
    UINT32 CailbVinStartX;
    UINT32 CailbVinStartY;
    UINT32 CailbVinWidth;
    UINT32 CailbVinHeight;
    UINT8  CailbVinHSubSampleFactorNum;
    UINT8  CailbVinHSubSampleFactorDen;
    UINT8  CailbVinVSubSampleFactorNum;
    UINT8  CailbVinVSubSampleFactorDen;
    char   MapPath[64];
} ITUNER_FPN_s;

typedef struct _ITUNER_FPN_BYPASS_s_ {
    UINT8   Enable;
    UINT16  PixelMapWidth;
    UINT16  PixelMapHeight;
    UINT16  PixelMapPitch;
    char    MapPath[64];
} ITUNER_FPN_BYPASS_s;

typedef struct _ITUNER_SENSOR_INPUT_s_ {
    UINT8    SensorId;
    UINT8    BayerPattern;
    UINT8    SensorResolution;
    UINT32   ReadoutMode;
} ITUNER_SENSOR_INPUT_s;

typedef struct _ITUNER_COLOR_CORRECTION_s_ {
    char    RegPath[64];
    char    ThreeDPath[64];
    //char    SecCcPath[64];
} ITUNER_COLOR_CORRECTION_s;

typedef struct _ITUNER_CHROMA_ABERRATION_s_ {
    UINT8   Enable;
    UINT32  CalibVersion;
    int     HorizontalGridNumber;
    int     VerticalGridNumber;
    int     TileWidthExponent;
    int     TileHeightExponent;
    UINT32  VinSensorStartX;
    UINT32  VinSensorStartY;
    UINT32  VinSensorWidth;
    UINT32  VinSensorHeight;
    UINT8   VinSensorHSubSampleFactorNum;
    UINT8   VinSensorHSubSampleFactorDen;
    UINT8   VinSensorVSubSampleFactorNum;
    UINT8   VinSensorVSubSampleFactorDen;
    UINT32  RedScaleFactor;
    UINT32  BlueScaleFactor;
    char    CaGridTablePath[64];
} ITUNER_CHROMA_ABERRATION_s;


typedef struct _ITUNER_CHROMA_ABERRATION_BYPASS_s_ {

    UINT16  HorzWarpEnable;
    UINT16  VertWarpEnable;
    UINT8   HorzPassGridArrayWidth;
    UINT8   HorzPassGridArrayHeight;
    UINT8   HorzPassHorzGridSpacingExponent;
    UINT8   HorzPassVertGridSpacingExponent;
    UINT8   VertPassGridArrayWidth;
    UINT8   VertPassGridArrayHeight;
    UINT8   VertPassHorzGridSpacingExponent;
    UINT8   VertPassVertGridSpacingExponent;
    UINT16  RedScaleFactor;
    UINT16  BlueScaleFactor;
    char    WarpHorzTablePath[64];
    char    WarpVertTablePath[64];
} ITUNER_CHROMA_ABERRATION_BYPASS_s;

typedef struct _ITUNER_ASF_INFO_s_{
    AMBA_DSP_IMG_ASF_INFO_s    AsfInfo;
} ITUNER_ASF_INFO_s;

typedef struct _ITUNER_CHROMA_ASF_INFO_s_{
    AMBA_DSP_IMG_CHROMA_ASF_INFO_s    ChromaAsfInfo;
    char    ThreeDTablePath[64];
} ITUNER_CHROMA_ASF_INFO_s;

typedef struct _ITUNER_SHARPEN_BOTH_s_{
    AMBA_DSP_IMG_SHARPEN_BOTH_s  BothInfo;
} ITUNER_SHARPEN_BOTH_s;

typedef struct _ITUNER_HISO_CHROMA_FILTER_COMBINE_s_{
    AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s CombineInfo;
} ITUNER_HISO_CHROMA_FILTER_COMBINE_s;

typedef struct _ITUNER_HISO_LUMA_FILTER_COMBINE_s_{
    AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s CombineInfo;
} ITUNER_HISO_LUMA_FILTER_COMBINE_s;

typedef struct _ITUNER_HISO_COMBINE_s_{
    AMBA_DSP_IMG_HISO_COMBINE_s CombineInfo;
    char ThreeDTablePath[64];
} ITUNER_HISO_COMBINE_s;

typedef struct _ITUNER_VIDEO_HDR_ALPHA_CALC_ALPHA_s_ {
    AMBA_DSP_IMG_HDR_ALPHA_CALC_ALPHA_s Alpha;
    // TODO: Real Alpha Table Buffer
    char AlphaTablePath[64];
} ITUNER_VIDEO_HDR_ALPHA_CALC_ALPHA_s;

typedef struct _ITUNER_VIDEO_HDR_AMP_LINEARIZATION_s_{
    AMBA_DSP_IMG_AMP_LINEARIZATION_s AmpLinear;
    char Linear0LookupTablePath[64];
    char Linear1LookupTablePath[64];
} ITUNER_VIDEO_HDR_AMP_LINEARIZATION_s;

typedef struct _ITUNER_VIDEO_HDR_TABLE_s_{
    UINT8  AlphaCalcAlphaTable[3][128];
    UINT16 AmpLinear0LookupTable[3][AMBA_DSP_IMG_HDR_AMPLINEAR_LUT_SIZE];
    UINT16 AmpLinear1LookupTable[3][AMBA_DSP_IMG_HDR_AMPLINEAR_LUT_SIZE];
} ITUNER_VIDEO_HDR_TABLE_s;


typedef struct _ITUNER_VIDEO_HDR_RAW_INFO_s_{
    UINT16 RawAreaWidth[AMBA_DSP_IMG_NUM_MAX_HDR_EXPO];
    UINT16 RawAreaHeight[AMBA_DSP_IMG_NUM_MAX_HDR_EXPO];
    AMBA_DSP_IMG_HDR_RAW_OFFSET_s Offset;
    UINT8 NumberOfExposures;
} ITUNER_VIDEO_HDR_RAW_INFO_s;

typedef struct _ITUNER_VIDEO_HDR_s_{
    /*
    AMBA_DSP_IMG_HDR_BLKLVL_AMPLINEAR_s AmpLinearHdr[3];
    AMBA_DSP_IMG_BLACK_CORRECTION_s SEStaticBlackLevel[3];
    */
    AMBA_DSP_IMG_BLACK_CORRECTION_s LEStaticBlackLevel[3];
    AMBA_DSP_IMG_HDR_ALPHA_CALC_CONFIG_s AlphaCalcConfig[3];
    AMBA_DSP_IMG_HDR_ALPHA_CALC_THRESH_s AlphaCalcThresh[3];
    AMBA_DSP_IMG_BLACK_CORRECTION_s AlphaCalcBlackLevel[3];
    ITUNER_VIDEO_HDR_ALPHA_CALC_ALPHA_s AlphaCalcAlpha[3];
    ITUNER_VIDEO_HDR_AMP_LINEARIZATION_s AmpLinear[3];
    AMBA_DSP_IMG_CONTRAST_ENHANCE_s ContrastEnhance;
    ITUNER_VIDEO_HDR_RAW_INFO_s RawInfo;
    ITUNER_VIDEO_HDR_TABLE_s Tables;
} ITUNER_VIDEO_HDR_s;

typedef struct _Ituner_Internal_s_ {
    INT8   FpnHighlight;
    UINT16 Internal_u16_1;
    UINT16 Internal_u16_2;
    int    Internal_s32_0;
    int    Internal_s32_1;
    int    Internal_s32_2;
    int    Internal_s32_3;
    int    Internal_s32_4;
    int    Internal_sha_0;
    int    Internal_sha_1;
    int    Internal_sha_2;
    int    Internal_sha_3;
    int    Internal_sha_4;
    int    Internal_sha_5;
    int    Internal_shb_0;
    int    Internal_shb_1;
    int    Internal_shb_2;
    int    Internal_shb_3;
    int    Internal_shb_4;
    int    Internal_shb_5;
    int    Internal_wm_0;
    int    Internal_wm_1;
    int    Internal_wm_2;
    int    Internal_wm_3;
    int    NOT_CUSTOMER_VISIBLE_FULL_RAW_W;
    int    NOT_CUSTOMER_VISIBLE_FULL_MAIN_W;
    UINT8  Reserved[3];
    int    Internal_hdr_0;
    int    Internal_hdr_1;
    UINT8  DiagMode;
} Ituner_Internal_s;

#if 0
typedef struct postproc_info_s {
    UINT32 src_raw_addr[POSTPROC_MAX_RAW_PICTURE];             /* < source raw address, recorder only */
    UINT32 src_raw_count;                                      /* < Current raw number, recorder only */
    UINT32 src_raw_pitch;                                      /* < source raw pitch,   recorder only */
    UINT32 src_raw_width;                                      /* < source raw width,   recorder only */
    UINT32 src_raw_height;                                     /* < source raw height,  recorder only */
    UINT32 src_thm_y_addr;                                     /* < source thumbnail y address,  recorder only, count is equal to src_y_count */
    UINT32 src_thm_uv_addr;                                    /* < source thumbnail uv address, recorder only */
    UINT32 src_thm_y_pitch;                                    /* < source thumbnail y pitch,  recorder only */
    UINT32 src_thm_width;                                      /* < source thumbnail y width,  recorder only */
    UINT32 src_thm_height;                                     /* < source thumbnail y height, recorder only */
    UINT32 src_yuv_type;                                       /* < source yuv type */
    UINT32 src_y_addr;                                         /* < source y address */
    UINT32 src_uv_addr;                                        /* < source uv address */
    UINT32 src_y_pitch;                                        /* < source y pitch */
    UINT32 src_y_width;                                        /* < source y width */
    UINT32 src_y_height;                                       /* < source y height */
    UINT32 scaled_yuv_type;                                    /* < scaled yuv type,   player only */
    UINT32 scaled_y_addr;                                      /* < scaled y address,  player only */
    UINT32 scaled_uv_addr;                                     /* < scaled uv address, player only */
    UINT32 scaled_pitch;                                       /* < scaled pitch,      player only */
    UINT32 scaled_width;                                       /* < scaled width,      player only */
    UINT32 scaled_height;                                      /* < scaled height,     player only */
    UINT32 preview_y_addr;                                     /* < preview y address */
    UINT32 preview_uv_addr;                                    /* < preview uv address */
    UINT32 preview_pitch;                                      /* < preview pitch */
    UINT32 preview_width;                                      /* < preview width */
    UINT32 preview_height;                                     /* < preview height */
    UINT32 secondary_stream_y_addr;                            /* < secondary stream y address */
    UINT32 secondary_stream_uv_addr;                           /* < secondary stream uv address */
    UINT32 secondary_stream_pitch;                             /* < secondary stream pitch */
    UINT32 secondary_stream_width;                             /* < secondary stream width */
    UINT32 secondary_stream_height;                            /* < secondary stream height */
    UINT32 src_scrn_thm_y_addr;                                /* < source screen thumbnail y address,  recorder only, count is equal to src_y_count */
    UINT32 src_scrn_thm_uv_addr;                               /* < source screen thumbnail uv address, recorder only */
    UINT32 src_scrn_thm_y_pitch;                               /* < source screen thumbnail y pitch,    recorder only */
    UINT32 src_scrn_thm_width;                                 /* < source screen thumbnail y width,    recorder only */
    UINT32 src_scrn_thm_height;                                /* < source screen thumbnail y height,   recorder only */
    UINT32 src_decision_map;                                   /* < For selection flow, callback function should tell flow controller which candidate is to
                                                                    be saved to jpeg. bit[0] means the 1st picture*/
    UINT32 src_uncompressed_raw_addr;                          /* < Current round's uncompressed raw data,             recorder only*/
    UINT32 src_uncompressed_raw_modified;                      /* < Postprocessor has modified source raw data or not, recorder only*/
    UINT32 sec_scaled_yuv_type;                                /* < scaled yuv type,   player only */
    UINT32 sec_scaled_y_addr;                                  /* < scaled y address,  player only */
    UINT32 sec_scaled_uv_addr;                                 /* < scaled uv address, player only */
    UINT32 sec_scaled_pitch;                                   /* < scaled pitch,      player only */
    UINT32 sec_scaled_width;                                   /* < scaled width,      player only */
    UINT32 sec_scaled_height;                                  /* < scaled height,     player only */
    UINT32 src_y_count;                                        /* < Last y array position */
    UINT32 src_y_addr_ext[POSTPROC_MAX_YUV_PICTURE];           /* < source y address */
    UINT32 src_uv_addr_ext[POSTPROC_MAX_YUV_PICTURE];          /* < source uv address */
    UINT32 src_thm_y_addr_ext[POSTPROC_MAX_YUV_PICTURE];       /* < source thumbnail y address,  recorder only, count is equal to src_y_count */
    UINT32 src_thm_uv_addr_ext[POSTPROC_MAX_YUV_PICTURE];      /* < source thumbnail uv address, recorder only */
    UINT32 src_scrn_thm_y_addr_ext[POSTPROC_MAX_YUV_PICTURE];  /* < source screen thumbnail y address, recorder only, count is equal to src_y_count */
    UINT32 src_scrn_thm_uv_addr_ext[POSTPROC_MAX_YUV_PICTURE]; /* < source screen thumbnail uv address, recorder only */
    UINT32 piv_y_addr;                                         /* < piv y address */
    UINT32 piv_uv_addr;                                        /* < piv uv address */
    UINT32 piv_pitch;                                          /* < piv pitch */
    UINT32 piv_width;                                          /* < piv width */
    UINT32 piv_height;                                         /* < piv height */
    UINT32 alpha_stream_addr;                                  /* < for yuv blending */
    UINT32 selection_queue_total;                              /* < Total number of raw_select or yuv_select when selection begins */
} postproc_info_t;

typedef struct vin_status_s {
    UINT8  video_mode;
    UINT8  input_format;
    UINT8  input_frame_rate_video;
    UINT8  input_frame_rate_mjpg;
    UINT8  input_frame_rate_still;
    UINT8  padding[3];
    UINT16 input_aspect_ratio;
    UINT8  sensor_id;
    UINT8  bayer_pattern;
    UINT16 vidcap_w;
    UINT16 vidcap_h;
    UINT16 stillcap_w;
    UINT16 stillcap_h;
    UINT16 mjpegcap_w;
    UINT16 mjpegcap_h;
    UINT16 vio_sync_start_line;
    UINT16 vio_sync_start_line1;
    UINT32 sensor_readout_mode;
    UINT8  video_source;
    UINT8  bit_resolution;
    UINT16 reserved[3];
} vin_status_t;

typedef struct rec_stillproc_from_mem_s {
    UINT8  input_format;
#define MW_REC_STILL_INPUT_RAW        0
#define MW_REC_STILL_INPUT_YUV422    1
#define MW_REC_STILL_INPUT_YUV420    2
    UINT8  bayer_pattern;
    UINT8  resolution;
    UINT8  still_process_mode;
#define MW_REC_STILL_PROCESS_MODE_DEFAULT    2
    void (*src_load)(UINT32 input_addr, UINT32 buf_size);
    UINT32 input_bytes;
    UINT16 input_pitch;
    UINT16 input_chroma_pitch;
    UINT16 input_h;
    UINT16 input_w;
    UINT16 encode_h;
    UINT16 encode_w;
    UINT16 cap_cntl;
    UINT16 reserved;
    UINT32 src_buf_addr;        /* pre-allocated buffer address, if non-zero, MW will not call src_load() */
    UINT32 number_of_frames;    /* For simulating multiple frame features */
    UINT32 input_pic_offset;
    UINT32 yuv_buffer_num;
    UINT32 yuv422_buffer_num;
    UINT32 sensor_readout_mode;
} rec_stillproc_from_mem_info_t;
#endif
typedef struct _IMG_SLICE_INFO_s_ {
    /* System */
    UINT8             SliceNumberX;
    UINT8             SliceNumberY;
    /* Raw frame DMA */
    AMBA_DSP_WINDOW_s RawFrnDmaInfo;
    /* Cfa Scaler */
    AMBA_DSP_WINDOW_s CfaScalerOutInfo;
    /* Mctf */
    AMBA_DSP_WINDOW_s MctfOutWin;
    UINT32            MctfOutCfgAddr;
    /* IDSP section 2 */
    AMBA_DSP_WINDOW_s Sec2CfgWin;
    UINT32            Sec2CfgAddr;
    /* IDSP section 3 */
    AMBA_DSP_WINDOW_s Sec3CfgWin;
    UINT32            Sec3CfgAddr;
} IMG_SLICE_INFO_s;

#define MAX_IDSP_CONFIG_VIDEO_CHANNEL_NUM   5
#define MAX_IDSP_CONFIG_VIDEO_SLICE_NUM 2
#define MAX_IDSP_CONFIG_STILL_SLICE_NUM 4

typedef struct _IMG_IDSP_CONFIG_INFO_s_ {
    UINT32            IdspCfgInfoSize;
    AMBA_DSP_WINDOW_s VCapWin;
    AMBA_DSP_WINDOW_s EncWin;
    UINT8             TotalVideoSliceNumberX;
    UINT8             TotalVideoSliceNumberY;
    UINT8             TotalStillSliceNumberX;
    UINT8             TotalStillSliceNumberY;
    IMG_SLICE_INFO_s  VideoSliceInfo[MAX_IDSP_CONFIG_VIDEO_CHANNEL_NUM][MAX_IDSP_CONFIG_VIDEO_SLICE_NUM];
    IMG_SLICE_INFO_s  StillSliceInfo[MAX_IDSP_CONFIG_STILL_SLICE_NUM];
    UINT32            Sec2CfgLen;
    UINT32            Sec3CfgLen;
} IMG_IDSP_CONFIG_INFO_s;

typedef struct _ITUNER_SYSTEM_s_ {
    char    AmbacamRev[16];
    char    ChipRev[8];
    char    SensorId[32];
    char    UserMode[32];
    char    TuningMode[32];
    char    TuningModeExt[32];
    char    RawPath[64];
    char    ExtConfigPath[64];
    UINT16  RawWidth;
    UINT16  RawHeight;
    UINT16  RawPitch;
    UINT16  RawResolution;
    UINT16  RawBayer;
    UINT16  MainWidth;
    UINT16  MainHeight;
    UINT16  InputPicCnt;
    UINT16  CompressedRaw;
    UINT16  SensorReadoutMode;
    UINT32  RawStartX;
    UINT32  RawStartY;
    UINT8   HSubSampleFactorNum;
    UINT8   HSubSampleFactorDen;
    UINT8   VSubSampleFactorNum;
    UINT8   VSubSampleFactorDen;
    char    OutputFilename[64];
    UINT8   EnableRaw2Raw;
    UINT8   JpgQuality;
    //UINT16  reserved[2];
} ITUNER_SYSTEM_s;

/*!@brief Specified Calib Table*/
#define ALIGN_PAD_SIZE(size, alignbyte) ((alignbyte)-((size)%(alignbyte)))
typedef struct {
    UINT8 FPNMap[ITUNER_MAX_FPN_MAP_SIZE]; //!< FPN Map Need 4 Bytes Alignment
    UINT8 VignetteTable[ITUNER_VIGNETTE_GAIN_MAP_SIZE * 4];
    UINT8 Reserved1[ALIGN_PAD_SIZE((ITUNER_VIGNETTE_GAIN_MAP_SIZE * 4), 32)];
    UINT8 WarpGrid[4*96*64];
    UINT8 Ca_grid[4*96*64];
    UINT8 CcReg[AMBA_DSP_IMG_CC_REG_SIZE];
    UINT8 Reserved2[ALIGN_PAD_SIZE(AMBA_DSP_IMG_CC_REG_SIZE, 32)];
    UINT8 Cc3d[AMBA_DSP_IMG_CC_3D_SIZE];
    //UINT8 SecCc[AMBA_DSP_IMG_SEC_CC_SIZE];
} ITUNER_Calib_Table_s __attribute__((aligned (32))) ;

typedef struct {
    UINT8 FPNMap[ITUNER_MAX_FPN_MAP_SIZE]; //!< FPN Map Need 4 Bytes Alignment
} ITUNER_DualChan_Calib_Table_s __attribute__((aligned (32))) ;

/*!@brief Specified Ituner Filter Type*/
typedef enum {
    ITUNER_SYSTEM_INFO = 0,
    ITUNER_INTERNAL_INFO,
    ITUNER_AAA_FUNCTION,
    ITUNER_AE_INFO,
    ITUNER_WB_SIM_INFO,
    ITUNER_AE_STATISTICS_INFO,
    ITUNER_AWB_STATISTICS_INFO,
    ITUNER_AF_STATISTICS_INFO,
    ITUNER_STATIC_BAD_PIXEL_CORRECTION,
    ITUNER_STATIC_BAD_PIXEL_CORRECTION_BY_PASS,
    ITUNER_VIGNETTE_COMPENSATION,
    ITUNER_VIGNETTE_COMPENSATION_BY_PASS,
    ITUNER_WARP_COMPENSATION,
    ITUNER_DZOOM,
    ITUNER_WARP_COMPENSATION_DZOOM_BY_PASS,
    ITUNER_SENSOR_INPUT_INFO,
    ITUNER_STATIC_BLACK_LEVEL,
    ITUNER_DEFERRED_BLACK_LEVEL,
    ITUNER_DYNAMIC_BAD_PIXEL_CORRECTION,
    ITUNER_HISO_DYNAMIC_BAD_PIXEL_CORRECTION,
    ITUNER_CFA_LEAKAGE_FILTER,
    ITUNER_HISO_CFA_LEAKAGE_FILTER,
    ITUNER_CFA_NOISE_FILTER,
    ITUNER_HISO_CFA_NOISE_FILTER,
    ITUNER_ANTI_ALIASING_STRENGTH,
    ITUNER_HISO_ANTI_ALIASING_STRENGTH,
    ITUNER_GLOBAL_DGAIN,
    ITUNER_WB_GAIN,
    ITUNER_DGAIN_SATURATION_LEVEL,
    ITUNER_LOCAL_EXPOSURE,
    ITUNER_COLOR_CORRECTION,
    ITUNER_TONE_CURVE,
    ITUNER_RGB_TO_YUV_MATRIX,
    ITUNER_CHROMA_SCALE,
    ITUNER_CHROMA_MEDIAN_FILTER,
    ITUNER_HISO_CHROMA_MEDIAN_FILTER,
    ITUNER_DEMOSAIC_FILTER,
    ITUNER_SHARPEN_BOTH,
    ITUNER_SHARPEN_NOISE,
    ITUNER_FIR,
    ITUNER_CORING,
    ITUNER_CORING_INDEX_SCALE,
    ITUNER_MIN_CORING_RESULT,
    ITUNER_SCALE_CORING,
    ITUNER_FINAL_SHARPEN_BOTH,
    ITUNER_FINAL_SHARPEN_NOISE,
    ITUNER_FINAL_FIR,
    ITUNER_FINAL_CORING,
    ITUNER_FINAL_CORING_INDEX_SCALE,
    ITUNER_FINAL_MIN_CORING_RESULT,
    ITUNER_FINAL_SCALE_CORING,
    ITUNER_HISO_DEMOSAIC_FILTER,
    ITUNER_HISO_HIGH_SHARPEN_BOTH,
    ITUNER_HISO_MED_SHARPEN_BOTH,
    ITUNER_HISO_HIGH_SHARPEN_NOISE,
    ITUNER_HISO_MED_SHARPEN_NOISE,
    ITUNER_HISO_HIGH_FIR,
    ITUNER_HISO_MED_FIR,
    ITUNER_HISO_HIGH_CORING,
    ITUNER_HISO_MED_CORING,
    ITUNER_HISO_HIGH_CORING_INDEX_SCALE,
    ITUNER_HISO_HIGH_MIN_CORING_RESULT,
    ITUNER_HISO_HIGH_SCALE_CORING,
    ITUNER_HISO_MED_CORING_INDEX_SCALE,
    ITUNER_HISO_MED_MIN_CORING_RESULT,
    ITUNER_HISO_MED_SCALE_CORING,
    ITUNER_VIDEO_MCTF,
    ITUNER_VIDEO_MCTF_TEMPORAL_ADJUST,
    ITUNER_CDNR,
    ITUNER_HISO_DEFER_COLOR_CORRECTION,
    ITUNER_SHP_A_OR_SPATIAL_FILTE_SELECT,
    ITUNER_ASF_INFO,
    ITUNER_HISO_ASF,
    ITUNER_HISO_HIGH_ASF,
    ITUNER_HISO_LOW_ASF,
    ITUNER_HISO_MED_ASF,
    ITUNER_HISO_CHROMA_ASF,
    ITUNER_CHROMA_FILTER,
    ITUNER_WIDE_CHROMA_FILTER,
    ITUNER_WIDE_CHROMA_FILTER_COMBINE,
    ITUNER_HISO_CHROMA_FILTER_HIGH,
    ITUNER_GB_GR_MISMATCH,
    ITUNER_HISO_GB_GR_MISMATCH,
    ITUNER_CHROMA_ABERRATION_INFO,
    ITUNER_CHROMA_ABERRATION_INFO_BY_PASS,
    ITUNER_HISO_CHROMA_FILTER_LOW_VERY_LOW,
    ITUNER_HISO_CHROMA_FILTER_PRE,
    ITUNER_HISO_CHROMA_FILTER_MED,
    ITUNER_HISO_CHROMA_FILTER_LOW,
    ITUNER_HISO_CHROMA_FILTER_VERY_LOW,
    ITUNER_HISO_CHROMA_FILTER_VERY_LOW_COMBINE,
    ITUNER_HISO_CHROMA_FILTER_MED_COMBINE,
    ITUNER_HISO_CHROMA_FILTER_LOW_COMBINE,
    ITUNER_HISO_LUMA_NOISE_COMBINE,
    ITUNER_HISO_LOW_ASF_COMBINE,
    ITUNER_HISO_FREQ_RECOVER,
    ITUNER_SENSOR_INFO,
    /*
    ITUNER_VHDR_AMP_LINEAR_HDR,
    ITUNER_VHDR_SE_STATIC_BLACK_LEVEL,
    */
    ITUNER_VHDR_ALPHA_CALC_CFG,
    ITUNER_VHDR_ALPHA_CALC_THRESH,
    ITUNER_VHDR_ALPHA_CALC_BLACK_LEVEL,
    ITUNER_VHDR_ALPHA_CALC_ALPHA,
    ITUNER_VHDR_AMP_LINEAR,
    ITUNER_VHDR_CONTRAST_ENHANCE,
    ITUNER_VHDR_RAW_INFO,
    ITUNER_VHDR_LE_STATIC_BLACK_LEVEL,

    ITUNER_MAX,
} ITUNER_VALID_s;

/*!@brief Specified Filter Valid Array*/
typedef UINT8 AMBA_ITUNER_VALID_FILTER_t[ITUNER_MAX];

/*!@brief Specified External File Type that Rawenc, Rawcpat or ituner need*/
typedef enum {
    EXT_FILE_FPN_MAP = 0,
    EXT_FILE_BYPASS_FPN_MAP,
    EXT_FILE_VIGNETTE_GAIN,
    EXT_FILE_BYPASS_VIGNETTE_GAIN,
    EXT_FILE_WARP_TABLE,
    EXT_FILE_BYPASS_WARP_HORIZONTAL_TABLE,
    EXT_FILE_BYPASS_WARP_VERTICAL_TABLE,
    EXT_FILE_CA_TABLE,
    EXT_FILE_BYPASS_CA_HORIZONTAL_TABLE,
    EXT_FILE_BYPASS_CA_VERTICAL_TABLE,
    EXT_FILE_CC_REG,
    EXT_FILE_CC_THREED,

    EXT_FILE_FIRST_SHARPEN,
    EXT_FILE_ASF,

    EXT_FILE_HISO_LOW_ASF,


    EXT_FILE_HISO_CHROMA_MED_COMBINE,
    EXT_FILE_HISO_CHROMA_VERYLOW_COMBINE,

    EXT_FILE_RAW,
    EXT_FILE_JPG,
    EXT_FILE_YUV,
    EXT_FILE_CFA_STAT,
    EXT_FILE_RGB_STAT,
    EXT_FILE_HDR_ALPHA_TABLE,//22
    EXT_FILE_HDR_LINEAR_TABLE,//23
    EXT_FILE_MAX,
} Ituner_Ext_File_Type_e;

typedef struct {
    void* Address;
    UINT32 Size;
} Load_Data_Info_s;

typedef struct {
    void* Address;
    UINT32 Size;
} Save_Data_Info_s;

typedef struct {
    UINT32* Address;
    UINT32* Size;
    UINT8 Index;
} Load_Multi_Table_Info_s;

typedef struct {
    void* Address;
    UINT32 Max_Size;
    UINT32 InputPitch;
} Load_Data_Info_Unknow_Size_s;

typedef struct {
    void *Address;
    UINT16 StartX;
    UINT16 StartY;
    UINT16 Width;
    UINT16 Height;
    UINT16 Pitch;
    char *Target_File_Path;
    UINT8 Reserved;
} Save_Data_to_Path_Info_s;

typedef struct {
    Save_Data_to_Path_Info_s Y_Info;
    Save_Data_to_Path_Info_s UV_Info;
} Save_YUV_Info_s;

typedef struct {
    void *Address;
    UINT32 Size;
} Save_Raw_Info_s;

typedef struct {
    void *Address;
    UINT32 Size;
    char *Target_File_Path;
} Save_JPG_Info_s;

typedef struct {
    void *Address;
    UINT32 Size;
    char *Target_File_Path;
} Save_STAT_Info_s;
typedef struct {
    UINT8 Index;
    void *Address;
    UINT32 Size;
} Load_Data_HDR_Alpha_Info_s;

typedef struct {
    UINT8 Index;
    void *Address[2];
    UINT32 Size[2];
} Load_Data_HDR_Linear_Info_s;

/*!@brief Specified the parameters that we need to use when load and save external files*/
typedef union {
    Load_Data_Info_Unknow_Size_s Raw_Load_Param;
    Save_Raw_Info_s Raw_Save_Param;
    Save_Data_Info_s FPN_MAP_Ext_Save_Param;
    Load_Data_Info_s FPN_MAP_Ext_Load_Param;
    Save_Data_Info_s Bypass_FPN_MAP_Ext_Save_Param;
    Load_Data_Info_s Bypass_FPN_MAP_Ext_Load_Param;
    Save_Data_Info_s Vignette_Gain_Save_Param;
    Load_Data_Info_s Vignette_Gain_Load_Param;
    Save_Data_Info_s Bypass_Vignette_Gain_Save_Param;
    Load_Data_Info_s Bypass_Vignette_Gain_Load_Param;
    Save_Data_Info_s Warp_Table_Save_Param;
    Load_Data_Info_s Warp_Table_Load_Param;
    Load_Data_Info_Unknow_Size_s Bypass_Warp_Horizontal_Table_Load_Param;
    Load_Data_Info_Unknow_Size_s Bypass_Warp_Vertical_Table_Load_Param;
    Save_Data_Info_s Ca_Table_Save_Param;
    Load_Data_Info_s Ca_Table_Load_Param;
    Load_Data_Info_Unknow_Size_s Bypass_Ca_Horizontal_Table_Load_Param;
    Load_Data_Info_Unknow_Size_s Bypass_Ca_Vertical_Table_Load_Param;
    Save_Data_Info_s CC_Reg_Save_Param;
    Load_Data_Info_s CC_Reg_Load_Param;
    Save_Data_Info_s CC_ThreeD_Save_Param;
    Load_Data_Info_s CC_ThreeD_Load_Param;

    Save_Data_Info_s FIRST_SHARPEN_Save_Param;
    Load_Data_Info_s FIRST_SHARPEN_Load_Param;
    Save_Data_Info_s FINAL_SHARPEN_Save_Param;
    Load_Data_Info_s FINAL_SHARPEN_Load_Param;
    Save_Data_Info_s ASF_Save_Param;
    Load_Data_Info_s ASF_Load_Param;

    Save_Data_Info_s HISO_ASF_Save_Param;
    Load_Data_Info_s HISO_ASF_Load_Param;
    Save_Data_Info_s HISO_HIGH_ASF_Save_Param;
    Load_Data_Info_s HISO_HIGH_ASF_Load_Param;
    Save_Data_Info_s HISO_LOW_ASF_Save_Param;
    Load_Data_Info_s HISO_LOW_ASF_Load_Param;
    Save_Data_Info_s HISO_MED_ASF_Save_Param;
    Load_Data_Info_s HISO_MED_ASF_Load_Param;
    Save_Data_Info_s HISO_LI2ND_ASF_Save_Param;
    Load_Data_Info_s HISO_LI2ND_ASF_Load_Param;
    Save_Data_Info_s HISO_CHROMA_ASF_Save_Param;
    Load_Data_Info_s HISO_CHROMA_ASF_Load_Param;

    Save_Data_Info_s HISO_HIGH_SHARPEN_Save_Param;
    Load_Data_Info_s HISO_HIGH_SHARPEN_Load_Param;
    Save_Data_Info_s HISO_MED_SHARPEN_Save_Param;
    Load_Data_Info_s HISO_MED_SHARPEN_Load_Param;
    Save_Data_Info_s HISO_LISO1_SHARPEN_Save_Param;
    Load_Data_Info_s HISO_LISO1_SHARPEN_Load_Param;
    Save_Data_Info_s HISO_LISO2_SHARPEN_Save_Param;
    Load_Data_Info_s HISO_LISO2_SHARPEN_Load_Param;

    Save_Data_Info_s HISO_CHROMA_MED_COMBINE_Save_Param;
    Load_Data_Info_s HISO_CHROMA_MED_COMBINE_Load_Param;
    Save_Data_Info_s HISO_CHROMA_LOW_COMBINE_Save_Param;
    Load_Data_Info_s HISO_CHROMA_LOW_COMBINE_Load_Param;
    Save_Data_Info_s HISO_CHROMA_VERYLOW_COMBINE_Save_Param;
    Load_Data_Info_s HISO_CHROMA_VERYLOW_COMBINE_Load_Param;
    Save_Data_Info_s HISO_LUMA_NOISE_COMBINE_Save_Param;
    Load_Data_Info_s HISO_LUMA_NOISE_COMBINE_Load_Param;
    Save_Data_Info_s HISO_LOW_ASF_COMBINE_Save_Param;
    Load_Data_Info_s HISO_LOW_ASF_COMBINE_Load_Param;
    Save_Data_Info_s HISO_COMBINE_Save_Param;
    Load_Data_Info_s HISO_COMBINE_Load_Param;

    Save_YUV_Info_s YUV_Save_Param;
    Save_JPG_Info_s JPG_Save_Param;
    Save_STAT_Info_s Stat_Save_Param;
    Load_Data_HDR_Alpha_Info_s HDR_AlphaTable_Load_Param;
    Load_Data_HDR_Linear_Info_s HDR_Linear_Load_Param;
} Ituner_Ext_File_Param_s;

/*!@brief Specified the call back function, we need to use when ituner load and ituner save*/
typedef struct {
    int (*Save_Data)(Ituner_Ext_File_Type_e Ext_File_Type, Ituner_Ext_File_Param_s* Ext_File_Param); /**!<Callback Function, use to save calib data, retun 0 means success*/
    int (*Load_Data)(Ituner_Ext_File_Type_e Ext_File_Type, Ituner_Ext_File_Param_s* Ext_File_Param); /**!<Callback Function, use to load calib data, retun 0 means success*/
} AmbaItuner_Hook_Func_t;

typedef struct{
    AmbaItuner_Hook_Func_t Hook_Func; //!< Specified the method of read / write calib data, detail please reference AmbaItuner_Hook_Func_t
    AMBA_KAL_BYTE_POOL_t *pMemPool;
} AmbaItuner_Config_t;

typedef struct _AMBA_ITUNER_PROC_INFO_s_{
    UINT32  HisoBatchId; //!< Specified the High iso batch id, only use in high iso case
} AMBA_ITUNER_PROC_INFO_s;

typedef struct{
    AMBA_KAL_BYTE_POOL_t *pMemPool;
} Ituner_Initial_Config_t;

/**
 * @brief AmbaItuner Initial
 * If you want to use any AmbaItuner_xxx(), please call this function first.
 *
 * @param[in] AmbaItuner_Config_t *Config: Setup call back function and other param, that tuning need.\n
 *                                         Detail please reference AmbaItuner_Confit_t
 *
 * @return 0 (Success) / -1 (Failure)
 */
int AmbaItuner_Init(AmbaItuner_Config_t *Config);

/**
 * @brief Refresh GData(The database, use to store config of the image kernel filter, U can use AmbaItuner_Get/Set_XXX() access GData)
 * This function will access the api of image kernel, to get current filter config, and store those config to GData.
 *
 * @param[in] AMBA_DSP_IMG_MODE_CFG_s Mode: set Pipe and AlgoMode
 *
 * @return 0 (Success) / -1 (Failure)
 */
int AmbaItuner_Refresh(AMBA_DSP_IMG_MODE_CFG_s *pMode);

/**
 * @brief Apply idsp configure of GData to ucode
 * This function will apply a part of the idsp config that filter status is valid to ucode.\n
 * Please make sure the config of GData was already updated.
 *
 * @param[in] AMBA_DSP_IMG_MODE_CFG_s Mode: set Pipe and AlgoMode
 * @param[in] AMBA_ITUNER_PROC_INFO_s *ProcInfo: set high iso batch id
 *
 * @return 0 (Success) / -1 (Failure)
 */
int AmbaItuner_Execute(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_ITUNER_PROC_INFO_s *ProcInfo);

/**
 * @brief Set SystemInfo of GData
 * This function will replace SystemInfo of GData, and set ITUNER_SYSTEM_INFO filter status is valid.\n
 * If you want to update a part of SystemInfo, you should call AmbaItuner_Get_SystemInfo() to get current SystemInfo before the replace process.
 *
 * @param[in] ITUNER_SYSTEM_s *System: System Info Config. Detail please reference ITUNER_SYSTEM_s.
 *
 * @return None
 */
void AmbaItuner_Set_SystemInfo(ITUNER_SYSTEM_s *System);

/**
 * @brief Get SystemInfo of GData
 * This function will copy SystemInfo of GData.
 *
 * @param[out] ITUNER_SYSTEM_s *System: System Info Config. Detail please reference ITUNER_SYSTEM_s.
 *
 * @return None
 */
void AmbaItuner_Get_SystemInfo(ITUNER_SYSTEM_s *System);

/**
 * @brief Set InternalInfo of GData
 * This function will replace InternalInfo of GData, and set ITUNER_INTERNAL_INFO filter status is valid.\n
 * If you want to update a part of InternalInfo, you should call AmbaItuner_Get_InternalInfo() to get current InternalInfo before the replace process.
 *
 * @param[in] ITUNER_Internal_s *Internal: Internal Info Config. Detail please reference ITUNER_Internal_s.
 *
 * @return None
 */
void AmbaItuner_Set_InternalInfo(Ituner_Internal_s *Internal);

/**
 * @brief Get InternalInfo of GData
 * This function will copy InternalInfo of GData.
 *
 * @param[out] Ituner_Internal_s *Internal: Internal Info Config. Detail please reference Ituner_Internal_s.
 *
 * @return None
 */
void AmbaItuner_Get_InternalInfo(Ituner_Internal_s *Internal);

/**
 * @brief Set AaaFunctionInfo of GData
 * This function will replace AaaFunctionInfo of GData, and set ITUNER_AAA_FUNCTION filter status is valid.\n
 * If you want to update a part of AaaFunctionInfo, you should call AmbaItuner_Get_AAAFunction() to get current AaaFunctionInfo before the replace process.
 *
 * @param[in] AAA_FUNC_s *AaaFunction: AaaFunction Info Config. Detail please reference AAA_FUNC_s.
 *
 * @return None
 */
void AmbaItuner_Set_AAAFunction(AAA_FUNC_s *AaaFunction);

/**
 * @brief Get AaaFunctionInfo of GData
 * This function will copy AaaFunctionInfo of GData.
 *
 * @param[out] AAA_FUNC_s *AaaFunction: AaaFunction Info Config. Detail please reference AAA_FUNC_s.
 *
 * @return None
 */
void AmbaItuner_Get_AAAFunction(AAA_FUNC_s *AaaFunction);

/**
 * @brief Set AeInfo of GData
 * This function will replace AeInfo of GData, and set ITUNER_AE_INFO is valid.\n
 * If you want to update a part of AeInfo, you should call AmbaItuner_Get_AeInfo() to get current AeInfo before the replace process.
 *
 * @param[in] ITUNER_AE_INFO_s *AeInfo: AeInfo Config. Detail please reference ITUNER_AE_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_AeInfo(ITUNER_AE_INFO_s *AeInfo);

/**
 * @brief Get AeInfo of GData
 * This function will copy AeInfo of GData.
 *
 * @param[out] ITUNER_AE_INFO_s *AeInfo: AeInfo Config. Detail please reference ITUNER_AE_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_AeInfo(ITUNER_AE_INFO_s *AeInfo);

/**
 * @brief Set WbSimInfo of GData
 * This function will replace WbSimInfo of GData, and set ITUNER_WB_SIM_INFO is valid.\n
 * If you want to update a part of WbSimInfo, you should call AmbaItuner_Get_WbSimInfo() to get current AeInfo before the replace process.
 *
 * @param[in] ITUNER_WB_SIM_INFO_s *WbSimInfo: WbSimInfo Config. Detail please reference ITUNER_WB_SIM_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_WbSimInfo(ITUNER_WB_SIM_INFO_s *WbSimInfo);

/**
 * @brief Get WbSimInfo of GData
 * This function will copy WbSimInfo of GData.
 *
 * @param[out] ITUNER_WB_SIM_INFO_s *WbSimInfo: WbSimInfo Config. Detail please reference ITUNER_WB_SIM_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_WbSimInfo(ITUNER_WB_SIM_INFO_s *WbSimInfo);

/**
 * @brief Set AeStatisticsInfo of GData
 * This function will replace AeStatisticsInfo of GData, and set ITUNER_AE_STATISTICS_INFO is valid.\n
 * If you want to update a part of AeStatisticsInfo, you should call AmbaItuner_Get_AeStatisticsInfo() to get current AeStatisticsInfo before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_AE_STAT_INFO_s *AeStatisticsInfo: AeStatisticsInfo Config. Detail please reference AMBA_DSP_IMG_AE_STAT_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_AeStatisticsInfo(AMBA_DSP_IMG_AE_STAT_INFO_s *AeStatisticsInfo);

/**
 * @brief Get AeStatisticsInfo of GData
 * This function will copy AeStatisticsInfo of GData.
 *
 * @param[out] AMBA_DSP_IMG_AE_STAT_INFO_s *AeStatisticsInfo: AeStatisticsInfo Config. Detail please reference AMBA_DSP_IMG_AE_STAT_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_AeStatisticsInfo(AMBA_DSP_IMG_AE_STAT_INFO_s *AeStatisticsInfo);

/**
 * @brief Set AwbStatisticsInfo of GData
 * This function will replace AwbStatisticsInfo of GData, and set ITUNER_AWB_STATISTICS_INFO is valid.\n
 * If you want to update a part of AwbStatisticsInfo, you should call AmbaItuner_Get_AwbStatisticsInfo() to get current AwbStatisticsInfo before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_AWB_STAT_INFO_s *AwbStatisticsInfo: AwbStatisticsInfo Config. Detail please reference AMBA_DSP_IMG_AWB_STAT_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_AwbStatisticsInfo(AMBA_DSP_IMG_AWB_STAT_INFO_s *AwbStatisticsInfo);

/**
 * @brief Get AwbStatisticsInfo of GData
 * This function will copy AwbStatisticsInfo of GData.
 *
 * @param[out] AMBA_DSP_IMG_AWB_STAT_INFO_s *AwbStatisticsInfo: AwbStatisticsInfo Config. Detail please reference AMBA_DSP_IMG_AWB_STAT_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_AwbStatisticsInfo(AMBA_DSP_IMG_AWB_STAT_INFO_s *AwbStatisticsInfo);

/**
 * @brief Set AfStatisticsInfo of GData
 * This function will replace AfStatisticsInfo of GData, and set ITUNER_AF_STATISTICS_INFO is valid.\n
 * If you want to update a part of AfStatisticsInfo, you should call AmbaItuner_Get_AfStatisticsInfo() to get current AfStatisticsInfo before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_AF_STAT_INFO_s *AfStatisticsInfo: AfStatisticsInfo Config. Detail please reference AMBA_DSP_IMG_AF_STAT_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_AfStatisticsInfo(AMBA_DSP_IMG_AF_STAT_INFO_s *AfStatisticsInfo);

/**
 * @brief Get AfStatisticsInfo of GData
 * This function will copy AfStatisticsInfo of GData.
 *
 * @param[out] AMBA_DSP_IMG_AF_STAT_INFO_s *AfStatisticsInfo: AfStatisticsInfo Config. Detail please reference AMBA_DSP_IMG_AF_STAT_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_AfStatisticsInfo(AMBA_DSP_IMG_AF_STAT_INFO_s *AfStatisticsInfo);

/**
 * @brief Set StaticBadPixelCorrection of GData
 * This function will replace StaticBadPixelCorrection of GData, and set ITUNER_STATIC_BAD_PIXEL_CORRECTION is valid.\n
 * If you want to update a part of StaticBadPixelCorrection, you should call AmbaItuner_Get_StaticBadPixelCorrection() to get current StaticBadPixelCorrection before the replace process.
 *
 * @param[in] ITUNER_FPN_s *StaticBadPixelCorrection: StaticBadPixelCorrection Config. Detail please reference ITUNER_FPN_s.
 *
 * @return None
 */
void AmbaItuner_Set_StaticBadPixelCorrection(ITUNER_FPN_s *StaticBadPixelCorrection);

/**
 * @brief Get StaticBadPixelCorrection of GData
 * This function will copy StaticBadPixelCorrection of GData.
 *
 * @param[out] ITUNER_FPN_s *StaticBadPixelCorrection: StaticBadPixelCorrection Config. Detail please reference ITUNER_FPN_s.
 *
 * @return None
 */
void AmbaItuner_Get_StaticBadPixelCorrection(ITUNER_FPN_s *StaticBadPixelCorrection);

/**
 * @brief Get FPN Map
 * This function will copy FPN Map of GData.(Read only)
 *
 * @param[out] AMBA_DSP_IMG_SBP_CORRECTION_s *SbpCorr: FPN Map, It's read only, please don't edit it.
 *
 * @return None
 */
void AmbaItuner_Get_SbpCorr(AMBA_DSP_IMG_SBP_CORRECTION_s *SbpCorr);

/**
 * @brief Set StaticBadPixelCorrectionByPass of GData
 * This function will replace StaticBadPixelCorrectionByPass of GData, and set ITUNER_STATIC_BAD_PIXEL_CORRECTION_BY_PASS is valid.\n
 * If you want to update a part of StaticBadPixelCorrectionByPass, you should call AmbaItuner_Get_StaticBadPixelCorrectionByPass() to get current StaticBadPixelCorrectionByPass before the replace process.
 *
 * @param[in] ITUNER_FPN_BYPASS_s *StaticBadPixelCorrectionByPass: StaticBadPixelCorrectionByPass Config. Detail please reference ITUNER_FPN_BYPASS_s.
 *
 * @return None
 */
void AmbaItuner_Set_StaticBadPixelCorrectionByPass(ITUNER_FPN_BYPASS_s *StaticBadPixelCorrectionByPass);

/**
 * @brief Get StaticBadPixelCorrectionByPass of GData
 * This function will copy StaticBadPixelCorrectionByPass of GData.
 *
 * @param[out] ITUNER_FPN_BYPASS_s *StaticBadPixelCorrectionByPass: StaticBadPixelCorrectionByPass Config. Detail please reference ITUNER_FPN_BYPASS_s.
 *
 * @return None
 */
void AmbaItuner_Get_StaticBadPixelCorrectionByPass(ITUNER_FPN_BYPASS_s *StaticBadPixelCorrectionByPass);

/**
 * @brief Set VignetteCompensation of GData
 * This function will replace VignetteCompensation of GData, and set ITUNER_VIGNETTE_COMPENSATION is valid.\n
 * If you want to update a part of VignetteCompensation, you should call AmbaItuner_Get_VignetteCompensation() to get current VignetteCompensation before the replace process.
 *
 * @param[in] ITUNER_VIGNETTE_s *VignetteCompensation: VignetteCompensation Config. Detail please reference ITUNER_VIGNETTE_s.
 *
 * @return None
 */
void AmbaItuner_Set_VignetteCompensation(ITUNER_VIGNETTE_s *VignetteCompensation);

/**
 * @brief Get VignetteCompensation of GData
 * This function will copy VignetteCompensation of GData.
 *
 * @param[out] ITUNER_VIGNETTE_s *VignetteCompensation: VignetteCompensation Config. Detail please reference ITUNER_VIGNETTE_s.
 *
 * @return None
 */
void AmbaItuner_Get_VignetteCompensation(ITUNER_VIGNETTE_s *VignetteCompensation);

/**
 * @brief Get Vignette Table
 * This function will copy Vignette Table of GData.(Read only)
 *
 * @param[out] AMBA_DSP_IMG_VIGNETTE_CALC_INFO_s *VignetteCalcInfo: Vignette Table, It's read only, please don't edit it.
 *
 * @return None
 */
void AmbaItuner_Get_VignetteCalcInfo(AMBA_DSP_IMG_VIGNETTE_CALC_INFO_s *VignetteCalcInfo);

/**
 * @brief Set VignetteCompensationByPass of GData
 * This function will replace VignetteCompensationByPass of GData, and set ITUNER_VIGNETTE_COMPENSATION_BY_PASS is valid.\n
 * If you want to update a part of VignetteCompensationByPass, you should call AmbaItuner_Get_VignetteCompensationByPass() to get current VignetteCompensationByPass before the replace process.
 *
 * @param[in] ITUNER_VIGNETTE_BYPASS_s *VignetteCompensationByPass: VignetteCompensationByPass Config. Detail please reference ITUNER_VIGNETTE_BYPASS_s.
 *
 * @return None
 */
void AmbaItuner_Set_VignetteCompensationByPass(ITUNER_VIGNETTE_BYPASS_s *VignetteCompensationByPass);

/**
 * @brief Get VignetteCompensationByPass of GData
 * This function will copy VignetteCompensationByPass of GData.
 *
 * @param[out] ITUNER_VIGNETTE_BYPASS_s *VignetteCompensationByPass: VignetteCompensationByPass Config. Detail please reference ITUNER_VIGNETTE_BYPASS_s.
 *
 * @return None
 */
void AmbaItuner_Get_VignetteCompensationByPass(ITUNER_VIGNETTE_BYPASS_s *VignetteCompensationByPass);

/**
 * @brief Set WarpCompensation of GData
 * This function will replace WarpCompensation of GData, and set ITUNER_WARP_COMPENSATION is valid.\n
 * If you want to update a part of WarpCompensation, you should call AmbaItuner_Get_WarpCompensation() to get current WarpCompensation before the replace process.
 *
 * @param[in] ITUNER_WARP_s *WarpCompensation: WarpCompensation Config. Detail please reference ITUNER_WARP_s.
 *
 * @return None
 */
void AmbaItuner_Set_WarpCompensation(ITUNER_WARP_s *WarpCompensation);

/**
 * @brief Get WarpCompensation of GData
 * This function will copy WarpCompensation of GData.
 *
 * @param[out] ITUNER_WARP_s *WarpCompensation: WarpCompensation Config. Detail please reference ITUNER_WARP_s.
 *
 * @return None
 */
void AmbaItuner_Get_WarpCompensation(ITUNER_WARP_s *WarpCompensation);

/**
 * @brief Get Warp Table
 * This function will copy Warp Table of GData.(Read only)
 *
 * @param[out] AMBA_DSP_IMG_WARP_CALC_INFO_s *WarpCalcInfo: Warp Table, It's read only, please don't edit it.
 *
 * @return None
 */
void AmbaItuner_Get_WarpCalcInfo(AMBA_DSP_IMG_WARP_CALC_INFO_s *WarpCalcInfo);

/**
 * @brief Set Dzoom of GData
 * This function will replace Dzoom of GData, and set ITUNER_DZOOM is valid.\n
 * If you want to update a part of Dzoom, you should call AmbaItuner_Get_Dzoom() to get current Dzoom before the replace process.
 *
 * @param[in] ITUNER_DZOOM_s *Dzoom: Dzoom Config. Detail please reference ITUNER_DZOOM_s.
 *
 * @return None
 */
void AmbaItuner_Set_Dzoom(ITUNER_DZOOM_s *Dzoom);

/**
 * @brief Get Dzoom of GData
 * This function will copy Dzoom of GData.
 *
 * @param[out] ITUNER_DZOOM_s *Dzoom: Dzoom Config. Detail please reference ITUNER_DZOOM_s.
 *
 * @return None
 */
void AmbaItuner_Get_Dzoom(ITUNER_DZOOM_s *Dzoom);

/**
 * @brief Set WarpCompensationDzoomByPass of GData
 * This function will replace WarpCompensationDzoomByPass of GData, and set ITUNER_WARP_COMPENSATION_DZOOM_BY_PASS is valid.\n
 * If you want to update a part of WarpCompensationDzoomByPass, you should call AmbaItuner_Get_WarpCompensationDzoomByPass() to get current WarpCompensationDzoomByPass before the replace process.
 *
 * @param[in] ITUNER_WARP_DZOOM_BYPASS_s *WarpCompensationDzoomByPass: WarpCompensationDzoomByPass Config. Detail please reference ITUNER_WARP_DZOOM_BYPASS_s.
 *
 * @return None
 */
void AmbaItuner_Set_WarpCompensationDzoomByPass(ITUNER_WARP_DZOOM_BYPASS_s *WarpCompensationDzoomByPass);

/**
 * @brief Get WarpCompensationDzoomByPass of GData
 * This function will copy WarpCompensationDzoomByPass of GData.
 *
 * @param[out] ITUNER_WARP_DZOOM_BYPASS_s *WarpCompensationDzoomByPass: WarpCompensationDzoomByPass Config. Detail please reference ITUNER_WARP_DZOOM_BYPASS_s.
 *
 * @return None
 */
void AmbaItuner_Get_WarpCompensationDzoomByPass(ITUNER_WARP_DZOOM_BYPASS_s *WarpCompensationDzoomByPass);

/**
 * @brief Set SensorInputInfo of GData
 * This function will replace SensorInputInfo of GData, and set ITUNER_SENSOR_INPUT_INFO is valid.\n
 * If you want to update a part of SensorInputInfo, you should call AmbaItuner_Get_SensorInputInfo() to get current SensorInputInfo before the replace process.
 *
 * @param[in] ITUNER_SENSOR_INPUT_s *SensorInputInfo: SensorInputInfo Config. Detail please reference ITUNER_SENSOR_INPUT_s.
 *
 * @return None
 */
void AmbaItuner_Set_SensorInputInfo(ITUNER_SENSOR_INPUT_s *SensorInputInfo);

/**
 * @brief Get SensorInputInfo of GData
 * This function will copy SensorInputInfo of GData.
 *
 * @param[out] ITUNER_SENSOR_INPUT_s *SensorInputInfo: SensorInputInfo Config. Detail please reference ITUNER_SENSOR_INPUT_s.
 *
 * @return None
 */
void AmbaItuner_Get_SensorInputInfo(ITUNER_SENSOR_INPUT_s *SensorInputInfo);

/**
 * @brief Set StaticBlackLevel of GData
 * This function will replace StaticBlackLevel of GData, and set ITUNER_STATIC_BLACK_LEVEL is valid.\n
 * If you want to update a part of StaticBlackLevel, you should call AmbaItuner_Get_StaticBlackLevel() to get current StaticBlackLevel before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_BLACK_CORRECTION_s *StaticBlackLevel: StaticBlackLevel Config. Detail please reference AMBA_DSP_IMG_BLACK_CORRECTION_s.
 *
 * @return None
 */
void AmbaItuner_Set_StaticBlackLevel(AMBA_DSP_IMG_BLACK_CORRECTION_s *StaticBlackLevel);

/**
 * @brief Get StaticBlackLevel of GData
 * This function will copy StaticBlackLevel of GData.
 *
 * @param[out] AMBA_DSP_IMG_BLACK_CORRECTION_s *StaticBlackLevel: StaticBlackLevel Config. Detail please reference AMBA_DSP_IMG_BLACK_CORRECTION_s.
 *
 * @return None
 */
void AmbaItuner_Get_StaticBlackLevel(AMBA_DSP_IMG_BLACK_CORRECTION_s *StaticBlackLevel);

/**
 * @brief Set DeferredBlackLevel of GData
 * This function will replace DeferredBlackLevel of GData, and set ITUNER_DEFERRED_BLACK_LEVEL is valid.\n
 * If you want to update a part of DeferredBlackLevel, you should call AmbaItuner_Get_DeferredBlackLevel() to get current DeferredBlackLevel before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_DEF_BLC_s *DeferredBlackLevel: DeferredBlackLevel Config. Detail please reference AMBA_DSP_IMG_DEF_BLC_s.
 *
 * @return None
 */
void AmbaItuner_Set_DeferredBlackLevel(AMBA_DSP_IMG_DEF_BLC_s *DeferredBlackLevel);

/**
 * @brief Get DeferredBlackLevel of GData
 * This function will copy DeferredBlackLevel of GData.
 *
 * @param[out] AMBA_DSP_IMG_DEF_BLC_s *DeferredBlackLevel: DeferredBlackLevel Config. Detail please reference AMBA_DSP_IMG_DEF_BLC_s.
 *
 * @return None
 */
void AmbaItuner_Get_DeferredBlackLevel(AMBA_DSP_IMG_DEF_BLC_s *DeferredBlackLevel);

/**
 * @brief Set DynamicBadPixelCorrection of GData
 * This function will replace DynamicBadPixelCorrection of GData, and set ITUNER_DYNAMIC_BAD_PIXEL_CORRECTION is valid.\n
 * If you want to update a part of DynamicBadPixelCorrection, you should call AmbaItuner_Get_DynamicBadPixelCorrection() to get current DynamicBadPixelCorrection before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_DBP_CORRECTION_s *DynamicBadPixelCorrection: DynamicBadPixelCorrection Config. Detail please reference AMBA_DSP_IMG_DBP_CORRECTION_s.
 *
 * @return None
 */
void AmbaItuner_Set_DynamicBadPixelCorrection(AMBA_DSP_IMG_DBP_CORRECTION_s *DynamicBadPixelCorrection);

/**
 * @brief Get DynamicBadPixelCorrection of GData
 * This function will copy DynamicBadPixelCorrection of GData.
 *
 * @param[out] AMBA_DSP_IMG_DBP_CORRECTION_s *DynamicBadPixelCorrection: DynamicBadPixelCorrection Config. Detail please reference AMBA_DSP_IMG_DBP_CORRECTION_s.
 *
 * @return None
 */
void AmbaItuner_Get_DynamicBadPixelCorrection(AMBA_DSP_IMG_DBP_CORRECTION_s *DynamicBadPixelCorrection);

/**
 * @brief Set HisoDynamicBadPixelCorrection of GData
 * This function will replace HisoDynamicBadPixelCorrection of GData, and set ITUNER_HISO_DYNAMIC_BAD_PIXEL_CORRECTION is valid.\n
 * If you want to update a part of HisoDynamicBadPixelCorrection, you should call AmbaItuner_Get_HisoDynamicBadPixelCorrection() to get current HisoDynamicBadPixelCorrection before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_DBP_CORRECTION_s *HisoDynamicBadPixelCorrection: HisoDynamicBadPixelCorrection Config. Detail please reference AMBA_DSP_IMG_DBP_CORRECTION_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoDynamicBadPixelCorrection(AMBA_DSP_IMG_DBP_CORRECTION_s *HisoDynamicBadPixelCorrection);

/**
 * @brief Get HisoDynamicBadPixelCorrection of GData
 * This function will copy HisoDynamicBadPixelCorrection of GData.
 *
 * @param[out] AMBA_DSP_IMG_DBP_CORRECTION_s *HisoDynamicBadPixelCorrection: HisoDynamicBadPixelCorrection Config. Detail please reference AMBA_DSP_IMG_DBP_CORRECTION_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoDynamicBadPixelCorrection(AMBA_DSP_IMG_DBP_CORRECTION_s *HisoDynamicBadPixelCorrection);

/**
 * @brief Set CfaLeakageFilter of GData
 * This function will replace CfaLeakageFilter of GData, and set ITUNER_CFA_LEAKAGE_FILTER is valid.\n
 * If you want to update a part of CfaLeakageFilter, you should call AmbaItuner_Get_CfaLeakageFilter() to get current CfaLeakageFilter before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *CfaLeakageFilter: CfaLeakageFilter Config. Detail please reference AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_CfaLeakageFilter(AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *CfaLeakageFilter);

/**
 * @brief Get CfaLeakageFilter of GData
 * This function will copy CfaLeakageFilter of GData.
 *
 * @param[out] AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *CfaLeakageFilter: CfaLeakageFilter Config. Detail please reference AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_CfaLeakageFilter(AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *CfaLeakageFilter);

/**
 * @brief Set HisoCfaLeakageFilter of GData
 * This function will replace HisoCfaLeakageFilter of GData, and set ITUNER_HISO_CFA_LEAKAGE_FILTER is valid.\n
 * If you want to update a part of HisoCfaLeakageFilter, you should call AmbaItuner_Get_HisoCfaLeakageFilter() to get current HisoCfaLeakageFilter before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *HisoCfaLeakageFilter: HisoCfaLeakageFilter Config. Detail please reference AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoCfaLeakageFilter(AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *HisoCfaLeakageFilter);

/**
 * @brief Get HisoCfaLeakageFilter of GData
 * This function will copy HisoCfaLeakageFilter of GData.
 *
 * @param[out] AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *HisoCfaLeakageFilter: HisoCfaLeakageFilter Config. Detail please reference AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoCfaLeakageFilter(AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *HisoCfaLeakageFilter);

/**
 * @brief Set CfaNoiseFilter of GData
 * This function will replace CfaNoiseFilter of GData, and set ITUNER_CFA_NOISE_FILTER is valid.\n
 * If you want to update a part of CfaNoiseFilter, you should call AmbaItuner_Get_CfaNoiseFilter() to get current CfaNoiseFilter before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CFA_NOISE_FILTER_s *CfaNoiseFilter: CfaNoiseFilter Config. Detail please reference AMBA_DSP_IMG_CFA_NOISE_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_CfaNoiseFilter(AMBA_DSP_IMG_CFA_NOISE_FILTER_s *CfaNoiseFilter);

/**
 * @brief Get CfaNoiseFilter of GData
 * This function will copy CfaNoiseFilter of GData.
 *
 * @param[out] AMBA_DSP_IMG_CFA_NOISE_FILTER_s *CfaNoiseFilter: CfaNoiseFilter Config. Detail please reference AMBA_DSP_IMG_CFA_NOISE_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_CfaNoiseFilter(AMBA_DSP_IMG_CFA_NOISE_FILTER_s *CfaNoiseFilter);

/**
 * @brief Set HisoCfaNoiseFilter of GData
 * This function will replace HisoCfaNoiseFilter of GData, and set ITUNER_HISO_CFA_NOISE_FILTER is valid.\n
 * If you want to update a part of HisoCfaNoiseFilter, you should call AmbaItuner_Get_HisoCfaNoiseFilter() to get current HisoCfaNoiseFilter before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CFA_NOISE_FILTER_s *HisoCfaNoiseFilter: HisoCfaNoiseFilter Config. Detail please reference AMBA_DSP_IMG_CFA_NOISE_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoCfaNoiseFilter(AMBA_DSP_IMG_CFA_NOISE_FILTER_s *HisoCfaNoiseFilter);

/**
 * @brief Get HisoCfaNoiseFilter of GData
 * This function will copy HisoCfaNoiseFilter of GData.
 *
 * @param[out] AMBA_DSP_IMG_CFA_NOISE_FILTER_s *HisoCfaNoiseFilter: HisoCfaNoiseFilter Config. Detail please reference AMBA_DSP_IMG_CFA_NOISE_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoCfaNoiseFilter(AMBA_DSP_IMG_CFA_NOISE_FILTER_s *HisoCfaNoiseFilter);

/**
 * @brief Set AntiAliasingStrength of GData
 * This function will replace AntiAliasingStrength of GData, and set ITUNER_ANTI_ALIASING_STRENGTH is valid.\n
 * If you want to update a part of AntiAliasingStrength, you should call AmbaItuner_Get_AntiAliasingStrength() to get current AntiAliasingStrength before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_ANTI_ALIASING_s *AntiAliasing: AntiAliasing Config.
 *
 * @return None
 */
void AmbaItuner_Set_AntiAliasingStrength(AMBA_DSP_IMG_ANTI_ALIASING_s *AntiAliasing);

/**
 * @brief Get AntiAliasingStrength of GData
 * This function will copy AntiAliasingStrength of GData.
 *
 * @param[out] AMBA_DSP_IMG_ANTI_ALIASING_s *AntiAliasing: AntiAliasing Config.
 *
 * @return None
 */
void AmbaItuner_Get_AntiAliasingStrength(AMBA_DSP_IMG_ANTI_ALIASING_s *AntiAliasing);

/**
 * @brief Set HisoAntiAliasingStrength of GData
 * This function will replace HisoAntiAliasingStrength of GData, and set ITUNER_HISO_ANTI_ALIASING_STRENGTH is valid.\n
 * If you want to update a part of HisoAntiAliasingStrength, you should call AmbaItuner_Get_HisoAntiAliasingStrength() to get current HisoAntiAliasingStrength before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_ANTI_ALIASING_s HisoAntiAliasing: HisoAntiAliasing Config.
 *
 * @return None
 */
void AmbaItuner_Set_HisoAntiAliasingStrength(AMBA_DSP_IMG_ANTI_ALIASING_s *HisoAntiAliasing);


/**
 * @brief Get HisoAntiAliasingStrength of GData
 * This function will copy HisoAntiAliasingStrength of GData.
 *
 * @param[out] AMBA_DSP_IMG_ANTI_ALIASING_s *HisoAntiAliasing: HisoAntiAliasing Config.
 *
 * @return None
 */
void AmbaItuner_Get_HisoAntiAliasingStrength(AMBA_DSP_IMG_ANTI_ALIASING_s *HisoAntiAliasing);

/**
 * @brief Set GlobalDgain of GData
 * This function will replace GlobalDgain of GData, and set ITUNER_GLOBAL_DGAIN is valid.\n
 * If you want to update a part of GlobalDgain, you should call AmbaItuner_Get_GlobalDgain() to get current GlobalDgain before the replace process.
 *
 * @param[in] UINT32 GlobalDgain: GlobalDgain Config.
 *
 * @return None
 */
void AmbaItuner_Set_GlobalDgain(UINT32 GlobalDgain);

/**
 * @brief Get GlobalDgain of GData
 * This function will copy GlobalDgain of GData.
 *
 * @param[out] UINT32 GlobalDgain: GlobalDgain Config.
 *
 * @return None
 */
void AmbaItuner_Get_GlobalDgain(UINT32 *GlobalDgain);

/**
 * @brief Set WbGain of GData
 * This function will replace WbGain of GData, and set ITUNER_WB_GAIN is valid.\n
 * If you want to update a part of WbGain, you should call AmbaItuner_Get_WbGain() to get current WbGain before the replace process.
 *
 * @param[in] : WbGain Config. Detail please reference AMBA_DSP_IMG_WB_GAIN_s.
 *
 * @return None
 */
void AmbaItuner_Set_WbGain(AMBA_DSP_IMG_WB_GAIN_s *WbGain);

/**
 * @brief Get WbGain of GData
 * This function will copy WbGain of GData.
 *
 * @param[out] AMBA_DSP_IMG_WB_GAIN_s *WbGain: WbGain Config. Detail please reference AMBA_DSP_IMG_WB_GAIN_s.
 *
 * @return None
 */
void AmbaItuner_Get_WbGain(AMBA_DSP_IMG_WB_GAIN_s *WbGain);

/**
 * @brief Set DgainSaturationLevel of GData
 * This function will replace DgainSaturationLevel of GData, and set ITUNER_DGAIN_SATURATION_LEVEL is valid.\n
 * If you want to update a part of DgainSaturationLevel, you should call AmbaItuner_Get_DgainSaturationLevel() to get current DgainSaturationLevel before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_DGAIN_SATURATION_s *DgainSaturationLevel: DgainSaturationLevel Config. Detail please reference AMBA_DSP_IMG_DGAIN_SATURATION_s.
 *
 * @return None
 */
void AmbaItuner_Set_DgainSaturationLevel(AMBA_DSP_IMG_DGAIN_SATURATION_s *DgainSaturationLevel);

/**
 * @brief Get DgainSaturationLevel of GData
 * This function will copy DgainSaturationLevel of GData.
 *
 * @param[out] AMBA_DSP_IMG_DGAIN_SATURATION_s *DgainSaturationLevel: DgainSaturationLevel Config. Detail please reference AMBA_DSP_IMG_DGAIN_SATURATION_s.
 *
 * @return None
 */
void AmbaItuner_Get_DgainSaturationLevel(AMBA_DSP_IMG_DGAIN_SATURATION_s *DgainSaturationLevel);

/**
 * @brief Set LocalExposure of GData
 * This function will replace LocalExposure of GData, and set ITUNER_LOCAL_EXPOSURE is valid.\n
 * If you want to update a part of LocalExposure, you should call AmbaItuner_Get_LocalExposure() to get current LocalExposure before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LOCAL_EXPOSURE_s *LocalExposure: LocalExposure Config. Detail please reference AMBA_DSP_IMG_LOCAL_EXPOSURE_s.
 *
 * @return None
 */
void AmbaItuner_Set_LocalExposure(AMBA_DSP_IMG_LOCAL_EXPOSURE_s *LocalExposure);

/**
 * @brief Get LocalExposure of GData
 * This function will copy LocalExposure of GData.
 *
 * @param[out] AMBA_DSP_IMG_LOCAL_EXPOSURE_s *LocalExposure: LocalExposure Config. Detail please reference AMBA_DSP_IMG_LOCAL_EXPOSURE_s.
 *
 * @return None
 */
void AmbaItuner_Get_LocalExposure(AMBA_DSP_IMG_LOCAL_EXPOSURE_s *LocalExposure);

/**
 * @brief Set ColorCorrection of GData
 * This function will replace ColorCorrection of GData, and set ITUNER_COLOR_CORRECTION is valid.\n
 * If you want to update a part of ColorCorrection, you should call AmbaItuner_Get_ColorCorrection() to get current ColorCorrection before the replace process.
 *
 * @param[in] : ColorCorrection Config. Detail please reference ITUNER_COLOR_CORRECTION_s.
 *
 * @return None
 */
void AmbaItuner_Set_ColorCorrection(ITUNER_COLOR_CORRECTION_s *ColorCorrection);

/**
 * @brief Get ColorCorrection of GData
 * This function will copy ColorCorrection of GData.
 *
 * @param[out] ITUNER_COLOR_CORRECTION_s *ColorCorrection: ColorCorrection Config. Detail please reference ITUNER_COLOR_CORRECTION_s.
 *
 * @return None
 */
void AmbaItuner_Get_ColorCorrection(ITUNER_COLOR_CORRECTION_s *ColorCorrection);

/**
 * @brief Get Filter Status
 * This function will report that which filters are valid.
 *
 * @param[out] AMBA_ITUNER_VALID_FILTER_t *FilterStatus: Filter Status Array, the index relationship please reference ITUNER_VALID_s
 *
 * @return None
 */
void AmbaItuner_Get_FilterStatus(AMBA_ITUNER_VALID_FILTER_t *FilterStatus);

/**
 * @brief Get CcReg Table
 * This function will copy CcReg Table of GData.(Read only)
 *
 * @param[out] AMBA_DSP_IMG_COLOR_CORRECTION_REG_s *CcReg: CcReg Table, It's read only, please don't edit it.
 *
 * @return None
 */
void AmbaItuner_Get_CcReg(AMBA_DSP_IMG_COLOR_CORRECTION_REG_s *CcReg);

/**
 * @brief Get CcThreeD Table
 * This function will copy CcThreeD Table of GData.(Read only)
 *
 * @param[out] AMBA_DSP_IMG_COLOR_CORRECTION_s *CcThreeD: CcThreeD Table, It's read only, please don't edit it.
 *
 * @return None
 */
void AmbaItuner_Get_CcThreeD(AMBA_DSP_IMG_COLOR_CORRECTION_s *CcThreeD);

/**
 * @brief Set ToneCurve of GData
 * This function will replace ToneCurve of GData, and set ITUNER_TONE_CURVE is valid.\n
 * If you want to update a part of ToneCurve, you should call AmbaItuner_Get_ToneCurve() to get current ToneCurve before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_TONE_CURVE_s *ToneCurve: ToneCurve Config. Detail please reference AMBA_DSP_IMG_TONE_CURVE_s.
 *
 * @return None
 */
void AmbaItuner_Set_ToneCurve(AMBA_DSP_IMG_TONE_CURVE_s *ToneCurve);

/**
 * @brief Get ToneCurve of GData
 * This function will copy ToneCurve of GData.
 *
 * @param[out] AMBA_DSP_IMG_TONE_CURVE_s *ToneCurve: ToneCurve Config. Detail please reference AMBA_DSP_IMG_TONE_CURVE_s.
 *
 * @return None
 */
void AmbaItuner_Get_ToneCurve(AMBA_DSP_IMG_TONE_CURVE_s *ToneCurve);

/**
 * @brief Set RgbToYuvMatrix of GData
 * This function will replace RgbToYuvMatrix of GData, and set ITUNER_RGB_TO_YUV_MATRIX is valid.\n
 * If you want to update a part of RgbToYuvMatrix, you should call AmbaItuner_Get_RgbToYuvMatrix() to get current RgbToYuvMatrix before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_RGB_TO_YUV_s *RgbToYuvMatrix: RgbToYuvMatrix Config. Detail please reference AMBA_DSP_IMG_RGB_TO_YUV_s.
 *
 * @return None
 */
void AmbaItuner_Set_RgbToYuvMatrix(AMBA_DSP_IMG_RGB_TO_YUV_s *RgbToYuvMatrix);

/**
 * @brief Get RgbToYuvMatrix of GData
 * This function will copy RgbToYuvMatrix of GData.
 *
 * @param[out] AMBA_DSP_IMG_RGB_TO_YUV_s *RgbToYuvMatrix: RgbToYuvMatrix Config. Detail please reference AMBA_DSP_IMG_RGB_TO_YUV_s.
 *
 * @return None
 */
void AmbaItuner_Get_RgbToYuvMatrix(AMBA_DSP_IMG_RGB_TO_YUV_s *RgbToYuvMatrix);

/**
 * @brief Set ChromaScale of GData
 * This function will replace ChromaScale of GData, and set ITUNER_CHROMA_SCALE is valid.\n
 * If you want to update a part of ChromaScale, you should call AmbaItuner_Get_ChromaScale() to get current ChromaScale before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CHROMA_SCALE_s *ChromaScale: ChromaScale Config. Detail please reference AMBA_DSP_IMG_CHROMA_SCALE_s.
 *
 * @return None
 */
void AmbaItuner_Set_ChromaScale(AMBA_DSP_IMG_CHROMA_SCALE_s *ChromaScale);

/**
 * @brief Get ChromaScale of GData
 * This function will copy ChromaScale of GData.
 *
 * @param[out] AMBA_DSP_IMG_CHROMA_SCALE_s *ChromaScale: ChromaScale Config. Detail please reference AMBA_DSP_IMG_CHROMA_SCALE_s.
 *
 * @return None
 */
void AmbaItuner_Get_ChromaScale(AMBA_DSP_IMG_CHROMA_SCALE_s *ChromaScale);

/**
 * @brief Set ChromaMedianFilter of GData
 * This function will replace ChromaMedianFilter of GData, and set ITUNER_CHROMA_MEDIAN_FILTER is valid.\n
 * If you want to update a part of ChromaMedianFilter, you should call AmbaItuner_Get_ChromaMedianFilter() to get current ChromaMedianFilter before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *ChromaMedianFilter: ChromaMedianFilter Config. Detail please reference AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_ChromaMedianFilter(AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *ChromaMedianFilter);

/**
 * @brief Get ChromaMedianFilter of GData
 * This function will copy ChromaMedianFilter of GData.
 *
 * @param[out] AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *ChromaMedianFilter: ChromaMedianFilter Config. Detail please reference AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_ChromaMedianFilter(AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *ChromaMedianFilter);

/**
 * @brief Set HisoChromaMedianFilter of GData
 * This function will replace HisoChromaMedianFilter of GData, and set ITUNER_HISO_CHROMA_MEDIAN_FILTER is valid.\n
 * If you want to update a part of HisoChromaMedianFilter, you should call AmbaItuner_Get_HisoChromaMedianFilter() to get current HisoChromaMedianFilter before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *HisoChromaMedianFilter: HisoChromaMedianFilter Config. Detail please reference AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoChromaMedianFilter(AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *HisoChromaMedianFilter);

/**
 * @brief Get HisoChromaMedianFilter of GData
 * This function will copy HisoChromaMedianFilter of GData.
 *
 * @param[out] AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *HisoChromaMedianFilter: HisoChromaMedianFilter Config. Detail please reference AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoChromaMedianFilter(AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *HisoChromaMedianFilter);

/**
 * @brief Set DemosaicFilter of GData
 * This function will replace DemosaicFilter of GData, and set ITUNER_DEMOSAIC_FILTER is valid.\n
 * If you want to update a part of DemosaicFilter, you should call AmbaItuner_Get_DemosaicFilter() to get current DemosaicFilter before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_DEMOSAIC_s *DemosaicFilter: DemosaicFilter Config. Detail please reference AMBA_DSP_IMG_DEMOSAIC_s.
 *
 * @return None
 */
void AmbaItuner_Set_DemosaicFilter(AMBA_DSP_IMG_DEMOSAIC_s *DemosaicFilter);

/**
 * @brief Get DemosaicFilter of GData
 * This function will copy DemosaicFilter of GData.
 *
 * @param[out] AMBA_DSP_IMG_DEMOSAIC_s *DemosaicFilter: DemosaicFilter Config. Detail please reference AMBA_DSP_IMG_DEMOSAIC_s.
 *
 * @return None
 */
void AmbaItuner_Get_DemosaicFilter(AMBA_DSP_IMG_DEMOSAIC_s *DemosaicFilter);

/**
 * @brief Set HisoDemosaicFilter of GData
 * This function will replace HisoDemosaicFilter of GData, and set ITUNER_HISO_DEMOSAIC_FILTER is valid.\n
 * If you want to update a part of HisoDemosaicFilter, you should call AmbaItuner_Get_HisoDemosaicFilter() to get current HisoDemosaicFilter before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_DEMOSAIC_s *HisoDemosaicFilter: HisoDemosaicFilter Config. Detail please reference AMBA_DSP_IMG_DEMOSAIC_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoDemosaicFilter(AMBA_DSP_IMG_DEMOSAIC_s *HisoDemosaicFilter);

/**
 * @brief Get HisoDemosaicFilter of GData
 * This function will copy HisoDemosaicFilter of GData.
 *
 * @param[out] AMBA_DSP_IMG_DEMOSAIC_s *HisoDemosaicFilter: HisoDemosaicFilter Config. Detail please reference AMBA_DSP_IMG_DEMOSAIC_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoDemosaicFilter(AMBA_DSP_IMG_DEMOSAIC_s *HisoDemosaicFilter);

/**
 * @brief Set SharpenBoth of GData
 * This function will replace SharpenBoth of GData, and set ITUNER_SHARPEN_BOTH is valid.\n
 * If you want to update a part of SharpenBoth, you should call AmbaItuner_Get_SharpenBoth() to get current SharpenBoth before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_SHARPEN_BOTH_s *SharpenBoth: SharpenBoth Config. Detail please reference AMBA_DSP_IMG_SHARPEN_BOTH_s.
 *
 * @return None
 */
void AmbaItuner_Set_SharpenBoth(ITUNER_SHARPEN_BOTH_s *SharpenBoth);

/**
 * @brief Get SharpenBoth of GData
 * This function will copy SharpenBoth of GData.
 *
 * @param[out] AMBA_DSP_IMG_SHARPEN_BOTH_s *SharpenBoth: SharpenBoth Config. Detail please reference AMBA_DSP_IMG_SHARPEN_BOTH_s.
 *
 * @return None
 */
void AmbaItuner_Get_SharpenBoth(ITUNER_SHARPEN_BOTH_s *SharpenBoth);

/**
 * @brief Set FinalSharpenBoth of GData
 * This function will replace FinalSharpenBoth of GData, and set ITUNER_FINAL_SHARPEN_BOTH is valid.\n
 * If you want to update a part of SharpenBoth, you should call AmbaItuner_Get_FinalSharpenBoth() to get current FinalSharpenBoth before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_SHARPEN_BOTH_s *SharpenBoth: FinalSharpenBoth Config. Detail please reference AMBA_DSP_IMG_SHARPEN_BOTH_s.
 *
 * @return None
 */
void AmbaItuner_Set_FinalSharpenBoth(ITUNER_SHARPEN_BOTH_s *FinalSharpenBoth);

/**
 * @brief Get FinalSharpenBoth of GData
 * This function will copy FinalSharpenBoth of GData.
 *
 * @param[out] AMBA_DSP_IMG_SHARPEN_BOTH_s *SharpenBoth: FinalSharpenBoth Config. Detail please reference AMBA_DSP_IMG_SHARPEN_BOTH_s.
 *
 * @return None
 */
void AmbaItuner_Get_FinalSharpenBoth(ITUNER_SHARPEN_BOTH_s *FinalSharpenBoth);

/**
 * @brief Set HisoHighSharpenBoth of GData
 * This function will replace HisoHighSharpenBoth of GData, and set ITUNER_HISO_HIGH_SHARPEN_BOTH is valid.\n
 * If you want to update a part of HisoHighSharpenBoth, you should call AmbaItuner_Get_HisoHighSharpenBoth() to get current HisoHighSharpenBoth before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_SHARPEN_BOTH_s *HisoHighSharpenBoth: HisoHighSharpenBoth Config. Detail please reference AMBA_DSP_IMG_SHARPEN_BOTH_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoHighSharpenBoth(ITUNER_SHARPEN_BOTH_s *HisoHighSharpenBoth);

/**
 * @brief Get HisoHighSharpenBoth of GData
 * This function will copy HisoHighSharpenBoth of GData.
 *
 * @param[out] AMBA_DSP_IMG_SHARPEN_BOTH_s *HisoHighSharpenBoth: HisoHighSharpenBoth Config. Detail please reference AMBA_DSP_IMG_SHARPEN_BOTH_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoHighSharpenBoth(ITUNER_SHARPEN_BOTH_s *HisoHighSharpenBoth);

/**
 * @brief Set HisoMedSharpenBoth of GData
 * This function will replace HisoMedSharpenBoth of GData, and set ITUNER_HISO_MED_SHARPEN_BOTH is valid.\n
 * If you want to update a part of HisoMedSharpenBoth, you should call AmbaItuner_Get_HisoMedSharpenBoth() to get current HisoMedSharpenBoth before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_SHARPEN_BOTH_s *HisoMedSharpenBoth: HisoMedSharpenBoth Config. Detail please reference AMBA_DSP_IMG_SHARPEN_BOTH_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoMedSharpenBoth(ITUNER_SHARPEN_BOTH_s *HisoMedSharpenBoth);

/**
 * @brief Get HisoMedSharpenBoth of GData
 * This function will copy HisoMedSharpenBoth of GData.
 *
 * @param[out] AMBA_DSP_IMG_SHARPEN_BOTH_s *HisoMedSharpenBoth: HisoMedSharpenBoth Config. Detail please reference AMBA_DSP_IMG_SHARPEN_BOTH_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoMedSharpenBoth(ITUNER_SHARPEN_BOTH_s *HisoMedSharpenBoth);

/**
 * @brief Set SharpenNoise of GData
 * This function will replace SharpenNoise of GData, and set ITUNER_SHARPEN_NOISE is valid.\n
 * If you want to update a part of SharpenNoise, you should call AmbaItuner_Get_SharpenNoise() to get current SharpenNoise before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_SHARPEN_NOISE_s *SharpenNoise: SharpenNoise Config. Detail please reference AMBA_DSP_IMG_SHARPEN_NOISE_s.
 *
 * @return None
 */
void AmbaItuner_Set_SharpenNoise(AMBA_DSP_IMG_SHARPEN_NOISE_s *SharpenNoise);

/**
 * @brief Get SharpenNoise of GData
 * This function will copy SharpenNoise of GData.
 *
 * @param[out] AMBA_DSP_IMG_SHARPEN_NOISE_s *SharpenNoise: SharpenNoise Config. Detail please reference AMBA_DSP_IMG_SHARPEN_NOISE_s.
 *
 * @return None
 */
void AmbaItuner_Get_FinalSharpenNoise(AMBA_DSP_IMG_SHARPEN_NOISE_s *FinalSharpenNoise);

/**
 * @brief Set FinalSharpenNoise of GData
 * This function will replace FinalSharpenNoise of GData, and set ITUNER_FINAL_SHARPEN_NOISE is valid.\n
 * If you want to update a part of FinalSharpenNoise, you should call AmbaItuner_Get_FinalSharpenNoise() to get current FinalSharpenNoise before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_SHARPEN_NOISE_s *FinalSharpenNoise: FinalSharpenNoise Config. Detail please reference AMBA_DSP_IMG_SHARPEN_NOISE_s.
 *
 * @return None
 */
void AmbaItuner_Set_FinalSharpenNoise(AMBA_DSP_IMG_SHARPEN_NOISE_s *FinalSharpenNoise);

/**
 * @brief Get FinalSharpenNoise of GData
 * This function will copy FinalSharpenNoise of GData.
 *
 * @param[out] AMBA_DSP_IMG_SHARPEN_NOISE_s *FinalSharpenNoise: FinalSharpenNoise Config. Detail please reference AMBA_DSP_IMG_SHARPEN_NOISE_s.
 *
 * @return None
 */
void AmbaItuner_Get_SharpenNoise(AMBA_DSP_IMG_SHARPEN_NOISE_s *SharpenNoise);

/**
 * @brief Set HisoHighSharpenNoise of GData
 * This function will replace HisoHighSharpenNoise of GData, and set ITUNER_HISO_HIGH_SHARPEN_NOISE is valid.\n
 * If you want to update a part of HisoHighSharpenNoise, you should call AmbaItuner_Get_HisoHighSharpenNoise() to get current HisoHighSharpenNoise before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_SHARPEN_NOISE_s *HisoHighSharpenNoise: HisoHighSharpenNoise Config. Detail please reference AMBA_DSP_IMG_SHARPEN_NOISE_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoHighSharpenNoise(AMBA_DSP_IMG_SHARPEN_NOISE_s *HisoHighSharpenNoise);

/**
 * @brief Get HisoHighSharpenNoise of GData
 * This function will copy HisoHighSharpenNoise of GData.
 *
 * @param[out] AMBA_DSP_IMG_SHARPEN_NOISE_s *HisoHighSharpenNoise: HisoHighSharpenNoise Config. Detail please reference AMBA_DSP_IMG_SHARPEN_NOISE_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoHighSharpenNoise(AMBA_DSP_IMG_SHARPEN_NOISE_s *HisoHighSharpenNoise);

/**
 * @brief Set HisoMedSharpenNoise of GData
 * This function will replace HisoMedSharpenNoise of GData, and set ITUNER_HISO_MED_SHARPEN_NOISE is valid.\n
 * If you want to update a part of HisoMedSharpenNoise, you should call AmbaItuner_Get_HisoMedSharpenNoise() to get current HisoMedSharpenNoise before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_SHARPEN_NOISE_s *HisoMedSharpenNoise: HisoMedSharpenNoise Config. Detail please reference AMBA_DSP_IMG_SHARPEN_NOISE_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoMedSharpenNoise(AMBA_DSP_IMG_SHARPEN_NOISE_s *HisoMedSharpenNoise);

/**
 * @brief Get HisoMedSharpenNoise of GData
 * This function will copy HisoMedSharpenNoise of GData.
 *
 * @param[out] AMBA_DSP_IMG_SHARPEN_NOISE_s *HisoMedSharpenNoise: HisoMedSharpenNoise Config. Detail please reference AMBA_DSP_IMG_SHARPEN_NOISE_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoMedSharpenNoise(AMBA_DSP_IMG_SHARPEN_NOISE_s *HisoMedSharpenNoise);

/**
 * @brief Set Fir of GData
 * This function will replace Fir of GData, and set ITUNER_FIR is valid.\n
 * If you want to update a part of Fir, you should call AmbaItuner_Get_Fir() to get current Fir before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_FIR_s *Fir: Fir Config. Detail please reference AMBA_DSP_IMG_FIR_s.
 *
 * @return None
 */
void AmbaItuner_Set_Fir(AMBA_DSP_IMG_FIR_s *Fir);

/**
 * @brief Get Fir of GData
 * This function will copy Fir of GData.
 *
 * @param[out] AMBA_DSP_IMG_FIR_s *Fir: Fir Config. Detail please reference AMBA_DSP_IMG_FIR_s.
 *
 * @return None
 */
void AmbaItuner_Get_Fir(AMBA_DSP_IMG_FIR_s *Fir);

/**
 * @brief Set FinalFir of GData
 * This function will replace FinalFir of GData, and set ITUNER_FINAL_FIR is valid.\n
 * If you want to update a part of FinalFir, you should call AmbaItuner_Get_FinalFir() to get current Fir before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_FIR_s *FinalFir: FinalFir Config. Detail please reference AMBA_DSP_IMG_FIR_s.
 *
 * @return None
 */
void AmbaItuner_Set_FinalFir(AMBA_DSP_IMG_FIR_s *FinalFir);

/**
 * @brief Get FinalFir of GData
 * This function will copy FinalFir of GData.
 *
 * @param[out] AMBA_DSP_IMG_FIR_s *FinalFir: FinalFir Config. Detail please reference AMBA_DSP_IMG_FIR_s.
 *
 * @return None
 */
void AmbaItuner_Get_FinalFir(AMBA_DSP_IMG_FIR_s *FinalFir);

/**
 * @brief Set HisoHighFir of GData
 * This function will replace HisoHighFir of GData, and set ITUNER_HISO_HIGH_FIR is valid.\n
 * If you want to update a part of HisoHighFir, you should call AmbaItuner_Get_HisoHighFir() to get current HisoHighFir before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_FIR_s *HisoHighFir: HisoHighFir Config. Detail please reference AMBA_DSP_IMG_FIR_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoHighFir(AMBA_DSP_IMG_FIR_s *HisoHighFir);

/**
 * @brief Get HisoHighFir of GData
 * This function will copy HisoHighFir of GData.
 *
 * @param[out] AMBA_DSP_IMG_FIR_s *HisoHighFir: HisoHighFir Config. Detail please reference AMBA_DSP_IMG_FIR_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoHighFir(AMBA_DSP_IMG_FIR_s *HisoHighFir);

/**
 * @brief Set HisoMedFir of GData
 * This function will replace HisoMedFir of GData, and set ITUNER_HISO_MED_FIR is valid.\n
 * If you want to update a part of HisoMedFir, you should call AmbaItuner_Get_HisoMedFir() to get current HisoMedFir before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_FIR_s *HisoMedFir: HisoMedFir Config. Detail please reference AMBA_DSP_IMG_FIR_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoMedFir(AMBA_DSP_IMG_FIR_s *HisoMedFir);

/**
 * @brief Get HisoMedFir of GData
 * This function will copy HisoMedFir of GData.
 *
 * @param[out] AMBA_DSP_IMG_FIR_s *HisoMedFir: HisoMedFir Config. Detail please reference AMBA_DSP_IMG_FIR_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoMedFir(AMBA_DSP_IMG_FIR_s *HisoMedFir);

/**
 * @brief Set Coring of GData
 * This function will replace Coring of GData, and set ITUNER_CORING is valid.\n
 * If you want to update a part of Coring, you should call AmbaItuner_Get_Coring() to get current Coring before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CORING_s *Coring: Coring Config. Detail please reference AMBA_DSP_IMG_CORING_s.
 *
 * @return None
 */
void AmbaItuner_Set_Coring(AMBA_DSP_IMG_CORING_s *Coring);

/**
 * @brief Get Coring of GData
 * This function will copy Coring of GData.
 *
 * @param[out] AMBA_DSP_IMG_CORING_s *Coring: Coring Config. Detail please reference AMBA_DSP_IMG_CORING_s.
 *
 * @return None
 */
void AmbaItuner_Get_Coring(AMBA_DSP_IMG_CORING_s *Coring);

/**
 * @brief Set FinalCoring of GData
 * This function will replace FinalCoring of GData, and set ITUNER_FINAL_CORING is valid.\n
 * If you want to update a part of FinalCoring, you should call AmbaItuner_Get_FinalCoring() to get current FinalCoring before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CORING_s *FinalCoring: FinalCoring Config. Detail please reference AMBA_DSP_IMG_CORING_s.
 *
 * @return None
 */
void AmbaItuner_Set_FinalCoring(AMBA_DSP_IMG_CORING_s *FinalCoring);

/**
 * @brief GetFinal Coring of GData
 * This function will copy FinalCoring of GData.
 *
 * @param[out] AMBA_DSP_IMG_CORING_s *FinalCoring: FinalCoring Config. Detail please reference AMBA_DSP_IMG_CORING_s.
 *
 * @return None
 */
void AmbaItuner_Get_FinalCoring(AMBA_DSP_IMG_CORING_s *FinalCoring);

/**
 * @brief Set HisoHighCoring of GData
 * This function will replace HisoHighCoring of GData, and set ITUNER_HISO_HIGH_CORING is valid.\n
 * If you want to update a part of HisoHighCoring, you should call AmbaItuner_Get_HisoHighCoring() to get current HisoHighCoring before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CORING_s *HisoHighCoring: HisoHighCoring Config. Detail please reference AMBA_DSP_IMG_CORING_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoHighCoring(AMBA_DSP_IMG_CORING_s *HisoHighCoring);

/**
 * @brief Get HisoHighCoring of GData
 * This function will copy HisoHighCoring of GData.
 *
 * @param[out] AMBA_DSP_IMG_CORING_s *HisoHighCoring: HisoHighCoring Config. Detail please reference AMBA_DSP_IMG_CORING_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoHighCoring(AMBA_DSP_IMG_CORING_s *HisoHighCoring);

/**
 * @brief Set HisoMedCoring of GData
 * This function will replace HisoMedCoring of GData, and set ITUNER_HISO_MED_CORING is valid.\n
 * If you want to update a part of HisoMedCoring, you should call AmbaItuner_Get_HisoMedCoring() to get current HisoMedCoring before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CORING_s *HisoMedCoring: HisoMedCoring Config. Detail please reference AMBA_DSP_IMG_CORING_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoMedCoring(AMBA_DSP_IMG_CORING_s *HisoMedCoring);

/**
 * @brief Get HisoMedCoring of GData
 * This function will copy HisoMedCoring of GData.
 *
 * @param[out] AMBA_DSP_IMG_CORING_s *HisoMedCoring: HisoMedCoring Config. Detail please reference AMBA_DSP_IMG_CORING_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoMedCoring(AMBA_DSP_IMG_CORING_s *HisoMedCoring);

/**
 * @brief Set CoringIndexScale of GData
 * This function will replace CoringIndexScale of GData, and set ITUNER_CORING_INDEX_SCALE is valid.\n
 * If you want to update a part of CoringIndexScale, you should call AmbaItuner_Get_CoringIndexScale() to get current CoringIndexScale before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *CoringIndexScale: CoringIndexScale Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_CoringIndexScale(AMBA_DSP_IMG_LEVEL_s *CoringIndexScale);

/**
 * @brief Get CoringIndexScale of GData
 * This function will copy CoringIndexScale of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *CoringIndexScale: CoringIndexScale Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_CoringIndexScale(AMBA_DSP_IMG_LEVEL_s *CoringIndexScale);

/**
 * @brief Set MinCoringResult of GData
 * This function will replace MinCoringResult of GData, and set ITUNER_MIN_CORING_RESULT is valid.\n
 * If you want to update a part of MinCoringResult, you should call AmbaItuner_Get_MinCoringResult() to get current MinCoringResult before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *MinCoringResult: MinCoringResult Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_MinCoringResult(AMBA_DSP_IMG_LEVEL_s *MinCoringResult);

/**
 * @brief Get MinCoringResult of GData
 * This function will copy MinCoringResult of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *MinCoringResult: MinCoringResult Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_MinCoringResult(AMBA_DSP_IMG_LEVEL_s *MinCoringResult);

/**
 * @brief Set FinalCoringIndexScale of GData
 * This function will replace FinalCoringIndexScale of GData, and set ITUNER_FINAL_CORING_INDEX_SCALE is valid.\n
 * If you want to update a part of FinalCoringIndexScale, you should call AmbaItuner_Get_FinalCoringIndexScale() to get current FinalCoringIndexScale before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *FinalCoringIndexScale: FinalCoringIndexScale Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_FinalCoringIndexScale(AMBA_DSP_IMG_LEVEL_s *FinalCoringIndexScale);

/**
 * @brief Get FinalCoringIndexScale of GData
 * This function will copy FinalCoringIndexScale of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *FinalCoringIndexScale: FinalCoringIndexScale Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_FinalCoringIndexScale(AMBA_DSP_IMG_LEVEL_s *FinalCoringIndexScale);

/**
 * @brief Set FinalMinCoringResult of GData
 * This function will replace FinalMinCoringResult of GData, and set ITUNER_FINAL_MIN_CORING_RESULT is valid.\n
 * If you want to update a part of FinalMinCoringResult, you should call AmbaItuner_Get_FinalMinCoringResult() to get current FinalMinCoringResult before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *FinalMinCoringResult: FinalMinCoringResult Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_FinalMinCoringResult(AMBA_DSP_IMG_LEVEL_s *FinalMinCoringResult);

/**
 * @brief Get FinalMinCoringResult of GData
 * This function will copy FinalMinCoringResult of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *FinalMinCoringResult: FinalMinCoringResult Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_FinalMinCoringResult(AMBA_DSP_IMG_LEVEL_s *FinalMinCoringResult);

/**
 * @brief Set HisoHighCoringIndexScale of GData
 * This function will replace HisoHighCoringIndexScale of GData, and set ITUNER_HISO_HIGH_CORING_INDEX_SCALE is valid.\n
 * If you want to update a part of HisoHighCoringIndexScale, you should call AmbaItuner_Get_HisoHighCoringIndexScale() to get current HisoHighCoringIndexScale before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *HisoHighCoringIndexScale: HisoHighCoringIndexScale Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoHighCoringIndexScale(AMBA_DSP_IMG_LEVEL_s *HisoHighCoringIndexScale);

/**
 * @brief Get HisoHighCoringIndexScale of GData
 * This function will copy HisoHighCoringIndexScale of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *HisoHighCoringIndexScale: HisoHighCoringIndexScale Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoHighCoringIndexScale(AMBA_DSP_IMG_LEVEL_s *HisoHighCoringIndexScale);

/**
 * @brief Set HisoHighMinCoringResult of GData
 * This function will replace HisoHighMinCoringResult of GData, and set ITUNER_HISO_HIGH_MIN_CORING_RESULT is valid.\n
 * If you want to update a part of HisoHighMinCoringResult, you should call AmbaItuner_Get_HisoHighMinCoringResult() to get current HisoHighMinCoringResult before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *HisoHighMinCoringResult: HisoHighMinCoringResult Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoHighMinCoringResult(AMBA_DSP_IMG_LEVEL_s *HisoHighMinCoringResult);

/**
 * @brief Get HisoHighMinCoringResult of GData
 * This function will copy HisoHighMinCoringResult of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *HisoHighMinCoringResult: HisoHighMinCoringResult Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoHighMinCoringResult(AMBA_DSP_IMG_LEVEL_s *HisoHighMinCoringResult);

/**
 * @brief Set HisoHighScaleCoring of GData
 * This function will replace HisoHighScaleCoring of GData, and set ITUNER_HISO_HIGH_SCALE_CORING is valid.\n
 * If you want to update a part of HisoHighScaleCoring, you should call AmbaItuner_Get_HisoHighScaleCoring() to get current HisoHighScaleCoring before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *HisoHighScaleCoring: HisoHighScaleCoring Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoHighScaleCoring(AMBA_DSP_IMG_LEVEL_s *HisoHighScaleCoring);

/**
 * @brief Get HisoHighScaleCoring of GData
 * This function will copy HisoHighScaleCoring of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *HisoHighScaleCoring: HisoHighScaleCoring Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoHighScaleCoring(AMBA_DSP_IMG_LEVEL_s *HisoHighScaleCoring);

/**
 * @brief Set HisoMedCoringIndexScale of GData
 * This function will replace HisoMedCoringIndexScale of GData, and set ITUNER_HISO_MED_CORING_INDEX_SCALE is valid.\n
 * If you want to update a part of HisoMedCoringIndexScale, you should call AmbaItuner_Get_HisoMedCoringIndexScale() to get current HisoMedCoringIndexScale before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *HisoMedCoringIndexScale: HisoMedCoringIndexScale Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoMedCoringIndexScale(AMBA_DSP_IMG_LEVEL_s *HisoMedCoringIndexScale);

/**
 * @brief Get HisoMedCoringIndexScale of GData
 * This function will copy HisoMedCoringIndexScale of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *HisoMedCoringIndexScale: HisoMedCoringIndexScale Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoMedCoringIndexScale(AMBA_DSP_IMG_LEVEL_s *HisoMedCoringIndexScale);

/**
 * @brief Set HisoMedMinCoringResult of GData
 * This function will replace HisoMedMinCoringResult of GData, and set ITUNER_HISO_MED_MIN_CORING_RESULT is valid.\n
 * If you want to update a part of HisoMedMinCoringResult, you should call AmbaItuner_Get_HisoMedMinCoringResult() to get current HisoMedMinCoringResult before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *HisoMedMinCoringResult: HisoMedMinCoringResult Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoMedMinCoringResult(AMBA_DSP_IMG_LEVEL_s *HisoMedMinCoringResult);

/**
 * @brief Get HisoMedMinCoringResult of GData
 * This function will copy HisoMedMinCoringResult of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *HisoMedMinCoringResult: HisoMedMinCoringResult Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoMedMinCoringResult(AMBA_DSP_IMG_LEVEL_s *HisoMedMinCoringResult);

/**
 * @brief Set HisoMedScaleCoring of GData
 * This function will replace HisoMedScaleCoring of GData, and set ITUNER_HISO_MED_SCALE_CORING is valid.\n
 * If you want to update a part of HisoMedScaleCoring, you should call AmbaItuner_Get_HisoMedScaleCoring() to get current HisoMedScaleCoring before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *HisoMedScaleCoring: HisoMedScaleCoring Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoMedScaleCoring(AMBA_DSP_IMG_LEVEL_s *HisoMedScaleCoring);

/**
 * @brief Get HisoMedScaleCoring of GData
 * This function will copy HisoMedScaleCoring of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *HisoMedScaleCoring: HisoMedScaleCoring Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoMedScaleCoring(AMBA_DSP_IMG_LEVEL_s *HisoMedScaleCoring);

/**
 * @brief Set ScaleCoring of GData
 * This function will replace ScaleCoring of GData, and set ITUNER_SCALE_CORING is valid.\n
 * If you want to update a part of ScaleCoring, you should call AmbaItuner_Get_ScaleCoring() to get current ScaleCoring before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *ScaleCoring: ScaleCoring Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_ScaleCoring(AMBA_DSP_IMG_LEVEL_s *ScaleCoring);

/**
 * @brief Get ScaleCoring of GData
 * This function will copy ScaleCoring of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *ScaleCoring: ScaleCoring Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_ScaleCoring(AMBA_DSP_IMG_LEVEL_s *ScaleCoring);

/**
 * @brief Set FinalScaleCoring of GData
 * This function will replace FinalScaleCoring of GData, and set ITUNER_FINAL_SCALE_CORING is valid.\n
 * If you want to update a part of FinalScaleCoring, you should call AmbaItuner_Get_FinalScaleCoring() to get current FinalScaleCoring before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_LEVEL_s *FinalScaleCoring: FinalScaleCoring Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Set_FinalScaleCoring(AMBA_DSP_IMG_LEVEL_s *FinalScaleCoring);

/**
 * @brief Get FinalScaleCoring of GData
 * This function will copy FinalScaleCoring of GData.
 *
 * @param[out] AMBA_DSP_IMG_LEVEL_s *FinalScaleCoring: FinalScaleCoring Config. Detail please reference AMBA_DSP_IMG_LEVEL_s.
 *
 * @return None
 */
void AmbaItuner_Get_FinalScaleCoring(AMBA_DSP_IMG_LEVEL_s *FinalScaleCoring);

/**
 * @brief Set VideoMctf of GData
 * This function will replace VideoMctf of GData, and set ITUNER_VIDEO_MCTF is valid.\n
 * If you want to update a part of VideoMctf, you should call AmbaItuner_Get_VideoMctf() to get current VideoMctf before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_VIDEO_MCTF_INFO_s *VideoMctf: VideoMctf Config. Detail please reference AMBA_DSP_IMG_VIDEO_MCTF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_VideoMctf(AMBA_DSP_IMG_VIDEO_MCTF_INFO_s *VideoMctf);

/**
 * @brief Get VideoMctf of GData
 * This function will copy VideoMctf of GData.
 *
 * @param[out] AMBA_DSP_IMG_VIDEO_MCTF_INFO_s *VideoMctf: VideoMctf Config. Detail please reference AMBA_DSP_IMG_VIDEO_MCTF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_VideoMctf(AMBA_DSP_IMG_VIDEO_MCTF_INFO_s *VideoMctf);

/**
 * @brief Set VideoMctfTemporalAdjust of GData
 * This function will replace VideoMctfTemporalAdjust of GData, and set ITUNER_VIDEO_MCTF_TEMPORAL_ADJUST is valid.\n
 * If you want to update a part of VideoMctfTemporalAdjust, you should call AmbaItuner_Get_VideoMctfTemporalAdjust() to get current VideoMctfTemporalAdjust before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_VIDEO_MCTF_TEMPORAL_ADJUST_s *VideoMctfTemporalAdjust: VideoMctfTA Config. Detail please reference AMBA_DSP_IMG_VIDEO_MCTF_TEMPORAL_ADJUST_s.
 *
 * @return None
 */
void AmbaItuner_Set_VideoMctfTemporalAdjust(AMBA_DSP_IMG_VIDEO_MCTF_TEMPORAL_ADJUST_s *VideoMctfTemporalAdjust);

/**
 * @brief Get VideoMctfTemporalAdjust of GData
 * This function will copy VideoMctfTemporalAdjust of GData.
 *
 * @param[out] AMBA_DSP_IMG_VIDEO_MCTF_TEMPORAL_ADJUST_s *VideoMctfTemporalAdjust: VideoMctfTA Config. Detail please reference AMBA_DSP_IMG_VIDEO_MCTF_TEMPORAL_ADJUST_s.
 *
 * @return None
 */
void AmbaItuner_Get_VideoMctfTemporalAdjust(AMBA_DSP_IMG_VIDEO_MCTF_TEMPORAL_ADJUST_s *VideoMctfTemporalAdjust);

/**
 * @brief Set CDNR of GData
 * This function will replace CDNR of GData, and set ITUNER_CDNR is valid.\n
 * If you want to update a part of CDNR, you should call AmbaItuner_Get_CDNR() to get current CDNR before the replace process.
 *
 * @param[in] : CDNR Config. Detail please reference AMBA_DSP_IMG_CDNR_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_CDNR(AMBA_DSP_IMG_CDNR_INFO_s *CDNR);

/**
 * @brief Get CDNR of GData
 * This function will copy CDNR of GData.
 *
 * @param[out] AMBA_DSP_IMG_CDNR_INFO_s *CDNR: CDNR Config. Detail please reference AMBA_DSP_IMG_CDNR_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_CDNR(AMBA_DSP_IMG_CDNR_INFO_s *CDNR);

/**
 * @brief Set HisoDeferColorCorrection of GData
 * This function will replace HisoDeferColorCorrection of GData, and set ITUNER_HISO_DEFER_COLOR_CORRECTION is valid.\n
 * If you want to update a part of HisoDeferColorCorrection, you should call AmbaItuner_Get_HisoDeferColorCorrection() to get current HisoDeferColorCorrection before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s *HisoDeferColorCorrection: HisoDeferColorCorrection Config. Detail please reference AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoDeferColorCorrection(AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s *HisoDeferColorCorrection);

/**
 * @brief Get HisoDeferColorCorrection of GData
 * This function will copy HisoDeferColorCorrection of GData.
 *
 * @param[out] AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s *HisoDeferColorCorrection: HisoDeferColorCorrection Config. Detail please reference AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoDeferColorCorrection(AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s *HisoDeferColorCorrection);

/**
 * @brief Set ShpAOrSpatialFilterSelect of GData
 * This function will replace ShpAOrSpatialFilterSelect of GData, and set ITUNER_SHP_A_OR_SPATIAL_FILTE_SELECT is valid.\n
 * If you want to update a part of ShpAOrSpatialFilterSelect, you should call AmbaItuner_Get_ShpAOrSpatialFilterSelect() to get current ShpAOrSpatialFilterSelect before the replace process.
 *
 * @param[in] UINT8 AMBA_DSP_IMG_LISO_PROCESS_SELECT_s *LisoProcessSelect: LisoProcessSelect Config. Detail please reference AMBA_DSP_IMG_LISO_PROCESS_SELECT_s.
 *
 * @return None
 */
void AmbaItuner_Set_ShpAOrSpatialFilterSelect(AMBA_DSP_IMG_LISO_PROCESS_SELECT_s *LisoProcessSelect);
/**
 * @brief Get ShpAOrSpatialFilterSelect of GData
 * This function will copy ShpAOrSpatialFilterSelect of GData.
 *
 * @param[out] UINT8 AMBA_DSP_IMG_LISO_PROCESS_SELECT_s *LisoProcessSelect: LisoProcessSelect Config. Detail please reference AMBA_DSP_IMG_LISO_PROCESS_SELECT_s.
 *
 * @return None
 */
void AmbaItuner_Get_ShpAOrSpatialFilterSelect(AMBA_DSP_IMG_LISO_PROCESS_SELECT_s *LisoProcessSelect);

/**
 * @brief Set AsfInfo of GData
 * This function will replace AsfInfo of GData, and set ITUNER_ASF_INFO is valid.\n
 * If you want to update a part of AsfInfo, you should call AmbaItuner_Get_AsfInfo() to get current AsfInfo before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_ASF_INFO_s *AsfInfo: AsfInfo Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_AsfInfo(ITUNER_ASF_INFO_s *AsfInfo);

/**
 * @brief Get AsfInfo of GData
 * This function will copy AsfInfo of GData.
 *
 * @param[out] AMBA_DSP_IMG_ASF_INFO_s *AsfInfo: AsfInfo Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_AsfInfo(ITUNER_ASF_INFO_s *AsfInfo);

/**
 * @brief Set HisoAsf of GData
 * This function will replace HisoAsf of GData, and set ITUNER_HISO_ASF is valid.\n
 * If you want to update a part of HisoAsf, you should call AmbaItuner_Get_HisoAsf() to get current HisoAsf before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_ASF_INFO_s *HisoAsf: HisoAsf Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoAsf(ITUNER_ASF_INFO_s *HisoAsf);

/**
 * @brief Get HisoAsf of GData
 * This function will copy HisoAsf of GData.
 *
 * @param[out] AMBA_DSP_IMG_ASF_INFO_s *HisoAsf: HisoAsf Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoAsf(ITUNER_ASF_INFO_s *HisoAsf);

/**
 * @brief Set HisoHighAsf of GData
 * This function will replace HisoHighAsf of GData, and set ITUNER_HISO_HIGH_ASF is valid.\n
 * If you want to update a part of HisoHighAsf, you should call AmbaItuner_Get_HisoHighAsf() to get current HisoHighAsf before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_ASF_INFO_s *HisoHighAsf: HisoHighAsf Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoHighAsf(ITUNER_ASF_INFO_s *HisoHighAsf);

/**
 * @brief Get HisoHighAsf of GData
 * This function will copy HisoHighAsf of GData.
 *
 * @param[out] AMBA_DSP_IMG_ASF_INFO_s *HisoHighAsf: HisoHighAsf Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoHighAsf(ITUNER_ASF_INFO_s *HisoHighAsf);

/**
 * @brief Set HisoLowAsf of GData
 * This function will replace HisoLowAsf of GData, and set ITUNER_HISO_LOW_ASF is valid.\n
 * If you want to update a part of HisoLowAsf, you should call AmbaItuner_Get_HisoLowAsf() to get current HisoLowAsf before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_ASF_INFO_s *HisoLowAsf: HisoLowAsf Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoLowAsf(ITUNER_ASF_INFO_s *HisoLowAsf);

/**
 * @brief Get HisoLowAsf of GData
 * This function will copy HisoLowAsf of GData.
 *
 * @param[out] AMBA_DSP_IMG_ASF_INFO_s *HisoLowAsf: HisoLowAsf Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoLowAsf(ITUNER_ASF_INFO_s *HisoLowAsf);

/**
 * @brief Set HisoMedAsf of GData
 * This function will replace HisoMedAsf of GData, and set ITUNER_HISO_MED_ASF is valid.\n
 * If you want to update a part of HisoMedAsf, you should call AmbaItuner_Get_HisoMedAsf() to get current HisoMedAsf before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_ASF_INFO_s *HisoMedAsf: HisoMedAsf Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoMedAsf(ITUNER_ASF_INFO_s *HisoMedAsf);

/**
 * @brief Get HisoMedAsf of GData
 * This function will copy HisoMedAsf of GData.
 *
 * @param[out] AMBA_DSP_IMG_ASF_INFO_s *HisoMedAsf: HisoMedAsf Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoMedAsf(ITUNER_ASF_INFO_s *HisoMedAsf);

/**
 * @brief Set HisoChromaAsf of GData
 * This function will replace HisoChromaAsf of GData, and set ITUNER_HISO_CHROMA_ASF is valid.\n
 * If you want to update a part of HisoChromaAsf, you should call AmbaItuner_Get_HisoChromaAsf() to get current HisoChromaAsf before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_ASF_INFO_s *HisoChromaAsf: HisoChromaAsf Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoChromaAsf(ITUNER_CHROMA_ASF_INFO_s *HisoChromaAsf);

/**
 * @brief Get HisoChromaAsf of GData
 * This function will copy HisoChromaAsf of GData.
 *
 * @param[out] AMBA_DSP_IMG_ASF_INFO_s *HisoChromaAsf: HisoChromaAsf Config. Detail please reference AMBA_DSP_IMG_ASF_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoChromaAsf(ITUNER_CHROMA_ASF_INFO_s *HisoChromaAsf);


/**
 * @brief Set ChromaFilter of GData
 * This function will replace ChromaFilter of GData, and set ITUNER_CHROMA_FILTER is valid.\n
 * If you want to update a part of ChromaFilter, you should call AmbaItuner_Get_ChromaFilter() to get current ChromaFilter before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CHROMA_FILTER_s *ChromaFilter: ChromaFilter Config. Detail please reference AMBA_DSP_IMG_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_ChromaFilter(AMBA_DSP_IMG_CHROMA_FILTER_s *ChromaFilter);

/**
 * @brief Get ChromaFilter of GData
 * This function will copy ChromaFilter of GData.
 *
 * @param[out] AMBA_DSP_IMG_CHROMA_FILTER_s *ChromaFilter: ChromaFilter Config. Detail please reference AMBA_DSP_IMG_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_ChromaFilter(AMBA_DSP_IMG_CHROMA_FILTER_s *ChromaFilter);

/**
 * @brief Set WideChromaFilter of GData
 * This function will replace WideChromaFilter of GData, and set ITUNER_WIDE_CHROMA_FILTER is valid.\n
 * If you want to update a part of WideChromaFilter, you should call AmbaItuner_Get_WideChromaFilter() to get current ChromaFilter before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CHROMA_FILTER_s *WideChromaFilter: ChromaFilter Config. Detail please reference AMBA_DSP_IMG_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_WideChromaFilter(AMBA_DSP_IMG_CHROMA_FILTER_s *WideChromaFilter);

/**
 * @brief Get WideChromaFilter of GData
 * This function will copy WideChromaFilter of GData.
 *
 * @param[out] AMBA_DSP_IMG_CHROMA_FILTER_s *WideChromaFilter: ChromaFilter Config. Detail please reference AMBA_DSP_IMG_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_WideChromaFilter(AMBA_DSP_IMG_CHROMA_FILTER_s *WideChromaFilter);

/**
 * @brief Set WideChromaFilterCombine of GData
 * This function will replace WideChromaFilterCombine of GData, and set ITUNER_WIDE_CHROMA_FILTER is valid.\n
 * If you want to update a part of WideChromaFilterCombine, you should call AmbaItuner_Get_WideChromaFilterCombine() to get current ChromaFilter before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *WideChromaFilterCombine: ChromaFilter Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Set_WideChromaFilterCombine(AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *WideChromaFilterCombine);

/**
 * @brief Get WideChromaFilterCombine of GData
 * This function will copy WideChromaFilterCombine of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *WideChromaFilterCombine: ChromaFilter Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Get_WideChromaFilterCombine(AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *WideChromaFilterCombine);

/**
 * @brief Set HisoChromaFilterHigh of GData
 * This function will replace HisoChromaFilterHigh of GData, and set ITUNER_HISO_CHROMA_FILTER_HIGH is valid.\n
 * If you want to update a part of HisoChromaFilterHigh, you should call AmbaItuner_Get_HisoChromaFilterHigh() to get current HisoChromaFilterHigh before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_CHROMA_FILTER_s *HisoChromaFilterHigh: HisoChromaFilterHigh Config. Detail please reference AMBA_DSP_IMG_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoChromaFilterHigh(AMBA_DSP_IMG_CHROMA_FILTER_s *HisoChromaFilterHigh);

/**
 * @brief Get HisoChromaFilterHigh of GData
 * This function will copy HisoChromaFilterHigh of GData.
 *
 * @param[out] AMBA_DSP_IMG_CHROMA_FILTER_s *HisoChromaFilterHigh: HisoChromaFilterHigh Config. Detail please reference AMBA_DSP_IMG_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoChromaFilterHigh(AMBA_DSP_IMG_CHROMA_FILTER_s *HisoChromaFilterHigh);

/**
 * @brief Set GbGrMismatch of GData
 * This function will replace GbGrMismatch of GData, and set ITUNER_GB_GR_MISMATCH is valid.\n
 * If you want to update a part of GbGrMismatch, you should call AmbaItuner_Get_GbGrMismatch() to get current GbGrMismatch before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_GBGR_MISMATCH_s *GbGrMismatch: GbGrMismatch Config. Detail please reference AMBA_DSP_IMG_GBGR_MISMATCH_s.
 *
 * @return None
 */
void AmbaItuner_Set_GbGrMismatch(AMBA_DSP_IMG_GBGR_MISMATCH_s *GbGrMismatch);

/**
 * @brief Get GbGrMismatch of GData
 * This function will copy GbGrMismatch of GData.
 *
 * @param[out] AMBA_DSP_IMG_GBGR_MISMATCH_s *GbGrMismatch: GbGrMismatch Config. Detail please reference AMBA_DSP_IMG_GBGR_MISMATCH_s.
 *
 * @return None
 */
void AmbaItuner_Get_GbGrMismatch(AMBA_DSP_IMG_GBGR_MISMATCH_s *GbGrMismatch);

/**
 * @brief Set HisoGbGrMismatch of GData
 * This function will replace HisoGbGrMismatch of GData, and set ITUNER_HISO_GB_GR_MISMATCH is valid.\n
 * If you want to update a part of HisoGbGrMismatch, you should call AmbaItuner_Get_HisoGbGrMismatch() to get current HisoGbGrMismatch before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_GBGR_MISMATCH_s *HisoGbGrMismatch: HisoGbGrMismatch Config. Detail please reference AMBA_DSP_IMG_GBGR_MISMATCH_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoGbGrMismatch(AMBA_DSP_IMG_GBGR_MISMATCH_s *HisoGbGrMismatch);

/**
 * @brief Get HisoGbGrMismatch of GData
 * This function will copy HisoGbGrMismatch of GData.
 *
 * @param[out] AMBA_DSP_IMG_GBGR_MISMATCH_s *HisoGbGrMismatch: HisoGbGrMismatch Config. Detail please reference AMBA_DSP_IMG_GBGR_MISMATCH_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoGbGrMismatch(AMBA_DSP_IMG_GBGR_MISMATCH_s *HisoGbGrMismatch);

/**
 * @brief Set ChromaAberrationInfo of GData
 * This function will replace ChromaAberrationInfo of GData, and set ITUNER_CHROMA_ABERRATION_INFO is valid.\n
 * If you want to update a part of ChromaAberrationInfo, you should call AmbaItuner_Get_ChromaAberrationInfo() to get current ChromaAberrationInfo before the replace process.
 *
 * @param[in] ITUNER_CHROMA_ABERRATION_s *ChromaAberrationInfo: ChromaAberrationInfo Config. Detail please reference ITUNER_CHROMA_ABERRATION_s.
 *
 * @return None
 */
void AmbaItuner_Set_ChromaAberrationInfo(ITUNER_CHROMA_ABERRATION_s *ChromaAberrationInfo);

/**
 * @brief Get ChromaAberrationInfo of GData
 * This function will copy ChromaAberrationInfo of GData.
 *
 * @param[out] ITUNER_CHROMA_ABERRATION_s *ChromaAberrationInfo: ChromaAberrationInfo Config. Detail please reference ITUNER_CHROMA_ABERRATION_s.
 *
 * @return None
 */
void AmbaItuner_Get_ChromaAberrationInfo(ITUNER_CHROMA_ABERRATION_s *ChromaAberrationInfo);


/**
 * @brief Get CA Table
 * This function will copy CA Table of GData.(Read only)
 *
 * @param[out] AMBA_DSP_IMG_CAWARP_CALC_INFO_s *CACalcInfo: CA Table, It's read only, please don't edit it.
 *
 * @return None
 */
void AmbaItuner_Get_CACalcInfo(AMBA_DSP_IMG_CAWARP_CALC_INFO_s *CACalcInfo);

/**
 * @brief Set ChromaAberrationInfoByPass of GData
 * This function will replace ChromaAberrationInfoByPass of GData, and set ITUNER_CHROMA_ABERRATION_INFO_BY_PASS is valid.\n
 * If you want to update a part of ChromaAberrationInfoByPass, you should call AmbaItuner_Get_ChromaAberrationInfoByPass() to get current ChromaAberrationInfoByPass before the replace process.
 *
 * @param[in] ITUNER_CHROMA_ABERRATION_BYPASS_s *ChromaAberrationInfoByPass: ChromaAberrationInfoByPass Config. Detail please reference ITUNER_CHROMA_ABERRATION_BYPASS_s.
 *
 * @return None
 */
void AmbaItuner_Set_ChromaAberrationInfoByPass(ITUNER_CHROMA_ABERRATION_BYPASS_s *ChromaAberrationInfoByPass);

/**
 * @brief Get ChromaAberrationInfoByPass of GData
 * This function will copy ChromaAberrationInfoByPass of GData.
 *
 * @param[out] ITUNER_CHROMA_ABERRATION_BYPASS_s *ChromaAberrationInfoByPass: ChromaAberrationInfoByPass Config. Detail please reference ITUNER_CHROMA_ABERRATION_BYPASS_s.
 *
 * @return None
 */
void AmbaItuner_Get_ChromaAberrationInfoByPass(ITUNER_CHROMA_ABERRATION_BYPASS_s *ChromaAberrationInfoByPass);

/**
 * @brief Set HisoChromaFilterLowVeryLow of GData
 * This function will replace HisoChromaFilterLowVeryLow of GData, and set ITUNER_HISO_CHROMA_FILTER_LOW_VERY_LOW is valid.\n
 * If you want to update a part of HisoChromaFilterLowVeryLow, you should call AmbaItuner_Get_HisoChromaFilterLowVeryLow() to get current HisoChromaFilterLowVeryLow before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s *HisoChromaFilterLowVeryLow: HisoChromaFilterLowVeryLow Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoChromaFilterLowVeryLow(AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s *HisoChromaFilterLowVeryLow);

/**
 * @brief Get HisoChromaFilterLowVeryLow of GData
 * This function will copy HisoChromaFilterLowVeryLow of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s *HisoChromaFilterLowVeryLow: HisoChromaFilterLowVeryLow Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoChromaFilterLowVeryLow(AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s *HisoChromaFilterLowVeryLow);

/**
 * @brief Set HisoChromaFilterPre of GData
 * This function will replace HisoChromaFilterPre of GData, and set ITUNER_HISO_CHROMA_FILTER_PRE is valid.\n
 * If you want to update a part of HisoChromaFilterPre, you should call AmbaItuner_Get_HisoChromaFilterPre() to get current HisoChromaFilterPre before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterPre: HisoChromaFilterPre Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoChromaFilterPre(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterPre);

/**
 * @brief Get HisoChromaFilterPre of GData
 * This function will copy HisoChromaFilterPre of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterPre: HisoChromaFilterPre Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoChromaFilterPre(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterPre);

/**
 * @brief Set HisoChromaFilterMed of GData
 * This function will replace HisoChromaFilterMed of GData, and set ITUNER_HISO_CHROMA_FILTER_MED is valid.\n
 * If you want to update a part of HisoChromaFilterMed, you should call AmbaItuner_Get_HisoChromaFilterMed() to get current HisoChromaFilterMed before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterMed: HisoChromaFilterMed Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoChromaFilterMed(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterMed);

/**
 * @brief Get HisoChromaFilterMed of GData
 * This function will copy HisoChromaFilterMed of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterMed: HisoChromaFilterMed Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoChromaFilterMed(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterMed);

/**
 * @brief Set HisoChromaFilterLow of GData
 * This function will replace HisoChromaFilterLow of GData, and set ITUNER_HISO_CHROMA_FILTER_LOW is valid.\n
 * If you want to update a part of HisoChromaFilterLow, you should call AmbaItuner_Get_HisoChromaFilterLow() to get current HisoChromaFilterLow before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterLow: HisoChromaFilterLow Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoChromaFilterLow(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterLow);

/**
 * @brief Get HisoChromaFilterLow of GData
 * This function will copy HisoChromaFilterLow of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterLow: HisoChromaFilterLow Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoChromaFilterLow(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterLow);

/**
 * @brief Set HisoChromaFilterVeryLow of GData
 * This function will replace HisoChromaFilterVeryLow of GData, and set ITUNER_HISO_CHROMA_FILTER_VERY_LOW is valid.\n
 * If you want to update a part of HisoChromaFilterVeryLow, you should call AmbaItuner_Get_HisoChromaFilterVeryLow() to get current HisoChromaFilterVeryLow before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterVeryLow: HisoChromaFilterVeryLow Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoChromaFilterVeryLow(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterVeryLow);

/**
 * @brief Get HisoChromaFilterVeryLow of GData
 * This function will copy HisoChromaFilterVeryLow of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterVeryLow: HisoChromaFilterVeryLow Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoChromaFilterVeryLow(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *HisoChromaFilterVeryLow);

/**
 * @brief Set HisoChromaFilterMedCombine of GData
 * This function will replace HisoChromaFilterMedCombine of GData, and set ITUNER_HISO_CHROMA_FILTER_MED_COMBINE is valid.\n
 * If you want to update a part of HisoChromaFilterMedCombine, you should call AmbaItuner_Get_HisoChromaFilterMedCombine() to get current HisoChromaFilterMedCombine before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterMedCombine: HisoChromaFilterMedCombine Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoChromaFilterMedCombine(ITUNER_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterMedCombine);

/**
 * @brief Get HisoChromaFilterMedCombine of GData
 * This function will copy HisoChromaFilterMedCombine of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterMedCombine: HisoChromaFilterMedCombine Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoChromaFilterMedCombine(ITUNER_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterMedCombine);

/**
 * @brief Set HisoChromaFilterVeryLowCombine of GData
 * This function will replace HisoChromaFilterVeryLowCombine of GData, and set ITUNER_HISO_CHROMA_FILTER_VERY_LOW_COMBINE is valid.\n
 * If you want to update a part of HisoChromaFilterVeryLowCombine, you should call AmbaItuner_Get_HisoChromaFilterVeryLowCombine() to get current HisoChromaFilterVeryLowCombine before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterVeryLowCombine: HisoChromaFilterVeryLowCombine Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoChromaFilterVeryLowCombine(ITUNER_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterVeryLowCombine);

/**
 * @brief Get HisoChromaFilterVeryLowCombine of GData
 * This function will copy HisoChromaFilterVeryLowCombine of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterVeryLowCombine: HisoChromaFilterVeryLowCombine Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoChromaFilterVeryLowCombine(ITUNER_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterVeryLowCombine);

/**
 * @brief Set HisoChromaFilterLowCombine of GData
 * This function will replace HisoChromaFilterLowCombine of GData, and set ITUNER_HISO_CHROMA_FILTER_LOW_COMBINE is valid.\n
 * If you want to update a part of HisoChromaFilterLowCombine, you should call AmbaItuner_Get_HisoChromaFilterLowCombine() to get current HisoChromaFilterLowCombine before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterLowCombine: HisoChromaFilterLowCombine Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoChromaFilterLowCombine(ITUNER_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterLowCombine);

/**
 * @brief Get HisoChromaFilterLowCombine of GData
 * This function will copy HisoChromaFilterLowCombine of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterLowCombine: HisoChromaFilterLowCombine Config. Detail please reference AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoChromaFilterLowCombine(ITUNER_HISO_CHROMA_FILTER_COMBINE_s *HisoChromaFilterLowCombine);

/**
 * @brief Set HisoLumaNoiseCombine of GData
 * This function will replace HisoLumaNoiseCombine of GData, and set ITUNER_HISO_LUMA_NOISE_COMBINE is valid.\n
 * If you want to update a part of HisoLumaNoiseCombine, you should call AmbaItuner_Get_HisoLumaNoiseCombine() to get current HisoLumaNoiseCombine before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s *HisoLumaNoiseCombine: HisoLumaNoiseCombine Config. Detail please reference AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoLumaNoiseCombine(ITUNER_HISO_LUMA_FILTER_COMBINE_s *HisoLumaNoiseCombine);

/**
 * @brief Get HisoLumaNoiseCombine of GData
 * This function will copy HisoLumaNoiseCombine of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s *HisoLumaNoiseCombine: HisoLumaNoiseCombine Config. Detail please reference AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoLumaNoiseCombine(ITUNER_HISO_LUMA_FILTER_COMBINE_s *HisoLumaNoiseCombine);

/**
 * @brief Set HisoLowASFCombine of GData
 * This function will replace HisoLowASFCombine of GData, and set ITUNER_HISO_LOW_ASF_COMBINE is valid.\n
 * If you want to update a part of HisoLowASFCombine, you should call AmbaItuner_Get_HisoLowASFCombine() to get current HisoLowASFCombine before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s *HisoLowASFCombine: HisoLowASFCombine Config. Detail please reference AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoLowASFCombine(ITUNER_HISO_LUMA_FILTER_COMBINE_s *HisoLowASFCombine);

/**
 * @brief Get HisoLowASFCombine of GData
 * This function will copy HisoLowASFCombine of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s *HisoLowASFCombine: HisoLowASFCombine Config. Detail please reference AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoLowASFCombine(ITUNER_HISO_LUMA_FILTER_COMBINE_s *HisoLowASFCombine);

/**
 * @brief Set HisoFreqRecover of GData
 * This function will replace HisoFreqRecover of GData, and set ITUNER_HISO_FREQ_RECOVER is valid.\n
 * If you want to update a part of HisoFreqRecover, you should call AmbaItuner_Get_HisoFreqRecover() to get current HisoFreqRecover before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_HISO_FREQ_RECOVER_s *HisoFreqRecover: HisoFreqRecover Config. Detail please reference AMBA_DSP_IMG_HISO_FREQ_RECOVER_s.
 *
 * @return None
 */
void AmbaItuner_Set_HisoFreqRecover(AMBA_DSP_IMG_HISO_FREQ_RECOVER_s *HisoFreqRecover);

/**
 * @brief Get HisoFreqRecover of GData
 * This function will copy HisoFreqRecover of GData.
 *
 * @param[out] AMBA_DSP_IMG_HISO_FREQ_RECOVER_s *HisoFreqRecover: HisoFreqRecover Config. Detail please reference AMBA_DSP_IMG_HISO_FREQ_RECOVER_s.
 *
 * @return None
 */
void AmbaItuner_Get_HisoFreqRecover(AMBA_DSP_IMG_HISO_FREQ_RECOVER_s *HisoFreqRecover);

/**
 * @brief Set SensorInfo of GData
 * This function will replace SensorInfo of GData, and set ITUNER_SENSOR_INFO is valid.\n
 * If you want to update a part of SensorInfo, you should call AmbaItuner_Get_SensorInfo() to get current SensorInfo before the replace process.
 *
 * @param[in] AMBA_DSP_IMG_SENSOR_INFO_s *SensorInfo: SensorInfo Config. Detail please reference AMBA_DSP_IMG_SENSOR_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Set_SensorInfo(AMBA_DSP_IMG_SENSOR_INFO_s *SensorInfo);

/**
 * @brief Get SensorInfo of GData
 * This function will copy SensorInfo of GData.
 *
 * @param[out] AMBA_DSP_IMG_SENSOR_INFO_s *SensorInfo: SensorInfo Config. Detail please reference AMBA_DSP_IMG_SENSOR_INFO_s.
 *
 * @return None
 */
void AmbaItuner_Get_SensorInfo(AMBA_DSP_IMG_SENSOR_INFO_s *SensorInfo);

/*
void AmbaItuner_Set_VideoHdrAmplifierLinearizationHdr(UINT8 Index, AMBA_DSP_IMG_HDR_BLKLVL_AMPLINEAR_s *AmpLinearHdr);
void AmbaItuner_Get_VideoHdrAmplifierLinearizationHdr(UINT8 Index, AMBA_DSP_IMG_HDR_BLKLVL_AMPLINEAR_s *AmpLinearHdr);
void AmbaItuner_Set_VideoHdrSEStaticBlackLevel(UINT8 Index, AMBA_DSP_IMG_BLACK_CORRECTION_s *StaticBlackLevel);
void AmbaItuner_Get_VideoHdrSEStaticBlackLevel(UINT8 Index, AMBA_DSP_IMG_BLACK_CORRECTION_s *StaticBlackLevel);
*/

void AmbaItuner_Set_VideoHdrLEStaticBlackLevel(UINT8 Index, AMBA_DSP_IMG_BLACK_CORRECTION_s *StaticBlackLevel);
void AmbaItuner_Get_VideoHdrLEStaticBlackLevel(UINT8 Index, AMBA_DSP_IMG_BLACK_CORRECTION_s *StaticBlackLevel);
void AmbaItuner_Set_VideoHdrAlphaCalcConfig(UINT8 Index, AMBA_DSP_IMG_HDR_ALPHA_CALC_CONFIG_s *AlphaCalcConfig);
void AmbaItuner_Get_VideoHdrAlphaCalcConfig(UINT8 Index, AMBA_DSP_IMG_HDR_ALPHA_CALC_CONFIG_s *AlphaCalcConfig);
void AmbaItuner_Set_VideoHdrAlphaCalcThresh(UINT8 Index, AMBA_DSP_IMG_HDR_ALPHA_CALC_THRESH_s *AlphaCalcThresh);
void AmbaItuner_Get_VideoHdrAlphaCalcThresh(UINT8 Index, AMBA_DSP_IMG_HDR_ALPHA_CALC_THRESH_s *AlphaCalcThresh);
void AmbaItuner_Set_VideoHdrAlphaCalcBlackLevel(UINT8 Index, AMBA_DSP_IMG_BLACK_CORRECTION_s *AlphaCalcBlackLevel);
void AmbaItuner_Get_VideoHdrAlphaCalcBlackLevel(UINT8 Index, AMBA_DSP_IMG_BLACK_CORRECTION_s *AlphaCalcBlackLevel);
void AmbaItuner_Set_VideoHdrAlphaCalcAlpha(UINT8 Index, ITUNER_VIDEO_HDR_ALPHA_CALC_ALPHA_s *AlphaCalcAlpha);
void AmbaItuner_Get_VideoHdrAlphaCalcAlpha(UINT8 Index, ITUNER_VIDEO_HDR_ALPHA_CALC_ALPHA_s *AlphaCalcAlpha);
void AmbaItuner_Set_VideoHdrAmplifierLinearization(UINT8 Index, ITUNER_VIDEO_HDR_AMP_LINEARIZATION_s *AmpLinear);
void AmbaItuner_Get_VideoHdrAmplifierLinearization(UINT8 Index, ITUNER_VIDEO_HDR_AMP_LINEARIZATION_s *AmpLinear);
void AmbaItuner_Set_VideoHdrContrastEnhance(AMBA_DSP_IMG_CONTRAST_ENHANCE_s *ContrastEnhance);
void AmbaItuner_Get_VideoHdrContrastEnhance(AMBA_DSP_IMG_CONTRAST_ENHANCE_s *ContrastEnhance);
void AmbaItuner_Set_VideoHdrRawInfo(ITUNER_VIDEO_HDR_RAW_INFO_s *RawInfo);
void AmbaItuner_Get_VideoHdrRawInfo(ITUNER_VIDEO_HDR_RAW_INFO_s *RawInfo);
__END_C_PROTO__

#endif /* _AMBA_IMG_CALIB_ITUNER_H_ */
/*!
 *
 * @} end of addtogroup Ituner
 */
