/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaCalibInfo_IMX179.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Camera Information Managements
\*-------------------------------------------------------------------------------------------------------------------*/
#include "AmbaCalibInfo.h"
#include "AmbaSensor.h"
#include "AmbaSensor_IMX179.h"

const UINT32 IMX179_AmbaBadPixelModeTable[] = {
    [0] = AMBA_SENSOR_IMX179_S1_3200_2400_30P,   /* full view */
    [1] = AMBA_SENSOR_IMX179_S2_1600_1200_30P,   /* 2 x 2 binning */
    [2] = AMBA_SENSOR_IMX179_S4_800_450_120P,    /* 4 x 4 binning */
};

const UINT32 IMX179_AmbaBadPixelModeAssignment[] = {
    [AMBA_SENSOR_IMX179_S1_3200_1800_30P]   = 0,
    [AMBA_SENSOR_IMX179_S1_3200_1800_30P_L] = 0,
    [AMBA_SENSOR_IMX179_S1_1920_1080_60P]   = 0,
    [AMBA_SENSOR_IMX179_S1_3200_2400_30P]   = 0,
    [AMBA_SENSOR_IMX179_S1_2112_1188_60P]   = 0,
    [AMBA_SENSOR_IMX179_S2_1600_900_60P]    = 1,
    [AMBA_SENSOR_IMX179_S2_1600_1200_30P]   = 1,
    [AMBA_SENSOR_IMX179_S2_1600_1200_30P_L] = 1,
    [AMBA_SENSOR_IMX179_S2_1600_1200_60P]   = 1,
    [AMBA_SENSOR_IMX179_S4_800_450_120P]    = 2,
};

AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_IMX179Obj = {
    .AmbaVignetteMode           = AMBA_SENSOR_IMX179_S1_3200_2400_30P,
    .AmbaNumBadPixelMode        = GetArraySize(IMX179_AmbaBadPixelModeTable),
    .AmbaBadPixelModeTable      = IMX179_AmbaBadPixelModeTable,
    .AmbaBadPixelModeAssignment = IMX179_AmbaBadPixelModeAssignment,
    .AmbaCalibWidth             = 3200,
    .AmbaCalibHeight            = 2400,
};
