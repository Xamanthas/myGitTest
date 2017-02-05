/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaCalibInfo_IMX117.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Camera Information Managements
 *
 *  @History        ::
 *      Date        Name        Comments
 *      09/18/2013  Y.H.Chen    Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>

#include "AmbaDataType.h"
#include "AmbaDSP.h"
#include "AmbaSensor.h"

#include "AmbaCalibInfo_test.h"
#include "AmbaSensor_IMX117.h"

const UINT32 AmbaVignetteMode1 = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_30P;

const UINT32 AmbaBadPixelModeTable1[] = {
    [0] = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_30P,   /* full view */
    [1] = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_2A_60P,  /* 2 x 2 binning */
    [2] = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4_120P,   /* 3 x 3 binning */
    [3] = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_5_240P,   /* 3 x 9 binning */
    [4] = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_7_480P,   /* 3 x 2/17 binning */
};
UINT32 AmbaBadPixelModeNum1 = sizeof(AmbaBadPixelModeTable1)/sizeof(AmbaBadPixelModeTable1[0]);

const UINT32 AmbaBadPixelModeAssignment1[] = {
    [AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_30P]    = 0,
    [AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0A_35P]   = 0,
    [AMBA_SENSOR_IMX117_TYPE_2_3_MODE_2A_60P]   = 1,
    [AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4A_240P]   = 2,
    [AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4_120P]    = 2,
    [AMBA_SENSOR_IMX117_TYPE_2_3_MODE_3_60P]    = 2,
    [AMBA_SENSOR_IMX117_TYPE_2_3_MODE_5_240P]    = 3,
    [AMBA_SENSOR_IMX117_TYPE_2_3_MODE_7_480P]    = 4,

    [AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_30P]    = 0,
    [AMBA_SENSOR_IMX117_TYPE_2_5_MODE_1_60P]    = 0,
    [AMBA_SENSOR_IMX117_TYPE_2_5_MODE_4_120P]    = 2,

    [AMBA_SENSOR_IMX117_TYPE_4_4_MODE_1_120P]    = 0,
};
