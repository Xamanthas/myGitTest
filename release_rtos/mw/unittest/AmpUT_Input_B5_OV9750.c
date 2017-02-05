 /**
  * @file src/app/sample/unittest/AmpUT_input_OV9750.c
  *
  * Video Encode/Liveview unit test
  *
  * History:
  *    2013/05/02 - [Wisdom Hung] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#include "AmpUnitTest.h"
#include "AmpUT_Input.h"
#include "AmbaDSP.h"
#include <recorder/VideoEnc.h>
#include <AmbaSensor_B5_OV9750.h>

INPUT_ENC_MGT_s VideoEncMgt_B5_OV9750[] = {
    [FHD_AR16_9_P30] = {
        .ForbidMode = 1,
    },
    [FHD_AR16_9_P60] = {
        .ForbidMode = 1,
    },
    [FHD_AR16_9_P120] = {
        .ForbidMode = 1,
    },
    [FHD_AR16_9_HDR_P30] = {
        .ForbidMode = 1,
    },
    [FHD_AR16_9_HDR_P60] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_B5_OV9750_V1_10_1280_960_30P_L2,
        .CaptureWidth = 1280,
        .CaptureHeight = 960,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_B5_OV9750_V1_10_1280_960_25P_L2,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_B5_OV9750_V1_10_1280_960_30P_L2,
        .StillCaptureWidth = 1280,
        .StillCaptureHeight = 960,
        .StillMainWidth = 1280,
        .StillMainHeight = 720,
        .ScrnWidth = 960,
        .ScrnHeight = 540,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 540,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 90,
        .MaxPivWidth = 4096,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 10,
        .MinBitRate = 6,
        .AverageBitRate = 8,
        .ReportRate = 1,
        .OSMode = 1,
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:O",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
    },
    [HD_AR16_9_P60] = {
        .InputMode = AMBA_SENSOR_B5_OV9750_V1_10_1280_960_60P_L2,
        .CaptureWidth = 1280,
        .CaptureHeight = 960,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_B5_OV9750_V1_10_1280_960_50P_L2,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_B5_OV9750_V1_10_1280_960_60P_L2,
        .StillCaptureWidth = 1280,
        .StillCaptureHeight = 960,
        .StillMainWidth = 1280,
        .StillMainHeight = 720,
        .ScrnWidth = 960,
        .ScrnHeight = 540,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 540,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 90,
        .MaxPivWidth = 2560,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 15,
        .MinBitRate = 9,
        .AverageBitRate = 12,
        .ReportRate = 1,
        .OSMode = 0,
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:O",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
    },
    [HD_AR16_9_P120] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_P240] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_HDR_P30] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_HDR_P60] = {
        .ForbidMode = 1,
    },
    [WQHD_AR16_9_P30] = {
        .ForbidMode = 1,
    },
    [WQHD_AR16_9_P60] = {
        .ForbidMode = 1,
    },
    [UHD_AR16_9_P30] = {
        .ForbidMode = 1,
    },
    [UHD_AR16_9_P60] = {
        .ForbidMode = 1,
    },
    [FHD_AR4_3_P30] = {
        .ForbidMode = 1,
    },
    [FHD_AR4_3_P60] = {
        .ForbidMode = 1,
    },
    [FHD_AR4_3_P120] = {
        .ForbidMode = 1,
    },
    [FHD_AR4_3_HDR_P30] = {
        .ForbidMode = 1,
    },
    [FHD_AR4_3_HDR_P60] = {
        .ForbidMode = 1,
    },
    [HD_AR4_3_P30] = {
        .ForbidMode = 1,
    },
    [HD_AR4_3_P60] = {
        .ForbidMode = 1,
    },
    [HD_AR4_3_P120] = {
        .ForbidMode = 1,
    },
    [HD_AR4_3_P240] = {
        .ForbidMode = 1,
    },
    [HD_AR4_3_HDR_P30] = {
        .ForbidMode = 1,
    },
    [HD_AR4_3_HDR_P60] = {
        .ForbidMode = 1,
    },
    [WQHD_AR4_3_P30] = {
        .ForbidMode = 1,
    },
    [WQHD_AR4_3_P60] = {
        .ForbidMode = 1,
    },
    [UHD_AR4_3_P30] = {
        .ForbidMode = 1,
    },
    [UHD_AR4_3_P60] = {
        .ForbidMode = 1,
    },
    [FHD_AR16_9_CFA_HDR_P30] = {
        .ForbidMode = 1,
    },
    [FHD_AR16_9_CFA_HDR_P60] = {
        .ForbidMode = 1,
    },
    [FHD_AR4_3_CFA_HDR_P30] = {
        .ForbidMode = 1,
    },
    [FHD_AR4_3_CFA_HDR_P60] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_CFA_HDR_P30] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_CFA_HDR_P60] = {
        .ForbidMode = 1,
    },
    [HD_AR4_3_CFA_HDR_P30] = {
        .ForbidMode = 1,
    },
    [HD_AR4_3_CFA_HDR_P60] = {
        .ForbidMode = 1,
    },
    [WQHD_AR16_9_HDR_P30] = {
        .ForbidMode = 1,
    },
    [WQHD_AR16_9_HDR_P60] = {
        .ForbidMode = 1,
    },

    /* Still */
    [STILL_AR4_3] = {
        .ForbidMode = 1,
    },
    [STILL_AR16_9] = {
        .ForbidMode = 1,
    },
    [STILL_AR4_3_1] = {
        .ForbidMode = 1,
    },
    [STILL_AR16_9_1] = {
        .ForbidMode = 1,
    },
};

