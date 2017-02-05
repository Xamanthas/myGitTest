/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_IMX377Table.c
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of SONY IMX377 CMOS sensor with MIPI interface
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaDSP.h"
#include "AmbaSensor.h"
#include "AmbaSensor_IMX377_1152M.h"

/*-----------------------------------------------------------------------------------------------*\
 * Sensor Device Information
\*-----------------------------------------------------------------------------------------------*/
const AMBA_SENSOR_DEVICE_INFO_s IMX377DeviceInfo = {
    .UnitCellWidth          = 1.55,
    .UnitCellHeight         = 1.55,
    .NumTotalPixelCols      = 4152,
    .NumTotalPixelRows      = 3062,
    .NumEffectivePixelCols  = 4056,
    .NumEffectivePixelRows  = 3046,
    .MinAnalogGainFactor    = 1.0,
    .MaxAnalogGainFactor    = 24.0, /* 27 dB */
    .MinDigitalGainFactor   = 1.0,
    .MaxDigitalGainFactor   = 8.0,  /* 18 dB */

    .FrameRateCtrlInfo = {
        .CommunicationTime      = AMBA_SENSOR_COMMUNICATION_AT_NON_VBLANK,
        .FirstReflectedFrame    = 2,
        .FirstBadFrame          = 2,
        .NumBadFrames           = 1
    },
    .ShutterSpeedCtrlInfo = {
        .CommunicationTime      = AMBA_SENSOR_COMMUNICATION_AT_VBLANK,
        .FirstReflectedFrame    = 1,
        .FirstBadFrame          = 0,
        .NumBadFrames           = 0
    },
    .AnalogGainCtrlInfo = {
        .CommunicationTime      = AMBA_SENSOR_COMMUNICATION_AT_VBLANK,
        .FirstReflectedFrame    = 0,
        .FirstBadFrame          = 0,
        .NumBadFrames           = 0
    },
    .DigitalGainCtrlInfo = {
        .CommunicationTime      = AMBA_SENSOR_COMMUNICATION_AT_NON_VBLANK,
        .FirstReflectedFrame    = 1,
        .FirstBadFrame          = 0,
        .NumBadFrames           = 0
    },
};

/*-----------------------------------------------------------------------------------------------*\
 * Readout drive pluse timing register settings
\*-----------------------------------------------------------------------------------------------*/
IMX377_PSTMG_REG_s IMX377PlstmgRegTable[IMX377_NUM_READOUT_PSTMG_REG] = {
    {0x3003, 0x20}, /* [7:0]: PLSTMG02 */
    {0x304e, 0x02}, /* [7:0]: PLSTMG31 */
    {0x3057, 0x4a}, /* [7:0]: PLSTMG22 */
    {0x3058, 0xf6}, /* [7:0]: PLSTMG23_LSB */
    {0x3059, 0x00}, /* [1:0]: PLSTMG23_MSB */
    {0x306b, 0x04}, /* [7:0]: PLSTMG24 */
    {0x3145, 0x00}, /* [1:0]: PLSTMG04 */
    {0x3202, 0x63}, /* [7:0]: PLSTMG25_LSB */
    {0x3203, 0x00}, /* [0]: PLSTMG25_MSB */
    {0x3236, 0x64}, /* [7:0]: PLSTMG26_LSB */
    {0x3237, 0x00}, /* [0]: PLSTMG26_MSB */
    {0x3304, 0x0b}, /* [7:0]: PLSTMG32_LSB */
    {0x3305, 0x00}, /* [3:0]: PLSTMG32_MSB */
    {0x3306, 0x0b}, /* [7:0]: PLSTMG33_LSB */
    {0x3307, 0x00}, /* [3:0]: PLSTMG33_MSB */
    {0x337f, 0x64}, /* [7:0]: PLSTMG27_LSB */
    {0x3380, 0x00}, /* [0]: PLSTMG27_MSB */
    {0x338d, 0x64}, /* [7:0]: PLSTMG28_LSB */
    {0x338e, 0x00}, /* [0]: PLSTMG27_MSB */
    {0x3510, 0x72}, /* [7:0]: PLSTMG05_LSB */
    {0x3511, 0x00}, /* [0]: PLSTMG05_MSB */
    {0x3528, 0x0f}, /* [7:0]: PLSTMG06 */
    {0x3529, 0x0f}, /* [7:0]: PLSTMG07 */
    {0x352a, 0x0f}, /* [7:0]: PLSTMG08 */
    {0x352b, 0x0f}, /* [7:0]: PLSTMG09 */
    {0x3538, 0x0f}, /* [7:0]: PLSTMG10 */
    {0x3539, 0x13}, /* [7:0]: PLSTMG11 */
    {0x353c, 0x01}, /* [7:0]: PLSTMG13 */
    {0x3553, 0x00}, /* [7:0]: PLSTMG34 */
    {0x3554, 0x00}, /* [7:0]: PLSTMG35 */
    {0x3555, 0x00}, /* [7:0]: PLSTMG36 */
    {0x3556, 0x00}, /* [7:0]: PLSTMG37 */
    {0x3557, 0x00}, /* [7:0]: PLSTMG38 */
    {0x3558, 0x00}, /* [7:0]: PLSTMG39 */
    {0x3559, 0x00}, /* [7:0]: PLSTMG40 */
    {0x355a, 0x00}, /* [7:0]: PLSTMG41 */
    {0x357d, 0x07}, /* [7:0]: PLSTMG14 */
    {0x357f, 0x07}, /* [7:0]: PLSTMG16 */
    {0x3580, 0x04}, /* [7:0]: PLSTMG17 */
    {0x3583, 0x60}, /* [2:0]: PLSTMG19, [6:4]: PLSTMG20 */
    {0x3587, 0x01}, /* [0]: PLSTMG21 */
    {0x3590, 0x0b}, /* [7:0]: PLSTMG42_LSB */
    {0x3591, 0x00}, /* [3:0]: PLSTMG42_MSB */
    {0x35ba, 0x0f}, /* [7:0]: PLSTMG43 */
    {0x366a, 0x0c}, /* [7:0]: PLSTMG44 */
    {0x366b, 0x0b}, /* [7:0]: PLSTMG45 */
    {0x366c, 0x07}, /* [7:0]: PLSTMG46 */
    {0x366d, 0x00}, /* [7:0]: PLSTMG47 */
    {0x366e, 0x00}, /* [7:0]: PLSTMG48 */
    {0x366f, 0x00}, /* [7:0]: PLSTMG49 */
    {0x3670, 0x00}, /* [7:0]: PLSTMG50 */
    {0x3671, 0x00}, /* [7:0]: PLSTMG51 */
    {0x3672, 0x00}, /* [7:0]: PLSTMG52_LSB */
    {0x3673, 0x00}, /* [2:0]: PLSTMG52_MSB */
    {0x3674, 0xdf}, /* [7:0]: PLSTMG53_LSB */
    {0x3675, 0x00}, /* [2:0]: PLSTMG53_MSB */
    {0x3676, 0xa7}, /* [7:0]: PLSTMG54_LSB */
    {0x3677, 0x01}, /* [2:0]: PLSTMG54_MSB */
    {0x3687, 0x00}, /* [7:0]: PLSTMG55 */
    {0x375c, 0x02}, /* [7:0]: PLSTMG56 */
    {0x380a, 0x0a}, /* [7:0]: PLSTMG29 */
    {0x382b, 0x16}, /* [7:0]: PLSTMG30 */
};

/*-----------------------------------------------------------------------------------------------*\
 * Sensor register settings of each readout modes
\*-----------------------------------------------------------------------------------------------*/
IMX377_MODE_REG_s IMX377ModeRegTable[IMX377_NUM_READOUT_MODE_REG] = {
    /*       [           Type 1/2.3 12.35M           ][      Type 1/2.5 9.03M      ][Custom] */
    /* Addr,    0A,    0,    1,   1A,    2,    3,    4,    0,    1,   1A,    2,    4,    1   */
    {0x3004, {0x00, 0x00, 0x00, 0x06, 0xa2, 0x45, 0x08, 0x10, 0x19, 0x06, 0xb1, 0x14, 0x19}}, /* [7:0]: MDSEL1 */
    {0x3005, {0x07, 0x07, 0x01, 0x01, 0x25, 0x35, 0x35, 0x07, 0x01, 0x01, 0x25, 0x35, 0x01}}, /* [7:0]: MDSEL2 */
    {0x3006, {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x04, 0x01, 0x01, 0x04}}, /* [7:0]: MDSEL3 */
    {0x3007, {0xa0, 0xa0, 0xa0, 0xa2, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa2, 0xa0, 0xa0, 0xA0}}, /* [7:0]: MDSEL4 */
    {0x300d, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: SVR_LSB */
    {0x300e, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: SVR_MSB */
    {0x301a, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [0]: MDVREV */
    {0x3039, {0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x58}}, /* [7:0]: HTRIMMING_END_LSB */
    {0x303a, {0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x0E}}, /* [4:0]: HTRIMMING_END_MSB */
    {0x303e, {0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x20}}, /* [7:0]: HTRIMMING_START_LSB */
    {0x303f, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}}, /* [4:0]: HTRIMMING_START_MSB */
    {0x3040, {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01}}, /* [0]: HTRIMMING_EN */
    {0x3068, {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}}, /* [7:0]: MDSEL7 */
    {0x307e, {0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDSEL5_LSB */
    {0x307f, {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08}}, /* [7:0]: MDSEL5_MSB */
    {0x3080, {0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS01_LSB */
    {0x3081, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS01_MSB */
    {0x3082, {0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS02_LSB */
    {0x3083, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS02_MSB */
    {0x3084, {0x00, 0x00, 0x00, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS03_LSB */
    {0x3085, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS03_MSB */
    {0x3086, {0x00, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS04_LSB */
    {0x3087, {0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS04_MSB */
    {0x3095, {0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS05_LSB */
    {0x3096, {0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS05_MSB */
    {0x3097, {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS06_LSB */
    {0x3098, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS06_MSB */
    {0x3099, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS07_LSB */
    {0x309a, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS07_MSB */
    {0x309b, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS08_LSB */
    {0x309c, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS08_MSB */
    {0x30bc, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS40_LSB */
    {0x30bd, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS40_MSB */
    {0x30be, {0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS41_LSB */
    {0x30bf, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS41_MSB */
    {0x30c0, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS42_LSB */
    {0x30c1, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS42_MSB */
    {0x30c2, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS43_LSB */
    {0x30c3, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS43_MSB */
    {0x30c4, {0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS44_LSB */
    {0x30c5, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS44_MSB */
    {0x30c6, {0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS45_LSB */
    {0x30c7, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS45_MSB */
    {0x30c8, {0x00, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS46_LSB */
    {0x30c9, {0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS46_MSB */
    {0x30ca, {0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS47_LSB */
    {0x30cb, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS47_MSB */
    {0x30cc, {0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS48 */
    {0x30d0, {0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x48}}, /* [7:0]: MDPLS52_LSB */
    {0x30d1, {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x0F}}, /* [7:0]: MDPLS52_MSB */
    {0x30d5, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}}, /* [0]: VWINDCUTEN */
    {0x30d6, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87}}, /* [7:0]: VWIDCUT_LSB */
    {0x30d7, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [2:0]: VWIDCUT_MSB */
    {0x30d8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43}}, /* [7:0]: VWINPOS_LSB */
    {0x30d9, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [3:0]: VWINPOS_MSB */
    {0x30da, {0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x05, 0x00, 0x00, 0x01, 0x02, 0x05, 0x00}}, /* [7:0]: MDREG01 */
    {0x30ee, {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}}, /* [7:0]: MDREG02 */
    {0x312f, {0xf6, 0xf6, 0xf6, 0xf6, 0xf4, 0xfe, 0xfc, 0x86, 0x86, 0x86, 0x4a, 0xe2, 0x78}}, /* [7:0]: MDSEL9_LSB */
    {0x3130, {0x0b, 0x0b, 0x0b, 0x0b, 0x05, 0x03, 0x03, 0x08, 0x08, 0x08, 0x04, 0x02, 0x07}}, /* [4:0]: MDSEL9_MSB */
    {0x3131, {0xe6, 0xe6, 0xe6, 0xe6, 0xf0, 0xfa, 0xf8, 0x7e, 0x7e, 0x7e, 0x46, 0xde, 0x70}}, /* [7:0]: MDSEL10_LSB */
    {0x3132, {0x0b, 0x0b, 0x0b, 0x0b, 0x05, 0x03, 0x03, 0x08, 0x08, 0x08, 0x04, 0x02, 0x07}}, /* [4:0]: MDSEL10_MSB */
    {0x3a41, {0x10, 0x10, 0x10, 0x10, 0x04, 0x04, 0x04, 0x08, 0x08, 0x08, 0x04, 0x04, 0x08}}, /* [7:0]: MDSEL11 */
};

const AMBA_SENSOR_OUTPUT_INFO_s IMX377OutputInfo[IMX377_NUM_READOUT_MODE] = {
/*IMX377_TYPE_2_3_MODE_0A */ {1440000000, 4, 12, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 4200, 3062, { 74, 40, 4096, 3000}, {0}},
/*IMX377_TYPE_2_3_MODE_0  */ {1440000000, 4, 12, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 4104, 3062, { 74, 40, 4000, 3000}, {0}},
/*IMX377_TYPE_2_3_MODE_1  */ {1440000000, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 4104, 3062, { 74, 40, 4000, 3000}, {0}},
/*IMX377_TYPE_2_3_MODE_1A */ {1440000000, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 4104, 3062, { 74, 40, 4000, 3000}, {0}},
/*IMX377_TYPE_2_3_MODE_2  */ {1440000000, 4, 12, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 2052, 1524, { 38, 14, 2000, 1500}, {0}},
/*IMX377_TYPE_2_3_MODE_3  */ {1440000000, 4, 12, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1368, 1022, { 26, 14, 1332,  998}, {0}},
/*IMX377_TYPE_2_3_MODE_4  */ {1440000000, 4, 12, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1368, 1020, { 26, 12, 1332, 1000}, {0}},
/*IMX377_TYPE_2_5_MODE_0  */ {1440000000, 4, 12, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 4200, 2182, { 74, 18, 4096, 2160}, {0}},
/*IMX377_TYPE_2_5_MODE_1  */ {1440000000, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 3912, 2182, { 60, 18, 3840, 2160}, {0}},
/*IMX377_TYPE_2_5_MODE_1A */ {1440000000, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 3912, 2182, { 60, 14, 3840, 2160}, {0}},
/*IMX377_TYPE_2_5_MODE_2  */ {1440000000, 4, 12, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 2100, 1098, { 38, 14, 2048, 1080}, {0}},
/*IMX377_TYPE_2_5_MODE_4  */ {1440000000, 4, 12, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1400,  738, { 26, 14, 1364,  720}, {0}},
/*IMX377_CUSTOM_MODE_1    */ {1440000000, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 3432, 1912, { 60, 18, 3360, 1890}, {0}},
};

const AMBA_SENSOR_INPUT_INFO_s IMX377InputInfoNormalReadout[IMX377_NUM_READOUT_MODE] = {
/*IMX377_TYPE_2_3_MODE_0A */ {{   0,   0, 4096, 3000}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_3_MODE_0  */ {{  48,   0, 4000, 3000}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_3_MODE_1  */ {{  48,   0, 4000, 3000}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_3_MODE_1A */ {{  48,   0, 4000, 3000}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_3_MODE_2  */ {{  50,   0, 4000, 3000}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX377_TYPE_2_3_MODE_3  */ {{  52,   2, 3996, 2994}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 3}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  3}, 1},
/*IMX377_TYPE_2_3_MODE_4  */ {{  52,   0, 3996, 3000}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 3}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  3}, 1},
/*IMX377_TYPE_2_5_MODE_0  */ {{   0, 420, 4096, 2160}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_5_MODE_1  */ {{ 130, 420, 3840, 2160}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_5_MODE_1A */ {{ 130, 420, 3840, 2160}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_5_MODE_2  */ {{   2, 420, 4096, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX377_TYPE_2_5_MODE_4  */ {{   4, 422, 4092, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 3}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  3}, 1},
/*IMX377_CUSTOM_MODE_1    */ {{ 370, 554, 3360, 1890}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
};

const AMBA_SENSOR_INPUT_INFO_s IMX377InputInfoInversionReadout[IMX377_NUM_READOUT_MODE] = {
/*IMX377_TYPE_2_3_MODE_0A */ {{   0,   0, 4096, 3000}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_3_MODE_0  */ {{  48,   0, 4000, 3000}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_3_MODE_1  */ {{  48,   0, 4000, 3000}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_3_MODE_1A */ {{  48,   0, 4000, 3000}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_3_MODE_2  */ {{  50,   2, 4000, 3000}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX377_TYPE_2_3_MODE_3  */ {{  52,   4, 3996, 2994}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 3}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  3}, 1},
/*IMX377_TYPE_2_3_MODE_4  */ {{  52,   2, 3996, 3000}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 3}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  3}, 1},
/*IMX377_TYPE_2_5_MODE_0  */ {{   0, 422, 4096, 2160}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_5_MODE_1  */ {{ 130, 422, 3840, 2160}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_5_MODE_1A */ {{ 130, 422, 3840, 2160}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX377_TYPE_2_5_MODE_2  */ {{   2, 422, 4096, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX377_TYPE_2_5_MODE_4  */ {{   4, 422, 4092, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 3}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  3}, 1},
/*IMX377_CUSTOM_MODE_1    */ {{ 370, 556, 3360, 1890}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
};

const IMX377_MODE_INFO_s IMX377ModeInfoList[AMBA_SENSOR_IMX377_NUM_MODE] = {
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0A_28P */ { IMX377_TYPE_2_3_MODE_0A, { 24000000, 3,  825, 1, 3120, 1, { .Interlace = 0, .TimeScale =  28000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0_28P  */ { IMX377_TYPE_2_3_MODE_0,  { 24000000, 3,  825, 1, 3120, 1, { .Interlace = 0, .TimeScale =  28000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0_25P  */ { IMX377_TYPE_2_3_MODE_0,  { 24000000, 3,  900, 1, 3200, 1, { .Interlace = 0, .TimeScale =     25,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_1_30P  */ { IMX377_TYPE_2_3_MODE_1,  { 24000000, 3,  770, 1, 3120, 1, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_1_25P  */ { IMX377_TYPE_2_3_MODE_1,  { 24000000, 3,  900, 1, 3200, 1, { .Interlace = 0, .TimeScale =     25,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_1A_30P */ { IMX377_TYPE_2_3_MODE_1A, { 24000000, 3,  385, 2, 6240, 1, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_1A_25P */ { IMX377_TYPE_2_3_MODE_1A, { 24000000, 3,  360, 2, 8000, 1, { .Interlace = 0, .TimeScale =     25,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_60P  */ { IMX377_TYPE_2_3_MODE_2,  { 24000000, 3,  390, 2, 3080, 1, { .Interlace = 0, .TimeScale =  60000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_50P  */ { IMX377_TYPE_2_3_MODE_2,  { 24000000, 3,  450, 2, 3200, 1, { .Interlace = 0, .TimeScale =     50,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_30P  */ { IMX377_TYPE_2_3_MODE_2,  { 24000000, 3,  780, 2, 3080, 1, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_25P  */ { IMX377_TYPE_2_3_MODE_2,  { 24000000, 3,  900, 2, 3200, 1, { .Interlace = 0, .TimeScale =     25,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_3_30P  */ { IMX377_TYPE_2_3_MODE_3,  { 24000000, 3,  546, 3, 4400, 1, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_3_25P  */ { IMX377_TYPE_2_3_MODE_3,  { 24000000, 3,  576, 3, 5000, 1, { .Interlace = 0, .TimeScale =     25,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_4_120P */ { IMX377_TYPE_2_3_MODE_4,  { 24000000, 3,  546, 1, 1100, 1, { .Interlace = 0, .TimeScale = 120000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_4_100P */ { IMX377_TYPE_2_3_MODE_4,  { 24000000, 3,  600, 1, 1200, 1, { .Interlace = 0, .TimeScale =    100,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_3_MODE_4_200P */ { IMX377_TYPE_2_3_MODE_4,  { 24000000, 3,  320, 1, 1125, 1, { .Interlace = 0, .TimeScale =    200,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_30P  */ { IMX377_TYPE_2_5_MODE_0,  { 24000000, 3, 1056, 1, 2275, 1, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_25P  */ { IMX377_TYPE_2_5_MODE_0,  { 24000000, 3, 1152, 1, 2500, 1, { .Interlace = 0, .TimeScale =     25,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_30P  */ { IMX377_TYPE_2_5_MODE_1,  { 24000000, 3, 1056, 1, 2275, 1, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_25P  */ { IMX377_TYPE_2_5_MODE_1,  { 24000000, 3, 1152, 1, 2500, 1, { .Interlace = 0, .TimeScale =     25,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_24P  */ { IMX377_TYPE_2_5_MODE_1,  { 24000000, 3,  660, 1, 4550, 1, { .Interlace = 0, .TimeScale =  24000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_120P */ { IMX377_TYPE_2_5_MODE_2,  { 24000000, 3,  264, 2, 2275, 1, { .Interlace = 0, .TimeScale = 120000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_100P */ { IMX377_TYPE_2_5_MODE_2,  { 24000000, 3,  288, 2, 2500, 1, { .Interlace = 0, .TimeScale =    100,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_60P  */ { IMX377_TYPE_2_5_MODE_2,  { 24000000, 3,  528, 2, 2275, 1, { .Interlace = 0, .TimeScale =  60000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_50P  */ { IMX377_TYPE_2_5_MODE_2,  { 24000000, 3,  576, 2, 2500, 1, { .Interlace = 0, .TimeScale =     50,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_TYPE_2_5_MODE_4_240P */ { IMX377_TYPE_2_5_MODE_4,  { 24000000, 3,  364, 1,  825, 1, { .Interlace = 0, .TimeScale = 240000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_TYPE_2_5_MODE_4_200P */ { IMX377_TYPE_2_5_MODE_4,  { 24000000, 3,  360, 1, 1000, 1, { .Interlace = 0, .TimeScale =    200,  .NumUnitsInTick =    1}}},
/*AMBA_SENSOR_IMX377_CUSTOM_MODE_1_60P    */ { IMX377_CUSTOM_MODE_1,    { 24000000, 3,  600, 1, 2002, 1, { .Interlace = 0, .TimeScale =  60000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX377_CUSTOM_MODE_1_50P    */ { IMX377_CUSTOM_MODE_1,    { 24000000, 3,  625, 1, 2304, 1, { .Interlace = 0, .TimeScale =     50,  .NumUnitsInTick =    1}}},
};
