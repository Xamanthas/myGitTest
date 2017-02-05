/**
 * @file mw/unittest/extSrc/sample_calib/warp/AmbaUTCalibWarp.c
 *
 * sample code for warp calibration
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
#include "AmbaDataType.h"
#include "AmbaUTCalibWarp.h"
#include "AmbaUtility.h"
#define IK2StageCompensation 1
#define Calib2StageCompensation 0

extern AMBA_KAL_BYTE_POOL_t G_MMPL;
Warp_Control_s AppWarpControl;
Warp_Storage_s WarpTable[CALIB_CH_NO];
AMBA_DSP_IMG_CALIB_WARP_INFO_s WarpInfo[CALIB_CH_NO];

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibWarp_DebugEnable
 *
 *  @Description:: turn on or turn off the debug flag for warp
 *
 *  @Input      ::
 *          Enable: enable flag to print debug message
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          None
\*-----------------------------------------------------------------------------------------------*/
void AmpUTCalibWarp_DebugEnable(UINT8 Enable)
{
    AppWarpControl.Debug = Enable;
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibWarp_PrintError
 *
 *  @Description:: print the error message for warp calibration
 *
 *  @Input      ::
 *          CalSite: calibration site status
 *
 *  @Output     ::
 *          OutputStr: OutputStr: debug message for this function
 *  @Return     ::
 *          INT: OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibWarp_PrintError(char *OutputStr, Cal_Stie_Status_s *CalSite)
{
    AmbaPrint("************************************************************");
    AmbaPrint("%s",OutputStr);
    AmbaPrint("************************************************************");
    CalSite->Status = CAL_SITE_RESET;
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibWarp_GetCalDataEnable
 *
 *  @Description:: get the warp enable flag
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: enable flag
\*-----------------------------------------------------------------------------------------------*/
UINT8  AmpUTCalibWarp_GetCalDataEnable(void)
{
    return AppWarpControl.Enable;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibWarp_SetCalDataEnable
 *
 *  @Description:: set the warp enable flag
 *
 *  @Input      ::
 *          En: enable flag
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: 0
\*-----------------------------------------------------------------------------------------------*/
UINT8  AmpUTCalibWarp_SetCalDataEnable(UINT8 En)
{
    AppWarpControl.Enable = En;
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibWarp_GetWarpCalData
 *
 *  @Description:: set the warp enable flag
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          Address
\*-----------------------------------------------------------------------------------------------*/
Warp_Control_s * AmpUTCalibWarp_GetWarpCalData(void)
{
    return &AppWarpControl;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibWarp_SelectWarpTable
 *
 *  @Description:: control the warp table
 *
 *  @Input      ::
 *          Enable      : enable flag to control the warp on/off
 *          Channel     : Channel
 *          Id1         : table ID 1
 *          Id2         : table ID 2
 *          BlendRatio  : blend ratio for table 1 and table 2
 *          Strength    : Strength for the warp table
 *
 *  @Output     ::
 *          None:
 *  @Return     ::
 *          INT: OK: 0/ NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibWarp_SelectWarpTable(UINT8 Enable, UINT8 Channel, UINT8 Id1, UINT8 Id2, int BlendRatio, INT32 Strength)
{
    int Rval = 0;
    int i;
    int Ratio1;
    AMBA_DSP_IMG_MODE_CFG_s ImgMode;
    UINT32 Config;
    UINT8 CalibMappingId1,CalibMappingId2;

    memset(&ImgMode, 0, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
    AmbaUT_CalibGetDspMode(&ImgMode);

	if(AppWarpControl.Debug) {
    AmbaPrint("AmpUTCalibWarp_SelectWarpTable: Enable: %d Channel: %d Id1: %d Id2: %d BlendRatio: %d Strength: %d",
              Enable, Channel, Id1, Id2, BlendRatio, Strength);
	}
    CalibMappingId1 = AmpUT_CalibTableMapping(Channel, Id1);
    CalibMappingId2 = AmpUT_CalibTableMapping(Channel, Id2);
    if (Enable) {
        //blending
        if ((CalibMappingId1 != CalibMappingId2)) {
            if (AppWarpControl.WarpTable[CalibMappingId1]->WarpHeader.HorGridNum != AppWarpControl.WarpTable[CalibMappingId2]->WarpHeader.HorGridNum) {
                if(AppWarpControl.Debug){
                AmbaPrint("Horizonal Grid number mismatch");
                }
                return -1;
            }
            if (AppWarpControl.WarpTable[CalibMappingId1]->WarpHeader.VerGridNum != AppWarpControl.WarpTable[CalibMappingId2]->WarpHeader.VerGridNum) {
                if(AppWarpControl.Debug){
                AmbaPrint("Vertical Grid number mismatch");
                }
                return -1;
            }
            if ((BlendRatio < 0) || (BlendRatio > 65536)) {
                if(AppWarpControl.Debug){
                AmbaPrint("BlendRatio should be between 0~65536, BlendRatio = %d",BlendRatio);
                }
                return -1;
            }
            Ratio1 = 65536 - BlendRatio;
            WarpTable[Channel].WarpHeader = AppWarpControl.WarpTable[CalibMappingId1]->WarpHeader;
            for (i = 0; i < AppWarpControl.WarpTable[CalibMappingId1]->WarpHeader.HorGridNum*AppWarpControl.WarpTable[CalibMappingId1]->WarpHeader.VerGridNum; i++) {
                WarpTable[Channel].WarpVector[i].X = (AppWarpControl.WarpTable[CalibMappingId1]->WarpVector[i].X*BlendRatio + \
                                                      AppWarpControl.WarpTable[CalibMappingId2]->WarpVector[i].X*Ratio1)>>16;
                WarpTable[Channel].WarpVector[i].Y = (AppWarpControl.WarpTable[CalibMappingId1]->WarpVector[i].Y*BlendRatio + \
                                                      AppWarpControl.WarpTable[CalibMappingId2]->WarpVector[i].Y*Ratio1)>>16;
            }
        } else if (CalibMappingId1 == CalibMappingId2) {
            WarpTable[Channel] = *(AppWarpControl.WarpTable[CalibMappingId1]);
        } else if (BlendRatio == 0) {
            WarpTable[Channel] = *(AppWarpControl.WarpTable[CalibMappingId1]);
        } else if (BlendRatio == 65536) {
            WarpTable[Channel] = *(AppWarpControl.WarpTable[CalibMappingId2]);
        }
        //decay
        if ((Strength < 0) || (Strength > 65536)) {
            if(AppWarpControl.Debug){
            AmbaPrint("Strength should be between 0~65536, Strength = %d",Strength);
            }
            return -1;
        }
        if (Strength != 65536) {
            for (i = 0; i < AppWarpControl.WarpTable[CalibMappingId1]->WarpHeader.HorGridNum*AppWarpControl.WarpTable[CalibMappingId1]->WarpHeader.VerGridNum; i++) {
                WarpTable[Channel].WarpVector[i].X = (WarpTable[Channel].WarpVector[i].X*Strength)>>16;
                WarpTable[Channel].WarpVector[i].Y = (WarpTable[Channel].WarpVector[i].Y*Strength)>>16;
            }
        }
        Config = 0;//enable warp
        WarpInfo[Channel].Version                            = WarpTable[Channel].WarpHeader.Version;
        WarpInfo[Channel].HorGridNum                         = WarpTable[Channel].WarpHeader.HorGridNum;
        WarpInfo[Channel].VerGridNum                         = WarpTable[Channel].WarpHeader.VerGridNum;
        WarpInfo[Channel].TileWidthExp                       = WarpTable[Channel].WarpHeader.TileWidthExp;
        WarpInfo[Channel].TileHeightExp                      = WarpTable[Channel].WarpHeader.TileHeightExp;
        WarpInfo[Channel].VinSensorGeo.Width                 = WarpTable[Channel].WarpHeader.VinSensorGeo.Width;
        WarpInfo[Channel].VinSensorGeo.Height                = WarpTable[Channel].WarpHeader.VinSensorGeo.Height;
        WarpInfo[Channel].VinSensorGeo.StartX                = WarpTable[Channel].WarpHeader.VinSensorGeo.StartX;
        WarpInfo[Channel].VinSensorGeo.StartY                = WarpTable[Channel].WarpHeader.VinSensorGeo.StartY;
        WarpInfo[Channel].VinSensorGeo.HSubSample.FactorDen  = WarpTable[Channel].WarpHeader.VinSensorGeo.HSubSample.FactorDen;
        WarpInfo[Channel].VinSensorGeo.HSubSample.FactorNum  = WarpTable[Channel].WarpHeader.VinSensorGeo.HSubSample.FactorNum;
        WarpInfo[Channel].VinSensorGeo.VSubSample.FactorDen  = WarpTable[Channel].WarpHeader.VinSensorGeo.VSubSample.FactorDen;
        WarpInfo[Channel].VinSensorGeo.VSubSample.FactorNum  = WarpTable[Channel].WarpHeader.VinSensorGeo.VSubSample.FactorNum;
        WarpInfo[Channel].Enb2StageCompensation              = (WarpTable[Channel].WarpHeader.Warp2StageFlag == 1) ? Calib2StageCompensation : IK2StageCompensation;
        WarpInfo[Channel].Reserved1                          = WarpTable[Channel].WarpHeader.Reserved1;
        WarpInfo[Channel].Reserved2                          = WarpTable[Channel].WarpHeader.Reserved2;
        WarpInfo[Channel].pWarp                              = (AMBA_DSP_IMG_GRID_POINT_s*)WarpTable[Channel].WarpVector;
    } else {
        Config = AMBA_DSP_IMG_WARP_CONFIG_FORCE_DISABLE;//disable warp
    }
	if(AppWarpControl.Debug) {
		AmbaPrint("version: 0x%x",WarpInfo[Channel].Version);
		AmbaPrint("horizontal grid number: %d",WarpInfo[Channel].HorGridNum);
		AmbaPrint("vertical grid number: %d",WarpInfo[Channel].VerGridNum);
		AmbaPrint("tile width: %d",WarpInfo[Channel].TileWidthExp);
		AmbaPrint("tile height: %d",WarpInfo[Channel].TileHeightExp);
		AmbaPrint("Image width: %d",WarpInfo[Channel].VinSensorGeo.Width);
		AmbaPrint("Image height: %d",WarpInfo[Channel].VinSensorGeo.Height);
		AmbaPrint("StartX: %d",WarpInfo[Channel].VinSensorGeo.StartX);
		AmbaPrint("StartY: %d",WarpInfo[Channel].VinSensorGeo.StartY);
		AmbaPrint("HSubSample FactorDen: %d",WarpInfo[Channel].VinSensorGeo.HSubSample.FactorDen);
		AmbaPrint("HSubSample FactorNum: %d",WarpInfo[Channel].VinSensorGeo.HSubSample.FactorNum);
		AmbaPrint("VSubSample FactorDen: %d",WarpInfo[Channel].VinSensorGeo.VSubSample.FactorDen);
		AmbaPrint("VSubSample FactorNum: %d",WarpInfo[Channel].VinSensorGeo.VSubSample.FactorNum);
		AmbaPrint("Warp table address: 0x%X",WarpInfo[Channel].pWarp);
	}
    ImgMode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
    AmbaDSP_WarpCore_SetCalibWarpInfo(&ImgMode, &WarpInfo[Channel]);
    AmbaDSP_WarpCore_CalcDspWarp(&ImgMode, Config);
    AmbaDSP_WarpCore_SetDspWarp(&ImgMode);
    /*set the warp info for still mode*/
    ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
    ImgMode.BatchId = AMBA_DSP_NO_BATCH_FILTER;
    AmbaDSP_WarpCore_SetCalibWarpInfo(&ImgMode, &WarpInfo[Channel]);

    return Rval;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibWarp_Init
 *
 *  @Description:: initial function for warp calibration
 *
 *  @Input      ::
 *          CalObj:  calibration object
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibWarp_Init(Cal_Obj_s *CalObj)
{
    int i;
    UINT8 *WarpAddress = CalObj->DramShadow;
    static UINT8 WarpInitFlag = 0;

    if (WarpInitFlag == 0) {
        memset(&AppWarpControl,0,sizeof(Warp_Control_s));
        WarpInitFlag = 1;
    }
    AppWarpControl.Enable = (UINT32)WarpAddress[CAL_WARP_ENABLE];
    //AppWarpControl.Enable = get_int((INT32*)&WarpAddress[CAL_WARP_ENABLE]);
    AmbaPrint("cal_init_warp.Enable %d",AppWarpControl.Enable);


    if (AppWarpControl.Enable == 1) {
        AppWarpControl.WarpTableCount = (UINT32)WarpAddress[CAL_WARP_TABLE_COUNT];
        //AppWarpControl.WarpTableCount = get_int((INT32*)&WarpAddress[CAL_WARP_TABLE_COUNT]);
        for (i=0; i < MAX_WARP_TABLE_COUNT; i++) {
            AppWarpControl.WarpTable[i] = (Warp_Storage_s*)&WarpAddress[CAL_WARP_TABLE(i)];
            //AppWarpControl.WarpTable[i] =(Warp_Storage_s*) &WarpAddress[CAL_WARP_TABLE(i)];
            //AppWarpControl.WarpTable[i] =(Warp_Storage_s*) (WarpAddress+CAL_WARP_TABLE(i));
        }

        if (AppWarpControl.WarpTable[0]->WarpHeader.Enable == 1) {
            AmbaPrint("Set default WARP!");
            AmpUTCalibWarp_SelectWarpTable(1, 0, 0, 0, 0x10000, 65536);
        }
        AmbaPrint("version: 0x%x", AppWarpControl.WarpTable[0]->WarpHeader.Version);
    }

    return 0;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibWarp_GenWarpFromSpec
 *
 *  @Description:: get multi lines from the text file
 *
 *  @Input      ::
 *          argc: the number of the input parameter
 *          argv: value of input parameter
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK:0 / NG:NG message
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibWarp_GenWarpFromSpec(int Argc, char *Argv[])
{
    int I=0, J =0,   Rval = -1;
    int IdxNumR = 0;
    int IdxNumE = 0;
    double RealWidth = 0, R = 0, Cos = 0, Sin = 0;
    INT16 TileWidthExp  =   7;
    INT16 TileHeightExp  =  7;
    INT32 StartX = 0;
    INT32 StartY = 0;
    Cal_Warp_Tool_Info_s ToolInfo;//sharon
    AMBA_FS_FILE *Fid = 0;
    AMBA_FS_FILE *FpScript = NULL;
    UINT32 Tmp;

    INT32 ExceptW = 0;
    INT32 ExceptH = 0;

    INT32 WarpHeader[16] = {0};
    int  ArgcR;
    char *ArgvR[MAX_CMD_TOKEN];
    char Buffer[256];
    char Change_line='\n';
    char Str[256];
    AMBA_FS_FILE *Fw_path = NULL,*Fr_path = NULL;
    int SensorFlag = 0;
    int RealFlag = 0;
    int ExpectFlag = 0;
    int CompensateRatioFlag = 0;
    int RawEncodeTextFlag = 0;
    int RatioRFlag = 0;
    int SensorCalFlag = 0;
    int SensorRawFlag = 0;
    Cal_Warp_Table_Info_s Info;
    char WarpHfileFn [20] = {'c',':','\\','w','a','r','p','_','b','Y','_','s','p','e','c','.','h','\0'};
    char WarpCalibFn[30] = {'c',':','\\','c','a','l','i','b','6','.','b','i','n','\0'};
    char WarpRawEncFn[30] = {'c',':','\\','w','a','r','p','R','a','w','E','n','c','o','d','e','.','b','i','n','\0'};
    char *WarpReadITunerFn;
    char *WarpWriteITunerFn;
    UINT8 WarpMaxIdxNum = 64;

    void *TempWp,*TempOldWp, *TempMwWp, *TempReal,*TempExpect;
    void *TempWpBuffer,*TempMwWpBuffer,*TempOldWpBuffer,*TempRealBuffer,*TempExpectBuffer;
    UINT8 HSubSampleFactorNum=1, HSubSampleFactorDen=1, VSubSampleFactorNum=1, VSubSampleFactorDen=1;
    UINT32 WarpZoomStep = 0,WarpChannel= 0;
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 0 } };
    UINT8 *BufferAddress;
    AMBA_SENSOR_MODE_INFO_s VinInfo;
    AMBA_SENSOR_MODE_ID_u SensorMode = {
        .Bits = {
            .Mode = 0,
            .VerticalFlip = 0,
        }
    };
    ToolInfo.IdxNum = 0;
    ToolInfo.Tile.TileWidth   =64;
    ToolInfo.Tile.TileHeight  =64;
    ToolInfo.Tile.HorGridNum =0;
    ToolInfo.Tile.VerGridNum =0;
    ToolInfo.Tile.AddTableGridNumH =0;
    ToolInfo.Tile.AddTableGridNumW =0;
    ToolInfo.CAL.ImgWidth   =4608;
    ToolInfo.CAL.ImgHeight  =3456;
    ToolInfo.RatioR =100;
    ToolInfo.CompensateRatio =100;
    ToolInfo.CompensateRatioSwaX =100;
    ToolInfo.CompensateRatioSwaY =100;
    ToolInfo.MwWp = NULL;
    ToolInfo.UnitCellSize = 1;


    SensorMode.Bits.Mode = pAmbaCalibInfoObj[0/*AMBA_VIN_CHANNEL0*/]->AmbaVignetteMode;
    AmbaSensor_GetModeInfo(Chan, SensorMode, &VinInfo);
    if (AmpCalib_GetOpticalBlackFlag() == 1) {
        StartX = VinInfo.InputInfo.PhotodiodeArray.StartX - (VinInfo.OutputInfo.RecordingPixels.StartX * VinInfo.InputInfo.HSubsample.FactorDen \
                                                                                /VinInfo.InputInfo.HSubsample.FactorNum);
        StartY = VinInfo.InputInfo.PhotodiodeArray.StartY -  (VinInfo.OutputInfo.RecordingPixels.StartY * VinInfo.InputInfo.VSubsample.FactorDen \
                                                                                /VinInfo.InputInfo.VSubsample.FactorNum);
    } else {
    StartX = VinInfo.InputInfo.PhotodiodeArray.StartX;
    StartY = VinInfo.InputInfo.PhotodiodeArray.StartY;
    }

    AmbaPrint("START_X = %d, START_y= %d",StartX,StartY);

    RawEncodeTextFlag = atoi(Argv[3]);

    //use Big array, release in the end of this function.

    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempReal, &TempRealBuffer, WarpMaxIdxNum*sizeof(double), 32);
    BufferAddress = (UINT8*)((UINT32)TempReal) ;
    if (Rval != OK) {
        AmbaPrint("ToolInfo.Real allocate fail in %s:%u", __FUNCTION__, __LINE__);
        return -1;
    } else {
        ToolInfo.Real = (double *)BufferAddress;
        AmbaPrint("[AmpUTCalibWarp]rawBuffAddr (0x%08X) (%u)!", ToolInfo.Real, WarpMaxIdxNum*sizeof(double));
    }
    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempExpect, &TempExpectBuffer, WarpMaxIdxNum*sizeof(double), 32);
    BufferAddress = (UINT8*)((UINT32)TempExpect) ;
    if (Rval != OK) {
        AmbaPrint("ToolInfo.Expect allocate fail in %s:%u", __FUNCTION__, __LINE__);
        return -1;
    } else {
        ToolInfo.Expect = (double *)BufferAddress;
        AmbaPrint("[AmpUTCalibWarp]rawBuffAddr (0x%08X) (%u)!", ToolInfo.Expect, WarpMaxIdxNum*sizeof(double));
    }

    // Read A line from the file
    AmbaPrint("Argc = %d", Argc);
    if (Argc == 7 || Argc == 5 ) {}
    else    {
        AmbaPrint(
            "t cal warp warp_spec: Generate WARP calibration table\n"
            "t cal warp warp_spec [raw encode text flag] [filename spec] [raw_input filename ] [raw_output filename] \n");
        return WARP_CALIB_INCORRECT_ARGUMENT_NUM;
    }
    if ((RawEncodeTextFlag == IDEA_WARP) || (RawEncodeTextFlag == IDEA_WARP_RAWENC) ||( RawEncodeTextFlag == DISTORTION_RAW) || \
            (RawEncodeTextFlag == SWA_XY_WARP)||(RawEncodeTextFlag ==SWA_XY_WARP_RAWENC)) {
    } else {
        AmbaPrint(
            "[raw encode text flag] = \n"
            "idea WARP ==>  0: raw encode disable , 1: raw encode enable, 99: generate the raw with distortion \n"
            "keep view angle WARPP (keep X & Y )==> 20 raw encode disable, 21:  raw encode enable \n"
            "t cal warp warp_spec [raw encode text flag] [filename spec] [raw_input filename ] [raw_output filename] \n");
        return WARP_CALIB_INCORRECT_PARAMETER;
    }
    if ((RawEncodeTextFlag == IDEA_WARP_RAWENC) ||(RawEncodeTextFlag == SWA_XY_WARP_RAWENC)) {
        if (Argc == 7) {
            WarpReadITunerFn = Argv[5];
            WarpWriteITunerFn = Argv[6];
        } else {
            AmbaPrint(
                "t cal warp warp_spec: Generate WARP calibration table\n"
                "t cal warp warp_spec [raw encode text flag] [filename spec] [raw_input filename ] [raw_output filename] \n");
            return WARP_CALIB_INCORRECT_PARAMETER;
        }
    }

    //text raw encode
    if ((RawEncodeTextFlag == IDEA_WARP_RAWENC) || (RawEncodeTextFlag == SWA_XY_WARP_RAWENC)) {
        WarpReadITunerFn[0] = AmpUT_CalibGetDriverLetter();
        Fr_path = AmbaFS_fopen(WarpReadITunerFn, "r");
        if (Fr_path == NULL) {
            AmbaPrint("AmbaFS_fopen %s fail.",WarpReadITunerFn);
            Rval = WARP_CALIB_OPEN_FILE_ERROR;
            return Rval;
        }
        WarpWriteITunerFn[0] = AmpUT_CalibGetDriverLetter();
        Fw_path = AmbaFS_fopen(WarpWriteITunerFn, "w");
        if (Fw_path == NULL) {
            AmbaPrint("AmbaFS_fopen %s fail. ",WarpWriteITunerFn);
            Rval = WARP_CALIB_OPEN_FILE_ERROR;
            return Rval;
        }
        //bug, need to add in the future...
        Rval = AmbaFS_fclose(Fr_path);
        if (Rval != OK) {
            AmbaPrint("AmbaFS_fclose %s fail.",Argv[4]);
            Rval = WARP_CALIB_CLOSE_FILE_ERROR;
            return Rval;
        }
        Rval = AmbaFS_fclose(Fw_path);
        if (Rval != OK) {
            AmbaPrint("AmbaFS_fclose %s fail.",Argv[5]);
            Rval = WARP_CALIB_CLOSE_FILE_ERROR;
            return Rval;
        }
    }    ///text raw encode end
    
    ToolInfo.Warp2StageFlag = 1; //set default value to 1
    //for generate 1D WARP table
    if ((RawEncodeTextFlag == IDEA_WARP) || (RawEncodeTextFlag == IDEA_WARP_RAWENC) ||( RawEncodeTextFlag == DISTORTION_RAW) || \
            (RawEncodeTextFlag == SWA_XY_WARP)||(RawEncodeTextFlag ==SWA_XY_WARP_RAWENC)) {
        //set initial value for Info.OffSensorFlag
        Info.OffSensorFlag = 0;
        FpScript = AmbaFS_fopen(Argv[4], "r");
        if (FpScript == NULL) {
            AmbaPrint("FpScript AmbaFS_fopen %s fail.",Argv[4]);
            Rval = WARP_CALIB_OPEN_FILE_ERROR;
            return Rval;
        }
        while (1) {
            Rval = AmpUT_MultiGetline(FpScript, Buffer);
            if (Rval < WARP_CALIB_OK) {
                break;
            } else {
                ArgcR = 0;
                memset(ArgvR, 0, MAX_CMD_TOKEN*sizeof(char *));
                {
                    // Parse the input string to multiple tokens
                    char *Token = strtok(Buffer, " ");
                    int NumToken = 0;
                    while (Token != NULL) {
                        ArgvR[NumToken] = Token;
                        NumToken++;
                        Token = strtok(NULL, " ");
                    }
                    ArgcR = NumToken;
                }
                if (strcmp(ArgvR[0], "TileWidth") == 0) {
                    ToolInfo.Tile.TileWidth = (INT16) atoi(ArgvR[1]);
                } else if (strcmp(ArgvR[0], "TileHeight") == 0) {
                    ToolInfo.Tile.TileHeight= (INT16) atoi(ArgvR[1]);
                } else if (strcmp(ArgvR[0], "off_sensor_calibration") == 0) {
                    SensorCalFlag = 1;
                    Info.OffSensorFlag = 1;
                    ToolInfo.CAL.ImgWidth = (INT32) atoi(ArgvR[1]);
                    ToolInfo.CAL.ImgHeight = (INT32) atoi(ArgvR[2]);
                    RealWidth = (double) atof(ArgvR[3]);
                    StartX = (INT32) atoi(ArgvR[4]);
                    StartY = (INT32) atoi(ArgvR[5]);
                    if (ArgcR == 8) {
                        ToolInfo.CAL.CenterX = (INT32) atoi(ArgvR[6]);
                        ToolInfo.CAL.CenterY = (INT32) atoi(ArgvR[7]);
                        if (ToolInfo.CAL.CenterX < (ToolInfo.CAL.ImgWidth*3>>3) || ToolInfo.CAL.CenterX > (ToolInfo.CAL.ImgWidth*5>>3)) {
                            SensorFlag = 0;
                            AmbaPrint("Error : CenterX out of the range ((ImgWidth*3/8) < CenterX < (ImgWidth*5/8))");
                        }
                        if (ToolInfo.CAL.CenterY < (ToolInfo.CAL.ImgHeight*3>>3)  || ToolInfo.CAL.CenterY > (ToolInfo.CAL.ImgHeight*5>>3)) {
                            SensorFlag = 0;
                            AmbaPrint("Error : CenterY out of the range ((ImgHeight*3/8) < CenterY < (ImgHeight*5/8))");
                        }
                    } else {
                        ToolInfo.CAL.CenterX = ToolInfo.CAL.ImgWidth>>1;
                        ToolInfo.CAL.CenterY = ToolInfo.CAL.ImgHeight>>1;
                    }
                    ToolInfo.UnitCellSize = RealWidth / ToolInfo.CAL.ImgWidth;
                    ToolInfo.Tile.HorGridNum = (ToolInfo.CAL.ImgWidth+ToolInfo.Tile.TileWidth-1)/ToolInfo.Tile.TileWidth + 1 ;
                    ToolInfo.Tile.VerGridNum = (ToolInfo.CAL.ImgHeight+ToolInfo.Tile.TileHeight-1)/ToolInfo.Tile.TileHeight+ 1 ;
                } else if (strcmp(ArgvR[0], "off_sensor_raw_image") == 0) {
                    SensorRawFlag = 1;
                    Info.OffSensorFlag = 1;
                    Info.ImgW = (INT32) atoi(ArgvR[1]);
                    Info.ImgH = (INT32) atoi(ArgvR[2]);
                    Info.StartX = (INT32) atoi(ArgvR[3]);
                    Info.StartY = (INT32) atoi(ArgvR[4]);

                } else if (strcmp(ArgvR[0], "Enable2StageWarp") == 0) {
                    ToolInfo.Warp2StageFlag = (INT8) atoi(ArgvR[1]);
                    AmbaPrint("Enable2StageWarp = %d",ToolInfo.Warp2StageFlag);
                } else if (strcmp(ArgvR[0], "sensor") == 0) {
                    SensorFlag = 1;
                    Info.ImgW = ToolInfo.CAL.ImgWidth = (INT32) atoi(ArgvR[1]);
                    Info.ImgH = ToolInfo.CAL.ImgHeight = (INT32) atoi(ArgvR[2]);
                    RealWidth = (double) atof(ArgvR[3]);
                    if (ArgcR == 6) {
                        ToolInfo.CAL.CenterX = (INT32) atoi(ArgvR[4]);
                        ToolInfo.CAL.CenterY = (INT32) atoi(ArgvR[5]);
                        AmbaPrint("center x = %d center y = %d",ToolInfo.CAL.CenterX,ToolInfo.CAL.CenterY);
                        if (ToolInfo.CAL.CenterX < (ToolInfo.CAL.ImgWidth*3>>3) || ToolInfo.CAL.CenterX > (ToolInfo.CAL.ImgWidth*5>>3)) {
                            SensorFlag = 0;
                            AmbaPrint("Error : CenterX out of the range ((ImgWidth*3/8) < CenterX < (ImgWidth*5/8))");
                        }
                        if (ToolInfo.CAL.CenterY < (ToolInfo.CAL.ImgHeight*3>>3)  || ToolInfo.CAL.CenterY > (ToolInfo.CAL.ImgHeight*5>>3)) {
                            SensorFlag = 0;
                            AmbaPrint("Error : CenterY out of the range ((ImgHeight*3/8) < CenterY < (ImgHeight*5/8))");
                        }
                    } else {
                        ToolInfo.CAL.CenterX = ToolInfo.CAL.ImgWidth>>1;
                        ToolInfo.CAL.CenterY = ToolInfo.CAL.ImgHeight>>1;
                    }
                    ToolInfo.UnitCellSize = RealWidth / ToolInfo.CAL.ImgWidth;
                    ToolInfo.Tile.HorGridNum = (ToolInfo.CAL.ImgWidth+ToolInfo.Tile.TileWidth-1)/ToolInfo.Tile.TileWidth + 1 ;
                    ToolInfo.Tile.VerGridNum = (ToolInfo.CAL.ImgHeight+ToolInfo.Tile.TileHeight-1)/ToolInfo.Tile.TileHeight+ 1 ;
                    AmbaPrint("cell size = %f HorGridNum = %d VerGridNum = %d",ToolInfo.UnitCellSize,ToolInfo.Tile.HorGridNum,ToolInfo.Tile.VerGridNum);
                    ToolInfo.Tile.AddTableGridNumW =0;
                    ToolInfo.Tile.AddTableGridNumH = 0;

                } else if (strcmp(ArgvR[0], "real") == 0) {
                    IdxNumR = ArgcR - 1;
                    for ( I=1; I<ArgcR; I++) {
                        RealFlag = 1;
                        ToolInfo.Real[I-1] = (double) atof(ArgvR[I]);
                    }
                } else if (strcmp(ArgvR[0], "expect") == 0) {
                    IdxNumE = ArgcR - 1;
                    for ( I=1; I<ArgcR; I++) {
                        ExpectFlag = 1;
                        ToolInfo.Expect[I-1] = (double) atof(ArgvR[I]);
                    }
                } else if (strcmp(ArgvR[0], "Compensate_ratio") == 0) {
                    ToolInfo.CompensateRatio = atoi(ArgvR[1]);
                    if (ToolInfo.CompensateRatio<0 || ToolInfo.CompensateRatio>200) {
                        AmbaPrint("Compensate_ratio must have be 0~200.");
                    } else {
                        CompensateRatioFlag = 1;
                    }
                } else if (strcmp(ArgvR[0], "Compensate_ratio_swa_y") == 0) {
                    ToolInfo.CompensateRatioSwaY = atoi(ArgvR[1]);
                    AmbaPrint("Compensate_ratio_swa_y %d",ToolInfo.CompensateRatioSwaY);
                    if (ToolInfo.CompensateRatioSwaY<0 || ToolInfo.CompensateRatioSwaY>200) {
                        AmbaPrint("Compensate_ratio_swa_y must have be 0~200.");
                    }
                } else if (strcmp(ArgvR[0], "Compensate_ratio_swa_x") == 0) {
                    ToolInfo.CompensateRatioSwaX = atoi(ArgvR[1]);
                    AmbaPrint("Compensate_ratio_swa_x %d",ToolInfo.CompensateRatioSwaX);
                    if (ToolInfo.CompensateRatioSwaX<0 || ToolInfo.CompensateRatioSwaX>200) {
                        AmbaPrint("Compensate_ratio_swa_x must have be 0~200.");
                    }
                } else if (strcmp(ArgvR[0], "r_ratio") == 0) {
                    ToolInfo.RatioR = atoi(ArgvR[1]);
                    if (ToolInfo.RatioR<=0 || ToolInfo.RatioR>200) {
                        AmbaPrint("r_ratio must have be 1~200.");
                    } else {
                        RatioRFlag = 1;
                    }
                } else if (strcmp(ArgvR[0], "zoom_step") == 0) {
                    WarpZoomStep = atoi(ArgvR[1]);
                }
            }
        }
        Rval = AmbaFS_fclose(FpScript);
        if (Rval != OK) {
            AmbaPrint("FpScript AmbaFS_fclose fail .");
            Rval = WARP_CALIB_CLOSE_FILE_ERROR;
            return Rval;
        }
        if (Info.OffSensorFlag ) {
            SensorFlag = (SensorRawFlag & SensorCalFlag);
        }
        if ((SensorFlag == 0)||(RealFlag == 0)||(ExpectFlag == 0)||(CompensateRatioFlag == 0)||(RatioRFlag == 0)) {
            if (SensorFlag == 0)            AmbaPrint(" (off) Sensor parameter not Found or some error");
            if (RealFlag == 0)              AmbaPrint("Real warp spec parameter not Found or some error");
            if (ExpectFlag == 0)            AmbaPrint("Expect warp spec parameter not Found or some error");
            if (CompensateRatioFlag == 0)   AmbaPrint("CompensateRatio parameter not Found or some error");
            if (RatioRFlag == 0)            AmbaPrint("R_ratio parameter not Found or some error");
            Rval = WARP_CALIB_INCORRECT_PARAMETER;
            return Rval;
        }
        if (IdxNumE != IdxNumR) {
            AmbaPrint("Real spec Index num (%d) != Expect spec Index num (%d)",IdxNumR,IdxNumE);
            Rval = WARP_CALIB_INCORRECT_INPUT_NUM;
            return Rval;
        }
        if ((IdxNumR > WarpMaxIdxNum) || (IdxNumE > WarpMaxIdxNum)) {
            AmbaPrint("the input real/expect parameters should be smaller than %d",WarpMaxIdxNum);
            Rval = WARP_CALIB_TOO_MUCH_INPUT_NUM;
            return Rval;
        }
        ToolInfo.IdxNum = IdxNumE;
        for (I = 0; I < ToolInfo.IdxNum; I++) {
            ToolInfo.Real[I] = ToolInfo.Real[I]/ToolInfo.UnitCellSize; //in pixel
            ToolInfo.Expect[I] = ToolInfo.Expect[I]/ToolInfo.UnitCellSize;//in pixel

        }
        AmbaPrint("ToolInfo.Expect[IdxNum-1]  %f",ToolInfo.Expect[ToolInfo.IdxNum-1] );

        Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempMwWp, &TempMwWpBuffer, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(AMBA_DSP_IMG_GRID_POINT_s), 32);
        BufferAddress = (UINT8*)((UINT32)TempMwWp) ;
        if (Rval != OK) {
            AmbaPrint("ToolInfo.MwWp allocate fail in %s:%u", __FUNCTION__, __LINE__);
            return -1;
        } else {
            ToolInfo.MwWp = (AMBA_DSP_IMG_GRID_POINT_s *)BufferAddress;
            AmbaPrint("[AmpUTCalibWarp]rawBuffAddr (0x%08X) (%u)!", ToolInfo.MwWp, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(AMBA_DSP_IMG_GRID_POINT_s));
        }

        R = pow((ToolInfo.CAL.ImgWidth*ToolInfo.CAL.ImgWidth + ToolInfo.CAL.ImgHeight*ToolInfo.CAL.ImgHeight), 0.5);
        if (R ==0 ) {
            AmbaPrint("[AppLibCalibWarp] ERROR! div 0 ");
            Rval = WARP_CALIB_DIV0;
            return Rval;
        }
        Cos = ToolInfo.CAL.ImgWidth/R;
        Sin = ToolInfo.CAL.ImgHeight/R;
        ExceptW = (UINT32)(ToolInfo.Expect[ToolInfo.IdxNum-1] * Cos);
        ExceptH = (UINT32)(ToolInfo.Expect[ToolInfo.IdxNum-1] * Sin);


        if ((RawEncodeTextFlag == SWA_XY_WARP)||(RawEncodeTextFlag == SWA_XY_WARP_RAWENC)) { // if gen a special warp table have be a bigger table
            ToolInfo.Tile.AddTableGridNumW =((ExceptW>>1)/ToolInfo.Tile.TileWidth )<<1 ;
            ToolInfo.Tile.AddTableGridNumH = ((ExceptH>>1)/ToolInfo.Tile.TileHeight)<<1;
            ToolInfo.Tile.HorGridNum += (ToolInfo.Tile.AddTableGridNumW )<<1;
            ToolInfo.Tile.VerGridNum  += (ToolInfo.Tile.AddTableGridNumH )<<1;
        }

        AmbaPrint("AddTableGridNumW  %d  AddTableGridNumH  %d",ToolInfo.Tile.AddTableGridNumW,ToolInfo.Tile.AddTableGridNumH);
        AmbaPrint("HorGridNum  %d, VerGridNum  %d",ToolInfo.Tile.HorGridNum,ToolInfo.Tile.VerGridNum);

        //use Big array, release in the end of this function.
        Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempWp, &TempWpBuffer, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(Warp_TOOL_IMG_GRID_POINT_s), 32);
        BufferAddress = (UINT8*)((UINT32)TempWp) ;
        if (Rval != OK) {
            AmbaPrint("ToolInfo.Wp allocate fail in %s:%u", __FUNCTION__, __LINE__);
            return -1;
        } else {
            ToolInfo.Wp = (Warp_TOOL_IMG_GRID_POINT_s *)BufferAddress;
            AmbaPrint("[AmpUTCalibWarp]rawBuffAddr (0x%08X) (%u)!", ToolInfo.Wp, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(Warp_TOOL_IMG_GRID_POINT_s));
        }
        Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempOldWp, &TempOldWpBuffer, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(Warp_TOOL_IMG_GRID_POINT_s), 32);
        BufferAddress = (UINT8*)((UINT32)TempOldWp) ;
        if (Rval != OK) {
            AmbaPrint("ToolInfo.OldWp allocate fail in %s:%u", __FUNCTION__, __LINE__);
            return -1;
        } else {
            ToolInfo.OldWp = (Warp_TOOL_IMG_GRID_POINT_s *)BufferAddress;
            AmbaPrint("[AppLibCalibWarp]rawBuffAddr (0x%08X) (%u)!", ToolInfo.OldWp, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(Warp_TOOL_IMG_GRID_POINT_s));
        }
        Rval= AmbaCalib_CalWarpTable(&ToolInfo, RawEncodeTextFlag);
        if (Rval != OK) {
            AmbaPrint("[AppLibCalibWarp]Error Rval: NG!");
            Rval = WARP_CALIB_DIV0;
            return Rval;
        }
    }

    WarpHfileFn[0] = AmpUT_CalibGetDriverLetter();
    Fid = AmbaFS_fopen(WarpHfileFn, "w");
    if (Fid == NULL) {
        AmbaPrint("AmbaFS_fopen fail .");
        Rval = WARP_CALIB_OPEN_FILE_ERROR;
        return Rval;
    }
    //TileWidthExp;   /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    //TileWidthExp;   /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    switch (ToolInfo.Tile.TileWidth) {
        case 16:
            TileWidthExp = 4;
            break;
        case 32:
            TileWidthExp = 5;
            break;
        case 64:
            TileWidthExp = 6;
            break;
        case 128:
            TileWidthExp = 7;
            break;
        case 256:
            TileWidthExp = 8;
            break;
        case 512:
            TileWidthExp = 9;
            break;
        default:
            TileWidthExp = 7;//set default value to 7;
    }

    switch (ToolInfo.Tile.TileHeight) {
        case 16:
            TileHeightExp = 4;
            break;
        case 32:
            TileHeightExp = 5;
            break;
        case 64:
            TileHeightExp = 6;
            break;
        case 128:
            TileHeightExp = 7;
            break;
        case 256:
            TileHeightExp = 8;
            break;
        case 512:
            TileHeightExp = 9;
            break;
        default:
            TileHeightExp = 7;//set default value to 7;
    }

    WarpHeader[0]   = CAL_WARP_VER;
    WarpHeader[1]   = ToolInfo.Tile.HorGridNum;
    WarpHeader[2]   = ToolInfo.Tile.VerGridNum;
    WarpHeader[3]   = TileWidthExp;
    WarpHeader[4]   = TileHeightExp;
    WarpHeader[5]   = StartX;
    WarpHeader[6]   = StartY;
    WarpHeader[7]   = ToolInfo.CAL.ImgWidth;
    WarpHeader[8]   = ToolInfo.CAL.ImgHeight;
    WarpHeader[9]   = HSubSampleFactorNum;
    WarpHeader[10]  = HSubSampleFactorDen;
    WarpHeader[11]  = VSubSampleFactorNum;
    WarpHeader[12]  = VSubSampleFactorDen;
    WarpHeader[13]  = RESERVED1;
    WarpHeader[14]  = RESERVED2;
    WarpHeader[15]  = RESERVED3;
    //write warp header into file
    sprintf(Str, "%s",  "#define HorGridNum ");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    sprintf(Str, " %5d ",  WarpHeader[1]);
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "%s",  "#define VerGridNum ");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    sprintf(Str, " %5d ",  WarpHeader[2]);
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "%s",  "INT32 calib_warp_header_spec[64] =");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "%s",  "{");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "0x%X,",  CAL_WARP_VER);
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    for (I=1; I<16; I++) {
        sprintf(Str, " %5d, ", WarpHeader[I] );
        AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
        AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    }
    sprintf(Str, "%s",  "};");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);

    //write warp table into file
    sprintf(Str, "%s",  "INT16 calib_warp_table_spec horizontal [HorGridNum * VerGridNum *2] =");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "%s",  "{");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    for (J = 0; J < ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum; J++) {
        sprintf(Str, " %5d, ", ToolInfo.MwWp[J].X);
        AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
        if (J % ToolInfo.Tile.HorGridNum == (ToolInfo.Tile.HorGridNum-1)) {
            AmbaFS_fwrite(&Change_line, 1, 1, Fid);
        }
    }
    sprintf(Str, "%s",  "};");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);

    sprintf(Str, "%s",  "INT16 calib_warp_table_spec vertical [HorGridNum * VerGridNum *2] =");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "%s",  "{");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    for (J = 0; J < ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum; J++) {
        sprintf(Str, " %5d, ", ToolInfo.MwWp[J].Y);
        AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
        if (J % ToolInfo.Tile.HorGridNum == (ToolInfo.Tile.HorGridNum-1)) {
            AmbaFS_fwrite(&Change_line, 1, 1, Fid);
        }
    }
    sprintf(Str, "%s",  "};");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);

    Rval = AmbaFS_fclose(Fid);
    if (Rval != OK) {
        AmbaPrint("AmbaFS_fclose fail.");
        Rval = WARP_CALIB_CLOSE_FILE_ERROR;
        return Rval;
    }
    WarpCalibFn[0] = AmpUT_CalibGetDriverLetter();
    Fid = AmbaFS_fopen(WarpCalibFn, "w");

    if (Fid == NULL) {
        AmbaPrint("AmbaFS_fopen %s fail.", WarpCalibFn);
        Rval = WARP_CALIB_OPEN_FILE_ERROR;
        return Rval;
    }
    //write for calibration data format
    J = 0;
    Tmp = WARP_ENABLE;          AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//WARP_ENABLE
    Tmp = COUNT_WARP_TABLE;     AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//WARP_TABLE_COUNT
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[0]
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[1]
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[2]
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[3]
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[4]
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[5]
    Tmp = 1;                    AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Warp table enable
    Tmp = CAL_WARP_VER;         AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//WARP version
    Tmp = ToolInfo.Tile.HorGridNum;          AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// Horizontal grid number
    Tmp = ToolInfo.Tile.VerGridNum;          AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Vertical grid number
    Tmp = TileWidthExp;         AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// 4:16, 5:32, 6:64, 7:128, 8:256, 9:512
    Tmp = TileHeightExp;        AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// 4:16, 5:32, 6:64, 7:128, 8:256, 9:512
    Tmp = StartX;               AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// StartX
    Tmp = StartY;               AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// StartY
    Tmp = ToolInfo.CAL.ImgWidth;                AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// ImgWidth
    Tmp = ToolInfo.CAL.ImgHeight;           AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// ImgHeight
    Tmp = HSubSampleFactorNum;  AmbaFS_fwrite(&Tmp, 1, 1, Fid);     J+=1;// HSubSampleFactorNum
    Tmp = HSubSampleFactorDen;  AmbaFS_fwrite(&Tmp, 1, 1, Fid);     J+=1;// HSubSampleFactorDen
    Tmp = VSubSampleFactorNum;  AmbaFS_fwrite(&Tmp, 1, 1, Fid);     J+=1;// VSubSampleFactorNum
    Tmp = VSubSampleFactorDen;  AmbaFS_fwrite(&Tmp, 1, 1, Fid);     J+=1;// VSubSampleFactorDen
    Tmp = ToolInfo.Warp2StageFlag;AmbaFS_fwrite(&Tmp, 4, 1, Fid);   J+=4;// ToolInfo.Warp2StageFlag
    Tmp = RESERVED2;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// Reserved2
    Tmp = RESERVED3;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// Reserved3
    Tmp = WarpZoomStep;         AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// WarpZoomStep
    Tmp = WarpChannel;          AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// WarpChannel

    Rval = AmbaFS_fwrite(ToolInfo.MwWp, CAL_WARP_SIZE-(sizeof(Warp_Storage_Header_s)+32), 1, Fid);
    if (Rval == 0) {
        AmbaPrint("AmbaFS_fwrite fail.");
        Rval = WARP_CALIB_CLOSE_FILE_ERROR;
        return Rval;
    }
    Rval = AmbaFS_fclose(Fid);
    if (Rval != OK) {
        AmbaPrint("AmbaFS_fclose fail.");
        Rval = WARP_CALIB_CLOSE_FILE_ERROR;
        return Rval;
    }

    //save the warp calibration data for raw encode
    WarpRawEncFn[0] = AmpUT_CalibGetDriverLetter();
    Fid = AmbaFS_fopen(WarpRawEncFn, "w");
    if (Fid == NULL) {
        AmbaPrint("AmbaFS_fopen %s fail.", WarpRawEncFn);
        Rval = WARP_CALIB_OPEN_FILE_ERROR;
        return Rval;
    }
    Rval = AmbaFS_fwrite(ToolInfo.MwWp, (ToolInfo.Tile.HorGridNum*ToolInfo.Tile.VerGridNum<<2), 1, Fid);
    if (Rval == 0) {
        AmbaPrint("AmbaFS_fwrite fail.");
        Rval = WARP_CALIB_CLOSE_FILE_ERROR;
        return Rval;
    }
    Rval = AmbaFS_fclose(Fid);
    if (Rval != OK) {
        AmbaPrint("AmbaFS_fclose fail.");
        Rval = WARP_CALIB_CLOSE_FILE_ERROR;
        return Rval;
    }
    {

        AMBA_DSP_IMG_MODE_CFG_s Mode;

        memset(&Mode, 0, sizeof(AMBA_DSP_IMG_MODE_CFG_s));

        WarpInfo[0].Version                            = CAL_WARP_VER;
        WarpInfo[0].HorGridNum                         = ToolInfo.Tile.HorGridNum;
        WarpInfo[0].VerGridNum                         = ToolInfo.Tile.VerGridNum;
        WarpInfo[0].TileWidthExp                       = TileWidthExp;
        WarpInfo[0].TileHeightExp                      = TileHeightExp;
        WarpInfo[0].VinSensorGeo.Width                 = ToolInfo.CAL.ImgWidth;
        WarpInfo[0].VinSensorGeo.Height                = ToolInfo.CAL.ImgHeight;
        WarpInfo[0].VinSensorGeo.StartX                = StartX;
        WarpInfo[0].VinSensorGeo.StartY                = StartY;
        WarpInfo[0].VinSensorGeo.HSubSample.FactorDen  = HSubSampleFactorDen;
        WarpInfo[0].VinSensorGeo.HSubSample.FactorNum  = HSubSampleFactorNum;
        WarpInfo[0].VinSensorGeo.VSubSample.FactorDen  = HSubSampleFactorDen;
        WarpInfo[0].VinSensorGeo.VSubSample.FactorNum  = VSubSampleFactorNum;
        WarpInfo[0].Enb2StageCompensation              = ((ToolInfo.Warp2StageFlag == 1) ? Calib2StageCompensation : IK2StageCompensation);
        WarpInfo[0].Reserved1                          = RESERVED2;
        WarpInfo[0].Reserved2                          = RESERVED3;
        WarpInfo[0].pWarp                              = ToolInfo.MwWp;

        AmbaPrint("HSubSampleFactorDen  %d",HSubSampleFactorDen);
        AmbaPrint("HSubSampleFactorNum  %d",HSubSampleFactorNum);
        AmbaPrint("HSubSampleFactorDen  %d",HSubSampleFactorDen);
        AmbaPrint("VSubSampleFactorNum  %d",VSubSampleFactorNum);

        /*set warp table*/
        Mode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
        AmbaDSP_WarpCore_SetCalibWarpInfo(&Mode, &WarpInfo[0]);
        AmbaDSP_WarpCore_CalcDspWarp(&Mode, 0);
        AmbaDSP_WarpCore_SetDspWarp(&Mode);
        /*set the warp info for still mode*/
        Mode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
        AmbaDSP_WarpCore_SetCalibWarpInfo(&Mode, &WarpInfo[0]);

    }

    AmbaKAL_BytePoolFree(TempWpBuffer);
    AmbaKAL_BytePoolFree(TempOldWpBuffer);
    AmbaKAL_BytePoolFree(TempMwWpBuffer);
    AmbaKAL_BytePoolFree(TempRealBuffer);
    AmbaKAL_BytePoolFree(TempExpectBuffer);
    AmbaPrint("Warp_by_spec succeed.");
    return Rval;

}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibWarp_GenTopViewWarpFromSpec
 *
 *  @Description:: to generate the top view warp table
 *
 *  @Input      ::
 *          Argc: the number of the input parameter
 *          Argv: value of input parameter
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK:0 / NG:NG message
\*-----------------------------------------------------------------------------------------------*/


int AmpUTCalibWarp_GenTopViewWarpFromSpec(int Argc, char *Argv[])
{
    INT32 I=0, J =0,  Rval;

    INT32 IdxNumR = 0;
    INT32 IdxNumE = 0;
    double RealWidth = 0, R = 0,Cos = 0, Sin = 0;
    INT32 ExceptW = 0;
    INT32 ExceptH = 0;
    INT16 TileWidthExp  =   7;
    INT16 TileHeightExp  =  7;
    INT32 StartX = 0;
    INT32 StartY = 0;
    Cal_Warp_Tool_Info_s ToolInfo;//sharon
    AMBA_FS_FILE *Fid = 0;
    AMBA_FS_FILE *FpScript;
    UINT32 Tmp;
    INT32 WarpHeader[16] = {0};
    INT32  ArgcR;
    char *ArgvR[MAX_CMD_TOKEN];
    char Buffer[256];
    char Change_line='\n';
    char Str[256];
    INT32 SensorFlag = 0;
    INT32 RealFlag = 0;
    INT32 ExpectFlag = 0;
    INT32 RawPoint0 = 0;
    INT32 RawPoint1 = 0;
    INT32 RawPoint2 = 0;
    INT32 RawPoint3 = 0;
    INT32 TargetPoint0 = 0;
    INT32 TargetPoint1 = 0;
    INT32 TargetPoint2 = 0;
    INT32 TargetPoint3 = 0;
    INT32 CompensateRatioFlag = 0;
    INT32 RatioRFlag = 0;
    INT32 SensorCalFlag = 0;
    INT32 SensorRawFlag = 0;
    Cal_Warp_Table_Info_s Info;
    char TopViewWarpHfileFn [20] = {'c',':','\\','\0'};
    char TopViewWarpCalibFn[30] = {'c',':','\\','\0'};
    char TopViewWarpRawEncFn[30] = {'c',':','\\','\0'};
    char WarpHfileFn [20] = {'c',':','\\','\0'};
    char WarpCalibFn[30] = {'c',':','\\','\0'};
    char WarpRawEncFn[30] = {'c',':','\\','\0'};
    char RawEncFn[30] = {'_','R','a','w','E','n','c','o','d','e','\0'};
    char BinFn[5] = {'.','b','i','n','\0'};
    char HFn[3] = {'.','h','\0'};
    UINT8 WarpMaxIdxNum = 64;
    void *TempWp,*TempOldWp,*TempMwWp, *TempReal,*TempExpect;
    void *TempWpBuffer,*TempMwWpBuffer,*TempOldWpBuffer,*TempRealBuffer,*TempExpectBuffer;
    UINT8 HSubSampleFactorNum=1, HSubSampleFactorDen=1, VSubSampleFactorNum=1, VSubSampleFactorDen=1;
    UINT32 WarpZoomStep = 0,WarpChannel= 0;
    Top_View_Point_s Point;//  1.raw  2. input (dewarp point ) 3. output(top view)
    Top_View_Hmatrix_s Matrix = {0};
    UINT8 DebugFlag = 0;
    AMBA_DSP_CHANNEL_ID_u Chan = { .Bits = { .VinID = 0, .SensorID = 1, .HdrID = 0, } };
    UINT8 *BufferAddress;
    AMBA_SENSOR_MODE_INFO_s VinInfo;
    AMBA_SENSOR_MODE_ID_u SensorMode = {
        .Bits = {
            .Mode = 0,
            .VerticalFlip = 0,
        }
    };

    memset(&Matrix, 0, sizeof(Top_View_Hmatrix_s));
    ToolInfo.IdxNum = 0;
    ToolInfo.Tile.TileWidth   =64;
    ToolInfo.Tile.TileHeight  =64;
    ToolInfo.Tile.HorGridNum =0;
    ToolInfo.Tile.VerGridNum =0;
    ToolInfo.Tile.AddTableGridNumH =0;
    ToolInfo.Tile.AddTableGridNumW =0;
    ToolInfo.CAL.ImgWidth   =4608;
    ToolInfo.CAL.ImgHeight  =3456;
    ToolInfo.RatioR =100;
    ToolInfo.CompensateRatio =100;
    ToolInfo.CompensateRatioSwaX =100;
    ToolInfo.CompensateRatioSwaY =100;
    ToolInfo.Threshold = 0;
    ToolInfo.MwWp = NULL;
    ToolInfo.UnitCellSize = 1;
    AmbaSensor_GetModeInfo(Chan, SensorMode, &VinInfo);

    if (AmpCalib_GetOpticalBlackFlag() == 1) {
        StartX = VinInfo.InputInfo.PhotodiodeArray.StartX - (VinInfo.OutputInfo.RecordingPixels.StartX * VinInfo.InputInfo.HSubsample.FactorDen \
                                                                                /VinInfo.InputInfo.HSubsample.FactorNum);
        StartY = VinInfo.InputInfo.PhotodiodeArray.StartY -  (VinInfo.OutputInfo.RecordingPixels.StartY * VinInfo.InputInfo.VSubsample.FactorDen \
                                                                                /VinInfo.InputInfo.VSubsample.FactorNum);
    } else {
        StartX = VinInfo.InputInfo.PhotodiodeArray.StartX;
        StartY = VinInfo.InputInfo.PhotodiodeArray.StartY;
    }
    AmbaPrint("START_X = %d, START_y= %d",StartX,StartY);


    ToolInfo.Threshold = (double) atof(Argv[3]);
    strcat(TopViewWarpHfileFn, Argv[5]);
    strcat(TopViewWarpHfileFn, HFn);
    strcat(TopViewWarpCalibFn, Argv[5]);
    strcat(TopViewWarpCalibFn, BinFn);
    strcat(TopViewWarpRawEncFn, Argv[5]);
    strcat(TopViewWarpRawEncFn, RawEncFn);
    strcat(TopViewWarpRawEncFn, BinFn);


    strcat(WarpHfileFn, Argv[6]);
    strcat(WarpHfileFn, HFn);
    strcat(WarpCalibFn, Argv[6]);
    strcat(WarpCalibFn, BinFn);
    strcat(WarpRawEncFn, Argv[6]);
    strcat(WarpRawEncFn, RawEncFn);
    strcat(WarpRawEncFn, BinFn);


    //use Big array, release in the end of this function.
    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempReal,&TempRealBuffer, WarpMaxIdxNum*sizeof(double), 32);
    BufferAddress = (UINT8*)((UINT32)TempRealBuffer) ;
    if (Rval != OK) {
        AmbaPrint("allocate fail in %s:%u", __FUNCTION__, __LINE__);
    } else {
        ToolInfo.Real = (double *)BufferAddress;
        AmbaPrint("[AmpUTCalibWarp]rawBuffAddr (0x%08X) (%u)!", ToolInfo.Real, WarpMaxIdxNum*sizeof(double));
    }
    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempExpect, &TempExpectBuffer, WarpMaxIdxNum*sizeof(double),32);
    BufferAddress = (UINT8*)((UINT32)TempExpectBuffer) ;
    if (Rval != OK) {
        AmbaPrint("allocate fail in %s:%u", __FUNCTION__, __LINE__);
    } else {
        ToolInfo.Expect = (double *)BufferAddress;
        AmbaPrint("[AmpUTCalibWarp]rawBuffAddr (0x%08X) (%u)!", ToolInfo.Expect, WarpMaxIdxNum*sizeof(double));
    }


    // Read A line from the file
    AmbaPrint("Argc = %d", Argc);
    if (Argc == 7 ) {}
    else    {
        AmbaPrint(
            "t cal warp top_view: Generate top_view WARP calibration table\n"
            "t cal warp top_view [raw encode text flag] [filename spec] [top view calibration output filename] [main window calibration output filename]\n"
            "[raw encode text flag]: 0 will only generate the calibration binary file; 1 will generate the calibration binary file & rae encode binary file\n"
            "[filename spec]: the path of the script \n");
        return WARP_CALIB_INCORRECT_ARGUMENT_NUM;
    }

    //for generate 1D WARP table   set initial value for Info.OffSensorFlag
    Info.OffSensorFlag = 0;
    FpScript = AmbaFS_fopen(Argv[4], "r");
    if (FpScript == NULL) {
        AmbaPrint("FpScript AmbaFS_fopen %s fail.",Argv[4]);
        Rval = WARP_CALIB_OPEN_FILE_ERROR;
        return Rval;
    }
    while (1) {
        Rval = AmpUT_MultiGetline(FpScript, Buffer);
        if (Rval < WARP_CALIB_OK) {
            break;
        } else {
            ArgcR = 0;
            memset(ArgvR, 0, MAX_CMD_TOKEN*sizeof(char *));
            {
                // Parse the input string to multiple tokens
                char *Token = strtok(Buffer, " ");
                int NumToken = 0;
                while (Token != NULL) {
                    ArgvR[NumToken] = Token;
                    NumToken++;
                    Token = strtok(NULL, " ");
                }
                ArgcR = NumToken;
            }
            if (strcmp(ArgvR[0], "TileWidth") == 0) {
                ToolInfo.Tile.TileWidth = (INT16) atoi(ArgvR[1]);
            } else if (strcmp(ArgvR[0], "TileHeight") == 0) {
                ToolInfo.Tile.TileHeight = (INT16) atoi(ArgvR[1]);
            } else if (strcmp(ArgvR[0], "target_point0") == 0) {
                TargetPoint0 = 1;
                Point.Output[0].X = (double) atoi(ArgvR[1]);
                Point.Output[0].Y = (double) atoi(ArgvR[2]);
            } else if (strcmp(ArgvR[0], "target_point1") == 0) {
                TargetPoint1 = 1;
                Point.Output[1].X = (double) atoi(ArgvR[1]);
                Point.Output[1].Y = (double) atoi(ArgvR[2]);
            } else if (strcmp(ArgvR[0], "target_point2") == 0) {
                TargetPoint2 = 1;
                Point.Output[2].X = (double) atoi(ArgvR[1]);
                Point.Output[2].Y = (double) atoi(ArgvR[2]);
            } else if (strcmp(ArgvR[0], "target_point3") == 0) {
                TargetPoint3 = 1;
                Point.Output[3].X = (double) atoi(ArgvR[1]);
                Point.Output[3].Y = (double) atoi(ArgvR[2]);
            } else if (strcmp(ArgvR[0], "raw_point0") == 0) {
                RawPoint0 = 1;
                Point.Raw[0].X = (double) atoi(ArgvR[1]);
                Point.Raw[0].Y = (double) atoi(ArgvR[2]);
            } else if (strcmp(ArgvR[0], "raw_point1") == 0) {
                RawPoint1 = 1;
                Point.Raw[1].X = (double) atoi(ArgvR[1]);
                Point.Raw[1].Y = (double) atoi(ArgvR[2]);
            } else if (strcmp(ArgvR[0], "raw_point2") == 0) {
                RawPoint2 = 1;
                Point.Raw[2].X = (double) atoi(ArgvR[1]);
                Point.Raw[2].Y = (double) atoi(ArgvR[2]);
            } else if (strcmp(ArgvR[0], "raw_point3") == 0) {
                RawPoint3 = 1;
                Point.Raw[3].X = (double) atoi(ArgvR[1]);
                Point.Raw[3].Y = (double) atoi(ArgvR[2]);
            } else if (strcmp(ArgvR[0], "debug") == 0) {
                DebugFlag = 1;
            } else if (strcmp(ArgvR[0], "off_sensor_calibration") == 0) {
                SensorCalFlag = 1;
                Info.OffSensorFlag = 1;
                ToolInfo.CAL.ImgWidth = (INT32) atoi(ArgvR[1]);
                ToolInfo.CAL.ImgHeight = (INT32) atoi(ArgvR[2]);
                RealWidth = (double) atof(ArgvR[3]);
                StartX = (INT32) atoi(ArgvR[4]);
                StartY = (INT32) atoi(ArgvR[5]);
                if (ArgcR == 8) {
                    ToolInfo.CAL.CenterX = (INT32) atoi(ArgvR[6]);
                    ToolInfo.CAL.CenterY = (INT32) atoi(ArgvR[7]);
                    if (ToolInfo.CAL.CenterX < (ToolInfo.CAL.ImgWidth*3>>3) || ToolInfo.CAL.CenterX > (ToolInfo.CAL.ImgWidth*5>>3)) {
                        SensorFlag = 0;
                        AmbaPrint("Error : CenterX out of the range ((ImgWidth*3/8) < CenterX < (ImgWidth*5/8))");
                    }
                    if (ToolInfo.CAL.CenterY < (ToolInfo.CAL.ImgHeight*3>>3)  || ToolInfo.CAL.CenterY > (ToolInfo.CAL.ImgHeight*5>>3)) {
                        SensorFlag = 0;
                        AmbaPrint("Error : CenterY out of the range ((ImgHeight*3/8) < CenterY < (ImgHeight*5/8))");
                    }
                } else {
                    ToolInfo.CAL.CenterX = ToolInfo.CAL.ImgWidth>>1;
                    ToolInfo.CAL.CenterY = ToolInfo.CAL.ImgHeight>>1;
                }
                ToolInfo.UnitCellSize = RealWidth / ToolInfo.CAL.ImgWidth;
                ToolInfo.Tile.HorGridNum = (ToolInfo.CAL.ImgWidth+ToolInfo.Tile.TileWidth-1)/ToolInfo.Tile.TileWidth + 1 ;
                ToolInfo.Tile.VerGridNum = (ToolInfo.CAL.ImgHeight+ToolInfo.Tile.TileHeight-1)/ToolInfo.Tile.TileHeight + 1 ;
            } else if (strcmp(ArgvR[0], "off_sensor_raw_image") == 0) {
                SensorRawFlag = 1;
                Info.OffSensorFlag = 1;
                Info.ImgW = (INT32) atoi(ArgvR[1]);
                Info.ImgH = (INT32) atoi(ArgvR[2]);
                Info.StartX = (INT32) atoi(ArgvR[3]);
                Info.StartY = (INT32) atoi(ArgvR[4]);

            } else if (strcmp(ArgvR[0], "sensor") == 0) {
                SensorFlag = 1;
                Info.ImgW = ToolInfo.CAL.ImgWidth = (INT32) atoi(ArgvR[1]);
                Info.ImgH = ToolInfo.CAL.ImgHeight = (INT32) atoi(ArgvR[2]);
                RealWidth = (double) atof(ArgvR[3]);
                if (ArgcR == 6) {
                    ToolInfo.CAL.CenterX = (INT32) atoi(ArgvR[4]);
                    ToolInfo.CAL.CenterY = (INT32) atoi(ArgvR[5]);
                    AmbaPrint("center x = %d center y = %d",ToolInfo.CAL.CenterX,ToolInfo.CAL.CenterY);
                    if (ToolInfo.CAL.CenterX < (ToolInfo.CAL.ImgWidth*3>>3) || ToolInfo.CAL.CenterX > (ToolInfo.CAL.ImgWidth*5>>3)) {
                        SensorFlag = 0;
                        AmbaPrint("Error : CenterX out of the range ((ImgWidth*3/8) < CenterX < (ImgWidth*5/8))");
                    }
                    if (ToolInfo.CAL.CenterY < (ToolInfo.CAL.ImgHeight*3>>3)  || ToolInfo.CAL.CenterY > (ToolInfo.CAL.ImgHeight*5>>3)) {
                        SensorFlag = 0;
                        AmbaPrint("Error : CenterY out of the range ((ImgHeight*3/8) < CenterY < (ImgHeight*5/8))");
                    }
                } else {
                    ToolInfo.CAL.CenterX = ToolInfo.CAL.ImgWidth>>1;
                    ToolInfo.CAL.CenterY = ToolInfo.CAL.ImgHeight>>1;
                }
                ToolInfo.UnitCellSize = RealWidth / ToolInfo.CAL.ImgWidth;
                ToolInfo.Tile.HorGridNum = (ToolInfo.CAL.ImgWidth+ToolInfo.Tile.TileWidth-1)/ToolInfo.Tile.TileWidth + 1 ;
                ToolInfo.Tile.VerGridNum = (ToolInfo.CAL.ImgHeight+ToolInfo.Tile.TileHeight-1)/ToolInfo.Tile.TileHeight + 1 ;
                AmbaPrint("cell size = %f HorGridNum = %d VerGridNum = %d",ToolInfo.UnitCellSize,ToolInfo.Tile.HorGridNum,ToolInfo.Tile.VerGridNum);
                ToolInfo.Tile.AddTableGridNumW =0;
                ToolInfo.Tile.AddTableGridNumH = 0;

            } else if (strcmp(ArgvR[0], "real") == 0) {
                IdxNumR = ArgcR - 1;
                for ( I=1; I<ArgcR; I++) {
                    RealFlag = 1;
                    ToolInfo.Real[I-1] = (double) atof(ArgvR[I]);
                }
            } else if (strcmp(ArgvR[0], "expect") == 0) {
                IdxNumE = ArgcR - 1;
                for ( I=1; I<ArgcR; I++) {
                    ExpectFlag = 1;
                    ToolInfo.Expect[I-1] = (double) atof(ArgvR[I]);
                }
            } else if (strcmp(ArgvR[0], "Compensate_ratio") == 0) {
                ToolInfo.CompensateRatio = atoi(ArgvR[1]);
                if (ToolInfo.CompensateRatio<0 || ToolInfo.CompensateRatio>200) {
                    AmbaPrint("Compensate_ratio must have be 0~200.");
                } else {
                    CompensateRatioFlag = 1;
                }
            } else if (strcmp(ArgvR[0], "Compensate_ratio_swa_y") == 0) {
                ToolInfo.CompensateRatioSwaY = atoi(ArgvR[1]);
                AmbaPrint("Compensate_ratio_swa_y %d",ToolInfo.CompensateRatioSwaY);
                if (ToolInfo.CompensateRatioSwaY<0 || ToolInfo.CompensateRatioSwaY>200) {
                    AmbaPrint("Compensate_ratio_swa_y must have be 0~200.");
                }
            } else if (strcmp(ArgvR[0], "Compensate_ratio_swa_x") == 0) {
                ToolInfo.CompensateRatioSwaX = atoi(ArgvR[1]);
                AmbaPrint("Compensate_ratio_swa_x %d",ToolInfo.CompensateRatioSwaX);
                if (ToolInfo.CompensateRatioSwaX<0 || ToolInfo.CompensateRatioSwaX>200) {
                    AmbaPrint("Compensate_ratio_swa_x must have be 0~200.");
                }
            } else if (strcmp(ArgvR[0], "r_ratio") == 0) {
                ToolInfo.RatioR = atoi(ArgvR[1]);
                if (ToolInfo.RatioR<=0 || ToolInfo.RatioR>200) {
                    AmbaPrint("r_ratio must have be 1~200.");
                } else {
                    RatioRFlag = 1;
                }
            } else if (strcmp(ArgvR[0], "zoom_step") == 0) {
                WarpZoomStep = atoi(ArgvR[1]);
            }
        }
    }



    Rval = AmbaFS_fclose(FpScript);
    if (Rval != OK) {
        AmbaPrint("FpScript AmbaFS_fclose fail .");
        Rval = WARP_CALIB_CLOSE_FILE_ERROR;
        return Rval;
    }
    if (Info.OffSensorFlag ) {
        SensorFlag = (SensorRawFlag & SensorCalFlag);
    }
    if ((SensorFlag == 0)||(RealFlag == 0)||(ExpectFlag == 0)||(CompensateRatioFlag == 0)||(RatioRFlag == 0)) {
        if (SensorFlag == 0)            AmbaPrint(" (off) Sensor parameter not Found or some error");
        if (RealFlag == 0)              AmbaPrint("Real warp spec parameter not Found or some error");
        if (ExpectFlag == 0)            AmbaPrint("Expect warp spec parameter not Found or some error");
        if (CompensateRatioFlag == 0)   AmbaPrint("CompensateRatio parameter not Found or some error");
        if (RatioRFlag == 0)            AmbaPrint("R_ratio parameter not Found or some error");
        Rval = WARP_CALIB_INCORRECT_PARAMETER;
        return Rval;
    }
    if ((TargetPoint0 == 0)||(TargetPoint1 == 0)||(TargetPoint2 == 0)||(TargetPoint3 == 0)||\
            (RawPoint0 == 0)||(RawPoint1 == 0)||(RawPoint2 == 0)||(RawPoint3 == 0)) {
        AmbaPrint(" ERROR! wrong raw/top points ");
        Rval = WARP_CALIB_INCORRECT_PARAMETER;
        return Rval;
    }
    if (IdxNumE != IdxNumR) {
        AmbaPrint("Real spec Index num (%d) != Expect spec Index num (%d)",IdxNumR,IdxNumE);
        Rval = WARP_CALIB_INCORRECT_INPUT_NUM;
        return Rval;
    }
    if ((IdxNumR > WarpMaxIdxNum) || (IdxNumE > WarpMaxIdxNum)) {
        AmbaPrint("the input real/expect parameters should be smaller than %d",WarpMaxIdxNum);
        Rval = WARP_CALIB_TOO_MUCH_INPUT_NUM;
        return Rval;
    }
    ToolInfo.IdxNum = IdxNumE;
    for (I = 0; I < ToolInfo.IdxNum; I++) {
        ToolInfo.Real[I] = ToolInfo.Real[I]/ToolInfo.UnitCellSize; //in pixel
        ToolInfo.Expect[I] = ToolInfo.Expect[I]/ToolInfo.UnitCellSize;//in pixel
    }
    Point.Output[0].X = Point.Output[0].X*ToolInfo.UnitCellSize;
    Point.Output[0].Y = Point.Output[0].Y*ToolInfo.UnitCellSize;
    Point.Output[1].X = Point.Output[1].X*ToolInfo.UnitCellSize;
    Point.Output[1].Y = Point.Output[1].Y*ToolInfo.UnitCellSize;
    Point.Output[2].X = Point.Output[2].X*ToolInfo.UnitCellSize;
    Point.Output[2].Y = Point.Output[2].Y*ToolInfo.UnitCellSize;
    Point.Output[3].X = Point.Output[3].X*ToolInfo.UnitCellSize;
    Point.Output[3].Y = Point.Output[3].Y*ToolInfo.UnitCellSize;

    //use Big array, release in the end of this function.
    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempMwWp, &TempMwWpBuffer, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(AMBA_DSP_IMG_GRID_POINT_s), 32);
    BufferAddress = (UINT8*)((UINT32)TempMwWp) ;
    if (Rval != OK) {
        AmbaPrint("ToolInfo.MwWp allocate fail in %s:%u", __FUNCTION__, __LINE__);
		return -1;
    } else {
        ToolInfo.MwWp = (AMBA_DSP_IMG_GRID_POINT_s *)BufferAddress;
        AmbaPrint("[AmpUTCalibWarp]rawBuffAddr (0x%08X) (%u)!", ToolInfo.MwWp, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(AMBA_DSP_IMG_GRID_POINT_s));
    }

    R = pow((ToolInfo.CAL.ImgWidth*ToolInfo.CAL.ImgWidth + ToolInfo.CAL.ImgHeight*ToolInfo.CAL.ImgHeight), 0.5);
    if (R ==0 ) {
        AmbaPrint("[AppLibCalibWarp] ERROR! div 0 ");
        Rval = WARP_CALIB_DIV0;
        return Rval;
    }
    Cos = ToolInfo.CAL.ImgWidth/R;
    Sin = ToolInfo.CAL.ImgHeight/R;
    ExceptW = (UINT32)(ToolInfo.Expect[ToolInfo.IdxNum-1] * Cos);
    ExceptH = (UINT32)(ToolInfo.Expect[ToolInfo.IdxNum-1] * Sin);

    ToolInfo.Tile.AddTableGridNumW =((ExceptW>>1)/ToolInfo.Tile.TileWidth);
    ToolInfo.Tile.AddTableGridNumH =((ExceptH>>1)/ToolInfo.Tile.TileHeight);
    ToolInfo.Tile.HorGridNum += (ToolInfo.Tile.AddTableGridNumW )<<1;
    ToolInfo.Tile.VerGridNum  += (ToolInfo.Tile.AddTableGridNumH )<<1;
    AmbaPrint(" AddTableGridNumW  %d, AddTableGridNumH  %d",ToolInfo.Tile.AddTableGridNumW,ToolInfo.Tile.AddTableGridNumH);
    AmbaPrint(" HorGridNum  %d, VerGridNum  %d",ToolInfo.Tile.HorGridNum,ToolInfo.Tile.VerGridNum);


    //use Big array, release in the end of this function.
    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempWp, &TempWpBuffer, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(Warp_TOOL_IMG_GRID_POINT_s), 32);
    BufferAddress = (UINT8*)((UINT32)TempWp) ;
    if (Rval != OK) {
        AmbaPrint("ToolInfo.Wp allocate fail in %s:%u", __FUNCTION__, __LINE__);
    } else {
        ToolInfo.Wp = (Warp_TOOL_IMG_GRID_POINT_s *)BufferAddress;
        AmbaPrint("[AmpUTCalibWarp]rawBuffAddr (0x%08X) (%u)!", ToolInfo.Wp, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(Warp_TOOL_IMG_GRID_POINT_s));
    }
    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempOldWp, &TempOldWpBuffer, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(Warp_TOOL_IMG_GRID_POINT_s), 32);
    BufferAddress = (UINT8*)((UINT32)TempOldWp) ;
    if (Rval != OK) {
        AmbaPrint("ToolInfo.OldWp allocate fail in %s:%u", __FUNCTION__, __LINE__);
    } else {
        ToolInfo.OldWp = (Warp_TOOL_IMG_GRID_POINT_s *)BufferAddress;
        AmbaPrint("[AppLibCalibWarp]rawBuffAddr (0x%08X) (%u)!", ToolInfo.OldWp, ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum*sizeof(Warp_TOOL_IMG_GRID_POINT_s));
    }

    Rval = AmbaCalib_CalTopViewTable(&ToolInfo, &Point, Matrix, DebugFlag, 0); //Chester: hack ChId as 0
    if (Rval != OK) {
        AmbaPrint("[AppLibCalibWarp]Error Rval: NG!");
        Rval = WARP_CALIB_DIV0;
        return Rval;
    }
    TopViewWarpHfileFn[0] = AmpUT_CalibGetDriverLetter();
    Fid = AmbaFS_fopen(TopViewWarpHfileFn, "w");
    if (Fid == NULL) {
        AmbaPrint("AmbaFS_fopen fail .");
        Rval = WARP_CALIB_OPEN_FILE_ERROR;
        return Rval;
    }


    //TileWidthExp;   /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    //TileWidthExp;   /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    switch (ToolInfo.Tile.TileWidth) {
        case 16:
            TileWidthExp = 4;
            break;
        case 32:
            TileWidthExp = 5;
            break;
        case 64:
            TileWidthExp = 6;
            break;
        case 128:
            TileWidthExp = 7;
            break;
        case 256:
            TileWidthExp = 8;
            break;
        case 512:
            TileWidthExp = 9;
            break;
        default:
            TileWidthExp = 7;//set default value to 7;
    }

    switch (ToolInfo.Tile.TileHeight) {
        case 16:
            TileHeightExp = 4;
            break;
        case 32:
            TileHeightExp = 5;
            break;
        case 64:
            TileHeightExp = 6;
            break;
        case 128:
            TileHeightExp = 7;
            break;
        case 256:
            TileHeightExp = 8;
            break;
        case 512:
            TileHeightExp = 9;
            break;
        default:
            TileHeightExp = 7;//set default value to 7;
    }

    WarpHeader[0]   = CAL_WARP_VER;
    WarpHeader[1]   = ToolInfo.Tile.HorGridNum;
    WarpHeader[2]   = ToolInfo.Tile.VerGridNum;
    WarpHeader[3]   = TileWidthExp;
    WarpHeader[4]   = TileHeightExp;
    WarpHeader[5]   = StartX;
    WarpHeader[6]   = StartY;
    WarpHeader[7]   = ToolInfo.CAL.ImgWidth;
    WarpHeader[8]   = ToolInfo.CAL.ImgHeight;
    WarpHeader[9]   = HSubSampleFactorNum;
    WarpHeader[10]  = HSubSampleFactorDen;
    WarpHeader[11]  = VSubSampleFactorNum;
    WarpHeader[12]  = VSubSampleFactorDen;
    WarpHeader[13]  = ToolInfo.Warp2StageFlag;
    WarpHeader[14]  = RESERVED2;
    WarpHeader[15]  = RESERVED3;
    //write warp header into file
    sprintf(Str, "%s",  "#define HorGridNum ");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    sprintf(Str, " %5d ",  WarpHeader[1]);
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "%s",  "#define VerGridNum ");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    sprintf(Str, " %5d ",  WarpHeader[2]);
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "%s",  "INT32 calib_warp_header_spec[64] =");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "%s",  "{");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "0x%X,",  CAL_WARP_VER);
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    for (I=1; I<16; I++) {
        sprintf(Str, " %5d, ", WarpHeader[I] );
        AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
        AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    }
    sprintf(Str, "%s",  "};");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);

    //write warp table into file
    sprintf(Str, "%s",  "INT16 calib_warp_table_spec horizontal [HorGridNum * VerGridNum *2] =");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "%s",  "{");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    for (J = 0; J < ToolInfo.Tile.HorGridNum *ToolInfo.Tile.VerGridNum; J++) {
        sprintf(Str, " %5d, ", ToolInfo.MwWp[J].X);
        AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
        if (J % ToolInfo.Tile.HorGridNum == (ToolInfo.Tile.HorGridNum-1)) {
            AmbaFS_fwrite(&Change_line, 1, 1, Fid);
        }
    }
    sprintf(Str, "%s",  "};");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);

    sprintf(Str, "%s",  "INT16 calib_warp_table_spec vertical [HorGridNum * VerGridNum *2] =");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    sprintf(Str, "%s",  "{");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);
    for (J = 0; J < ToolInfo.Tile.HorGridNum * ToolInfo.Tile.VerGridNum; J++) {
        sprintf(Str, " %5d, ", ToolInfo.MwWp[J].Y);
        AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
        if (J % ToolInfo.Tile.HorGridNum == (ToolInfo.Tile.HorGridNum-1)) {
            AmbaFS_fwrite(&Change_line, 1, 1, Fid);
        }
    }
    sprintf(Str, "%s",  "};");
    AmbaFS_fwrite(Str, strlen(Str), 1, Fid);
    AmbaFS_fwrite(&Change_line, 1, 1, Fid);

    Rval = AmbaFS_fclose(Fid);

    if (Rval != OK) {
        AmbaPrint("AmbaFS_fclose fail.");
        Rval = WARP_CALIB_CLOSE_FILE_ERROR;
        return Rval;
    }

    TopViewWarpCalibFn[0] = AmpUT_CalibGetDriverLetter();
    Fid = AmbaFS_fopen(TopViewWarpCalibFn, "w");

    if (Fid == NULL) {
        AmbaPrint("AmbaFS_fopen %s fail.", TopViewWarpCalibFn);
        Rval = WARP_CALIB_OPEN_FILE_ERROR;
        return Rval;
    }
    //write for calibration data format
    J = 0;
    Tmp = WARP_ENABLE;          AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//WARP_ENABLE
    Tmp = COUNT_WARP_TABLE;     AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//WARP_TABLE_COUNT
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[0]
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[1]
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[2]
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[3]
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[4]
    Tmp = RESERVED1;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Reserve[5]
    Tmp = 1;                    AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Warp table enable
    Tmp = CAL_WARP_VER;         AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//WARP version
    Tmp = ToolInfo.Tile.HorGridNum;          AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// Horizontal grid number
    Tmp = ToolInfo.Tile.VerGridNum;          AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;//Vertical grid number
    Tmp = TileWidthExp;         AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// 4:16, 5:32, 6:64, 7:128, 8:256, 9:512
    Tmp = TileHeightExp;        AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// 4:16, 5:32, 6:64, 7:128, 8:256, 9:512
    Tmp = StartX;               AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// StartX
    Tmp = StartY;               AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// StartY
    Tmp = ToolInfo.CAL.ImgWidth;                AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// ImgWidth
    Tmp = ToolInfo.CAL.ImgHeight;           AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// ImgHeight
    Tmp = HSubSampleFactorNum;  AmbaFS_fwrite(&Tmp, 1, 1, Fid);     J+=1;// HSubSampleFactorNum
    Tmp = HSubSampleFactorDen;  AmbaFS_fwrite(&Tmp, 1, 1, Fid);     J+=1;// HSubSampleFactorDen
    Tmp = VSubSampleFactorNum;  AmbaFS_fwrite(&Tmp, 1, 1, Fid);     J+=1;// VSubSampleFactorNum
    Tmp = VSubSampleFactorDen;  AmbaFS_fwrite(&Tmp, 1, 1, Fid);     J+=1;// VSubSampleFactorDen
    Tmp = ToolInfo.Warp2StageFlag;AmbaFS_fwrite(&Tmp, 4, 1, Fid);   J+=4;// ToolInfo.Warp2StageFlag
    Tmp = RESERVED2;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// Reserved2
    Tmp = RESERVED3;            AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// Reserved3
    Tmp = WarpZoomStep;         AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// WarpZoomStep
    Tmp = WarpChannel;          AmbaFS_fwrite(&Tmp, 4, 1, Fid);     J+=4;// WarpChannel


    Rval = AmbaFS_fwrite(ToolInfo.MwWp, CAL_WARP_SIZE-(sizeof(Warp_Storage_Header_s)+32), 1, Fid);
    if (Rval == 0) {
        Rval = WARP_CALIB_CLOSE_FILE_ERROR;
        return Rval;
    }
    Rval = AmbaFS_fclose(Fid);
    if (Rval != OK) {
        Rval = WARP_CALIB_CLOSE_FILE_ERROR;
        return Rval;
    }

    //save the warp calibration data for raw encode
    TopViewWarpRawEncFn[0] = AmpUT_CalibGetDriverLetter();
    Fid = AmbaFS_fopen(TopViewWarpRawEncFn, "w");

    if (Fid == NULL) {
        AmbaPrint("AmbaFS_fopen %s fail.", TopViewWarpRawEncFn);
        Rval = WARP_CALIB_OPEN_FILE_ERROR;
        return Rval;
    }
    AmbaPrint("sizeof(ToolInfo.MwWp  )   %d  sizeof (ToolInfo.HorGridNum*ToolInfo.VerGridNum<<2)  %d",sizeof(ToolInfo.MwWp),sizeof (ToolInfo.Tile.HorGridNum*ToolInfo.Tile.VerGridNum<<2));
    Rval = AmbaFS_fwrite(ToolInfo.MwWp, (ToolInfo.Tile.HorGridNum*ToolInfo.Tile.VerGridNum<<2), 1, Fid);
    if (Rval == 0) {
        AmbaPrint("AmbaFS_fwrite fail.");
        Rval = WARP_CALIB_CLOSE_FILE_ERROR;
        return Rval;
    }
    Rval = AmbaFS_fclose(Fid);
    if (Rval != OK) {
        AmbaPrint("AmbaFS_fclose fail.");
        Rval = WARP_CALIB_CLOSE_FILE_ERROR;
        return Rval;
    }

    AmbaKAL_BytePoolFree(TempWpBuffer);
    AmbaKAL_BytePoolFree(TempOldWpBuffer);
    AmbaKAL_BytePoolFree(TempMwWpBuffer);
    AmbaKAL_BytePoolFree(TempRealBuffer);
    AmbaKAL_BytePoolFree(TempExpectBuffer);
    AmbaPrint("Warp_by_spec succeed.");
    return Rval;

}

int AmpUTCalibWarp_WarpMergeBinFile(int Argc, char *Argv[])
{
    int i;

    AMBA_FS_FILE *Fp = 0,*Fid = 0;
    char WarpFileName[30] = {'c',':','\\','c','a','l','i','b','6','.','b','i','n','\0'};
    char CaFileName[30] = {'c',':','\\','c','a','l','i','b','1','3','.','b','i','n','\0'};
    char AscFname[64];
    UINT32 Tmp = 0;
    UINT32 NvdSize = 0;
    UINT32 Offset;
    UINT8 *Buffer;
    UINT8 Reserved = 0;
    UINT32 TableCount;
    INT32 Rval;
    void *BufferAddress,*BufferAddressRel;


    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &BufferAddress, &BufferAddressRel, sizeof(Warp_Storage_s), 32);
    Buffer = (UINT8*)((UINT32)BufferAddress) ;
    if (Rval != OK) {
        AmbaPrint("allocate fail in %s:%u", __FUNCTION__, __LINE__);
        return Rval;
    } else {
        AmbaPrint("[AmpUTCalibWarp]Buffer (0x%08X) (%d)!", Buffer, (sizeof(Warp_Storage_s)+32));
    }

    if (strcmp(Argv[3], "warp") == 0) {
        WarpFileName[0] = AmpUT_CalibGetDriverLetter();
        Fp = AmbaFS_fopen(WarpFileName, "w");

        if (Fp == NULL) {
            AmbaPrint("AmbaFS_fopen %s fail.", WarpFileName);
            Rval = WARP_CALIB_OPEN_FILE_ERROR;
            return Rval;
        }
    } else if (strcmp(Argv[3], "ca") == 0) {
        CaFileName[0] = AmpUT_CalibGetDriverLetter();
        Fp = AmbaFS_fopen(CaFileName, "w");

        if (Fp == NULL) {
            AmbaPrint("AmbaFS_fopen %s fail.", CaFileName);
            Rval = WARP_CALIB_OPEN_FILE_ERROR;
            return Rval;
        }
    } else {
        AmbaPrintColor(RED,"unknow format");
        return -1;
    }
    if (Fp == NULL) {
        AmbaPrint("Can't open the file %s\n", Argv[3]);
        return -1;
    }

    if (Argc >= 5) {
        if (strcmp(Argv[4], "repeat") == 0) {
            AmbaPrintColor(RED,"repeat");
            Tmp = 1;
            AmbaFS_fwrite(&Tmp,4,1,Fp);//enable
            TableCount = Tmp = atoi(Argv[5]);
            AmbaFS_fwrite(&Tmp,4,1,Fp);//table count
            Reserved = 0;
            AmbaFS_fwrite(&Reserved,1,24,Fp);//Reserved

            Fid = AmbaFS_fopen(Argv[6], "r");
            if (Fid == NULL) {
                AmbaPrint("Can't open the file %s\n", Argv[6]);
                return -1;
            }
            AmbaFS_fseek(Fid, 32, AMBA_FS_SEEK_START);// 32 bytes
            AmbaFS_fread(Buffer,sizeof(Warp_Storage_s),1,Fid);
            AmbaFS_fclose(Fid);

            for (i = 0; i < TableCount; i++) {
                AmbaFS_fwrite(Buffer,sizeof(Warp_Storage_s),1,Fp);

            }
            //fill zero to match the WARP size.
            Offset = TableCount*sizeof(Warp_Storage_s)+32;

            if (strcmp(Argv[3], "warp") == 0) {
                NvdSize = CAL_WARP_SIZE;
            } else if (strcmp(Argv[3], "ca") == 0) {
                NvdSize = CAL_CA_SIZE;
            }
            AmbaPrintColor(BLUE,"Offset = %d NvdSize = %d",Offset,NvdSize);
            Reserved = 0;
            AmbaFS_fwrite(&Reserved,1,(NvdSize-Offset),Fp);
            Rval = 0;
        } else if (strcmp(Argv[4], "merge") == 0) {
            AmbaPrintColor(RED,"merge");
            Tmp = 1;
            AmbaFS_fwrite(&Tmp,4,1,Fp); //enable
            TableCount = atoi(Argv[6]);
            AmbaFS_fwrite(&TableCount,4,1,Fp);  //table count
            Reserved = 0;
            AmbaFS_fwrite(&Reserved,1,24,Fp);//Reserved

            for (i = 0; i < TableCount; i++) {
                sprintf(AscFname, "%s%d.bin", Argv[5], i);

                Fid = AmbaFS_fopen(AscFname, "r");
                if (Fid == NULL) {
                    AmbaPrint("Can't open the file %s\n", AscFname);
                    return -1;
                }
                AmbaFS_fseek(Fid, 32, AMBA_FS_SEEK_START);
                AmbaFS_fread(Buffer,sizeof(Warp_Storage_s),1,Fid);
                AmbaFS_fwrite(Buffer,sizeof(Warp_Storage_s),1,Fp);
                AmbaFS_fclose(Fid);
            }
            //fill zero to match the WARP size.
            Offset = TableCount*sizeof(Warp_Storage_s)+32;

            if (strcmp(Argv[3], "warp") == 0) {
                NvdSize = CAL_WARP_SIZE;
            } else if (strcmp(Argv[3], "ca") == 0) {
                NvdSize = CAL_CA_SIZE;
            }
            Reserved = 0;
            AmbaFS_fwrite(&Reserved,1,(NvdSize-Offset),Fp);

            Rval = 0;
        } else {
            AmbaPrintColor(RED,"else");
            Tmp = 1;
            AmbaFS_fwrite(&Tmp,4,1,Fp);//enable
            Tmp = Argc-4;
            AmbaFS_fwrite(&Tmp,4,1,Fp);//table count
            Reserved = 0;
            AmbaFS_fwrite(&Reserved,1,24,Fp);//Reserved

            for (i = 0; i <= (Argc-5); i++) {

                //read warp data from the file to Buf
                Fid = AmbaFS_fopen(Argv[4+i], "r");
                if (Fid == NULL) {
                    AmbaPrint("Can't open the file %s\n", Argv[4+i]);
                    return -1;
                }
                AmbaFS_fseek(Fid, 32, AMBA_FS_SEEK_START);
                AmbaFS_fread(Buffer,sizeof(Warp_Storage_s),1,Fid);
                AmbaFS_fwrite(Buffer,sizeof(Warp_Storage_s),1,Fp);
                AmbaFS_fclose(Fid);
            }
            //fill zero to match the WARP size.
            Offset = (Argc-4)*sizeof(Warp_Storage_s)+8;

            if (strcmp(Argv[3], "warp") == 0) {
                NvdSize = CAL_WARP_SIZE;
            } else if (strcmp(Argv[3], "ca") == 0) {
                NvdSize = CAL_CA_SIZE;
            }
            Reserved = 0;
            AmbaFS_fwrite(&Reserved,1,(NvdSize-Offset),Fp);
            Rval = 0;
        }
    } else {
        Rval = -1;
    }

    AmbaKAL_BytePoolFree(BufferAddressRel);//release memory
    AmbaFS_fclose(Fp);
    if (Rval == -1) {
        AmbaPrint("t cal warp merge_bin [ca/warp] [filename] [filename] [filename] ....  : combine ca/warp binary for amba calibration"
                  "t cal warp merge_bin [ca/warp] merge [filename] [num]: the file name is warp0.bin warp1.bin .... warp7.bin [filename]= driver:warp [num]= 8. "
                 );
    }

    return Rval;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibWarp_Upgrade
 *
 *  @Description:: the upgrade function for warp calibration
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
int AmpUTCalibWarp_Upgrade(Cal_Obj_s *CalObj, Cal_Stie_Status_s* CalSite)
{
    if (CalObj->Version != CalSite->Version) {
        // This API is an example to handle calibration data upgrade
        AmbaPrint("[CAL] Site %s Version mismatch (FW:0x%08X, NAND:0x%08X)", CalObj->Name, CalObj->Version, CalSite->Version);
    }
    // The default behavior is to do-nothing when Version mismatch
    return 0;

}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibWarp_FUNC
 *
 *  @Description:: the entry function for Warp calibration function
 *
 *  @Input      ::
 *          Argc: input number for WB calibration
 *          Argv: input value for WB calibration
 *          CalSite: calibration site status
 *          CalObj: calibration object
 *
 *  @Output     ::
 *          OutputStr: output debug message
 *  @Return     ::
 *          INT: OK:0/NG:error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibWarp_Func(int Argc, char *Argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, Cal_Obj_s *CalObj)
{
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTWarp_UTFunc
 *
 *  @Description:: the unit test function for warp calibration
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
int AmpUTWarp_UTFunc(int Argc, char *Argv[])
{
    Cal_Obj_s           *CalObj;
    int Rval = -1;

    CalObj = AmpUT_CalibGetObj(CAL_WARP_ID);
    if ((strcmp(Argv[2], "test") == 0)) {
        //register calibration site
        AmpUT_CalibSiteInit();
        Rval = 0;
    } else if ((strcmp(Argv[2], "reset") == 0)) {
        AmpUT_CalibMemReset(CAL_WARP_ID);
        Rval = 0;
    } else if ((strcmp(Argv[2], "init") == 0)) {
        AmpUTCalibWarp_Init(CalObj);
        Rval = 0;
    } else if ((strcmp(Argv[2], "warp_spec") == 0)) {
        AmpUTCalibWarp_GenWarpFromSpec(Argc,Argv);
        Rval = 0;
    } else if ((strcmp(Argv[2], "top_view") == 0)) {
        AmpUTCalibWarp_GenTopViewWarpFromSpec(Argc,Argv);
        Rval = 0;
    } else if ((strcmp(Argv[2], "select") == 0)) {
        UINT8 Enable      = atoi(Argv[3]);
        UINT8 Channel     = atoi(Argv[4]);
        UINT8 Id1         = atoi(Argv[5]);
        UINT8 Id2         = atoi(Argv[6]);
        int BlendRatio    = atoi(Argv[7]);
        int Strength    = atoi(Argv[8]);
        Rval = AmpUTCalibWarp_SelectWarpTable(Enable, Channel, Id1, Id2, BlendRatio, Strength);
    } else if ((strcmp(Argv[2], "merge_bin") == 0)) {
        AmpUTCalibWarp_WarpMergeBinFile(Argc,Argv);
    } else if((strcmp(Argv[2], "debug") == 0)){
        int Enable = atoi(Argv[3]);
        AmpUTCalibWarp_DebugEnable(Enable);
        Rval = 0;
    } else if((strcmp(Argv[2], "info") == 0)){
        int i;

        if (AppWarpControl.Enable) {
            AmbaPrint("enable = %d , WarpTableCount = %d", AppWarpControl.Enable, AppWarpControl.WarpTableCount);
            for(i = 0; i < MAX_WARP_TABLE_COUNT; i++) {
                AmbaPrint("---------- storage #%d ----------", i);
                AmbaPrint("enable = %d", AppWarpControl.WarpTable[i]->WarpHeader.Enable);
                AmbaPrint("[swin_info] start_x = %u, start_y = %u, width = %u, height = %u",
                            AppWarpControl.WarpTable[i]->WarpHeader.VinSensorGeo.StartX, AppWarpControl.WarpTable[i]->WarpHeader.VinSensorGeo.StartY,
                            AppWarpControl.WarpTable[i]->WarpHeader.VinSensorGeo.Width, AppWarpControl.WarpTable[i]->WarpHeader.VinSensorGeo.Height);
                AmbaPrint("HorGridNum = %d, VerGridNum = %d, TileWidthExp = %d, TileHeightExp = %d", \
                            AppWarpControl.WarpTable[i]->WarpHeader.HorGridNum,AppWarpControl.WarpTable[i]->WarpHeader.HorGridNum, \
                            AppWarpControl.WarpTable[i]->WarpHeader.TileWidthExp,AppWarpControl.WarpTable[i]->WarpHeader.TileHeightExp);
            }
        } else {
            AmbaPrint("NO Warp calibration");
        }
    }



    if (Rval == -1) {
        AmbaPrint("t cal warp init : re-init warp");
        AmbaPrint("t cal warp warp_spec [raw_encode_text_flag] [script.txt] ");
        AmbaPrint("t cal warp select Enable Channel Id1 Id2 BlendRatio Strength: select warp table");
        AmbaPrint("t cal warp debug enable :enable debug flag to print message");
        AmbaPrint("t cal warp info :warp calibration information");
    }


    return Rval;
}

