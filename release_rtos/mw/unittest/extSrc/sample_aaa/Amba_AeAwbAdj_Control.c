/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: Amba_AeAwbAdj_Control.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Ae/Awb/Adj Algo. control.
 *
 *  @History        ::
 *      Date        Name             Comments
 *      05/06/2006  Jyh-Jiun Li      Created
 *
 \*-------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "AmbaDataType.h"
#include "AmbaPrint.h"
#include "AmbaKAL.h"

#include "AmbaDSP_ImgFilter.h"
#include "AmbaImg_Proc.h"
#include "AmbaImg_AeAwb.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_AntiFlicker.h"
#include "AmbaSample_AeAwbAdj.h"
#include "AmbaImg_Impl_Cmd.h"
#include "AmbaImg_VDsp_Handler.h"
#include "AmbaUTCalibAdjust.h"
#include "scheduler.h"

//static UINT32 ChannelCount = 1;
//static UINT32 AmbaSampleChNo = 0;
static AE_ALGO_INFO_s  AeAlgoInfo[10];
UINT32 IpctestChno = 0xFFFFFFFF;

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: Amba_Adj_VideoHdr_Control
 *
 *  @Description:: Video Hdr Control
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    None
\*-----------------------------------------------------------------------------------------------*/
void Amba_Adj_VideoHdr_Control(AMBA_AE_INFO_s          *videoAeInfo)
{
    static HDR_INFO_s               HdrInfo;

    memset(&HdrInfo, 0, sizeof(HdrInfo));
    HdrInfo.AeInfo[0] = videoAeInfo[0];
    HdrInfo.AeInfo[1] = videoAeInfo[1];

    AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, 0, IP_MODE_VIDEO, (UINT32)&HdrInfo.WbGain[0]); //long
    AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, 0, IP_MODE_VIDEO, (UINT32)&HdrInfo.WbGain[1]); //short
    AmbaImg_Proc_Cmd(MW_IP_ADJ_VIDEO_HDR_CONTROL, 0, (UINT32)&HdrInfo, 0);

    videoAeInfo[0] = HdrInfo.AeInfo[0];
    videoAeInfo[1] = HdrInfo.AeInfo[1];
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: Amba_AeAwbAdj_Set_SchdlrExpInfo
 *
 *  @Description:: To Setup MW exposure scheduler module
 *
 *  @Input      ::
 *    UINT32 chNo : Channel number.
 *    AMBA_DSP_IMG_MODE_CFG_s *pMode : Pointer to DSP mode
 *    UINT8 type : Type of operation mode of MW exposure scheduler module
 *                 0 : Normal mode
 *                 1 : Direct set mode
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    None
\*-----------------------------------------------------------------------------------------------*/
void Amba_AeAwbAdj_Set_SchdlrExpInfo(UINT32 chNo, AMBA_DSP_IMG_MODE_CFG_s *pMode, UINT8 type, UINT16 UpdateFlg)
{
    AMBA_IMG_SCHDLR_EXP_s   VideoSchdlrExpInfo = {0};
    AMBA_AE_INFO_s          VideoAeInfo[2];
    AMBA_DSP_IMG_WB_GAIN_s  VideoWbGain = {WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN};
    UINT32                  FrontWbCheck = 0;
    UINT8                   HdrEnable = 0;
    PIPELINE_CONTROL_s*     VideoPipeCtrl = 0;
    AMBA_3A_OP_INFO_s       AaaOpInfo;

    AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, chNo, (UINT32)&AaaOpInfo, 0);
    AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, chNo, IP_MODE_VIDEO, (UINT32)&VideoAeInfo[0]);
    AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_HDR_ENABLE, chNo, (UINT32)&HdrEnable, 0);
    if ((HdrEnable == IMG_DSP_HDR_MODE_0) && (AaaOpInfo.AeOp == ENABLE)) {
        UINT32 VideoPipeAddr = 0;

        AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, (chNo + 1), IP_MODE_VIDEO, (UINT32)&VideoAeInfo[1]);
        Amba_Img_Get_Video_Pipe_Ctrl_Addr((UINT32)chNo, &VideoPipeAddr);
        VideoPipeCtrl = (PIPELINE_CONTROL_s *)VideoPipeAddr;

        //AmbaPrint("+++++++ %12.5f,%5d,    %12.5f,%5d, +++++++++",VideoAeInfo[0].AgcGain,VideoAeInfo[0].Dgain,VideoAeInfo[1].AgcGain,VideoAeInfo[1].Dgain);
        AmbaImg_Proc_Cmd(MW_IP_ADJ_VIDEO_HDR_CHK_DGAINMODE, (UINT32)&VideoAeInfo[0],0,0);
        //AmbaPrint("        %12.5f,%5d,    %12.5f,%5d,          ",VideoAeInfo[0].AgcGain,VideoAeInfo[0].Dgain,VideoAeInfo[1].AgcGain,VideoAeInfo[1].Dgain);
        Amba_Adj_VideoHdr_Control(VideoAeInfo);
        //AmbaPrint("------- %12.5f,%5d,    %12.5f,%5d, ---------",VideoAeInfo[0].AgcGain,VideoAeInfo[0].Dgain,VideoAeInfo[1].AgcGain,VideoAeInfo[1].Dgain);
    }
    AmbaImg_Proc_Cmd(MW_IP_GET_MULTI_AE_ALGO_PARAMS, chNo, (UINT32)&AeAlgoInfo[chNo], 0);
    AmbaImg_Proc_Cmd(MW_IP_Get_FRO_WB_CHK, chNo, (UINT32)&FrontWbCheck, 0);

    if((FrontWbCheck == 0) || (HdrEnable == 0)){
       AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, chNo, IP_MODE_VIDEO, (UINT32)&VideoWbGain);
    }

    VideoSchdlrExpInfo.Info.SubChannelNum = 0;
    VideoSchdlrExpInfo.Info.AGC[0] = VideoAeInfo[0].AgcGain;
    VideoSchdlrExpInfo.Info.Shutter[0] = VideoAeInfo[0].ShutterTime;
    VideoSchdlrExpInfo.Info.IrisIndex = VideoAeInfo[0].IrisIndex;
    VideoSchdlrExpInfo.Info.Mode = *pMode;
    VideoSchdlrExpInfo.Info.DGain = VideoWbGain;
    VideoSchdlrExpInfo.Info.DGain.AeGain = VideoAeInfo[0].Dgain;
    VideoSchdlrExpInfo.Info.DGain.GlobalDGain = AeAlgoInfo[chNo].DefSetting.GlobalDGain;
    if(HdrEnable == IMG_DSP_HDR_MODE_0){
        VideoSchdlrExpInfo.Info.SubChannelNum = 1;
        VideoSchdlrExpInfo.Info.AGC[1] = VideoAeInfo[1].AgcGain;
        VideoSchdlrExpInfo.Info.Shutter[1] = VideoAeInfo[1].ShutterTime;
        //AmbaPrint("%s, %f, %f", __FUNCTION__, VideoSchdlrExpInfo.Info.AGC[1], VideoSchdlrExpInfo.Info.Shutter[1]);

        /* HDR filter */
        if (VideoPipeCtrl) {
            if (VideoPipeCtrl->HdrBlendingInfoUpdate) {
                memcpy(&VideoSchdlrExpInfo.Info.BlendInfo[0], &VideoPipeCtrl->HdrBlendingInfo, sizeof(AMBA_DSP_IMG_HDR_BLENDING_INFO_s));
                VideoSchdlrExpInfo.Info.HdrUpdated[0].Bits.BlendInfo = 1;
            }
            if (VideoPipeCtrl->HdrAlphaConfig0Update) {
                memcpy(&VideoSchdlrExpInfo.Info.AlphaCalc[0], &VideoPipeCtrl->HdrAlphaConfig0, sizeof(AMBA_DSP_IMG_HDR_ALPHA_CALC_CONFIG_s));
                VideoSchdlrExpInfo.Info.HdrUpdated[0].Bits.AlphaCalc = 1;
            }
            if (VideoPipeCtrl->HdrAlphaThreshold0Update) {
                memcpy(&VideoSchdlrExpInfo.Info.AlphaThresh[0], &VideoPipeCtrl->HdrAlphaThreshold0, sizeof(AMBA_DSP_IMG_HDR_ALPHA_CALC_THRESH_s));
                VideoSchdlrExpInfo.Info.HdrUpdated[0].Bits.AlphaCalcThrd = 1;
            }
            if (VideoPipeCtrl->BlackCorrUpdate) {
                memcpy(&VideoSchdlrExpInfo.Info.BlackCorr[0], &VideoPipeCtrl->BlackCorr, sizeof(AMBA_DSP_IMG_BLACK_CORRECTION_s));
                VideoSchdlrExpInfo.Info.HdrUpdated[0].Bits.BlackCorr = 1;
            }
            if (VideoPipeCtrl->HdrAmpLinearization0Update) {
                memcpy(&VideoSchdlrExpInfo.Info.AmpLinear[0], &VideoPipeCtrl->HdrAmpLinearization0, sizeof(AMBA_DSP_IMG_AMP_LINEARIZATION_s));
                VideoSchdlrExpInfo.Info.HdrUpdated[0].Bits.AmpLinear = 1;
            }
        }
    }

    if(type == 255){
        VideoSchdlrExpInfo.Type = AMBA_IMG_SCHDLR_SET_TYPE_DIRECT;
        VideoSchdlrExpInfo.Info.AdjUpdated = 1;
        //fill hdr update above
        //VideoSchdlrExpInfo.Info.HdrUpdated = 1;
    }else{
        VideoSchdlrExpInfo.Type = AMBA_IMG_SCHDLR_SET_TYPE_NORMAL;//By scheduler
        VideoSchdlrExpInfo.Info.AdjUpdated = (UpdateFlg % 2);
        //fill hdr update above
        //VideoSchdlrExpInfo.Info.HdrUpdated = (UpdateFlg >> 1);
        //AmbaPrint("<%s>, AdjUpdated : %d,HdrUpdated :%d", __FUNCTION__, VideoSchdlrExpInfo.Info.AdjUpdated, VideoSchdlrExpInfo.Info.HdrUpdated);
    }
    if(HdrEnable == IMG_DSP_HDR_MODE_0){
        if(AaaOpInfo.AeOp == ENABLE){
            AmbaImgSchdlr_SetExposure(chNo, &VideoSchdlrExpInfo);
        }
    }else{
        AmbaImgSchdlr_SetExposure(chNo, &VideoSchdlrExpInfo);
    }

}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: Amba_AeAwbAdj_Init
 *
 *  @Description:: To initialize the AE/AWB/ADJ algo related settings
 *
 *  @Input      ::
 *    UINT32 chNo : Channel number.
 *    UINT8 initFlg : All(0), Ae(1), Awb(2), Adj(3)
 *    AMBA_KAL_BYTE_POOL_t *pMMPL : pointer to mempool control block
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    None
\*-----------------------------------------------------------------------------------------------*/
                                                        //Task ID->                                         // ViewID
static IMGPROC_TASK_ENABLE_s DefTaskEnableTable[MAX_CHAN_NUM] = {{ 0, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // 0  |
                                                                 { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // 1  V
                                                                 { 2, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // 2
                                                                 { 3, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // 3
                                                                 { 4, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // 4
                                                                 { 5, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // 5
                                                                 { 6, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // 6
                                                                 { 7, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // 7
                                                                 { 8, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // 8
                                                                 { 9, -1, -1, -1, -1, -1, -1, -1, -1, -1}}; // 9
static IMGPROC_TASK_ENABLE_s TaskEnableTmp[MAX_CHAN_NUM];
void Amba_AeAwbAdj_Init(UINT32 chNo,UINT8 initFlg, AMBA_KAL_BYTE_POOL_t *pMMPL)
{
    AMBA_3A_OP_INFO_s  AaaOpInfo = {ENABLE, ENABLE, ENABLE, ENABLE};
    AMBA_3A_STATUS_s    VideoStatus = {AMBA_IDLE, AMBA_IDLE, AMBA_IDLE};
    AMBA_3A_STATUS_s    StillStatus = {AMBA_IDLE, AMBA_IDLE, AMBA_IDLE};
    AMBA_DSP_IMG_MODE_CFG_s Mode;
    UINT16 PipeMode = 0;
    UINT16 VideoAlgoMode = 0;
    AMBA_DSP_IMG_ALGO_MODE_e AlgoMode;
    UINT16 UpdateFlg = 0;
    UINT8  HdrEnable = 0;

    memset(&Mode, 0, sizeof(Mode));
    AmbaImg_Proc_Cmd(MW_IP_GET_MODE_CFG, chNo, 0, (UINT32)&Mode);
    AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_HDR_ENABLE, chNo, (UINT32)&HdrEnable, 0);
    if(HdrEnable == IMG_DSP_HDR_MODE_0){
        Mode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_VHDR;
        Amba_Img_Get_TaskEnableTable(0, TaskEnableTmp); //for Ae
        TaskEnableTmp[0].TaskEnableId[0] = 0;
        TaskEnableTmp[0].TaskEnableId[1] = 1;
        Amba_Img_Set_TaskEnableTable(0, TaskEnableTmp); //for Ae
    }else{
        Amba_Img_Set_TaskEnableTable(0, DefTaskEnableTable); //for Ae
	Amba_Img_Set_TaskEnableTable(1, DefTaskEnableTable); //for Awb
	Amba_Img_Set_TaskEnableTable(2, DefTaskEnableTable); //for Adj
    }

    AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_MODE, chNo, (UINT32)&PipeMode, 0);
    AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_ALGO_MODE, (UINT32)chNo, (UINT32)&VideoAlgoMode, 0);
    if (VideoAlgoMode == IP_MODE_LISO_VIDEO) {
        AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
    } else if (VideoAlgoMode == IP_MODE_HISO_VIDEO) {
        AlgoMode = AMBA_DSP_IMG_ALGO_MODE_HISO;
    } else {
        AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
    }

    if(PipeMode == IP_HYBRID_MODE){
        Mode.ContextId = AmbaImgSchdlr_GetIsoCtxIndex(chNo, AlgoMode);
    } else {
        Mode.ContextId = 0; // TBD
    }

    if (pMMPL != NULL) {
      AmbaImg_Proc_Cmd(MW_IP_SET_MEM_CTRLADDR, (UINT32)pMMPL, 0, 0);
    }

    AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, chNo, (UINT32)&AaaOpInfo, 0);
    AmbaImg_Proc_Cmd(MW_IP_SET_3A_STATUS, chNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);

    if (AaaOpInfo.AeOp == ENABLE && (initFlg == 0 || initFlg == 1)) {
        UINT32 ChCount = 1,i;
        AmbaImg_Proc_Cmd(MW_IP_GET_TOTAL_CH_COUNT, (UINT32)&ChCount, 1, 0);
        //AmbaSample_AeInit(chNo);
        for(i = 0;i < ChCount;i++) {
            AmbaImg_Proc_Cmd(MW_IP_AMBA_AEAWBADJ_INIT, i, 1, 0);
        }
    }

    if (AaaOpInfo.AwbOp == ENABLE && (initFlg == 0 || initFlg == 2)) {
        //AmbaSample_AwbInit(chNo);
        AmbaImg_Proc_Cmd(MW_IP_AMBA_AEAWBADJ_INIT, chNo, 2, 0);
    }

    if (AaaOpInfo.AdjOp == ENABLE && (initFlg == 0 || initFlg == 3)) {

        AmbaSample_AdjInit(chNo);

        //AmbaImg_Proc_Cmd(MW_IP_AMBA_AEAWBADJ_INIT, chNo, 3, 0);
    }
    AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_PIPE_CTRL_PARAMS, chNo, (UINT32)&Mode, 0);

    Amba_Img_Get_Video_Pipe_Ctrl_UpdateFlg(chNo, &UpdateFlg);
    Amba_AeAwbAdj_Set_SchdlrExpInfo(chNo, &Mode, 255, UpdateFlg);
    AmbaImg_Proc_Cmd(MW_IP_RESET_VIDEO_PIPE_CTRL_FLAGS, chNo, 0, 0);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: Amba_AeAwbAdj_Control
 *
 *  @Description:: Entry point of AE/AWB/ADJ algo.
 *
 *  @Input      ::
 *    UINT32 chNo : Handler for multi-task.
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    None
\*-----------------------------------------------------------------------------------------------*/
void Amba_AeAwbAdj_Control(UINT32 chNo)
{
    AMBA_DSP_IMG_MODE_CFG_s Mode;
    AMBA_3A_OP_INFO_s   AaaOpInfo;
    UINT16 PipeMode = 0;
    UINT16 VideoAlgoMode = 0;
    AMBA_DSP_IMG_ALGO_MODE_e AlgoMode;
    UINT16 UpdateFlg = 0;
    UINT8  HdrEnable = 0;

    memset(&Mode, 0, sizeof(Mode));
    AmbaImg_Proc_Cmd(MW_IP_GET_MODE_CFG, chNo, 0, (UINT32)&Mode);
    AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_HDR_ENABLE, chNo, (UINT32)&HdrEnable, 0);
    if(HdrEnable == IMG_DSP_HDR_MODE_0){
        Mode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_VHDR;
    }

    AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, chNo, (UINT32)&AaaOpInfo, 0);

    AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_MODE, chNo, (UINT32)&PipeMode, 0);
    AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_ALGO_MODE, (UINT32)chNo, (UINT32)&VideoAlgoMode, 0);
    if (VideoAlgoMode == IP_MODE_LISO_VIDEO) {
        AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
    } else if (VideoAlgoMode == IP_MODE_HISO_VIDEO) {
        AlgoMode = AMBA_DSP_IMG_ALGO_MODE_HISO;
    } else {
        AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
    }

    if(PipeMode == IP_HYBRID_MODE){
        Mode.ContextId = AmbaImgSchdlr_GetIsoCtxIndex(chNo, AlgoMode);
    } else {
        Mode.ContextId = 0; // TBD
    }

    if (AaaOpInfo.AdjOp == ENABLE) {
        Amba_Img_Get_Video_Pipe_Ctrl_UpdateFlg(chNo, &UpdateFlg);
        AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_PIPE_CTRL_PARAMS, chNo, (UINT32)&Mode, 0);
//        AmbaImg_Proc_Cmd(MW_IP_RESET_VIDEO_PIPE_CTRL_FLAGS, chNo, 0, 0);
    }

    Amba_AeAwbAdj_Set_SchdlrExpInfo(chNo, &Mode, 0, UpdateFlg);
    if (AaaOpInfo.AdjOp == ENABLE) {
        AmbaImg_Proc_Cmd(MW_IP_RESET_VIDEO_PIPE_CTRL_FLAGS, chNo, 0, 0);
    }

    //AmbaPrint("<%s>, %d", __FUNCTION__, chNo);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: Amba_Ae_Ctrl
 *
 *  @Description:: Entry point of Ae algo.
 *
 *  @Input      ::
 *    UINT32 chNo : Handler for multi-task.
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    None
\*-----------------------------------------------------------------------------------------------*/
void Amba_Ae_Ctrl(UINT32 chNo)
{
    AMBA_3A_OP_INFO_s   AaaOpInfo;
    AMBA_3A_STATUS_s    VideoStatus = {0, 0, 0};
    AMBA_3A_STATUS_s    StillStatus = {0, 0, 0};

    //AmbaPrint("<%s>, %d", __FUNCTION__, chNo);
    AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, chNo, (UINT32)&AaaOpInfo, 0);
    AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, chNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
    if (AaaOpInfo.AeOp == ENABLE) {
        {  /* Start of flicker detection */
            FLICKER_DETECT_STATUS_s     FlkDetectStatus;
            Img_Get_Flicker_Detection_Status(&FlkDetectStatus);
            if (FlkDetectStatus.Enable == ENABLE) {
                if (FlkDetectStatus.Running == STOP) {
                    Img_Flicker_Detection(DISABLE, 0);
                }
                Img_Flicker_Detection(FlkDetectStatus.Enable, 0);
            }
        }  /* End of flicker detection */

        AmbaImg_Proc_Cmd(MW_IP_AMBA_AE_CONTROL, chNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
        AmbaImg_Proc_Cmd(MW_IP_SET_AE_STATUS, chNo, (UINT32)VideoStatus.Ae, (UINT32)StillStatus.Ae);
    //
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: Amba_Awb_Ctrl
 *
 *  @Description:: Entry point of Awb algo.
 *
 *  @Input      ::
 *    UINT32 chNo : Handler for multi-task.
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    None
\*-----------------------------------------------------------------------------------------------*/
void Amba_Awb_Ctrl(UINT32 chNo)
{
    AMBA_3A_OP_INFO_s   AaaOpInfo;
    AMBA_3A_STATUS_s    VideoStatus = {0, 0, 0};
    AMBA_3A_STATUS_s    StillStatus = {0, 0, 0};

    AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, chNo, (UINT32)&AaaOpInfo, 0);
    AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, chNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
    if (AaaOpInfo.AwbOp == ENABLE) {
        //AmbaSample_AwbControl(chNo, &VideoStatus, &VideoStatus);
        AmbaImg_Proc_Cmd(MW_IP_AMBA_AWB_CONTROL, chNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
        //AmbaImg_Proc_Cmd(MW_IP_SET_3A_STATUS, chNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
        AmbaImg_Proc_Cmd(MW_IP_SET_AWB_STATUS, chNo, (UINT32)VideoStatus.Awb, (UINT32)StillStatus.Awb);
        //AmbaPrint("<%s>, %d", __FUNCTION__, chNo);
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: Amba_Adj_Ctrl
 *
 *  @Description:: Entry point of ADJ algo.
 *
 *  @Input      ::
 *    UINT32 chNo : Handler for multi-task.
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    None
\*-----------------------------------------------------------------------------------------------*/
void Amba_Adj_Ctrl(UINT32 chNo)
{

    AMBA_3A_OP_INFO_s   AaaOpInfo;
    AMBA_DSP_IMG_MODE_CFG_s Mode;

    memset(&Mode, 0, sizeof(Mode));
    AmbaImg_Proc_Cmd(MW_IP_GET_MODE_CFG, chNo, 0, (UINT32)&Mode);
    AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, chNo, (UINT32)&AaaOpInfo, 0);
    if (AaaOpInfo.AdjOp == ENABLE) {
		AmbaSample_AdjControl(chNo);
		AmpUTCalibAdjust_Func();
    }
}