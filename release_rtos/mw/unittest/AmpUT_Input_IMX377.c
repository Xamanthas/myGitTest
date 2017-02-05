 /**
  * @file src/app/sample/unittest/AmpUT_input_IMX377.c
  *
  * Video Input IMX377
  *
  * Copyright (C) 2015, Ambarella, Inc.
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
#include <AmbaSensor_IMX377_1440M.h>

INPUT_ENC_MGT_s VideoEncMgtIMX377[] = {
    [FHD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_30P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
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
        .Name = "1920x1080P30\0",
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:O",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 0,
    },
    [FHD_AR16_9_P60] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_60P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_60P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
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
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 0,
    },
    [FHD_AR16_9_P120] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_120P,
        .CaptureWidth = 1920,
        .CaptureHeight = 1080,
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 120000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_120P, //TBD
        .TimeScalePAL = 100,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .ScrnWidth = 960,
        .ScrnHeight = 540,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 540,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 90,
        .MaxPivWidth = 1920,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 45,
        .MinBitRate = 27,
        .AverageBitRate = 36,
        .ReportRate = 2,
        .OSMode = 0,
        .Name = "1920x1080P120\0",
        .Desc = " DualStrm:O MJPG:O OSDBlend:X DualHDStrm:O",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                0, 0, 0, 0, 0, 0, 0, \
                0, 1},
            [1] = { 1, 1, 1, 1, \
                0, 0, 0, 0, 0, 0, 0, \
                0, 1},
            [2] = { 1, 1, 1, 1, \
                0, 0, 0, 0, 0, 0, 0, \
                0, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 1,
    },
    [FHD_AR16_9_HDR_P30] = {
        .ForbidMode = 1,
    },
    [FHD_AR16_9_HDR_P60] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_30P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
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
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 1,
    },
    [HD_AR16_9_P60] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_60P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_60P, //TBD
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
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
        .Name = "1280x720P60\0",
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:O",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 1,
    },
    [HD_AR16_9_P120] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_120P,
        .CaptureWidth = 2048,
        .CaptureHeight = 1080,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 120000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_120P, //TBD
        .TimeScalePAL = 100,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .ScrnWidth = 960,
        .ScrnHeight = 540,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 540,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 90,
        .MaxPivWidth = 2048,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 22,
        .MinBitRate = 13,
        .AverageBitRate = 18,
        .ReportRate = 2,
        .OSMode = 0,
        .Name = "1280x720P120\0",
        .Desc = " DualStrm:O MJPG:O OSDBlend:X DualHDStrm:O",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                0, 0, 0, 0, 0, 0, 0, \
                0, 1},
            [1] = { 1, 1, 1, 1, \
                0, 0, 0, 0, 0, 0, 0, \
                0, 1},
            [2] = { 1, 1, 1, 1, \
                0, 0, 0, 0, 0, 0, 0, \
                0, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 1,
    },
    [HD_AR16_9_P240] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_4_240P,
        .CaptureWidth = 1280,
        .CaptureHeight = 720,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 240000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_4_240P, // TBD
        .TimeScalePAL = 200,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .ScrnWidth = 960,
        .ScrnHeight = 540,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 540,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 90,
        .MaxPivWidth = 1280,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 45,
        .MinBitRate = 27,
        .AverageBitRate = 36,
        .ReportRate = 4,
        .OSMode = 0,
        .Name = "1280x720P240\0",
        .Desc = " DualStrm:O MJPG:O OSDBlend:X DualHDStrm:O (PAL only)",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                0, 0, 0, 0, 0, 0, 0, \
                0, 1},
            [1] = { 1, 1, 1, 1, \
                0, 0, 0, 0, 0, 0, 0, \
                0, 1},
            [2] = { 1, 1, 1, 1, \
                0, 0, 0, 0, 0, 0, 0, \
                0, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 1,
    },
    [HD_AR16_9_HDR_P30] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_HDR_P60] = {
        .ForbidMode = 1,
    },
    [UHD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_30P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = 3840,
        .MainHeight = 2160,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
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
        .MaxBitRate = 75,
        .MinBitRate = 45,
        .AverageBitRate = 60,
        .ReportRate = 1,
        .OSMode = 1,
        .Name = "3840x2160P30\0",
        .Desc = " DualStrm:O MJPG:O OSDBlend:O DualHDStrm:X",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 0},
            [1] = { 1, 1, 1, 0, \
                1, 0, 0, 1, 1, 0, 0, \
                1, 0},
            [2] = { 1, 1, 1, 0, \
                1, 0, 0, 1, 1, 0, 0, \
                1, 0},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 1,
    },
    [UHD_AR16_9_P60] = {
        .ForbidMode = 1,
    },
    [FHD_AR4_3_P30] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0A_30P,
        .CaptureWidth = 4000,
        .CaptureHeight = 3000,
        .MainWidth = 1920,
        .MainHeight = 1440,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX377_NUM_MODE,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .ScrnWidth = 960,
        .ScrnHeight = 720,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 720,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 120,
        .MaxPivWidth = 4000,
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
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 0,
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
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0A_30P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2880,
        .MainWidth = 1280,
        .MainHeight = 960,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX377_NUM_MODE,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .ScrnWidth = 960,
        .ScrnHeight = 720,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 720,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 120,
        .MaxPivWidth = 3840,
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
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 1,
    },
    [HD_AR4_3_P60] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_60P,
        .CaptureWidth = 2000,
        .CaptureHeight = 1500,
        .MainWidth = 1280,
        .MainHeight = 960,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_60P, //TBD
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .ScrnWidth = 960,
        .ScrnHeight = 720,
        .ScrnWidthAct = 960,
        .ScrnHeightAct = 720,
        .ThmWidth = 160,
        .ThmHeight = 120,
        .ThmWidthAct = 160,
        .ThmHeightAct = 120,
        .MaxPivWidth = 2000,
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
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
        .DualStrmUptoHD = 1,
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
    [UHD_AR4_3_P30] = {
        .ForbidMode = 1,
    },
    [UHD_AR4_3_P60] = {
        .ForbidMode = 1,
    },
    /* Still */
    /* Still 4:3 4000x3000 , video_preview is 960X720*/
    [STILL_AR4_3] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_3_30P,
        .CaptureWidth = 1312,
        .CaptureHeight = 984,
        .MainWidth = 960,
        .MainHeight = 720,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0_35P,
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
        .Desc = "Raw: 3840x2880 Main: 4000x3000 Scrn: 960x720 Thmb: 160x120",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
        },
        .ForbidMode = 0,
    },
    /* 16:9 4096*2160 , photo_preview is 960X540*/
    [STILL_AR16_9] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_3_30P,
        .CaptureWidth = 1312,
        .CaptureHeight = 738,
        .MainWidth = 960,
        .MainHeight = 540,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_30P,
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
        .Desc = "Raw: 4096x2160 Main: 4096x2160 Scrn: 960x540 Thmb: 160x90",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
        },
        .ForbidMode = 0,
    },
    /* Still 4:3 2304X1728 , video_preview is 960X720*/
    [STILL_AR4_3_1] = {
        .InputMode = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_3_30P,
        .CaptureWidth = 1312,
        .CaptureHeight = 984,
        .MainWidth = 960,
        .MainHeight = 720,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0_35P,
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
        .Desc = "Raw: 3840x2880 Main: 2304x1728 Scrn: 960x720 Thmb: 160x120",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
        },
        .ForbidMode = 0,
    },
    [STILL_AR4_3_1] = {
        .ForbidMode = 1,
    },
    [STILL_AR16_9_1] = {
        .ForbidMode = 1,
    },
};

