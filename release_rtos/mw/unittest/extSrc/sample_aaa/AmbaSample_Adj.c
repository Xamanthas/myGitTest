/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSample_Adj.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Sample Auto Adjust algorithm
 *
 *  @History        ::
 *      Date        Name        Comments
 *      03/12/2013  Jyh-Jiun Li Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "AmbaDataType.h"
#include "AmbaPrint.h"

#include "AmbaDSP_ImgFilter.h"
#include "AmbaDSP_Img3aStatistics.h"

#include "AmbaImg_Proc.h"
#include "AmbaImg_AeAwb.h"
#include "AmbaSample_AeAwbAdj.h"
#include "AmbaImg_Impl_Cmd.h"

#ifdef CHIP_REV_A9 
    #include "AmbaImg_Adjustment_A9.h"
#else
    #include "AmbaImg_Adjustment_A12.h"
#endif 
//#include "AmbaImg_Adj_Func.h"

#include "AmbaImg_AaaDef.h"

static ADJ_STILL_CONTROL_s AmbaAdjStillCtrlPre[8];
//static ADJ_STILL_CONTROL_s AmbaAdjPivCtrlPre[8];
/*
 * Variables of App Adj test code, for testing bin file load from SD card
*/
static UINT8 AmbaAdjVideoLoadBinTest = 0;
static UINT8 AmbaAdjPhotoLoadBinTest = 0;
static UINT8 AmbaAdjStillLoadBinTest = 0;

static ADJ_VIDEO_PARAM_s AmbaAdjVideoTmp = {0};
static ADJ_PHOTO_PARAM_s AmbaAdjPhotoTmp = {0};
static ADJ_STILL_FAST_LISO_PARAM_S AmbaAdjStillLIsoTmp = {0};
static ADJ_STILL_HISO_PARAM_s      AmbaAdjStillHIsoTmp = {0};
static COLOR_3D_s        AmbaAdjColorTmp = {0};
static ADJ_VIDEO_PARAM_s *pAmbaAdjVideoTmp;
static ADJ_PHOTO_PARAM_s *pAmbaAdjPhotoTmp;
static ADJ_STILL_FAST_LISO_PARAM_S *pAmbaAdjStillLIsoTmp;
static ADJ_STILL_HISO_PARAM_s      *pAmbaAdjStillHIsoTmp;

static UINT8  AmbaAdjStillLIsoAddFlg = 0; 
static UINT32 AmbaAdjStillLIsoAddOri = 0;
static UINT8  AmbaAdjStillHIsoAddFlg = 0;
static UINT32 AmbaAdjStillHIsoAddOri = 0;

static int AmbaAdjTableNo[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static UINT8  AmbaAdjMaunalEnable[10] = {0,0,0,0,0,0,0,0,0,0};
static UINT32 AmbaAdjMaunalTableNo[10] = {0,0,0,0,0,0,0,0,0,0};

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaSample_AdjGetNfEv
 *
 *  @Description:: Calculate nfIndex, shutterIndex, and evIndex
 *
 *  @Input      ::
 *    UINT32 chNo : channel number
 *
 *  @Output     :: none
 *    UINT32 *nfIndex      : Pointer to nfIndex
 *    UINT32 *shutterIndex : Pointer to shutterIndex
 *    UINT32 *evIndex      : Pointer to evIndex
 *
 *  @Return     :: none
 *
\*-----------------------------------------------------------------------------------------------*/
void AmbaSample_AdjGetNfEv(UINT32 chNo,UINT32 *nfIndex,UINT32 *shutterIndex,UINT32 *evIndex)
{
    float       shutter, agc;
    UINT32      dgain;
    float       BaseRate = 1.001 / 30;

    AeGetVideoExposureValue(chNo, &shutter, &agc, &dgain);
    agc = agc * 1024 / 6;
    *nfIndex = (UINT32)agc * dgain / 4096;
    *shutterIndex = (UINT32)(shutter * 3 * 1024 / BaseRate);
    shutter = (UINT32)(shutter * 7 * 1024 / BaseRate);
    *evIndex = (UINT32)shutter + (*nfIndex);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Decide_AdjTableNo
 *
 *  @Description:: decide ADJ table number
 *
 *  @Input      ::
 *    none
 *
 *  @Output     :: 
 *   UINT32 *pAdjTableNo : pointer to ADJ table number
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
void _Decide_AdjTableNo(UINT32 chNo, UINT32 *pAdjTableNo)
{
    /*
    *  Setup conditions to choose ADJ table number
    */
    if(AmbaAdjTableNo[chNo] > 0){
        *pAdjTableNo = AmbaAdjTableNo[chNo];
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Set_AdjTableNo
 *
 *  @Description:: Set ADJ table number
 *
 *  @Input      ::
 *   UINT32 AdjTableNo : the ADJ table number
 *
 *  @Output     :: 
 *   none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
void _Set_AdjTableNo(UINT32 chNo, int AdjTableNo)
{
    /*
    *  Setup conditions to choose ADJ table number
    */
    AmbaAdjTableNo[chNo] = AdjTableNo;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Decide_StillAdjTableNo
 *
 *  @Description:: decide Still ADJ table number
 *
 *  @Input      ::
 *    none
 *
 *  @Output     :: 
 *   UINT32 *pAdjTableNo : pointer to ADJ table number
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
void _Decide_StillAdjTableNo(UINT32 *pAdjTableNo)
{
    /*
    *  Setup conditions to choose ADJ table number
    */
    //*pAdjTableNo = 0;
}

void AmbaSample_Adj_VideoHdr_Init(void)
{
     HDR_INFO_s               HdrInfo;
     
     memset(&HdrInfo, 0, sizeof(HdrInfo));
     
     AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, 0, IP_MODE_VIDEO, (UINT32)&HdrInfo.AeInfo[0]); //long
     AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, 1, IP_MODE_VIDEO, (UINT32)&HdrInfo.AeInfo[1]); //short
     
     AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, 0, IP_MODE_VIDEO, (UINT32)&HdrInfo.WbGain[0]); //long
     AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, 0, IP_MODE_VIDEO, (UINT32)&HdrInfo.WbGain[1]); //short
     
     AmbaImg_Proc_Cmd(MW_IP_ADJ_VIDEO_HDR_INIT, 0, (UINT32)&HdrInfo, 0);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaSample_AdjInit
 *
 *  @Description:: To initialize the ADJ algorithm
 *
 *  @Input      ::
 *    UINT32 chNo : channel number
 *
 *  @Output     :: none
 *
 *  @Return     :: none
 *
\*-----------------------------------------------------------------------------------------------*/
extern int Amba_Adj_Video_Init(UINT32 chNo);
void AmbaSample_AdjInit(UINT32 chNo)
{
    UINT32                    NfIndex, ShutterIndex, EvIndex, IsoValue, Dgain;
    UINT8                     ChkPhotoPreview = 0;
    ADJ_IQ_INFO_s             AdjVideoIqInfo;
    AMBA_DSP_IMG_WB_GAIN_s    WbGain;
    ADJ_VIDEO_PARAM_s         *AdjVideoAdd;
    ADJ_VIDEO_HISO_PARAM_s    *AdjVideoHIsoAdd;
    ADJ_PHOTO_PARAM_s         *AdjPhotoAdd;
    UINT32                    VideoAddTmp, TableNo = 0;
    AMBA_AE_INFO_s            AeInfotmp;
    UINT16                    VideoAlgoMode = 0;
    AMBA_DSP_IMG_MODE_CFG_s   Mode;
    UINT8                     HdrEnable = 0;
    UINT16                    PipeMode = 0;
    AMBA_DSP_IMG_ALGO_MODE_e  AlgoMode;

    memset(&Mode, 0, sizeof(Mode));

    Amba_Adj_Video_Init(chNo);
    //Amba AE algo.
    //////////////////////////////////////////////////
    AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, (UINT32)chNo, IP_MODE_VIDEO, (UINT32)&AeInfotmp);
    ShutterIndex = AeInfotmp.ShutterIndex;
    EvIndex = AeInfotmp.EvIndex;
    NfIndex = AeInfotmp.NfIndex;
    Dgain =  AeInfotmp.Dgain;
    IsoValue = AeInfotmp.IsoValue;
    //////////////////////////////////////////////////
    AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_ALGO_MODE, (UINT32)chNo, (UINT32)&VideoAlgoMode, 0);
    AmbaImg_Proc_Cmd(MW_IP_CHK_PHOTO_PREVIEW, (UINT32)&ChkPhotoPreview, 0, 0);
    
    //AmbaSample_AdjGetNfEv(chNo,&NfIndex,&ShutterIndex,&EvIndex);
    AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, chNo, IP_MODE_VIDEO, (UINT32)&WbGain);
    
    AdjVideoIqInfo.Ae.ShutterIndex = ShutterIndex;
    AdjVideoIqInfo.Ae.EvIndex = EvIndex;
    AdjVideoIqInfo.Ae.NfIndex = NfIndex;
    AdjVideoIqInfo.Ae.Dgain = Dgain;
    AdjVideoIqInfo.Ae.IsoValue = IsoValue;
    AdjVideoIqInfo.Ae.Flash = 0;
    AdjVideoIqInfo.Wb = WbGain;
    
    

    if (ChkPhotoPreview == 0) {
        
        if(AmbaAdjMaunalEnable[chNo]==0) {
		AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_TABLE_NO, 0, (UINT32)&TableNo, 0);
		_Decide_AdjTableNo(chNo,&TableNo); //Get Adj Table No.
	} else {
		TableNo = AmbaAdjMaunalTableNo[chNo];
	}
        
        if(VideoAlgoMode == IP_MODE_HISO_VIDEO){
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)chNo, IQ_PARAMS_VIDEO_HISO_ADJ, (UINT32)&VideoAddTmp);
            AdjVideoHIsoAdd = (ADJ_VIDEO_HISO_PARAM_s *)VideoAddTmp;
            AdjVideoIqInfo.Mode = IP_PREVIEW_MODE;    
            AdjVideoIqInfo.AwbAeParamAdd = (UINT32)&AdjVideoHIsoAdd[TableNo].NormalAwbAe;
            AdjVideoIqInfo.HisoFilterParamAdd = (UINT32)&AdjVideoHIsoAdd[TableNo].FilterParam;
            AdjVideoIqInfo.ColorParamAdd  = (UINT32)&AdjVideoHIsoAdd[TableNo].FilterParam.Def.Color;        
        }else{
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)chNo, IQ_PARAMS_VIDEO_ADJ, (UINT32)&VideoAddTmp);
            AdjVideoAdd = (ADJ_VIDEO_PARAM_s *)VideoAddTmp;
            AdjVideoIqInfo.Mode = IP_PREVIEW_MODE;    
            AdjVideoIqInfo.AwbAeParamAdd = (UINT32)&AdjVideoAdd[TableNo].AwbAe;
            AdjVideoIqInfo.FilterParamAdd = (UINT32)&AdjVideoAdd[TableNo].FilterParam;
            AdjVideoIqInfo.ColorParamAdd  = (UINT32)&AdjVideoAdd[TableNo].FilterParam.Def.Color;        
        }
        
    } else {
        
        AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_TABLE_NO, 1, (UINT32)&TableNo, 0);
	_Decide_AdjTableNo(chNo,&TableNo); //Get Adj Table No.
        AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)chNo, IQ_PARAMS_PHOTO_ADJ, (UINT32)&VideoAddTmp);
        AdjPhotoAdd = (ADJ_PHOTO_PARAM_s *)VideoAddTmp;
        VideoAlgoMode = IP_MODE_LISO_VIDEO;
        AdjVideoIqInfo.Mode = IP_CAPTURE_MODE;    
        AdjVideoIqInfo.AwbAeParamAdd = (UINT32)&AdjPhotoAdd[TableNo].NormalAwbAe;
        AdjVideoIqInfo.FilterParamAdd = (UINT32)&AdjPhotoAdd[TableNo].FilterParam;
        AdjVideoIqInfo.ColorParamAdd  = (UINT32)&AdjPhotoAdd[TableNo].FilterParam.Def.Color;
        
    }
    AmbaImg_Proc_Cmd(MW_IP_SET_IQ_INFO, (UINT32)chNo, (UINT32)IP_MODE_VIDEO , (UINT32)&AdjVideoIqInfo);
    if (VideoAlgoMode == IP_MODE_HISO_VIDEO) {
        AdjVideoIqInfo.Mode = IP_MODE_HISO_VIDEO;    
    } else {
        AdjVideoIqInfo.Mode = IP_MODE_LISO_VIDEO;    
    }
    
    AmbaImg_Proc_Cmd(MW_IP_ADJ_VIDEO_CONTROL, (UINT32)chNo , (UINT32)&AdjVideoIqInfo , 1);
    AmbaImg_Proc_Cmd(MW_IP_GET_MODE_CFG, chNo, 0, (UINT32)&Mode);

    AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_HDR_ENABLE, chNo, (UINT32)&HdrEnable, 0);
    
    if(HdrEnable == IMG_DSP_HDR_MODE_0){
        AmbaSample_Adj_VideoHdr_Init();
	Mode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_VHDR;
    }else{
        HDR_INFO_s               HdrInfo;
        memset(&HdrInfo, 0, sizeof(HdrInfo));
        AmbaImg_Proc_Cmd(MW_IP_ADJ_VIDEO_HDR_INIT, 0, (UINT32)&HdrInfo, 0);
    }
    AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_MODE, chNo, (UINT32)&PipeMode, 0);
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
    AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_PIPE_CTRL_PARAMS, chNo, (UINT32)&Mode, 0);
    //AmbaImg_Proc_Cmd(MW_IP_RESET_VIDEO_PIPE_CTRL_FLAGS, chNo, 0, 0);
 
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaSample_AdjControl
 *
 *  @Description:: ADJ algorithm control API
 *
 *  @Input      ::
 *    UINT32 chNo : channel number
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
void AmbaSample_AdjControl(UINT32 chNo)
{
    UINT32                    NfIndex, ShutterIndex, EvIndex, IsoValue, Dgain;
    UINT8                     ChkPhotoPreview = 0;
    ADJ_IQ_INFO_s             AdjVideoIqInfo, AdjVideoIqInfo1;
    AMBA_DSP_IMG_WB_GAIN_s    WbGain;
    ADJ_VIDEO_PARAM_s         *AdjVideoAdd;
    ADJ_VIDEO_HISO_PARAM_s    *AdjVideoHIsoAdd;
    ADJ_PHOTO_PARAM_s         *AdjPhotoAdd;
    UINT32                    VideoAddTmp, TableNo = 0;
    AMBA_AE_INFO_s AeInfotmp;
    UINT16                    VideoAlgoMode = 0;
    UINT8                     HdrEnable = 0;
    
    //Amba AE algo.
    //////////////////////////////////////////////////
    AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, (UINT32)chNo, IP_MODE_VIDEO, (UINT32)&AeInfotmp);
    ShutterIndex = AeInfotmp.ShutterIndex;
    EvIndex = AmbaImg_Proc_Cmd(MW_IP_EXPS_TO_EV_IDX, (UINT32)chNo, IP_MODE_VIDEO, (UINT32)&AeInfotmp);
    NfIndex = AmbaImg_Proc_Cmd(MW_IP_EXPS_TO_NF_IDX, (UINT32)chNo, IP_MODE_VIDEO, (UINT32)&AeInfotmp);
    Dgain =  AeInfotmp.Dgain;
    IsoValue = AeInfotmp.IsoValue;
    //////////////////////////////////////////////////
    AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_ALGO_MODE, (UINT32)chNo, (UINT32)&VideoAlgoMode, 0);    
    AmbaImg_Proc_Cmd(MW_IP_CHK_PHOTO_PREVIEW, (UINT32)&ChkPhotoPreview, 0, 0);
    //ChkPhotoPreview = 1; //Test
    //AmbaSample_AdjGetNfEv(chNo,&NfIndex,&ShutterIndex,&EvIndex);
    AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, chNo, IP_MODE_VIDEO, (UINT32)&WbGain);
    
    AdjVideoIqInfo.Ae.ShutterIndex = ShutterIndex;
    AdjVideoIqInfo.Ae.EvIndex = EvIndex;
    AdjVideoIqInfo.Ae.NfIndex = NfIndex;
    AdjVideoIqInfo.Ae.Dgain = Dgain;
    AdjVideoIqInfo.Ae.IsoValue = IsoValue;
    AdjVideoIqInfo.Ae.Flash = 0;
    AdjVideoIqInfo.Wb = WbGain;
    
    /*
     * Start of App Adj test code, for testing bin file load from SD card
    */
    if ((ChkPhotoPreview == 0) && (AmbaAdjVideoLoadBinTest == 1)) {
        if(AmbaAdjVideoTmp.VersionNumber != 0){
            AdjVideoAdd = &AmbaAdjVideoTmp;
            TableNo = 0;
        }
    } else if ((ChkPhotoPreview == 1) && (AmbaAdjPhotoLoadBinTest == 1)) {
        if(AmbaAdjPhotoTmp.VersionNumber != 0){
            AdjPhotoAdd = &AmbaAdjPhotoTmp;
            TableNo = 0;
        }
    }
    
    /*
     * End of App Adj test code, for testing bin file load from SD card
     */       
    if (ChkPhotoPreview == 0) {
        
        if(AmbaAdjMaunalEnable[chNo]==0) {
		AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_TABLE_NO, 0, (UINT32)&TableNo, 0);
		_Decide_AdjTableNo(chNo,&TableNo); //Get Adj Table No.
	} else {
		TableNo = AmbaAdjMaunalTableNo[chNo];
	}
        
        if(VideoAlgoMode == IP_MODE_HISO_VIDEO){
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)chNo, IQ_PARAMS_VIDEO_HISO_ADJ, (UINT32)&VideoAddTmp);
            AdjVideoHIsoAdd = (ADJ_VIDEO_HISO_PARAM_s *)VideoAddTmp;
            AdjVideoIqInfo.Mode = IP_PREVIEW_MODE;    
            AdjVideoIqInfo.AwbAeParamAdd = (UINT32)&AdjVideoHIsoAdd[TableNo].NormalAwbAe;
            AdjVideoIqInfo.HisoFilterParamAdd = (UINT32)&AdjVideoHIsoAdd[TableNo].FilterParam;
            AdjVideoIqInfo.ColorParamAdd  = (UINT32)&AdjVideoHIsoAdd[TableNo].FilterParam.Def.Color;        
        }else{
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)chNo, IQ_PARAMS_VIDEO_ADJ, (UINT32)&VideoAddTmp);
            AdjVideoAdd = (ADJ_VIDEO_PARAM_s *)VideoAddTmp;
            AdjVideoIqInfo.Mode = IP_PREVIEW_MODE;    
            AdjVideoIqInfo.AwbAeParamAdd = (UINT32)&AdjVideoAdd[TableNo].AwbAe;
            AdjVideoIqInfo.FilterParamAdd = (UINT32)&AdjVideoAdd[TableNo].FilterParam;
            AdjVideoIqInfo.ColorParamAdd  = (UINT32)&AdjVideoAdd[TableNo].FilterParam.Def.Color;        
        }
        
    } else {
        
        AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_TABLE_NO, 1, (UINT32)&TableNo, 0);
	_Decide_AdjTableNo(chNo,&TableNo); //Get Adj Table No.
        AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)chNo, IQ_PARAMS_PHOTO_ADJ, (UINT32)&VideoAddTmp);
        AdjPhotoAdd = (ADJ_PHOTO_PARAM_s *)VideoAddTmp;
        VideoAlgoMode = IP_MODE_LISO_VIDEO;
        AdjVideoIqInfo.Mode = IP_CAPTURE_MODE;    
        AdjVideoIqInfo.AwbAeParamAdd = (UINT32)&AdjPhotoAdd[TableNo].NormalAwbAe;
        AdjVideoIqInfo.FilterParamAdd = (UINT32)&AdjPhotoAdd[TableNo].FilterParam;
        AdjVideoIqInfo.ColorParamAdd  = (UINT32)&AdjPhotoAdd[TableNo].FilterParam.Def.Color;
        
    }
    AmbaImg_Proc_Cmd(MW_IP_ADJ_AWBAE_CONTROL, (UINT32)chNo , (UINT32)&AdjVideoIqInfo , 0); //Amba only    
    
    AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_HDR_ENABLE, chNo, (UINT32)&HdrEnable, 0);
    if(HdrEnable == IMG_DSP_HDR_MODE_0){
		AdjVideoIqInfo1 = AdjVideoIqInfo;
        //Amba AE algo.
        //////////////////////////////////////////////////
        AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, (UINT32)chNo + 1, IP_MODE_VIDEO, (UINT32)&AeInfotmp);
        ShutterIndex = AeInfotmp.ShutterIndex;
        EvIndex = AmbaImg_Proc_Cmd(MW_IP_EXPS_TO_EV_IDX, (UINT32)chNo + 1, IP_MODE_VIDEO, (UINT32)&AeInfotmp);
        NfIndex = AmbaImg_Proc_Cmd(MW_IP_EXPS_TO_NF_IDX, (UINT32)chNo + 1, IP_MODE_VIDEO, (UINT32)&AeInfotmp);
        Dgain =  AeInfotmp.Dgain;
        IsoValue = AeInfotmp.IsoValue;
        //////////////////////////////////////////////////
        
        AdjVideoIqInfo1.Ae.ShutterIndex = ShutterIndex;
        AdjVideoIqInfo1.Ae.EvIndex = EvIndex;
        AdjVideoIqInfo1.Ae.NfIndex = NfIndex;
        AdjVideoIqInfo1.Ae.Dgain = Dgain;
        AdjVideoIqInfo1.Ae.IsoValue = IsoValue;
        AdjVideoIqInfo1.Ae.Flash = 0;
        AdjVideoIqInfo1.Wb = WbGain;		
        AmbaImg_Proc_Cmd(MW_IP_ADJ_AWBAE_CONTROL, (UINT32)chNo + 1 , (UINT32)&AdjVideoIqInfo1 , 0); //Amba only, HDR

	//AmbaPrint("%s, chNo : %d , ShutterIndex : %d , EvIndex : %d", __FUNCTION__, chNo, AdjVideoIqInfo1.Ae.ShutterIndex, AdjVideoIqInfo1.Ae.EvIndex);
    }
    
    AmbaImg_Proc_Cmd(MW_IP_SET_IQ_INFO, (UINT32)chNo, (UINT32)IP_MODE_VIDEO , (UINT32)&AdjVideoIqInfo);
    if (VideoAlgoMode == IP_MODE_HISO_VIDEO) {
        AdjVideoIqInfo.Mode = IP_MODE_HISO_VIDEO;    
    } else {
        AdjVideoIqInfo.Mode = IP_MODE_LISO_VIDEO;    
    }
    
    //if(HdrEnable == IMG_DSP_HDR_MODE_0){
        //AmbaSample_Adj_VideoHdr_Control();
	//AmbaPrint("%s, chNo : %d", __FUNCTION__, chNo);
    //}
    AmbaImg_Proc_Cmd(MW_IP_ADJ_VIDEO_CONTROL, (UINT32)chNo , (UINT32)&AdjVideoIqInfo , 1);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaSample_AdjStillControl
 *
 *  @Description:: STILL ADJ algorithm control API
 *
 *  @Input      ::
 *    UINT32 chNo : channel number
 *    UINT32 aeIndx : Index to certain still AE information
 *    AMBA_DSP_IMG_MODE_CFG_s *mode : pointer to DSP control mode
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static float BaseStillBlcShtTime = 60.0;
void AmbaSample_AdjStillControl(UINT32 chNo, UINT32 aeIndx, AMBA_DSP_IMG_MODE_CFG_s *mode, UINT16 algoMode)
{
    /* Run Adj compute */
    
    UINT8 StillAlgoMode = 0;
    UINT32 ChNoChk = 0, StillAdjTableNo = 0;
    UINT16 ShutterIndex = 0;
    AMBA_3A_OP_INFO_s AaaOpInfo;
    AMBA_AE_INFO_s stillAeInfo[MAX_AEB_NUM];
    ADJ_STILL_CONTROL_s adjStillCtrl;
    AMBA_DSP_IMG_WB_GAIN_s    StillWbGain = {WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN};
    
    AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, chNo, (UINT32)&AaaOpInfo, 0);
    
    if (chNo > 9) {
        ChNoChk = 9;
    } else {
        ChNoChk = chNo;
    }
    
    if (AaaOpInfo.AdjOp == 1) {
        memset(&adjStillCtrl, 0x0, sizeof(ADJ_STILL_CONTROL_s));
        AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, chNo, IP_MODE_STILL, (UINT32)stillAeInfo);
        AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, chNo, IP_MODE_STILL, (UINT32)&StillWbGain);
        
        StillAlgoMode = (algoMode == 0)? (IP_MODE_HISO_STILL): (IP_MODE_LISO_STILL);
        ShutterIndex = (UINT16)(log2(BaseStillBlcShtTime/stillAeInfo[aeIndx].ShutterTime) * 128);
        //AmbaPrint("\n\n\n<%s>, ShutterIndex : %4d ", __FUNCTION__, ShutterIndex);
        adjStillCtrl.StillMode = StillAlgoMode;//stillAeInfo[0].Mode;//IP_MODE_HISO_STILL;//IP_MODE_LISO_STILL;
        adjStillCtrl.ShIndex = ShutterIndex;//stillAeInfo[aeIndx].ShutterIndex;
        adjStillCtrl.EvIndex = stillAeInfo[aeIndx].EvIndex;
        adjStillCtrl.NfIndex = stillAeInfo[aeIndx].NfIndex;
        adjStillCtrl.WbGain = StillWbGain;
        adjStillCtrl.DZoomStep = 0;
        adjStillCtrl.FlashMode = 0;
        
        AmbaPrint("\n\n StillMode : %d, ShIndex : %5d, EvIndex : %5d, NfIndex : %5d \n\n", 
            adjStillCtrl.StillMode,adjStillCtrl.ShIndex,adjStillCtrl.EvIndex,adjStillCtrl.NfIndex);

        _Decide_StillAdjTableNo(&StillAdjTableNo);//Get Adj Table No.
    
        /*
         * Start of App Adj test code, for testing bin file load from SD card
         */
        if(AmbaAdjStillLoadBinTest == 0){
                adjStillCtrl.LutNo = StillAdjTableNo;
        }else{
            adjStillCtrl.LutNo = 0;
        }
        /*
         * End of App Adj test code, for testing bin file load from SD card
         */ 
     
        AmbaAdjStillCtrlPre[ChNoChk] = adjStillCtrl;
    
    } else if (AmbaAdjStillCtrlPre[ChNoChk].ShIndex == 0) {
        AmbaAdjStillCtrlPre[ChNoChk].StillMode = IP_MODE_LISO_STILL;
        AmbaAdjStillCtrlPre[ChNoChk].ShIndex = 1012;
        AmbaAdjStillCtrlPre[ChNoChk].EvIndex = 0;
        AmbaAdjStillCtrlPre[ChNoChk].NfIndex = 0;
        AmbaAdjStillCtrlPre[ChNoChk].WbGain = StillWbGain;
        AmbaAdjStillCtrlPre[ChNoChk].DZoomStep = 0;
        AmbaAdjStillCtrlPre[ChNoChk].FlashMode = 0;
        AmbaAdjStillCtrlPre[ChNoChk].LutNo = 0;   
    }
        
    AmbaImg_Proc_Cmd(MW_IP_ADJ_STILL_CONTROL, chNo , (UINT32)&AmbaAdjStillCtrlPre[ChNoChk] , 0);
    AmbaImg_Proc_Cmd(MW_IP_SET_STILL_PIPE_CTRL_PARAMS, chNo , (UINT32)mode , 0);
}  

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: Amba_Adj_AppTest
 *
 *  @Description:: App test code for Adj function
 *
 *  @Input      ::
 *    int argc, char **argv
 *
 *  @Output     :: none
 *
 *  @Return     :: none
 *
\*-----------------------------------------------------------------------------------------------*/
int Amba_Adj_AppTest(int argc, char **argv)
{
    UINT32 Size = 0, Addr = 0;
    UINT16 i = 0;
    UINT8 Type = 0, Enable = 0, TableNo = 0;
    int RVal = 0;
    
    if (strcmp(argv[2],"loadbin") == 0) {
        Type  = (UINT8)atoi(argv[3]);
        Enable  = (UINT8)atoi(argv[4]);
      
        if(Type == 0){ //Video
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)0, IQ_PARAMS_VIDEO_ADJ, (UINT32)&Addr);
            pAmbaAdjVideoTmp = (ADJ_VIDEO_PARAM_s *)Addr;
            AmbaAdjColorTmp = pAmbaAdjVideoTmp->FilterParam.Def.Color;
        
            Size = sizeof(ADJ_VIDEO_PARAM_s);
            RVal = AmbaImg_Proc_Cmd(MW_IP_LOADBIN, (UINT32)&AmbaAdjVideoTmp, (UINT32)argv[5], Size);
            for(i = 0;i < 5;i++){
                AmbaAdjVideoTmp.FilterParam.Def.Color.Table[i].MatrixThreeDTableAddr = AmbaAdjColorTmp.Table[i].MatrixThreeDTableAddr;
            }
            AmbaAdjVideoLoadBinTest = Enable;
        }else if(Type == 1){ //Photo
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)0, IQ_PARAMS_PHOTO_ADJ, (UINT32)&Addr);
            pAmbaAdjPhotoTmp = (ADJ_PHOTO_PARAM_s *)Addr;
            AmbaAdjColorTmp = pAmbaAdjPhotoTmp->FilterParam.Def.Color;
        
            Size = sizeof(ADJ_PHOTO_PARAM_s);
            RVal = AmbaImg_Proc_Cmd(MW_IP_LOADBIN, (UINT32)&AmbaAdjPhotoTmp, (UINT32)argv[5], Size);
            for(i = 0;i < 5;i++){
                AmbaAdjPhotoTmp.FilterParam.Def.Color.Table[i].MatrixThreeDTableAddr = AmbaAdjColorTmp.Table[i].MatrixThreeDTableAddr;
            }
            AmbaAdjPhotoLoadBinTest = Enable;
        }else if(Type == 2){ //Still LISO
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)0, IQ_PARAMS_STILL_LISO_ADJ, (UINT32)&Addr);
            if(AmbaAdjStillLIsoAddFlg == 0){
                AmbaAdjStillLIsoAddOri = Addr;
                AmbaAdjStillLIsoAddFlg = 1;
            }
            pAmbaAdjStillLIsoTmp = (ADJ_STILL_FAST_LISO_PARAM_S *)Addr;
            AmbaAdjColorTmp = pAmbaAdjStillLIsoTmp->Def.Color;
        
            Size = sizeof(ADJ_STILL_FAST_LISO_PARAM_S);
            RVal = AmbaImg_Proc_Cmd(MW_IP_LOADBIN, (UINT32)&AmbaAdjStillLIsoTmp, (UINT32)argv[5], Size);
            for(i = 0;i < 5;i++){
                AmbaAdjStillLIsoTmp.Def.Color.Table[i].MatrixThreeDTableAddr = AmbaAdjColorTmp.Table[i].MatrixThreeDTableAddr;
            }
            AmbaAdjStillLoadBinTest = Enable;
            if(AmbaAdjStillLoadBinTest == 1){
                AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_PARAMS_ADD, 0, IQ_PARAMS_STILL_LISO_ADJ, (UINT32)&AmbaAdjStillLIsoTmp);
            }else{
                AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_PARAMS_ADD, 0, IQ_PARAMS_STILL_LISO_ADJ, (UINT32)AmbaAdjStillLIsoAddOri);        
            }
        }else if(Type == 3){ //Still HISO
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)0, IQ_PARAMS_STILL_HISO_ADJ, (UINT32)&Addr);
            if(AmbaAdjStillHIsoAddFlg == 0){
                AmbaAdjStillHIsoAddOri = Addr;
                AmbaAdjStillHIsoAddFlg = 1;
            }
            pAmbaAdjStillHIsoTmp = (ADJ_STILL_HISO_PARAM_s *)Addr;
            AmbaAdjColorTmp = pAmbaAdjStillHIsoTmp->Def.Color;
        
            Size = sizeof(ADJ_STILL_HISO_PARAM_s);
            RVal = AmbaImg_Proc_Cmd(MW_IP_LOADBIN, (UINT32)&AmbaAdjStillHIsoTmp, (UINT32)argv[5], Size);
            for(i = 0;i < 5;i++){
                AmbaAdjStillHIsoTmp.Def.Color.Table[i].MatrixThreeDTableAddr = AmbaAdjColorTmp.Table[i].MatrixThreeDTableAddr;
            }
            AmbaAdjStillLoadBinTest = Enable;
            if(AmbaAdjStillLoadBinTest == 1){
                AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_PARAMS_ADD, 0, IQ_PARAMS_STILL_HISO_ADJ, (UINT32)&AmbaAdjStillHIsoTmp);
            }else{
                AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_PARAMS_ADD, 0, IQ_PARAMS_STILL_HISO_ADJ, (UINT32)AmbaAdjStillHIsoAddOri);        
            }
        }else{ //Video
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)0, IQ_PARAMS_VIDEO_ADJ, (UINT32)&Addr);
            pAmbaAdjVideoTmp = (ADJ_VIDEO_PARAM_s *)Addr;
            AmbaAdjColorTmp = pAmbaAdjVideoTmp->FilterParam.Def.Color;
        
            Size = sizeof(ADJ_VIDEO_PARAM_s);
            RVal = AmbaImg_Proc_Cmd(MW_IP_LOADBIN, (UINT32)&AmbaAdjVideoTmp, (UINT32)argv[5], Size);
            for(i = 0;i < 5;i++){
                AmbaAdjVideoTmp.FilterParam.Def.Color.Table[i].MatrixThreeDTableAddr = AmbaAdjColorTmp.Table[i].MatrixThreeDTableAddr;
            }
            AmbaAdjVideoLoadBinTest = Enable;
        }
    } else if (strcmp(argv[2],"savebin") == 0) {
        Type  = (UINT8)atoi(argv[3]);
        TableNo = (UINT32)atoi(argv[4]);
    
        if(Type == 0){ //Video
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)0, IQ_PARAMS_VIDEO_ADJ, (UINT32)&Addr);
            Size = sizeof(ADJ_VIDEO_PARAM_s);
        }else if(Type == 1){ //Photo
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)0, IQ_PARAMS_PHOTO_ADJ, (UINT32)&Addr);
            Size = sizeof(ADJ_PHOTO_PARAM_s);
        }else if(Type == 2){ //Still LISO
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)0, IQ_PARAMS_STILL_LISO_ADJ, (UINT32)&Addr);
            Size = sizeof(ADJ_STILL_FAST_LISO_PARAM_S);
        }else if(Type == 3){ //Still HISO
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)0, IQ_PARAMS_STILL_HISO_ADJ, (UINT32)&Addr);
            Size = sizeof(ADJ_STILL_HISO_PARAM_s);
        }else{ //Video
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD,(UINT32)0, IQ_PARAMS_VIDEO_ADJ, (UINT32)&Addr);
            Size = sizeof(ADJ_VIDEO_PARAM_s);
        }
        Addr += Size*TableNo;
        RVal = AmbaImg_Proc_Cmd(MW_IP_SAVEBIN, Addr, (UINT32)argv[5], Size);
        AmbaPrint("--- size : %6d, type : %d, Addr : 0x0%x, RVal : %2d ----", Size, Type, Addr, RVal);     
    } else if (strcmp(argv[2],"EnableBinTest") == 0) {
        if (strcmp(argv[3],"Video") == 0) {
            AmbaAdjVideoLoadBinTest = (UINT8)atoi(argv[4]);
        }else{
            AmbaAdjPhotoLoadBinTest = (UINT8)atoi(argv[4]);
        }
    } else{
        AmbaPrint("Usage: t imgproc -adjapptest");
        AmbaPrint("                              savebin  [Video|Photo : 0|1] [TableNo] [path]");
        AmbaPrint("                              loadbin  [Video|Photo : 0|1] [EnableTest : 0|1] [path]");
    }
    return 0;
} 

void AmbaSample_Set_MADJ(UINT32 chNo, UINT8 enable, UINT32 madj_no)
{
	AmbaAdjMaunalEnable[chNo] = enable;
	AmbaAdjMaunalTableNo[chNo] = madj_no;
	
	return;
}
