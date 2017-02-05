/**
 * @file mw/unittest/extSrc/sample_calib/audio/AmbaUTCalibAudio.h
 *
 * Header file for audio calibration
 *
 * History:
 *    07/10/2013  Allen Chiu Created
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "../AmbaUTCalibMgr.h"
#include "AmbaAudio.h"
#include "recorder/AudioEnc.h"

typedef enum _AUDIO_CALIB_ERROR_MSG_ {
    AUDIO_CALIB_OK                                                              = 0,     /**< OK */
    AUDIO_CALIB_OPEN_FILE_ERROR                                       = -1,    /**< open output file error */
    AUDIO_CALIB_GET_CURVE_ERROR                                      = -2,    /**< get audio curve error */
    AUDIO_CALIB_READ_DATA_ERROR                                      = -3,    /**< read data error */
    AUDIO_CALIB_PARAM_CALIB_MODE_ERROR                        = -4,    /**< CalibMode error */
    AUDIO_CALIB_PARAM_CALIB_OPERATE_MODE_ERROR         = -5, /**< CalibOperateMode error */
    AUDIO_CALIB_FREQUENCY_INDEX_ERROR                           = -6, /**< CalibreFreqIdx error */
    AUDIO_CALIB_NULL_BUFFER_ERROR                                   =-7, /**< Null buffer error */
    AUDIO_CALIB_SILENT_THRESHOLD_ERROR                          =-8, /**< Silent threshold out of range*/    
    AUDIO_CALIB_RANGE_INDEX_ERROR                                   =-9, /**< Range Index out of range*/        
    AUDIO_CALIB_PARAMETER_ERROR                                      =-10, /**<audio calibration parameter error*/            
    AUDIO_CALIB_PARAMETER_NUMBER_ERROR                        =-11, /**<number of audio calibration parameter error*/
    AUDIO_CALIB_ALLOCATE_MEMORY_ERROR                        =-12, /**<allocate memeory error*/                    
} BPC_CALIB_ERROR_MSG;




typedef struct Audio_Calibration_s_ {
    UINT8  AudioProcModeEn[4];
    UINT8  Resverd[12];
    INT8    AudioCalibCurve[4][2048];
} Audio_Calibration_s;

