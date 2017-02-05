/**
 * @file src/app/connected/applib/src/calibration/adjust/ApplibCalibAdjust.c
 *
 * sample code for vignette calibration
 *
 * History:
 *    07/10/2013  Allen Chiu Created
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "AmbaPrintk.h"
#include <AmbaUTCalibAdjust.h>

#define WB_BLEND            (1)
#define OZ_BLEND            (2)
#define VIG_CTL_VERSION     (0x14031400)
#define ABS(a)      (((a) < 0) ? -(a) : (a))

Vignette_Control_s *G_CtlVigDataAddr = 0;
VIG_LUMA_TABLE_s *VigLumaTableAddr[CALIB_CH_NO];
VIG_LUMA_TABLE_s *VigChromaTableAddr[CALIB_CH_NO];
WB_BLEND_CURVE_s *VigWbBlendTable[CALIB_CH_NO];
UINT8 VignetteControlEnable = 0;
UINT8 DebugFlag = 0;
UINT8 StrengthFlag = 0;
UINT8  VignetteTableChange = 0;
double InverseAverageNum;
CALIBRATION_ADJUST_PARAM_s CalibAdjustParam;

/**
 *  @brief calculate the interpolation by 1D curve
 *
 *  calculate the interpolation by 1D curve
 *
 *  @param [in] CurrValue the base value of the interpolation
 *  @param [in] Idx0 the index 0 for interpolation
 *  @param [in] Idx1 the index 1 for interpolation
 *  @param [in] Weight0 the weighting 0 for interpolation
 *  @param [in] Weight1 the weighting 0 for interpolation
 *
 *  @return >=0 success, <0 failure
 */
int AmpUTCalibAdjust_Vignette1dInterpolation(UINT CurrValue, int Idx0, int Idx1, UINT16 Weight0, UINT16 Weight1){

    int Result = 0;

    if(Idx1 == Idx0) {
        Result = (Weight0 + Weight1)>>1;
    }
    else {
        Result = (Weight0 * (Idx1 - CurrValue) + Weight1 * (CurrValue - Idx0)) / (Idx1 - Idx0);
    }
    return Result;
}

/**
 *  @brief calculate the interpolation of the strength
 *
 *  calculate the interpolation of the strength
 *
 *  @param [in] Ch current channel of the sensor
 *  @param [in] CurrLuma the current luma
 *  @param [in] VigStrengthTableAddr Address of the vignette strength table
 *  @param [in] Mode video/still mode
 *
 *  @return strength
 */
int AmpUTCalibAdjust_VignetteStrengthInterpolation(UINT8 Ch,UINT CurrLuma, VIG_LUMA_TABLE_s *VigStrengthTableAddr, UINT8 Mode){
	int Result = 0;
	int LumaTab[LV_TABLE_NUM] = {  0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700,1800, 1900, 2000 };
	UINT8 Count;

	if(CurrLuma >= LumaTab[LV_TABLE_NUM - 1]){
		CurrLuma = LumaTab[LV_TABLE_NUM - 1];
	}
	Count = (UINT8)((CurrLuma) ? ((CurrLuma - 1) * 0.01 + 1) : 1);


    if(Mode == AMBA_DSP_IMG_PIPE_VIDEO){
        Result = AmpUTCalibAdjust_Vignette1dInterpolation(CurrLuma, LumaTab[Count - 1],LumaTab[Count],VigStrengthTableAddr->VideoWeightTable[Count-1],VigStrengthTableAddr->VideoWeightTable[Count]);
    }else if(Mode == AMBA_DSP_IMG_PIPE_STILL){
        Result = AmpUTCalibAdjust_Vignette1dInterpolation(CurrLuma, LumaTab[Count - 1],LumaTab[Count],VigStrengthTableAddr->StillWeightTable[Count-1],VigStrengthTableAddr->StillWeightTable[Count]);
    }
	Result = Result << 8; // mapping the luma table range (0~256) and strength range (0~65536)

    return Result;
}

/**
 *  @brief calculate the interpolation of the vignette table by ozoom step or wb index
 *
 *  calculate the interpolation of the vignette table by ozoom step or wb index
 *
 *  @param [in] Ch current channel of the sensor
 *  @param [in] BlendType ozoom or WB 
 *  @param [in] CurrValue the current index
 *
 *  @return strength
 */
int AmpUTCalibAdjust_VignetteBlendInterpolation(UINT8 Ch, UINT8 BlendType, INT32 CurrValue){

	int Rval = 0;
    /*TBD*/
	return Rval;
	
}

/**
 *  @brief enable/disable the vignette controller
 *
 *  enable/disable the vignette controller
 *
 *  @param [in] Enable the flag of the vignette controller
 *
 *  @return Null
 */
void AmpUTCalibAdjust_SetControlEnable(UINT8 Enable){
	VignetteControlEnable = Enable;
}

/**
 *  @brief get the enable flag for the vignette controller
 *
 *  get the enable flag for the vignette controller
 *
 *
 *  @return VignetteControlEnable
 */
UINT8 AmpUTCalibAdjust_GetControlEnable(void){
	return VignetteControlEnable;
}

/**
 *  @brief calculate the strength from the vignette table by luma. 
 *
 *  calculate the strength from the vignette table by luma. 
 *
 *  @param [in] Ch current channel of the sensor
 *
 *  @return OK
 */
int AmpUTCalibAdjust_VideoVignetteStrength(UINT8 Ch){

	UINT16 CurrLuma[CALIB_CH_NO] = {0};    
    static UINT16 PreLuma[CALIB_CH_NO] = {0xFFFF};
    int CurrStr[CALIB_CH_NO] = {0};    
    int PreStr[CALIB_CH_NO] = {0};
    int LumaDiff = 0;
    int StrengthDiff = 0;
    Vignette_Select_Param_s VigSelectParameter;
   
	/*A. Get the current strength value*/
    AmbaImg_Proc_Cmd(MW_IP_GET_CURR_LV, (UINT32)Ch,(UINT32)&CurrLuma[Ch], 0);    // LV 100 200 300 ...
    LumaDiff = ABS((int)PreLuma[Ch] - (int)CurrLuma[Ch]) ;
    if(DebugFlag){
        AmbaPrint("[VIG CTL]Video PreLuma %d , CurrLuma %d",PreLuma[Ch],CurrLuma[Ch]);
        AmpUTCalibVignette_GetParameter(&VigSelectParameter, Ch, AMBA_DSP_IMG_PIPE_VIDEO);
        PreStr[Ch] = VigSelectParameter.VignetteLumaStrength;
        AmbaPrint("[VIG CTL]Video The Old Vig luma str: %d",PreStr[Ch]);
    }
  
    /*B. Interpolation the luma table*/

    //calculate the strength for luma strength
    AmpUTCalibVignette_GetParameter(&VigSelectParameter, Ch, AMBA_DSP_IMG_PIPE_VIDEO);
    CurrStr[Ch] = AmpUTCalibAdjust_VignetteStrengthInterpolation(Ch, CurrLuma[Ch], VigLumaTableAddr[Ch],AMBA_DSP_IMG_PIPE_VIDEO);

/*C. compare the current strength ratio and the pre-strength ration*/      
    PreStr[Ch] = VigSelectParameter.VignetteLumaStrength;
    StrengthDiff = ABS((int)PreStr[Ch] - (int)CurrStr[Ch]) ;
    if(DebugFlag){
        AmbaPrint("[VIG CTL] The Vig Chroma strength after interpolation : %d PreStr: %d",CurrStr[Ch],PreStr[Ch]);
    }    
    if (StrengthDiff >= (CalibAdjustParam.LumaThreshold<<8)) {
        CurrStr[Ch] = (int)((CurrStr[Ch] + (PreStr[Ch]*(CalibAdjustParam.AverageNum-1)))*InverseAverageNum); // alpha filter (new*a + old*(1-a))
        if(DebugFlag){
            AmbaPrint("[VIG CTL] The new Luma Str: %d",CurrStr[Ch]);
        }
        VignetteTableChange = 1;
        VigSelectParameter.VignetteLumaStrength = CurrStr[Ch];
    }
    //calculate the strength for chroma strength
    CurrStr[Ch] = AmpUTCalibAdjust_VignetteStrengthInterpolation(Ch, CurrLuma[Ch], VigChromaTableAddr[Ch],AMBA_DSP_IMG_PIPE_VIDEO);
    PreStr[Ch] = VigSelectParameter.VignetteChromaStrength;
    StrengthDiff = ABS((int)PreStr[Ch] - (int)CurrStr[Ch]) ;
    if(DebugFlag){
        AmbaPrint("[VIG CTL] The Vig Chroma strength after interpolation : %d PreStr: %d",CurrStr[Ch],PreStr[Ch]);
    }        
    if (StrengthDiff >= (CalibAdjustParam.ChromaThreshold<<8)) {
        CurrStr[Ch] = (int)((CurrStr[Ch] + (PreStr[Ch]*(CalibAdjustParam.AverageNum-1)))*InverseAverageNum); // alpha filter (new*a + old*(1-a))
        if(DebugFlag){
            AmbaPrint("[VIG CTL] The new chromatic Str: %d",CurrStr[Ch]);
        }
        VignetteTableChange = 1;
        VigSelectParameter.VignetteChromaStrength = CurrStr[Ch];
    }
    PreLuma[Ch] = CurrLuma[Ch];

	/*D. Set the strength to select parameter*/
    if (VignetteTableChange) {
        AmpUTCalibVignette_SetParameter(&VigSelectParameter, Ch, AMBA_DSP_IMG_PIPE_VIDEO);
    }
    return 0;
}

/**
 *  @brief (still mode)calculate the strength from the vignette table by luma
 *
 *  (still mode)calculate the strength from the vignette table by luma
 *
 *  @param [in] Ch current channel of the sensor
 *
 *  @return OK
 */
int AmpUTCalibAdjust_StillVignetteStrength(UINT8 Ch){

    UINT16 CurrStrStill = 0;  
    UINT16 CurrLuma[CALIB_CH_NO] = {0};    
    static UINT16 PreLuma[CALIB_CH_NO] = {0xFFFF};
    Vignette_Select_Param_s VigSelectParameter;

    if(AmpUTCalibAdjust_GetControlEnable()&&(CalibAdjustParam.VigBlendOzoomEnable|CalibAdjustParam.VigBlendWBEnable|StrengthFlag) != 1) {
        return 0;
    }
	/*A. Get the current strength value*/
    AmbaImg_Proc_Cmd(MW_IP_GET_CURR_LV, (UINT32)Ch,(UINT32)&CurrLuma[Ch], 0);    // LV 100 200 300 ...
    if(DebugFlag){
        AmbaPrint("[VIG CTL]Current Luma Still = %d, Old Luma Still = %d",CurrLuma[Ch],PreLuma[Ch]);
    }
    if(CurrLuma[Ch] == PreLuma[Ch]) {
        return 0;
    }
    PreLuma[Ch] = CurrLuma[Ch];
    /*B. Interpolation the luma table*/       
    AmpUTCalibVignette_GetParameter(&VigSelectParameter, Ch, AMBA_DSP_IMG_PIPE_STILL);
    CurrStrStill = AmpUTCalibAdjust_VignetteStrengthInterpolation(Ch, CurrLuma[Ch], VigLumaTableAddr[Ch], AMBA_DSP_IMG_PIPE_STILL);
    VigSelectParameter.VignetteLumaStrength = CurrStrStill;
    if(DebugFlag){
        AmbaPrint("[VIG CTL] The luma strength after interpolation : %d (still mode)",CurrStrStill);
    }

    CurrStrStill = AmpUTCalibAdjust_VignetteStrengthInterpolation(Ch, CurrLuma[Ch], VigChromaTableAddr[Ch], AMBA_DSP_IMG_PIPE_STILL);
    VigSelectParameter.VignetteChromaStrength = CurrStrStill;
	/*C. Set the strength to select parameter*/
    AmpUTCalibVignette_SetParameter(&VigSelectParameter, Ch, AMBA_DSP_IMG_PIPE_STILL);
    VignetteTableChange = 1;
    return 0;
}

/**
 *  @brief  blend the vignette calibration table by ozoom step or WB index
 *
 *   blend the vignette calibration table by ozoom step or WB index
 *
 *  @param [in] Ch current channel of the sensor
 *  @param [in] BlendType : ozoom or WB 
 *
 *  @return OK
 */
int AmpUTCalibAdjust_VignetteBlend(UINT8 Ch,UINT8 BlendType){
    /*TBD*/
    return 0;
}


/**
 *  @brief the vignette controller main function
 *
 *   the vignette controller main function
 *
 *
 *  @return OK
 */
int AmpUTCalibAdjust_VignetteMain(void){
	UINT8 Channel = 0;

	/*A. get vignette blend ratio*/
	//AmpUTCalibAdjust_VignetteBlend(Ch,BlendType);

	/*B. get vignette luma strength ratio*/
    AmpUTCalibAdjust_VideoVignetteStrength(Channel);
    //AmpUTCalibAdjust_StillVignetteStrength(Channel);
		
    if(VignetteTableChange == 1){
	/*C. set vignette table*/
        if(DebugFlag){
            AmbaPrint("[VIG CTL] Vignette table update. ");
        }
        
        AmpUTCalibVignette_SelectVignetteTable(VIG_UPDATE_VIN_INFO, 0, 0, 0, 0, 0, 0);
        VignetteTableChange = 0;
    }
    return 0;
}

/**
 *  @brief enable/disable the vignette controller debug message
 *
 *  enable/disable the vignette controller debug message
 *
 *  @param [in] Enable the flag of the debug message
 *
 *  @return Null
 */
void AmpUTCalibAdjust_Debug(UINT8 Enable){
	DebugFlag = Enable;
}


/**
 *  @brief run the vignette controller main function
 *
 *  run the vignette controller main function
 *
 *  @return Null
 */
void AmpUTCalibAdjust_Func(void){
    static UINT8 VigCtlCount = 0;
    static int Frame = 0;
    if(VignetteControlEnable == 1){
        if(CalibAdjustParam.VigBlendOzoomEnable|CalibAdjustParam.VigBlendWBEnable|StrengthFlag){
            if(Frame == CalibAdjustParam.CheckFrameNum) {
        		if(VigCtlCount == 0){
        			AmpUTCalibAdjust_VignetteMain();
        			VigCtlCount = 0;
        		}else{
        			VigCtlCount++;
        		}
                Frame = 0;
            }
            Frame++;
    	}
    }
}


/**
 *  @brief initialize the vignette blend controller 
 *
 *  initialize the vignette blend controller 
 *
 *  @param [in] VignetteDataAddr vignette data address
 *  @param [in] BlendType ozoom or WB 
 *
 *  @return 0
 */
int AmpUTCalibAdjust_VignetteBlendInit(Vignette_Control_s *VignetteDataAddr, UINT8 BlendType){
    /*TBD*/
    return 0;
}

/**
 *  @brief Set WB blend table from the vignette calibration parameter
 *
 *  Set WB blend table from the vignette calibration parameter
 *
 *  @param [in] CalibParameter the vignette calibration parameter
 *
 *  @return 0
 */
void AmpUTCalibAdjust_SetWBBlendTable(CALIBRATION_ADJUST_PARAM_s *CalibParameter)
{
    int Ch= 0;
    for(Ch = 0; Ch < CALIB_CH_NO; Ch++){
        VigWbBlendTable[Ch] = &(CalibParameter->VigWbBlendTable[Ch]);
    } 
}

/**
 *  @brief Set luma table from the vignette calibration parameter
 *
 *  Set luma table from the vignette calibration parameter
 *
 *  @param [in] CalibParameter the vignette calibration parameter
 *
 *  @return Null
 */
void AmpUTCalibAdjust_SetLumaTable(CALIBRATION_ADJUST_PARAM_s *CalibParameter)
{
    int Ch= 0;
    for(Ch = 0; Ch < CALIB_CH_NO; Ch++){
        VigLumaTableAddr[Ch] = &(CalibParameter->VigLumaTable[Ch]);
    } 
}


/**
 *  @brief Set chroma table from the vignette calibration parameter
 *
 *  Set chroma table from the vignette calibration parameter
 *
 *  @param [in] CalibParameter the vignette calibration parameter
 *
 *  @return Null
 */
void AmpUTCalibAdjust_SetChromaTable(CALIBRATION_ADJUST_PARAM_s *CalibParameter)
{
    int Ch= 0;
    for(Ch = 0; Ch < CALIB_CH_NO; Ch++){
        VigChromaTableAddr[Ch] = &(CalibParameter->VigChromaTable[Ch]);
    } 
}

/**
 *  @brief initialize the vignette calibration controller
 *
 *  initialize the vignette calibration controller
 *
 *
 *  @return OK:0/NG:-1
 */
int AmpUTCalibAdjust_Init(void)
{
    INT32 Rval = 0;
    CALIBRATION_ADJUST_PARAM_s *CalibParameter = 0;
    Vignette_Control_s *VignetteDataAddr = 0;
    UINT8 BlendType = 0;
    if(DebugFlag){    
        AmbaPrint("[VIG CTL] Start: AmpUTCalibAdjust_Init");
    }
    /*A. Get the calibration parameter from the table (set by sensor)*/
    CalibParameter = AmpUTCalibAdjustGetCalibrationParam();
    CalibAdjustParam.VigBlendOzoomEnable = CalibParameter->VigBlendOzoomEnable;
    CalibAdjustParam.VigBlendWBEnable = CalibParameter->VigBlendWBEnable;
    StrengthFlag = CalibParameter->VigLumaStrCtlEnable|CalibParameter->VigChromaStrCtlEnable;
    CalibAdjustParam.CheckFrameNum = CalibParameter->CheckFrameNum;
    CalibAdjustParam.AverageNum = CalibParameter->AverageNum;
    CalibAdjustParam.LumaThreshold = CalibParameter->LumaThreshold;
    CalibAdjustParam.ChromaThreshold = CalibParameter->ChromaThreshold;
    if(CalibAdjustParam.AverageNum!=0){
        InverseAverageNum = 1.0/(double)CalibAdjustParam.AverageNum;
    }else {
        AmbaPrint("Incorrect parameter AverageNum %d, it should not zero",CalibAdjustParam.AverageNum);
        return -1;
    }
        
    if(DebugFlag){
        AmbaPrint("FWVersionNumber = %x",CalibParameter->FWVersionNumber);
        AmbaPrint("CalibAdjustParam.VigBlendOzoomEnable = %d",CalibAdjustParam.VigBlendOzoomEnable);
        AmbaPrint("CalibAdjustParam.VigBlendWBEnable = %d",CalibAdjustParam.VigBlendWBEnable);
        AmbaPrint("StrengthFlag = %d",StrengthFlag);
        AmbaPrint("CheckFrameNum = %d",CalibAdjustParam.CheckFrameNum);
        AmbaPrint("AverageNum = %d",CalibAdjustParam.AverageNum);
    }
    /*B. Get the vignette calibration data */   
    if (AmpUTCalibVignette_GetVignetteCalDataEnable() != 1) {
        // Vignette calibration data is not correct. Don't run this task
        AmbaPrintColor(BLUE,"[VIG CTL] no vignette data");
        CalibAdjustParam.VigBlendOzoomEnable = CalibParameter->VigBlendOzoomEnable = 0;
        CalibAdjustParam.VigBlendWBEnable = CalibParameter->VigBlendWBEnable = 0;
        StrengthFlag = CalibParameter->VigLumaStrCtlEnable = CalibParameter->VigChromaStrCtlEnable = 0;
        VignetteControlEnable = 0;
        Rval = -1;
    }else{
        VignetteDataAddr = (Vignette_Control_s *)AmpUTCalibVignette_GetVignetteCalData();
        G_CtlVigDataAddr = VignetteDataAddr;
        if(DebugFlag){
            AmbaPrint("[VIG CTL] GetVigCalData success!");
        }
    }
    /*C. check the version with CALIBRATION_ADJUST_PARAM_s*/
    if(CalibParameter->FWVersionNumber != VIG_CTL_VERSION){
        // Vignette calibration Version Number is not correct. Don't run this task
        AmbaPrintColor(BLUE,"[VIG CTL] VersionNumber mismatch. Stop the vignette controller." );
        CalibAdjustParam.VigBlendOzoomEnable = CalibParameter->VigBlendOzoomEnable = 0;
        CalibAdjustParam.VigBlendWBEnable = CalibParameter->VigBlendWBEnable = 0;
        StrengthFlag = CalibParameter->VigLumaStrCtlEnable = CalibParameter->VigChromaStrCtlEnable = 0;
        VignetteControlEnable = 0;
        Rval = -1;
    }
    
    /*D. Set the WB blend/luma strength table */
    AmpUTCalibAdjust_SetLumaTable(CalibParameter);
    AmpUTCalibAdjust_SetChromaTable(CalibParameter);
    AmpUTCalibAdjust_SetWBBlendTable(CalibParameter);
    
    /*C. Init the blend/strength function */
    //check the blend flag: there are only one bland flag can be enable.
    if((CalibParameter->VigBlendWBEnable & CalibParameter->VigBlendOzoomEnable) == 1){
        AmbaPrintColor(RED,"[VIG CTL] Error! Can not set the enable flag (VigBlendOzoomEnable/VigBlendOzoomEnable)in the same time");
        CalibAdjustParam.VigBlendOzoomEnable = CalibParameter->VigBlendOzoomEnable = 0;
        CalibAdjustParam.VigBlendWBEnable = CalibParameter->VigBlendWBEnable = 0;
        Rval = -1;
    }else if(CalibParameter->VigBlendOzoomEnable || CalibParameter->VigBlendWBEnable){
        if(CalibParameter->VigBlendOzoomEnable == 1){
            BlendType = OZ_BLEND; 
            if(DebugFlag){
                AmbaPrint("[VIG CTL] Set the BlendType : OZ_BLEND");
            }
        }else{
            BlendType =WB_BLEND;
            if(DebugFlag){
                AmbaPrint("[VIG CTL] Set the BlendType : WB_BLEND");
            }
        }
 
        if ( AmpUTCalibAdjust_VignetteBlendInit(VignetteDataAddr,BlendType)== -1 ){
            AmbaPrintColor(RED, "[VIG CTL] Blend controller init fail, turn off blend function!");
            CalibAdjustParam.VigBlendOzoomEnable = CalibParameter->VigBlendOzoomEnable = 0;
            CalibAdjustParam.VigBlendWBEnable = CalibParameter->VigBlendWBEnable = 0;
            Rval = -1;
        }else{
            if(DebugFlag){
                AmbaPrint("[VIG CTL] Blend controller init success!");
            }
        }
    }
 
    if(DebugFlag){
        AmbaPrint("[VIG CTL]End : AmpUTCalibAdjust_Init");
    }    
    return Rval;
}
