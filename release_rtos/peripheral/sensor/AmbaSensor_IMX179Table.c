/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_IMX179Table.c
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of SONY IMX179 CMOS sensor with MIPI interface
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaDSP.h"
#include "AmbaSensor.h"
#include "AmbaSensor_IMX179.h"

/*-----------------------------------------------------------------------------------------------*\
 * Sensor Device Information
\*-----------------------------------------------------------------------------------------------*/
const AMBA_SENSOR_DEVICE_INFO_s IMX179DeviceInfo = {
    .UnitCellWidth          = 1.40,
    .UnitCellHeight         = 1.40,
    .NumTotalPixelCols      = 3288,
    .NumTotalPixelRows      = 2512,
    .NumEffectivePixelCols  = 3280,
    .NumEffectivePixelRows  = 2464,
    .MinAnalogGainFactor    = 1.0,
    .MaxAnalogGainFactor    = 8.0,    /* 18.06 dB */
    .MinDigitalGainFactor   = 1.0,
    .MaxDigitalGainFactor   = 15.996,  /* 24 dB */

    .FrameRateCtrlInfo = {
        .CommunicationTime      = AMBA_SENSOR_COMMUNICATION_AT_NON_VBLANK,
        .FirstReflectedFrame    = 2,
        .FirstBadFrame          = 0,
        .NumBadFrames           = 0
    },
    .ShutterSpeedCtrlInfo = {
        .CommunicationTime      = AMBA_SENSOR_COMMUNICATION_AT_NON_VBLANK,
        .FirstReflectedFrame    = 2,
        .FirstBadFrame          = 0,
        .NumBadFrames           = 0
    },
    .AnalogGainCtrlInfo = {
        .CommunicationTime      = AMBA_SENSOR_COMMUNICATION_AT_NON_VBLANK,
        .FirstReflectedFrame    = 1,
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
 * Sensor register settings of each readout modes
 * ============================================================================
 * Fvco = INCK / PREPLLCK_DIV * PLL_MPY
 * (1) PIX_RATE = Fvco / VTPXCK_DIV / VTSYCK_DIV * 2,  (2) BIT_RATE = Fvco / 2
 * ============================================================================
 * PLL0: INCK 18MHz, PIXRATE 259.2Mp/s, MIPI 648Mbps (4-lane)
 * PLL1: INCK 18MHz, PIXRATE 186.0Mp/s, MIPI 465Mbps (4-lane)
 * PLL2: INCK 18MHz, PIXRATE 180.0Mp/s, MIPI 450Mbps (4-lane)
 * PLL3: INCK 18MHz, PIXRATE 129.6Mp/s, MIPI 648Mbps (2-lane)
 * PLL4: INCK 18MHz, PIXRATE 203.4Mp/s, MIPI 508.5Mbps (4-lane)
\*-----------------------------------------------------------------------------------------------*/
IMX179_MODE_REG_s IMX179ModeRegTable[IMX179_NUM_READOUT_MODE_REG] = {
    /* Addr,     0,    1,    2,    3,    4,    5,    6,    7,    8,    9 */
    /*  PLL,  PLL0, PLL1, PLL0, PLL0, PLL0, PLL0, PLL2, PLL3, PLL0, PLL4 */
    {0x0202, {0x09, 0x07, 0x04, 0x09, 0x04, 0x04, 0x06, 0x04, 0x04, 0x01}}, /* 00: COARSE_INTEG_TIME[15:8] */
    {0x0203, {0xcc, 0x1c, 0xe4, 0xcc, 0xe4, 0xe4, 0xcc, 0xe4, 0xe4, 0xe9}}, /* 01: COARSE_INTEG_TIME[7:0] */
    {0x0301, {0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x0a, 0x05, 0x05}}, /* 02: VTPXCK_DIV */
    {0x0303, {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}}, /* 03: VTSYCK_DIV */
    {0x0305, {0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x04}}, /* 04: PREPLLCK_ DIV */
    {0x0309, {0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x0a, 0x05, 0x05}}, /* 05: OPPXCK_DIV */
    {0x030b, {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}}, /* 06: OPSYCK_DIV */
    {0x030c, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* 07: PLL_MPY[10:8] */
    {0x030d, {0xd8, 0x9b, 0xd8, 0xd8, 0xd8, 0xd8, 0x96, 0xd8, 0xd8, 0x71}}, /* 08: PLL_MPY[7:0] */
    {0x0340, {0x09, 0x07, 0x04, 0x09, 0x04, 0x04, 0x06, 0x04, 0x04, 0x01}}, /* 09: FRM_LENGTH[15:8] */
    {0x0341, {0xd0, 0x20, 0xe8, 0xd0, 0xe8, 0xe8, 0xd0, 0xe8, 0xe8, 0xed}}, /* 10: FRM_LENGTH[7:0] */
    {0x0342, {0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d}}, /* 11: LINE_LENGTH[15:8] */
    {0x0343, {0x70, 0x48, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70}}, /* 12: LINE_LENGTH[7:0] */
    {0x0344, {0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* 13: X_ADD_STA[11:8] */
    {0x0345, {0x28, 0x28, 0xa8, 0x00, 0x3a, 0x28, 0x08, 0x08, 0x08, 0x28}}, /* 14: X_ADD_STA[7:0] */
    {0x0346, {0x01, 0x01, 0x02, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01}}, /* 15: Y_ADD_STA[11:8] */
    {0x0347, {0x4c, 0x4c, 0xb4, 0x00, 0x64, 0x4c, 0x08, 0x08, 0x08, 0x4c}}, /* 16: Y_ADD_STA[7:0] */
    {0x0348, {0x0c, 0x0c, 0x0a, 0x0c, 0x0a, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c}}, /* 17: X_ADD_END[11:8] */
    {0x0349, {0xa7, 0xa7, 0x27, 0xcf, 0x95, 0xa7, 0xc7, 0xc7, 0xc7, 0xa7}}, /* 18: X_ADD_END[7:0] */
    {0x034a, {0x08, 0x08, 0x06, 0x09, 0x07, 0x08, 0x09, 0x09, 0x09, 0x08}}, /* 19: Y_ADD_END[11:8] */
    {0x034b, {0x53, 0x53, 0xeb, 0x9f, 0x3b, 0x53, 0x97, 0x97, 0x97, 0x53}}, /* 20: Y_ADD_END[7:0] */
    {0x034c, {0x0c, 0x0c, 0x07, 0x0c, 0x08, 0x06, 0x06, 0x06, 0x06, 0x03}}, /* 21: X_OUT_SIZE[11:8] */
    {0x034d, {0x80, 0x80, 0x80, 0xd0, 0x5c, 0x40, 0x60, 0x60, 0x60, 0x20}}, /* 22: X_OUT_SIZE[7:0] */
    {0x034e, {0x07, 0x07, 0x04, 0x09, 0x04, 0x03, 0x04, 0x04, 0x04, 0x01}}, /* 23: Y_OUT_SIZE[11:8] */
    {0x034f, {0x08, 0x08, 0x38, 0xa0, 0xd8, 0x84, 0xc8, 0xc8, 0xc8, 0xc2}}, /* 24: Y_OUT_SIZE[7:0] */
    {0x0383, {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}}, /* 25: X_ODD_INC */
    {0x0387, {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}}, /* 26: Y_ODD_INC */
    {0x0390, {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02}}, /* 27: BINNING_MODE */
    {0x0401, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* 28: scale_mode */
    {0x0405, {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}}, /* 29: scale_m */
    {0x3020, {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}}, /* 30:  */
    {0x3041, {0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15}}, /* 31:  */
    {0x3042, {0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87}}, /* 32:  */
    {0x3089, {0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f}}, /* 33:  */
    {0x3309, {0x9a, 0x9a, 0x9a, 0x9a, 0x9a, 0x9a, 0x9a, 0x9a, 0x9a, 0x9a}}, /* 34:  */
    {0x3344, {0x57, 0x47, 0x57, 0x57, 0x57, 0x57, 0x47, 0x57, 0x57, 0x47}}, /* 35:  */
    {0x3345, {0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}}, /* 36:  */
    {0x3362, {0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a}}, /* 37:  */
    {0x3363, {0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a}}, /* 38:  */
    {0x3364, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00}}, /* 39:  */
    {0x3368, {0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12}}, /* 40:  */
    {0x3369, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* 41:  */
    {0x3370, {0x77, 0x67, 0x77, 0x77, 0x77, 0x77, 0x67, 0x77, 0x77, 0x67}}, /* 42:  */
    {0x3371, {0x2f, 0x1f, 0x2f, 0x2f, 0x2f, 0x2f, 0x1f, 0x2f, 0x2f, 0x1f}}, /* 43:  */
    {0x3372, {0x4f, 0x47, 0x4f, 0x4f, 0x4f, 0x4f, 0x47, 0x4f, 0x4f, 0x47}}, /* 44:  */
    {0x3373, {0x2f, 0x27, 0x2f, 0x2f, 0x2f, 0x2f, 0x27, 0x2f, 0x2f, 0x27}}, /* 45:  */
    {0x3374, {0x2f, 0x1f, 0x2f, 0x2f, 0x2f, 0x2f, 0x1f, 0x2f, 0x2f, 0x1f}}, /* 46:  */
    {0x3375, {0x37, 0x1f, 0x37, 0x37, 0x37, 0x37, 0x1f, 0x37, 0x37, 0x1f}}, /* 47:  */
    {0x3376, {0x9f, 0x7f, 0x9f, 0x9f, 0x9f, 0x9f, 0x7f, 0x9f, 0x9f, 0x7f}}, /* 48:  */
    {0x3377, {0x37, 0x2f, 0x37, 0x37, 0x37, 0x37, 0x2f, 0x37, 0x37, 0x2f}}, /* 49:  */
    {0x33c8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* 50:  */
    {0x33d4, {0x0c, 0x0c, 0x07, 0x0c, 0x08, 0x06, 0x06, 0x06, 0x06, 0x03}}, /* 51:  */
    {0x33d5, {0x80, 0x80, 0x80, 0xd0, 0x5c, 0x40, 0x60, 0x60, 0x60, 0x20}}, /* 52:  */
    {0x33d6, {0x07, 0x07, 0x04, 0x09, 0x04, 0x03, 0x04, 0x04, 0x04, 0x01}}, /* 53:  */
    {0x33d7, {0x08, 0x08, 0x38, 0xa0, 0xd8, 0x84, 0xc8, 0xc8, 0xc8, 0xc2}}, /* 54:  */
    {0x4100, {0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e}}, /* 55:  */
    {0x4108, {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}}, /* 56:  */
    {0x4109, {0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c}}, /* 57:  */
};

const AMBA_SENSOR_OUTPUT_INFO_s IMX179OutputInfo[IMX179_NUM_READOUT_MODE] = {
/*IMX179_READOUT_MODE_0 */ { 647448552, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 3200, 1800, {  0,  0, 3200, 1800}, {0, 0, 0, 0}},
/*IMX179_READOUT_MODE_1 */ { 464655358, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 3200, 1800, {  0,  0, 3200, 1800}, {0, 0, 0, 0}},
/*IMX179_READOUT_MODE_2 */ { 647448552, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1920, 1080, {  0,  0, 1920, 1080}, {0, 0, 0, 0}},
/*IMX179_READOUT_MODE_3 */ { 647448552, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 3280, 2464, { 40, 32, 3200, 2400}, {0, 0, 0, 0}},
/*IMX179_READOUT_MODE_4 */ { 647448552, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 2140, 1240, { 14, 26, 2112, 1188}, {0, 0, 0, 0}},
/*IMX179_READOUT_MODE_5 */ { 647448552, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1600,  900, {  0,  0, 1600,  900}, {0, 0, 0, 0}},
/*IMX179_READOUT_MODE_6 */ { 449502500, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1632, 1224, { 16, 12, 1600, 1200}, {0, 0, 0, 0}},
/*IMX179_READOUT_MODE_7 */ { 647448552, 2, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1632, 1224, { 16, 12, 1600, 1200}, {0, 0, 0, 0}},
/*IMX179_READOUT_MODE_8 */ { 647448552, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1632, 1224, { 16, 12, 1600, 1200}, {0, 0, 0, 0}},
/*IMX179_READOUT_MODE_9 */ { 508267729, 4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0},  800,  450, {  0,  0,  800,  450}, {0, 0, 0, 0}},
};

const AMBA_SENSOR_INPUT_INFO_s IMX179InputInfoNormalReadout[IMX179_NUM_READOUT_MODE] = {
/*IMX179_READOUT_MODE_0 */ {{  40, 332, 3200, 1800}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX179_READOUT_MODE_1 */ {{  40, 332, 3200, 1800}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX179_READOUT_MODE_2 */ {{ 680, 692, 1920, 1080}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX179_READOUT_MODE_3 */ {{  40,  32, 3200, 2400}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX179_READOUT_MODE_4 */ {{ 584, 638, 2112, 1188}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX179_READOUT_MODE_5 */ {{  40, 332, 3200, 1800}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX179_READOUT_MODE_6 */ {{  40,  32, 3200, 2400}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX179_READOUT_MODE_7 */ {{  40,  32, 3200, 2400}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX179_READOUT_MODE_8 */ {{  40,  32, 3200, 2400}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX179_READOUT_MODE_9 */ {{  40, 332, 3200, 1800}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 4}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  4}, 1},
};

const AMBA_SENSOR_INPUT_INFO_s IMX179InputInfoInversionReadout[IMX179_NUM_READOUT_MODE] = {
/*IMX179_READOUT_MODE_0 */ {{  40, 332, 3200, 1800}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX179_READOUT_MODE_1 */ {{  40, 332, 3200, 1800}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX179_READOUT_MODE_2 */ {{ 680, 692, 1920, 1080}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX179_READOUT_MODE_3 */ {{  40,  32, 3200, 2400}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX179_READOUT_MODE_4 */ {{ 584, 638, 2112, 1188}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {   AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX179_READOUT_MODE_5 */ {{  40, 332, 3200, 1800}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX179_READOUT_MODE_6 */ {{  40,  32, 3200, 2400}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX179_READOUT_MODE_7 */ {{  40,  32, 3200, 2400}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX179_READOUT_MODE_8 */ {{  40,  32, 3200, 2400}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX179_READOUT_MODE_9 */ {{  40, 332, 3200, 1800}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 4}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  4}, 1},
};

const IMX179_MODE_INFO_s IMX179ModeInfoList[AMBA_SENSOR_IMX179_NUM_MODE] = {
/*AMBA_SENSOR_IMX179_S1_3200_1800_30P   */ { IMX179_READOUT_MODE_0, { 17984682, 3440, 2512, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX179_S1_3200_1800_30P_L */ { IMX179_READOUT_MODE_1, { 17986659, 3400, 1824, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX179_S1_1920_1080_60P   */ { IMX179_READOUT_MODE_2, { 17984682, 3440, 1256, { .Interlace = 0, .TimeScale =  60000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX179_S1_3200_2400_30P   */ { IMX179_READOUT_MODE_3, { 17984682, 3440, 2512, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX179_S1_2112_1188_60P   */ { IMX179_READOUT_MODE_4, { 17984682, 3440, 1256, { .Interlace = 0, .TimeScale =  60000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX179_S2_1600_900_60P    */ { IMX179_READOUT_MODE_5, { 17984682, 3440, 1256, { .Interlace = 0, .TimeScale =  60000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX179_S2_1600_1200_30P   */ { IMX179_READOUT_MODE_6, { 17980100, 3440, 1744, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX179_S2_1600_1200_30P_L */ { IMX179_READOUT_MODE_7, { 17984682, 3440, 1256, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX179_S2_1600_1200_60P   */ { IMX179_READOUT_MODE_8, { 17984682, 3440, 1256, { .Interlace = 0, .TimeScale =  60000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX179_S4_800_450_120P    */ { IMX179_READOUT_MODE_9, { 17991778, 3440,  493, { .Interlace = 0, .TimeScale = 120000,  .NumUnitsInTick = 1001}}},
};
