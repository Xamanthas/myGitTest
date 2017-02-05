 /**
  * @file src/app/sample/unittest/AmpUT_input_IMX117.c
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
#include <AmbaSensor_IMX117.h>

INPUT_ENC_MGT_s VideoEncMgtIMX117[] = {
    [FHD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_30P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_30P,
        .StillCaptureWidth = 3840,
        .StillCaptureHeight = 2160,
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
        .MaxPivWidth = 3840,
#ifdef CONFIG_SOC_A9
        .GopM = 3,
#else
        .GopM = 1,
#endif
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 15,
        .MinBitRate = 9,
        .AverageBitRate = 12,
        .ReportRate = 1,
        .OSMode = 1,
        .Name = "1920x1080P30\0",
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
    [FHD_AR16_9_P60] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_1_60P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_1_50P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_1_60P,
        .StillCaptureWidth = 3840,
        .StillCaptureHeight = 2160,
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
        .MaxBitRate = 21,
        .MinBitRate = 13,
        .AverageBitRate = 18,
        .ReportRate = 1,
        .OSMode = 1,
        .Name = "1920x1080P60\0",
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
    [FHD_AR16_9_P120] = {
#ifdef CONFIG_SOC_A9
        .InputMode = AMBA_SENSOR_IMX117_TYPE_4_4_MODE_1_120P,
        .CaptureWidth = 2304,
        .CaptureHeight = 1296,
#else
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_10_120P,
        .CaptureWidth = 1920, //Keep 3840*2160 FOV
        .CaptureHeight = 1080, //Keep 3840*2160 FOV
#endif
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 120000,
        .TickPerPicture = 1001,
#ifdef CONFIG_SOC_A9
        .InputPALMode = AMBA_SENSOR_IMX377_NUM_MODE,
#else
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_10_100P,
#endif
        .TimeScalePAL = 100,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
#ifdef CONFIG_SOC_A9
        .InputStillMode = AMBA_SENSOR_IMX377_NUM_MODE,
#else
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_10_120P,
#endif
        .StillCaptureWidth = 1920,
        .StillCaptureHeight = 1080,
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
        .Name = "1920x1080P120\0",
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
        .ForbidMode = 0,
        .PALModeOnly = 1,
    },
    [FHD_AR16_9_HDR_P30] = {
        .ForbidMode = 1,
    },
    [FHD_AR16_9_HDR_P60] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_30P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_30P,
        .StillCaptureWidth = 3840,
        .StillCaptureHeight = 2160,
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
        .MaxPivWidth = 4096,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 10,
        .MinBitRate = 6,
        .AverageBitRate = 8,
        .ReportRate = 1,
        .OSMode = 1,
        .Name = "1280x720P30\0",
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
    [HD_AR16_9_P60] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_1_60P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_1_50P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_1_60P,
        .StillCaptureWidth = 3840,
        .StillCaptureHeight = 2160,
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
        .Name = "1280x720P60\0",
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
    [HD_AR16_9_P120] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4_120P,
        .CaptureWidth = 1332,
        .CaptureHeight = 1000,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 120000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4_100P,
        .TimeScalePAL = 100,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4_120P,
        .StillCaptureWidth = 1332,
        .StillCaptureHeight = 1000,
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
        .Name = "1280x720P120\0",
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
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4A_240P,
        .CaptureWidth = 1920, //Keep 3840*2160 FOV
        .CaptureHeight = 720, //Keep 3840*2160 FOV
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 240000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4A_200P,
        .TimeScalePAL = 200,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4A_240P,
        .StillCaptureWidth = 1920,
        .StillCaptureHeight = 720,
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
        .Name = "1280x720P240\0",
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:X (PAL only)",
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
        .PALModeOnly = 1,
    },
    [HD_AR16_9_HDR_P30] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_HDR_P60] = {
        .ForbidMode = 1,
    },
    [WQHD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_30P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 2560,
        .MainHeight = 1440,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_30P,
        .StillCaptureWidth = 3840,
        .StillCaptureHeight = 2160,
        .StillMainWidth = 2560,
        .StillMainHeight = 1440,
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
        .MaxBitRate = 30,
        .MinBitRate = 18,
        .AverageBitRate = 24,
        .ReportRate = 1,
        .OSMode = 1,
        .Name = "2560x1440P30\0",
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
    [WQHD_AR16_9_P60] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_1_60P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 2560,
        .MainHeight = 1440,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_1_50P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_1_60P,
        .StillCaptureWidth = 3840,
        .StillCaptureHeight = 2160,
        .StillMainWidth = 2560,
        .StillMainHeight = 1440,
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
        .DualValid = 0,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 45,
        .MinBitRate = 27,
        .AverageBitRate = 36,
        .ReportRate = 1,
        .OSMode = 1,
        .Name = "2560x1440P60\0",
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:O",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 0, 0, 1, 1, 0, 0, \
                0, 1, 0},
            [1] = { 1, 1, 1, 1, \
                1, 0, 0, 1, 1, 0, 0, \
                0, 1, 0},
            [2] = { 1, 1, 1, 1, \
                1, 0, 0, 1, 1, 0, 0, \
                0, 1, 0},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 1,
    },
    [UHD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_30P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 3840,
        .MainHeight = 2160,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_30P,
        .StillCaptureWidth = 3840,
        .StillCaptureHeight = 2160,
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
        .DualValid = 0,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 75,
        .MinBitRate = 45,
        .AverageBitRate = 60,
        .ReportRate = 1,
        .OSMode = 1,
        .Name = "3840x2160P30\0",
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:X (A9 Only)",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 0, 0},
            [1] = { 1, 1, 1, 0, \
                1, 0, 0, 1, 1, 0, 0, \
                1, 0, 0},
            [2] = { 1, 1, 1, 0, \
                1, 0, 0, 1, 1, 0, 0, \
                1, 0, 0},
        },
#ifdef CONFIG_SOC_A9
        .ForbidMode = 0,
        .PALModeOnly = 0,
#else
        .ForbidMode = 1,
        .PALModeOnly = 0,
#endif
    },
    [UHD_AR16_9_P60] = {
        .ForbidMode = 1,
    },
    [FHD_AR4_3_P30] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_30P,
        .CaptureWidth = 4000,
        .CaptureHeight = 3000,
        .MainWidth = 1920,
        .MainHeight = 1440,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_30P,
        .StillCaptureWidth = 4000,
        .StillCaptureHeight = 3000,
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
        .Name = "1920x1440P30\0",
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
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_30P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2880,
        .MainWidth = 1280,
        .MainHeight = 960,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_30P,
        .StillCaptureWidth = 3840,
        .StillCaptureHeight = 2880,
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
        .Name = "1280x960P30\0",
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
    [HD_AR4_3_P60] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_2A_60P,
        .CaptureWidth = 2000,
        .CaptureHeight = 1500,
        .MainWidth = 1280,
        .MainHeight = 960,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_2A_50P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_2A_60P,
        .StillCaptureWidth = 2000,
        .StillCaptureHeight = 1500,
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
        .Name = "1280x960P60\0",
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
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4_120P,
        .CaptureWidth = 1332,
        .CaptureHeight = 999,
        .MainWidth = 1280,
        .MainHeight = 960,
        .TimeScale = 120000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4_100P,
        .TimeScalePAL = 100,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_4_120P,
        .StillCaptureWidth = 3840,
        .StillCaptureHeight = 2880,
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
        .Name = "1280x960P120\0",
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
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_30P,
        .CaptureWidth = 4000,
        .CaptureHeight = 3000,
        .MainWidth = 2560,
        .MainHeight = 1920,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_30P,
        .StillCaptureWidth = 4000,
        .StillCaptureHeight = 3000,
        .StillMainWidth = 2560,
        .StillMainHeight = 1920,
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
        .MaxBitRate = 45,
        .MinBitRate = 27,
        .AverageBitRate = 36,
        .ReportRate = 1,
        .OSMode = 1,
        .Name = "2560x1920P30\0",
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
        .DualStrmUptoHD = 1,
    },
    [WQHD_AR4_3_P60] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_2A_60P,
        .CaptureWidth = 1920,
        .CaptureHeight = 1440,
        .MainWidth = 2560,
        .MainHeight = 1920,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_2A_50P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_2A_60P,
        .StillCaptureWidth = 1920,
        .StillCaptureHeight = 1440,
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
        .MaxBitRate = 45,
        .MinBitRate = 27,
        .AverageBitRate = 36,
        .ReportRate = 1,
        .OSMode = 1,
        .Name = "2560x1920P60\0",
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
        .ForbidMode = 1,
        .PALModeOnly = 0,
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
    /* Still 4:3 4000x3000 , video_preview is 960X720*/
    [STILL_AR4_3] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_3_30P,
        .CaptureWidth = 1312,
        .CaptureHeight = 984,
        .MainWidth = 960,
        .MainHeight = 720,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_30P,
        .StillCaptureWidth = 3840,
        .StillCaptureHeight = 2880,
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
        .MaxBitRate = 9,
        .MinBitRate = 6,
        .AverageBitRate = 8,
        .ReportRate = 1,
        .OSMode = 0,
        .Desc = "Raw: 3840x2880 Main: 4000x3000 Scrn: 960x720 Thmb: 160x120",
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
    /* 16:9 4096*2160 , photo_preview is 960X540*/
    [STILL_AR16_9] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_3_30P,
        .CaptureWidth = 1312,
        .CaptureHeight = 738,
        .MainWidth = 960,
        .MainHeight = 540,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
#ifdef CONFIG_SOC_A9
        .InputPALMode = AMBA_SENSOR_IMX377_NUM_MODE,
#else
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_2A_25P,
#endif
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_30P,
        .StillCaptureWidth = 4096,
        .StillCaptureHeight = 2160,
        .StillMainWidth = 4096,
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
        .OSMode = 1,
        .Desc = "Raw: 4096x2160 Main: 4096x2160 Scrn: 960x540 Thmb: 160x90",
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
    /* Still 4:3 2304X1728 , video_preview is 960X720*/
    [STILL_AR4_3_1] = {
        .InputMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_3_30P,
        .CaptureWidth = 1312,
        .CaptureHeight = 984,
        .MainWidth = 960,
        .MainHeight = 720,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0_30P,
        .StillCaptureWidth = 3840,
        .StillCaptureHeight = 2880,
        .StillMainWidth = 2304,
        .StillMainHeight = 1728,
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
        .OSMode = 0,
        .Desc = "Raw: 3840x2880 Main: 2304x1728 Scrn: 960x720 Thmb: 160x120",
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
    [STILL_AR16_9_1] = {
        .ForbidMode = 1,
    },
};


