/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmpUT_ImgProcTest.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Image Proc Test Function Implementation
 *
 *  @History        ::
 *      Date        Name             Comments
 *      08/01/2013  Tsung-Han Wu      Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "AmbaDataType.h"
#include "AmbaPrint.h"
#include "AmbaKAL.h"

#include "AmbaDSP_ImgDef.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Proc.h"
#include "AmbaImg_Impl_Cmd.h"
#include "AmbaKAL.h"
#include "AmbaShell.h"
#include <common/common.h>
#include "AmbaTest.h"
#include "AmbaImg_AntiFlicker.h"
#include "AmbaImg_Exif.h"

static SCENE_DATA_s SceneDataTmp;
static UINT32 AmbaSampleChNo = 0;
extern UINT32 IpctestChno;
UINT8 DeTmp = 0;
extern void AmbaSample_Set_MADJ(UINT32 chNo, UINT8 enable, UINT32 madj_no);

#if 0
static void Amba_Detect5060Hz(void)
{
        FLICKER_DETECT_STATUS_s status;

        Img_Get_Flicker_Detection_Status(&status);
        Img_Flicker_Detection_Enable(1);
        status.Running = RUNNING;
        while (status.Running == RUNNING) {
            Img_Get_Flicker_Detection_Status(&status);
            AmbaKAL_TaskSleep(100);
        }
        AmbaPrint("------------flicker : %dHz------------",status.FlickerHz);

}
#endif

float _Sample_Me_Bias(UINT32 target)
{
    static AMBA_AE_TILES_INFO_s AeTileInfoTmp;
    static UINT32 MeteringTable[96] = 
         { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //AE_METER_CENTER
           1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 2, 3, 3, 2, 1, 1, 1, 1,
           1, 1, 1, 2, 3, 5, 5, 3, 2, 1, 1, 1,
           1, 1, 1, 2, 3, 5, 5, 3, 2, 1, 1, 1,
           1, 1, 2, 3, 4, 5, 5, 4, 3, 2, 1, 1,
           1, 2, 3, 4, 4, 4, 4, 4, 4, 3, 2, 1,
           2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2};
    
    UINT32 LumaStat = 0, Weight = 0;
    UINT16 i = 0, Total = 0;       
    float Bias = 0.0;
    
    memset(&AeTileInfoTmp, 0, sizeof(AeTileInfoTmp));
    AmbaImg_Proc_Cmd(MW_IP_GET_AE_TILE_INFO, 0, 0, (UINT32)&AeTileInfoTmp);
    Total = AeTileInfoTmp.Rows * AeTileInfoTmp.Cols;
    
    for(i = 0;i < Total;i++){
        LumaStat += (UINT32)AeTileInfoTmp.TilesValue[i] * MeteringTable[i];
        Weight += MeteringTable[i];
    }
    if(Weight != 0){
        LumaStat = (LumaStat / Weight) >> 4 ;
    }
    if(target != 0){
        Bias = log2((float)LumaStat/(float)target);
    }
    AmbaPrint("target : %d, LumaStat : %d, EvBias : %f", target, LumaStat, Bias);
    return (Bias);
}

int AmpUT_ImgProcTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    //void *hdlr;
    AmbaPrint("AmpUT_ImgProcTest cmd: %s", argv[1]);

    if (strcmp(argv[1],"-chno") == 0) {
        IpctestChno = (UINT32)atoi(argv[2]);
    } else if (strcmp(argv[1],"-mebias") == 0) {
        UINT32 Target = 0;
        float EvBias = 0;
        
        Target = (UINT32)atoi(argv[2]);
        EvBias = _Sample_Me_Bias(Target);
        AmbaPrint("Target : %d, EvBias : %f", Target, EvBias);
    
    } else if (strcmp(argv[1],"-ver") == 0) {
        UINT32 IpVer = 0;
        AmbaImg_Proc_Cmd(MW_IP_GET_IPLIB_VER, (UINT32)&IpVer, 0, 0);
        AmbaPrint("--- Amba ImgProc Lib., Verison : %5d ----", IpVer);
    } else if (strcmp(argv[1],"-setmode") == 0) {
        UINT8 CurrMode;
        UINT8 NextMode;

        CurrMode = (UINT8)atoi(argv[2]);
        NextMode = (UINT8)atoi(argv[3]);
        AmbaImg_Proc_Cmd(MW_IP_SET_MODE, (UINT32)&CurrMode, (UINT32)&NextMode, 0);
    } else if (strcmp(argv[1],"-getmode") == 0) {
        UINT8 CurrMode;
        UINT8 NextMode;
        AmbaImg_Proc_Cmd(MW_IP_GET_MODE, (UINT32)&CurrMode, (UINT32)&NextMode, 0);
        AmbaPrint("---- CurrMode : %d, NextMode : %d ----", CurrMode, NextMode);
    } else if (strcmp(argv[1],"-get3astatus") == 0) {
        AMBA_3A_STATUS_s    VideoStatus;
        AMBA_3A_STATUS_s    StillStatus;
        AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, AmbaSampleChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
        AmbaPrint("---- VideoStatus : %d,%d,%d ----", VideoStatus.Ae, VideoStatus.Awb, VideoStatus.Af);
        AmbaPrint("---- StillStatus : %d,%d,%d ----", StillStatus.Ae, StillStatus.Awb, StillStatus.Af);
    } else if (strcmp(argv[1],"-set3astatus") == 0) {
        AMBA_3A_STATUS_s    VideoStatus;
        AMBA_3A_STATUS_s    StillStatus;
        AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, AmbaSampleChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);

        if (strcmp(argv[2],"video") == 0) {
            VideoStatus.Ae = (UINT8)atoi(argv[3]);
            VideoStatus.Awb = (UINT8)atoi(argv[4]);
            VideoStatus.Af = (UINT8)atoi(argv[5]);
        } else {
            StillStatus.Ae = (UINT8)atoi(argv[3]);
            StillStatus.Awb = (UINT8)atoi(argv[4]);
            StillStatus.Af = (UINT8)atoi(argv[5]);
        }

        AmbaImg_Proc_Cmd(MW_IP_SET_3A_STATUS, AmbaSampleChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);

    } else if (strcmp(argv[1],"-3a") == 0) {

        AMBA_3A_OP_INFO_s AaaOpInfoTmp;
        UINT32 AeVer = 0, AwbVer = 0, AdjVer = 0;
        UINT32 ChannelCount;
        UINT32 i;
        
        if ((strcmp(argv[2],"on") == 0) || (strcmp(argv[2],"off") == 0)) {
            if (strcmp(argv[2],"on") == 0) {
                AaaOpInfoTmp.AeOp = 1;
                AaaOpInfoTmp.AwbOp = 1;
                AaaOpInfoTmp.AfOp = 1;
                AaaOpInfoTmp.AdjOp = 1;
            } else if (strcmp(argv[2],"off") == 0) {
                AaaOpInfoTmp.AeOp = 0;
                AaaOpInfoTmp.AwbOp = 0;
                AaaOpInfoTmp.AfOp = 0;
                AaaOpInfoTmp.AdjOp = 0;
            }
            AmbaImg_Proc_Cmd(MW_IP_GET_TOTAL_CH_COUNT, (UINT32)&ChannelCount,0, 0);
            for(i = 0; i < ChannelCount; i++){
                 AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, i, (UINT32)&AaaOpInfoTmp, 0);
            }
        } else if (strcmp(argv[2],"ver") == 0) {
            AmbaImg_Proc_Cmd(MW_IP_GET_AAALIB_VER, (UINT32)&AeVer, (UINT32)&AwbVer, (UINT32)&AdjVer);
            AmbaPrint("--- Amba Ae  Lib.,Verison : %5d ----", AeVer);
            AmbaPrint("--- Amba Awb Lib.,Verison : %5d ----", AwbVer);
            AmbaPrint("--- Amba Adj Lib.,Verison : %5d ----", AdjVer);
        } else {
            AmbaImg_Proc_Cmd(MW_IP_GET_TOTAL_CH_COUNT, (UINT32)&ChannelCount, 0, 0);
            for (i = 0; i < ChannelCount; i++) {
                AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, i, (UINT32)&AaaOpInfoTmp, 0);
                AmbaPrint("ChNo[%u]: --- AeOp : %d, AwbOp : %d, AdjOp : %d, AfOp : %d ---", \
                    i, \
                    AaaOpInfoTmp.AeOp, \
                    AaaOpInfoTmp.AwbOp, \
                    AaaOpInfoTmp.AfOp, \
                    AaaOpInfoTmp.AdjOp);
            }
        }
    } else if (strcmp(argv[1],"-ae") == 0) {
        AmbaImg_Proc_Cmd(MW_IP_AMBA_AE_TEST_CMD, (UINT32)argc, (UINT32)argv, 0);
    } else if (strcmp(argv[1],"-awb") == 0) {
        AmbaImg_Proc_Cmd(MW_IP_AMBA_AWB_TEST_CMD, (UINT32)argc, (UINT32)argv, 0);
    } else if (strcmp(argv[1],"-af") == 0) {
        AMBA_3A_OP_INFO_s AaaOpInfoTmp;

        AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, AmbaSampleChNo, (UINT32)&AaaOpInfoTmp, 0);
        if (strcmp(argv[2],"on") == 0) {
            AaaOpInfoTmp.AfOp = 1;
        } else if (strcmp(argv[2],"off") == 0) {
            AaaOpInfoTmp.AfOp = 0;
        }
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, AmbaSampleChNo, (UINT32)&AaaOpInfoTmp, 0);
    } else if (strcmp(argv[1],"-adj") == 0) {
        AmbaImg_Proc_Cmd(MW_IP_AMBA_ADJ_TEST_CMD, (UINT32)argc, (UINT32)argv, 0);

    } else if (strcmp(argv[1],"-madj") == 0) {
	    UINT8 enable;
	    UINT32 madj_no;
	    UINT32 chNo = 0;
	    
	    enable = (UINT8)atoi(argv[2]);
	    madj_no = (UINT32)atoi(argv[3]);
	    chNo = (UINT32)atoi(argv[4]);
	    AmbaSample_Set_MADJ(chNo, enable,madj_no);
	    
    } else if (strcmp(argv[1],"-sc") == 0) {
       AmbaImg_Proc_Cmd(MW_IP_AMBA_SC_TEST_CMD, (UINT32)argc, (UINT32)argv, 0);
       /*
       if (strcmp(argv[2],"") == 0) {
            AmbaImg_Proc_Cmd(MW_IP_AMBA_SC_TEST_CMD, (UINT32)argc, (UINT32)argv, 0);
       }else{
            extern int AmbaIQParam_Scene_Mode_Load_Color_Table(UINT32 chNo, int sceneMode, int post, UINT8 modeSwitch);
            DeTmp = (UINT8)atoi(argv[2]);
            AmbaIQParam_Scene_Mode_Load_Color_Table(0, (int)DeTmp, 0, 1);
            AmbaImg_Proc_Cmd(MW_IP_SET_CURR_SCENE_MODE, IP_MODE_VIDEO, DeTmp, 0); 
	    AmbaImg_Proc_Cmd(MW_IP_SET_CURR_SCENE_MODE, IP_MODE_STILL, DeTmp, 0); 
       }
       */
    } else if (strcmp(argv[1],"-de") == 0) {
        if (strcmp(argv[2],"") == 0) {
           AmbaImg_Proc_Cmd(MW_IP_AMBA_DE_TEST_CMD, (UINT32)argc, (UINT32)argv, 0);
        }else{
            extern int AmbaIQParam_DigitalEffect_Load_Color_Table(UINT32 chNo, int DigitalEffect, int post, UINT8 modeSwitch);
            DeTmp = (UINT8)atoi(argv[2]);
            AmbaIQParam_DigitalEffect_Load_Color_Table(0, (int)DeTmp, 0, 1);
            AmbaImg_Proc_Cmd(MW_IP_SET_DIGITAL_EFFECT, 0, DeTmp, 0);
        }
    } else if (strcmp(argv[1],"-exif") == 0) {
        static COMPUTE_EXIF_PARAMS_s ComputeExifParams;
        static EXIF_INFO_s           ExifInfo;

        ComputeExifParams.ChannelNo = 0;
        ComputeExifParams.Mode = IMG_EXIF_PIV;
        ComputeExifParams.AeIdx = 0;
        Amba_Img_Exif_Compute_AAA_Exif (&ComputeExifParams);
        Amba_Img_Exif_Get_Exif_Info(0, &ExifInfo);

    } else if (strcmp(argv[1],"-VideoOb") == 0) {
        UINT8 ChNo,VideoObEn;
        ChNo = (UINT8)atoi(argv[2]);
        VideoObEn = (UINT8)atoi(argv[3]);
        
         AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_OB_ENABLE, (UINT32)ChNo,(UINT32)VideoObEn ,0);
        
    } else if (strcmp(argv[1],"-StillOb") == 0) {
        UINT8 ChNo,StillObEn;
        ChNo = (UINT8)atoi(argv[2]);
        StillObEn = (UINT8)atoi(argv[3]);
        AmbaImg_Proc_Cmd(MW_IP_SET_STILL_OB_ENABLE, (UINT32)ChNo,(UINT32)StillObEn ,0);         

    } else if (strcmp(argv[1],"-ObCont") == 0) {
        UINT8 ChNo,ObCont;
        ChNo = (UINT8)atoi(argv[2]);
        ObCont = (UINT8)atoi(argv[3]);   
        AmbaImg_Proc_Cmd(MW_IP_SET_OB_CONT_MODE_ENABLE, (UINT32)ChNo,(UINT32)ObCont ,0);   
    } else if (strcmp(argv[1],"-ObInfo") == 0) {
        UINT32 ChNo;
        UINT8 Mode;
        AMBA_OB_INFO_s ObInfoTemp;
        
        ChNo = (UINT8)atoi(argv[2]);
        Mode = (UINT8)atoi(argv[3]);  
        AmbaImg_CtrlFunc_Get_ObInfo(ChNo,Mode,(UINT32)&ObInfoTemp);
        AmbaPrint("Dynamic OB cal result(R,Gr,Gb,B):%5d,%5d,%5d,%5d",
            ObInfoTemp.DynamicOB.BlackR,ObInfoTemp.DynamicOB.BlackGr,ObInfoTemp.DynamicOB.BlackGb,ObInfoTemp.DynamicOB.BlackB);;
        
    } else if (strcmp(argv[1],"-wb_sim") == 0) {
        WB_SIM_INFO_s WbSimInfo;
        AmbaImg_Proc_Cmd(MW_IP_GET_WB_SIM_INFO, 0, (UINT32)&WbSimInfo, 0);
        AmbaPrint("LumaIdx : %d",WbSimInfo.LumaIdx);
        AmbaPrint("OutDoorIdx : %d",WbSimInfo.OutDoorIdx);
        AmbaPrint("HighLightIdx : %d",WbSimInfo.HighLightIdx);
        AmbaPrint("LowLightIdx : %d",WbSimInfo.LowLightIdx);
        AmbaPrint("AwbRatio[0] : %d",WbSimInfo.AwbRatio[0]);
        AmbaPrint("AwbRatio[1] : %d",WbSimInfo.AwbRatio[1]);

    } else if (strcmp(argv[1],"-exif_dbg") == 0) {
        UINT8 flg;

        if (strcmp(argv[2], "") != 0) {
            flg = (UINT8)atoi(argv[2]);
            AmbaImg_Proc_Cmd(MW_IP_SET_DEBUG_EXIF_FLAG, (UINT32)&flg, 0, 0);
        }
        AmbaImg_Proc_Cmd(MW_IP_GET_DEBUG_EXIF_FLAG, (UINT32)&flg, 0, 0);

        AmbaPrint("Current EXIF Debug Flg: %d", flg);

    } else if (strcmp(argv[1],"-me") == 0) {
        UINT8 Mode = 0;
        float ShtTime = 0, AgcGain = 0;
        UINT32 DGain = 0;
        AMBA_AE_INFO_s AeInfo, StillAeInfo[MAX_AEB_NUM];
        
        Mode = (UINT8)atoi(argv[2]);
        ShtTime = (float)(atoi(argv[3]) / 1000000.0);
        AgcGain = (float)(atoi(argv[4]) / 1000.0);
        DGain = (UINT32)(atoi(argv[5]));
        
        AmbaPrint(" Mode : %d, ShtTime : %f, AgcGain : %f, DGain : %4d ", Mode, ShtTime, AgcGain, DGain);

        if (Mode == 0) {
            AeInfo.AgcGain = AgcGain;
            AeInfo.ShutterTime = ShtTime;
            AeInfo.Dgain = DGain;
            AmbaImg_Proc_Cmd(MW_IP_SET_AE_INFO, 0, IP_MODE_VIDEO, (UINT32)&AeInfo);
        }else{
            StillAeInfo[0].AgcGain = AgcGain;
            StillAeInfo[0].ShutterTime = ShtTime;
            StillAeInfo[0].Dgain = DGain;
            AmbaImg_Proc_Cmd(MW_IP_SET_AE_INFO, 0, IP_MODE_STILL, (UINT32)StillAeInfo);
        }


    } else if (strcmp(argv[1],"-mwb") == 0) {
        UINT8 Mode = 0;
        AMBA_DSP_IMG_WB_GAIN_s    WbGain = {WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN};

        Mode = (UINT8)atoi(argv[2]);
        if (Mode == 0) {
            AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, 0, IP_MODE_VIDEO, (UINT32)&WbGain);
        }else{
            AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, 0, IP_MODE_STILL, (UINT32)&WbGain);
        }
        
        WbGain.GainR = (UINT16)atoi(argv[3]);
        WbGain.GainG = (UINT16)atoi(argv[4]);
        WbGain.GainB = (UINT16)atoi(argv[5]);
        
        AmbaPrint(" Mode : %d, GainR : %5d, GainG : %5d, GainB : %5d ", Mode, WbGain.GainR, WbGain.GainG, WbGain.GainB);

        if (Mode == 0) {
            AmbaImg_Proc_Cmd(MW_IP_SET_PIPE_WB_GAIN, 0, IP_MODE_VIDEO, (UINT32)&WbGain);
        }else{
            AmbaImg_Proc_Cmd(MW_IP_SET_PIPE_WB_GAIN, 0, IP_MODE_STILL, (UINT32)&WbGain);
        }


    } else if (strcmp(argv[1],"-evbias") == 0) {
        INT16 EvBias = 0;
        AE_CONTROL_s AeCtrlMode;
        EvBias = (INT16)atoi(argv[2]);
        AmbaImg_Proc_Cmd(MW_IP_GET_MULTI_AE_CONTROL_CAPABILITY, 0, (UINT32)&AeCtrlMode,0);
        AeCtrlMode.EvBias = EvBias;
        AmbaImg_Proc_Cmd(MW_IP_SET_MULTI_AE_CONTROL_CAPABILITY, 0, (UINT32)&AeCtrlMode,0);


    } else if (strcmp(argv[1],"-sharp") == 0) {
        UINT16 Sharp = 0;
        AMBA_DSP_IMG_MODE_CFG_s Mode;
        memset(&Mode, 0, sizeof(Mode));
        
        Sharp = (UINT16)atoi(argv[2]);
        AmbaImg_Proc_Cmd(MW_IP_SET_IMAGE_SHARPNESS, 0, (UINT32)&Mode, (UINT32)Sharp);

    } else if (strcmp(argv[1],"-slowshutter") == 0) {
        UINT8 SlowShutterFps = 0;
        UINT8 Mode = 0, Enable = 0;
        AE_CONTROL_s AeCtrlMode;
        
        Mode = (UINT8)atoi(argv[2]);
        Enable = (UINT8)atoi(argv[3]);
        SlowShutterFps = (UINT8)atoi(argv[4]);
        AmbaImg_Proc_Cmd(MW_IP_GET_MULTI_AE_CONTROL_CAPABILITY, 0, (UINT32)&AeCtrlMode,0);
        if (Mode == 0) { //Video Preview
            AeCtrlMode.SlowShutter = Enable;
            AeCtrlMode.SlowShutterFps = SlowShutterFps;
        }else{ //Photo Preview
            AeCtrlMode.PhotoSlowShutter = Enable;
            AeCtrlMode.PhotoSlowShutterFps = SlowShutterFps;
        }
        AmbaImg_Proc_Cmd(MW_IP_SET_MULTI_AE_CONTROL_CAPABILITY, 0, (UINT32)&AeCtrlMode,0);

    } else if (strcmp(argv[1],"-scenedata") == 0) {
        UINT32 SceneModeTmp = 0;
        UINT16 IsoTmp = 0;
        UINT16 ShtTmp = 0;
        SceneModeTmp = (UINT32)atoi(argv[2]);
        IsoTmp = (UINT16)atoi(argv[3]);
        ShtTmp = (UINT16)atoi(argv[4]);
        AmbaImg_Proc_Cmd(MW_IP_GET_SCENE_MODE_INFO, (UINT32)SceneModeTmp, (UINT32)&SceneDataTmp, 0);
        SceneDataTmp.AeControl.Video.DefExp[0] = IsoTmp;
        SceneDataTmp.AeControl.Video.DefExp[1] = ShtTmp;
        SceneDataTmp.AeControl.Video.DefExp[2] = ShtTmp;
    
        SceneDataTmp.AeControl.Photo.DefExp[0] = IsoTmp;
        SceneDataTmp.AeControl.Photo.DefExp[1] = ShtTmp;
        SceneDataTmp.AeControl.Photo.DefExp[2] = ShtTmp;
        AmbaPrint("IsoTmp : %d, ShtTmp : %d", IsoTmp, ShtTmp);
        AmbaImg_Proc_Cmd(MW_IP_SET_SCENE_MODE_INFO, (UINT32)SceneModeTmp, (UINT32)&SceneDataTmp, 0);
    
    }else if (strcmp(argv[1],"-chgiqparam") == 0) {
        extern UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID);
        UINT32 chNo = 0;
        int sensorID = 0;
        sensorID = (int)atoi(argv[2]);
        App_Image_Init_Iq_Params(chNo, sensorID);
    
    } else {
        AmbaPrint("Usage: t imgproc");
        AmbaPrint("         -ver");
        AmbaPrint("         -3a         [on|off]");
        AmbaPrint("         -3a ver");
        AmbaPrint("         -ae         [on|off]");
        AmbaPrint("         -ae info    [chNo]");
        AmbaPrint("         -awb        [on|off]");
        AmbaPrint("         -awb info   [chNo]");
        AmbaPrint("         -adj        [on|off]");
	AmbaPrint("         -madj       [Enable][AdjTableNo][ChNo]");
        AmbaPrint("         -exif_dbg   [0|1]");
        AmbaPrint("         -me         [Video|Still : 0|1] [ShtTime : us] [Gain : 1x 1000] [Dgain : Unit 4096]");
        AmbaPrint("         -mwb        [Video|Still : 0|1] [GainR] [GainG] [GainB], Unit 4096");
        AmbaPrint("         -slowshutter[Video|Photo : 0|1] [SlowShutterEnable] [SlowShutterFps]");
        AmbaPrint("         -sharp      [Level : 0~6]");
    }
    return 0;
}

int AmpUT_ImgProcTestAdd(void)
{
    AmbaPrint("Adding AmpUT_ImgProcTest");
    AmbaTest_RegisterCommand("imgproc", AmpUT_ImgProcTest);

    return 0;
}

