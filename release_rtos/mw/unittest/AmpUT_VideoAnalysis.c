 /**
  * @file \src\unittest\MWUnitTest\AmpUT_YUVTask.c
  *
  * YUV task unit test
  *
  * History:
  *    2014/11/18 - [Bill Chou] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vin/vin.h>
#include <recorder/Encode.h>
#include <recorder/VideoEnc.h>
#include <util.h>
#include "AmbaUtility.h"
#include "AmbaKAL.h"
#include "AmbaCache_Def.h"
#include "AmbaDataType.h"
#include "AmbaTimer.h"

//#include "Config.h"
#include "ambava.h"
#include "ambava_frmhdlr.h"
#include "ambava_3ahdlr.h"
#include "AmpUT_YUVReadSD.h"
#include "ambava_adas.h"
#include "ambava_adas_FCMD.h"
#include "ambava_adas_LLWS.h"
#include "ambava_adas_MD_yuv.h"
#include "ambava_adas_MD_ae.h"
#include "ambava_traffic_sign_detector.h"

#ifdef CONFIG_BUILD_FOSS_OPENCV
#include "opencv2/core/core_c.h"
#endif
#ifdef CONFIG_GPS_UBLOX6_GM6XX
#include "gps.h"
#include "gps_dev.h"
#include "gps_dev_list.h"
#include "gps_parser.h"
#endif

#ifndef LINE_DBG
#define LINE_DBG AmbaPrint("[[%s]] %d", __func__, __LINE__);
#endif

#ifdef CONFIG_GPS_UBLOX6_GM6XX
static gps_dev_list_t *gps_dev = 0;
#endif

#define VALOGOUTPUT 1
#if VALOGOUTPUT
#define SD_VA_LOG_FN2   "C:\\valogbackup.amyuv.elog"
#define VALOGDATALENGTH (160)
static int IsUseBckName = 0;
static char valogdata[VALOGDATALENGTH] = {0};
static char OutElogName[128] = {0};
#endif

extern AMBA_KAL_BYTE_POOL_t G_MMPL;
static int fcmd_init = 0;
static int llws_init = 0;
static int mdy_init = 0;
static int mdae_init = 0;
static int adas_init = 0;
static int VAgps_init= 0;
static int VAgps_firstW= 0;
static AMBA_ADAS_GPS_INFO_s mwut_gps = {0};
static float gps_sim[36] = {  0, 0, 1, 0, 0, 0, 0, 0, 0,
                            20, 20, 0, 20, 20, 20, 20, 20, 20,
                            20, 0, 0, 0, 0, 1, 0, 0, 0,
                            20, 20, 20, 20, 20, 20, 20, 20, 20};
static int gps_index = 0;
static int gps_id = 0; //HACK

static float VAUT_HoodLevel = DEFAULT_HOODLEVEL;
static float VAUT_HorizonLevel = DEFAULT_HORIZLEVEL;
static float VAUT_HorizAngle = 129;
static float VAUT_VertAngle = 83;

static int Detec_Init = 0;
static int Test_Init = 0;

AMBA_ADAS_TSD_PAR_t DetecPar[1] = {0};
AMBA_ADAS_TSD_CFG_t DetecCfg[1] = {0};

/// tools
int VA_File_Exists(const char * filename)
{
  AMBA_FS_FILE * file;
  if (file = AmbaFS_fopen(filename, "r"))
  {
    AmbaFS_fclose(file);
    return 1;
  }
  return 0;
}

int VA_fputs(const char* pBuf, AMBA_FS_FILE *pFile)
{
    int Rval = 0;
    AmbaFS_fwrite(pBuf, strlen(pBuf) , 1, pFile);
    return Rval;
}

#if VALOGOUTPUT
int VAAdas_OutputELogger(AMP_ENC_YUV_INFO_s* img,AMBA_ADAS_OUTPUTEVENT_s* OutEvent , int FrameCount,  int IsInited)
{
    int  Rval = 0;
    if ((!IsInited)) {
        int csvExist;
        AMBA_FS_FILE * file;
        char headerInfo[160];
        char header[160];
        char datahead[160];
        char datahead2[160];
        char *CurrentPlayFileName;
        int ERR = 0;
        char elog[20];
        ERR = AmbaVa_GetTheCurren_SDreadName( &CurrentPlayFileName);
        if (ERR < 0) {
        AmbaPrintColor(RED, "there is no file reading\n");
        return -1;
        }
        memcpy(OutElogName, CurrentPlayFileName, 64);
        strcpy(elog, ".elog");
        strcat(OutElogName, elog);
        AmbaPrintColor(RED, "OutElogName:  %s \n", OutElogName);

        csvExist = VA_File_Exists( OutElogName);
        file = AmbaFS_fopen( OutElogName, "w");
        /*
        if (!csvExist){
        file = AmbaFS_fopen( OutElogName, "w");
        } else{
        file = AmbaFS_fopen( SD_VA_LOG_FN2, "w");
        IsUseBckName = 1;
        }
        */
        sprintf(header,       "  ,src width, src height, pitch, ysize, colortype  \r\n");
        sprintf(headerInfo,   " , %d, %d, %d, %d, %d \r\n", img->width, img->height, img->pitch, img->ySize, img->colorFmt);
        sprintf(datahead,       " FrmNumber , LLWS, Direction, LINETYPE, LINESOLIDITY, LINECOLOR  \r\n");
        sprintf(datahead2,       " FrmNumber , FCWS, xxxx  \r\n");
        VA_fputs(header, file);
        VA_fputs(headerInfo, file);
        VA_fputs(datahead, file);
        VA_fputs(datahead2, file);
        AmbaFS_fclose(file);
    } else {
        if ((OutEvent->LdwEvent != NULL) || (OutEvent->FcwEvent != NULL)) {
            AMBA_FS_FILE * datafile;
            //AmbaPrintColor(RED, "frame number %d \n", event);
            if (!IsUseBckName) {
                    datafile = AmbaFS_fopen( OutElogName, "a");
                } else{
                    datafile = AmbaFS_fopen( SD_VA_LOG_FN2, "a");
                }
            /// event in log mod is frame count. It is ADD by framehandler
            if (OutEvent->LdwEvent != NULL) {
                //sprintf(datahead,       " FrmNumber , FCWS / LDWS, Direction, LINETYPE, LINESOLIDITY, LINECOLOR  \r\n");
                memset(valogdata, 0, VALOGDATALENGTH);
                sprintf(valogdata, " %d,LDWS,%d,%d,%d,%d  \r\n",
                                        FrameCount, OutEvent->LdwEvent->Direction, OutEvent->LdwEvent->MarkType.Type,
                                        OutEvent->LdwEvent->MarkType.Solidity, OutEvent->LdwEvent->MarkType.Color);
                VA_fputs(valogdata, datafile);
                AmbaPrintColor(RED, "frame number %d \n", FrameCount);
            }
            if (OutEvent->FcwEvent != NULL) {
                memset(valogdata, 0, VALOGDATALENGTH);
                sprintf(valogdata, " %d,FCWS,0 \r\n", FrameCount);
                VA_fputs(valogdata, datafile);
                AmbaPrintColor(RED, "frame number %d \n", FrameCount);
            }
            AmbaFS_fclose(datafile);
        }
    }
    return Rval;
}


int VATD_OutputELogger(AMP_ENC_YUV_INFO_s* img, AMBA_ADAS_TSD_OUT_t* OutEvent , int FrameCount,  int IsInited)
{
    int  Rval = 0;
    int i;
    if ((!IsInited)) {
        int csvExist;
        AMBA_FS_FILE * file;
        char headerInfo[160];
        char header[160];
        char datahead[160];
        char datahead2[160];
        char *CurrentPlayFileName;
        int ERR = 0;
        char elog[64];
        ERR = AmbaVa_GetTheCurren_SDreadName( &CurrentPlayFileName);
        if (ERR < 0) {
            AmbaPrintColor(RED, "there is no file reading\n");
            return -1;
        }
        memcpy(OutElogName, CurrentPlayFileName, 64);
        strcpy(elog, ".elog");
        strcat(OutElogName, elog);
        AmbaPrintColor(RED, "OutElogName:  %s \n", OutElogName);

        csvExist = VA_File_Exists( OutElogName);
        file = AmbaFS_fopen( OutElogName, "w");
        /*
        if (!csvExist){
        file = AmbaFS_fopen( OutElogName, "w");
        } else{
        file = AmbaFS_fopen( SD_VA_LOG_FN2, "w");
        IsUseBckName = 1;
        }
        */
        sprintf(header,       "  ,src width, src height, pitch, ysize, colortype  \r\n");
        sprintf(headerInfo,   " , %d,%d,%d,%d,%d \r\n", img->width, img->height, img->pitch, img->ySize, img->colorFmt);
        sprintf(datahead,       " FrmNumber , TD, ROI-X, ROI-Y, ROI-W, ROI-H, TD-Type  \r\n");
        sprintf(datahead2,       " \r\n");
        VA_fputs(header, file);
        VA_fputs(headerInfo, file);
        VA_fputs(datahead, file);
        VA_fputs(datahead2, file);
        AmbaFS_fclose(file);
    } else {
        if (OutEvent->ObjNum > 0) {
            AMBA_FS_FILE *datafile;
            //AmbaPrintColor(RED, "frame number %d \n", event);
            if (!IsUseBckName) {
                datafile = AmbaFS_fopen( OutElogName, "a");
            } else{
                datafile = AmbaFS_fopen( SD_VA_LOG_FN2, "a");
            }
            /// event in log mod is frame count. It is ADD by framehandler
            ///sprintf(datahead,       " FrmNumber , FCWS / LDWS, Direction, LINETYPE, LINESOLIDITY, LINECOLOR  \r\n");
            for (i = 0; i < OutEvent->ObjNum; i++) {
                memset(valogdata, 0, VALOGDATALENGTH);
                sprintf(valogdata, " %d,TD,%d,%d,%d,%d,%d  \r\n",
                                    FrameCount, OutEvent->Object[i].X,  OutEvent->Object[i].Y,
                                     OutEvent->Object[i].W,  OutEvent->Object[i].H, OutEvent->TrafficSign[i]);
                VA_fputs(valogdata, datafile);
                AmbaPrintColor(RED, "VA_fputs frame number %d \n", FrameCount);
            }
            AmbaFS_fclose(datafile);
        }
    }
    return Rval;
}
#endif

/// sample fuction this will be the adas modules
int cardvws_process_FCMD(UINT32 event, AMP_ENC_YUV_INFO_s* img)
{
    int rval = 0;
    AMBA_ADAS_FCMD_EVENT_e fcmd_event = 0;
    //set flag framein
    AMP_FCMD_CFG_t cfg = {0};
    AMP_FCMD_PAR_t par = {0};

    if (fcmd_init == 0) {
        if ( Amba_AdasFCMD_GetDef_Cfg_Par(img, &cfg, &par) != OK){
            AmbaPrint("Amba_AdasFCMD_GetDef_Cfg_Par failed");
            return 0;
        }
        rval = AmbaKAL_MemAllocate(&G_MMPL, &(cfg.Buf), cfg.Buf_Size, 32);
        if (rval != OK) {
            AmbaPrint(" cardvws_process_FCMD Can't allocate memory.");
            return 0;
        }else {
            AmbaPrint("AmbaKAL_MemAllocate Alignedbuf = 0x%X / size = %d  !", cfg.Buf.pMemAlignedBase, cfg.Buf_Size);
        }
            /// set config
        cfg.FCMDSensitivity = ADAS_SL_MEDIUM;
        cfg.IsEnabled = 1;
        /// set pars
        par.HoodLevel = DEFAULT_HOODLEVEL;
        par.HorizonLevel = DEFAULT_HORIZLEVEL;
        par.IsUpdate = 1;



        rval = Amba_AdasFCMD_SetCfg(&cfg);
        if ( rval != OK){
            AmbaPrint("Amba_AdasFCMD_SetCfg failed");
            return 0;
        }
        rval = Amba_AdasFCMD_Init(img, &par);
        if (rval != OK){
            AmbaPrint("Amba_AdasFCMD_Init failed");
            return 0;
        }
        fcmd_init = 1;
    }
    if (fcmd_init == 1) {
        mwut_gps.Speed = (float)gps_sim[gps_index];
        par.IsGPSandDashboard = 1;
        par.Aux_data.pGpsInfo = &mwut_gps;
        gps_index ++;
        if (gps_index == 35 ) {
            gps_index = 0;
        }
        rval = Amba_AdasFCMD_Proc(img, &par, &fcmd_event);
        //AmbaPrintColor(rval, "Amba_AdasFCMD_Proc  return %d", rval);
        if (rval < 0) {
            fcmd_init = 0;
        }
    }

    if (fcmd_event == AMBA_FCMD_MOVE) {
        AmbaPrintColor(RED, "Frontal Vehicle Move \n");
        AmbaKAL_TaskSleep(3000);
    }

    //AmbaPrintColor(RED, "End \n");
    return rval;
}

int cardvws_process_LLWS(UINT32 event, AMBA_DSP_EVENT_CFA_3A_DATA_s* pdata)
{
    int rval = 0;
    int llws_event = 0;

    AMP_LLWS_CFG_t cfg = {0};
    AMP_LLWS_PAR_t par = {0};
    /// set config
    cfg.LLWSSensitivity = ADAS_SL_MEDIUM;
    cfg.IsEnabled = 1;
    /// set pars
    par.HoodLevel = DEFAULT_HOODLEVEL;
    par.HorizonLevel = DEFAULT_HORIZLEVEL;
    if (llws_init == 0) {
        rval = Amba_AdasLLWS_SetCfg(&cfg);
        rval |= Amba_AdasLLWS_Init(&par);
        if (rval == 0) {
            llws_init = 1;
        }
        AmbaPrintColor(rval, "Amba_AdasLLWS_Init  return %d", rval);
    }
    if (llws_init == 1) {
        rval = Amba_AdasLLWS_Proc(pdata, &par, &llws_event);
        //AmbaPrintColor(rval, "Amba_AdasFCMD_Proc  return %d", rval);
        if (rval < 0) {
            llws_init = 0;
        }
    }

    if (llws_event == AMBA_LLWS_LOW_LIGHT) {
        AmbaPrintColor(RED, "Turn  on the light******************************************************** \n");
        AmbaPrintColor(RED, "Turn  on the light******************************************************** \n");
        AmbaKAL_TaskSleep(3000);
    }


    //AmbaPrintColor(RED, "End \n");
    return rval;
}


int Process_MDY(UINT32 event, AMP_ENC_YUV_INFO_s* img)
{
    int rval = 0;
    int md_event = 0;
    //set flag framein

    ///{
    ///    AmbaPrintColor( GREEN, "cardvws_process_preview_stream..........\n ");
    ///    AmbaPrintColor( GREEN, "width = %d,  height %d , pitch %d, Size %d \n", img->width, img->height, img->pitch, img->ySize);
    ///    AmbaPrintColor( GREEN, "luma = 0x%x,  \n",  img->yAddr);
    ///}
    AMBA_MDY_CFG_t cfg = {0};
    AMBA_MDY_PAR_t par = {0};
    AMP_ENC_YUV_INFO_s Img = {0};
    /// set pars
    par.Method = MDY_DEFAULT;
    if (mdy_init == 0) {
        Amba_AdasMDY_GetDefCfg( &Img, &cfg);
        rval = AmbaKAL_MemAllocate(&G_MMPL, &(cfg.MDYBuf), cfg.MDYBuf_Size, 32);
        if (rval != OK) {
            AmbaPrint(" Process_MDY Can't allocate memory.");
            return 0;
        }else {
            AmbaPrint("AmbaKAL_MemAllocate Alignedbuf = 0x%X / size = %d   \n!", cfg.MDYBuf.pMemAlignedBase, cfg.MDYBuf_Size);
        }
        /// set config
        cfg.RoiData[0].Location.X = 0;
        cfg.RoiData[0].Location.Y = 0;
        cfg.RoiData[0].Location.W = 300;
        cfg.RoiData[0].Location.H = 200;
        cfg.RoiData[0].Sensitivity = 10;
        rval = Amba_AdasMDY_SetCfg(&cfg);
        par.IsUpdate = 1;
        rval |= Amba_AdasMDY_Init(&par);
        if (rval == 0) {
            mdy_init = 1;
        }
    }
    if (mdy_init == 1) {
        rval = Amba_AdasMDY_Proc(img, &par, &md_event);
        if (rval < 0) {
            mdy_init = 0;
        }
    }

    if (md_event == AMBA_MDY_MOVE_DET) {
        AmbaPrintColor(RED, "AMBA_MDY_MOVE_DET******************************************************** \n");
        AmbaPrintColor(RED, "AMBA_MDY_MOVE_DET******************************************************** \n");
        AmbaKAL_TaskSleep(3000);
    }

    //AmbaPrintColor(RED, "End \n");
    return rval;
}

int Process_MDAE(UINT32 event, AMBA_DSP_EVENT_CFA_3A_DATA_s* pdata)
{
    int rval = 0;
    int md_event = 0;

    AMBA_MDAE_CFG_t cfg = {0};
    AMBA_MDAE_PAR_t par = {0};

    /// set pars
    par.Method = MDAE_DEFAULT;

    if (mdae_init == 0) {
        Amba_AdasMDAE_GetDefCfg(&cfg);
        rval = AmbaKAL_MemAllocate(&G_MMPL, &(cfg.MDAEBuf), cfg.MDAEBuf_Size, 32);
        if (rval != OK) {
            AmbaPrint(" Process_MDAE Can't allocate memory.");
            return 0;
        }else {
            AmbaPrint("AmpUtil_GetAlignedPool Alignedbuf = 0x%X / size = %d   \n!", cfg.MDAEBuf.pMemAlignedBase, cfg.MDAEBuf_Size);
        }
        cfg.RoiData[0].Location.X = 0;
        cfg.RoiData[0].Location.Y = 0;
        cfg.RoiData[0].Location.W = 12;
        cfg.RoiData[0].Location.H = 8;
        cfg.RoiData[0].Sensitivity = 2; /// Sensitivity is Multiplier
        cfg.RoiData[0].Threshold = 3900;
        cfg.RoiData[0].Update_Freq = 1;
        cfg.RoiData[0].Update_Cnt = cfg.RoiData[0].Update_Freq - 1;
        rval = Amba_AdasMDAE_SetCfg(&cfg);
        AmbaPrint("Amba_AdasMDAE_SetCfg  %d\n", rval);
        par.IsUpdate = 1;
        rval = Amba_AdasMDAE_Init(&par);
        AmbaPrint("Amba_AdasMDAE_Init  %d\n", rval);
        if (rval == 0) {
            mdae_init = 1;
        }
        AmbaPrintColor(rval, "Amba_AdasMDAE_Init  return %d", rval);
    }
    if (mdae_init == 1) {
        rval = Amba_AdasMDAE_Proc(pdata, &par, &md_event);
        //AmbaPrintColor(rval, "Amba_AdasFCMD_Proc  return %d", rval);
        if (rval < 0) {
            mdae_init = 0;
        }
    }

    if (md_event == AMBA_MDAE_MOVE_DET) {
        AmbaPrintColor(RED, "AMBA_MDY_MOVE_DET******************************************************** \n");
        AmbaPrintColor(RED, "AMBA_MDY_MOVE_DET******************************************************** \n");
        AmbaKAL_TaskSleep(1000);
    }

    //AmbaPrintColor(RED, "End \n");
    return rval;
}

int Set_CarDVPosition_Pars( float HoodLevel, float HorizonLevel, float HorizAngle, float VertAngle)
{
    int Rval = 0;
    if (HoodLevel <= 0 || HorizonLevel <= 0 || HorizAngle <= 0 || VertAngle  <= 0) {
        AmbaPrintColor(RED, " Set_CarDVPosition_Pars Range Error set it to Default \n");
        return -1;
    }
    VAUT_HoodLevel      = HoodLevel;
    VAUT_HorizonLevel   = HorizonLevel;
    VAUT_HorizAngle     = HorizAngle;
    VAUT_VertAngle      = VertAngle;
    AmbaPrintColor(RED, " Set_CarDVPosition_Pars    HoodLevel = %f  \n",
                   HoodLevel);
    AmbaPrintColor(RED, "                           HorizonLevel = %f \n",
                   HorizonLevel);
    AmbaPrintColor(RED, "                           HorizAngle = %f \n",
                   HorizAngle);
    AmbaPrintColor(RED, "                           VertAngle = %f \n",
                   VertAngle);
    return Rval;
}
#ifdef CONFIG_CC_CXX_SUPPORT
int Init_ADAS( AMP_ENC_YUV_INFO_s* img)
{
    int Rval = 0;
    AMBA_ADAS_SCENE_PARAMS_s par;
    AMBA_ADAS_VIEWANGLE_s Vagl;
    AMBA_ADAS_LDW_PARAMS_s ldw_par = {0};
    AMBA_ADAS_FCW_PARAMS_s fcw_par = {0};
    AMBA_ADAS_LDW_OUTPUTEVENT_s LdwEvent = {0};
    AMBA_ADAS_FCW_OUTPUTEVENT_s FcwEvent = {0};
    Vagl.HorizAngle = VAUT_HorizAngle;
    Vagl.VertAngle = VAUT_VertAngle;
    Amba_Adas_GetSceneParams( &par);
    par.CameraMountHeight = ADAS_CMTGOLFCLASS ;
    par.AutomaticCalibration = 0;
    par.HoodLevel = VAUT_HoodLevel;
    par.HorizonLevel = VAUT_HorizonLevel;
    par.HorizontalPan = 50.0;
    Rval = Amba_Adas_Init(img, &par, &Vagl);
    Amba_Adas_GetLdwParams(&ldw_par);
    ldw_par.IsEnabled = 1;
    Amba_Adas_SetLdwParams(ldw_par);
    Amba_Adas_GetFcwParams(&fcw_par);
    fcw_par.IsEnabled = 1;
    Amba_Adas_SetFcwParams(fcw_par);
    return Rval;
}


int Process_ADAS(UINT32 event, AMP_ENC_YUV_INFO_s* img)
{
    int Rval = 0;
    int s = 0;
    AMBA_ADAS_GPS_INFO_s gpsInfo;
    AMBA_ADAS_AUX_DATA_s AuxData;
    AMBA_ADAS_LDW_OUTPUTEVENT_s LdwEvent = {0};
    AMBA_ADAS_FCW_OUTPUTEVENT_s FcwEvent = {0};
    AMBA_ADAS_SCENE_STATUS_s SceneStatus = {0};
    AMBA_ADAS_OUTPUTEVENT_s OutEvent = {0};
    unsigned int ts = (unsigned int) AmbaTimer_GetSysTickCount();
    ts = (unsigned int) AmbaTimer_GetSysTickCount();

    if (adas_init == 0) {
        int err = 0;
        err = Init_ADAS( img);
#if VALOGOUTPUT
        VAAdas_OutputELogger( img, &OutEvent, event, adas_init);
#endif
        if (err == 0) {
            adas_init = 1;
        } else {
            AmbaPrintColor(RED, "UT Init_ADAS err = %d \n", err );
            return -1;
        }
    }
    memset(&gpsInfo, 0, sizeof(gpsInfo));
    gpsInfo.Speed = 100;

    memset(&AuxData, 0, sizeof(AuxData));
    AuxData.pGpsInfo = &gpsInfo;

    OutEvent.FcwEvent = &FcwEvent;
    OutEvent.LdwEvent = &LdwEvent;
    OutEvent.SceneStatus = &SceneStatus;

    Amba_Adas_Proc( img, &AuxData, &OutEvent, ts);
    if (OutEvent.LdwEvent != NULL) {
        AmbaPrintColor(RED, "Departure: %s \n",(OutEvent.LdwEvent->Direction == AMBA_ADAS_DdTowardsLeft) ? "left" : "right");
    }
    if (OutEvent.FcwEvent != NULL) {
        AmbaPrintColor(RED, "Frontal Collision Warning \n");
    }
#if VALOGOUTPUT
    VAAdas_OutputELogger( img, &OutEvent, event, adas_init);
#endif
    if (event % 500 == 0) {
        AmbaPrintColor(RED, "frame number %d \n", event);
    }
    return Rval;
}
#ifdef CONFIG_BUILD_FOSS_OPENCV
int Amba_OpenCV_TestInit(void)
{
    int Rval = 0;
    Rval = Amba_CvMemoryPoolInit( &G_MMPL);
    return Rval;
}

int Amba_ADAS_TrafficSignDetector_Test(UINT32 event, AMBAVA_YUV_INFO_s* Src)
{
    int Rval = 0;
    int i;
    int CurTickCount = 0;
    int PreTickCount = 0;
    UINT8* pGray;
    AMBA_VA_ROI_t* pRect;
    AMBA_ADAS_TSD_OUT_t Out[1] = {0};
    if (Detec_Init == 0) {
        int err = 0;
        DetecCfg->IsEnabled = 1;
        DetecCfg->Sensitivity = ADAS_SL_MEDIUM;

        Out->Object;
        //LINE_DBG;
        err = Amba_ADAS_TrafficSignDetector_Init(Src, DetecCfg);
        if (err != 0){
            AmbaPrintColor(RED,"Amba_Detec_Init failed\n");
            return 0;
        }
        else{
#if VALOGOUTPUT
            VATD_OutputELogger( Src, Out, event, Detec_Init);
#endif
            Detec_Init = 1;
        }
        if ((Test_Init)) {
            AmbaPrintColor(RED,"Amba_Detec_Init OK \n");
            return 0;
        }

    }
    PreTickCount = AmbaKAL_GetTickCount();
    Amba_ADAS_TrafficSignDetector_Proc(Src, DetecPar, Out);
    CurTickCount = AmbaKAL_GetTickCount();
#if VALOGOUTPUT
    VATD_OutputELogger( Src, Out, event, Detec_Init);
#endif
    if (event % 500 == 0) {
        AmbaPrintColor(RED, "frame number %d \n", event);
        AmbaPrintColor(RED,"Detec Time %d ", CurTickCount -  PreTickCount);
        AmbaPrintColor(GREEN,"Out->ObjNum = %d", Out->ObjNum );
    }

    for (i = 0, pRect = Out->Object; i < Out->ObjNum; i++, pRect++)
    {
        AmbaPrintColor(RED,"Detec result%d : x=%d, y =%d, h=%d, w=%d", i, pRect->X, pRect->Y, pRect->H, pRect->W );
        AmbaPrintColor(RED,"Detec Time %d ", CurTickCount -  PreTickCount);
    }

    return Rval;
}
#endif
#endif
#ifdef CONFIG_GPS_UBLOX6_GM6XX
int Process_GPS(UINT32 event, AMP_ENC_YUV_INFO_s* img)
{
    int Rval = 0;
    int status = 0;
    AMBA_FS_FILE *fheader;
    gps_data_t GpsData = {0 };
    char GPS_tmp[128] = {0};
    double speed = 999;
    double bearing = 999;
    double altitude = 999;
    double latitude = 999;
    if (VAgps_init == 0) {
        Rval = gps_drv_init( &G_MMPL);
        if (Rval < 0) {
        AmbaPrintColor(RED, "gps_drv_init init failed !! \n");
        AmbaKAL_TaskSleep(100);
        return -1;
        }
        gps_dev = gps_get_dev_list();
        if (gps_dev != NULL) {
            Rval = gps_open(gps_dev->name, &gps_id);
            if (Rval == 0) {
                VAgps_init = 1;
            }
            else{
                AmbaPrintColor(RED, "gps_open failed !! \n");
                AmbaKAL_TaskSleep(100);
                return -1;
            }
        }
    }
    if (gps_id == -1) {
        AmbaPrintColor(RED, "gps got no id !! \n");
        AmbaKAL_TaskSleep(100);
        return -1;
    }
    Rval = gps_get_data(gps_id, &GpsData);
    if (Rval == -1) {
        AmbaPrintColor(RED, "G_gps.dev[id].get_raw_data != NULL !! \n");
        return -1;
    }
    speed = GpsData.fix.speed *  (3.6);
    bearing = GpsData.fix.track;
    altitude = GpsData.fix.altitude;
    latitude = GpsData.fix.latitude;
    status = GpsData.status;
    if (status == 0 ) {
        AMBA_FS_FILE *ftest;
        AmbaPrintColor(RED, "gps no signal !! \n");
        AmbaKAL_TaskSleep(200);
        if (VAgps_firstW == 0)
        {
            /*
            AMBA_FS_FILE *fheader;
            sprintf(GPS_tmp, "GPS start !! \n");
            fheader = AmbaFS_fopen( "C:\\gps.txt", "w");
            if (fheader == NULL) {
                AmbaPrint("fheader gps Open fail");
            } else {
                AmbaFS_fwrite(GPS_tmp, 128, 1, fheader);
                AmbaFS_fclose(fheader);
                memset(GPS_tmp, 0, 128);
            }*/
            VAgps_firstW = 1;
        }
    } else {
        AMBA_FS_FILE *fw;
        AmbaPrintColor(RED, "gps status %d !! \n", status);
        AmbaPrintColor(RED, "gps altitude %f !! \n", altitude);
        AmbaPrintColor(RED, "gps latitude %f !! \n", latitude);
        AmbaPrintColor(RED, "gps speed %f !! \n", speed);
        AmbaPrintColor(RED, "gps bearing %f !! \n", bearing);
        AmbaKAL_TaskSleep(200);
        //AmbaKAL_TaskSleep(1000);
        //
        /*
        sprintf(GPS_tmp, "speed %f, bearing %f, altitude %f, latitude %f  \n", speed, bearing, altitude, latitude);
        fw = AmbaFS_fopen( "C:\\gps.txt", "a");
        if (fw == NULL) {
            AmbaPrint("gps Open fail");
        } else {
            AmbaFS_fwrite(GPS_tmp, 128, 1, fw);
            AmbaFS_fclose(fw);
        }
        */
    }
    return Rval;
}
#endif

#ifdef CONFIG_GPS_UBLOX6_GM6XX
#define MAX_GPS_INSTANCES   4
struct gps_obj_s {
    gps_dev_list_t  *dev_list;
    gps_dev_s   dev[MAX_GPS_INSTANCES];
    gps_update  hdl[MAX_GPS_INSTANCES];
    UINT32     total_gps_dev;
    AMBA_KAL_BYTE_POOL_t* pBytePool;
};
extern struct gps_obj_s G_gps;
extern gps_drv_hookup driver_list[];
#endif

/////////////////////////////////////////
/// external part
////////////////////////////////////////


int AmpUT_VATest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_VATest cmd: %s", argv[1]);
     if ( strcmp(argv[1],"fcmd") == 0 ) {
        int err = 0;
        extern int cardvws_process_FCMD(UINT32 event, AMP_ENC_YUV_INFO_s* img);
        err = AmbaVA_FrmHdlr_Register(AMP_ENC_EVENT_VCAP_2ND_YUV_READY, cardvws_process_FCMD);
        if (err == OK) {
            AmbaPrint("FCMD is on the road !\n");
        }
    } else if ( strcmp(argv[1],"llws") == 0 ) {
        int err = 0;
        extern int cardvws_process_LLWS(UINT32 event, AMBA_DSP_EVENT_CFA_3A_DATA_s* pdata);
        err = AmbaVA_TriAHdlr_Register(AMBA_DSP_EVENT_CFA_3A_DATA_READY, cardvws_process_LLWS);
        if (err == OK) {
            AmbaPrint("LLWS is on the road !\n");
        }
    } else if ( strcmp(argv[1],"mdy") == 0 ) {
        int err = 0;
        extern int Process_MDY(UINT32 event, AMP_ENC_YUV_INFO_s* img);
        err = AmbaVA_FrmHdlr_Register(AMP_ENC_EVENT_VCAP_2ND_YUV_READY, Process_MDY);
        if (err == OK) {
            AmbaPrint("MdY is on the road !\n");
        }
    } else if ( strcmp(argv[1],"mdae") == 0 ) {
        int err = 0;
        extern int Process_MDAE(UINT32 event, AMBA_DSP_EVENT_CFA_3A_DATA_s* pdata);
        AmbaVA_TriAHdlr_Register(AMBA_DSP_EVENT_CFA_3A_DATA_READY, Process_MDAE);
        if (err == OK) {
            AmbaPrint("mdae is on the road !\n");
        }
    }
#ifdef CONFIG_CC_CXX_SUPPORT
    else if ( strcmp(argv[1],"adas") == 0 ) {
        int err = 0;
        extern int Process_ADAS(UINT32 event, AMP_ENC_YUV_INFO_s* img);
        err = AmbaVA_FrmHdlr_Register(AMP_ENC_EVENT_VCAP_2ND_YUV_READY, Process_ADAS);
        if (err == OK) {
            AmbaPrint("adas is on the road !\n");
        }
    } else if ( strcmp(argv[1],"sdadas") == 0 ) {
        int err = 0;
        extern int Process_ADAS(UINT32 event, AMP_ENC_YUV_INFO_s* img);
        if (argc == 6) {
            float HoodLevel =     atof(argv[2]);
            float HorizonLevel =  atof(argv[3]);
            float HorizAngle =    atof(argv[4]);
            float VertAngle =     atof(argv[5]);
            Set_CarDVPosition_Pars(HoodLevel, HorizonLevel, HorizAngle, VertAngle);
            err = AmbaVA_FrmHdlr_Register((AMP_ENC_EVENT_START_NUM+99), Process_ADAS);
            if (err == OK) {
                AmbaPrint("sd read test adas is on the road !\n");
            }
        }
        else {
            AmbaPrint("sd read test adas need 4 parmas  !\n");
        }
    } else if ( strcmp(argv[1],"prt") == 0 ) {
        AMBA_ADAS_SCENE_PARAMS_s par = {0};
        AMBA_ADAS_SCENE_STATUS_s *scsta = 0;
        Amba_Adas_GetSceneParams( &par);
        AmbaPrintColor(GREEN,"AutoCalibrationActive %d !\n", par.AutoCalibrationActive);
        AmbaPrintColor(GREEN,"AutomaticCalibration %d !\n", par.AutomaticCalibration);
        AmbaPrintColor(GREEN,"HoodLevel %f !\n", par.HoodLevel);
        AmbaPrintColor(GREEN,"HorizonLevel %f !\n", par.HorizonLevel);
        Amba_Adas_GetSceneStatus( &scsta);
        AmbaPrintColor(GREEN,"AMBA_ADAS_SCENE_STATUS_s  HoodLevel %f !\n", scsta->HoodLevel);
        AmbaPrintColor(GREEN,"AMBA_ADAS_SCENE_STATUS_s HorizonLevel %f !\n", scsta->HorizonLevel);
        AmbaPrintColor(GREEN,"AMBA_ADAS_SCENE_STATUS_s IsCalibrationDetected %d !\n", scsta->IsCalibrationDetected);
        AmbaPrintColor(GREEN,"AMBA_ADAS_SCENE_STATUS_s HorizontalPan %d !\n", scsta->HorizontalPan);
    }
#ifdef CONFIG_BUILD_FOSS_OPENCV
    else if ( strcmp(argv[1],"tsd") == 0 ) {
        int err = 0;
        extern int Amba_ADAS_TrafficSignDetector_Test(UINT32 event, AMBAVA_YUV_INFO_s* Src);
        err = AmbaVA_FrmHdlr_Register(AMP_ENC_EVENT_VCAP_2ND_YUV_READY, Amba_ADAS_TrafficSignDetector_Test);
        if (err == OK) {
            AmbaPrint("tsd is on the road !\n");
        }
    }
    else if ( strcmp(argv[1], "cvregsd") == 0 ) {
        extern int AmbaSD_FrmReader_Register(UINT32 event, SD_FRMRdr_CB func);
        extern int Amba_ADAS_TrafficSignDetector_Test(UINT32 event, AMBAVA_YUV_INFO_s* Src);
        Amba_OpenCV_TestInit();
        AmbaSD_FrmReader_Register(AMP_ENC_EVENT_START_NUM+99, Amba_ADAS_TrafficSignDetector_Test);
    } else if ( strcmp(argv[1], "cvregfh") == 0 ) {
        extern int Amba_ADAS_TrafficSignDetector_Test(UINT32 event, AMBAVA_YUV_INFO_s* Src);
        Amba_OpenCV_TestInit();
        AmbaVA_FrmHdlr_Register(AMP_ENC_EVENT_START_NUM+99, Amba_ADAS_TrafficSignDetector_Test);
    } else if ( strcmp(argv[1], "cvinit") == 0 ) {
        {
            Test_Init = 1;
            AmbaPrintColor(RED,"Test_Init = 1  ");
        }
    }
#endif
#endif
#ifdef CONFIG_GPS_UBLOX6_GM6XX
    else if ( strcmp(argv[1],"gps") == 0 ) {
        extern int Process_GPS(UINT32 event, AMP_ENC_YUV_INFO_s* img);
        AmbaVA_FrmHdlr_Register(AMP_ENC_EVENT_VCAP_2ND_YUV_READY, Process_GPS);
        AmbaPrint("GPS is on the road !\n");
    }
#endif
    else if ( strcmp(argv[1],"release") == 0 ) {

        AmbaPrint("release done !\n");
    } else {
        AmbaPrint("Usage: t yuvt cmd ...");
        AmbaPrint("    cmd:");
        AmbaPrint("       fcmd: fcmd funciton register on yuv task");
        AmbaPrint("       llws: llws funciton register on 3a task");
        AmbaPrint("       adas: adas funciton register on yuv task");
        AmbaPrint("       mdy: motion detection funciton register on yuv task");
        AmbaPrint("       mdae: motion detection funciton register on 3a task");
    }
    return 0;
}

int AmpUT_VATestAdd(void)
{
    AmbaPrint("Adding AmpUT_VATestAdd");
    // hook command
    AmbaTest_RegisterCommand("vatest", AmpUT_VATest);
    AmbaTest_RegisterCommand("va", AmpUT_VATest);
    return AMP_OK;
}
