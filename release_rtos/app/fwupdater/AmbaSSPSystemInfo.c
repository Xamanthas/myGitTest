/**
 * @file src/app/sample/sspunittest/src/AmbaSPPSystemInfo.c
 *
 * SSP UnitTest System Information
 *
 * History:
 *    2013/02/25 - [Peter Weng] created file
 *
 * Copyright (C) 2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "AmbaSSPSystemInfo.h"
#include "AmbaDSP_StillCapture.h"

AMBA_SSP_SYSTEM_SETTING_s AmbaSSPSystemInfo = {
    .AudioClk = 12288000,
    .CoreClk = 396000000,
    .IDspClk = 504000000,
    .CortexClk = 800000000,
    .DdrClk = 600000000,

    /* DSP working buffer */
    .DspLogSize = 0x20000,
    .DspLogAddr = 0x0,
    .DspWorkSize = (150<<20),
    .DspWorkAddr = 0x0,

    /* IK config */
    .VideoCtxNum = 1,
    .VideoCfgNum = 2,
    .StillCtxNum = 1,
    .StillCfgNum = 1,
    .DecodeCtxNum = 0,
    .DecodeCfgNum = 0,
    .IKWorkSize = 0,
    .IKWorkAddr = 0x0,

    /* BS/Desc Config */
    .JpegBSSize = 20<<20,
    .JpegBSAddr = 0x0,
    .JpegDescSize = 32*128,
    .JpegDescAddr = 0x0,
    .H264BSSize = 30<<20,
    .H264BSAddr = 0x0,

    /* DSP mode */
    .CurBootMode = 0xFF,
    .EventCfgInit = 0,
    .VideoPipeline = SSP_VIDEO_PIPE_EXPRESS_OS,

    /* Vin */
    .VinCapWidth = 2688,
    .VinCapHeight = 1512,
    .VinCapX = 0,
    .VinCapY = 0,

    /* Vout setting */
    .PrevWidthA = 960,
    .PrevHeightA = 480,
    .PrevActWidthA = 960,
    .PrevActHeightA = 360,
    .PrevWidthB = 1920,
    .PrevHeightB = 1080,
    .PrevActWidthB = 1920,
    .PrevActHeightB = 1080,
    .FratePrevA = {
        .Interlace = 0,
        .TimeScale = 60000,
        .NumUnitsInTick = 1001,
    },
    .FratePrevB = {
        .Interlace = 0,
        .TimeScale = 60000,
        .NumUnitsInTick = 1001,
    },
    .PrevRotateA = AMBA_DSP_ROTATE_0,
    .PrevRotateB = AMBA_DSP_ROTATE_0,
    .LcdDevId = 0,
    .TVDevId = 0,

    /* Encode setting */
    .EncodeSrc = 1, //0:Vin 1:Mem
    .EncodePurpose = 0, //0:Video 1:Still
    .EncodeStillAlgoMode = AMBA_DSP_STILL_FAST_MODE,
    .EncodeStillRawCapCtrl = 0,
    .EncodeStillRawCapNum = 1,

    .EncodeInputFmt[0] = 0,
    .EncodeInputRawDataBits = 0,
    .EncodeInputRawBayer = 10,
    .EncodeInputWidth[0] = 1920,
    .EncodeInputHeight[0] = 1080,
    .EncodeInputAddr[0] = 0x0,
    .EncodeInputName[0][0] = '\0',
    .EncodeOutputName[0][0] = '\0',
    .EncodeOutputMainFmt[0] = 1, //0:UnCompressRaw 1:Yuv422 2:Yuv420 3:CompressRaw
    .EncodePreviewAEnable = 1, // FIXME: should comes from vout setting,,
    .EncodePreviewBEnable = 0, // FIXME: should comes from vout setting,,

    .StrmNum = 1,
    .StrmFrate[0] = {
        .Interlace = 0,
        .TimeScale = 60000,
        .NumUnitsInTick = 1001,
    },
    .StrmFrate[1] = {
        .Interlace = 0,
        .TimeScale = 60000,
        .NumUnitsInTick = 1001,
    },
    .StrmFrate[2] = {
        .Interlace = 0,
        .TimeScale = 60000,
        .NumUnitsInTick = 1001,
    },
    .StrmFtm[0] = 0,
    .StrmFtm[1] = 0,
    .StrmRot[0] = AMBA_DSP_ROTATE_0,
    .StrmRot[1] = AMBA_DSP_ROTATE_0,
    .StrmFrateDivisor[0] = 0,
    .StrmFrateDivisor[1] = 0,
    .MainWidth[0] = 1920/*1920*/,
    .MainHeight[0] = 1080/*1080*/,
    .MainWidth[1] = 720,
    .MainHeight[1] = 400,
    .ScrnWidth = 960,
    .ScrnHeight = 720,
    .ScrnWidthAct = 960,
    .ScrnHeightAct = 720,
    .ThmbWidth = 160,
    .ThmbHeight = 120,
    .ThmbWidthAct = 160,
    .ThmbHeightAct = 90,
    .QvLcdWidth = 960,
    .QvLcdHeight = 480,
    .QvHDMIWidth = 1920,
    .QvHDMIHeight = 1080,
    .MainJpegQLevel = 95,
    .ThmbJpegQLevel = 95,
    .ScrnJpegQLevel = 95,
    .JpegReEncodeLoop = 0,

    /* H264 config */
    .StrmProIDC[0] = 77,
    .StrmLvIDC[0] = 40,
    .StrmCabac[0] = 1,
    .StrmGopHeir[0] = 0, // 0 for simple GOP, 1 for HeirGOP
    .StrmGopPHeir[0] = 0,
    .StrmGopM[0] = 1,
    .StrmGopN[0] = 8,
    .StrmGopIDR[0] = 4,
    .StrmNumPRef[0] = 0, // use for HeirGoP, 0 for default setting
    .StrmNumBRef[0] = 0, // use for HeirGoP, 0 for default setting
    .StrmBrateCtrl[0] = AMBA_DSP_BITRATE_CBR,
    .StrmQLevel[0] = 0,
    .StrmVBRComplx[0] = 0,
    .StrmVBRPercent[0] = 0,
    .StrmVBRMinRatio[0] = 0,
    .StrmVBRMaxRatio[0] = 0,
    .StrmBitRate[0] = 12000000,
    .StrmQpMinI[0] = 0,
    .StrmQpMaxI[0] = 51,
    .StrmQpMinP[0] = 0,
    .StrmQpMaxP[0] = 51,
    .StrmQpMinB[0] = 0,
    .StrmQpMaxB[0] = 51,

    .StrmProIDC[1] = 77,
    .StrmLvIDC[1] = 40,
    .StrmCabac[1] = 1,
    .StrmGopHeir[1] = 0, // 0 for simple GOP, 1 for HeirGOP
    .StrmGopPHeir[1] = 0,
    .StrmGopM[1] = 1,
    .StrmGopN[1] = 8,
    .StrmGopIDR[1] = 4,
    .StrmNumPRef[1] = 0, // use for HeirGoP, 0 for default setting
    .StrmNumBRef[1] = 0, // use for HeirGoP, 0 for default setting
    .StrmBrateCtrl[1] = AMBA_DSP_BITRATE_CBR,
    .StrmQLevel[1] = 0,
    .StrmVBRComplx[1] = 0,
    .StrmVBRPercent[1] = 0,
    .StrmVBRMinRatio[1] = 0,
    .StrmVBRMaxRatio[1] = 0,
    .StrmBitRate[1] = 6000000,
    .StrmQpMinI[1] = 0,
    .StrmQpMaxI[1] = 51,
    .StrmQpMinP[1] = 0,
    .StrmQpMaxP[1] = 51,
    .StrmQpMinB[1] = 0,
    .StrmQpMaxB[1] = 51,
    .StrmStopOption[0] = AMBA_DSP_H264ENC_STOP_PAUSE_NEXT_IP,
    .StrmStopOption[1] = AMBA_DSP_H264ENC_STOP_PAUSE_NEXT_IP,

    .EncodeOutputLumaBufAddr = 0x0,
    .EncodeOutputChromaBufAddr = 0x0,
    .EncodeOutputScrnLumaBufAddr = 0x0,
    .EncodeOutputScrnChromaBufAddr = 0x0,
    .EncodeOutputThmbLumaBufAddr = 0x0,
    .EncodeOutputThmbChromaBufAddr = 0x0,

    /* Resource */
    .DisablePiv = 0,
    .DisableH264 = 0,
    .DisableMJPEG = 0,
    .Disable2ndStream = 0,
    .DisableHDPreview = 0,
    .EnableSqueezeYuvBuffer = 0,
    .MaxPivWidth = 1920,
    .MaxPivHeight = 1080,
    .MaxPivThumbWidth = 160,
    .MaxPivThumbHeight = 120,

    /* Sensor */
    .SensorID = 0, //0:ov4689 1:imx117
    .SensorReadoutMode = 0,
    .SensorShutterType = 0,
};

