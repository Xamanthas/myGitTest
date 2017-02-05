/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_IMX317Table.c
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of SONY IMX317 CMOS sensor with LVDS interface
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaDSP.h"
#include "AmbaSensor.h"
#include "AmbaSensor_IMX317.h"

/*-----------------------------------------------------------------------------------------------*\
 * Sensor Device Information
\*-----------------------------------------------------------------------------------------------*/
const AMBA_SENSOR_DEVICE_INFO_s IMX317DeviceInfo = {
    .UnitCellWidth          = 1.62,
    .UnitCellHeight         = 1.62,
    .NumTotalPixelCols      = 3864,
    .NumTotalPixelRows      = 2218,
    .NumEffectivePixelCols  = 3864,
    .NumEffectivePixelRows  = 2202,
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
 * Readout drive pluse timing register settings
\*-----------------------------------------------------------------------------------------------*/
IMX317_PSTMG_REG_s IMX317PlstmgRegTable[IMX317_NUM_READOUT_PSTMG_REG] = {
    {0x004c, 0x00}, /* [7:0]: PLSTMG01 */
    {0x004d, 0x03}, /* [1:0]: PLSTMG02 */
    {0x031c, 0x1a}, /* [7:0]: PLSTMG03_LSB */
    {0x031d, 0x00}, /* [7:0]: PLSTMG03_MSB */
    {0x0502, 0x02}, /* [7:0]: PLSTMG04 */
    {0x0529, 0x0e}, /* [7:0]: PLSTMG05 */
    {0x052a, 0x0e}, /* [7:0]: PLSTMG06 */
    {0x052b, 0x0e}, /* [7:0]: PLSTMG07 */
    {0x0538, 0x0e}, /* [7:0]: PLSTMG08 */
    {0x0539, 0x0e}, /* [7:0]: PLSTMG09 */
    {0x0553, 0x00}, /* [4:0]: PLSTMG10 */
    {0x057d, 0x05}, /* [4:0]: PLSTMG11 */
    {0x057f, 0x05}, /* [4:0]: PLSTMG12 */
    {0x0581, 0x04}, /* [4:0]: PLSTMG13 */
    {0x0583, 0x76}, /* [7:0]: PLSTMG14 */
    {0x0587, 0x01}, /* [7:0]: PLSTMG15 */
    {0x05bb, 0x0e}, /* [7:0]: PLSTMG16 */
    {0x05bc, 0x0e}, /* [7:0]: PLSTMG17 */
    {0x05bd, 0x0e}, /* [7:0]: PLSTMG18 */
    {0x05be, 0x0e}, /* [7:0]: PLSTMG19 */
    {0x05bf, 0x0e}, /* [7:0]: PLSTMG20 */
    {0x066e, 0x00}, /* [4:0]: PLSTMG21 */
    {0x066f, 0x00}, /* [4:0]: PLSTMG22 */
    {0x0670, 0x00}, /* [4:0]: PLSTMG23 */
    {0x0671, 0x00}, /* [4:0]: PLSTMG24 */
};

/*-----------------------------------------------------------------------------------------------*\
 * Sensor register settings of each readout modes
\*-----------------------------------------------------------------------------------------------*/
IMX317_MODE_REG_s IMX317ModeRegTable[IMX317_NUM_READOUT_MODE_REG] = {
    /*       [              Type 1/2.5 8.51M               ]   */
    /* Addr,     C,    0,    1,    2,    3,    4,    5,    6   */
    {0x0003, {0x22, 0x22, 0x00, 0x33, 0x22, 0x22, 0x33, 0x33}}, /* [3:0]: STBLVDS, [7:0]: LANESEL */
    {0x0004, {0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x03, 0x04}}, /* [7:0]: MDSEL1 */
    {0x0005, {0x07, 0x07, 0x01, 0x27, 0x21, 0x61, 0x31, 0x31}}, /* [7:0]: MDSEL2 */
    {0x0006, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDSEL3 */
    {0x0007, {0x02, 0x02, 0x02, 0x11, 0x11, 0x19, 0x09, 0x02}}, /* [7:0]: MDSEL4 */
    {0x000e, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: SVR_LSB */
    {0x000f, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: SVR_MSB */
    {0x001a, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [0]: MDVREV */
    {0x00e2, {0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x03, 0x04}}, /* [7:0]: VCOUTMODE */
    {0x00ee, {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}}, /* [7:0]: PSMOVEN */
    {0x0342, {0xff, 0xff, 0x0a, 0xff, 0x0a, 0x0a, 0x0a, 0x0a}}, /* [7:0]: MDPLS01_LSB */
    {0x0343, {0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS01_MSB */
    {0x0344, {0xff, 0xff, 0x16, 0xff, 0x1a, 0x1b, 0x1b, 0x1a}}, /* [7:0]: MDPLS02_LSB */
    {0x0345, {0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}}, /* [7:0]: MDPLS02_MSB */
    {0x0528, {0x0f, 0x0f, 0x0e, 0x0f, 0x0e, 0x0e, 0x0e, 0x0e}}, /* [7:0]: MDPLS03 */
    {0x0554, {0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* [4:0]: MDPLS04 */
    {0x0555, {0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01}}, /* [4:0]: MDPLS05 */
    {0x0556, {0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01}}, /* [4:0]: MDPLS06 */
    {0x0557, {0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01}}, /* [4:0]: MDPLS07 */
    {0x0558, {0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01}}, /* [4:0]: MDPLS08 */
    {0x0559, {0x1f, 0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00}}, /* [4:0]: MDPLS09 */
    {0x055a, {0x1f, 0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00}}, /* [4:0]: MDPLS10 */
    {0x05ba, {0x0f, 0x0f, 0x0e, 0x0f, 0x0e, 0x0e, 0x0e, 0x0e}}, /* [7:0]: MDPLS11 */
    {0x066a, {0x00, 0x00, 0x1b, 0x00, 0x1b, 0x1b, 0x1b, 0x1b}}, /* [4:0]: MDPLS12 */
    {0x066b, {0x00, 0x00, 0x1a, 0x00, 0x1a, 0x19, 0x19, 0x19}}, /* [4:0]: MDPLS13 */
    {0x066c, {0x00, 0x00, 0x19, 0x00, 0x19, 0x17, 0x17, 0x17}}, /* [4:0]: MDPLS14 */
    {0x066d, {0x00, 0x00, 0x17, 0x00, 0x17, 0x17, 0x17, 0x17}}, /* [4:0]: MDPLS15 */
};

const AMBA_SENSOR_OUTPUT_INFO_s IMX317OutputInfo[IMX317_NUM_READOUT_MODE] = {
/*IMX317_TYPE_2_5_MODE_C */ {576000000,  6, 12, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 3864, 2218, { 12, 40, 3840, 2162}, {0}},
/*IMX317_TYPE_2_5_MODE_0 */ {576000000,  6, 12, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 3864, 2218, { 12, 40, 3840, 2160}, {0}},
/*IMX317_TYPE_2_5_MODE_1 */ {576000000, 10, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 3864, 2218, { 12, 40, 3840, 2160}, {0}},
/*IMX317_TYPE_2_5_MODE_2 */ {576000000,  4, 12, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1932, 1102, {  6, 18, 1920, 1080}, {0}},
/*IMX317_TYPE_2_5_MODE_3 */ {576000000,  6, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1932, 1102, {  6, 18, 1920, 1080}, {0}},
/*IMX317_TYPE_2_5_MODE_4 */ {576000000,  6, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1932, 1102, {  6, 18, 1920, 1080}, {0}},
/*IMX317_TYPE_2_5_MODE_5 */ {576000000,  4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1288,  738, {  4, 14, 1280,  720}, {0}},
/*IMX317_TYPE_2_5_MODE_6 */ {576000000,  4, 10, AMBA_DSP_BAYER_RG, {AMBA_DSP_PHASE_SHIFT_MODE_0, AMBA_DSP_PHASE_SHIFT_MODE_0}, 1288,  550, {  4,  8, 1280,  540}, {0}},
};

const AMBA_SENSOR_INPUT_INFO_s IMX317InputInfoNormalReadout[IMX317_NUM_READOUT_MODE] = {
/*IMX317_TYPE_2_5_MODE_C */ {{  12,  42, 3840, 2162}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX317_TYPE_2_5_MODE_0 */ {{  12,  42, 3840, 2160}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX317_TYPE_2_5_MODE_1 */ {{  12,  42, 3840, 2160}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX317_TYPE_2_5_MODE_2 */ {{  12,  42, 3840, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX317_TYPE_2_5_MODE_3 */ {{  12,  42, 3840, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX317_TYPE_2_5_MODE_4 */ {{  12,  42, 3840, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX317_TYPE_2_5_MODE_5 */ {{  12,  42, 3840, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 3}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  3}, 1},
/*IMX317_TYPE_2_5_MODE_6 */ {{  12,  44, 3840, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 3}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  4}, 1},
};

const AMBA_SENSOR_INPUT_INFO_s IMX317InputInfoInversionReadout[IMX317_NUM_READOUT_MODE] = {
/*IMX317_TYPE_2_5_MODE_C */ {{  12,  42, 3840, 2162}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX317_TYPE_2_5_MODE_0 */ {{  12,  42, 3840, 2160}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX317_TYPE_2_5_MODE_1 */ {{  12,  42, 3840, 2160}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1, 1}, {  AMBA_SENSOR_SUBSAMPLE_TYPE_NORMAL, 1,  1}, 1},
/*IMX317_TYPE_2_5_MODE_2 */ {{  12,  42, 3840, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX317_TYPE_2_5_MODE_3 */ {{  12,  42, 3840, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX317_TYPE_2_5_MODE_4 */ {{  12,  42, 3840, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 2}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  2}, 1},
/*IMX317_TYPE_2_5_MODE_5 */ {{  12,  42, 3840, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 3}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  3}, 1},
/*IMX317_TYPE_2_5_MODE_6 */ {{  12,  44, 3840, 2160}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1, 3}, { AMBA_SENSOR_SUBSAMPLE_TYPE_BINNING, 1,  4}, 1},
};

const IMX317_MODE_INFO_s IMX317ModeInfoList[AMBA_SENSOR_IMX317_NUM_MODE] = {
/*AMBA_SENSOR_IMX317_TYPE_2_5_MODE_C_30P  */ { IMX317_TYPE_2_5_MODE_C, { 72000000, 528, 2, 4550, 1, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX317_TYPE_2_5_MODE_0_30P  */ { IMX317_TYPE_2_5_MODE_0, { 72000000, 528, 2, 4550, 1, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX317_TYPE_2_5_MODE_1_60P  */ { IMX317_TYPE_2_5_MODE_1, { 72000000, 264, 2, 4550, 1, { .Interlace = 0, .TimeScale =  60000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX317_TYPE_2_5_MODE_2_60P  */ { IMX317_TYPE_2_5_MODE_2, { 72000000, 520, 2, 2310, 1, { .Interlace = 0, .TimeScale =  60000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX317_TYPE_2_5_MODE_3_120P */ { IMX317_TYPE_2_5_MODE_3, { 72000000, 264, 2, 2275, 1, { .Interlace = 0, .TimeScale = 120000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX317_TYPE_2_5_MODE_4_30P  */ { IMX317_TYPE_2_5_MODE_4, { 72000000, 264, 2, 9100, 4, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX317_TYPE_2_5_MODE_5_120P */ { IMX317_TYPE_2_5_MODE_5, { 72000000, 364, 2, 1650, 1, { .Interlace = 0, .TimeScale = 120000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX317_TYPE_2_5_MODE_5_60P  */ { IMX317_TYPE_2_5_MODE_5, { 72000000, 364, 2, 3300, 2, { .Interlace = 0, .TimeScale =  60000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX317_TYPE_2_5_MODE_6_240P */ { IMX317_TYPE_2_5_MODE_6, { 72000000, 260, 2, 1155, 1, { .Interlace = 0, .TimeScale = 240000,  .NumUnitsInTick = 1001}}},
/*AMBA_SENSOR_IMX317_TYPE_2_5_MODE_6_30P  */ { IMX317_TYPE_2_5_MODE_6, { 72000000, 260, 2, 9240, 8, { .Interlace = 0, .TimeScale =  30000,  .NumUnitsInTick = 1001}}},
};
