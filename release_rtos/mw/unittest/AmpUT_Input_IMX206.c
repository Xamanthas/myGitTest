 /**
  * @file src/app/sample/unittest/AmpUT_input_IMX206.c
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
#include <AmbaSensor_IMX206.h>

INPUT_ENC_MGT_s VideoEncMgtIMX206[] = {
    [FHD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_30P,
        .CaptureWidth = 2304,
        .CaptureHeight = 1296,
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_30P,
        .StillCaptureWidth = 2304,
        .StillCaptureHeight = 1296,
        .StillMainWidth = 2304,
        .StillMainHeight = 1296,
        .ScrnWidth = 960,
        .ScrnHeight = 540,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 540,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 90,
        .MaxPivWidth = 3840,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 15,
        .MinBitRate = 9,
        .AverageBitRate = 12,
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
    [FHD_AR16_9_P60] = {
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2_60P,
        .CaptureWidth = 2304,
        .CaptureHeight = 1296,
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2_50P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2_60P,
        .StillCaptureWidth = 2304,
        .StillCaptureHeight = 1296,
        .StillMainWidth = 2304,
        .StillMainHeight = 1296,
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
        .MaxBitRate = 21,
        .MinBitRate = 13,
        .AverageBitRate = 18,
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
    [FHD_AR16_9_P120] = {
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_3B_120P,
        .CaptureWidth = 1536,
        .CaptureHeight = 864,
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 120000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_3B_100P,
        .TimeScalePAL = 100,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_3B_120P,
        .StillCaptureWidth = 1536,
        .StillCaptureHeight = 864,
        .StillMainWidth = 1920,
        .StillMainHeight = 1080,
        .ScrnWidth = 960,
        .ScrnHeight = 540,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 540,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 90,
        .GopM = 1,
        .DualValid = 0,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 45,
        .MinBitRate = 27,
        .AverageBitRate = 36,
        .ReportRate = 2,
        .OSMode = 0,
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:O (PAL only)",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
        },
        .ForbidMode = 1, //MW not officailly support FHDp120 binning mode
        .PALModeOnly = 1,
    },
    [FHD_AR16_9_HDR_P30] = {
        .ForbidMode = 1,
    },
    [FHD_AR16_9_HDR_P60] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_30P,
        .CaptureWidth = 2304,
        .CaptureHeight = 1296,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_30P,
        .StillCaptureWidth = 2304,
        .StillCaptureHeight = 1296,
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
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2_60P,
        .CaptureWidth = 2304,
        .CaptureHeight = 1296,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2_50P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2_60P,
        .StillCaptureWidth = 2304,
        .StillCaptureHeight = 1296,
        .StillMainWidth = 1920,
        .StillMainHeight = 1080,
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
    [HD_AR16_9_P120] = {
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_3B_120P,
        .CaptureWidth = 1536,
        .CaptureHeight = 864,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 120000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_3B_100P,
        .TimeScalePAL = 100,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_3B_120P,
        .StillCaptureWidth = 1536,
        .StillCaptureHeight = 864,
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
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 22,
        .MinBitRate = 13,
        .AverageBitRate = 18,
        .ReportRate = 2,
        .OSMode = 0,
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:O",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
    },
    [HD_AR16_9_P240] = {
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_4_240P,
        .CaptureWidth = 1536,
        .CaptureHeight = 288,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 240000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_4_200P,
        .TimeScalePAL = 200,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_4_240P,
        .StillCaptureWidth = 1536,
        .StillCaptureHeight = 288,
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
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 45,
        .MinBitRate = 27,
        .AverageBitRate = 36,
        .ReportRate = 4,
        .OSMode = 0,
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:O (PAL only)",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
        },
        .ForbidMode = 1, //poor quality
        .PALModeOnly = 1,
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
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_30P,
        .CaptureWidth = 2304,
        .CaptureHeight = 1728,
        .MainWidth = 1920,
        .MainHeight = 1440,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_30P,
        .StillCaptureWidth = 2304,
        .StillCaptureHeight = 1728,
        .StillMainWidth = 1920,
        .StillMainHeight = 1440,
        .ScrnWidth = 960,
        .ScrnHeight = 720,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 720,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 120,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 22,
        .MinBitRate = 13,
        .AverageBitRate = 18,
        .ReportRate = 1,
        .OSMode = 1,
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:X",
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
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_30P,
        .CaptureWidth = 2304,
        .CaptureHeight = 1728,
        .MainWidth = 1280,
        .MainHeight = 960,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_30P,
        .StillCaptureWidth = 2304,
        .StillCaptureHeight = 1728,
        .StillMainWidth = 4000,
        .StillMainHeight = 3000,
        .ScrnWidth = 960,
        .ScrnHeight = 720,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 720,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 120,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 15,
        .MinBitRate = 10,
        .AverageBitRate = 13,
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
    #ifdef CONFIG_SOC_A9
        .ForbidMode = 0,
        .PALModeOnly = 0,
    #else
        .ForbidMode = 1,
        .PALModeOnly = 0,
    #endif
    },
    [HD_AR4_3_P60] = {
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_3_60P,
        .CaptureWidth = 1536,
        .CaptureHeight = 1152,
        .MainWidth = 1280,
        .MainHeight = 960,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_3_50P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_3_60P,
        .StillCaptureWidth = 1536,
        .StillCaptureHeight = 1152,
        .StillMainWidth = 4000,
        .StillMainHeight = 3000,
        .ScrnWidth = 960,
        .ScrnHeight = 720,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 720,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 120,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 18,
        .MinBitRate = 12,
        .AverageBitRate = 15,
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
    #ifdef CONFIG_SOC_A9
        .ForbidMode = 0,
        .PALModeOnly = 0,
    #else
        .ForbidMode = 1,
        .PALModeOnly = 0,
    #endif
    },
    [HD_AR4_3_P120] = {
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_4_120P,
        .CaptureWidth = 1536,
        .CaptureHeight = 384,
        .MainWidth = 1280,
        .MainHeight = 960,
        .TimeScale = 120000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_4_100P,
        .TimeScalePAL = 100,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_4_120P,
        .StillCaptureWidth = 1536,
        .StillCaptureHeight = 384,
        .StillMainWidth = 4000,
        .StillMainHeight = 3000,
        .ScrnWidth = 960,
        .ScrnHeight = 720,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 720,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 120,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 27,
        .MinBitRate = 18,
        .AverageBitRate = 24,
        .ReportRate = 2,
        .OSMode = 0,
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:O",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1, 0},
        },
    #ifdef CONFIG_SOC_A9
        .ForbidMode = 0,
        .PALModeOnly = 0,
    #else
        .ForbidMode = 1,
        .PALModeOnly = 0,
    #endif
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
    /* Still 4:3 4608X3456p10(16M) , photo_preview is 960X720*/
    [STILL_AR4_3] = {
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_30P,
        .CaptureWidth = 2304,
        .CaptureHeight = 1728,
        .MainWidth = 960,
        .MainHeight = 720,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_25P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_0_10P,
        .StillCaptureWidth = 4608,
        .StillCaptureHeight = 3456,
        .StillMainWidth = 4608,
        .StillMainHeight = 3456,
        .ScrnWidth = 960,
        .ScrnHeight = 720,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 720,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 120,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 9,
        .MinBitRate = 6,
        .AverageBitRate = 8,
        .ReportRate = 1,
        .OSMode = 1,
        .Desc = "Raw: 4608x3456 Main: 4608x3456 Scrn: 960x720 Thmb: 160x120",
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
    /* Still 16:9 3840X2160p10(8M) , photo_preview is 960X540*/
    [STILL_AR16_9] = {
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_30P,
        .CaptureWidth = 2304,
        .CaptureHeight = 1296,
        .MainWidth = 960,
        .MainHeight = 540,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_25P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_0_10P,
        .StillCaptureWidth = 4608,
        .StillCaptureHeight = 2592,
        .StillMainWidth = 3840,
        .StillMainHeight = 2160,
        .ScrnWidth = 960,
        .ScrnHeight = 540,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 540,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 90,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 9,
        .MinBitRate = 6,
        .AverageBitRate = 8,
        .ReportRate = 1,
        .OSMode = 0,
        .Desc = "Raw: 4608x2592 Main: 3840x2160 Scrn: 960x540 Thmb: 160x90",
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
    [STILL_AR4_3_1] = {
        .ForbidMode = 1,
    },
    /* Still 16:9 4608X2592p10 , photo_preview is 960X540*/
    [STILL_AR16_9_1] = {
        .InputMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_30P,
        .CaptureWidth = 2304,
        .CaptureHeight = 1296,
        .MainWidth = 960,
        .MainHeight = 540,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_2A_25P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX206_TYPE_2_3_MODE_0_10P,
        .StillCaptureWidth = 4608,
        .StillCaptureHeight = 2592,
        .StillMainWidth = 4608,
        .StillMainHeight = 2592,
        .ScrnWidth = 960,
        .ScrnHeight = 540,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 540,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 90,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 9,
        .MinBitRate = 6,
        .AverageBitRate = 8,
        .ReportRate = 1,
        .OSMode = 0,
        .Desc = "Raw: 4608x2592 Main: 4608x2592 Scrn: 960x540 Thmb: 160x90",
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
};

