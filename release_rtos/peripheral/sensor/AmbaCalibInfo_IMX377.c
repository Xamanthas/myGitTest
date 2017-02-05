/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaCalibInfo_IMX377.c
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
#include "AmbaSensor_IMX377_1152M.h"

const UINT32 IMX377_AmbaBadPixelModeTable[] = {
    [0] = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0A_28P,  /* full view */
    [1] = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_3_30P,   /* H: 3binning V: 3binning */
    [2] = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_4_200P,  /* H: 3binning V: 1/3subsampling */
    [3] = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_60P,   /* H: 2binning V: 2binning */
};

const UINT32 IMX377_AmbaBadPixelModeAssignment[] = {
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0A_28P] = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0_28P]  = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0_25P]  = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_1_30P]  = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_1_25P]  = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_1A_30P] = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_1A_25P] = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_60P]  = 3,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_50P]  = 3,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_30P]  = 3,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_25P]  = 3,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_3_30P]  = 1,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_3_25P]  = 1,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_4_120P] = 2,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_4_100P] = 2,
    [AMBA_SENSOR_IMX377_TYPE_2_3_MODE_4_200P] = 2,
    [AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_30P]  = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_25P]  = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_30P]  = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_25P]  = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_24P]  = 0,
    [AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_120P] = 3,
    [AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_100P] = 3,
    [AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_60P]  = 3,
    [AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_50P]  = 3,
    [AMBA_SENSOR_IMX377_TYPE_2_5_MODE_4_240P] = 2,
    [AMBA_SENSOR_IMX377_TYPE_2_5_MODE_4_200P] = 2,
};

AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_IMX377Obj = {
    .AmbaVignetteMode           = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0A_28P,
    .AmbaNumBadPixelMode        = GetArraySize(IMX377_AmbaBadPixelModeTable),
    .AmbaBadPixelModeTable      = IMX377_AmbaBadPixelModeTable,
    .AmbaBadPixelModeAssignment = IMX377_AmbaBadPixelModeAssignment,
    .AmbaCalibWidth             = 4056,
    .AmbaCalibHeight            = 3046,
};
