 /**
  * @file src/unittest/MWUnitTest/AmpUT_VideoMultiDec.c
  *
  * Unit Test of Multiple Channel Video Decode
  *
  * History:
  *    2014/06/23 - [phcheng] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include <player/Decode.h>
#include <player/VideoDec.h>
#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaUtility.h>
#include <util.h>
#include <AmbaLCD.h>
#include "AmbaCache.h"
#include "AmbaPLL.h"
#include "VideoDummyDemux.h"
#include "AmpUT_Display.h"
#include <display/Display.h>

#define VIDEODEC_RAW_SIZE (32<<20)  ///< 32MB raw buffer
#define VIDEODEC_RAW_DESC_NUM (128) ///< descriptor number = 128
#define VIDEODEC_MAX_CODEC_NUM (2)  ///< Max number of video codec in a pipe. Should not larger than AMP_VIDEODEC_CODEC_MAX_NUM in mw_VideoDec.h
// Global variables
// Video decoder
static void *AvcCodecBufferOri = NULL;                                  ///< Original buffer address of video codec module
static void *AvcRawBufferOri[VIDEODEC_MAX_CODEC_NUM] = { NULL };        ///< Original buffer address of video raw data (for each codec)
static void *AvcRawBuffer[VIDEODEC_MAX_CODEC_NUM] = { NULL };           ///< Aligned buffer address of video raw data (for each codec)
static void *AvcDescBufferOri[VIDEODEC_MAX_CODEC_NUM] = { NULL };       ///< Original buffer address of video descripter (for each codec)
static void *AvcDescBuffer[VIDEODEC_MAX_CODEC_NUM] = { NULL };          ///< Aligned buffer address of video descripter (for each codec)
static AMP_AVDEC_HDLR_s *AvcDecHdlr[VIDEODEC_MAX_CODEC_NUM] = { NULL }; ///< Video Codec Handler (for each codec)
static AMP_DEC_PIPE_HDLR_s *AvcDecPipeHdlr = NULL;                      ///< Video Pipe Handler that sync all video codecs
static UINT8 IsInit = 0;                                                ///< Whether the multiple video decoder is initialized

// Feature control
static AMP_AVDEC_PLAY_DIRECTION_e PlayDirection = AMP_VIDEO_PLAY_FW; ///< Play direction
static UINT32 PreFeedNum = 1024;  // Number of frames to feed before start

// Dummy demux
static AMP_UT_DUMMY_DEMUX_HDLR_s* DmxHdlr[VIDEODEC_MAX_CODEC_NUM];  ///< Dummy demux handler (for each codec)

// Display
#define UTMDispMsg(Fn, Str, ...) {AmbaPrint("UT_MULTI_DISPLAY [%s]: %s", Fn, Str, ##__VA_ARGS__);}
#define MULTI_DISP_CHANNEL_LCD (0)
#define MULTI_DISP_CHANNEL_TV  (1)

static UINT8 IsDisplayInit = 0;
static UINT8 MultiDispUTRunCVBS = 0;                ///< Running CVBS as TV
static AMP_DISP_HDLR_s *pMultiDispUTHdlrLCD = NULL; ///< LCD display handler. Should be got from AmpUT_Display. Do not create it.
static AMP_DISP_HDLR_s *pMultiDispUTHdlrTV = NULL;  ///< TV  display handler. Should be got from AmpUT_Display. Do not create it.
static AMP_DISP_WINDOW_HDLR_s *pMultiDispUTWindowHdlrLCD[VIDEODEC_MAX_CODEC_NUM] = { NULL };
static AMP_DISP_WINDOW_HDLR_s *pMultiDispUTWindowHdlrTV[VIDEODEC_MAX_CODEC_NUM] = { NULL };
static UINT8 MultiDispLCDStatus = 0;     //0:disable 1:off 2:on
static UINT8 MultiDispTVStatus = 0;      //0:disable 1:off 2:on

static int AmpUT_MultiDisplay_Init(void)
{
    UTMDispMsg(__FUNCTION__, "Begin");

    if (IsDisplayInit == 0) {
        /* Init Display & Register LCD driver */
        if (AmpUT_Display_Init() != AMP_OK) {
            AmbaPrint("%s:%u AmpUT_Display_Init failed.", __FUNCTION__, __LINE__);
            return -1;
        }

        /* Get DCHAN display handler */
        if (AmpUT_Display_GetDisplayHandler(AMP_DISP_CHANNEL_DCHAN, &pMultiDispUTHdlrLCD) != AMP_OK) {
            UTMDispMsg(__FUNCTION__, "AmpUT_Display_GetDisplayHandler() LCD Fail");
            return -1;
        } else if (pMultiDispUTHdlrLCD == NULL) {
            UTMDispMsg(__FUNCTION__, "AmpUT_Display_GetDisplayHandler() LCD Fail");
            return -1;
        }

        /* Get FCHAN display handler */
        if (AmpUT_Display_GetDisplayHandler(AMP_DISP_CHANNEL_FCHAN, &pMultiDispUTHdlrTV) != AMP_OK) {
            UTMDispMsg(__FUNCTION__, "AmpUT_Display_GetDisplayHandler() TV Fail");
            return -1;
        } else if (pMultiDispUTHdlrTV == NULL) {
            UTMDispMsg(__FUNCTION__, "AmpUT_Display_GetDisplayHandler() TV Fail");
            return -1;
        }
        MultiDispLCDStatus = 1;
        MultiDispTVStatus = 1;

        IsDisplayInit = 1;
        UTMDispMsg(__FUNCTION__, "Success");
    } else {
        UTMDispMsg(__FUNCTION__, "Already done");
    }

    return 0;
}

static int AmpUT_MultiDisplay_Start(UINT8 Channel)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    UTMDispMsg(__FUNCTION__, "Begin");
    if (IsDisplayInit == 0) {
        UTMDispMsg(__FUNCTION__, "call AmpUT_MultiDisplay_Init()");
        Rval = AmpUT_MultiDisplay_Init();
    }
    if (Channel == MULTI_DISP_CHANNEL_LCD) {
        if (MultiDispLCDStatus == 1) {
            Rval = AmpDisplay_Start(pMultiDispUTHdlrLCD);
            MultiDispLCDStatus = 2;
        } else if (MultiDispLCDStatus == 2) {
            UTMDispMsg(__FUNCTION__, "LCD started");
            Rval = AMP_OK;
        } else {
            UTMDispMsg(__FUNCTION__, "LCD disabled");
            Rval = AMP_OK;
        }
    } else if (Channel == MULTI_DISP_CHANNEL_TV) {
        if (MultiDispTVStatus == 1) {
            Rval = AmpDisplay_Start(pMultiDispUTHdlrTV);
            MultiDispTVStatus = 2;
        } else if (MultiDispTVStatus == 2) {
            UTMDispMsg(__FUNCTION__, "TV started");
            Rval = AMP_OK;
        } else {
            UTMDispMsg(__FUNCTION__, "TV disabled");
            Rval = AMP_OK;
        }
    } else {
        UTMDispMsg(__FUNCTION__, "Please enter device channel: dchan / fchan");
        return Rval;
    }
    UTMDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return 0;
}

static int AmpUT_MultiDisplay_Window_Create(UINT8 Channel,
                                            UINT32 windowID,
                                            AMP_DISP_WINDOW_CFG_s *pWindowCfg)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;

    UTMDispMsg(__FUNCTION__, "Begin");

    if (pWindowCfg == NULL) {
        UTMDispMsg(__FUNCTION__, "pWindowCfg invalid");
        return Rval;
    }
    if (windowID >= VIDEODEC_MAX_CODEC_NUM) {
        UTMDispMsg(__FUNCTION__, "windowID invalid");
        return Rval;
    }

    if (Channel == MULTI_DISP_CHANNEL_LCD) {
        if (pMultiDispUTWindowHdlrLCD[windowID] != NULL) {
            Rval = AmpDisplay_SetWindowCfg(pMultiDispUTWindowHdlrLCD[windowID], pWindowCfg);
//            UTMDispMsg(__FUNCTION__, "LCD window created, do update");
        } else {
            Rval = AmpDisplay_CreateWindow(pMultiDispUTHdlrLCD, pWindowCfg, &pMultiDispUTWindowHdlrLCD[windowID]);
        }
    } else if (Channel == MULTI_DISP_CHANNEL_TV) {
        if (pMultiDispUTWindowHdlrTV[windowID] != NULL) {
            Rval = AmpDisplay_SetWindowCfg(pMultiDispUTWindowHdlrTV[windowID], pWindowCfg);
//            UTMDispMsg(__FUNCTION__, "TV window created, do update");
        } else {
            if (MultiDispUTRunCVBS) {
                pWindowCfg->TargetAreaOnPlane.Width = 720;
                pWindowCfg->TargetAreaOnPlane.Height = 480;
            }
            Rval = AmpDisplay_CreateWindow(pMultiDispUTHdlrTV, pWindowCfg, &pMultiDispUTWindowHdlrTV[windowID]);
        }
    } else {
        UTMDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
    }
    UTMDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

static int AmpUT_MultiDisplay_Window_Delete(UINT8 Channel,
                                            UINT32 windowID)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;

    UTMDispMsg(__FUNCTION__, "Begin");

    if (windowID >= VIDEODEC_MAX_CODEC_NUM) {
        UTMDispMsg(__FUNCTION__, "windowID invalid");
        return Rval;
    }

    if (Channel == MULTI_DISP_CHANNEL_LCD) {
        if (pMultiDispUTWindowHdlrLCD[windowID] != NULL) {
            Rval = AmpDisplay_DeleteWindow(pMultiDispUTWindowHdlrLCD[windowID]);
            pMultiDispUTWindowHdlrLCD[windowID] = NULL;
        }
    } else if (Channel == MULTI_DISP_CHANNEL_TV) {
        if (pMultiDispUTWindowHdlrTV[windowID] != NULL) {
            Rval = AmpDisplay_DeleteWindow(pMultiDispUTWindowHdlrTV[windowID]);
            pMultiDispUTWindowHdlrTV[windowID] = NULL;
        }
    } else {
        UTMDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
    }
    UTMDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));

    return Rval;
}

static int AmpUT_MultiDisplay_Act_Window(UINT8 Channel,
                                         UINT32 windowID)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AMP_DISP_WINDOW_HDLR_s *pWindowHdlr;
    AMP_DISP_HDLR_s *pDispHdlr;
    UINT8 DispStatus;

    UTMDispMsg(__FUNCTION__, "Begin");

    if (Channel == MULTI_DISP_CHANNEL_LCD) {
        pWindowHdlr = pMultiDispUTWindowHdlrLCD[windowID];
        pDispHdlr = pMultiDispUTHdlrLCD;
        DispStatus = MultiDispLCDStatus;
    } else if (Channel == MULTI_DISP_CHANNEL_TV) {
        pWindowHdlr = pMultiDispUTWindowHdlrTV[windowID];
        pDispHdlr = pMultiDispUTHdlrTV;
        DispStatus = MultiDispTVStatus;
    } else {
        UTMDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
        return Rval;
    }
    if (pWindowHdlr == NULL) {
        UTMDispMsg(__FUNCTION__, "Error window:%d not created", Channel);
        return Rval;
    }
    if (DispStatus == 0) {
        UTMDispMsg(__FUNCTION__, "Disp:%d disabled not act", Channel);
        Rval = AMP_OK;
    } else if (DispStatus == 1) {
    #if 1
        UTMDispMsg(__FUNCTION__, "Disp:%d off, Config it first", Channel);
        Rval = AMP_OK;
    #else
        UTMDispMsg(__FUNCTION__, "Disp:%d off, starting...", Channel);
        Rval = AmpUT_MultiDisplay_Start(Channel);
        Rval = AmpDisplay_SetWindowActivateFlag(pWindowHdlr, 1);
        Rval |= AmpDisplay_Update(pDispHdlr);
        Rval = AMP_OK;
    #endif
    } else {
        Rval = AmpDisplay_SetWindowActivateFlag(pWindowHdlr, 1);
        Rval |= AmpDisplay_Update(pDispHdlr);
    }
    UTMDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

static int AmpUT_MultiDisplay_GetInfo(UINT8 Channel, AMP_DISP_INFO_s *pOutDispInfo)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AMP_DISP_HDLR_s *pDispHdlr;
    UTMDispMsg(__FUNCTION__, "Begin");
    if (pOutDispInfo == NULL) {
        UTMDispMsg(__FUNCTION__, "Error pOutWindowCfg:0x%x", pOutDispInfo);
        return Rval;
    }

    if (Channel == MULTI_DISP_CHANNEL_LCD) {
        pDispHdlr = pMultiDispUTHdlrLCD;
    } else if (Channel == MULTI_DISP_CHANNEL_TV) {
        pDispHdlr = pMultiDispUTHdlrTV;
    } else {
        UTMDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
        return Rval;
    }
    Rval = AmpDisplay_GetInfo(pDispHdlr, pOutDispInfo);
    UTMDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

/**
 * Create display handlers and windows on Vout.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_ConfigDisplay(void)
{
    AMP_DISP_WINDOW_CFG_s DispWindow;
    UINT32 T = 0;
    int Er = NG;

    if (AmpUT_MultiDisplay_Init() == NG) {
        return Er;
    }

    memset(&DispWindow, 0, sizeof(AMP_DISP_WINDOW_CFG_s));

    DispWindow.CropArea.Width = 0;
    DispWindow.CropArea.Height = 0;
    DispWindow.CropArea.X = 0;
    DispWindow.CropArea.Y = 0;
    DispWindow.TargetAreaOnPlane.Width = 960;
    DispWindow.TargetAreaOnPlane.Height = 360;
    DispWindow.TargetAreaOnPlane.X = 0;
    DispWindow.TargetAreaOnPlane.Y = 60;
    DispWindow.Source = AMP_DISP_DEC;
    DispWindow.SourceDesc.Dec.DecHdlr = AvcDecHdlr;
    for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
        if (AmpUT_MultiDisplay_Window_Create(MULTI_DISP_CHANNEL_LCD, T, &DispWindow) == NG) {
            return Er;
        }
    }

#ifdef  DEC_1080
    DispWindow.TargetAreaOnPlane.Width = 1920;
    DispWindow.TargetAreaOnPlane.Height = 1080;
#else
    DispWindow.TargetAreaOnPlane.Width = 3840;
    DispWindow.TargetAreaOnPlane.Height = 2160;
#endif
    for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
        if (AmpUT_MultiDisplay_Window_Create(MULTI_DISP_CHANNEL_TV, T, &DispWindow) == NG) {
            return Er;
        }
    }
    return 0;
}

/**
 * Set display channel configuration.
 *
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_ConfigDisplayCh(char **argv)
{
    UINT32 T = 0;
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    if (atoi(argv[2]) == 1) {
        // Start up TV handler
        Er = AmpUT_MultiDisplay_Start(MULTI_DISP_CHANNEL_TV);
        if (Er != 0) {
            ErrMsg = "AmpUT_MultiDisplay_Start failed";
            goto ReturnError;
        }
        for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
            Er = AmpUT_MultiDisplay_Act_Window(MULTI_DISP_CHANNEL_TV, T);
            if (Er != 0) {
                ErrMsg = "AmpUT_MultiDisplay_Act_Window failed";
                goto ReturnError;
            }
        }
    } else {
        // Start up LCD handler
        Er = AmpUT_MultiDisplay_Start(MULTI_DISP_CHANNEL_LCD);
        if (Er != 0) {
            ErrMsg = "AmpUT_MultiDisplay_Start failed";
            goto ReturnError;
        }
        for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
            Er = AmpUT_MultiDisplay_Act_Window(MULTI_DISP_CHANNEL_LCD, T);
            if (Er != 0) {
                ErrMsg = "AmpUT_MultiDisplay_Act_Window failed";
                goto ReturnError;
            }
        }
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Callback handler of video codec.
 *
 * @param [in] hdlr         Video codec handler
 * @param [in] event        Event ID
 * @param [in] info         Event info
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_CodecCB(void *hdlr,
                                       UINT32 event,
                                       void* info)
{
    UINT32 I = 0;
    UINT32 FrameCount = 0;
    //AmbaPrint("%s on Event: 0x%x ", __FUNCTION__, event);

    switch (event) {
    case AMP_DEC_EVENT_FIRST_FRAME_DISPLAYED:
        AmbaPrint("[AmpUT - VideoMultiDec] <DecCallback> AMP_DEC_EVENT_FIRST_FRAME_DISPLAYED");
        break;
    case AMP_DEC_EVENT_PLAYBACK_EOS:
        AmbaPrint("[AmpUT - VideoMultiDec] <DecCallback> AMP_DEC_EVENT_PLAYBACK_EOS");
        break;
    case AMP_DEC_EVENT_STATE_CHANGED:
        AmbaPrint("[AmpUT - VideoMultiDec] <DecCallback> AMP_DEC_EVENT_STATE_CHANGED");
        break;
    case AMP_DEC_EVENT_DATA_UNDERTHRSHOLDD:
        for (I = 0; I < VIDEODEC_MAX_CODEC_NUM; ++I) {
            if (DmxHdlr[I]->IsInit) {
                AmpUT_Video_DummyDmx_Lock(DmxHdlr[I], AMBA_KAL_WAIT_FOREVER);
                if (DmxHdlr[I]->AvcDecHdlr == (AMP_AVDEC_HDLR_s *)hdlr) {
                    FrameCount = DmxHdlr[I]->VideoDecFrmCnt;
                    AmpUT_Video_DummyDmx_UnLock(DmxHdlr[I]);
                    break;
                }
                AmpUT_Video_DummyDmx_UnLock(DmxHdlr[I]);
            }
        }

        if (I >= VIDEODEC_MAX_CODEC_NUM) {
            AmbaPrintColor(RED, "%s: Handler not found (0x%08X)", __FUNCTION__, hdlr);
            return -1;
        }

        if (FrameCount != VIDEODEC_FRAME_COUNT_EOS) {
            // It's necessary to give mutex before calling this function
            AmpUT_Video_DummyDmx_Feed(DmxHdlr[I], 4); // feed 4 frames;
        }
        break;
    default:
        break;
    }

    return 0;
}

/**
 * Initialize multiple video decode.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_Init(UINT32 disableVoutAtInit)
{
    UINT8 HdlrGroupID;
    int Er = 0;
    UINT32 T = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    if (IsInit) {
        AmbaPrint("%s: Already initialized", __FUNCTION__);
        goto ReturnSuccess;
    }

    {
        extern int AmbaLoadDSPuCode(void);
        AmbaLoadDSPuCode(); /* Load uCodes from NAND */
    }


    // Get from Isaac for VideoHISO's implementation
    {
        AMBA_OPMODE_CLK_PARAM_s ClkParam = {0};
        ClkParam.CoreClkFreq = 350000000;
        ClkParam.IdspClkFreq = 432000000/*400000000*/;
        ClkParam.CortexClkFreq = 504000000;
        Er = AmbaPLL_SetOpMode(&ClkParam);
        if (Er != 0) {
            ErrMsg = "AmbaPLL_SetOpMode failed";
            goto ReturnError;
        }
    }

    /** Initialize codec module */
    {
        AMP_VIDEODEC_INIT_CFG_s CodecInitCfg;

        if (AvcCodecBufferOri == NULL) {
            // Get the default codec module settings
            AmpVideoDec_GetInitDefaultCfg(&CodecInitCfg);
            // Customize the module settings
                // You can add some codes here ...
            // Allocate memory for codec module
            Er = AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(CodecInitCfg.Buf), &AvcCodecBufferOri, CodecInitCfg.BufSize, 1 << 5);
            if (Er != AMP_OK) {
                AmbaPrint("%s:%u Failed to allocate memory.", __FUNCTION__, __LINE__);
                ErrMsg = "AmpUtil_GetAlignedPool failed";
                goto ReturnError;
            }
            // Configure the initial settings
            Er = AmpVideoDec_Init(&CodecInitCfg);
            if (Er != AMP_OK) {
                ErrMsg = "AmpVideoDec_Init failed";
                goto ReturnError;
            }
        }
    }

    /** Create codec handler */
    Er = AmpVideoDec_GetFreeHdlrGroupID(&HdlrGroupID);
    if (Er != AMP_OK) {
        AmbaPrint("%s:%u Failed to get handler group ID.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpVideoDec_GetFreeHdlrGroupID failed";
        goto ReturnError;
    }
    for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
        AMP_VIDEODEC_CFG_s CodecCfg;
        // Get the default codec handler settings
        AmpVideoDec_GetDefaultCfg(&CodecCfg);
        CodecCfg.HandlerGroupID = HdlrGroupID;
        if (AvcRawBufferOri[T] == NULL) {
            // Allocate memory for codec raw buffer
            Er = AmpUtil_GetAlignedPool(&G_MMPL, &AvcRawBuffer[T], &AvcRawBufferOri[T], VIDEODEC_RAW_SIZE, 1 << 6);
            if (Er != AMP_OK) {
                AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
                ErrMsg = "AmpUtil_GetAlignedPool failed";
                goto ReturnError;
            }
        }
        CodecCfg.RawBuffer = (char*) AvcRawBuffer[T];
        CodecCfg.RawBufferSize = VIDEODEC_RAW_SIZE;
        if (AvcDescBufferOri[T] == NULL) {
            Er = AmpUtil_GetAlignedPool(&G_MMPL, &AvcDescBuffer[T], &AvcDescBufferOri[T], VIDEODEC_RAW_DESC_NUM * sizeof(AMP_BITS_DESC_s), 1 << 5);
            if (Er != AMP_OK) {
                AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
                ErrMsg = "AmpUtil_GetAlignedPool failed";
                goto ReturnError;
            }
        }
        CodecCfg.DescBuffer = (char*) AvcDescBuffer[T];
        CodecCfg.NumDescBuffer = VIDEODEC_RAW_DESC_NUM;
        // Customize the handler settings
        CodecCfg.CbCodecEvent = AmpUT_VideoMultiDec_CodecCB;
        // TDB feature config
        CodecCfg.Feature.MaxVoutWidth = 2560; // In A9, multiple channel decode supports 1280x720. In A12, dual decode supports 2560x1440.
        CodecCfg.Feature.MaxVoutHeight = 1440; // In A9, multiple channel decode supports 1280x720. In A12, dual decode supports 2560x1440.
        // Create a codec handler, and configure the initial settings
        Er = AmpVideoDec_Create(&CodecCfg, &AvcDecHdlr[T]);
        if (Er != AMP_OK) {
            AmbaPrint("%s:%u Cannot create video codec.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpVideoDec_Create failed";
            goto ReturnError;
        }
        AmbaPrint("AvcDecHdlr = 0x%08X", AvcDecHdlr[T]);
    }

    /** Create decoder manager */
    {
        AMP_DEC_PIPE_CFG_s PipeCfg;

        // Get the default decoder manager settings
        AmpDec_GetDefaultCfg(&PipeCfg);
        // Customize the manager settings
        //PipeCfg.cbEvent
        for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
            PipeCfg.Decoder[T] = AvcDecHdlr[T];
        }
        PipeCfg.NumDecoder = VIDEODEC_MAX_CODEC_NUM;
        PipeCfg.Type = AMP_DEC_VID_PIPE;
        // Create a decoder manager, and insert the codec handler into the manager
        Er = AmpDec_Create(&PipeCfg, &AvcDecPipeHdlr);
        if (Er != AMP_OK) {
            ErrMsg = "AmpDec_Create failed";
            goto ReturnError;
        }
    }

    /** Create LCD handler, TV handler, LCD window, TV window */
    Er = AmpUT_VideoMultiDec_ConfigDisplay();
    if (Er != AMP_OK) {
        ErrMsg = "AmpUT_VideoDec_ConfigDisplay failed";
        goto ReturnError;
    }

    /** Activate decoder manager */
    // Activate the decoder manager and all the codec handlers in the manager
    Er = AmpDec_Add(AvcDecPipeHdlr);
    if (Er != AMP_OK) {
        ErrMsg = "AmpDec_Add failed";
        goto ReturnError;
    }

    /** Dummy mux */
    // Initialize dummy demuxer
    Er = AmpUT_Video_DummyDmx_Init();
    if (Er != 0) {
        AmbaPrintColor(RED, "%s:%u Failed to init demuxer.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpUT_Video_DummyDmx_Init failed";
        goto ReturnError;
    }

    // Create a dummy demuxer
    {
        AMP_UT_DUMMY_DEMUX_CREATE_CFG_s DummyDemuxCfg;
        SET_ZERO(DummyDemuxCfg);
        for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
            DummyDemuxCfg.AvcDecHdlr = AvcDecHdlr[T];
            DummyDemuxCfg.AvcRawBuffer = AvcRawBuffer[T];
            DummyDemuxCfg.AvcRawLimit = (UINT8*) ((UINT32) AvcRawBuffer[T] + VIDEODEC_RAW_SIZE - 1);
            Er = AmpUT_Video_DummyDmx_Create(&DummyDemuxCfg, &DmxHdlr[T]);
            if (Er != 0) {
                AmbaPrintColor(RED, "%s:%u Failed to create demuxer.", __FUNCTION__, __LINE__);
                ErrMsg = "AmpUT_Video_DummyDmx_Create failed";
                goto ReturnError;
            }
        }
    }

    AmbaKAL_TaskSleep(17);

    /** Enable Vout */
    if ((disableVoutAtInit&1) == 0) {
        // Start up LCD handler
        Er = AmpUT_MultiDisplay_Start(MULTI_DISP_CHANNEL_LCD);
        if (Er != AMP_OK) {
            ErrMsg = "AmpUT_MultiDisplay_Start failed";
            goto ReturnError;
        }
    }
    if ((disableVoutAtInit&2) == 0) {
        // Start up TV handler
        Er = AmpUT_MultiDisplay_Start(MULTI_DISP_CHANNEL_TV);
        if (Er != AMP_OK) {
            ErrMsg = "AmpUT_MultiDisplay_Start failed";
            goto ReturnError;
        }
    }

    IsInit = 1;

ReturnSuccess:
    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Start decoding multiple videos.
 *
 * @param [in] argc         Command line argument count
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_Start(int argc, char** argv)
{
    AMP_AVDEC_TRICKPLAY_s Trick = { 0 };
    AMP_VIDEODEC_DISPLAY_s Display = { 0 };
    char AvcFn[VIDEODEC_MAX_CODEC_NUM][80];    // Path of .h264 file in ASCII format
    char IdxFn[VIDEODEC_MAX_CODEC_NUM][80];    // Path of .nhnt file in ASCII format
    char UdtFn[VIDEODEC_MAX_CODEC_NUM][80];    // Path of .udta file in ASCII format
    AMP_DISP_WINDOW_CFG_s WinCfg;
    AMP_DISP_INFO_s DispInfo;
    int InputValue;         // A temporary space storing input value
    char* VideoPath = NULL; // The path of videos
    UINT16 VideoWidth = 0;  // The width of videos. All of them should have same width.
    UINT16 VideoHeight = 0; // The height of videos. All of them should have same height.
    UINT32 StartTime = 0;   // Start from the beginning
    UINT32 Speed = 0x100;   // Normal speed
    UINT32 Direction = 0;   // Play forwards
    AMP_ROTATION_e Rotation[VIDEODEC_MAX_CODEC_NUM];
    UINT32 T;
    UINT32 ID;
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    /** Get and check user input */
    if (argc < 3) {
        AmbaPrint("%s: Insufficient parameters", __FUNCTION__);
        goto ReturnError;
    } else if (argc > 12) {
        AmbaPrint("%s: To many parameters", __FUNCTION__);
        goto ReturnError;
    }

    VideoPath = argv[2];
    if (strcmp(VideoPath, "") == 0) {
        AmbaPrint("%s: Please enter the path of video files.", __FUNCTION__);
        goto ReturnError;
    }

    // If input width = 0, then VideoWidth = DmxHdlr[0]->AvcFileWidth
    VideoWidth = DmxHdlr[0]->AvcFileWidth;
    if (argc >= 4) {
        InputValue = atoi(argv[3]);
        if (InputValue < 0) {
            AmbaPrint("%s: Illegal value of width (%d)", __FUNCTION__, InputValue);
            goto ReturnError;
        } else if (InputValue > 0) {
            VideoWidth = (UINT32)InputValue;
        }
    }

    // If input height = 0, then VideoHeight = DmxHdlr[0]->AvcFileHeight
    VideoHeight = DmxHdlr[0]->AvcFileHeight;
    if (argc >= 5) {
        InputValue = atoi(argv[4]);
        if (InputValue < 0) {
            AmbaPrint("%s: Illegal value of height (%d)", __FUNCTION__, InputValue);
            goto ReturnError;
        } else if (InputValue > 0) {
            VideoHeight = (UINT32)InputValue;
        }
    }

    if (argc >= 6) {
        InputValue = atoi(argv[5]);
        if (InputValue < 0) {
            AmbaPrint("%s: Illegal value of start time (%d)", __FUNCTION__, InputValue);
            goto ReturnError;
        }
        StartTime = (UINT32)InputValue;
    }

    if (argc >= 7) {
        InputValue = atoi(argv[6]);
        Speed = (UINT32)InputValue;

        if (Speed == 0) {
            AmbaPrint("%s: Speed cannot be 0. Set to normal speed", __FUNCTION__);
            Speed = 0x100;
        }
    }

    if (argc >= 8) {
        InputValue = atoi(argv[7]);
        if (InputValue == 1) {
            AmbaPrint("%s: Backward in multiple channel decode is not supported", __FUNCTION__, InputValue);
            goto ReturnError;
        }
        if (InputValue != 0) {
            AmbaPrint("%s: Illegal value of direction (%d)", __FUNCTION__, InputValue);
            goto ReturnError;
        }
        Direction = (UINT32)InputValue;
    }

    // Set direction before feeding frames
    if (Direction == 0) {
        PlayDirection = AMP_VIDEO_PLAY_FW;
    } else {
        PlayDirection = AMP_VIDEO_PLAY_BW;
    }

    SET_ZERO(Rotation);
    for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
        if (argc >= (T + 9)) {
            InputValue = atoi(argv[T + 8]);
            if (InputValue < 0 || InputValue > 7) {
                AmbaPrint("%s: Illegal value of rotation (%d)", __FUNCTION__, InputValue);
                goto ReturnError;
            }
            Rotation[T] = (AMP_ROTATION_e) InputValue;
        }
    }

    /** Get target file name (.h264, .nhnt) */
    for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
        AmbaPrint("%s: target file - %s_%02u", __FUNCTION__, VideoPath, T);
        sprintf(AvcFn[T], "%s_%02u.h264", VideoPath, T);
        sprintf(IdxFn[T], "%s_%02u.nhnt", VideoPath, T);
        sprintf(UdtFn[T], "%s_%02u.udta", VideoPath, T);
    }

    /** Open target file */
    for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
        // Open file by dummy demuxer
        if (AmpUT_Video_DummyDmx_Open(DmxHdlr[T], AvcFn[T], IdxFn[T], UdtFn[T]) != 0) {
            AmbaPrint("%s: Failed to open demux[%u]", __FUNCTION__, T);
            ErrMsg = "AmpUT_Video_DummyDmx_Open failed";
            goto ReturnError;
        }
    }

    /** Pre-feed */
    AmbaPrint("%s: StartTime = %d  Speed = 0x%3x", __FUNCTION__, StartTime, Speed);
    for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
        Er = AmpUT_Video_DummyDmx_Start(DmxHdlr[T], StartTime, PlayDirection, Speed, PreFeedNum);
        if (Er != 0) {
            ErrMsg = "AmpUT_Video_DummyDmx_Start failed";
            goto ReturnError;
        }
    }

    /** Configure fading settings */
    // Not supported in multiple channel decode

    /** Configure Display settings */
    Display.SrcWidth = VideoWidth;
    Display.SrcHeight = VideoHeight;
    Display.AOI.X = 0;
    Display.AOI.Y = 0;
    Display.AOI.Width = Display.SrcWidth;
    Display.AOI.Height = Display.SrcHeight;

    /** Re-config window */
    // Get display info
    Er = AmpUT_MultiDisplay_GetInfo(MULTI_DISP_CHANNEL_LCD, &DispInfo);
    if (Er != 0) {
        ErrMsg = "AmpUT_MultiDisplay_GetInfo failed";
        goto ReturnError;
    }
    if (VIDEODEC_MAX_CODEC_NUM >= 1) {
        ID = 0;
        // Get window settings
        AmpDisplay_GetDefaultWindowCfg(&WinCfg);
        // Rotate & Flip settings
        WinCfg.SourceDesc.Dec.Rotate = Rotation[ID];

        {   // Center align the image
            float WidthPxlRatioOnDev, HeightPxlRatioOnDev;
            if (WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_0 ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_0_HORZ_FLIP ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_180 ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_180_HORZ_FLIP) {
                WidthPxlRatioOnDev = Display.SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
                HeightPxlRatioOnDev = Display.SrcHeight;
            } else {
                WidthPxlRatioOnDev = Display.SrcHeight / DispInfo.DeviceInfo.DevPixelAr;
                HeightPxlRatioOnDev = Display.SrcWidth;
            }
            if (WidthPxlRatioOnDev/HeightPxlRatioOnDev >
            (float)DispInfo.DeviceInfo.VoutWidth/2/((float)DispInfo.DeviceInfo.VoutHeight)) {
                // black bar at top and bottom
                WinCfg.TargetAreaOnPlane.Width = DispInfo.DeviceInfo.VoutWidth / 2;
                WinCfg.TargetAreaOnPlane.Height = (UINT32)((WinCfg.TargetAreaOnPlane.Width*HeightPxlRatioOnDev)/WidthPxlRatioOnDev);
                WinCfg.TargetAreaOnPlane.X = 0;
                WinCfg.TargetAreaOnPlane.Y = (DispInfo.DeviceInfo.VoutHeight - WinCfg.TargetAreaOnPlane.Height) / 2;
            } else {
                // black bar at left and right
                WinCfg.TargetAreaOnPlane.Height = DispInfo.DeviceInfo.VoutHeight;
                WinCfg.TargetAreaOnPlane.Width = (UINT32)((WinCfg.TargetAreaOnPlane.Height*WidthPxlRatioOnDev)/HeightPxlRatioOnDev);
                WinCfg.TargetAreaOnPlane.X = DispInfo.DeviceInfo.VoutWidth / 2 - WinCfg.TargetAreaOnPlane.Width;
                WinCfg.TargetAreaOnPlane.Y = 0;
            }
            // LCD Align to 6
            {
                UINT32 Align = 6;
                WinCfg.TargetAreaOnPlane.X = WinCfg.TargetAreaOnPlane.X / Align * Align;
                WinCfg.TargetAreaOnPlane.Y = WinCfg.TargetAreaOnPlane.Y / Align * Align;
                WinCfg.TargetAreaOnPlane.Width = (WinCfg.TargetAreaOnPlane.Width + Align - 1) / Align * Align;
                WinCfg.TargetAreaOnPlane.Height = (WinCfg.TargetAreaOnPlane.Height + Align - 1) / Align * Align;
            }
            AmbaPrintColor(GREEN, "LCD Hdlr%u = (%d,%d,%d,%d) Rotation = %u", ID, WinCfg.TargetAreaOnPlane.X, WinCfg.TargetAreaOnPlane.Y,
                    WinCfg.TargetAreaOnPlane.Width, WinCfg.TargetAreaOnPlane.Height, WinCfg.SourceDesc.Dec.Rotate);
        }
        // Configure & update window settings
        WinCfg.Source = AMP_DISP_DEC;
        WinCfg.SourceDesc.Dec.DecHdlr = AvcDecHdlr[ID];
        Er = AmpUT_MultiDisplay_Window_Create(MULTI_DISP_CHANNEL_LCD, ID, &WinCfg);
        if (Er != 0) {
            ErrMsg = "AmpUT_MultiDisplay_Window_Create failed";
            goto ReturnError;
        }
        // active and Update window
        Er = AmpUT_MultiDisplay_Act_Window(MULTI_DISP_CHANNEL_LCD, ID);
        if (Er != 0) {
            ErrMsg = "AmpUT_MultiDisplay_Act_Window failed";
            goto ReturnError;
        }
    }

    if (VIDEODEC_MAX_CODEC_NUM >= 2) {
        ID = 1;
        // Get window settings
        AmpDisplay_GetDefaultWindowCfg(&WinCfg);
        // Rotate & Flip settings
        WinCfg.SourceDesc.Dec.Rotate = Rotation[ID];

        {   // Center align the image
            float WidthPxlRatioOnDev, HeightPxlRatioOnDev;
            if (WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_0 ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_0_HORZ_FLIP ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_180 ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_180_HORZ_FLIP) {
                WidthPxlRatioOnDev = Display.SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
                HeightPxlRatioOnDev = Display.SrcHeight;
            } else {
                WidthPxlRatioOnDev = Display.SrcHeight / DispInfo.DeviceInfo.DevPixelAr;
                HeightPxlRatioOnDev = Display.SrcWidth;
            }
            if (WidthPxlRatioOnDev/HeightPxlRatioOnDev >
            (float)DispInfo.DeviceInfo.VoutWidth/2/((float)DispInfo.DeviceInfo.VoutHeight)) {
                // black bar at top and bottom
                WinCfg.TargetAreaOnPlane.Width = DispInfo.DeviceInfo.VoutWidth / 2;
                WinCfg.TargetAreaOnPlane.Height = (UINT32)((WinCfg.TargetAreaOnPlane.Width*HeightPxlRatioOnDev)/WidthPxlRatioOnDev);
                WinCfg.TargetAreaOnPlane.X = DispInfo.DeviceInfo.VoutWidth / 2;
                WinCfg.TargetAreaOnPlane.Y = (DispInfo.DeviceInfo.VoutHeight - WinCfg.TargetAreaOnPlane.Height) / 2;
            } else {
                // black bar at left and right
                WinCfg.TargetAreaOnPlane.Height = DispInfo.DeviceInfo.VoutHeight;
                WinCfg.TargetAreaOnPlane.Width = (UINT32)((WinCfg.TargetAreaOnPlane.Height*WidthPxlRatioOnDev)/HeightPxlRatioOnDev);
                WinCfg.TargetAreaOnPlane.X = DispInfo.DeviceInfo.VoutWidth / 2;
                WinCfg.TargetAreaOnPlane.Y = 0;
            }
            // LCD Align to 6
            {
                UINT32 Align = 6;
                WinCfg.TargetAreaOnPlane.X = WinCfg.TargetAreaOnPlane.X / Align * Align;
                WinCfg.TargetAreaOnPlane.Y = WinCfg.TargetAreaOnPlane.Y / Align * Align;
                WinCfg.TargetAreaOnPlane.Width = (WinCfg.TargetAreaOnPlane.Width + Align - 1) / Align * Align;
                WinCfg.TargetAreaOnPlane.Height = (WinCfg.TargetAreaOnPlane.Height + Align - 1) / Align * Align;
            }
            AmbaPrintColor(GREEN, "LCD Hdlr%u = (%d,%d,%d,%d) Rotation = %u", ID, WinCfg.TargetAreaOnPlane.X, WinCfg.TargetAreaOnPlane.Y,
                    WinCfg.TargetAreaOnPlane.Width, WinCfg.TargetAreaOnPlane.Height, WinCfg.SourceDesc.Dec.Rotate);
        }
        // Configure & update window settings
        WinCfg.Source = AMP_DISP_DEC;
        WinCfg.SourceDesc.Dec.DecHdlr = AvcDecHdlr[ID];
        Er = AmpUT_MultiDisplay_Window_Create(MULTI_DISP_CHANNEL_LCD, ID, &WinCfg);
        if (Er != 0) {
            ErrMsg = "AmpUT_MultiDisplay_Window_Create failed";
            goto ReturnError;
        }
        // active and Update window
        Er = AmpUT_MultiDisplay_Act_Window(MULTI_DISP_CHANNEL_LCD, ID);
        if (Er != 0) {
            ErrMsg = "AmpUT_MultiDisplay_Act_Window failed";
            goto ReturnError;
        }
    }

    // Get display info
    Er = AmpUT_MultiDisplay_GetInfo(MULTI_DISP_CHANNEL_TV, &DispInfo);
    if (Er != 0) {
        ErrMsg = "AmpUT_MultiDisplay_GetInfo failed";
        goto ReturnError;
    }
    if (VIDEODEC_MAX_CODEC_NUM >= 1) {
        ID = 0;
        // Get window settings
        AmpDisplay_GetDefaultWindowCfg(&WinCfg);
        // Rotate & Flip settings
        WinCfg.SourceDesc.Dec.Rotate = Rotation[ID];

        {   // Center align the image
            float WidthPxlRatioOnDev, HeightPxlRatioOnDev;
            WidthPxlRatioOnDev = Display.SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
            HeightPxlRatioOnDev = Display.SrcHeight;
            if (WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_0 ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_0_HORZ_FLIP ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_180 ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_180_HORZ_FLIP) {
                WidthPxlRatioOnDev = Display.SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
                HeightPxlRatioOnDev = Display.SrcHeight;
            } else {
                WidthPxlRatioOnDev = Display.SrcHeight / DispInfo.DeviceInfo.DevPixelAr;
                HeightPxlRatioOnDev = Display.SrcWidth;
            }
            if (WidthPxlRatioOnDev/HeightPxlRatioOnDev >
            (float)DispInfo.DeviceInfo.VoutWidth/2/((float)DispInfo.DeviceInfo.VoutHeight)) {
                // black bar at top and bottom
                WinCfg.TargetAreaOnPlane.Width = DispInfo.DeviceInfo.VoutWidth / 2;
                WinCfg.TargetAreaOnPlane.Height = (UINT32)((WinCfg.TargetAreaOnPlane.Width*HeightPxlRatioOnDev)/WidthPxlRatioOnDev);
                WinCfg.TargetAreaOnPlane.X = 0;
                WinCfg.TargetAreaOnPlane.Y = (DispInfo.DeviceInfo.VoutHeight - WinCfg.TargetAreaOnPlane.Height) / 2;
            } else {
                // black bar at left and right
                WinCfg.TargetAreaOnPlane.Height = DispInfo.DeviceInfo.VoutHeight;
                WinCfg.TargetAreaOnPlane.Width = (UINT32)((WinCfg.TargetAreaOnPlane.Height*WidthPxlRatioOnDev)/HeightPxlRatioOnDev);
                WinCfg.TargetAreaOnPlane.X = DispInfo.DeviceInfo.VoutWidth / 2 - WinCfg.TargetAreaOnPlane.Width;
                WinCfg.TargetAreaOnPlane.Y = 0;
            }
            // TV Align to 2
            {
                UINT32 Align = 2;
                WinCfg.TargetAreaOnPlane.X = WinCfg.TargetAreaOnPlane.X / Align * Align;
                WinCfg.TargetAreaOnPlane.Y = WinCfg.TargetAreaOnPlane.Y / Align * Align;
                WinCfg.TargetAreaOnPlane.Width = (WinCfg.TargetAreaOnPlane.Width + Align - 1) / Align * Align;
                WinCfg.TargetAreaOnPlane.Height = (WinCfg.TargetAreaOnPlane.Height + Align - 1) / Align * Align;
            }
            AmbaPrintColor(GREEN, "TV Hdlr%u = (%d,%d,%d,%d) Rotation = %u", ID, WinCfg.TargetAreaOnPlane.X, WinCfg.TargetAreaOnPlane.Y,
                    WinCfg.TargetAreaOnPlane.Width, WinCfg.TargetAreaOnPlane.Height, WinCfg.SourceDesc.Dec.Rotate);
        }
        // Configure window settings
        WinCfg.Source = AMP_DISP_DEC;
        WinCfg.SourceDesc.Dec.DecHdlr = AvcDecHdlr[ID];
        Er = AmpUT_MultiDisplay_Window_Create(MULTI_DISP_CHANNEL_TV, ID, &WinCfg);
        if (Er != 0) {
            ErrMsg = "AmpUT_MultiDisplay_Window_Create failed";
            goto ReturnError;
        }
        // active and Update window
        Er = AmpUT_MultiDisplay_Act_Window(MULTI_DISP_CHANNEL_TV, ID);
        if (Er != 0) {
            ErrMsg = "AmpUT_MultiDisplay_Act_Window failed";
            goto ReturnError;
        }
    }

    if (VIDEODEC_MAX_CODEC_NUM >= 2) {
        ID = 1;
        // Get window settings
        AmpDisplay_GetDefaultWindowCfg(&WinCfg);
        // Rotate & Flip settings
        WinCfg.SourceDesc.Dec.Rotate = Rotation[ID];

        {   // Center align the image
            float WidthPxlRatioOnDev, HeightPxlRatioOnDev;
            WidthPxlRatioOnDev = Display.SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
            HeightPxlRatioOnDev = Display.SrcHeight;
            if (WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_0 ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_0_HORZ_FLIP ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_180 ||
                    WinCfg.SourceDesc.Dec.Rotate == AMP_ROTATE_180_HORZ_FLIP) {
                WidthPxlRatioOnDev = Display.SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
                HeightPxlRatioOnDev = Display.SrcHeight;
            } else {
                WidthPxlRatioOnDev = Display.SrcHeight / DispInfo.DeviceInfo.DevPixelAr;
                HeightPxlRatioOnDev = Display.SrcWidth;
            }
            if (WidthPxlRatioOnDev/HeightPxlRatioOnDev >
            (float)DispInfo.DeviceInfo.VoutWidth/2/((float)DispInfo.DeviceInfo.VoutHeight)) {
                // black bar at top and bottom
                WinCfg.TargetAreaOnPlane.Width = DispInfo.DeviceInfo.VoutWidth / 2;
                WinCfg.TargetAreaOnPlane.Height = (UINT32)((WinCfg.TargetAreaOnPlane.Width*HeightPxlRatioOnDev)/WidthPxlRatioOnDev);
                WinCfg.TargetAreaOnPlane.X = DispInfo.DeviceInfo.VoutWidth / 2;
                WinCfg.TargetAreaOnPlane.Y = (DispInfo.DeviceInfo.VoutHeight - WinCfg.TargetAreaOnPlane.Height) / 2;
            } else {
                // black bar at left and right
                WinCfg.TargetAreaOnPlane.Height = DispInfo.DeviceInfo.VoutHeight;
                WinCfg.TargetAreaOnPlane.Width = (UINT32)((WinCfg.TargetAreaOnPlane.Height*WidthPxlRatioOnDev)/HeightPxlRatioOnDev);
                WinCfg.TargetAreaOnPlane.X = DispInfo.DeviceInfo.VoutWidth / 2;
                WinCfg.TargetAreaOnPlane.Y = 0;
            }
            // TV Align to 2
            {
                UINT32 Align = 2;
                WinCfg.TargetAreaOnPlane.X = WinCfg.TargetAreaOnPlane.X / Align * Align;
                WinCfg.TargetAreaOnPlane.Y = WinCfg.TargetAreaOnPlane.Y / Align * Align;
                WinCfg.TargetAreaOnPlane.Width = (WinCfg.TargetAreaOnPlane.Width + Align - 1) / Align * Align;
                WinCfg.TargetAreaOnPlane.Height = (WinCfg.TargetAreaOnPlane.Height + Align - 1) / Align * Align;
            }
            AmbaPrintColor(GREEN, "TV Hdlr%u = (%d,%d,%d,%d) Rotation = %u", ID, WinCfg.TargetAreaOnPlane.X, WinCfg.TargetAreaOnPlane.Y,
                    WinCfg.TargetAreaOnPlane.Width, WinCfg.TargetAreaOnPlane.Height, WinCfg.SourceDesc.Dec.Rotate);
        }
        // Configure window settings
        WinCfg.Source = AMP_DISP_DEC;
        WinCfg.SourceDesc.Dec.DecHdlr = AvcDecHdlr[ID];
        Er = AmpUT_MultiDisplay_Window_Create(MULTI_DISP_CHANNEL_TV, ID, &WinCfg);
        if (Er != 0) {
            ErrMsg = "AmpUT_MultiDisplay_Window_Create failed";
            goto ReturnError;
        }
        // active and Update window
        Er = AmpUT_MultiDisplay_Act_Window(MULTI_DISP_CHANNEL_TV, ID);
        if (Er != 0) {
            ErrMsg = "AmpUT_MultiDisplay_Act_Window failed";
            goto ReturnError;
        }
    }


    /** Configure play settings */
    Trick.Speed = Speed;
    Trick.TimeOffsetOfFirstFrame = StartTime;
    Trick.Direction = PlayDirection;

    /** Play video */
    AmbaPrint("%s: AmpDec_Start start", __FUNCTION__);
    Er = AmpDec_Start(AvcDecPipeHdlr, &Trick, &Display);
    if (Er != 0) {
        ErrMsg = "AmpDec_Start failed";
        goto ReturnError;
    }
    AmbaPrint("%s: AmpDec_Start end", __FUNCTION__);

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Zoom multiple videos.
 *
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_Zoom(char** argv)
{
    /** Configure display settings */
    AMP_VIDEODEC_DISPLAY_s Display = { 0 };
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    Display.SrcWidth = atoi(argv[2]);
    Display.SrcHeight = atoi(argv[3]);
    Display.AOI.X = atoi(argv[4]);
    Display.AOI.Y = atoi(argv[5]);
    Display.AOI.Width = atoi(argv[6]);
    Display.AOI.Height = atoi(argv[7]);

    /** Play video */
    Er = AmpDec_Start(AvcDecPipeHdlr, NULL, &Display);
    if (Er != 0) {
        ErrMsg = "AmpDec_Start failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Pause multiple videos.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_Pause(void)
{
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    /** Pause decoding */
    Er = AmpDec_Pause(AvcDecPipeHdlr);
    if (Er != 0) {
        ErrMsg = "AmpDec_Pause failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Step ahead.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_Step(void)
{
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    /** Step ahead */
    Er = AmpDec_Step(AvcDecPipeHdlr);
    if (Er != 0) {
        ErrMsg = "AmpDec_Step failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Resume multiple videos.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_Resume(void)
{
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    /** Resume decoding */
    Er = AmpDec_Resume(AvcDecPipeHdlr);
    if (Er != 0) {
        ErrMsg = "AmpDec_Resume failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Capture current frame and save it in SD card.
 *
 * @param [in] argc         Command line argument count
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_Cap(int argc,
                                   char **argv)
{
#define MAX_IMAGE_WIDTH (4608)
#define MAX_IMAGE_HEIGHT (3456)
#define MAX_IMAGE_SIZE (MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT)

    AMP_YUV_BUFFER_s Buffer;
    UINT8* RawY;
    UINT8* RawUV;
    char Mode[] = "wb";
    int Er = 0;
    char *ErrMsg = "";

    /** Set target file name (dump.y, dump.uv) */
    char YFn[] = "C:\\dump.y";
    char UVFn[] = "C:\\dump.uv";
    AMP_CFS_FILE_s *YFile, *UVFile;

    AmbaPrint("%s", __FUNCTION__);

    /** Allocate memory for YUV buffer */
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(Buffer.LumaAddr), (void**) &RawY, MAX_IMAGE_SIZE, 1 << 6) != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpUtil_GetAlignedPool failed";
        goto ReturnError;
    }
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(Buffer.ChromaAddr), (void**) &RawUV, MAX_IMAGE_SIZE >> 1, 1 << 6) != AMP_OK) { // Half size in 4:2:0 format
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpUtil_GetAlignedPool failed";
        goto ReturnError;
    }

    /** Configure buffer settings */
    Buffer.ColorFmt = AMP_YUV_420;
    if (atoi(argv[2]) == 0) {
        Buffer.Pitch = 1920;
        Buffer.Width = 1920;
        Buffer.Height = 1080;
        Buffer.AOI.X = 0;
        Buffer.AOI.Y = 0;
        Buffer.AOI.Width = 1920;
        Buffer.AOI.Height = 1080;
    } else {
        Buffer.Pitch = atoi(argv[2]);
        Buffer.Width = atoi(argv[2]);
        Buffer.Height = atoi(argv[3]);
        Buffer.AOI.X = 0;
        Buffer.AOI.Y = 0;
        Buffer.AOI.Width = Buffer.Width;
        Buffer.AOI.Height = Buffer.Height;
    }
    /** Capture image */
    AmbaPrint("Cap: %d %d", Buffer.Width, Buffer.Height);
    // Dump current frame to the buffer
    Er = AmpVideoDec_DumpFrame(AvcDecHdlr[0], &Buffer);
    if (Er != 0) {
        ErrMsg = "AmpVideoDec_DumpFrame failed";
        goto ReturnError;
    }

    // Since the cached buffer is written by uCode, the cache needs to be invalidated in order to get currect data.
    AmbaCache_Invalidate(Buffer.LumaAddr, MAX_IMAGE_SIZE);
    AmbaCache_Invalidate(Buffer.ChromaAddr, MAX_IMAGE_SIZE >> 1);

    /** Save image */
    // Open file
    YFile = AmpDecCFS_fopen(YFn, Mode);
    if (YFile == NULL) {
        ErrMsg = "AmpDecCFS_fopen failed";
        goto ReturnError;
    }
    UVFile = AmpDecCFS_fopen(UVFn, Mode);
    if (UVFile == NULL) {
        ErrMsg = "AmpDecCFS_fopen failed";
        goto ReturnError;
    }
    // Write buffer to the file
    Er = AmpCFS_fwrite(Buffer.LumaAddr, Buffer.Height * Buffer.Pitch, 1, YFile);
    if (Er != 1) {
        ErrMsg = "AmpCFS_fwrite failed";
        goto ReturnError;
    }
    Er = AmpCFS_fwrite(Buffer.ChromaAddr, (Buffer.Height * Buffer.Pitch) >> 1, 1, UVFile); // Half size in 4:2:0 format
    if (Er != 1) {
        ErrMsg = "AmpCFS_fwrite failed";
        goto ReturnError;
    }
    // Close file
    Er = AmpCFS_fclose(YFile);
    if (Er != 0) {
        ErrMsg = "AmpCFS_fclose failed";
        goto ReturnError;
    }
    Er = AmpCFS_fclose(UVFile);
    if (Er != 0) {
        ErrMsg = "AmpCFS_fclose failed";
        goto ReturnError;
    }

    /** Free memory of YUV buffer */
    Er = AmbaKAL_BytePoolFree(RawY);
    if (Er != 0) {
        ErrMsg = "AmbaKAL_BytePoolFree failed";
        goto ReturnError;
    }
    Er = AmbaKAL_BytePoolFree(RawUV);
    if (Er != 0) {
        ErrMsg = "AmbaKAL_BytePoolFree failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Stop multiple video decode.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_Stop(void)
{
    int Er = 0;
    char *ErrMsg = "";
    UINT32 T = 0;

    AmbaPrint("%s", __FUNCTION__);

    // Close dummy demuxer
    for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
        if (DmxHdlr[T] == NULL) {
            AmbaPrint("%s: Demuxer[%u] is not initialized", T);
            continue;
        }
        if (DmxHdlr[T]->IsInit == 0) {
            AmbaPrint("%s: Demuxer[%u] is not initialized", T);
            continue;
        }
        Er = AmpUT_Video_DummyDmx_Close(DmxHdlr[T]);
        if (Er != 0) {
            ErrMsg = "AmpUT_Video_DummyDmx_Close failed";
            goto ReturnError;
        }
        Er = AmpFifo_EraseAll(DmxHdlr[T]->AvcDecFifoHdlr);
        if (Er != 0) {
            ErrMsg = "AmpFifo_EraseAll failed";
            goto ReturnError;
        }
    }

    // Stop decoder
    Er = AmpDec_Stop(AvcDecPipeHdlr);
    if (Er != 0) {
        ErrMsg = "AmpDec_Stop failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Stop multiple video decode and keep the last frame on-screen.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_Stop1(void)
{
    UINT32 I = 0;
    int Er = 0;
    char *ErrMsg = "";
    AmbaPrint("%s", __FUNCTION__);

    // Close dummy demuxer
    for (I = 0; I < VIDEODEC_MAX_CODEC_NUM; ++I) {
        if (DmxHdlr[I] == NULL) {
            AmbaPrint("%s: Demuxer[%u] is not initialized", I);
            continue;
        }
        if (DmxHdlr[I]->IsInit == 0) {
            AmbaPrint("%s: Demuxer[%u] is not initialized", I);
            continue;
        }
        Er = AmpUT_Video_DummyDmx_Close(DmxHdlr[I]);
        if (Er != 0) {
            ErrMsg = "AmpUT_Video_DummyDmx_Close failed";
            goto ReturnError;
        }
        Er = AmpFifo_EraseAll(DmxHdlr[I]->AvcDecFifoHdlr);
        if (Er != 0) {
            ErrMsg = "AmpFifo_EraseAll failed";
            goto ReturnError;
        }
    }

    // Stop decoder while keeping the last frame on screen
    Er = AmpDec_StopWithLastFrm(AvcDecPipeHdlr);
    if (Er != 0) {
        ErrMsg = "AmpDec_StopWithLastFrm failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Deinit multiple video decode.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_Exit(void)
{
    char *ErrMsg = "";
    UINT32 T = 0;
    AmbaPrint("%s", __FUNCTION__);

    if (IsInit == 0) {
        AmbaPrint("No need to exit");
        goto ReturnSuccess;
    }

    // Deinit demux handler
    if (AmpUT_Video_DummyDmx_Exit() != AMP_OK) {
        AmbaPrint("%s:%u Failed to exit demux.", __FUNCTION__, __LINE__);
    }

    // Deinit video decoder manager
    if (AvcDecPipeHdlr != NULL) {
        if (AmpDec_Stop(AvcDecPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to stop the video decoder manager.", __FUNCTION__, __LINE__);
        }
        if (AmpDec_Remove(AvcDecPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to remove the video decoder manager.", __FUNCTION__, __LINE__);
        }
        if (AmpDec_Delete(AvcDecPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to delete the video decoder manager.", __FUNCTION__, __LINE__);
        }
        AvcDecPipeHdlr = NULL;
    }

    // Deinit video decoder
    for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
        if (AvcDecHdlr[T] != NULL) {
            if (AmpVideoDec_Delete(AvcDecHdlr[T]) != AMP_OK) {
                AmbaPrint("%s:%u Failed to deinit the video decoder.", __FUNCTION__, __LINE__);
            }
            AvcDecHdlr[T] = NULL;
        }
    }

    // Deinit LCD and TV windows
    {
        for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
            // Delete LCD Window
            if (AmpUT_MultiDisplay_Window_Delete(MULTI_DISP_CHANNEL_LCD, T) != AMP_OK) {
                AmbaPrint("%s:%u Failed to delete LCD window handler.", __FUNCTION__, __LINE__);
                ErrMsg = "AmpUT_MultiDisplay_Window_Delete failed";
                goto ReturnError;
            }

            // Delete TV Window
            if (AmpUT_MultiDisplay_Window_Delete(MULTI_DISP_CHANNEL_TV, T) != AMP_OK) {
                AmbaPrint("%s:%u Failed to delete TV window handler.", __FUNCTION__, __LINE__);
                ErrMsg = "AmpUT_MultiDisplay_Window_Delete failed";
                goto ReturnError;
            }
        }
    }

    // Release descriptor buffer
    for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
        if (AvcDescBufferOri[T] != NULL) {
            if (AmbaKAL_BytePoolFree(AvcDescBufferOri[T]) != AMP_OK) {
                AmbaPrint("%s:%u Failed to release the descriptor buffer.", __FUNCTION__, __LINE__);
            }
            AvcDescBufferOri[T] = NULL;
        }
    }

    // Release raw buffer
    for (T = 0; T < VIDEODEC_MAX_CODEC_NUM; ++T) {
        if (AvcRawBufferOri[T] != NULL) {
            if (AmbaKAL_BytePoolFree(AvcRawBufferOri[T]) != AMP_OK) {
                AmbaPrint("%s:%u Failed to release the raw buffer.", __FUNCTION__, __LINE__);
            }
            AvcRawBuffer[T] = NULL;
            AvcRawBufferOri[T] = NULL;
        }
    }

    // Don't have to delete codec module
    //AmbaKAL_BytePoolFree(AvcCodecBufferOri);
    //AvcCodecBufferOri = NULL;

    IsInit = 0;

ReturnSuccess:
    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Set prefeed number.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_SetPreFeed(char** argv)
{
    // Configure prefeed settings
    AmbaPrint("%s", __FUNCTION__);

    PreFeedNum = atoi(argv[2]);

    AmbaPrint("Set prefeed number: %u", PreFeedNum);

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;
}

/**
 * Multiple video decode usage.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDec_Usage(void)
{
    AmbaPrint("AmpUT_VideoMultiDec");
    AmbaPrint("\t init - initialize");
    AmbaPrint("\t start [filePath] [videoWidth] [videoHeight] [startTime] [speed] [dir] [rotate0] [rotate1] [rotate2] [rotate3]");
    AmbaPrint("\t       - start decode");
    AmbaPrint("\t       [filePath]      the file path of .nhnt/.h264/.udta file");
    AmbaPrint("\t       [videoWidth]    video width");
    AmbaPrint("\t       [videoHeight]   video height");
    AmbaPrint("\t       [startTime]     time (in ms) of the first displayed video frame");
    AmbaPrint("\t       [speed]         128 - 0.5x, 256 - 1x, 512 - 2x");
    AmbaPrint("\t       [dir]           0 - forward, 1 - backward");
    AmbaPrint("\t       [rotate0]       Rotation of the first video. 0 ~ 7, 0 - no rotation, 1 - horizontal flip, ...etc");
    AmbaPrint("\t       [rotate1]       Rotation of the second video");
    AmbaPrint("\t       [rotate2]       Rotation of the third video");
    AmbaPrint("\t       [rotate3]       Rotation of the fourth video");
    AmbaPrint("\t zoom [videoWidth] [videoHeight] [cropOffsetX] [cropOffsetY] [cropWidth] [cropHeight]");
    AmbaPrint("\t      - crop part of video");
    AmbaPrint("\t      [videoWidth]     video width");
    AmbaPrint("\t      [videoHeight]    video height");
    AmbaPrint("\t      [cropOffsetX]    cropping area horizontal offset");
    AmbaPrint("\t      [cropOffsetY]    cropping area vertical offset");
    AmbaPrint("\t      [cropWidth]      cropping area width");
    AmbaPrint("\t      [cropHeight]     cropping area height");
    AmbaPrint("\t pause - pause decode");
    AmbaPrint("\t step - step ahead during pause");
    AmbaPrint("\t resume - resume decode");
    AmbaPrint("\t stop - stop decode");
    AmbaPrint("\t stop1 - stop decode and keep the last frame on-screen");
    AmbaPrint("\t exit - deinitialize");
//    AmbaPrint("\t cap [width] [height] - capture YUV during pause");
    return 0;
}

/**
 * Multiple video decode test.
 *
 * @param [in] env
 * @param [in] argc         Command line argument count
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoMultiDecTest(struct _AMBA_SHELL_ENV_s_ *env,
                       int argc,
                       char **argv)
{
    // In case that user accesses an index larger than "argc - 1".
    // Use a local "argv" array that is large enough.
    // Each element is initialized as "0" string so that atoi can return 0 by defualt.
    // NOTE: Use "ArgvArray" in the following code instead of the input "argv"
    static char *ArgvArray[64];     // A large argv array
    static char Zero[2];            // A string of "0"
    int T = 0;
    // Initialize the values everytime for safety. Doesn't cause much time.
    Zero[0] = '0';
    Zero[1] = '\0';
    for (T = 0; T < 64; ++T) {
        if (T < argc) {
            ArgvArray[T] = argv[T];
        } else {
            ArgvArray[T] = Zero;
        }
    }
    AmbaPrint("%s cmd: %s", __FUNCTION__, ArgvArray[1]);
    if (strcmp(ArgvArray[1], "init") == 0) {
        AmpUT_VideoMultiDec_Init(atoi(ArgvArray[2]));
    } else if (strcmp(ArgvArray[1], "start") == 0) {
        AmpUT_VideoMultiDec_Start(argc, ArgvArray);
    } else if (strcmp(ArgvArray[1], "pause") == 0) {
        AmpUT_VideoMultiDec_Pause();
    } else if (strcmp(ArgvArray[1], "step") == 0) {
        AmpUT_VideoMultiDec_Step();
    } else if (strcmp(ArgvArray[1], "resume") == 0) {
        AmpUT_VideoMultiDec_Resume();
    } else if (strcmp(ArgvArray[1], "stop") == 0) {
        AmpUT_VideoMultiDec_Stop();
    } else if (strcmp(ArgvArray[1], "stop1") == 0) {
        AmpUT_VideoMultiDec_Stop1();
    } else if (strcmp(ArgvArray[1], "cap") == 0) {
        AmpUT_VideoMultiDec_Cap(argc, ArgvArray);
    } else if (strcmp(ArgvArray[1], "exit") == 0) {
        AmpUT_VideoMultiDec_Exit();
    } else if (strcmp(ArgvArray[1], "zoom") == 0) {
        AmpUT_VideoMultiDec_Zoom(ArgvArray);
    } else if (strcmp(ArgvArray[1], "ch") == 0) {
        AmpUT_VideoMultiDec_ConfigDisplayCh(ArgvArray);
    } else if (strcmp(ArgvArray[1], "prefeed") == 0) {
        AmpUT_VideoMultiDec_SetPreFeed(ArgvArray);
    } else if (strcmp(ArgvArray[1], "cvbs") == 0) {
        MultiDispUTRunCVBS = atoi(ArgvArray[2]);
        AmbaPrint("CVBS: %d", MultiDispUTRunCVBS);
    } else if (strcmp(ArgvArray[1], "suspend") == 0) {
    } else {
        AmpUT_VideoMultiDec_Usage();
    }
    return 0;
}

/**
 * Hook multiple video decode command.
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_VideoMultiDecInit(void)
{
    // Hook command
    AmbaTest_RegisterCommand("videomultidec", AmpUT_VideoMultiDecTest);
    AmbaTest_RegisterCommand("vmd", AmpUT_VideoMultiDecTest);

    return AMP_OK;
}
