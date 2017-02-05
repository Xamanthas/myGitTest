/**
 * @file mw/unittest/extSrc/sample_calib/audio/AmbaUTCalibAudio.c
 *
 * sample code for audio calibration
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

#include "AmbaUTCalibAudio.h"
#include "AmbaUtility.h"

#define DUMP_CALIB_DATA 1
#define NUM_OF_ARGV 7
#define TABLE_SIZE 1024
#define MAX_TOKEN 32
Audio_Calibration_s *AudioAddress;
extern AMBA_KAL_BYTE_POOL_t G_MMPL;
extern AMBA_KAL_BYTE_POOL_t G_NC_MMPL;
INT8 *pCalibCurve = NULL;
INT32 *pTargetdBFS = NULL;
INT32 *pTargetTHD_N = NULL;

 
/**
 *  @brief print the error message for audio calibration
 *
 *  print the error message for audio calibration
 *
 *  @param [in] CalSite calibration site status
 *  @param [out] OutputStr debug message for this function
 *
 *  @return 0 success, -1 failure
 */
int AmpUTCalibAudio_PrintError(char *OutputStr, Cal_Stie_Status_s *CalSite)
{
    AmbaPrint("************************************************************");
    AmbaPrintColor(RED,"%s",OutputStr);
    AmbaPrint("************************************************************");
    CalSite->Status = CAL_SITE_RESET;

    return 0;
}


/**
 *  @brief the call back function for audio calibration
 *
 *  the call back function for audio calibration
 *
 *  @param [out] Curve pointer of silence calibration curve
 *  @param [out] dBFs pointer of dBFS
 *  @param [out] THD_N pointer of THD_N
 *
 *  @return 0 success, -1 failure
 */
int AmpUTCalibAudio_InputCalibCB(INT8 *Curve, INT32 *dBFs, INT32 *THD_N)
{
    int Rtval = 0;

    if(Curve){
        AmbaPrint("Curve[0] %d,Curve[1] %d", *Curve, *(Curve+1));
        pCalibCurve = Curve;
    }
    if(dBFs){
        AmbaPrint("dBFs[0] %d,dBFs[1] %d", *dBFs, *(dBFs+1));
        pTargetdBFS = dBFs;
    }
    if(THD_N){
        AmbaPrint("THD_N[0] %d,THD_N[1] %d", *THD_N, *(THD_N+1));
        pTargetTHD_N = THD_N;
    }
    return Rtval;
}    



/**
 *  @brief start video record
 *
 *  start video record
 *
 *
 *  @return 0 success, -1 failure
 */
static int AmpUTCalibAudio_StartRecordVideo(void)
{
	int Rval = 0;
//bug
#if 0
	Rval = app_video_setup_encode_config();
	app_dzoom_set_lock(1);
	app_dzoom_set_stop();
	Rval = app_rec_exec_record(0);
#endif	
	return Rval;
}

/**
 *  @brief stop video record
 *
 *  stop video record
 *
 *
 *  @return 0 success, -1 failure
 */
static int AmpUTCalibAudio_StopRecordVideo(void)
{
	int Rval = 0;
//bug
#if 0
	Rval = app_dzoom_set_stop();
	Rval = app_lens_set_zoom_stop();
	Rval = app_dzoom_wait_stop();
	app_dzoom_set_lock(0);
	Rval = app_rec_exec_preview(0);
#endif	
	return Rval;
}

#if defined DUMP_CALIB_DATA
/**
 *  @brief dump calibration data
 *
 *  dump calibration data
 *
 *  @param [in] CalibOperateMode calibration operate mode
 *
 *  @return 0 success, -1 failure
 */
int AmpUTCalibAudio_DumpCalibData(UINT32 CalibOperateMode)
{
    int Rval = 0;
    AMBA_FS_FILE *FpOut = NULL;
    char CalibFn[] = "c:\\audio_calibration_data.txt";
    char CalibFreqFn[] = "c:\\audio_freq_curve.txt";
    char FnOut[64];
    INT8 *CalibCurve;
    int Strlen, Gaincnter, i, j;
    char outtext[16 * 5] = {0};

    if (CalibOperateMode == 0) {
        strcpy(FnOut, CalibFn);//save calibration data
    } else if (CalibOperateMode == 2) {
        strcpy(FnOut, CalibFreqFn);//save frequency curve
    }
    CalibCurve = pCalibCurve;    
    FnOut[0] = AmpUT_CalibGetDriverLetter();
    FpOut = AmbaFS_fopen(FnOut, "w");
    if (FpOut == NULL) {
        AmbaPrint("Audio calibration NG:open File fail");
        return AUDIO_CALIB_OPEN_FILE_ERROR;
    }

    Gaincnter = 0;
    for (j=0; j<128; j++) {
        for (i=0; i<16; i++) {
            Strlen = sprintf(outtext,"%5d",*(CalibCurve+Gaincnter));
            AmbaFS_fwrite(outtext, 1, Strlen, FpOut);
            Gaincnter++;
        }
        Strlen = sprintf(outtext,"\n");
        AmbaFS_fwrite(outtext,1,Strlen,FpOut);
    }	
        AmbaFS_fclose(FpOut);

    
    return Rval;
}
#endif

/**
 *  @brief read data from the text file
 *
 *  read data from the text file
 *
 *  @param [in] Fp file pointer
 *  @param [out] Buf output buffer
 *
 *  @return 0 success, -1 failure
 */
static int AmpUTCalibAudio_GetLine(AMBA_FS_FILE *Fp, char Buf[])
{
    // Normal state
    do {
        unsigned char Ch;

        if (AmbaFS_fread(&Ch, 1, 1, Fp) == 0) {// read 1 byte
            return -1;
        }

        if ( (Ch == '\n') || (Ch == '\r') ) {
            break;	// go to end-of-line Status
        }
        *Buf = Ch;
        Buf++;
    } while(1);

    // End of line state
    do {
        unsigned char Ch;

        if (AmbaFS_fread(&Ch, 1, 1, Fp) == 0) {// read 1 byte
            break;
        }

        if( (Ch == '\n') || (Ch == '\r') ) {
        /* do nothing */
        } else {
            // Reach to next line, roll back 1 byte
            AmbaFS_fseek(Fp, -1, SEEK_CUR);
            break;
        }
    } while(1);

    *Buf = '\0';
    return 0;
}

/**
 *  @brief read data from binary file
 *
 *  read data from binary file
 *
 *  @param [in] Table table index
 *  @param [in] File file pointer
 *
 *  @return 0 success, -1 failure
 */
static int AmpUTCalibAudio_ReadDataFromFile(INT8* Table, char* File)
{
    int Rval = AUDIO_CALIB_OK;
    int i = 0;
    int Index = 0;
    char Buf[256];
    char *Argv[MAX_TOKEN];
    AMBA_FS_FILE *Fp = NULL;

    File[0] = AmpUT_CalibGetDriverLetter();
    Fp = AmbaFS_fopen(File, "r");

    if (Fp == NULL) {
        AmbaPrintColor(RED,"OPEN FILE %s FAILED!",File);
        Rval = AUDIO_CALIB_READ_DATA_ERROR;
    } else {
            // do copy data from File to Array
        while(1) {
            if (AmpUTCalibAudio_GetLine(Fp, Buf) == -1) {
                break;
            } else {
                char *Token = strtok(Buf, " ");
                int NumToken = 0;

                while (Token != NULL) {
                    Argv[NumToken] = Token;
                    NumToken++;
                    Token = strtok(NULL, " ");
                }
                for (i = 0; i < NumToken; i++) {
                    Table[Index] = atoi(Argv[i]);
                    Index++;
                }
            }
        }
        AmbaFS_fclose(Fp);
    }

    if (Index != TABLE_SIZE) {
        Rval = AUDIO_CALIB_READ_DATA_ERROR;
        AmbaPrint("number of Token in File %s is error",File);
    }

    for (i = 0; i < 10; i++) {
        AmbaPrint("Index = %d , %d",i, Table[i]);
    }

    return Rval;
}

/**
 *  @brief check the audio input parameter
 *
 *  check the audio input parameter
 *
 *  @param [in] AudioConfig audio configuration
 *
 *  @return 0 success, -1 failure
 */
static int AmpUTCalibAudio_CheckAudioCalibConfig(AMBA_AUDIO_CALIB_CTRL_s* AudioConfig)
{
    int Rval = AUDIO_CALIB_OK;

    if ((AudioConfig->CalibMode > AUDIO_CALIB_APPLY_AAC_ENC) ||(AudioConfig->CalibMode < AUDIO_CALIB_PROC)) {
        AmbaPrint("CalibMode should be between 0~2, CalibMode = %d",AudioConfig->CalibMode);
        return AUDIO_CALIB_PARAM_CALIB_MODE_ERROR;
    }
    if ((AudioConfig->CalibProcCtrl.CalibOperateMode > 2)) {
        AmbaPrint("CalibOperateMode should be between 0~2, CalibOperateMode = %d",AudioConfig->CalibProcCtrl.CalibOperateMode);
        return AUDIO_CALIB_PARAM_CALIB_OPERATE_MODE_ERROR;
    }
    if (AudioConfig->CalibMode == AUDIO_CALIB_PROC) {
        if  ( (AudioConfig->CalibProcCtrl.pCalibNoiseThAddr == NULL) || (AudioConfig->CalibProcCtrl.pCalibRangeAddr == NULL)) {
            AmbaPrintColor(RED,"Null Buffer");
            return AUDIO_CALIB_NULL_BUFFER_ERROR;
        } else if (( AudioConfig->CalibProcCtrl.CalibOperateMode == 1)) {
            if ((AudioConfig->CalibProcCtrl.CalibreFreqIdx < 0) || (AudioConfig->CalibProcCtrl.CalibreFreqIdx > 1023)) {
                AmbaPrintColor(RED,"Incorrect frequency index: Frequency index should be between 0~1023,CalibreFreqIdx = %d",
                    AudioConfig->CalibProcCtrl.CalibreFreqIdx);
                return AUDIO_CALIB_FREQUENCY_INDEX_ERROR;
            }
            
        }
    }

   return Rval;
}

/**
 *  @brief audio calibraion function
 *
 *  audio calibraion function
 *
 *  @param [in] AudioConfig audio configuration
 *  @param [in] CalObj calibration object
 *
 *  @return 0 success, -1 failure
 */
static int AmpUTCalibAudio_DoAudioCalibration(AMBA_AUDIO_CALIB_CTRL_s* AudioConfig, Cal_Obj_s *CalObj, UINT8 ProcMode)
{
    int Rval = 0;
    UINT8 *AudioCalibworkNCBuffer;
    UINT8 *AudioCalibworkCacheBuffer;
    INT32 *AudioCalibWorkBuffer=0;
    Audio_Calibration_s *AppAudioAddress = (Audio_Calibration_s *)CalObj->DramShadow;;
    /*
    Audio calibration processing operations mode.
    0: Calibration for silence calibration curve.
    1: Calculate the dBFS and the THD+N for specified frequency.
    2: Calculate the dBFS of the whole frequency spectrum.
    */    
    if (AudioConfig->CalibProcCtrl.CalibOperateMode == 0) {
        AmbaPrint( "0: Calibration for silence calibration curve. !!!");
    } else if (AudioConfig->CalibProcCtrl.CalibOperateMode == 1) {
        AmbaPrint( "1: Calculate the dBFS and the THD+N for specified frequency. !!!");	
    } else if (AudioConfig->CalibProcCtrl.CalibOperateMode == 2) {
        AmbaPrint( "2: Calculate the dBFS of the whole frequency spectrum. !!!");	
    }

    //create audio pipe and start record
    
    {
        AMBA_AUDIO_IO_CREATE_INFO_s InputInfo;
        int er;
  
        InputInfo.I2sIndex = 0; // depends on project?
        InputInfo.MaxChNum = 2;
        InputInfo.MaxDmaDescNum = 16; // depends on project?
        InputInfo.MaxDmaSize = 1024; // depends on project?
        InputInfo.MaxSampleFreq = 48000;

        // Assign working buffer
        er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&AudioCalibworkCacheBuffer, ALIGN_4(AmbaAudio_InputCachedSizeQuery(&InputInfo))*2 + 32, 100);
        if (er != OK) {
            AmbaPrint("Out of Cache memory for audio working!!");
        }

        er = AmbaKAL_BytePoolAllocate(&G_NC_MMPL, (void **)&AudioCalibworkNCBuffer, ALIGN_8(AmbaAudio_InputNonCachedSizeQuery(&InputInfo))*2 + 32, 100);
        if (er != OK) {
            AmbaPrint("Out of NC memory for audio working!!");
    }
        AmpAudioCalib_Create(AudioCalibworkCacheBuffer,AudioCalibworkNCBuffer,&InputInfo);
        AmbaPrint("Audio Calib create AudioCalibInputHandler");

        er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&AudioCalibWorkBuffer, ALIGN_4(AmbaAudio_EffectCalibBufferSize()), 100);
        if (er != OK) {
            AmbaPrint("Out of EffectCalibBufferSize memory for audio working!!");
        }
    }
    AudioConfig->CalibProcCtrl.pCalibBuffer = AudioCalibWorkBuffer;

    //bug
    AmpUTCalibAudio_StartRecordVideo();
    
    //stop record
    AmpUTCalibAudio_StopRecordVideo();
    AmbaPrintColor(RED, "AUDIO CALIBRATION start!!! ");
    if ((AudioConfig->CalibProcCtrl.CalibOperateMode == 0) ||(AudioConfig->CalibProcCtrl.CalibOperateMode == 2)) {
        AmpAudioCalib_Setup(AudioConfig,&AmpUTCalibAudio_InputCalibCB);
        AmbaKAL_TaskSleep(1000);
        if (AudioConfig->CalibProcCtrl.CalibOperateMode == 0) {
            memcpy(AppAudioAddress->AudioCalibCurve[ProcMode], pCalibCurve, 2048);
            AppAudioAddress->AudioProcModeEn[ProcMode] = 1;
        }

#if defined DUMP_CALIB_DATA
        AmpUTCalibAudio_DumpCalibData(AudioConfig->CalibProcCtrl.CalibOperateMode);
#endif
        } else if (AudioConfig->CalibProcCtrl.CalibOperateMode == 1){
        AmpAudioCalib_Setup(AudioConfig,&AmpUTCalibAudio_InputCalibCB);
        AmbaKAL_TaskSleep(1000);
    }
            
    /* Free audio working memory */

    AmbaKAL_BytePoolFree((void *)AudioCalibworkNCBuffer);
    AmbaKAL_BytePoolFree((void *)AudioCalibworkCacheBuffer);
    AmbaKAL_BytePoolFree((void *)AudioCalibWorkBuffer);
    pCalibCurve = NULL;
    pTargetdBFS = NULL;
    pTargetTHD_N = NULL;
    AmpAudioCalib_Delete();
            
            
    return Rval;
}
/**
 *  @brief control the audio calibration table
 *
 *  control the audio calibration table
 *
 *  @param [in]Enable enable flag
 *  @param [in]Id table ID 1
 *  @param [in]Buffer audio buffer pointer
 *
 *  @return 0 success, -1 failure
 */
int AmpUTCalibAudio_SelectAudioCalibTable(UINT8 Enable, UINT8 Id, INT32 *Buffer)
{
    AMBA_AUDIO_CALIB_CTRL_s Config;
    if (Enable == 1) {
        AmbaPrint("Enable of table %d is %d",Id,AudioAddress->AudioProcModeEn[Id]);
        if (AudioAddress->AudioProcModeEn[Id] == 1) {
            Config.CalibMode = AUDIO_CALIB_APPLY_INPUT;
            Config.CalibApplyCtrl.pCalibBuffer = Buffer;
            AmbaPrintColor(RED,"Buffer = %x", Buffer);
            Config.CalibApplyCtrl.pCalibCurveAddr = AudioAddress->AudioCalibCurve[Id];
            Config.CalibApplyCtrl.pUseAacEncHdlr = NULL;
            AmpAudioCalib_Setup(&Config,NULL);
        }
    }
    return 0;
}

/**
 *  @brief initialize audio calibraion
 *
 *  initialize audio calibraion
 *
 *  @param [in] CalObj calibration object
 *
 *  @return 0 success, -1 failure
 */
int AmpUTCalibAudio_Init(Cal_Obj_s *CalObj)
{
    INT Rval = 0;

    UINT32 Size;
    AMBA_MEM_CTRL_s AmbaAudio_CalibData;
    UINT8 *AudioCalibworkNCBuffer;
    UINT8 *AudioCalibworkCacheBuffer;          
    AMBA_AUDIO_IO_CREATE_INFO_s InputInfo;
    int er;

    AudioAddress = (Audio_Calibration_s *)CalObj->DramShadow;
    
    Size = AmbaAudio_EffectCalibBufferSize();

    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &AmbaAudio_CalibData.pMemAlignedBase, &AmbaAudio_CalibData.pMemBase, Size, 32);

    if (Rval != OK) {
        AmbaPrint("AUDIO calibration NG: Allocate memory error");
        Rval = AUDIO_CALIB_ALLOCATE_MEMORY_ERROR;
        return Rval;
    }

    {
        InputInfo.I2sIndex = 0; // depends on project?
        InputInfo.MaxChNum = 2;
        InputInfo.MaxDmaDescNum = 16; // depends on project?
        InputInfo.MaxDmaSize = 1024; // depends on project?
        InputInfo.MaxSampleFreq = 48000;

        // Assign working buffer
        er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&AudioCalibworkCacheBuffer, ALIGN_4(AmbaAudio_InputCachedSizeQuery(&InputInfo))*2 + 32, 100);
        if (er != OK) {
            AmbaPrint("Out of Cache memory for audio working!!");
        }

        er = AmbaKAL_BytePoolAllocate(&G_NC_MMPL, (void **)&AudioCalibworkNCBuffer, ALIGN_8(AmbaAudio_InputNonCachedSizeQuery(&InputInfo))*2 + 32, 100);
        if (er != OK) {
            AmbaPrint("Out of NC memory for audio working!!");
        }
        AmpAudioCalib_Create(AudioCalibworkCacheBuffer,AudioCalibworkNCBuffer,&InputInfo);
        AmbaPrint("Audio Calib create AudioCalibInputHandler");

    }
    
    AmbaPrintColor(RED,"AmbaAudio_CalibData.pMemAlignedBase = %x", AmbaAudio_CalibData.pMemAlignedBase);
    AmpUTCalibAudio_SelectAudioCalibTable(1, 0, AmbaAudio_CalibData.pMemAlignedBase);
    return 0;
}


/**
 *  @brief the entry function for audio calibration
 *
 *  the entry function for audio calibration
 *
 *  @param [in] Argc number of input parameters
 *  @param [in] Argv value of input parameters
 *  @param [in] CalSite calibration site status
 *  @param [in] CalObj calibration object
 *  @param [out] OutputStr debug message for this function 
 *
 *  @return 0 success, -1 failure
 */
int AmpUTCalibAudio_Func(int argc, char *Argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, Cal_Obj_s *CalObj)
{	
    int Rval = 0;
    INT8 CalibThd[TABLE_SIZE];
    INT8 CalibRange[TABLE_SIZE];
    AMBA_AUDIO_CALIB_CTRL_s AudioConfig;
    INT8 SourceSelect = 0, SilentIndex = 0, RangeIndex = 0;
    UINT8 ProcMode = 0;
    //bug, check mode         
    /*	app_t *videoApp;
    app_t *dvApp;
    app_t *cardvApp;
    app_t *currApp;

    //check curr app
    appmgt_get_app(APP_REC_DSC, &videoApp);
    appmgt_get_app(APP_REC_DV, &dvApp);
    appmgt_get_app(APP_REC_CAR_VIDEO, &cardvApp);
    appmgt_get_curapp(&currApp);

    if ((currApp != videoApp) && (currApp != dvApp) && (currApp != cardvApp)) {
        AmbaPrintColor(RED, "AUDIO CALIBRATION NOT IN VIDEO STATE");
        return -1;
    }
    */
    if (argc != NUM_OF_ARGV) {
        AmbaPrintColor(RED,"AUDIO calibration NG: calibration paramter number ERROR!");
        return AUDIO_CALIB_PARAMETER_NUMBER_ERROR;
    }


//AUDIO [ProcMode] [CalibOperateMode] [SourceSelect] [SilentIndex] [RangeIndex ] [CalibreFreqIdx]

    ProcMode = (AMBA_AUDIO_CALIB_MODE_e)atoi(Argv[1]);
    AudioConfig.CalibMode = AUDIO_CALIB_PROC;
    //assign paramter
    /*
    0: Calibration for silence calibration curve.
    1: Calculate the dBFS and the THD+N for specified frequency.
    2: Calculate the dBFS of the whole frequency spectrum.*/
    
    AudioConfig.CalibProcCtrl.CalibOperateMode = atoi(Argv[2]); 
    if ((AudioConfig.CalibProcCtrl.CalibOperateMode > 2)) {
        AmbaPrint("CalibOperateMode should be between 0~2, CalibOperateMode = %d",AudioConfig.CalibProcCtrl.CalibOperateMode);
        Rval = AUDIO_CALIB_PARAM_CALIB_OPERATE_MODE_ERROR;
        sprintf(OutputStr, "[NG]AUDIO: CalibOperateMode error!");
        return Rval;
    }
    SourceSelect = atoi(Argv[3]);
    SilentIndex = atoi(Argv[4]);
    RangeIndex = atoi(Argv[5]);
    if ( SourceSelect == 0) {
        char SilentFn[] = "c:\\audio_silent_table.txt";
        char RangeFn[] = "c:\\audio_range_table.txt";

        if (AmpUTCalibAudio_ReadDataFromFile(CalibThd, SilentFn) < 0) {
            AmbaPrintColor(RED,"AUDIO calibration NG: read audio silent table error");
            Rval = AUDIO_CALIB_READ_DATA_ERROR;
            sprintf(OutputStr, "[NG]AUDIO: read audio silent table error!");
            return Rval;
        }
        if (AmpUTCalibAudio_ReadDataFromFile(CalibRange, RangeFn) < 0) {
            AmbaPrintColor(RED,"AUDIO calibration NG: read audio range table error");
            sprintf(OutputStr, "[NG]AUDIO: read audio range table error!");
            Rval = AUDIO_CALIB_READ_DATA_ERROR;
            return Rval;
        }
        
    } else if (SourceSelect == 1) {
        if (((SilentIndex < 0 || SilentIndex > 30)) ){
            AmbaPrintColor(RED,"AUDIO calibration NG: silent_thd out of range!");
            Rval = AUDIO_CALIB_SILENT_THRESHOLD_ERROR;
            sprintf(OutputStr, "[NG]AUDIO: silent index out of range!");
            return Rval;
        } 
        if ((RangeIndex < 0 || RangeIndex > 30)) {
            AmbaPrintColor(RED,"AUDIO calibration NG: index range out of range!");
            sprintf(OutputStr, "[NG]AUDIO: range index out of range!");
            Rval = AUDIO_CALIB_RANGE_INDEX_ERROR;
            return Rval; 
        }
        memset(CalibThd, RangeIndex, TABLE_SIZE * sizeof(INT8));
        memset(CalibRange, RangeIndex, TABLE_SIZE * sizeof(INT8));
    } else {
        AmbaPrintColor(RED,"AUDIO calibration NG: thd select param error! It should be 0 or 1");
        sprintf(OutputStr, "[NG]AUDIO: source select param error! It should be 0 or 1!");
        Rval = AUDIO_CALIB_PARAMETER_ERROR;
        return Rval; 
    }
    AudioConfig.CalibProcCtrl.pCalibNoiseThAddr = CalibThd;
    AudioConfig.CalibProcCtrl.pCalibRangeAddr = CalibRange;
    AudioConfig.CalibProcCtrl.CalibreFreqIdx = atoi(Argv[6]);

    if ( (AmpUTCalibAudio_CheckAudioCalibConfig(&AudioConfig) >= 0) && (Rval >= 0) ) {
        Rval = AmpUTCalibAudio_DoAudioCalibration(&AudioConfig, CalObj, ProcMode);
    }	

    if (Rval == 0) {
        CalSite->Status = CAL_SITE_DONE;
        CalSite->Version = CAL_AUDIO_VER;
        CalSite->SubSiteStatus[0] = CAL_SITE_DONE;
        CalSite->SubSiteStatus[1] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[2] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[3] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[4] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[5] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[6] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[7] = CAL_SITE_RESET;
        sprintf(OutputStr, "[OK]AUDIO: Calibration OK!");
    } else {
        CalSite->Status = CAL_SITE_RESET;
        CalSite->Version = CAL_SITE_RESET;
        CalSite->SubSiteStatus[0] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[1] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[2] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[3] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[4] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[5] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[6] = CAL_SITE_RESET;
        CalSite->SubSiteStatus[7] = CAL_SITE_RESET;
        sprintf(OutputStr, "[NG]AUDIO: Calibration NG!");        
    }

    return Rval;
}

/**
 *  @brief the update function for audio calibration
 *
 *  the update function for audio calibration
 *
 *  @param [in] CalSite calibration site status
 *  @param [in] CalObj calibration object
 *
 *  @return 0 success, -1 failure
 */
int AmpUTCalibAudio_Upgrade(Cal_Obj_s *CalObj, Cal_Stie_Status_s *CalSite)
{
    if(CalObj->Version != CalSite->Version) {
        // This API is an example to handle calibration data upgrade
        AmbaPrint("[CAL] Site %s Version mismatch (FW:0x%08X, NAND:0x%08X)", CalObj->Name, CalObj->Version, CalSite->Version);
    }

    // The default behavior is to do-nothing when Version mismatch
    return 0;	
}


void AmpUTCalibAudio_Disable(void) {
    //AmbaAudio_InputDisableCalib();
}

/**
 *  @brief the unit test function for audio calibration
 *
 *  the unit test function for audio calibration
 *
 *  @param [in] Argc number of input parameters
 *  @param [in] Argv value of input parameters
 *
 *  @return 0 success, -1 failure
 */
 int AmpUTAudio_UTFunc(int Argc, char *Argv[])
{
    Cal_Obj_s           *CalObj;
    int Rval = -1;

    CalObj = AmpUT_CalibGetObj(CAL_AUDIO_ID);
    if ((strcmp(Argv[2], "init") == 0)) {
        AmpUTCalibAudio_Init(CalObj);
        Rval = 0;
    } else if ((strcmp(Argv[2], "disable") == 0)) {
        AmpUTCalibAudio_Disable();
        Rval = 0;
    }

    if (Rval == -1) {
        AmbaPrint("t cal audio init : re-init Audio");
        AmbaPrint("t cal audio disable : disable Audio calibration");
    }
    return Rval;
}


