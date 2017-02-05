 /**
  * @file src/app/sample/unittest/AmpUT_input.c
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
#include <AmbaCalibInfo.h>
#ifdef CONFIG_SOC_A12
#include "AmbaUTCalibMgr.h"
#endif

#include <AmbaSensor.h>
#ifdef CONFIG_SENSOR_B5_SONY_IMX122
#include <AmbaSensor_B5_IMX122.h>
#endif

#include <AmbaYuv.h>
#ifdef CONFIG_YUVINPUT_TI5150
#include <AmbaYuv_TI5150.h>
#endif

#ifdef CONFIG_SENSOR_B5_SONY_IMX122
extern AMBA_SENSOR_OBJ_s AmbaSensor_B5_IMX122Obj;
#endif

#ifdef CONFIG_YUVINPUT_TI5150
extern AMBA_YUV_OBJ_s AmbaYuv_TI5150Obj;
#endif

char InputDeviceName[][24] = {
    [SENSOR_OV4689]             = {"OV4689\0"},
    [SENSOR_IMX117]             = {"IMX117\0"},
    [SENSOR_OV2710]             = {"OV2710\0"},
    [SENSOR_OV10823]            = {"OV10823\0"},
    [SENSOR_IMX206]             = {"IMX206\0"},
    [SENSOR_AR0330_PARALLEL]    = {"AR0330 Parallel\0"},
    [SENSOR_AR0230]             = {"AR0230\0"},
    [SENSOR_OV9750]             = {"OV9750\0"},
    [SENSOR_MN34120]            = {"MN34120\0"},
    [SENSOR_B5_OV4689]          = {"B5_OV4689\0"},
    [SENSOR_IMX290]             = {"IMX290\0"},
    [SENSOR_MN34222]            = {"MN34222\0"},
    [SENSOR_IMX377]             = {"IMX377\0"},
    [SENSOR_B5_OV9750]          = {"B5_OV9750\0"},
};

char InputModeName[][32] = {
    [FHD_AR16_9_P30]         = {"1920X1080  30P\0"},
    [FHD_AR16_9_P60]         = {"1920X1080  60P\0"},
    [FHD_AR16_9_P120]        = {"1920X1080 120P\0"},
    [FHD_AR16_9_HDR_P30]     = {"1920X1080  30P HDR\0"},
    [FHD_AR16_9_HDR_P60]     = {"1920X1080  60P HDR\0"},
    [HD_AR16_9_P30]          = {"1280X 720  30P\0"},
    [HD_AR16_9_P60]          = {"1280X 720  60P\0"},
    [HD_AR16_9_P120]         = {"1280X 720 120P\0"},
    [HD_AR16_9_P240]         = {"1280X 720 240P\0"},
    [HD_AR16_9_HDR_P30]      = {"1280X 720  30P HDR\0"},
    [HD_AR16_9_HDR_P60]      = {"1280X 720  60P HDR\0"},
    [WQHD_AR16_9_P30]        = {"2560X1440  30P\0"},
    [WQHD_AR16_9_P60]        = {"2560X1440  60P\0"},
    [UHD_AR16_9_P30]         = {"3840X2160  30P\0"},
    [UHD_AR16_9_P60]         = {"3840X2160  60P\0"},
    [FHD_AR4_3_P30]          = {"1920X1440  30P\0"},
    [FHD_AR4_3_P60]          = {"1920X1440  60P\0"},
    [FHD_AR4_3_P120]         = {"1920X1440 120P\0"},
    [FHD_AR4_3_HDR_P30]      = {"1920X1440  30P HDR\0"},
    [FHD_AR4_3_HDR_P60]      = {"1920X1440  60P HDR\0"},
    [HD_AR4_3_P30]           = {"1280X 960  30P\0"},
    [HD_AR4_3_P60]           = {"1280X 960  60P\0"},
    [HD_AR4_3_P120]          = {"1280X 960 120P\0"},
    [HD_AR4_3_P240]          = {"1280X 960 240P\0"},
    [HD_AR4_3_HDR_P30]       = {"1280X 960  30P HDR\0"},
    [HD_AR4_3_HDR_P60]       = {"1280X 960  60P HDR\0"},
    [WQHD_AR4_3_P30]         = {"2560X1920  30P\0"},
    [WQHD_AR4_3_P60]         = {"2560X1920  60P\0"},
    [UHD_AR4_3_P30]          = {"3840X2880  30P\0"},
    [UHD_AR4_3_P60]          = {"3840X2880  60P\0"},
    [FHD_AR16_9_CFA_HDR_P30] = {"1920X1080  30P CFA HDR\0"},
    [FHD_AR16_9_CFA_HDR_P60] = {"1920X1080  60P CFA HDR\0"},
    [FHD_AR4_3_CFA_HDR_P30]  = {"1920X1440  30P CFA HDR\0"},
    [FHD_AR4_3_CFA_HDR_P60]  = {"1920X1440  60P CFA HDR\0"},
    [HD_AR16_9_CFA_HDR_P30]  = {"1280X 720  30P CFA HDR\0"},
    [HD_AR16_9_CFA_HDR_P60]  = {"1280X 720  60P CFA HDR\0"},
    [HD_AR4_3_CFA_HDR_P30]   = {"1280X 960  30P CFA HDR\0"},
    [HD_AR4_3_CFA_HDR_P60]   = {"1280X 960  60P CFA HDR\0"},
    [WQHD_AR16_9_HDR_P30]    = {"2560X1440  30P HDR\0"},
    [WQHD_AR16_9_HDR_P60]    = {"2560X1440  60P HDR\0"},
};

char InputModePALName[][32] = {
    [FHD_AR16_9_P30]         = {"1920X1080  25P\0"},
    [FHD_AR16_9_P60]         = {"1920X1080  50P\0"},
    [FHD_AR16_9_P120]        = {"1920X1080 100P\0"},
    [FHD_AR16_9_HDR_P30]     = {"1920X1080  25P HDR\0"},
    [FHD_AR16_9_HDR_P60]     = {"1920X1080  50P HDR\0"},
    [HD_AR16_9_P30]          = {"1280X 720  25P\0"},
    [HD_AR16_9_P60]          = {"1280X 720  50P\0"},
    [HD_AR16_9_P120]         = {"1280X 720 100P\0"},
    [HD_AR16_9_P240]         = {"1280X 720 200P\0"},
    [HD_AR16_9_HDR_P30]      = {"1280X 720  25P HDR\0"},
    [HD_AR16_9_HDR_P60]      = {"1280X 720  50P HDR\0"},
    [WQHD_AR16_9_P30]        = {"2560X1440  25P\0"},
    [WQHD_AR16_9_P60]        = {"2560X1440  50P\0"},
    [UHD_AR16_9_P30]         = {"3840X2160  25P\0"},
    [UHD_AR16_9_P60]         = {"3840X2160  50P\0"},
    [FHD_AR4_3_P30]          = {"1920X1440  25P\0"},
    [FHD_AR4_3_P60]          = {"1920X1440  50P\0"},
    [FHD_AR4_3_P120]         = {"1920X1440 100P\0"},
    [FHD_AR4_3_HDR_P30]      = {"1920X1440  25P HDR\0"},
    [FHD_AR4_3_HDR_P60]      = {"1920X1440  50P HDR\0"},
    [HD_AR4_3_P30]           = {"1280X 960  25P\0"},
    [HD_AR4_3_P60]           = {"1280X 960  50P\0"},
    [HD_AR4_3_P120]          = {"1280X 960 100P\0"},
    [HD_AR4_3_P240]          = {"1280X 960 200P\0"},
    [HD_AR4_3_HDR_P30]       = {"1280X 960  25P HDR\0"},
    [HD_AR4_3_HDR_P60]       = {"1280X 960  50P HDR\0"},
    [WQHD_AR4_3_P30]         = {"2560X1920  25P\0"},
    [WQHD_AR4_3_P60]         = {"2560X1920  50P\0"},
    [UHD_AR4_3_P30]          = {"3840X2880  25P\0"},
    [UHD_AR4_3_P60]          = {"3840X2880  50P\0"},
    [FHD_AR16_9_CFA_HDR_P30] = {"1920X1080  25P CFA HDR\0"},
    [FHD_AR16_9_CFA_HDR_P60] = {"1920X1080  50P CFA HDR\0"},
    [FHD_AR4_3_CFA_HDR_P30]  = {"1920X1440  25P CFA HDR\0"},
    [FHD_AR4_3_CFA_HDR_P60]  = {"1920X1440  50P CFA HDR\0"},
    [HD_AR16_9_CFA_HDR_P30]  = {"1280X 720  25P CFA HDR\0"},
    [HD_AR16_9_CFA_HDR_P60]  = {"1280X 720  50P CFA HDR\0"},
    [HD_AR4_3_CFA_HDR_P30]   = {"1280X 960  25P CFA HDR\0"},
    [HD_AR4_3_CFA_HDR_P60]   = {"1280X 960  50P CFA HDR\0"},
    [WQHD_AR16_9_HDR_P30]    = {"2560X1440  25P HDR\0"},
    [WQHD_AR16_9_HDR_P60]    = {"2560X1440  50P HDR\0"},
};

char InputStillModeName[][24] = {
    [STILL_AR4_3 - STILL_AR4_3]        = {" 960X 720  30P\0"},
    [STILL_AR16_9 - STILL_AR4_3]       = {" 960X 540  30P\0"},
    [STILL_AR4_3_1 - STILL_AR4_3]      = {" 960X 720  30P\0"},
    [STILL_AR16_9_1 - STILL_AR4_3]     = {" 960X 540  30P\0"},
};

char InputStillModePALName[][24] = {
    [STILL_AR4_3 - STILL_AR4_3]        = {" 960X 720  25P\0"},
    [STILL_AR16_9 - STILL_AR4_3]       = {" 960X 540  25P\0"},
    [STILL_AR4_3_1 - STILL_AR4_3]      = {" 960X 720  25P\0"},
    [STILL_AR16_9_1 - STILL_AR4_3]     = {" 960X 540  25P\0"},
};

#ifdef CONFIG_YUVINPUT_TI5150
INPUT_ENC_MGT_s VideoEncMgtTI5150[] = {
    [0] = {
        .InputMode = TI5150_720_480_60I,
        .CaptureWidth = 720,
        .CaptureHeight = 480,
        .MainWidth = 720,
        .MainHeight = 480,
        .TimeScale = 60000,
        .TickPerPicture = 1001,
        .Interlace = 1,
        .AspectRatio = VAR_16x9,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 9,
        .MinBitRate = 6,
        .AverageBitRate = 8,
        .Name = "480I60\0",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 0},
        },
    },
    [1] = {
        .InputMode = TI5150_720_576_50I,
        .CaptureWidth = 720,
        .CaptureHeight = 576,
        .MainWidth = 720,
        .MainHeight = 576,
        .TimeScale = 50,
        .TickPerPicture = 1,
        .Interlace = 1,
        .AspectRatio = VAR_16x9,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_SMART_VBR,
        .MaxBitRate = 9,
        .MinBitRate = 6,
        .AverageBitRate = 8,
        .Name = "576I50\0",
        .limit = {
            [0] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [1] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 1},
            [2] = { 1, 1, 1, 1, \
                1, 1, 1, 1, 1, 1, 1, \
                1, 0},
        },
    },
};
#endif

INPUT_ENC_MGT_s *RegisterMWUT_Sensor_Driver(AMBA_DSP_CHANNEL_ID_u Chan, int SensorID)
{
    INPUT_ENC_MGT_s *SensorInputEncMgt = NULL;

    if (SensorID == SENSOR_OV4689) {
#ifdef CONFIG_SENSOR_OV4689
        extern INPUT_ENC_MGT_s VideoEncMgtOV4689[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_OV4689Obj;
        extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_OV4689Obj;
        AmbaPrint("Hook OV4689 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_OV4689Obj);
        AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_OV4689Obj);
        SensorInputEncMgt = VideoEncMgtOV4689;
#else
        AmbaPrint("OV4689 Sensor Not Support in this CHIP");
#endif
    } else if (SensorID == SENSOR_IMX117) {
#ifdef CONFIG_SENSOR_SONY_IMX117
        extern INPUT_ENC_MGT_s VideoEncMgtIMX117[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_IMX117Obj;
        AmbaPrint("Hook IMX117 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_IMX117Obj);
#ifdef CONFIG_SOC_A12
        {
            extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_IMX117Obj;
            AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_IMX117Obj);
        }
#endif
        SensorInputEncMgt = VideoEncMgtIMX117;
#else
        AmbaPrint("IMX117 Sensor Not Support in this CHIP");
#endif
    } else if (SensorID == SENSOR_OV2710) {
#ifdef CONFIG_SENSOR_TI913914_OV2710
        extern INPUT_ENC_MGT_s VideoEncMgtOV2710[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_OV2710Obj;
        AmbaPrint("Hook OV2710 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_OV2710Obj);
#ifdef CONFIG_SOC_A12
        {
            extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_TI913914_OV2710Obj;
            AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_TI913914_OV2710Obj);
        }
#endif
        SensorInputEncMgt = VideoEncMgtOV2710;
#else
        AmbaPrint("OV2710 Sensor Not Support in this CHIP");
#endif
    } else if (SensorID == SENSOR_OV10823) {
#ifdef CONFIG_SENSOR_OV10823
        extern INPUT_ENC_MGT_s VideoEncMgtOV10823[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_OV10823Obj;
        AmbaPrint("Hook OV10823 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_OV10823Obj);
#ifdef CONFIG_SOC_A12
        {
            extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_OV10823Obj;
            AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_OV10823Obj);
        }
#endif
        SensorInputEncMgt = VideoEncMgtOV10823;
#else
        AmbaPrint("OV10823 Sensor Not Support in this CHIP");
#endif
    } else if (SensorID == SENSOR_IMX206) {
#ifdef CONFIG_SENSOR_SONY_IMX206
        extern INPUT_ENC_MGT_s VideoEncMgtIMX206[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_IMX206Obj;
        extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_IMX206Obj;
        AmbaPrint("Hook IMX206 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_IMX206Obj);
#ifdef CONFIG_SOC_A12
        AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_IMX206Obj);
#endif
        SensorInputEncMgt = VideoEncMgtIMX206;
#else
        AmbaPrint("IMX206 Sensor Not Support in this CHIP");
#endif
    } else if (SensorID == SENSOR_AR0330_PARALLEL) {
#ifdef CONFIG_SENSOR_AR0330_PARALLEL
        extern INPUT_ENC_MGT_s VideoEncMgtAR0330_PARALLEL[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_AR0330_PARALLELObj;
        AmbaPrint("Hook ARO330 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_AR0330_PARALLELObj);
#ifdef CONFIG_SOC_A12
        {
            extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_AR0330_PARALLELObj;
            AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_AR0330_PARALLELObj);
        }
#endif
        SensorInputEncMgt = VideoEncMgtAR0330_PARALLEL;
#else
        AmbaPrint("AR0330 Sensor Not Support in this CHIP");
#endif
    } else if (SensorID == SENSOR_AR0230) {
#ifdef CONFIG_SENSOR_AR0230
        extern INPUT_ENC_MGT_s VideoEncMgtAR0230[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_AR0230Obj;
        AmbaPrint("Hook ARO230 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_AR0230Obj);
#ifdef CONFIG_SOC_A12
        {
            extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_AR0230Obj;
            AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_AR0230Obj);
        }
#endif
        SensorInputEncMgt = VideoEncMgtAR0230;
#else
        AmbaPrint("ARO230 Sensor Not Support in this CHIP");
#endif
    } else if (SensorID == SENSOR_OV9750) {
#ifdef CONFIG_SENSOR_OV9750
        extern INPUT_ENC_MGT_s VideoEncMgtOV9750[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_OV9750Obj;
        AmbaPrint("Hook OV9750 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_OV9750Obj);
#ifdef CONFIG_SOC_A12
        {
            extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_OV9750Obj;
            AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_OV9750Obj);
        }
#endif
        SensorInputEncMgt = VideoEncMgtOV9750;
#else
        AmbaPrint("OV9750 Sensor Not Support in this CHIP");
#endif
    } else if (SensorID == SENSOR_MN34120) {
#ifdef CONFIG_SENSOR_MN34120
        extern INPUT_ENC_MGT_s VideoEncMgtMN34120[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_MN34120Obj;
        AmbaPrint("Hook MN34120 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_MN34120Obj);
#ifdef CONFIG_SOC_A12
        {
            extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_MN34120Obj;
            AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_MN34120Obj);
        }
#endif
        SensorInputEncMgt = VideoEncMgtMN34120;
#else
        AmbaPrint("MN34120 Sensor Not Support in this CHIP");
#endif

    } else if (SensorID == SENSOR_B5_OV4689) {
#ifdef CONFIG_SENSOR_B5_OV4689
        extern INPUT_ENC_MGT_s VideoEncMgt_B5_OV4689[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_B5_OV4689Obj;
        extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_B5_OV4689Obj;
        AmbaPrint("Hook B5_OV4689 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_B5_OV4689Obj);
        AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_B5_OV4689Obj);
        SensorInputEncMgt = VideoEncMgt_B5_OV4689;
#else
        AmbaPrint("B5_OV4689 Sensor Not Support in this CHIP");
#endif
    } else if (SensorID == SENSOR_IMX290) {
#ifdef CONFIG_SENSOR_SONY_IMX290
        extern INPUT_ENC_MGT_s VideoEncMgtIMX290[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_IMX290Obj;
        AmbaPrint("Hook IMX290 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_IMX290Obj);
#ifdef CONFIG_SOC_A12
        {
            extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_IMX290Obj;
            AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_IMX290Obj);
        }
#endif
        SensorInputEncMgt = VideoEncMgtIMX290;
#else
        AmbaPrint("IMX290 Sensor Not Support in this CHIP");
#endif
    } else if (SensorID == SENSOR_MN34222) {
#ifdef CONFIG_SENSOR_MN34222
        extern INPUT_ENC_MGT_s VideoEncMgtMN34222[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_MN34222Obj;
        AmbaPrint("Hook MN34222 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_MN34222Obj);
#ifdef CONFIG_SOC_A12
        {
            extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_MN34222Obj;
            AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_MN34222Obj);
        }
#endif
        SensorInputEncMgt = VideoEncMgtMN34222;
#else
        AmbaPrint("MN34222 Sensor Not Support in this CHIP");
#endif
    }else if (SensorID == SENSOR_B5_OV9750) {
#ifdef CONFIG_SENSOR_B5_OV9750
        extern INPUT_ENC_MGT_s VideoEncMgt_B5_OV9750[];
        extern AMBA_SENSOR_OBJ_s AmbaSensor_B5_OV9750Obj;
        extern AMBA_CALIB_INFO_OBJ_s AmbaCalibInfo_B5_OV9750Obj;
        AmbaPrint("Hook B5_OV4689 Sensor");
        AmbaSensor_Hook(Chan, &AmbaSensor_B5_OV9750Obj);
        AmbaCalibInfo_Hook(Chan, &AmbaCalibInfo_B5_OV9750Obj);
        SensorInputEncMgt = VideoEncMgt_B5_OV9750;
#else
        AmbaPrint("B5_OV4689 Sensor Not Support in this CHIP");
#endif
    }else {
        AmbaPrint("This Sensor ID Not Support Now");
    }
    if (SensorInputEncMgt) {
        AmbaSensor_Init(Chan);
    }
    return SensorInputEncMgt;
}

INPUT_ENC_MGT_s *RegisterMWUT_YUV_Driver(AMBA_DSP_CHANNEL_ID_u Chan, int YUVID)
{
    INPUT_ENC_MGT_s *YUVInputEncMgt=NULL;

    if (YUVID == 0) {
#ifdef CONFIG_YUVINPUT_TI5150
        AmbaPrint("Hook TI5150 YUV");
        AmbaYuv_Hook(Chan, &AmbaYuv_TI5150Obj);
        YUVInputEncMgt = VideoEncMgtTI5150;
#else
        AmbaPrint("TI5150 YUV Not Support in this CHIP");
        return NG;
#endif
    } else {
        AmbaPrint("This YUV input ID Not Support Now");
    }

    if (YUVInputEncMgt) {
        AmbaYuv_Init(Chan);
    }

    return YUVInputEncMgt;
}

char* MWUT_GetInputDeviceName(UINT16 index)
{
    if (index >= INPUT_DEVICE_NUM) {
        return NULL;
    }

    return InputDeviceName[index];
}

void MWUT_InputVideoModePrintOutAll(INPUT_ENC_MGT_s *encMgt)
{
    UINT16 i = 0;

    if (encMgt == NULL) {
        return;
    }

    for (i = 0; i<ENC_VIDEO_MODE_NUM; i++) {
        if (encMgt[i].ForbidMode == 0) {
            AmbaPrint("%3d -- %s", i, &InputModeName[i]);
            AmbaPrint("      %s", encMgt[i].Desc);
        }
    }
}

void MWUT_InputStillModePrintOutAll(INPUT_ENC_MGT_s *encMgt)
{
    UINT16 i = 0;

    if (encMgt == NULL) {
        return;
    }

    for (i = STILL_AR4_3; i<ENC_STILL_MODE_NUM; i++) {
        if (encMgt[i].ForbidMode == 0) {
            AmbaPrint("%3d -- %s", i, &InputStillModeName[i - STILL_AR4_3]);
            AmbaPrint("      %s", encMgt[i].Desc);
        }
    }
}

char* MWUT_GetInputVideoModeName(UINT16 index, UINT8 system)
{
    if (index >= ENC_VIDEO_MODE_NUM) {
        return NULL;
    }

    if (system == 0) {
        return InputModeName[index];
    } else {
        return InputModePALName[index];
    }
}

char* MWUT_GetInputStillModeName(UINT16 index, UINT8 system)
{
    if (index >= ENC_STILL_MODE_NUM) {
        return NULL;
    }

    if (system == 0) {
        return InputStillModeName[index - STILL_AR4_3];
    } else {
        return InputStillModePALName[index - STILL_AR4_3];
    }
}

void MWUT_InputSetDualHDWindow(INPUT_ENC_MGT_s *encMgt, UINT8 mode, UINT16 *outWidth, UINT16 *outHeight)
{
    if (encMgt[mode].MainWidth > SEC_STREAM_MAX_WIDTH) {
        *outWidth = SEC_STREAM_MAX_WIDTH;
        *outHeight = SEC_STREAM_MAX_WIDTH*encMgt[mode].MainHeight/encMgt[mode].MainWidth;
    } else if ((encMgt[mode].MainWidth > SEC_STREAM_HD_WIDTH) && encMgt[mode].DualStrmUptoHD) {
        *outWidth = SEC_STREAM_HD_WIDTH;
        *outHeight = SEC_STREAM_HD_WIDTH*encMgt[mode].MainHeight/encMgt[mode].MainWidth;
    } else {
        *outWidth = encMgt[mode].MainWidth;
        *outHeight = encMgt[mode].MainHeight;
    }
}

