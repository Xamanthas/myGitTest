/**
 *  @file src/unittest/MWUnitTest/AmpUT_VideoDec.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/03/25 |cyweng      |Created        |
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
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
#include "DecCFSWrapper.h"
#include "AmpUT_Display.h"

/* For Standby mode test */
#include "AmbaINT.h"
#include "AmbaLink.h"

#define VIDEODEC_FRAME_COUNT_EOS (0xFFFFFFFF) ///< VideoDecFrmCnt of 0xFFFFFFFF indicates EOS
#define VIDEODEC_RAW_SIZE (32<<20) ///< 32MB raw buffer
#define VIDEODEC_RAW_DESC_NUM (128) ///< descriptor number = 128
// global var
static void* AvcCodecBufferOri = NULL; ///< Original buffer address of video codec
static void* AvcRawBufferOri = NULL;   ///< Original buffer address of video decode raw file
static void* AvcRawBuffer = NULL;      ///< Aligned buffer address of video decode raw file
static void* AvcDescBufferOri = NULL;  ///< Original buffer address of video descripter
static void* AvcDescBuffer = NULL;     ///< Aligned buffer address of video descripter
static AMP_AVDEC_HDLR_s *AvcDecHdlr = NULL;     ///< video codec handler
static AMP_DEC_PIPE_HDLR_s *DecPipeHdlr = NULL; ///< decode mgr pipe handler
static UINT8 IsInit = 0; // Whether the video decoder is initialized

// feature ctrl
static UINT8 FadeEnable = 0;                    ///< enable fade or not
static AMP_AVDEC_PLAY_DIRECTION_e PlayDirection = AMP_VIDEO_PLAY_FW; ///< Play direction
static UINT32 PreFeedNum = 32;  // Number of frames to feed before start

// Dummy demux
static AMP_UT_DUMMY_DEMUX_HDLR_s* DmxHdlr = NULL;  ///< Dummy demux handler

/**
 * Read elementary stream data from memory
 *
 * @param buf [in] - data to copy
 * @param size [in] - data size
 * @param ptr [in] - dest address
 * @param base [in] - buffer base
 * @param limit [in] - buffer limitation
 * @return
 */
UINT8 *fmt_read_stream_mem(UINT8 *buf,
                                   int size,
                                   UINT8 *ptr,
                                   UINT8 *base,
                                   UINT8 *limit)
{
    UINT32 rear;

    if (!((ptr >= base) && (ptr < limit))) {
        K_ASSERT(0);
    }

    if ((ptr + size) <= limit) {
        memcpy(ptr, buf, size);
        ptr += size;
    } else {
        rear = limit - ptr;
        memcpy(ptr, buf, rear);
        buf += rear;
        memcpy(base, buf, size - rear);
        ptr = base + size - rear;
    }

    if (ptr == limit) {
        ptr = base;
    }

    return ptr;
}

/**
 * Create display handlers and windows on Vout.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_ConfigDisplay(void)
{
    AMP_DISP_WINDOW_CFG_s DispWindow;
    int Er = NG;

    if (AmpUT_Display_Init() == NG) {
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
    if (AmpUT_Display_Window_Create(0, &DispWindow) == NG) {
        return Er;
    }

#ifdef  DEC_1080
    DispWindow.TargetAreaOnPlane.Width = 1920;
    DispWindow.TargetAreaOnPlane.Height = 1080;
#else
    DispWindow.TargetAreaOnPlane.Width = 3840;
    DispWindow.TargetAreaOnPlane.Height = 2160;
#endif
    if (AmpUT_Display_Window_Create(1, &DispWindow) == NG) {
        return Er;
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
static int AmpUT_VideoDec_ConfigDisplayCh(char **argv)
{
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    if (atoi(argv[2]) == 1) {
        // Start up TV handler
        Er = AmpUT_Display_Start(1);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Start failed";
            goto ReturnError;
        }
        Er = AmpUT_Display_Act_Window(1);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Act_Window failed";
            goto ReturnError;
        }
    } else {
        // Start up LCD handler
        Er = AmpUT_Display_Start(0);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Start failed";
            goto ReturnError;
        }
        Er = AmpUT_Display_Act_Window(0);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Act_Window failed";
            goto ReturnError;
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
static int AmpUT_VideoDec_CodecCB(void *hdlr,
                                  UINT32 event,
                                  void* info)
{
    UINT32 FrameCount = 0;
    //AmbaPrint("AmpUT_VideoDec_CodecCB on Event: 0x%x ", event);

    switch (event) {
    case AMP_DEC_EVENT_FIRST_FRAME_DISPLAYED:
        AmbaPrint("[AmpUT - VideoDec] <DecCallback> AMP_DEC_EVENT_FIRST_FRAME_DISPLAYED");
        break;
    case AMP_DEC_EVENT_PLAYBACK_EOS:
        AmbaPrint("[AmpUT - VideoDec] <DecCallback> AMP_DEC_EVENT_PLAYBACK_EOS");
        break;
    case AMP_DEC_EVENT_STATE_CHANGED:
        AmbaPrint("[AmpUT - VideoDec] <DecCallback> AMP_DEC_EVENT_STATE_CHANGED");
        break;
    case AMP_DEC_EVENT_DATA_UNDERTHRSHOLDD:
        if (DmxHdlr->IsInit == 0) {
            AmbaPrintColor(RED, "%s: Handler not found (0x%08X)", __FUNCTION__, hdlr);
            return -1;
        }
        AmpUT_Video_DummyDmx_Lock(DmxHdlr, AMBA_KAL_WAIT_FOREVER);
        if (DmxHdlr->AvcDecHdlr == (AMP_AVDEC_HDLR_s *)hdlr) {
            FrameCount = DmxHdlr->VideoDecFrmCnt;
            AmpUT_Video_DummyDmx_UnLock(DmxHdlr);
        } else {
            AmbaPrintColor(RED, "%s: Handler not found (0x%08X)", __FUNCTION__, hdlr);
            AmpUT_Video_DummyDmx_UnLock(DmxHdlr);
            return -1;
        }

        if (FrameCount != VIDEODEC_FRAME_COUNT_EOS) {
            // It's necessary to give mutex before calling this function
            AmpUT_Video_DummyDmx_Feed(DmxHdlr, 4); // feed 4 frames;
        }
        break;
    default:
        break;
    }

    return 0;
}

/**
 * Set fading effect.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_SetFading(void)
{
    AMP_VIDEODEC_FADING_EFFECT_s FadingCfg[2] = {0};
    FadingCfg[0].Duration = 1000;
    FadingCfg[0].StartTime = 0;
    FadingCfg[0].StartMatrix[0] = 0;
    FadingCfg[0].StartMatrix[1] = 0;
    FadingCfg[0].StartMatrix[2] = 0;
    FadingCfg[0].StartMatrix[3] = 0;
    FadingCfg[0].StartMatrix[4] = 0;
    FadingCfg[0].StartMatrix[5] = 0;
    FadingCfg[0].StartMatrix[6] = 0;
    FadingCfg[0].StartMatrix[7] = 0;
    FadingCfg[0].StartMatrix[8] = 0;
    FadingCfg[0].StartYOffset = 128;
    FadingCfg[0].StartUOffset = 255;
    FadingCfg[0].StartVOffset = 255;

    FadingCfg[0].EndMatrix[0] = 1024;
    FadingCfg[0].EndMatrix[1] = 0;
    FadingCfg[0].EndMatrix[2] = 0;
    FadingCfg[0].EndMatrix[3] = 0;
    FadingCfg[0].EndMatrix[4] = 1024;
    FadingCfg[0].EndMatrix[5] = 0;
    FadingCfg[0].EndMatrix[6] = 0;
    FadingCfg[0].EndMatrix[7] = 0;
    FadingCfg[0].EndMatrix[8] = 1024;
    FadingCfg[0].EndYOffset = 0;
    FadingCfg[0].EndUOffset = 128;
    FadingCfg[0].EndVOffset = 128;
    AmpVideoDec_SetFadingEffect(AvcDecHdlr, 1, // only fade in
            FadingCfg);
    return 0;
}

/**
 * Initialize video decode.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_Init(UINT32 disableVoutAtInit)
{
    AMP_VIDEODEC_CFG_s CodecCfg;
    AMP_VIDEODEC_INIT_CFG_s CodecInitCfg;
    AMP_DEC_PIPE_CFG_s PipeCfg;
    int Er = 0;
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
    if (AvcCodecBufferOri == NULL) {
        // Get the default codec module settings
        AmpVideoDec_GetInitDefaultCfg(&CodecInitCfg);
        // Customize the module settings
            // You can add some codes here ...
        // Allocate memory for codec module
        if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(CodecInitCfg.Buf), &AvcCodecBufferOri, CodecInitCfg.BufSize, 1 << 5) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpUtil_GetAlignedPool failed";
            goto ReturnError;
        }
        // Configure the initial settings
        Er = AmpVideoDec_Init(&CodecInitCfg);
        if (Er != 0) {
            ErrMsg = "AmpVideoDec_Init failed";
            goto ReturnError;
        }
    }

    /** Create codec handler */
    // Get the default codec handler settings
    AmpVideoDec_GetDefaultCfg(&CodecCfg);
    if (AvcRawBufferOri == NULL) {
        // Allocate memory for codec raw buffer
        if (AmpUtil_GetAlignedPool(&G_MMPL, &AvcRawBuffer, &AvcRawBufferOri, VIDEODEC_RAW_SIZE, 1 << 6) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpUtil_GetAlignedPool failed";
            goto ReturnError;
        }
    }
    CodecCfg.RawBuffer = (char*) AvcRawBuffer;
    AmbaPrint("%s:%u RawBuffer:%x -> %x", __FUNCTION__, __LINE__, AvcRawBufferOri, CodecCfg.RawBuffer);
    CodecCfg.RawBufferSize = VIDEODEC_RAW_SIZE;
    if (AvcDescBufferOri == NULL) {
        if (AmpUtil_GetAlignedPool(&G_MMPL, &AvcDescBuffer, &AvcDescBufferOri, VIDEODEC_RAW_DESC_NUM * sizeof(AMP_BITS_DESC_s), 1 << 5) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpUtil_GetAlignedPool failed";
            goto ReturnError;
        }
    }
    CodecCfg.DescBuffer = (char*) AvcDescBuffer;
    CodecCfg.NumDescBuffer = VIDEODEC_RAW_DESC_NUM;
    // Customize the handler settings
    CodecCfg.CbCodecEvent = AmpUT_VideoDec_CodecCB;
    // TDB feature config
    CodecCfg.Feature.MaxVoutWidth = 3840;
    CodecCfg.Feature.MaxVoutHeight = 2160;
    // Create a codec handler, and configure the initial settings
    if (AmpVideoDec_Create(&CodecCfg, &AvcDecHdlr) != AMP_OK) {
        AmbaPrint("%s:%u Cannot create video codec.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpVideoDec_Create failed";
        goto ReturnError;
    }
    AmbaPrint("AvcDecHdlr = 0x%x ", AvcDecHdlr);

    /** Create decoder manager */
    // Get the default decoder manager settings
    AmpDec_GetDefaultCfg(&PipeCfg);
    // Customize the manager settings
    //PipeCfg.cbEvent
    PipeCfg.Decoder[0] = AvcDecHdlr;
    PipeCfg.NumDecoder = 1;
    PipeCfg.Type = AMP_DEC_VID_PIPE;
    // Create a decoder manager, and insert the codec handler into the manager
    Er = AmpDec_Create(&PipeCfg, &DecPipeHdlr);
    if (Er != 0) {
        ErrMsg = "AmpDec_Create failed";
        goto ReturnError;
    }

    /** Create LCD handler, TV handler, LCD window, TV window */
    Er = AmpUT_VideoDec_ConfigDisplay();
    if (Er != 0) {
        ErrMsg = "AmpUT_VideoDec_ConfigDisplay failed";
        goto ReturnError;
    }

    /** Activate decoder manager */
    // Activate the decoder manager and all the codec handlers in the manager
    Er = AmpDec_Add(DecPipeHdlr);
    if (Er != 0) {
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
        DummyDemuxCfg.AvcDecHdlr = AvcDecHdlr;
        DummyDemuxCfg.AvcRawBuffer = AvcRawBuffer;
        DummyDemuxCfg.AvcRawLimit = (UINT8*) ((UINT32) AvcRawBuffer + VIDEODEC_RAW_SIZE - 1);
        Er = AmpUT_Video_DummyDmx_Create(&DummyDemuxCfg, &DmxHdlr);
        if (Er != 0) {
            AmbaPrintColor(RED, "%s:%u Failed to create demuxer.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpUT_Video_DummyDmx_Create failed";
            goto ReturnError;
        }
    }

    AmbaKAL_TaskSleep(17);

    /** Enable vout */
    if ((disableVoutAtInit&1) == 0) {
        // Start up LCD handler
        Er = AmpUT_Display_Start(0);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Start failed";
            goto ReturnError;
        }
    }
    if ((disableVoutAtInit&2) == 0) {
        // Start up TV handler
        Er = AmpUT_Display_Start(1);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Start failed";
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
 * Start decoding video.
 *
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_Start(int argc, char** argv)
{
    AMP_AVDEC_TRICKPLAY_s Trick = { 0 };
    AMP_VIDEODEC_DISPLAY_s Display = { 0 };
    char AvcFn[80];         // Path of .h264 file in ASCII format
    char IdxFn[80];         // Path of .nhnt file in ASCII format
    char UdtFn[80];         // Path of .udta file in ASCII format
    AMP_DISP_WINDOW_CFG_s WinCfg;
    AMP_DISP_INFO_s DispInfo;
    int InputValue;         // A temporary space storing input value
    char* VideoPath = NULL; // The path of video
    UINT16 VideoWidth = 0;  // The width of video
    UINT16 VideoHeight = 0; // The height of video
    UINT32 StartTime = 0;   // Start from the beginning
    UINT32 Speed = 0x100;   // Normal speed
    UINT32 Direction = 0;   // Play forwards
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    /** Get and check user input */
    if (argc < 3) {
        AmbaPrint("%s: Insufficient parameters", __FUNCTION__);
        goto ReturnError;
    } else if (argc > 8) {
        AmbaPrint("%s: To many parameters", __FUNCTION__);
        goto ReturnError;
    }

    VideoPath = argv[2];
    if (strcmp(VideoPath, "") == 0) {
        AmbaPrint("%s: Please enter the index of video files named 0UT_XXXX.h264 and 0UT_XXXX.nhnt located in SD root.", __FUNCTION__);
        goto ReturnError;
    }

    // If input width = 0, then VideoWidth = AvcFileWidth
    VideoWidth = DmxHdlr->AvcFileWidth;
    if (argc >= 4) {
        InputValue = atoi(argv[3]);
        if (InputValue < 0) {
            AmbaPrint("%s: Illegal value of width (%d)", __FUNCTION__, InputValue);
            goto ReturnError;
        } else if (InputValue > 0) {
            VideoWidth = (UINT32)InputValue;
        }
    }

    // If input height = 0, then VideoHeight = AvcFileHeight
    VideoHeight = DmxHdlr->AvcFileHeight;
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
        if (InputValue != 0 && InputValue != 1) {
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

    /** Get target file name (.h264, .nhnt) */
    AmbaPrint("%s: target file - %s", __FUNCTION__, VideoPath);
    sprintf(AvcFn, "%s.h264", VideoPath);
    sprintf(IdxFn, "%s.nhnt", VideoPath);
    sprintf(UdtFn, "%s.udta", VideoPath);
    // Convert file name to Unicode

    /** Open target file */
    // Open file by dummy demuxer
    if (AmpUT_Video_DummyDmx_Open(DmxHdlr, AvcFn, IdxFn, UdtFn) != 0) {
        AmbaPrint("%s: Failed to open demux", __FUNCTION__);
        ErrMsg = "AmpUT_Video_DummyDmx_Open failed";
        goto ReturnError;
    }

    /** Pre-feed */
    AmbaPrint("%s: StartTime = %d  Speed = 0x%3x", __FUNCTION__, StartTime, Speed);
    Er = AmpUT_Video_DummyDmx_Start(DmxHdlr, StartTime, PlayDirection, Speed, PreFeedNum);
    if (Er != 0) {
        ErrMsg = "AmpUT_Video_DummyDmx_Start failed";
        goto ReturnError;
    }

    /** Configure fading settings */
    if (FadeEnable) {
        Er = AmpUT_VideoDec_SetFading();
        if (Er != 0) {
            ErrMsg = "AmpUT_VideoDec_SetFading failed";
            goto ReturnError;
        }
    }

    /** Configure Display settings */
    Display.SrcWidth = VideoWidth;
    Display.SrcHeight = VideoHeight;
    Display.AOI.X = 0;
    Display.AOI.Y = 0;
    Display.AOI.Width = Display.SrcWidth;
    Display.AOI.Height = Display.SrcHeight;

    /** Re-config window */
    // Get display info
    Er = AmpUT_Display_GetInfo(0, &DispInfo);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_GetInfo failed";
        goto ReturnError;
    }
    // Get window settings
    AmpDisplay_GetDefaultWindowCfg(&WinCfg);

    {   // Center align the image
        float WidthPxlRatioOnDev, HeightPxlRatioOnDev;
        WidthPxlRatioOnDev = Display.SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
        HeightPxlRatioOnDev = Display.SrcHeight;
        if (WidthPxlRatioOnDev/HeightPxlRatioOnDev >
        (float)DispInfo.DeviceInfo.VoutWidth/((float)DispInfo.DeviceInfo.VoutHeight)) {
            // black bar at top and bottom
            WinCfg.TargetAreaOnPlane.Width = DispInfo.DeviceInfo.VoutWidth;
            WinCfg.TargetAreaOnPlane.Height = (UINT32)((WinCfg.TargetAreaOnPlane.Width*HeightPxlRatioOnDev)/WidthPxlRatioOnDev);
            WinCfg.TargetAreaOnPlane.X = 0;
            WinCfg.TargetAreaOnPlane.Y = (DispInfo.DeviceInfo.VoutHeight - WinCfg.TargetAreaOnPlane.Height)>>1;
        } else {
            // black bar at left and right
            WinCfg.TargetAreaOnPlane.Height = DispInfo.DeviceInfo.VoutHeight;
            WinCfg.TargetAreaOnPlane.Width = (UINT32)((WinCfg.TargetAreaOnPlane.Height*WidthPxlRatioOnDev)/HeightPxlRatioOnDev);
            WinCfg.TargetAreaOnPlane.Y = 0;
            WinCfg.TargetAreaOnPlane.X = (DispInfo.DeviceInfo.VoutWidth - WinCfg.TargetAreaOnPlane.Width)>>1;
        }
        // LCD Align to 6
        {
            UINT32 Align = 6;
            WinCfg.TargetAreaOnPlane.X = WinCfg.TargetAreaOnPlane.X / Align * Align;
            WinCfg.TargetAreaOnPlane.Y = WinCfg.TargetAreaOnPlane.Y / Align * Align;
            WinCfg.TargetAreaOnPlane.Width = (WinCfg.TargetAreaOnPlane.Width + Align - 1) / Align * Align;
            WinCfg.TargetAreaOnPlane.Height = (WinCfg.TargetAreaOnPlane.Height + Align - 1) / Align * Align;
        }
    }
    // Configure & update window settings
    WinCfg.Source = AMP_DISP_DEC;
    Er = AmpUT_Display_Window_Create(0, &WinCfg);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_SetWindowCfg failed";
        goto ReturnError;
    }
    // active and Update window
    Er = AmpUT_Display_Act_Window(0);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_Update failed";
        goto ReturnError;
    }

    // Get display info
    Er = AmpUT_Display_GetInfo(1, &DispInfo);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_GetInfo failed";
        goto ReturnError;
    }
    // Get window settings
    AmpDisplay_GetDefaultWindowCfg(&WinCfg);

    {   // Center align the image
        float WidthPxlRatioOnDev, HeightPxlRatioOnDev;
        WidthPxlRatioOnDev = Display.SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
        HeightPxlRatioOnDev = Display.SrcHeight;
        if (WidthPxlRatioOnDev/HeightPxlRatioOnDev >
        (float)DispInfo.DeviceInfo.VoutWidth/((float)DispInfo.DeviceInfo.VoutHeight)) {
            // black bar at top and bottom
            WinCfg.TargetAreaOnPlane.Width = DispInfo.DeviceInfo.VoutWidth;
            WinCfg.TargetAreaOnPlane.Height = (UINT32)((WinCfg.TargetAreaOnPlane.Width*HeightPxlRatioOnDev)/WidthPxlRatioOnDev);
            WinCfg.TargetAreaOnPlane.X = 0;
            WinCfg.TargetAreaOnPlane.Y = (DispInfo.DeviceInfo.VoutHeight - WinCfg.TargetAreaOnPlane.Height)>>1;
        } else {
            // black bar at left and right
            WinCfg.TargetAreaOnPlane.Height = DispInfo.DeviceInfo.VoutHeight;
            WinCfg.TargetAreaOnPlane.Width = (UINT32)((WinCfg.TargetAreaOnPlane.Height*WidthPxlRatioOnDev)/HeightPxlRatioOnDev);
            WinCfg.TargetAreaOnPlane.Y = 0;
            WinCfg.TargetAreaOnPlane.X = (DispInfo.DeviceInfo.VoutWidth - WinCfg.TargetAreaOnPlane.Width)>>1;
        }
        // TV Align to 2
        {
            UINT32 Align = 2;
            WinCfg.TargetAreaOnPlane.X = WinCfg.TargetAreaOnPlane.X / Align * Align;
            WinCfg.TargetAreaOnPlane.Y = WinCfg.TargetAreaOnPlane.Y / Align * Align;
            WinCfg.TargetAreaOnPlane.Width = (WinCfg.TargetAreaOnPlane.Width + Align - 1) / Align * Align;
            WinCfg.TargetAreaOnPlane.Height = (WinCfg.TargetAreaOnPlane.Height + Align - 1) / Align * Align;
        }
    }
    // Configure window settings
    WinCfg.Source = AMP_DISP_DEC;
    Er = AmpUT_Display_Window_Create(1, &WinCfg);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_SetWindowCfg failed";
        goto ReturnError;
    }
    // active and Update window
    Er = AmpUT_Display_Act_Window(1);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_Update failed";
        goto ReturnError;
    }


    /** Configure play settings */
    Trick.Speed = Speed;
    Trick.TimeOffsetOfFirstFrame = StartTime;
    Trick.Direction = PlayDirection;

    /** Play video */
    AmbaPrint("%s: AmpDec_Start start", __FUNCTION__);
    Er = AmpDec_Start(DecPipeHdlr, &Trick, &Display);
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
 * Zoom video.
 *
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_Zoom(char** argv)
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
    Er = AmpDec_Start(DecPipeHdlr, NULL, &Display);
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
 * Start video.
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_VideoDec_Trick(void)
{
    AMP_AVDEC_TRICKPLAY_s Trick = { 0 };

    Trick.Speed = 0x100;
    Trick.TimeOffsetOfFirstFrame = 0;
    Trick.Direction = AMP_VIDEO_PLAY_FW;
    AmpDec_Start(DecPipeHdlr, &Trick, NULL );
    return 0;
}

/**
 * Pause video.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_Pause(void)
{
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    /** Pause decoding */
    Er = AmpDec_Pause(DecPipeHdlr);
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
static int AmpUT_VideoDec_Step(void)
{
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    /** Step ahead */
    Er = AmpDec_Step(DecPipeHdlr);
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
 * Resume video.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_Resume(void)
{
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    /** Resume decoding */
    Er = AmpDec_Resume(DecPipeHdlr);
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

/*
 * JPEG QTable
 */
#if 0
#pragma data_alignment = 32
static UINT8 StdJpegQTable[128] = {
        0x10,
        0x0B,
        0x0C,
        0x0E,
        0x0C,
        0x0A,
        0x10,
        0x0E,
        0x0D,
        0x0E,
        0x12,
        0x11,
        0x10,
        0x13,
        0x18,
        0x28,
        0x1A,
        0x18,
        0x16,
        0x16,
        0x18,
        0x31,
        0x23,
        0x25,
        0x1D,
        0x28,
        0x3A,
        0x33,
        0x3D,
        0x3C,
        0x39,
        0x33,
        0x38,
        0x37,
        0x40,
        0x48,
        0x5C,
        0x4E,
        0x40,
        0x44,
        0x57,
        0x45,
        0x37,
        0x38,
        0x50,
        0x6D,
        0x51,
        0x57,
        0x5F,
        0x62,
        0x67,
        0x68,
        0x67,
        0x3E,
        0x4D,
        0x71,
        0x79,
        0x70,
        0x64,
        0x78,
        0x5C,
        0x65,
        0x67,
        0x63,
        0x11,
        0x12,
        0x12,
        0x18,
        0x15,
        0x18,
        0x2F,
        0x1A,
        0x1A,
        0x2F,
        0x63,
        0x42,
        0x38,
        0x42,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63,
        0x63 };
#endif

/**
 * Capture current frame and save it in SD card.
 *
 * @param [in] argc         Command line argument count
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_Cap(int argc,
                           char **argv)
{
    AMP_YUV_BUFFER_s Buffer;
    UINT8* RawY;
    UINT8* RawUV;
    char Mode[] = "wb";
    UINT32 FileWriteCount = 0; // Return value from AmpCFS_fwrite, it's expected to be equal to "count" parameter when success.
    int Er = 0;
    char *ErrMsg = "";

    /** Set target file name (dump.y, dump.uv) */
    char YFn[] = "C:\\dump.y";
    char UvFn[] = "C:\\dump.uv";
    AMP_CFS_FILE_s *YFp, *UvFp;
//AMBA_JPEG_ENC_CONFIG_s jpegEncConfig;
//AMBA_DSP_JPEG_ENC_CTRL_s jpgEncCtrl;

    AmbaPrint("%s", __FUNCTION__);

    /** Allocate memory for YUV buffer */
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(Buffer.LumaAddr), (void**) &RawY, 4608 * 3456, 1 << 6) != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpUtil_GetAlignedPool failed";
        goto ReturnError;
    }
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(Buffer.ChromaAddr), (void**) &RawUV, 4608 * 3456 >> 1, 1 << 6) != AMP_OK) { // Half size in 4:2:0 format
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
    Er = AmpVideoDec_DumpFrame(AvcDecHdlr, &Buffer);
    if (Er != 0) {
        ErrMsg = "AmpVideoDec_DumpFrame failed";
        goto ReturnError;
    }

    // Since the buffer is cached, the cache needs to be flushed in order to get currect data.
    AmbaCache_Invalidate(Buffer.LumaAddr, 4608 * 3456);
    AmbaCache_Invalidate(Buffer.ChromaAddr, 4608 * 3456 >> 1);

    /** Save image */
    // Open file
    YFp = AmpDecCFS_fopen(YFn, Mode);
    if (YFp == NULL) {
        ErrMsg = "AmpDecCFS_fopen failed";
        goto ReturnError;
    }
    UvFp = AmpDecCFS_fopen(UvFn, Mode);
    if (UvFp == NULL) {
        ErrMsg = "AmpDecCFS_fopen failed";
        goto ReturnError;
    }
    // Write buffer to the file
    FileWriteCount = AmpCFS_fwrite(Buffer.LumaAddr, Buffer.Height * Buffer.Pitch, 1, YFp);
    // Return value from AmpCFS_fwrite is expected to be equal to "count" when success.
    if (FileWriteCount != 1) {
        AmbaPrint("%s: Warning: Inconsistent count (%u and %u)", __FUNCTION__, FileWriteCount, 1);
        // Don't go to ReturnError.
    }
    FileWriteCount = AmpCFS_fwrite(Buffer.ChromaAddr, (Buffer.Height * Buffer.Pitch) >> 1, 1, UvFp); // Half size in 4:2:0 format
    // Return value from AmpCFS_fwrite is expected to be equal to "count" when success.
    if (FileWriteCount != 1) {
        AmbaPrint("%s: Warning: Inconsistent count (%u and %u)", __FUNCTION__, FileWriteCount, 1);
        // Don't go to ReturnError.
    }
    // Close file
    Er = AmpCFS_fclose(YFp);
    if (Er != 0) {
        ErrMsg = "AmpCFS_fclose failed";
        goto ReturnError;
    }
    Er = AmpCFS_fclose(UvFp);
    if (Er != 0) {
        ErrMsg = "AmpCFS_fclose failed";
        goto ReturnError;
    }
    /*
     jpegEncConfig.MaxNumQTable = 1; //main+thm+scrn
     AmpUT_initJpegDqt(AmpUTJpegQTable[0], 100);
     AmpUT_initJpegDqt(AmpUTJpegQTable[1], 95);
     AmpUT_initJpegDqt(AmpUTJpegQTable[2], 98);
     jpegEncConfig.pQTable = AmpUTJpegQTable;
     jpegEncConfig.pBitsBufAddr = BSAddr;
     jpegEncConfig.BitsBufSize = 16*1024*1024;
     AmbaDSP_JpegEncConfig(&jpegEncConfig);
     */

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
 * Stop video.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_Stop(void)
{
    int Er = 0;
    char *ErrMsg = "";
    AmbaPrint("%s", __FUNCTION__);

    // Close dummy demuxer
    Er = AmpUT_Video_DummyDmx_Close(DmxHdlr);
    if (Er != 0) {
        ErrMsg = "AmpUT_Video_DummyDmx_Close failed";
        goto ReturnError;
    }
    Er = AmpFifo_EraseAll(DmxHdlr->AvcDecFifoHdlr);
    if (Er != 0) {
        ErrMsg = "AmpFifo_EraseAll failed";
        goto ReturnError;
    }

    // Stop decoder
    Er = AmpDec_Stop(DecPipeHdlr);
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
 * Stop video and keep the last frame.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_Stop1(void)
{
    int Er = 0;
    char *ErrMsg = "";
    AmbaPrint("%s", __FUNCTION__);

    // Close dummy demuxer
    Er = AmpUT_Video_DummyDmx_Close(DmxHdlr);
    if (Er != 0) {
        ErrMsg = "AmpUT_Video_DummyDmx_Close failed";
        goto ReturnError;
    }
    Er = AmpFifo_EraseAll(DmxHdlr->AvcDecFifoHdlr);
    if (Er != 0) {
        ErrMsg = "AmpFifo_EraseAll failed";
        goto ReturnError;
    }

    // Stop decoder while keeping the last frame on screen
    Er = AmpDec_StopWithLastFrm(DecPipeHdlr);
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
 * Deinit video decode.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_Exit(void)
{
    char *ErrMsg = "";
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
    if (DecPipeHdlr != NULL) {
        if (AmpDec_Remove(DecPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to remove the video decoder manager.", __FUNCTION__, __LINE__);
        }
        if (AmpDec_Delete(DecPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to delete the video decoder manager.", __FUNCTION__, __LINE__);
        }
        DecPipeHdlr = NULL;
    }

    // Deinit video decoder
    if (AvcDecHdlr != NULL) {
        if (AmpVideoDec_Delete(AvcDecHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to deinit the video decoder.", __FUNCTION__, __LINE__);
        }
        AvcDecHdlr = NULL;
    }

    // Deinit LCD and TV windows
    {
        // Delete LCD Window
        if (AmpUT_Display_Window_Delete(0) != AMP_OK) {
            AmbaPrint("%s:%u Failed to delete LCD window handler.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpUT_Display_Window_Delete failed";
            goto ReturnError;
        }

        // Delete TV Window
        if (AmpUT_Display_Window_Delete(1) != AMP_OK) {
            AmbaPrint("%s:%u Failed to delete TV window handler.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpUT_Display_Window_Delete failed";
            goto ReturnError;
        }
    }

    // Release descriptor buffer
    if (AvcDescBufferOri != NULL) {
        if (AmbaKAL_BytePoolFree(AvcDescBufferOri) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the descriptor buffer.", __FUNCTION__, __LINE__);
        }
        AvcDescBufferOri = NULL;
    }

    // Release raw buffer
    if (AvcRawBufferOri != NULL) {
        if (AmbaKAL_BytePoolFree(AvcRawBufferOri) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the raw buffer.", __FUNCTION__, __LINE__);
        }
        AvcRawBuffer = NULL;
        AvcRawBufferOri = NULL;
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
 * Enable/disable fading.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_Fade(char** argv)
{
    /** Configure fading settings */
    AmbaPrint("%s", __FUNCTION__);

    FadeEnable = atoi(argv[2]);
    if (FadeEnable == 0) {
        AmbaPrint("%s: disable", __FUNCTION__);
    }else{
        AmbaPrint("%s: enable", __FUNCTION__);
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;
}

/**
 * Set prefeed number.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_SetPreFeed(char** argv)
{
    // Configure prefeed settings
    AmbaPrint("%s", __FUNCTION__);

    PreFeedNum = atoi(argv[2]);

    AmbaPrint("Set prefeed number: %u", PreFeedNum);

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;
}

/**
 * Video decode usage.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDec_Usage(void)
{
    AmbaPrint("AmpUT_VideoDec");
    AmbaPrint("\t init");
    AmbaPrint("\t start [filePath] [videoWidth] [videoHeight] [startTime] [Speed] [Dir] - start decode");
    AmbaPrint("\t       [filePath]      the .nhnt/.h264/.udta file full path excluding file extension");
    AmbaPrint("\t       [videoWidth]    video width");
    AmbaPrint("\t       [videoHeight]   video height");
    AmbaPrint("\t       [startTime]     time (in ms) of the first displayed video frame");
    AmbaPrint("\t       [Speed]         128 - 0.5x, 256 - 1x, 512 - 2x");
    AmbaPrint("\t       [Dir]           0 - forward, 1 - backward");
    AmbaPrint("\t zoom [videoWidth] [videoHeight] [cropOffsetX] [cropOffsetY] [cropWidth] [cropHeight] - crop part of video");
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
    AmbaPrint("\t cap [width] [height] - capture YUV during pause");
    AmbaPrint("\t fade [enable] - 1: enable 0: disable");
    return 0;
}

/**
 * Pre-processing before entering standby
 */
static void AmpUT_VideoDec_RegisterResumeInt(void)
{
    // At this moment, all INTs are disabled by AmbaPLL, we have to enable some in order to wake up.
#ifdef CONFIG_SOC_A12
    AmbaINT_IntEnable(AMBA_VIC_INT_ID9_UART0); // Use UART INT to wake
#endif
}

/**
 * Video decode test.
 *
 * @param [in] env
 * @param [in] argc         Command line argument count
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_VideoDecTest(struct _AMBA_SHELL_ENV_s_ *env,
                              int argc,
                              char **argv)
{
    AmbaPrint("AmpUT_VideoDecTest cmd: %s", argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_VideoDec_Init(atoi(argv[2]));
    } else if (strcmp(argv[1], "start") == 0) {
        AmpUT_VideoDec_Start(argc, argv);
    } else if (strcmp(argv[1], "pause") == 0) {
        AmpUT_VideoDec_Pause();
    } else if (strcmp(argv[1], "step") == 0) {
        AmpUT_VideoDec_Step();
    } else if (strcmp(argv[1], "resume") == 0) {
        AmpUT_VideoDec_Resume();
    } else if (strcmp(argv[1], "stop1") == 0) {
        AmpUT_VideoDec_Stop1();
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_VideoDec_Stop();
    } else if (strcmp(argv[1], "cap") == 0) {
        AmpUT_VideoDec_Cap(argc, argv);
    } else if (strcmp(argv[1], "exit") == 0) {
        AmpUT_VideoDec_Exit();
    } else if (strcmp(argv[1], "zoom") == 0) {
        AmpUT_VideoDec_Zoom(argv);
    } else if (strcmp(argv[1], "fade") == 0) {
        AmpUT_VideoDec_Fade(argv);
    } else if (strcmp(argv[1], "ch") == 0) {
        AmpUT_VideoDec_ConfigDisplayCh(argv);
    } else if (strcmp(argv[1], "prefeed") == 0) {
        AmpUT_VideoDec_SetPreFeed(argv);
    } else if (strcmp(argv[1], "cvbs") == 0) {
        AmpUT_Display_CvbsCtrl(atoi(argv[2]));
    } else if (strcmp(argv[1], "suspend") == 0) {
        AMP_DEC_CODEC_CONFIG_SUS_DSP_ON_STOP_s DecSuspend = {0};
        AmbaPrint("AmpUT_VideoDec_SetSuspend %u", (UINT8)atoi(argv[2]));
        DecSuspend.Idx = AMP_DEC_CODEC_CONFIG_SUS_DSP_ON_STOP;
        DecSuspend.SusDspOnStop = (UINT8)atoi(argv[2]);
        AmpDec_Config(DecPipeHdlr, &DecSuspend);
    } else if ((strcmp(argv[1],"standby") == 0)) {
        AmbaPrint("Enter Standby mode");
        AmbaIPC_LinkCtrlSuspendLinux(AMBA_LINK_SLEEP_TO_RAM);
        if (AmbaIPC_LinkCtrlWaitSuspendLinux(5000) != OK) {
            AmbaPrint("SuspendLinux (%d) failed!\n", AMBA_LINK_SLEEP_TO_RAM);
        }
        AmbaPrintk_Flush();
        AmbaLCD_SetBacklight(0, 0);
        AmbaPLL_SetStandbyMode(0, AmpUT_VideoDec_RegisterResumeInt, NULL);
        AmbaIPC_LinkCtrlResumeLinux(AMBA_LINK_SLEEP_TO_RAM);
        if (AmbaIPC_LinkCtrlWaitResumeLinux(5000) != OK) {
           AmbaPrint("ResumeLinux (%d) failed.\n", AMBA_LINK_SLEEP_TO_RAM);
        }
        AmbaLCD_SetBacklight(0, 1);
    } else {
        AmpUT_VideoDec_Usage();
    }
    return 0;
}

/**
 * Hook video decode command.
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_VideoDecInit(void)
{
// hook command
    AmbaTest_RegisterCommand("videodec", AmpUT_VideoDecTest);
    AmbaTest_RegisterCommand("vd", AmpUT_VideoDecTest);

    return AMP_OK;
}
