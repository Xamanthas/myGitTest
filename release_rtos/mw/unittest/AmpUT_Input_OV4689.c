 /**
  * @file src/app/sample/unittest/AmpUT_input_OV4689.c
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
#include <AmbaSensor_OV4689.h>

INPUT_ENC_MGT_s VideoEncMgtOV4689[] = {
    [FHD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P,
        .CaptureWidth = 2688,
        .CaptureHeight = 1512,
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P,
        .StillCaptureWidth = 2688,
        .StillCaptureHeight = 1512,
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
        .InputMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_60P,
        .CaptureWidth = 2688,
        .CaptureHeight = 1512,
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_50P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_60P,
        .StillCaptureWidth = 2688,
        .StillCaptureHeight = 1512,
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
        .ForbidMode = 1,
    },
    [FHD_AR16_9_HDR_P30] = {
        .InputMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P_HDR,
        .CaptureWidth = 2688,
        .CaptureHeight = 3640,
        .MainWidth = 1920,
        .MainHeight = 1080,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_25P_HDR,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P_HDR,
        .StillCaptureWidth = 2688,
        .StillCaptureHeight = 1512,
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
                1, 0, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
    },
    [FHD_AR16_9_HDR_P60] = {
        .ForbidMode = 1,
    },
    [HD_AR16_9_P30] = {
        .InputMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P,
        .CaptureWidth = 2688,
        .CaptureHeight = 1512,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P,
        .StillCaptureWidth = 2688,
        .StillCaptureHeight = 1512,
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
        .InputMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_60P,
        .CaptureWidth = 2688,
        .CaptureHeight = 1512,
        .MainWidth = 1280,
        .MainHeight = 720,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_50P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_60P,
        .StillCaptureWidth = 2688,
        .StillCaptureHeight = 1512,
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
        .InputMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P,
        .CaptureWidth = 2688,
        .CaptureHeight = 1512,
        .MainWidth = 2560,
        .MainHeight = 1440,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P,
        .StillCaptureWidth = 2688,
        .StillCaptureHeight = 1512,
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
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 30,
        .MinBitRate = 18,
        .AverageBitRate = 24,
        .ReportRate = 1,
        .OSMode = 1,
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
        .InputMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_60P,
        .CaptureWidth = 2688,
        .CaptureHeight = 1512,
        .MainWidth = 2560,
        .MainHeight = 1440,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_50P,
        .TimeScalePAL = 50,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_60P,
        .StillCaptureWidth = 2688,
        .StillCaptureHeight = 1512,
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
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 45,
        .MinBitRate = 27,
        .AverageBitRate = 36,
        .ReportRate = 1,
        .OSMode = 1,
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
        .InputMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P_HDR,
        .CaptureWidth = 2688,
        .CaptureHeight = 3640,
        .MainWidth = 2560,
        .MainHeight = 1440,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_25P_HDR,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P_HDR,
        .StillCaptureWidth = 2688,
        .StillCaptureHeight = 1512,
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
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 30,
        .MinBitRate = 18,
        .AverageBitRate = 24,
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
                1, 0, 1},
        },
        .ForbidMode = 0,
        .PALModeOnly = 0,
    },
    [WQHD_AR16_9_HDR_P60] = {
        .ForbidMode = 1,
    },

    /* Still */
    /* still 4:3 2016x1512, video_preview is 960X720* */
    [STILL_AR4_3] = {
        .InputMode = AMBA_SENSOR_OV4689_V2_10_1344_760_30P,
        .CaptureWidth = 1024,
        .CaptureHeight = 756,
        .MainWidth = 960,
        .MainHeight = 720,
        .Interlace = 0,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_OV4689_V2_10_1344_760_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_4x3,
        .InputStillMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P,
        .StillCaptureWidth = 2016,
        .StillCaptureHeight = 1512,
        .StillMainWidth = 2016,
        .StillMainHeight = 1512,
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
        .Desc = "Raw: 2016x1512 Main: 2016x1512 Scrn: 960x720 Thmb: 160x120",
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
    /* still 16:9 2688x1512, video_preview is 960x540* */
    [STILL_AR16_9] = {
        .InputMode = AMBA_SENSOR_OV4689_V2_10_1344_760_30P,
        .CaptureWidth = 1344,
        .CaptureHeight = 756,
        .MainWidth = 960,
        .MainHeight = 540,
        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .InputPALMode = AMBA_SENSOR_OV4689_V2_10_1344_760_25P,
        .TimeScalePAL = 25,
        .TickPerPicturePAL = 1,
        .Interlace = 0,
        .AspectRatio = VAR_16x9,
        .InputStillMode = AMBA_SENSOR_OV4689_V1_10_2688_1512_30P,
        .StillCaptureWidth = 2688,
        .StillCaptureHeight = 1512,
        .StillMainWidth = 2688,
        .StillMainHeight = 1512,
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
        .Desc = "Raw: 2688x1512 Main: 2688x1512 Scrn: 960x540 Thmb: 160x90",
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
    [STILL_AR16_9_1] = {
        .ForbidMode = 1,
    },
};
