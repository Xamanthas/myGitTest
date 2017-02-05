/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaCalibInfo_IMX117.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Calibration Information Managements
\*-------------------------------------------------------------------------------------------------------------------*/
#include "AmbaCalibInfo.h"
#include "AmbaSensor.h"
#include "AmbaSensor_AR1335.h"

const UINT32 AR1335_AmbaBadPixelModeTable[] = {
    [0] = AMBA_SENSOR_AR1335_4208_3120_30P,  /* full view */
    [1] = AMBA_SENSOR_AR1335_4208_1560_30P,  /* 1 x 2 binning */
    [2] = AMBA_SENSOR_AR1335_2104_1560_30P,  /* 16/32 x 2 binning */
};

const UINT32 AR1335_AmbaBadPixelModeAssignment[] = {
    [AMBA_SENSOR_AR1335_4208_3120_30P] = 0,
    [AMBA_SENSOR_AR1335_4208_2368_30P] = 0,
    [AMBA_SENSOR_AR1335_4208_1560_30P] = 1,
    [AMBA_SENSOR_AR1335_4096_2880_30P] = 0,
    [AMBA_SENSOR_AR1335_4096_2160_30P] = 0,
    [AMBA_SENSOR_AR1335_4096_1440_60P] = 1,
    [AMBA_SENSOR_AR1335_4096_1080_60P] = 1,
    [AMBA_SENSOR_AR1335_4096_1080_90P] = 1,
    [AMBA_SENSOR_AR1335_2104_1560_30P] = 2,

    [AMBA_SENSOR_AR1335_3168_2376_30P] = 0,
    [AMBA_SENSOR_AR1335_3168_1782_30P] = 0,
    [AMBA_SENSOR_AR1335_2880_1620_30P] = 0,
    [AMBA_SENSOR_AR1335_2880_1620_60P] = 0,

    [AMBA_SENSOR_AR1335_2080_1560_30P] = 0,
    [AMBA_SENSOR_AR1335_2080_1560_60P] = 0,
    [AMBA_SENSOR_AR1335_2104_1284_30P] = 0,
    [AMBA_SENSOR_AR1335_2104_1284_60P] = 0,
};

AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_AR1335Obj = {
    .AmbaVignetteMode           = AMBA_SENSOR_AR1335_4208_3120_30P,
    .AmbaNumBadPixelMode        = GetArraySize(AR1335_AmbaBadPixelModeTable),
    .AmbaBadPixelModeTable      = AR1335_AmbaBadPixelModeTable,
    .AmbaBadPixelModeAssignment = AR1335_AmbaBadPixelModeAssignment,
    .AmbaCalibWidth             = 4208,
    .AmbaCalibHeight            = 3120,
};
