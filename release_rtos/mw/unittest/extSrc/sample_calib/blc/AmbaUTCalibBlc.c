/**
 * @file mw/unittest/extSrc/sample_calib/blc/AmbaUTCalibBlc.c
 *
 * sample code for black level calibration
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
#include <math.h>
#include "AmbaUTCalibBlc.h"
#include "AmbaUtility.h"

extern AMBA_KAL_BYTE_POOL_t G_MMPL;
extern AMBA_KAL_BYTE_POOL_t G_MMPL;
#define ABS(a)      (((a) < 0) ? -(a) : (a))

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBLC_Init
 *
 *  @Description:: initial function for black level calibration
 *
 *  @Input      ::
 *          CalObj: calibration object
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBLC_Init(Cal_Obj_s *CalObj)
{
    Cal_Blc_Info_s CalBLCInfo[CALIB_CH_NO];
    char OutputStr[128];
    UINT8 Channel;

    memcpy(&CalBLCInfo[0], CalObj->DramShadow, (sizeof(Cal_Blc_Info_s)*CALIB_CH_NO));
    for (Channel = 0; Channel < CALIB_CH_NO; Channel ++) {
        if (CalBLCInfo[Channel].BlCalResult == BL_CAL_OK) {
            CalBLCInfo[Channel].BlMode = BLC_MODE_APPLY; // apply
            AmpCalib_BLCFunc(&CalBLCInfo[Channel],0, OutputStr);
    } else {
            CalBLCInfo[Channel].BlMode = BLC_MODE_RESET; // reset
            AmpCalib_BLCFunc(&CalBLCInfo[Channel],0,OutputStr);
        }
    }
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBLC_PrintError
 *
 *  @Description:: print the error message for black level calibration
 *
 *  @Input      ::
 *          CalSite: calibration site status
 *
 *  @Output     ::
 *          OutputStr: OutputStr: debug message for this function
 *  @Return     ::
 *          INT: OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBLC_PrintError(char *OutputStr, Cal_Stie_Status_s *CalSite)
{
    AmbaPrint("************************************************************");
    AmbaPrintColor(RED,"%s",OutputStr);
    AmbaPrint("************************************************************");
    CalSite->Status = CAL_SITE_RESET;

    return 0;
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBLC_Func
 *
 *  @Description:: the entry function for black level calibration
 *
 *  @Input      ::
 *          Argc: number of input parameters
 *          Argv: value of input parameters
 *          CalSite: calibration site status
 *          CalObj: calibration object
 *
 *  @Output     ::
 *          OutputStr: OutputStr: debug message for this function
 *  @Return     ::
 *          INT: OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBLC_Func(int Argc, char *Argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, Cal_Obj_s *CalObj)
{
    Cal_Blc_Info_s CalBLCInfo[CALIB_CH_NO];
    UINT8 DbValue;
    AMBA_SENSOR_MODE_ID_u SensorMode = { .Bits = {   .Mode = 0,  .VerticalFlip = 0,  } };

    UINT8 ParamChk = 0;
    INT16 BLCThreshold[CALIB_CH_NO];
    AMBA_DSP_IMG_BLACK_CORRECTION_s BLCVar = { 0 };

    INT16 BLCVarTmp = 0;
    int Rval;
    int Channel;

    void *TempPtr,*TempPtrRel;
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s Geometry= {0};
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 1 } };
    AMBA_SENSOR_MODE_INFO_s VinInfo;
    UINT8 *RawBufferAddress = 0;
    UINT8 Flag = 0x1;
    UINT32 RawPitch;
    AMP_STILLENC_ITUNER_RAWCAPTURE_CTRL_s ItunerRawCapCtrl = {
        .RawBufSource = 1,
        .ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING,
        .SensorMode = {
            .Bits = {
                .Mode = 0,//AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0, //please config your desired Mode
                .VerticalFlip = 0,
            }
        },
    };
    AMBA_AE_INFO_s StillAEInfo[MAX_AEB_NUM], CurStillAEInfo[MAX_AEB_NUM];
    AMBA_AE_INFO_s  VideoAEInfo = {0},  CurVideoAEInfo = {0};
    AMBA_3A_OP_INFO_s NoAAAFunc = {DISABLE, DISABLE, DISABLE, DISABLE}, CurAAAFunc = {0};
    AMBA_SENSOR_STATUS_INFO_s SensorStatus;
    UINT8 ChNo = 0;

    memcpy(&CalBLCInfo[0], CalObj->DramShadow, (sizeof(Cal_Blc_Info_s)*CALIB_CH_NO));
    
    Channel =(int) atoi(Argv[1]);
    memset(&CalBLCInfo[Channel], 0, sizeof(Cal_Blc_Info_s));
    BLCThreshold[Channel] = 0;
    if ((Channel < 0) || (Channel >= CALIB_CH_NO)) {
        ParamChk = 0;
        AmbaPrint("Channel number out of range, Channel = %d and it should be between 0 ~ %d",Channel, (CALIB_CH_NO-1));
        sprintf(OutputStr,"Channel number out of range, Channel = %d and it should be between 0 ~ %d",Channel, (CALIB_CH_NO-1));
    }
    //bug, need to change VIN here. add it in the future........

    if (Argc >= 3) {
        CalBLCInfo[Channel].BlMode = (UINT8)(atoi(Argv[2]));
        if (CalBLCInfo[Channel].BlMode == BLC_MODE_TEST) {
            ParamChk = 1;
        } else {
            CalBLCInfo[Channel].BlMode = BLC_MODE_CALIBRATION;
            if (Argc >= 8) {
                ParamChk = 1;
                CalBLCInfo[Channel].BlStd.BlackR = (INT16)(atoi(Argv[3]));
                CalBLCInfo[Channel].BlStd.BlackGr = (INT16)(atoi(Argv[4]));
                CalBLCInfo[Channel].BlStd.BlackGb = (INT16)(atoi(Argv[5]));
                CalBLCInfo[Channel].BlStd.BlackB = (INT16)(atoi(Argv[6]));
                BLCThreshold[Channel] = (INT16)(atoi(Argv[7]));
            } else {
                ParamChk = 0;
                AmbaPrint("Script should be: BLC channel mode BlackR BlackGr BlackGb BlackGb Threshold");
                sprintf(OutputStr,"BLC Calibration fail: BLC Parameter Wrong!");
            }
        }
    } else {
        ParamChk = 0;
    }
    if (ParamChk == 0) {
        sprintf(OutputStr,"BLC Calibration fail: BLC Parameter Wrong!");
        AmpUTCalibBLC_PrintError(OutputStr,CalSite);
        return BL_CALIB_ERROR_PARAMETER;
    } else {
        AmbaPrint("*****BLC Calibration Info*****");
        AmbaPrint("Channel No = %d",Channel);
        AmbaPrint("BLC mode = %d",CalBLCInfo[Channel].BlMode);
        AmbaPrint("Reference black R = %d",CalBLCInfo[Channel].BlStd.BlackR);
        AmbaPrint("Reference black GR = %d",CalBLCInfo[Channel].BlStd.BlackGr);
        AmbaPrint("Reference black GB = %d",CalBLCInfo[Channel].BlStd.BlackGb);
        AmbaPrint("Reference black B = %d",CalBLCInfo[Channel].BlStd.BlackB);
        AmbaPrint("Threshold = %d",BLCThreshold[Channel]);
        AmbaPrint("******************************");        
    }


    /* allocate raw buffer address */
    SensorMode.Bits.Mode = pAmbaCalibInfoObj[Channel]->AmbaVignetteMode;
    SensorMode.Bits.VerticalFlip = 0;
    AmbaSensor_GetModeInfo(Chan, SensorMode, &VinInfo);
    CalBLCInfo[Channel].Bayer = VinInfo.OutputInfo.CfaPattern;

    Geometry.Width = VinInfo.OutputInfo.RecordingPixels.Width;
    Geometry.Height = VinInfo.OutputInfo.RecordingPixels.Height;
    RawPitch = ALIGN_32(Geometry.Width * 2);

    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempPtrRel, RawPitch*Geometry.Height, 32);
    if (Rval != OK) {
        AmbaPrint("allocate fail in %s:%u", __FUNCTION__, __LINE__);
    } else {
        RawBufferAddress = (UINT8*)((UINT32)TempPtr) ;
        AmbaPrint("[AmpUTCalibVig]RawBufferAddress s(0x%08X) (%u)!", RawBufferAddress, RawPitch*Geometry.Height);
    }

//backup current info
    AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, (UINT32)ChNo, (UINT32)&CurAAAFunc, 0);
    AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, (UINT32)ChNo, IP_MODE_STILL,  (UINT32)CurStillAEInfo);
    AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, (UINT32)ChNo, IP_MODE_VIDEO,  (UINT32)&CurVideoAEInfo);

//Stillmode
    for (DbValue = 0; DbValue < 7 ; ++DbValue) {
        AmbaKAL_TaskSleep(100);
		/* Lock 3a */
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, (UINT32)ChNo, (UINT32)&NoAAAFunc, 0);

        AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, (UINT32)ChNo, IP_MODE_STILL,  (UINT32)StillAEInfo);
        for (int i=0; i<MAX_AEB_NUM; ++i) {
            StillAEInfo[i].ShutterTime = (float)1/30;
            StillAEInfo[i].AgcGain      = (float)pow(2.0, (double)DbValue);
            StillAEInfo[i].Dgain         = WB_UNIT_GAIN;
            StillAEInfo[i].Flash          = 0;
            StillAEInfo[i].IrisIndex    = 0;
        }
        AmbaImg_Proc_Cmd(MW_IP_SET_AE_INFO, (UINT32)ChNo, IP_MODE_STILL,  (UINT32)StillAEInfo);
        AmbaPrintColor(RED, "BLC: ShutterTime: %f AgcGain: %f Dgain: %d", StillAEInfo[0].ShutterTime, StillAEInfo[0].AgcGain, StillAEInfo[0].Dgain);
        //AmpUT_CalibSetExposureValue(AeInfo.ShutterTime, AeInfo.AgcGain, AeInfo.Dgain);
        AmbaKAL_TaskSleep(100);

        CalBLCInfo[Channel].SrcRawWidth = VinInfo.OutputInfo.RecordingPixels.Width;
        CalBLCInfo[Channel].SrcRawHeight = VinInfo.OutputInfo.RecordingPixels.Height;
        //raw capture
        AmbaPrint("start to do raw capture");
        RawPitch = ALIGN_32(VinInfo.OutputInfo.RecordingPixels.Width * 2);
        ItunerRawCapCtrl.SensorMode.Bits.Mode = VinInfo.Mode.Bits.Mode;
        ItunerRawCapCtrl.SensorMode.Bits.VerticalFlip = VinInfo.Mode.Bits.VerticalFlip;
        ItunerRawCapCtrl.RawBuff.Raw.RawAddr = (UINT8 *)RawBufferAddress;
        ItunerRawCapCtrl.RawBuff.Raw.RawPitch = RawPitch; // Basic DMA transfer unit is 32-byte, which is 16-pixels, so pad to 16pixels boundary here;
        ItunerRawCapCtrl.RawBuff.Raw.RawWidth = VinInfo.OutputInfo.RecordingPixels.Width;
        ItunerRawCapCtrl.RawBuff.Raw.RawHeight = VinInfo.OutputInfo.RecordingPixels.Height;
        if (AmpUT_ItunerRawCapture) {
            Rval = AmpUT_ItunerRawCapture(Flag, ItunerRawCapCtrl);
        } else {
            AmbaPrint("Null AmpUT_ItunerRawCapture");
            K_ASSERT(0);
        }
        AmbaPrint("end of raw capture");
        AmbaKAL_TaskSleep(500);
        if (Rval != OK) {
            sprintf(OutputStr,"BLC Calibration fail: Raw capture error!");
            AmpUTCalibBLC_PrintError(OutputStr,CalSite);
            return Rval;
        }

        CalBLCInfo[Channel].SrcRawAddr = (UINT32)RawBufferAddress;
        Rval = AmpCalib_BLCFunc(&CalBLCInfo[Channel],&CalBLCInfo[Channel].BlStill[DbValue],OutputStr);
        {
            char Fn[32];
            char Mode[3] = {'w','b','\0'};
            AMBA_FS_FILE *Fp = NULL;
            
            sprintf(Fn, "c:\\blccal_still_%d.raw", DbValue);
            Fn[0] = AmpUT_CalibGetDriverLetter();
            AmbaPrint("AmpUTCalibBLC_SaveRawImage");
            //raw ready, dump it

            Fp = AmbaFS_fopen(Fn,(char const *) Mode);
            AmbaPrint("[AmpVig_UT]Dump Raw 0x%X width: %d height: %d  ", \
                      CalBLCInfo[Channel].SrcRawAddr, \
                      VinInfo.OutputInfo.RecordingPixels.Width, \
                      VinInfo.OutputInfo.RecordingPixels.Height);
            AmbaFS_fwrite(RawBufferAddress, RawPitch*VinInfo.OutputInfo.RecordingPixels.Height, 1, Fp);
            AmbaFS_fclose(Fp);
        }
        if (Rval < BL_CALIB_OK) {
            AmpUTCalibBLC_PrintError(OutputStr,CalSite);
            return Rval;
        }
    }

//Videomode
    memset(&SensorStatus, 0, sizeof(AMBA_SENSOR_STATUS_INFO_s));
    AmbaSensor_GetStatus(Chan, &SensorStatus);
    SensorMode.Bits.Mode= SensorStatus.ModeInfo.Mode.Bits.Mode;
    AmbaSensor_GetModeInfo(Chan, SensorMode, &VinInfo);

    for (DbValue = 0; DbValue < 7 ; ++DbValue) {

        AmbaKAL_TaskSleep(500);
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, (UINT32)ChNo, (UINT32)&NoAAAFunc, 0);

        AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, (UINT32)ChNo, IP_MODE_VIDEO,  (UINT32)&VideoAEInfo);
        VideoAEInfo.ShutterTime = (float)1/30;
        VideoAEInfo.AgcGain      = (float)pow(2.0, (double)DbValue);
        VideoAEInfo.Dgain         = WB_UNIT_GAIN;
        VideoAEInfo.Flash          = 0;
        VideoAEInfo.IrisIndex    = 0;
        AmbaImg_Proc_Cmd(MW_IP_SET_AE_INFO, (UINT32)ChNo, IP_MODE_VIDEO,  (UINT32)&VideoAEInfo);


        AmbaPrintColor(RED, "BLC: ShutterTime: %f AgcGain: %f Dgain: %d", VideoAEInfo.ShutterTime, VideoAEInfo.AgcGain, VideoAEInfo.Dgain);
        AmbaKAL_TaskSleep(500);
        //AmbaImg_Proc_Cmd(MW_IP_SET_AE_INFO, (UINT32)ChNo, IP_MODE_VIDEO, (UINT32)&AeInfo);
        //AmbaImg_Proc_Cmd(MW_IP_SET_AE_INFO, (UINT32)ChNo, IP_MODE_STILL, (UINT32)&AeInfo);
        //AmpUT_CalibSetExposureValue(AeInfo.ShutterTime, AeInfo.AgcGain, AeInfo.Dgain);

        CalBLCInfo[Channel].SrcRawWidth = VinInfo.OutputInfo.RecordingPixels.Width;
        CalBLCInfo[Channel].SrcRawHeight = VinInfo.OutputInfo.RecordingPixels.Height;
        //raw capture
        AmbaPrint("start to do raw capture");
        RawPitch = ALIGN_32(VinInfo.OutputInfo.RecordingPixels.Width * 2);
        ItunerRawCapCtrl.SensorMode.Bits.Mode = VinInfo.Mode.Bits.Mode;
        ItunerRawCapCtrl.SensorMode.Bits.VerticalFlip = VinInfo.Mode.Bits.VerticalFlip;
        ItunerRawCapCtrl.RawBuff.Raw.RawAddr = (UINT8 *)RawBufferAddress;
        ItunerRawCapCtrl.RawBuff.Raw.RawPitch = RawPitch;// Basic DMA transfer unit is 32-byte, which is 16-pixels, so pad to 16pixels boundary here;
        ItunerRawCapCtrl.RawBuff.Raw.RawWidth = VinInfo.OutputInfo.RecordingPixels.Width;
        ItunerRawCapCtrl.RawBuff.Raw.RawHeight = VinInfo.OutputInfo.RecordingPixels.Height;
        if (AmpUT_ItunerRawCapture) {
            Rval = AmpUT_ItunerRawCapture(Flag, ItunerRawCapCtrl);
        } else {
            AmbaPrint("Null AmpUT_ItunerRawCapture");
            K_ASSERT(0);
        }
        if (Rval != OK) {
            sprintf(OutputStr,"BLC Calibration fail: Raw capture error!");
            AmpUTCalibBLC_PrintError(OutputStr,CalSite);
            return Rval;
        }        
        AmbaPrint("end of raw capture");
        AmbaKAL_TaskSleep(500);

        CalBLCInfo[Channel].SrcRawAddr = (UINT32)RawBufferAddress;
        Rval = AmpCalib_BLCFunc(&CalBLCInfo[Channel],&CalBLCInfo[Channel].BlVideo[DbValue],OutputStr);
        {
            char Fn[32];
            AMBA_FS_FILE *Fp = NULL;
            
            sprintf(Fn, "c:\\blccal_video_%d.raw", DbValue);
            Fn[0] = AmpUT_CalibGetDriverLetter();
            AmbaPrint("AmpUTCalibBLC_SaveRawImage");
            //raw ready, dump it

            Fp = AmbaFS_fopen(Fn, "wb");
            AmbaPrint("[AmpVig_UT]Dump Raw 0x%X width: %d height: %d  ", \
                      CalBLCInfo[Channel].SrcRawAddr, \
                      VinInfo.OutputInfo.RecordingPixels.Width, \
                      VinInfo.OutputInfo.RecordingPixels.Height);
            AmbaFS_fwrite(RawBufferAddress, RawPitch*VinInfo.OutputInfo.RecordingPixels.Height, 1, Fp);
            AmbaFS_fclose(Fp);
        }

        if (Rval < BL_CALIB_OK) {
            AmpUTCalibBLC_PrintError(OutputStr,CalSite);
            return Rval;
        }
    }

    AmbaKAL_BytePoolFree(TempPtrRel);//release memory

//restore backup info
    AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, (UINT32)ChNo, (UINT32)&CurAAAFunc, 0);
    AmbaImg_Proc_Cmd(MW_IP_SET_AE_INFO, (UINT32)ChNo, IP_MODE_STILL,  (UINT32)CurStillAEInfo);
    AmbaImg_Proc_Cmd(MW_IP_SET_AE_INFO, (UINT32)ChNo, IP_MODE_VIDEO,  (UINT32)&CurVideoAEInfo);


    if (Rval == BL_CALIB_OK) {

        if (CalBLCInfo[Channel].BlMode == 0) {
            sprintf(OutputStr, "[OK]BLC - Test - %d, %d, %d, %d",
                    CalBLCInfo[Channel].BlVideo[0].BlackR,
                    CalBLCInfo[Channel].BlVideo[0].BlackGr,
                    CalBLCInfo[Channel].BlVideo[0].BlackGb,
                    CalBLCInfo[Channel].BlVideo[0].BlackB);
        } else {
            BLCVar.BlackR = (CalBLCInfo[Channel].BlVideo[0].BlackR - CalBLCInfo[Channel].BlStd.BlackR);
            BLCVar.BlackGr = (CalBLCInfo[Channel].BlVideo[0].BlackGr - CalBLCInfo[Channel].BlStd.BlackGr);
            BLCVar.BlackGb = (CalBLCInfo[Channel].BlVideo[0].BlackGb - CalBLCInfo[Channel].BlStd.BlackGb);
            BLCVar.BlackB = (CalBLCInfo[Channel].BlVideo[0].BlackB - CalBLCInfo[Channel].BlStd.BlackB);

            BLCVarTmp = BLCVar.BlackR;
            if ( ABS(BLCVarTmp) < ABS(BLCVar.BlackGr) ) {
                BLCVarTmp = BLCVar.BlackGr;
            }
            if ( ABS(BLCVarTmp) < ABS(BLCVar.BlackGb) ) {
                BLCVarTmp = BLCVar.BlackGb;
            }
            if ( ABS(BLCVarTmp) < ABS(BLCVar.BlackB) ) {
                BLCVarTmp = BLCVar.BlackB;
            }
            BLCVarTmp = ABS(BLCVarTmp);

            if (BLCVarTmp <= BLCThreshold[Channel]) {
                CalSite->Version = CAL_BLC_VER;
                CalSite->Status = CAL_SITE_DONE;
                CalSite->SubSiteStatus[0] = CAL_SITE_DONE;
                sprintf(OutputStr, "[OK]BLC");
                CalBLCInfo[Channel].BlMode = BLC_MODE_APPLY;

                AmpCalib_BLCFunc(&CalBLCInfo[Channel],0,OutputStr);
                CalBLCInfo[Channel].BlCalResult = BL_CAL_OK;
                memcpy((CalObj->DramShadow+Channel*(sizeof(Cal_Blc_Info_s))), &CalBLCInfo[Channel], sizeof(Cal_Blc_Info_s));
            } else {
                CalSite->Status = CAL_SITE_RESET;
                sprintf(OutputStr,
                        "[NG]BLC: over threshold BL_Cur/BL_Std/BL_Thr: %d, %d, %d, %d/ %d, %d, %d, %d/ %d",
                        CalBLCInfo[Channel].BlVideo[0].BlackR,
                        CalBLCInfo[Channel].BlVideo[0].BlackGr,
                        CalBLCInfo[Channel].BlVideo[0].BlackGb,
                        CalBLCInfo[Channel].BlVideo[0].BlackB,
                        CalBLCInfo[Channel].BlStd.BlackR,
                        CalBLCInfo[Channel].BlStd.BlackGr,
                        CalBLCInfo[Channel].BlStd.BlackGb,
                        CalBLCInfo[Channel].BlStd.BlackB,
                        BLCThreshold[Channel]);
                AmpUTCalibBLC_PrintError(OutputStr,CalSite);
                Rval = BL_CALIB_ERROR_OVER_THRESHOLD;
            }
        }
    } else {
        CalSite->Status = CAL_SITE_RESET;
        sprintf(OutputStr, "[NG]BLC: Overflow or Unknown bayer pattern!");
    }

    return Rval;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBLC_Upgrade
 *
 *  @Description:: the entry function for black level calibration
 *
 *  @Input      ::
 *          CalSite: calibration site status
 *          CalObj: calibration object
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBLC_Upgrade(Cal_Obj_s *CalObj, Cal_Stie_Status_s* CalSite)
{
    if (CalObj->Version != CalSite->Version) {
        // This API is an example to handle calibration data upgrade
        AmbaPrint("[CAL] Site %s Version mismatch (FW:0x%08X, NAND:0x%08X)", CalObj->Name, CalObj->Version, CalSite->Version);
    }
    // The default behavior is to do-nothing when Version mismatch
    return 0;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTBLC_UTFunc
 *
 *  @Description:: the unit test function for black level calibration
 *
 *  @Input      ::
 *          env: environment
 *          argc: the number of the input parameter
 *          argv: value of input parameter
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: 0/NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTBLC_UTFunc(int Argc, char *Argv[])
{
    Cal_Obj_s           *CalObj;
    int Rval = -1;
    int i;

    CalObj = AmpUT_CalibGetObj(CAL_BLC_ID);
    if ((strcmp(Argv[2], "test") == 0)) {
        //register calibration site
        AmpUT_CalibSiteInit();
        Rval = 0;
    } else if ((strcmp(Argv[2], "init") == 0)) {
        AmpUTCalibBLC_Init(CalObj);
        Rval = 0;
    } else if ((strcmp(Argv[2], "info") == 0)) {
        Cal_Blc_Info_s CalBLCInfo[CALIB_CH_NO];
        UINT8 Channel;

        memcpy(&CalBLCInfo[0], CalObj->DramShadow, (sizeof(Cal_Blc_Info_s)*CALIB_CH_NO));
        for (Channel = 0; Channel < CALIB_CH_NO; Channel ++) {
            AmbaPrint("Channel %d BLC Result = %d ",Channel,CalBLCInfo[Channel].BlCalResult);
            if (CalBLCInfo[Channel].BlCalResult == BL_CAL_OK) {
                AmbaPrint("Standard BlackR = %d BlackGr = %d BlackGb = %d BlackB = %d",CalBLCInfo[Channel].BlStd.BlackR, \
                    CalBLCInfo[Channel].BlStd.BlackGr,CalBLCInfo[Channel].BlStd.BlackGb,CalBLCInfo[Channel].BlStd.BlackB);  

                AmbaPrint("***************************************************");
                for(i = 0; i < 10; i++){
                    AmbaPrint("Video Gain[%d] BlackR = %d BlackGr = %d BlackGb = %d BlackB = %d",(int)pow(2.0, (double)i),CalBLCInfo[Channel].BlVideo[i].BlackR, \
                        CalBLCInfo[Channel].BlVideo[i].BlackGr,CalBLCInfo[Channel].BlVideo[i].BlackGb,CalBLCInfo[Channel].BlVideo[i].BlackB);
                }
                AmbaPrint("***************************************************");
                AmbaPrint("***************************************************");
                for(i = 0; i < 10; i++){
                    AmbaPrint("Still Gain[%d] BlackR = %d BlackGr = %d BlackGb = %d BlackB = %d",(int)pow(2.0, (double)i),CalBLCInfo[Channel].BlVideo[i].BlackR, \
                        CalBLCInfo[Channel].BlVideo[i].BlackGr,CalBLCInfo[Channel].BlVideo[i].BlackGb,CalBLCInfo[Channel].BlVideo[i].BlackB);
                }
                AmbaPrint("***************************************************");
                
            }
        }
        return 0;
    }

    if (Rval == -1) {
        AmbaPrint("t cal blc init : re-init blc");
        AmbaPrint("t cal blc info : BLC information");
    }


    return Rval;
}
