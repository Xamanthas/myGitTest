/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaCalibInfo_IMX317.c
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Calibration Information Managements
\*-------------------------------------------------------------------------------------------------------------------*/
#include "AmbaCalibInfo.h"
#include "AmbaSensor.h"
#include "AmbaSensor_IMX317.h"

const UINT32 IMX317_AmbaBadPixelModeTable[] = {
    [0] = AMBA_SENSOR_IMX317_TYPE_2_5_MODE_C_30P,  /* full view */
    [1] = AMBA_SENSOR_IMX317_TYPE_2_5_MODE_2_60P,  /* H: 2binning V: 2binning */
    [2] = AMBA_SENSOR_IMX317_TYPE_2_5_MODE_5_120P, /* H: 3binning V: 2/3subsampling */
    [3] = AMBA_SENSOR_IMX317_TYPE_2_5_MODE_6_240P, /* H: 3binning V: 2/8subsampling */
};

const UINT32 IMX317_AmbaBadPixelModeAssignment[] = {
    [AMBA_SENSOR_IMX317_TYPE_2_5_MODE_C_30P]  = 0,
    [AMBA_SENSOR_IMX317_TYPE_2_5_MODE_0_30P]  = 0,
    [AMBA_SENSOR_IMX317_TYPE_2_5_MODE_1_60P]  = 0,
    [AMBA_SENSOR_IMX317_TYPE_2_5_MODE_2_60P]  = 1,
    [AMBA_SENSOR_IMX317_TYPE_2_5_MODE_3_120P] = 1,
    [AMBA_SENSOR_IMX317_TYPE_2_5_MODE_4_30P]  = 1,
    [AMBA_SENSOR_IMX317_TYPE_2_5_MODE_5_120P] = 2,
    [AMBA_SENSOR_IMX317_TYPE_2_5_MODE_5_60P]  = 2,
    [AMBA_SENSOR_IMX317_TYPE_2_5_MODE_6_240P] = 3,
    [AMBA_SENSOR_IMX317_TYPE_2_5_MODE_6_30P]  = 3,
};

AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_IMX317Obj = {
    .AmbaVignetteMode           = AMBA_SENSOR_IMX317_TYPE_2_5_MODE_C_30P,
    .AmbaNumBadPixelMode        = GetArraySize(IMX317_AmbaBadPixelModeTable),
    .AmbaBadPixelModeTable      = IMX317_AmbaBadPixelModeTable,
    .AmbaBadPixelModeAssignment = IMX317_AmbaBadPixelModeAssignment,
    .AmbaCalibWidth             = 3852,
    .AmbaCalibHeight            = 2204,
};
