 /**
  * @file src/app/sample/unittest/AmpUT_MotorVu360.c
  *
  * MotorVu360 Encode/Liveview unit test
  *
  * History:
  *    2014/07/02 - [Jenghung Luo] created file
  *
  * Copyright (C) 2014~, Ambarella, Inc.
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
#include <AmbaSensor.h>
#include <vin/vin.h>
#include <recorder/Encode.h>
#include <recorder/VideoEnc.h>
#include <display/Osd.h>
#include <display/Display.h>
#include <util.h>
#include "AmbaAudio.h"
#include <AmbaSensor_B5_IMX122.h>
#include <AmbaSensor_IMX122.h>
#include <AmbaDSP_WarpCore.h>
#include "AmbaImg_VDsp_Handler.h"
#include "AmbaImg_VIn_Handler.h"
#include <recorder/StillEnc.h>
#include <scheduler.h>
#include "AmbaUtility.h"
#include "AmbaPLL.h"
#include <cfs/AmpCfs.h>

#include "AmbaImg_Proc.h"
#include "AmbaImg_Impl_Cmd.h"


static char DefaultSlot[] = "C";

void *UT_MotorVu360fopen(const char *pFileName, const char *pMode, BOOL8 AsyncMode)
{

    AMP_CFS_FILE_PARAM_s cfsParam;
    AmpCFS_GetFileParam(&cfsParam);
    cfsParam.AsyncMode = AsyncMode;
    cfsParam.AsyncData.MaxNumBank = 2;
    if (pMode[0] == 'w' && pMode[1] == '+') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_WRITE_READ;
    } else if (pMode[0] == 'w') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_WRITE_ONLY;
    } else if (pMode[0] == 'r' && pMode[1] == '+') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_READ_WRITE;
    } else if (pMode[0] == 'r') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
    } else if (pMode[0] == 'a' && pMode[1] == '+') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_APPEND_READ;
    } else if (pMode[0] == 'a') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_APPEND_ONLY;
    }

    if(AmbaFS_GetCodeMode() == AMBA_FS_UNICODE) {
        AmbaPrint("[%s]: Unicode should be abandoned", __func__);
        return NULL;
    } else {
        strcpy((char *)cfsParam.Filename, pFileName);
        return (void *)AmpCFS_fopen(&cfsParam);
    }
}

int UT_MotorVu360fclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_MotorVu360fread(void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fread(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_MotorVu360fwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_MotorVu360fsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_MotorVu360fsize(void *pFile)
{
    AMP_CFS_STAT fStat;

    AmpCFS_Stat((AMP_CFS_FILE_s *)pFile, &fStat);
    return fStat.Size;
}

// Global var for VideoEnc codec
static AMP_VIN_HDLR_s *MotorVuVinA = NULL;       // Vin instance
static AMP_AVENC_HDLR_s *MotorVuPri = NULL;      // Primary VideoEnc codec instance
static AMP_AVENC_HDLR_s *MotorVuSec = NULL;      // Secondary VideoEnc codec instance
static AMP_ENC_PIPE_HDLR_s *VideoEncPipe = NULL;  // Encode pipeline  instance
static AMBA_IMG_SCHDLR_HDLR_s *ImgSchdlr = NULL;          // Image scheduler instance
static UINT8 *VinWorkBuffer = NULL;               // Vin module working buffer
static UINT8 *VEncWorkBuffer = NULL;              // VideoEnc working buffer
static UINT8 *ImgSchdlrWorkBuffer = NULL;         // Img scheduler working buffer
static AMBA_DSP_CHANNEL_ID_u VinChannel;          // Vin channel

#define ENCODE_FOV_NUM           4
#define VOUT_FOV_NUM             6
#define TOTAL_FOV_NUM           (ENCODE_FOV_NUM + VOUT_FOV_NUM)
#define MULTICHAN_BLEND_TABLE_FROM_FILE
static AMBA_IMG_SCHDLR_HDLR_s *ImgMultiChanSchdlr[TOTAL_FOV_NUM] = {0};

#define STATUS_IDLE     1
#define STATUS_INIT     2
#define STATUS_LIVEVIEW 3
#define STATUS_ENCODE   4
#define STATUS_PAUSE    5
static UINT8 Status = STATUS_IDLE;                  // Simple state machine to avoid wrong command issuing

extern __weak AMBA_SENSOR_OBJ_s AmbaSensor_B5_IMX122Obj;
static UINT8 TvLiveview = 1;                        // Enable TV liveview or not
static UINT8 LCDLiveview = 1;                       // Enable LCD liveview or not
static UINT8 MVu3AEnable = 0;                       // Enable AE/AWB/ADJ algo or not
static UINT8 EncPriSpecH264 = 1;                    // Primary codec instance output spec. 1 = H264, 0 = MJPEG
static UINT8 EncSecSpecH264 = 1;                    // Secondary codec instance output spec. 1 = H264, 0 = MJPEG
static UINT8 EncDualStream = 0;                     // Enable dual stream or not

// OSD
static void *OsdhdlrAddr = NULL;
static UINT16 *OsdLcdBuffer, *OsdTvBuffer;
static AMP_OSD_HDLR_s *OsdLHdlr = NULL;              ///< osd handler for lcd
static AMP_OSD_HDLR_s *OsdTHdlr = NULL;               ///< osd handler for tv
static AMP_DISP_WINDOW_HDLR_s *OsdWinLcdHdlr = NULL;   ///< display window handler for LCD
static AMP_DISP_WINDOW_HDLR_s *OsdWinTvHdlr = NULL;    ///< display window handler for TV


static UINT8 MjpegQuantMatrix[128] = {              // Standard JPEG qualty 50 table.
    0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, 0x0E,
    0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28,
    0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25,
    0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33,
    0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44,
    0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57,
    0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71,
    0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63,
    0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2F, 0x1A,
    0x1A, 0x2F, 0x63, 0x42, 0x38, 0x42, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63
};

UINT32 AmpUT_MotorVu360_DspWork_Calculate(UINT8 **addr, UINT32 *size);


// A simple sensor mode management table
typedef struct a {
    UINT32 SensorMode;      // Sensor mode
    UINT16 CaptureWidth;    // Vin capture window width
    UINT16 CaptureHeight;   // Vin capture window height
    UINT16 MainWidth;       // Main window width
    UINT16 MainHeight;      // Main window height
    UINT32 TimeScale;       // Time scale of frame rate
    UINT32 TickPerPicture;  // Tick per frame
    UINT16 AspectRatio;     // Main window aspect ratio
    UINT8 GopM;             // P frame distance
    UINT8 DualValid;        // This mode can do dual stream or not
    UINT8 BrcMode;          // Bitrate control mode
    UINT8 MaxBitRate;       // Maximum bitrate
    UINT8 MinBitRate;       // Minimum bitrate
    UINT8 AverageBitRate;   // Average bitrate
    char Name[20];          // Name of this index
} encmgt;

static encmgt VideoEncMgt122B5[] = {
    [0] = {
        .SensorMode = AMBA_SENSOR_B5_IMX122_TYPE_2_9_MODE_1920_1080_30P,
//        .CaptureWidth = 3840,
        .CaptureWidth = 3840,
        .CaptureHeight = 1080,
//        .MainWidth = 992,
//        .MainHeight = 736,
         .MainWidth = 3840,
        .MainHeight = 1080,

        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .AspectRatio = VAR_16x9,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_CBR,
        .MaxBitRate = 18,
        .MinBitRate = 6,
        .AverageBitRate = 12,
        .Name = "1920x1080P30 16:9\0",
    },
};

static encmgt *VideoEncMgt;         // Pointer to above tables
static UINT8 EncModeIdx = 1;        // Current mode index

#define GOP_N   8                   // I-frame distance
#define GOP_IDR 8                   // IDR-frame distance

#define SEC_STREAM_WIDTH   432      // Secondary codec output width
#define SEC_STREAM_HEIGHT  240      // Secondary codec output height
#define SEC_STREAM_TIMESCALE 30000  // Secondary codec frame rate time scale
#define SEC_STREAM_TICK 1001        // Secondary codec frame rate tick per frame

static AMP_DISP_WINDOW_HDLR_s *LCDWinHdlr[VOUT_FOV_NUM] = {0}; // LCD video plane window handler
static AMP_DISP_WINDOW_HDLR_s *TVWinHdlr[VOUT_FOV_NUM] = {0}; // LCD video plane window handler

/** Description of the real sensors mounted on B5F */
 static AMBA_DSP_CHANNEL_ID_u SensorChannel[4] = {
    [0] = {
        .Bits.VinID = 0,
        .Bits.SensorID = 1,
    },
    [1] = {
        .Bits.VinID = 0,
        .Bits.SensorID = 2,
    },
    [2] = {
        .Bits.VinID = 0,
        .Bits.SensorID = 4,
    },
    [3] = {
        .Bits.VinID = 0,
        .Bits.SensorID = 8,
    },
};
static AMP_AREA_s SensorCaptureWindow[4] = {
    [0] = {
        .X = 0, .Y = 0,
        .Width = 1920, .Height = 1080,
    },
    [1] = {
        .X = 0, .Y = 0,
        .Width = 1920, .Height = 1080,
    },
    [2] = {
        .X = 0, .Y = 0,
        .Width = 1920, .Height = 1080,
    },
    [3] = {
        .X = 0, .Y = 0,
        .Width = 1920, .Height = 1080,
    },
};

#define SUR_MAIN_WIDTH 352
#define SUR_MAIN_HEIGHT 352
#define SUR_MAIN_HEIGHT_LARGE 480

/** Every FOV comes from a main window */
static AMP_MULTI_CHAN_MAIN_WINDOW_CFG_s FovMainWindow[TOTAL_FOV_NUM] = {
    /* Liveview */
    [0] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[0],
        .SensorCapWin = &SensorCaptureWindow[0],
        .MainViewID = 0,
        .MainCapWin = {.X = 0, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = SUR_MAIN_WIDTH,
        .MainHeight = SUR_MAIN_HEIGHT,
    },
    [1] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[1],
        .SensorCapWin = &SensorCaptureWindow[1],
        .MainViewID = 1,
        .MainCapWin = {.X = 1920, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = SUR_MAIN_WIDTH,
        .MainHeight = SUR_MAIN_HEIGHT,
    },
    [2] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[2],
        .SensorCapWin = &SensorCaptureWindow[2],
        .MainViewID = 2,
        .MainCapWin = {.X = 3840, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = SUR_MAIN_WIDTH,
        .MainHeight = SUR_MAIN_HEIGHT,
    },
    [3] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[3],
        .SensorCapWin = &SensorCaptureWindow[3],
        .MainViewID = 3,
        .MainCapWin = {.X = 5760, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = SUR_MAIN_WIDTH,
        .MainHeight = SUR_MAIN_HEIGHT,
    },
    [4] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[1],
        .SensorCapWin = &SensorCaptureWindow[1],
        .MainViewID = 4,
        .MainCapWin = {.X = 3840, .Y = 0, .Width = 720, .Height = 960},
        .MainWidth = SUR_MAIN_WIDTH,
        .MainHeight = SUR_MAIN_HEIGHT_LARGE,
    },
    [5] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[2],
        .SensorCapWin = &SensorCaptureWindow[2],
        .MainViewID = 5,
        .MainCapWin = {.X = 5760, .Y = 0, .Width = 720, .Height = 960},
        .MainWidth = SUR_MAIN_WIDTH,
        .MainHeight = SUR_MAIN_HEIGHT_LARGE,
    },
    /* Encode */
    [6] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[0],
        .SensorCapWin = &SensorCaptureWindow[0],
        .MainViewID = 6,
        .MainCapWin = {.X = 0, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = 1280,
        .MainHeight = 720,
    },
    [7] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[1],
        .SensorCapWin = &SensorCaptureWindow[1],
        .MainViewID = 7,
        .MainCapWin = {.X = 1920, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = 1280,
        .MainHeight = 720,
    },
    [8] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[2],
        .SensorCapWin = &SensorCaptureWindow[2],
        .MainViewID = 8,
        .MainCapWin = {.X = 3840, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = 1280,
        .MainHeight = 720,
    },
    [9] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[3],
        .SensorCapWin = &SensorCaptureWindow[3],
        .MainViewID = 9,
        .MainCapWin = {.X = 5760, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = 1280,
        .MainHeight = 720,
    },
};
#define FULLHD_VOUT

/** Definition for SD resolution */
#define SD_PLANE_TOTAL_WIDTH  720
#define SD_PLANE_TOTAL_HEIGHT 480
#define SD_SUR_HBAR_WIDTH     328
#define SD_SUR_HBAR_HEIGHT    118
#define SD_SUR_VBAR_WIDTH     118
#define SD_SUR_VBAR_HEIGHT    480
#define SD_FOCUS_WIDTH        192
#define SD_FOCUS_HEIGHT       256
#define SD_FOCUS_GAP          (SD_PLANE_TOTAL_WIDTH - SD_FOCUS_WIDTH*2 - SD_SUR_HBAR_WIDTH)
/** Definition for FHD resolution */
#define FHD_PLANE_TOTAL_WIDTH  1920
#define FHD_PLANE_TOTAL_HEIGHT 1080
#define FHD_SUR_HBAR_WIDTH      738
#define FHD_SUR_HBAR_HEIGHT     264
#define FHD_SUR_VBAR_WIDTH      264
#define FHD_SUR_VBAR_HEIGHT    1080
#define FHD_FOCUS_WIDTH         576
#define FHD_FOCUS_HEIGHT        768
#define FHD_FOCUS_GAP          (FHD_PLANE_TOTAL_WIDTH - FHD_FOCUS_WIDTH*2 - FHD_SUR_HBAR_WIDTH)

/**
 *   Liveview FOVs
 *
 *   +---------+
 *   |\   0   /|                FOV_0: From main window 0, front camera
 *   | +-----+ | +---+---+      FOV_1: From main window 1, rear camera
 *   | |     | | |   |   |      FOV_2: From main window 2, left camera
 *   |2|     |3| | 4 | 5 |      FOV_3: From main window 3, right camera
 *   | |     | | |   |   |      FOV_4: From main window 4, left camera, act as left side mirror
 *   | +-----+ | +---+---+      FOV_5: From main window 5, right camera, act as right side mirror
 *   |/   1   \|
 *   +---------+
 */
static AMP_MULTI_CHAN_VOUT_WINDOW_CFG_s FovVoutWindow[2][VOUT_FOV_NUM] = {
    /* LCD */
    [0] = {
        [0] = {
            .Enable = 1,
            .MainWindowView = 0,
            .LayerID = 0,
            .DisplayWin = {.X = 0, .Y = 0, .Width = SD_SUR_HBAR_WIDTH, .Height = SD_SUR_HBAR_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = SD_SUR_HBAR_WIDTH, .Height = SD_SUR_HBAR_HEIGHT, .Pitch = ALIGN_32(SD_SUR_HBAR_WIDTH),},
        },
        [1] = {
            .Enable = 1,
            .MainWindowView = 1,
            .LayerID = 1,
            .DisplayWin = {.X = 0, .Y = (SD_PLANE_TOTAL_HEIGHT - SD_SUR_HBAR_HEIGHT), .Width = SD_SUR_HBAR_WIDTH, .Height = SD_SUR_HBAR_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = SD_SUR_HBAR_WIDTH, .Height = SD_SUR_HBAR_HEIGHT, .Pitch = ALIGN_32(SD_SUR_HBAR_WIDTH),},
        },
        [2] = {
            .Enable = 1,
            .MainWindowView = 2,
            .LayerID = 2,
            .DisplayWin = {.X = 0, .Y = 0, .Width = SD_SUR_VBAR_WIDTH, .Height = SD_SUR_VBAR_HEIGHT},
            .Rotate = AMP_ROTATE_270,
            .BlendTable = {.Buf = NULL, .Width = SD_SUR_VBAR_WIDTH, .Height = SD_SUR_VBAR_HEIGHT, .Pitch = ALIGN_32(SD_SUR_VBAR_WIDTH),},
        },
        [3] = {
            .Enable = 1,
            .MainWindowView = 3,
            .LayerID = 3,
            .DisplayWin = {.X = (SD_SUR_HBAR_WIDTH - SD_SUR_VBAR_WIDTH), .Y = 0, .Width = SD_SUR_VBAR_WIDTH, .Height = SD_SUR_VBAR_HEIGHT},
            .Rotate = AMP_ROTATE_90,
            .BlendTable = {.Buf = NULL, .Width = SD_SUR_VBAR_WIDTH, .Height = SD_SUR_VBAR_HEIGHT, .Pitch = ALIGN_32(SD_SUR_VBAR_WIDTH),},
        },
        [4] = {
            .Enable = 1,
            .MainWindowView = 4,
            .LayerID = 4,
            .DisplayWin = {.X = SD_SUR_HBAR_WIDTH+SD_FOCUS_GAP, .Y = (SD_PLANE_TOTAL_HEIGHT - SD_FOCUS_HEIGHT)>>1, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT, .Pitch = ALIGN_32(SD_FOCUS_WIDTH),},
        },
        [5] = {
            .Enable = 1,
            .MainWindowView = 5,
            .LayerID = 5,
            .DisplayWin = {.X = SD_PLANE_TOTAL_WIDTH - SD_FOCUS_WIDTH, .Y = (SD_PLANE_TOTAL_HEIGHT - SD_FOCUS_HEIGHT)>>1, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT, .Pitch = ALIGN_32(SD_FOCUS_WIDTH),},
        },
    },
    /* TV */
    [1] = {
#ifdef FULLHD_VOUT
        [0] = {
            .Enable = 1,
            .MainWindowView = 0,
            .LayerID = 0,
            .DisplayWin = {.X = 0, .Y = 0, .Width = FHD_SUR_HBAR_WIDTH, .Height = FHD_SUR_HBAR_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = FHD_SUR_HBAR_WIDTH, .Height = FHD_SUR_HBAR_HEIGHT, .Pitch = ALIGN_32(FHD_SUR_HBAR_WIDTH),},
        },
        [1] = {
            .Enable = 1,
            .MainWindowView = 1,
            .LayerID = 1,
            .DisplayWin = {.X = 0, .Y = (FHD_PLANE_TOTAL_HEIGHT - FHD_SUR_HBAR_HEIGHT), .Width = FHD_SUR_HBAR_WIDTH, .Height = FHD_SUR_HBAR_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = FHD_SUR_HBAR_WIDTH, .Height = FHD_SUR_HBAR_HEIGHT, .Pitch = ALIGN_32(FHD_SUR_HBAR_WIDTH),},
        },
        [2] = {
            .Enable = 1,
            .MainWindowView = 2,
            .LayerID = 2,
            .DisplayWin = {.X = 0, .Y = 0, .Width = FHD_SUR_VBAR_WIDTH, .Height = FHD_SUR_VBAR_HEIGHT},
            .Rotate = AMP_ROTATE_270,
            .BlendTable = {.Buf = NULL, .Width = FHD_SUR_VBAR_WIDTH, .Height = FHD_SUR_VBAR_HEIGHT, .Pitch = ALIGN_32(FHD_SUR_VBAR_WIDTH),},
        },
        [3] = {
            .Enable = 1,
            .MainWindowView = 3,
            .LayerID = 3,
            .DisplayWin = {.X = (FHD_SUR_HBAR_WIDTH - FHD_SUR_VBAR_WIDTH), .Y = 0, .Width = FHD_SUR_VBAR_WIDTH, .Height = FHD_SUR_VBAR_HEIGHT},
            .Rotate = AMP_ROTATE_90,
            .BlendTable = {.Buf = NULL, .Width = FHD_SUR_VBAR_WIDTH, .Height = FHD_SUR_VBAR_HEIGHT, .Pitch = ALIGN_32(FHD_SUR_VBAR_WIDTH),},
        },
        [4] = {
            .Enable = 0,
            .MainWindowView = 4,
            .LayerID = 4,
            .DisplayWin = {.X = FHD_SUR_HBAR_WIDTH + FHD_FOCUS_GAP, .Y = (FHD_PLANE_TOTAL_HEIGHT - FHD_FOCUS_HEIGHT)>>1,
                          .Width = FHD_FOCUS_WIDTH, .Height = FHD_FOCUS_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = FHD_FOCUS_WIDTH, .Height = FHD_FOCUS_HEIGHT, .Pitch = ALIGN_32(FHD_FOCUS_WIDTH),},
        },
        [5] = {
            .Enable = 1,
            .MainWindowView = 5,
            .LayerID = 5,
            .DisplayWin = {.X = FHD_PLANE_TOTAL_WIDTH - FHD_FOCUS_WIDTH, .Y = (FHD_PLANE_TOTAL_HEIGHT - FHD_FOCUS_HEIGHT)>>1,
                          .Width = FHD_FOCUS_WIDTH, .Height = FHD_FOCUS_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = FHD_FOCUS_WIDTH, .Height = FHD_FOCUS_HEIGHT, .Pitch = ALIGN_32(FHD_FOCUS_WIDTH),},
        },
#else
        [0] = {
            .Enable = 1,
            .MainWindowView = 0,
            .LayerID = 0,
            .DisplayWin = {.X = 0, .Y = 0, .Width = SD_SUR_HBAR_WIDTH, .Height = SD_SUR_HBAR_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = SD_SUR_HBAR_WIDTH, .Height = SD_SUR_HBAR_HEIGHT, .Pitch = ALIGN_32(SD_SUR_HBAR_WIDTH),},
        },
        [1] = {
            .Enable = 1,
            .MainWindowView = 1,
            .LayerID = 1,
            .DisplayWin = {.X = 0, .Y = (SD_PLANE_TOTAL_HEIGHT - SD_SUR_HBAR_HEIGHT), .Width = SD_SUR_HBAR_WIDTH, .Height = SD_SUR_HBAR_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = SD_SUR_HBAR_WIDTH, .Height = SD_SUR_HBAR_HEIGHT, .Pitch = ALIGN_32(SD_SUR_HBAR_WIDTH),},
        },
        [2] = {
            .Enable = 1,
            .MainWindowView = 2,
            .LayerID = 2,
            .DisplayWin = {.X = 0, .Y = 0, .Width = SD_SUR_VBAR_WIDTH, .Height = SD_SUR_VBAR_HEIGHT},
            .Rotate = AMP_ROTATE_270,
            .BlendTable = {.Buf = NULL, .Width = SD_SUR_VBAR_WIDTH, .Height = SD_SUR_VBAR_HEIGHT, .Pitch = ALIGN_32(SD_SUR_VBAR_WIDTH),},
        },
        [3] = {
            .Enable = 1,
            .MainWindowView = 3,
            .LayerID = 3,
            .DisplayWin = {.X = (SD_SUR_HBAR_WIDTH - SD_SUR_VBAR_WIDTH), .Y = 0, .Width = SD_SUR_VBAR_WIDTH, .Height = SD_SUR_VBAR_HEIGHT},
            .Rotate = AMP_ROTATE_90,
            .BlendTable = {.Buf = NULL, .Width = SD_SUR_VBAR_WIDTH, .Height = SD_SUR_VBAR_HEIGHT, .Pitch = ALIGN_32(SD_SUR_VBAR_WIDTH),},
        },
        [4] = {
            .Enable = 1,
            .MainWindowView = 4,
            .LayerID = 4,
            .DisplayWin = {.X = SD_SUR_HBAR_WIDTH+SD_FOCUS_GAP, .Y = (SD_PLANE_TOTAL_HEIGHT - SD_FOCUS_HEIGHT)>>1, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT, .Pitch = ALIGN_32(SD_FOCUS_WIDTH),},
        },
        [5] = {
            .Enable = 1,
            .MainWindowView = 5,
            .LayerID = 5,
            .DisplayWin = {.X = SD_PLANE_TOTAL_WIDTH - SD_FOCUS_WIDTH, .Y = (SD_PLANE_TOTAL_HEIGHT - SD_FOCUS_HEIGHT)>>1, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT, .Pitch = ALIGN_32(SD_FOCUS_WIDTH),},
        },

#endif
    },
};

/**
 *    Encode FOVs
 *     +---+---+
 *     |   |   |   FOV_6: From main window 6, front camera
 *     | 6 | 7 |   FOV_7: From main window 7, rear camera
 *     |   |   |   FOV_8: From main window 8, left camera
 *     +---+---+   FOV_9: From main window 9, right camera
 *     |   |   |
 *     | 8 | 9 |
 *     |   |   |
 *     +---+---+
 */
static AMP_MULTI_CHAN_ENCODE_WINDOW_CFG_s FovEncodeWindow[2][ENCODE_FOV_NUM] = {
    /* 1st encoder */
    [0] = {
        [0] = {
            .Enable = 1,
            .MainWindowView = 6,
            .LayerID = 0,
            .SrcWin = {.X = 0, .Y = 0, .Width = 1280, .Height = 720},
            .DestWin = {.X = 0, .Y = 0, .Width = 1280, .Height = 720},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
        [1] = {
            .Enable = 1,
            .MainWindowView = 7,
            .LayerID = 1,
            .SrcWin = {.X = 0, .Y = 0, .Width = 1280, .Height = 720},
            .DestWin = {.X = 1280, .Y = 0, .Width = 1280, .Height = 720},
            .Rotate = AMP_ROTATE_180_HORZ_FLIP,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
        [2] = {
            .Enable = 1,
            .MainWindowView = 8,
            .LayerID = 2,
            .SrcWin = {.X = 0, .Y = 0, .Width = 1280, .Height = 720},
            .DestWin = {.X = 0, .Y = 720, .Width = 1280, .Height = 720},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
        [3] = {
            .Enable = 1,
            .MainWindowView = 9,
            .LayerID = 3,
            .SrcWin = {.X = 0, .Y = 0, .Width = 1280, .Height = 720},
            .DestWin = {.X = 1280, .Y = 720, .Width = 1280, .Height = 720},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
    },

    /* 2nd encoder */
    [1] = {
        [0] = {
            .Enable = 0,
            .MainWindowView = 5,
            .LayerID = 0,
            .SrcWin = {.X = 0, .Y = 0, .Width = 432, .Height = 240},
            .DestWin = {.X = 0, .Y = 0, .Width = 432, .Height = 240},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
        [1] = {
            .Enable = 0,
            .MainWindowView = 6,
            .LayerID = 1,
            .SrcWin = {.X = 0, .Y = 0, .Width = 432, .Height = 240},
            .DestWin = {.X = 432, .Y = 0, .Width = 432, .Height = 240},
            .Rotate = AMP_ROTATE_180_HORZ_FLIP,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
        [2] = {
            .Enable = 0,
            .MainWindowView = 7,
            .LayerID = 2,
            .SrcWin = {.X = 0, .Y = 0, .Width = 432, .Height = 240},
            .DestWin = {.X = 0, .Y = 240, .Width = 432, .Height = 240},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
        [3] = {
            .Enable = 0,
            .MainWindowView = 8,
            .LayerID = 3,
            .SrcWin = {.X = 0, .Y = 0, .Width = 432, .Height = 240},
            .DestWin = {.X = 432, .Y = 240, .Width = 432, .Height = 240},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
    },

};

/* Simple muxer to communicate with FIFO */
#define BITSFIFO_SIZE 14*1024*1024
static UINT8 *H264BitsBuf;          // H.264 bitstream buffer
static UINT8 *MjpgBitsBuf;          // MJPEG bitstream buffer
#define DESC_SIZE 40*1680
static UINT8 *H264DescBuf;          // H.264 descriptor buffer (physical)
static UINT8 *MjpgDescBuf;          // MJPEG descriptor buffer (physical)
static AMBA_KAL_TASK_t MotorVuPriMuxTask = {0};    // Primary stream muxer task
static AMBA_KAL_TASK_t MotorVuSecMuxTask = {0};    // Secondary stream muxer task
static UINT8 *MotorVuPriMuxStack = NULL;           // Stack for primary stream muxer task
static UINT8 *MotorVuSecMuxStack = NULL;           // Stack for secondary stream muxer task
static AMBA_KAL_SEM_t MotorVuPriSem = {0};         // Counting semaphore for primary stream muxer task and fifo callback
static AMBA_KAL_SEM_t MotorVuSecSem = {0};         // Counting semaphore for secondary stream muxer task and fifo callback

static AMP_CFS_FILE_PARAM_s cfsParam;
static AMP_CFS_FILE_s *outputPriFile = NULL;       // Output file pointer for primary stream
static AMP_CFS_FILE_s *outputSecFile = NULL;       // Output file pointer for secondary stream
static AMP_CFS_FILE_s *outputPriIdxFile = NULL;    // Output file pointer for primary stream index (frame offset/size...etc)
static AMP_CFS_FILE_s *outputSecIdxFile = NULL;    // Output file pointer for secondary stream index (frame offset/size...etc)
static AMP_CFS_FILE_s *UdtaPriFile = NULL;         // Output file pointer for primary stream UserData
static AMP_CFS_FILE_s *UdtaSecFile = NULL;         // Output file pointer for secondary stream UserData
static AMP_CFS_FILE_s *BlendTable = NULL;          // Input file pointer for Blending table

static int fnoPri = 0;                  // Frame number counter for primary stream muxer
static int fnoSec = 0;                  // Frame number counter for secondary stream muxer
static UINT32 EncFrameRate = 0;         // Remember current framerate for primary muxer to calculate actual bitrate
static UINT64 encPriBytesTotal = 0;     // total bytes primary stream muxer received
static UINT32 encPriTotalFrames = 0;    // total frames primary stream muxer received
static UINT64 encSecBytesTotal = 0;     // total bytes secondary stream muxer received
static UINT32 encSecTotalFrames = 0;    // total frames secondary stream muxer received
static AMP_FIFO_HDLR_s *VirtualPriFifoHdlr = NULL;  // Primary stream vitrual fifo
static AMP_FIFO_HDLR_s *VirtualSecFifoHdlr = NULL;  // Secondary stream vitrual fifo
static UINT8 LogMuxer = 0;              // Log muxing info
static UINT8 VdspPrint = 0;             // Print vdsp ack
static UINT32 VdspCount = 0;            // vdsp counter


/** UT function prototype */
int AmpUT_MotorVu360_EncStop(void);
int AmpUT_MotorVu360_ChangeResolution(UINT32 modeIdx);
int AmpUT_MotorVu360_LiveviewStart(UINT32 modeIdx);
int AmpUT_MotorVu360_ChangeMainView(UINT32 MainViewID);

/*************************************** Muxer (Data flow) implementation start ********************************************************/

/**
 * Primary muxer task
 *
 * @param [in] info initial value
 *
 */

static UINT32 BrcFrameCount = 0;
static UINT32 BrcByteCount = 0;

void AmpUT_MotorVu360_PriMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc;
    int Er;
    UINT8 *BitsLimit;

    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_MotorVu360_PriMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&MotorVuPriSem, 10000);  // Check if there is any pending frame to be muxed
        if (Er != OK) {
          //  AmbaPrint(" no sem fff");
            continue;
        }

#define AMPUT_FILE_DUMP     // When defined, it will write data to SD card. Otherwise it just lies to fifo that it has muxed pending data
    #ifdef AMPUT_FILE_DUMP

        if (outputPriFile == NULL) { // Open files when receiving the 1st frame
            char Fn[80];
            char mdASCII[3] = {'w','+','\0'};
            FORMAT_USER_DATA_s Udta;

            sprintf(Fn,"%s:\\OUT_%04d.%s", DefaultSlot, fnoPri, EncPriSpecH264? "h264": "mjpg");
            outputPriFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            while (outputPriFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                outputPriFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            }
            AmbaPrint("%s opened", Fn);

            sprintf(Fn,"%s:\\OUT_%04d.nhnt", DefaultSlot, fnoPri);
            outputPriIdxFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            while (outputPriIdxFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                outputPriIdxFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            }
            AmbaPrint("%s opened", Fn);

            sprintf(Fn,"%s:\\OUT_%04d.udta", DefaultSlot, fnoPri);
            UdtaPriFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            while (UdtaPriFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                UdtaPriFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            }
            AmbaPrint("%s opened", Fn);

            Udta.nIdrInterval = GOP_IDR/GOP_N;
            Udta.nTimeScale = VideoEncMgt[EncModeIdx].TimeScale;
            Udta.nTickPerPicture = VideoEncMgt[EncModeIdx].TickPerPicture;
            Udta.nN = GOP_N;
            Udta.nM = VideoEncMgt[EncModeIdx].GopM;
            Udta.nVideoWidth = VideoEncMgt[EncModeIdx].MainWidth;
            Udta.nVideoHeight = VideoEncMgt[EncModeIdx].MainHeight;

            UT_MotorVu360fwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)UdtaPriFile);
            UT_MotorVu360fclose((void *)UdtaPriFile);

            NhntHeader.Signature[0]='n';
            NhntHeader.Signature[1]='h';
            NhntHeader.Signature[2]='n';
            NhntHeader.Signature[3]='t';
            NhntHeader.TimeStampResolution = 90000;
            UT_MotorVu360fwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)outputPriIdxFile);

            if (EncPriSpecH264)
                BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
            else
                BitsLimit = MjpgBitsBuf + BITSFIFO_SIZE - 1;
        }
    #endif

        Er = AmpFifo_PeekEntry(VirtualPriFifoHdlr, &Desc, 0);  // Get a pending entry
        if (Er == 0) {
            char Ty[4];

            if (Desc->Type == AMP_FIFO_TYPE_IDR_FRAME) {
                Ty[0] = 'I'; Ty[1] = 'D'; Ty[2] = 'R'; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_I_FRAME) {
                Ty[0] = 'I'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_P_FRAME) {
                Ty[0] = 'P'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_B_FRAME) {
                Ty[0] = 'B'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_EOS) {
                Ty[0] = 'E'; Ty[1] = 'O'; Ty[2] = 'S'; Ty[3] = '\0';
            } else {
                Ty[0] = 'J'; Ty[1] = 'P'; Ty[2] = 'G'; Ty[3] = '\0';
            }
            if (LogMuxer)
                AmbaPrint("Pri[%d] %s pts:%8lld 0x%08x %d", encPriTotalFrames, Ty, Desc->Pts, Desc->StartAddr, Desc->Size);
        } else {
            while (Er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(VirtualPriFifoHdlr, &Desc, 0);
            }
        }
        if (Desc->Size == AMP_FIFO_MARK_EOS) {  // If EOS is met, close files
            UINT32 Avg;
            // EOS
        #ifdef AMPUT_FILE_DUMP
                if (outputPriFile) {
                    UT_MotorVu360fsync((void *)outputPriFile);
                    UT_MotorVu360fclose((void *)outputPriFile);
                    fnoPri++;
                    UT_MotorVu360fsync((void *)outputPriIdxFile);
                    UT_MotorVu360fclose((void *)outputPriIdxFile);
                    outputPriFile = NULL;
                    outputPriIdxFile = NULL;
                    FileOffset = 0;
                }
        #endif

            Avg = (UINT32)(encPriBytesTotal*8.0*EncFrameRate/encPriTotalFrames/1E6);

            AmbaPrint("Primary Muxer met EOS, total %d frames/fields", encPriTotalFrames);
            AmbaPrint("Primary Bitrate Average: %d Mbps\n", Avg);


            encPriBytesTotal = 0;
            encPriTotalFrames = 0;

        } else { // general muxing
        #ifdef AMPUT_FILE_DUMP
                if (outputPriFile) {
                    NhntSample.CompositionTimeStamp = Desc->Pts;
                    NhntSample.DecodingTimeStamp = Desc->Pts;
                    NhntSample.DataSize = Desc->Size;
                    NhntSample.FileOffset = FileOffset;
                    FileOffset += Desc->Size;
                    NhntSample.FrameType = Desc->Type;

                    UT_MotorVu360fwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)outputPriIdxFile);
             //       AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                    if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                        UT_MotorVu360fwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputPriFile);
                    } else {
                        UT_MotorVu360fwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputPriFile);
                        if (EncPriSpecH264) {
                            UT_MotorVu360fwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
                        } else {
                            UT_MotorVu360fwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
                        }
                    }
                }
        #else
               AmbaKAL_TaskSleep(1);    // Do nothing
        #endif

// Bitrate statistics
//            if (((BrcFrameCount%EncFrameRate) == 0)&& ((BrcFrameCount/EncFrameRate)%3 == 0) && BrcFrameCount) {
//                AmbaPrint("Pri AvgBitRate = %fMbps, Target %dMbps",(BrcByteCount*8.0*EncFrameRate/BrcFrameCount/1E6),VideoEncMgt[EncModeIdx].AverageBitRate);
//            }


            encPriBytesTotal += Desc->Size;
            encPriTotalFrames++;
            BrcFrameCount++;
            BrcByteCount+=Desc->Size;
        }
        AmpFifo_RemoveEntry(VirtualPriFifoHdlr, 1);
    }
}


/**
 * Secondary muxer task
 *
 * @param [in] info initial value
 *
 */

void AmpUT_MotorVu360_SecMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc;
    int Er;
    UINT8 *BitsLimit;

    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_MotorVu360_SecMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&MotorVuSecSem, 10000);
        if (Er!= OK) {
          //  AmbaPrint(" no sem fff");
            continue;
        }

#define AMPUT_FILE_DUMP
        #ifdef AMPUT_FILE_DUMP
        if (outputSecFile == NULL) { // Open files when receiving the 1st frame
            char Fn[80];
            char mdASCII[3] = {'w','+','\0'};
            FORMAT_USER_DATA_s Udta;

            sprintf(Fn,"%s:\\OUT_%04d_s.%s", DefaultSlot, fnoSec, EncSecSpecH264? "h264": "mjpg");
            outputSecFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            while (outputSecFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                outputSecFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            }
            AmbaPrint("%s opened", Fn);

            sprintf(Fn,"%s:\\OUT_%04d_s.nhnt", DefaultSlot, fnoSec);
            outputSecIdxFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            while (outputSecIdxFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                outputSecIdxFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            }
            AmbaPrint("%s opened", Fn);

            sprintf(Fn,"%s:\\OUT_%04d_s.udta", DefaultSlot, fnoSec);
            UdtaSecFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            while (UdtaSecFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                UdtaSecFile = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII,1);
            }
            AmbaPrint("%s opened", Fn);

            Udta.nIdrInterval = GOP_IDR/GOP_N;
            Udta.nTimeScale = SEC_STREAM_TIMESCALE;
            Udta.nTickPerPicture = SEC_STREAM_TICK;

            Udta.nN = 8;
            Udta.nM = 1;
            Udta.nVideoWidth = SEC_STREAM_WIDTH;
            Udta.nVideoHeight = SEC_STREAM_HEIGHT;

            UT_MotorVu360fwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)UdtaSecFile);
            UT_MotorVu360fclose((void *)UdtaSecFile);

            NhntHeader.Signature[0]='n';
            NhntHeader.Signature[1]='h';
            NhntHeader.Signature[2]='n';
            NhntHeader.Signature[3]='t';
            NhntHeader.TimeStampResolution = 90000;
            UT_MotorVu360fwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)outputSecIdxFile);
            fnoSec++;

            if (EncSecSpecH264) {
                BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
            } else {
                BitsLimit = MjpgBitsBuf + BITSFIFO_SIZE - 1;
            }
        }
        #endif

        Er = AmpFifo_PeekEntry(VirtualSecFifoHdlr, &Desc, 0); // Get a pending entry
        if (Er == 0) {
            char Ty[4];

            if (Desc->Type == AMP_FIFO_TYPE_IDR_FRAME) {
                Ty[0] = 'I'; Ty[1] = 'D'; Ty[2] = 'R'; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_I_FRAME) {
                Ty[0] = 'I'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_P_FRAME) {
                Ty[0] = 'P'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_B_FRAME) {
                Ty[0] = 'B'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_EOS) {
                Ty[0] = 'E'; Ty[1] = 'O'; Ty[2] = 'S'; Ty[3] = '\0';
            } else {
                Ty[0] = 'J'; Ty[1] = 'P'; Ty[2] = 'G'; Ty[3] = '\0';
            }

            if (LogMuxer)
                AmbaPrint("Sec[%d] %s pts:%8lld 0x%08x %d", encSecTotalFrames, Ty, Desc->Pts, Desc->StartAddr, Desc->Size);
        } else {
            while (Er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(VirtualSecFifoHdlr, &Desc, 0);
            }
        }
        if (Desc->Size == AMP_FIFO_MARK_EOS) {  // If EOS is met, close files
            UINT32 Avg;
            // EOS
            #ifdef AMPUT_FILE_DUMP
                if (outputSecFile) {
                    UT_MotorVu360fsync((void *)outputSecFile);
                    UT_MotorVu360fclose((void *)outputSecFile);
                    UT_MotorVu360fsync((void *)outputSecIdxFile);
                    UT_MotorVu360fclose((void *)outputSecIdxFile);
                    outputSecFile = NULL;
                    outputSecIdxFile = NULL;
                    FileOffset = 0;
                }
            #endif

            Avg = (UINT32)(encSecBytesTotal*8.0*(SEC_STREAM_TIMESCALE/SEC_STREAM_TICK)/encSecTotalFrames/1E6);

            AmbaPrint("Secondary Muxer met EOS, total %d frames/fields", encSecTotalFrames);
            AmbaPrint("Secondary Bitrate Average: %d Mbps\n", Avg);

            encSecBytesTotal = 0;
            encSecTotalFrames = 0;
        } else { // General muxing
            #ifdef AMPUT_FILE_DUMP
                if (outputSecFile) {
                    NhntSample.CompositionTimeStamp = Desc->Pts;
                    NhntSample.DecodingTimeStamp = Desc->Pts;
                    NhntSample.DataSize = Desc->Size;
                    NhntSample.FileOffset = FileOffset;
                    FileOffset += Desc->Size;
                    NhntSample.FrameType = Desc->Type;

                    encSecBytesTotal += Desc->Size;
                    encSecTotalFrames ++;

                    UT_MotorVu360fwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)outputSecIdxFile);
              //      AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                    if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                        UT_MotorVu360fwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputSecFile);
                    } else {
                        UT_MotorVu360fwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputSecFile);
                        if (EncSecSpecH264) {
                            UT_MotorVu360fwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputSecFile);
                        } else {
                            UT_MotorVu360fwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputSecFile);
                        }
                    }
                }
            #else
                AmbaKAL_TaskSleep(1);
            #endif
        }
        AmpFifo_RemoveEntry(VirtualSecFifoHdlr, 1);
    }
}


/**
 * Fifo callback when new entries are produced
 *
 * @param [in] hdlr  The event belongs to which virtual fifo
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */

static int AmpUT_MotorVu360_FifoCB(void *hdlr, UINT32 event, void* info)
{
    UINT32 *NumFrames = info;
//    UINT32 cnt;
    AMBA_KAL_SEM_t *pSem;

    if (hdlr == VirtualPriFifoHdlr) pSem = &MotorVuPriSem;
    else if (hdlr == VirtualSecFifoHdlr) pSem = &MotorVuSecSem;

//    AmbaKAL_SemQuery(&VideoEncSem, &cnt);
//    AmbaPrint("AmpUT_FifoCB: %d", cnt);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        int i;

        for (i=0; i<*NumFrames; i++) {
            AmbaKAL_SemGive(pSem);      // Give semaphore for muxer tasks
        }
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
            AmbaKAL_SemGive(pSem);
    }

    return 0;
}


/*************************************** Muxer (Data flow) implementation end ********************************************************/

/**
 * Generic MotorVu360 ImgSchdlr callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_MotorVu360ImgSchdlrCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMBA_IMG_SCHDLR_EVENT_CFA_STAT_READY:
            if (VdspPrint) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                 AMBA_DSP_EVENT_CFA_3A_DATA_s *cfa = (AMBA_DSP_EVENT_CFA_3A_DATA_s *)Meta->AAAStatAddress;
                 static UINT32 LastVdspTime = 0;
          //       int k;
          //  AmbaPrint("LinY of channel %d:",cfa->Header.ChannelId);
          //  for (k=0; k<8; k++) {
          //      AmbaPrint("%d %d %d %d %d %d %d %d %d %d %d %d",cfa->Ae[k].LinY,cfa->Ae[k*8+1].LinY,cfa->Ae[k*8+2].LinY,cfa->Ae[k*8+3].LinY,cfa->Ae[k*8+4].LinY\
          //          ,cfa->Ae[k*8+5].LinY,cfa->Ae[k*8+6].LinY,cfa->Ae[k*8+7].LinY,cfa->Ae[k*8+8].LinY,cfa->Ae[k*8+9].LinY,cfa->Ae[k*8+10].LinY,cfa->Ae[k*8+11].LinY);
          //  }

                if (cfa->Header.ChannelId == 0 && (VdspCount%60 == 0)) {
                    int delta = (AmbaSysTimer_GetTickCount() - LastVdspTime);

                    if (VdspCount != 0) {
                        AmbaPrint("FrameRate = %f ", 60000.0/delta);
                    }
                    LastVdspTime = AmbaSysTimer_GetTickCount();
                }
                if (cfa->Header.ChannelId == 0)
                    VdspCount++;
            }

            if (MVu3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                AMBA_DSP_EVENT_CFA_3A_DATA_s *cfa = (AMBA_DSP_EVENT_CFA_3A_DATA_s *)Meta->AAAStatAddress;
                if (cfa->Header.ChannelId == 0) {
                    Amba_Img_VDspCfa_Handler(hdlr, (UINT32 *)Meta);
                }
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_RGB_STAT_READY:
            if (MVu3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                AMBA_DSP_EVENT_CFA_3A_DATA_s *cfa = (AMBA_DSP_EVENT_CFA_3A_DATA_s *)Meta->AAAStatAddress;
                if (cfa->Header.ChannelId == 0) {
                    Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
                }
            }
           break;
        default:
            AmbaPrint("[%s] Unknown %X info: %x", __func__, event, info);
            break;
    }
    return 0;
}


/**
 * VIN sensor mode switch callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_MotorVu360VinSwitchCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {

        case AMP_VIN_EVENT_INVALID:
            UINT8 i;
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_INVALID info: %X %X", info, ImgSchdlr);

            for (i=0; i<TOTAL_FOV_NUM; i++) {
                if (ImgMultiChanSchdlr[i]) AmbaImgSchdlr_Enable(ImgMultiChanSchdlr[i], 0);
            }
            Amba_Img_VIn_Invalid(hdlr, (UINT32 *)NULL);
            break;
        case AMP_VIN_EVENT_VALID:
            {
                UINT8 i;
                AmbaPrint("VinSWCB: AMP_VIN_EVENT_VALID info: %X %X", info, ImgSchdlr);
                if (MVu3AEnable) {
                Amba_Img_VIn_Valid(hdlr, (UINT32 *)NULL);
                }

                for(i=0; i<TOTAL_FOV_NUM; i++) {
                    if (ImgMultiChanSchdlr[i]) AmbaImgSchdlr_Enable(ImgMultiChanSchdlr[i], 1);
                }
            }
            break;
        case AMP_VIN_EVENT_CHANGED_PRIOR:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_PRIOR info: %X", info);
            if (MVu3AEnable)
                Amba_Img_VIn_Changed_Prior(hdlr, (UINT32 *)NULL);
            break;
        case AMP_VIN_EVENT_CHANGED_POST:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_POST info: %X", info);

            if (MVu3AEnable) {
                UINT8 IsPhotoLiveView = 0;
                //inform 3A LiveView type
                AmbaImg_Proc_Cmd(MW_IP_SET_PHOTO_PREVIEW, IsPhotoLiveView, 0, 0);
                Amba_Img_VIn_Changed_Post(hdlr, (UINT32 *)NULL);
            }
            if (Status == STATUS_LIVEVIEW && LCDHdlr) { // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window;
                memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));

                if (LCDWinHdlr[0]) {
                    UINT8 i;
                    for (i=0; i<VOUT_FOV_NUM; i++) {
                        window.Source = AMP_DISP_ENC;
                        window.SourceDesc.Enc.VinCh = VinChannel;
                        window.SourceDesc.Enc.BlendTable = FovVoutWindow[0][i].BlendTable;
                        window.SourceDesc.Enc.ViewZoneID = FovVoutWindow[0][i].MainWindowView;
                        window.SourceDesc.Enc.Rotate = FovVoutWindow[0][i].Rotate;
                        window.CropArea.Width = 0;
                        window.CropArea.Height = 0;
                        window.CropArea.X = 0;
                        window.CropArea.Y = 0;
                        window.TargetAreaOnPlane = FovVoutWindow[0][i].DisplayWin;
                        window.Layer = FovVoutWindow[0][i].LayerID;
                        AmpDisplay_SetWindowCfg(LCDWinHdlr[i], &window);
                        if (LCDLiveview) {
                            AmpDisplay_SetWindowActivateFlag(LCDWinHdlr[i], 1);
                        } else {
                            AmpDisplay_SetWindowActivateFlag(LCDWinHdlr[i], 0);
                        }
                    }
                }
                AmpDisplay_Update(LCDHdlr);
            }
            if (Status == STATUS_LIVEVIEW && TVHdlr) {
                AMP_DISP_WINDOW_CFG_s window;

                memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));

                if (TVWinHdlr[0]) {
                    UINT8 i;
                    for (i=0; i<VOUT_FOV_NUM; i++) {
                        window.Source = AMP_DISP_ENC;
                        window.SourceDesc.Enc.VinCh = VinChannel;
                        window.SourceDesc.Enc.BlendTable = FovVoutWindow[1][i].BlendTable;
                        window.SourceDesc.Enc.ViewZoneID = FovVoutWindow[1][i].MainWindowView;
                        window.SourceDesc.Enc.Rotate = FovVoutWindow[1][i].Rotate;
                        window.CropArea.Width = 0;
                        window.CropArea.Height = 0;
                        window.CropArea.X = 0;
                        window.CropArea.Y = 0;
                        window.TargetAreaOnPlane = FovVoutWindow[1][i].DisplayWin;
                        window.Layer = FovVoutWindow[1][i].LayerID;
                        AmpDisplay_SetWindowCfg(TVWinHdlr[i], &window);
                        if (TvLiveview) {
                            AmpDisplay_SetWindowActivateFlag(TVWinHdlr[i], 1);
                        } else {
                            AmpDisplay_SetWindowActivateFlag(TVWinHdlr[i], 0);
                        }
                    }
                }
                AmpDisplay_Update(TVHdlr);
            }
            break;
        case AMP_VIN_EVENT_MAINVIEW_CHANGED_PRIOR:
            {
                UINT8 i;
                //disable ImgSchdlr
                AmbaPrint("VinSWCB: AMP_VIN_EVENT_MAINVIEW_CHANGED_PRIOR");
                for(i=0; i<TOTAL_FOV_NUM; i++) {
                    if (ImgMultiChanSchdlr[i]) AmbaImgSchdlr_Enable(ImgMultiChanSchdlr[i], 0);
                }
            }
            break;
        case AMP_VIN_EVENT_MAINVIEW_CHANGED_POST:
            {
                UINT8 i;
                //enable ImgSchdlr
                AmbaPrint("VinSWCB: AMP_VIN_EVENT_MAINVIEW_CHANGED_POST");
                for(i=0; i<TOTAL_FOV_NUM; i++) {
                    if (ImgMultiChanSchdlr[i]) AmbaImgSchdlr_Enable(ImgMultiChanSchdlr[i], 1);
                }
            }
            break;
        default:
            AmbaPrint("VinSWCB: Unknown %X info: %x", event, info);
           break;
    }
    return 0;
}


/**
 * Generic VIN event callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
//static int xcnt = 0, ycnt=0; // Just to reduce console print
static int AmpUT_MotorVu360VinEventCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_VIN_EVENT_FRAME_READY:
           // if (xcnt % 30 == 0)
           //     AmbaPrint("VinEVCB: AMP_VIN_EVENT_FRAME_READY info: %X", info);
           // xcnt++;
            break;
        case AMP_VIN_EVENT_FRAME_DROPPED:
           // AmbaPrint("VinEVCB: AMP_VIN_EVENT_FRAME_DROPPED info: %X", info);
            break;
        default:
            AmbaPrint("VinEVCB: Unknown %X info: %x", event, info);
           break;
    }
    return 0;
}

/**
 * Generic VideoEnc codec callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_MotorVu360Callback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
            break;
        case AMP_ENC_EVENT_RAW_CAPTURE_DONE:
            break;
        case AMP_ENC_EVENT_BACK_TO_LIVEVIEW:
            break;
        case AMP_ENC_EVENT_BG_PROCESS_DONE:
            break;
        case AMP_ENC_EVENT_LIVEVIEW_RAW_READY:
            {
              //AMP_ENC_RAW_INFO_s *ptr = info;
              //AMP_ENC_RAW_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
              //AmbaPrint("EncCB: AMP_ENC_EVENT_LIVEVIEW_RAW_READY addr: %X p:%d %dx%d", inf.RawAddr, inf.RawPitch, inf.RawWidth, inf.RawHeight);
            }
            break;
        case AMP_ENC_EVENT_LIVEVIEW_DCHAN_YUV_READY:
            {
              //AMP_ENC_YUV_INFO_s *ptr = info;
              //AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
              //AmbaPrint("EncCB: AMP_VIDEOENC_MSG_LIVEVIEW_DCHAN_YUV_READY addr: %X p:%d %dx%d", inf.yAddr, inf.pitch, inf.Width, inf.Height);
            }
            break;

        case AMP_ENC_EVENT_LIVEVIEW_FCHAN_YUV_READY:
            {
          //   AMP_ENC_YUV_INFO_s *ptr = info;
          //   AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
          //   AmbaPrint("EncCB: AMP_VIDEOENC_MSG_LIVEVIEW_FCHAN_YUV_READY info: %X", info);
            }
            break;
        case AMP_ENC_EVENT_VCAP_YUV_READY:
            //AmbaPrint("AmpUT_MotorVu360: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("AmpUT_MotorVu360: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD !!!!!!!!!!");
            AmpUT_MotorVu360_EncStop();
            Status = STATUS_LIVEVIEW;
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("AmpUT_MotorVu360: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVERRUN !!!!!!!!!!");
            break;
        default:
            AmbaPrint("AmpUT_MotorVu360: Unknown %X info: %x", event, info);
            break;
    }
    return 0;
}

/**
 * Pipeline callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_MotorVu360PipeCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
            {
                AMP_ENC_STATE_CHANGED_INFO_s *inf = info;
                AmbaPrint("PipeCB: Pipe[%X] AMP_ENC_EVENT_STATE_CHANGED newState %X", hdlr, inf->newState);
            }
            break;
    }
    return 0;

}


/** Initialize OSD */
int AmpUT_MotorVu360_OSDInit(void)
{
    if (1) {
        AMP_OSD_INIT_CFG_s OsdInitCfg = {0};
        AMP_OSD_CFG_s OsdCfg;
        AMP_DISP_WINDOW_CFG_s OsdWindowCfg;
        AMP_OSD_BUFFER_CFG_s OsdLcdBufferCfg, OsdTvBufferCfg;
        AMP_DISP_WINDOW_CFG_s OsdWindow;

        AmpOsd_GetDefaultInitCfg(&OsdInitCfg);
        AmbaKAL_BytePoolAllocate(&G_MMPL, &OsdhdlrAddr, OsdInitCfg.MemoryPoolSize, 100);
        OsdInitCfg.MemoryPoolAddr = OsdhdlrAddr;
        AmpOsd_Init(&OsdInitCfg);

        /* Allocate osd buffer */
        AmbaKAL_BytePoolAllocate(&G_MMPL, (void *)&OsdLcdBuffer, 960 * 480 * 2, 100);
        AmbaKAL_BytePoolAllocate(&G_MMPL, (void *)&OsdTvBuffer, 960 * 540 * 2, 100);
        AmbaPrint("OsdLcdBuffer = 0x%X, OsdTvBuffer = 0x%X", OsdLcdBuffer, OsdTvBuffer);

        #if 0
        // Create osd handler
        AmpOsd_GetDefaultCfg(&OsdCfg);
        OsdCfg.HwScalerType = HW_OSD_RESCALER_ANY;
        OsdCfg.OsdBufRepeatField = 0;
        OsdCfg.GlobalBlend = 0xFF;
        AmpOsd_Create(&OsdCfg, &OsdLHdlr);
        #endif

        AmpOsd_GetDefaultCfg(&OsdCfg);
        OsdCfg.HwScalerType = HW_OSD_RESCALER_ANY;

#ifdef FULLHD_VOUT
        OsdCfg.OsdBufRepeatField = 0;
#else
        OsdCfg.OsdBufRepeatField = 1;
#endif
        OsdCfg.GlobalBlend = 0xFF;
        AmpOsd_Create(&OsdCfg, &OsdTHdlr);

        // create osd window
        AmpDisplay_GetDefaultWindowCfg(&OsdWindowCfg);

        OsdWindowCfg.Source = AMP_DISP_OSD;
        OsdWindowCfg.CropArea.X = 0;
        OsdWindowCfg.CropArea.X = 0;
        OsdWindowCfg.TargetAreaOnPlane.X = 0;
        OsdWindowCfg.TargetAreaOnPlane.Y = 0;
        #if 0
        OsdWindowCfg.CropArea.Width = 960;
        OsdWindowCfg.CropArea.Height = 480;
        OsdWindowCfg.TargetAreaOnPlane.Width = 960;
        OsdWindowCfg.TargetAreaOnPlane.Height = 480;
        AmpDisplay_CreateWindow(LCDHdlr, &OsdWindowCfg, &OsdWinLcdHdlr);
        #endif

#ifdef FULLHD_VOUT
        OsdWindowCfg.CropArea.Width = 960;
        OsdWindowCfg.CropArea.Height = 540;
        OsdWindowCfg.TargetAreaOnPlane.Width = 1920;
        OsdWindowCfg.TargetAreaOnPlane.Height = 1080;
#else
        OsdWindowCfg.CropArea.Width = 720;
        OsdWindowCfg.CropArea.Height = 240;
        OsdWindowCfg.TargetAreaOnPlane.Width = 720;
        OsdWindowCfg.TargetAreaOnPlane.Height = 240;
#endif
        AmpDisplay_CreateWindow(TVHdlr, &OsdWindowCfg, &OsdWinTvHdlr);

        #if 0
        // assgin buf to OSD handler
        OsdLcdBufferCfg.BufAddr =(unsigned char *) OsdLcdBuffer;
        OsdLcdBufferCfg.BufWidth = 960;
        OsdLcdBufferCfg.BufHeight = 480;
        OsdLcdBufferCfg.BufPitch = ALIGN_32(OsdLcdBufferCfg.BufWidth);
        OsdLcdBufferCfg.PixelFormat = AMP_OSD_16BIT_ARGB_4444;

        AmpOsd_SetBufferCfg(OsdLHdlr, &OsdLcdBufferCfg);
        #endif

        OsdTvBufferCfg.BufAddr = (unsigned char *)OsdTvBuffer;
#ifdef FULLHD_VOUT
        OsdTvBufferCfg.BufWidth = 960;
        OsdTvBufferCfg.BufHeight = 540;
#else
        OsdTvBufferCfg.BufWidth = 720;
        OsdTvBufferCfg.BufHeight = 240;
#endif
        OsdTvBufferCfg.BufPitch = ALIGN_32(OsdTvBufferCfg.BufWidth)*2; ///< Byte pitch
        OsdTvBufferCfg.PixelFormat = AMP_OSD_16BIT_ARGB_4444;

        AmpOsd_SetBufferCfg(OsdTHdlr, &OsdTvBufferCfg);


        // Reset OSD
        {
            int i, j;
            for (i=0; i<540; i++) {
                for(j=0; j<960; j++) {
                    OsdTvBuffer[j + i*960] = 0xf00f;
                }
            }
            #if 0
            for (i=0; i<480; i++) {
                for (j=0; j<960; j++) {
                    OsdLcdBuffer[j + i*960] = 0xf00f;
              }
            }
            #endif
        }

        // Paint TV OSD
#ifdef FULLHD_VOUT
       {
            int x, y;

            // Surrounding area: top
            for (y=0; y<(FHD_SUR_HBAR_HEIGHT>>1); y++) {
                for (x=0; x<(FHD_SUR_HBAR_WIDTH>>1); x++) {
                    OsdTvBuffer[x + y*960] = 0x0000;
                }
            }
            // Surrounding area: bottom
            for (y=(FHD_PLANE_TOTAL_HEIGHT - FHD_SUR_HBAR_HEIGHT)>>1; y < FHD_PLANE_TOTAL_HEIGHT >> 1; y++) {
                for(x=0; x<(FHD_SUR_HBAR_WIDTH>>1); x++) {
                    OsdTvBuffer[x + y*960] = 0x0000;
                }
            }
            // Surrounding area: left
            for (y=FHD_SUR_HBAR_HEIGHT>>1; y<(FHD_PLANE_TOTAL_HEIGHT - FHD_SUR_HBAR_HEIGHT)>>1; y++) {
                for (x=0; x<(FHD_SUR_VBAR_WIDTH>>1); x++) {
                    OsdTvBuffer[x + y*960] = 0x0000;
                }
            }
            // Surrounding area: top
            for (y=FHD_SUR_HBAR_HEIGHT>>1; y<(FHD_PLANE_TOTAL_HEIGHT - FHD_SUR_HBAR_HEIGHT)>>1; y++) {
                for (x=(FHD_SUR_HBAR_WIDTH - FHD_SUR_VBAR_WIDTH)>>1; x < FHD_SUR_HBAR_WIDTH>>1; x++) {
                    OsdTvBuffer[x + y*960] = 0x0000;
                }
            }

            // Focus area
            for (y=(FHD_PLANE_TOTAL_HEIGHT - FHD_FOCUS_HEIGHT)>>2; y < ((FHD_PLANE_TOTAL_HEIGHT - FHD_FOCUS_HEIGHT)>>2) + (FHD_FOCUS_HEIGHT>>1); y++) {
                for (x=(FHD_PLANE_TOTAL_WIDTH - (FHD_FOCUS_WIDTH<<1))>>1; x < FHD_PLANE_TOTAL_WIDTH>>1; x++) {
                    OsdTvBuffer[x + y*960] = 0x0000;
                }
            }
        }

#else
        {
            int x, y;

            // Surrounding area: top
            for (y=0; y<SD_SUR_HBAR_HEIGHT>>1; y++) {
                for (x=0; x<SD_SUR_HBAR_WIDTH; x++) {
                    OsdTvBuffer[x + y*736] = 0x0000;
                }
            }
            // Surrounding area: bottom
            for (y=(SD_PLANE_TOTAL_HEIGHT - SD_SUR_HBAR_HEIGHT)>>1; y < SD_PLANE_TOTAL_HEIGHT>>1; y++) {
                for (x=0; x<SD_SUR_HBAR_WIDTH; x++) {
                    OsdTvBuffer[x + y*736] = 0x0000;
                }
            }
            // Surrounding area: left
            for (y=SD_SUR_HBAR_HEIGHT>>1; y<(SD_PLANE_TOTAL_HEIGHT - SD_SUR_HBAR_HEIGHT)>>1; y++) {
                for (x=0; x<SD_SUR_VBAR_WIDTH; x++) {
                    OsdTvBuffer[x + y*736] = 0x0000;
                }
            }
            // Surrounding area: top
            for (y=SD_SUR_HBAR_HEIGHT>>1; y<(SD_PLANE_TOTAL_HEIGHT - SD_SUR_HBAR_HEIGHT)>>1; y++) {
                for (x=SD_SUR_HBAR_WIDTH- SD_SUR_VBAR_WIDTH; x < SD_SUR_HBAR_WIDTH; x++) {
                    OsdTvBuffer[x + y*736] = 0x0000;
                }
            }
            // Focus area
            for (y=(SD_PLANE_TOTAL_HEIGHT - SD_FOCUS_HEIGHT)>>2; y < ((SD_PLANE_TOTAL_HEIGHT - SD_FOCUS_HEIGHT)>>2) + (SD_FOCUS_HEIGHT>>1) - 2; y++) {
                for (x=SD_PLANE_TOTAL_WIDTH - SD_FOCUS_WIDTH*2; x < SD_PLANE_TOTAL_WIDTH; x++) {
                    OsdTvBuffer[x + y*736] = 0x0000;
                }
            }

        }
#endif
        AmbaCache_Clean(OsdTvBuffer, 960*540*2);
        AmpOsd_ActivateOsdWindow(OsdTHdlr, OsdWinTvHdlr);

    }

}

/**
 * Display window initialization
 */
static int AmpUT_MotorVu360_DisplayInit(void)
{
    AMP_DISP_INIT_CFG_s dispInitCfg;
    AMP_DISP_CFG_s dispCfg;
    AMP_DISP_WINDOW_CFG_s window;
    static UINT8 DispStarted = 0;

    /** Step 1: Display config & window config */
    // Display Init (Would move to mw init)
    if (dispModuleMemPool == NULL) {
        memset(&dispInitCfg, 0x0, sizeof(AMP_DISP_INIT_CFG_s));

        AmpDisplay_GetDefaultInitCfg(&dispInitCfg);
        AmbaKAL_BytePoolAllocate(&G_MMPL, &dispModuleMemPool, dispInitCfg.MemoryPoolSize + 32, 100);
        dispInitCfg.MemoryPoolAddr = (UINT8 *)ALIGN_32((UINT32)dispModuleMemPool);
        AmpDisplay_Init(&dispInitCfg);
    }

    if (LCDHdlr == NULL && LCDLiveview) {
        // Create DCHAN display handler
        AmpDisplay_GetDefaultCfg(&dispCfg);
        dispCfg.Device.Channel = AMP_DISP_CHANNEL_DCHAN;
        dispCfg.Device.DeviceId = AMP_DISP_LCD;
        dispCfg.Device.DeviceMode = 0xFFFF;
        dispCfg.ScreenRotate = 0;
        dispCfg.MaxNumWindow = 10;
        AmpDisplay_Create(&dispCfg, &LCDHdlr);
    }

    if (TVHdlr == NULL) {
        // Create FCHAN display handler
        AmpDisplay_GetDefaultCfg(&dispCfg);
        dispCfg.Device.Channel = AMP_DISP_CHANNEL_FCHAN;
#ifdef FULLHD_VOUT
        dispCfg.Device.DeviceId = AMP_DISP_HDMI;
        dispCfg.Device.DeviceMode = AMP_DISP_ID_1080P;
#else
        dispCfg.Device.DeviceId = AMP_DISP_CVBS;
        dispCfg.Device.DeviceMode = AMP_DISP_ID_480I;
#endif
        dispCfg.ScreenRotate = 0;
        dispCfg.MaxNumWindow = 10;
        AmpDisplay_Create(&dispCfg, &TVHdlr);
    }

    // Creat LCD Window
    if (LCDHdlr) {
        if (LCDWinHdlr[0] == NULL) {
            UINT8 i;
            memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));

            for (i=0; i<VOUT_FOV_NUM; i++) {
                window.Source = AMP_DISP_ENC;
                window.SourceDesc.Enc.VinCh = VinChannel;
                window.SourceDesc.Enc.BlendTable = FovVoutWindow[0][i].BlendTable;
                window.SourceDesc.Enc.ViewZoneID = FovVoutWindow[0][i].MainWindowView;
                window.SourceDesc.Enc.Rotate = FovVoutWindow[0][i].Rotate;
                window.CropArea.Width = 0;
                window.CropArea.Height = 0;
                window.CropArea.X = 0;
                window.CropArea.Y = 0;
                window.TargetAreaOnPlane = FovVoutWindow[0][i].DisplayWin;
                window.Layer = FovVoutWindow[0][i].LayerID;
                AmpDisplay_CreateWindow(LCDHdlr, &window, &LCDWinHdlr[i]);
            }
        }
    }

    // Creat TV Window
    if (1) {
        if (TVWinHdlr[0] == NULL) {
            UINT8 i;
            memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));

            for (i=0; i<VOUT_FOV_NUM; i++) {
                window.Source = AMP_DISP_ENC;
                window.SourceDesc.Enc.VinCh = VinChannel;
                window.SourceDesc.Enc.BlendTable = FovVoutWindow[1][i].BlendTable;
                window.SourceDesc.Enc.ViewZoneID = FovVoutWindow[1][i].MainWindowView;
                window.SourceDesc.Enc.Rotate = FovVoutWindow[1][i].Rotate;
                window.CropArea.Width = 0;
                window.CropArea.Height = 0;
                window.CropArea.X = 0;
                window.CropArea.Y = 0;
                window.TargetAreaOnPlane = FovVoutWindow[1][i].DisplayWin;
                window.Layer = FovVoutWindow[1][i].LayerID;
                AmpDisplay_CreateWindow(TVHdlr, &window, &TVWinHdlr[i]);
            }
        }
    }

    if (DispStarted == 0) {
        /** Step 2: Setup device */
        // Setup LCD & TV
        if (LCDHdlr) {
            if (LCDLiveview) {
                AmpDisplay_Start(LCDHdlr);
            } else {
                AmpDisplay_Stop(LCDHdlr);
            }
        }
        if (TvLiveview) {
            AmpDisplay_Start(TVHdlr);
        } else {
            AmpDisplay_Stop(TVHdlr);
        }
/*
        // Active Window 7002 cmd
        AmpDisplay_ActivateVideoWindow(encLcdWinHdlr);

        if (TvLiveview) {
            AmpDisplay_ActivateVideoWindow(encTvWinHdlr);
        } else {
            AmpDisplay_DeactivateVideoWindow(encTvWinHdlr);
        }
*/
    }

    // Active Window 7002 cmd
    if (LCDHdlr) {
        if (LCDLiveview) {
            UINT8 i;
            for (i=0; i<VOUT_FOV_NUM; i++) {
                AmpDisplay_SetWindowActivateFlag(LCDWinHdlr[i], 1);
            }
            AmpDisplay_Update(LCDHdlr);
        } else {
            UINT8 i;
            for (i=0; i<VOUT_FOV_NUM; i++) {
                AmpDisplay_SetWindowActivateFlag(LCDWinHdlr[i], 0);
            }
            AmpDisplay_Update(LCDHdlr);
        }
    }
    if (TvLiveview) {
        UINT8 i;
        for (i=0; i<VOUT_FOV_NUM; i++) {
            AmpDisplay_SetWindowActivateFlag(TVWinHdlr[i], 1);
        }

        AmpDisplay_Update(TVHdlr);
    } else {
        UINT8 i;
        for (i=0; i<VOUT_FOV_NUM; i++) {
            AmpDisplay_SetWindowActivateFlag(TVWinHdlr[i], 0);
        }

        AmpDisplay_Update(TVHdlr);
    }
    DispStarted = 1;
    // Init OSD
    AmpUT_MotorVu360_OSDInit();
    return 0;
}

/**
 *  Unit Test Initialization
 */
int AmpUT_MotorVu360_Init(int sensorID)
{
    int Er;

    AmpCFS_GetFileParam(&cfsParam); // Get initial
    //imgproc initialization
    // Register sensor driver
    if (sensorID >= 0) {
        VinChannel.Bits.VinID = 0;
        VinChannel.Bits.SensorID = 0x1;

        if (sensorID == 0) {
            AmbaPrint("Hook IMX122+B5 Sensor");
            VinChannel.Bits.SensorID = 0xf; // Currently must config all sensors

            AmbaSensor_Hook(VinChannel, &AmbaSensor_B5_IMX122Obj);

            VideoEncMgt = VideoEncMgt122B5;
            //Load iq parameters
            {
                extern int AmbaIQParamImx117_A9_Register(void);
                extern int App_Image_Init(UINT32 ChCount, int sensorID);
                AmbaIQParamImx117_A9_Register();
                App_Image_Init(1, sensorID);
            }
        } else {
            AmbaPrint("Unsupported sensor");
            K_ASSERT(0);
        }
        AmbaSensor_Init(VinChannel);
    }

    // Create semaphores for muxers
    if (AmbaKAL_SemCreate(&MotorVuPriSem, 0) != OK) {
        AmbaPrint("VideoEnc UnitTest: Semaphore creation failed");
    }
    if (AmbaKAL_SemCreate(&MotorVuSecSem, 0) != OK) {
        AmbaPrint("VideoEnc UnitTest: Semaphore creation failed");
    }

    // Prepare stacks for muxer tasks
    Er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&MotorVuPriMuxStack, 6400 + 32, 100);
    if (Er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    MotorVuPriMuxStack = (UINT8 *)ALIGN_32((UINT32)MotorVuPriMuxStack);
    Er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&MotorVuSecMuxStack, 6400 + 32, 100);
    if (Er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    MotorVuSecMuxStack = (UINT8 *)ALIGN_32((UINT32)MotorVuSecMuxStack);

    // Create muxer tasks
    if (AmbaKAL_TaskCreate(&MotorVuPriMuxTask, "Video Encoder UnitTest Primary Muxing Task", 11, \
         AmpUT_MotorVu360_PriMuxTask, 0x0, MotorVuPriMuxStack, 6400, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("VideoEnc UnitTest: Muxer task creation failed");
    }
    if (AmbaKAL_TaskCreate(&MotorVuSecMuxTask, "Video Encoder UnitTest Secondary stream Muxing Task", 11, \
         AmpUT_MotorVu360_SecMuxTask, 0x0, MotorVuSecMuxStack, 6400, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("VideoEnc UnitTest: Muxer task creation failed");
    }

    // Initialize VIN module
    {
        AMP_VIN_INIT_CFG_s vinInitCfg;

        AmpVin_GetInitDefaultCfg(&vinInitCfg);

        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&VinWorkBuffer, vinInitCfg.MemoryPoolSize + 32, 100);
        if (Er != OK) {
            AmbaPrint("Out of memory for vin!!");
        }
        VinWorkBuffer = (UINT8 *)ALIGN_32((UINT32)VinWorkBuffer);

        vinInitCfg.MemoryPoolAddr = VinWorkBuffer;
        AmpVin_Init(&vinInitCfg);
    }

    // Initialize VIDEOENC module
    {
        AMP_VIDEOENC_INIT_CFG_s encInitCfg;

        AmpVideoEnc_GetInitDefaultCfg(&encInitCfg);

        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&VEncWorkBuffer, encInitCfg.MemoryPoolSize + 32, 100);
        if (Er != OK) {
            AmbaPrint("Out of memory for enc!!");
        }
        VEncWorkBuffer = (UINT8 *)ALIGN_32((UINT32)VEncWorkBuffer);
        encInitCfg.MemoryPoolAddr = VEncWorkBuffer;
        AmpVideoEnc_Init(&encInitCfg);
    }

    // Initialize Image scheduler module
    {
        AMBA_IMG_SCHDLR_INIT_CFG_s ISInitCfg;
        UINT32 ISPoolSize = 0;
        UINT32 mainViewNum;
        AMBA_DSP_IMG_PIPE_e Pipe = AMBA_DSP_IMG_PIPE_VIDEO;

        AmbaImgSchdlr_GetInitDefaultCfg(&ISInitCfg);
            // For runtime enable/disable mainViewID, allocate MAX possible memory
        mainViewNum = TOTAL_FOV_NUM;

        ISInitCfg.MainViewNum = mainViewNum;
        AmbaImgSchdlr_QueryMemsize(mainViewNum, &ISPoolSize);
        ISInitCfg.MemoryPoolSize = ISPoolSize;

        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&ImgSchdlrWorkBuffer, ISInitCfg.MemoryPoolSize + 32, 100);
        if (Er != OK) {
            AmbaPrint("Out of memory for imgschdlr!!");
        }
        ImgSchdlrWorkBuffer = (UINT8 *)ALIGN_32((UINT32)ImgSchdlrWorkBuffer);

        ISInitCfg.IsoCfgNum = AmpResource_GetIKIsoConfigNumber(Pipe);
        ISInitCfg.MemoryPoolAddr = ImgSchdlrWorkBuffer;
        Er = AmbaImgSchdlr_Init(&ISInitCfg);
        if (Er != AMP_OK) {
            AmbaPrint("AmbaImgSchdlr_Init Fail!");
        }
    }

    // Allocate bitstream buffers
    {
        extern UINT8 *DspWorkAreaResvLimit;

        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&H264BitsBuf, BITSFIFO_SIZE + 32, 100);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }
        H264BitsBuf = (UINT8 *)ALIGN_32((UINT32)H264BitsBuf);

        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&H264DescBuf, DESC_SIZE + 32, 100);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }
        H264DescBuf = (UINT8 *)ALIGN_32((UINT32)H264DescBuf);

        // This is an example how to use DSP working memory when APP knows these memory area is not used.
        // We steal 15MB here
//        MjpgDescBuf = DspWorkAreaResvLimit + 1 - 1*1024*1024;
//        MjpgBitsBuf = MjpgDescBuf - BITSFIFO_SIZE;

    }


    // Generate quantization tables for MJPEG
    if (1) {
        int i, temp, quality = 20, scale;

        if (quality <= 0) {
            scale = 5000;
        } else if (quality >= 100) {
            scale = 0;
        } else if (quality < 50) {
            scale = (5000/quality);
        } else {
            scale = (200-quality*2);
        }

        for (i=0; i<128; i++) {
            temp = ((long) MjpegQuantMatrix[i] * scale + 50L) / 100L;
            /* limit the values to the valid range */
            if (temp <= 0L) temp = 1L;
            if (temp > 255L) temp = 255L; /* max quantizer needed for baseline */
            MjpegQuantMatrix[i] = temp;
        }

    }

    return 0;
}

/**
 * Config CFA window
 *
 * @param [in,out] pCfaInfo Callback will pass you input and main dimension, you can decide cfa dimension
 *
 */

int AmpUT_MotorVu360_CalculateCFA(AMBA_DSP_IMG_WARP_CALC_CFA_INFO_s *pCfaInfo)
{
    // Since all FOV main use the same dimension, we use [0]'s W/H.
    pCfaInfo->pOutCfaWinDim->Width = FovMainWindow[0].MainCapWin.Width;
    pCfaInfo->pOutCfaWinDim->Height = FovMainWindow[0].MainCapWin.Height;

    return 0;
}


/**
 * Liveview start
 *
 * @param [in]
 *
 */
int AmpUT_MotorVu360_LiveviewStart(UINT32 modeIdx)
{
    EncModeIdx = modeIdx;


//#define KEEP_ALL_INSTANCES    // When defined, LiveviewStop won't delete all instances, so the next LiveviewStart is equal to change resolution.
#ifdef KEEP_ALL_INSTANCES
    if (MotorVuVinA) {
        AmpUT_MotorVu360_ChangeResolution(modeIdx);
        AmpEnc_StartLiveview(VideoEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
        return 0;
    }
#endif

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" AmbaUnitTest: Liveview at %s",VideoEncMgt[EncModeIdx].Name);
    AmbaPrint(" =========================================================");

    // Create Vin instance
    if (MotorVuVinA == 0) {
        AMP_VIN_HDLR_CFG_s VinCfg;
        AMBA_SENSOR_MODE_INFO_s VinInfo;
        AMP_VIN_LAYOUT_CFG_s Layout[2]; // Dualstream from same vin/vcapwindow
        AMBA_SENSOR_MODE_ID_u Mode = {0};

        memset(&vinCfg, 0x0, sizeof(AMP_VIN_HDLR_CFG_s));
        memset(&Layout, 0x0, sizeof(AMP_VIN_LAYOUT_CFG_s)*2);

        Mode.Data = VideoEncMgt[EncModeIdx].SensorMode;
        AmbaSensor_GetModeInfo(VinChannel, Mode, &VinInfo);

        AmpVin_GetDefaultCfg(&VinCfg);

        VinCfg.Channel = VinChannel;
        VinCfg.Mode = Mode;
        VinCfg.LayoutNumber = 2;
        if (1) {
            UINT16 MaxCapWidth = 0, MaxCapHeight = 0;
            UINT8 i;
            for (i=0; i<TOTAL_FOV_NUM; i++) {
                if (FovMainWindow[i].Enable) {
                #if 0
                    if (MaxCapWidth < FovMainWindow[i].MainCapWin.X + FovMainWindow[i].MainCapWin.Width)
                        MaxCapWidth = FovMainWindow[i].MainCapWin.X + FovMainWindow[i].MainCapWin.Width;
                    if (MaxCapHeight < FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].MainCapWin.Height)
                        MaxCapHeight = FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].MainCapWin.Height;
                #else
                    if (MaxCapWidth < FovMainWindow[i].MainCapWin.X + FovMainWindow[i].SensorCapWin->Width)
                        MaxCapWidth = FovMainWindow[i].MainCapWin.X + FovMainWindow[i].SensorCapWin->Width;
                    if (MaxCapHeight < FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].SensorCapWin->Height)
                        MaxCapHeight = FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].SensorCapWin->Height;
                #endif
                }
            }
            VinCfg.HwCaptureWindow.Width = MaxCapWidth;
            VinCfg.HwCaptureWindow.Height = MaxCapHeight;
            VinCfg.HwCaptureWindow.X = VinCfg.HwCaptureWindow.Y = 0;
            Layout[0].Width = FovMainWindow[0].MainWidth;
            Layout[0].Height = FovMainWindow[0].MainHeight;
        }
        Layout[0].EnableSourceArea = 0; // Get all capture window to main
        Layout[0].DzoomFactorX = 1<<16; // 16.16 format
        Layout[0].DzoomFactorY = 1<<16;
        Layout[0].DzoomOffsetX = 0;
        Layout[0].DzoomOffsetY = 0;
        Layout[1].Width = SEC_STREAM_WIDTH;
        Layout[1].Height = SEC_STREAM_HEIGHT;
        Layout[1].EnableSourceArea = 0; // Get all capture window to main
        Layout[1].DzoomFactorX = 1<<16;
        Layout[1].DzoomFactorY = 1<<16;
        Layout[1].DzoomOffsetX = 0;
        Layout[1].DzoomOffsetY = 0;
        VinCfg.Layout = Layout;
        Layout[0].cfaOutputCallback = AmpUT_MotorVu360_CalculateCFA;

        VinCfg.cbEvent = AmpUT_MotorVu360VinEventCallback;
        VinCfg.cbSwitch= AmpUT_MotorVu360VinSwitchCallback;

        AmpVin_Create(&VinCfg, &MotorVuVinA);
    }

    // Remember frame/field rate for muxers storing frame rate info
    EncFrameRate = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;


    // Create ImgSchdlr instance
    {
        AMBA_IMG_SCHDLR_CFG_s ISCfg;
        UINT8 i, j;
        AmbaImgSchdlr_GetDefaultCfg(&ISCfg);

        for(i=0; i<TOTAL_FOV_NUM; i++) {
            if (FovMainWindow[i].Enable) {
                UINT8 sensorBitmap = 0;
                for(j=0; j<FovMainWindow[i].ChannelNumber; j++) {
                    sensorBitmap |= FovMainWindow[i].ChannelID[j].Bits.SensorID;
                }
                ISCfg.MainViewID = i;
                ISCfg.Channel.Bits.VinID = VinChannel.Bits.VinID;
                ISCfg.Channel.Bits.SensorID = sensorBitmap;
                ISCfg.Vin = MotorVuVinA;
                ISCfg.cbEvent = AmpUT_MotorVu360ImgSchdlrCallback;
                AmbaImgSchdlr_Create(&ISCfg, &ImgMultiChanSchdlr[i]);  // One MainViewID (not vin) need one scheduler.
            }
        }
    }

    // Create video encoder instances
    if (MotorVuPri == 0 && MotorVuSec == 0) {
        AMP_VIDEOENC_HDLR_CFG_s EncCfg;
        AMP_VIDEOENC_LAYER_DESC_s MultChanEncLayer[TOTAL_FOV_NUM];
#if 0
        AMP_VIDEOENC_H264_CFG_s *H264Cfg;
        AMP_VIDEOENC_MJPEG_CFG_s *MjpegCfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s HeaderInfo;
#endif
        memset(&EncCfg, 0x0, sizeof(AMP_VIDEOENC_HDLR_CFG_s));
        memset(MultChanEncLayer, 0x0, sizeof(AMP_VIDEOENC_LAYER_DESC_s)*TOTAL_FOV_NUM);
        EncCfg.MainLayout.Layer = MultChanEncLayer;
        AmpVideoEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_MotorVu360Callback;

        if (1) {
            UINT8 *tempBuf = NULL;
            UINT8 i, j;
            // Assign all main windows
            EncCfg.EnableMultiView = 1;
            EncCfg.MainTimeScale = VideoEncMgt[EncModeIdx].TimeScale;
            EncCfg.MainTickPerPicture = VideoEncMgt[EncModeIdx].TickPerPicture;
            EncCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
            EncCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
            EncCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
            {
                UINT16 MaxCapWidth = 0, MaxCapHeight = 0;
                EncCfg.MainLayout.Width = 0;
                EncCfg.MainLayout.Height = 0;
                EncCfg.MainLayout.LayerNumber = 0;
                for (i=0; i<TOTAL_FOV_NUM; i++) {
                    if (FovMainWindow[i].Enable) {
                        MultChanEncLayer[i].SourceType = AMP_ENC_SOURCE_VIN;
                        MultChanEncLayer[i].Source = MotorVuVinA; //TBD, find VinID
                        MultChanEncLayer[i].SourceLayoutId = FovMainWindow[i].MainViewID;
                        MultChanEncLayer[i].EnableSourceArea = 1;
                        MultChanEncLayer[i].EnableTargetArea = 1;
                        MultChanEncLayer[i].SourceArea = FovMainWindow[i].MainCapWin;
                        MultChanEncLayer[i].TargetArea.Width = FovMainWindow[i].MainWidth;
                        MultChanEncLayer[i].TargetArea.Height = FovMainWindow[i].MainHeight;
                        if (MaxCapWidth < FovMainWindow[i].MainCapWin.X + FovMainWindow[i].MainCapWin.Width) {
                            MaxCapWidth = FovMainWindow[i].MainCapWin.X + FovMainWindow[i].MainCapWin.Width;
                        }
                        if (MaxCapHeight < FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].MainCapWin.Height) {
                            MaxCapHeight = FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].MainCapWin.Height;
                        }
                        EncCfg.MainLayout.LayerNumber++;
                        for (j=0; j<ENCODE_FOV_NUM; j++) {
                            if (FovEncodeWindow[0][j].Enable && FovEncodeWindow[0][j].MainWindowView == FovMainWindow[i].MainViewID) {
                                memcpy(&MultChanEncLayer[i].MultiChanEncSrcArea, &FovEncodeWindow[0][j].SrcWin, sizeof(AMP_AREA_s));
                                memcpy(&MultChanEncLayer[i].MultiChanEncDestArea, &FovEncodeWindow[0][j].DestWin, sizeof(AMP_AREA_s));
                                MultChanEncLayer[i].SourceRotate = FovEncodeWindow[0][j].Rotate;
                                MultChanEncLayer[i].MultiChanEncBlendTable = FovEncodeWindow[0][j].BlendTable;
                            }
                        }
                    }
                }
                EncCfg.MainLayout.Width = MaxCapWidth;
                EncCfg.MainLayout.Height = MaxCapHeight;
            }

            //Fill blend table
            for (i = 0; i < 2; i++) {
                for (j=0; j<VOUT_FOV_NUM; j++) {
                    if (FovVoutWindow[i][j].Enable) {
                        UINT32 size = 0;
                        //Get Buffer, only need blend table in Left/Right side of TV
                        if (FovVoutWindow[i][j].BlendTable.Buf == NULL && (i==1) && (j==2 || j==3)) {
                            int er;
                            size = FovVoutWindow[i][j].BlendTable.Pitch*FovVoutWindow[i][j].BlendTable.Height;
                            er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&tempBuf, size, 100);
                            if (er == NG) {
                                AmbaPrint("[MultiChan] blend Buff alloc fail (%d %d) (%d*%d=%d)", i, j, \
                                    FovVoutWindow[i][j].BlendTable.Pitch, FovVoutWindow[i][j].BlendTable.Height, size);
                            } else {
                                AmbaPrint("[MultiChan] blend Buff (%d %d) 0x%X (%d*%d=%d)", i, j, tempBuf, \
                                    FovVoutWindow[i][j].BlendTable.Pitch, FovVoutWindow[i][j].BlendTable.Height, size);
                                FovVoutWindow[i][j].BlendTable.Buf = tempBuf;
                            }
                        }
                        //Fill table
                    #ifdef MULTICHAN_BLEND_TABLE_FROM_FILE
                        if (FovVoutWindow[i][j].BlendTable.Buf) {
                            char Fn[32];

                            if (i == 0 && j == 2) {
                                //TBD
                            } else if (i == 0 && j == 3) {
                                //TBD
                            } else if (i == 1 && j == 2) {
                                #ifdef FULLHD_VOUT
                                sprintf(Fn,"%s:\\%s_288_1080_zero.bin", DefaultSlot, "left");
                                #else
                                sprintf(Fn,"%s:\\%s_128_480.bin", DefaultSlot, "left");
                                #endif
                            } else if (i == 1 && j == 3) {
                                #ifdef FULLHD_VOUT
                                sprintf(Fn,"%s:\\%s_288_1080_zero.bin", DefaultSlot, "right");
                                #else
                                sprintf(Fn,"%s:\\%s_128_480.bin", DefaultSlot, "right");
                                #endif
                            }

                            if (UT_MotorVu360fsize((void *)Fn) > size) {
                                AmbaPrint("[MultiChan]blend_table binary large than allocate Buf");
                            } else {
                                char mdASCII[3] = {'r','b','\0'};
                                BlendTable = UT_MotorVu360fopen((const char *)Fn, (const char *)mdASCII, 0);
                                AmbaPrint("[MultiChan]Read blend_table 0x%X %d from %s Start!", \
                                    FovVoutWindow[i][j].BlendTable.Buf, 288*1080, Fn);
                                UT_MotorVu360fread((void *)FovVoutWindow[i][j].BlendTable.Buf, 1, 288*1080, (void *)BlendTable);
                                UT_MotorVu360fclose((void *)BlendTable);
                                AmbaCache_Clean((void *)FovVoutWindow[i][j].BlendTable.Buf, 288*1080);
                            }
                        }
                    #endif
                    }
                }
            }

            EncCfg.EventDataReadySkipNum = 0;  // File data ready every frame
            EncCfg.StreamId = AMP_VIDEOENC_STREAM_PRIMARY;
        }

        {
            UINT8 *dspWorkAddr;
            UINT32 dspWorkSize;
            AmpUT_MotorVu360_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
            EncCfg.DspWorkBufAddr = dspWorkAddr;
            EncCfg.DspWorkBufSize = dspWorkSize;
        }


       // Create primary stream handler
        AmpVideoEnc_Create(&EncCfg, &MotorVuPri); // Don't have to worry about h.264 spec settings when liveview
        // Assign Secondary main layout
        EncCfg.MainLayout.Width = SEC_STREAM_WIDTH;
        EncCfg.MainLayout.Height = SEC_STREAM_HEIGHT;
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.MainTickPerPicture = SEC_STREAM_TICK;
        EncCfg.MainTimeScale = SEC_STREAM_TIMESCALE;
        EncCfg.EventDataReadySkipNum = 0;  // File data ready every frame
        EncCfg.StreamId = AMP_VIDEOENC_STREAM_SECONDARY;
        EncCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;

        // Create secondary stream handler
        AmpVideoEnc_Create(&EncCfg, &MotorVuSec); // Don't have to worry about h.264 spec settings when liveview
    }

    if (strcmp(VideoEncMgt[EncModeIdx].Name, "2560x1440P60 16:9\0") == 0) {
        AmbaPrint("Turn OFF Tv");
        TvLiveview = 0;
    } else {
        AmbaPrint("Turn %s Tv",TvLiveview?"ON":"OFF");
    }

    //MultiChan dualVout will have preformance issue
    if (0) {
        AmbaPrint("Turn OFF LCD");
        LCDLiveview = 0;
    } else {
        AmbaPrint("Turn ON LCD");
        LCDLiveview = 1;
    }

    // Setup display windows
    AmpUT_MotorVu360_DisplayInit();

    // Register pipeline
    {
        AMP_ENC_PIPE_CFG_s pipeCfg;
        // Register pipeline
        AmpEnc_GetDefaultCfg(&pipeCfg);
        //pipeCfg.cbEvent
        pipeCfg.encoder[0] = MotorVuPri;
        pipeCfg.encoder[1] = MotorVuSec;
        if (EncDualStream) {
            pipeCfg.numEncoder = 2;
        } else {
            pipeCfg.numEncoder = 1;
        }
        pipeCfg.cbEvent = AmpUT_MotorVu360PipeCallback;
        pipeCfg.type = AMP_ENC_AV_PIPE;
        AmpEnc_Create(&pipeCfg, &VideoEncPipe);

        AmpEnc_Add(VideoEncPipe);
    }

    AmpEnc_StartLiveview(VideoEncPipe, AMP_ENC_FUNC_FLAG_WAIT);

    return 0;
}

int AmpUT_MotorVu360_EncodeStart()
{
    AMP_FIFO_CFG_s fifoDefCfg;

    if (VirtualPriFifoHdlr == NULL) {
        // create a virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = MotorVuPri;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = 1024;
        fifoDefCfg.cbEvent = AmpUT_MotorVu360_FifoCB;
        AmpFifo_Create(&fifoDefCfg, &VirtualPriFifoHdlr);
    }
    if (VirtualSecFifoHdlr == NULL) {
        // create a virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = MotorVuSec;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = 1024;
        fifoDefCfg.cbEvent = AmpUT_MotorVu360_FifoCB;
        AmpFifo_Create(&fifoDefCfg, &VirtualSecFifoHdlr);
    }

    //
    // Note: For A9, Bitstream-specific configs can be assigned here (before encode start), or when codec instance creation.
    //       But for A7L/A12 family, it is better to assign them when codec instance creation.
    //
    {
        AMP_VIDEOENC_H264_CFG_s *H264Cfg;
        AMP_VIDEOENC_MJPEG_CFG_s *MjpegCfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s HeaderInfo;
        AMP_VIDEOENC_BITSTREAM_CFG_s BitsCfg;

        memset(&BitsCfg, 0x0, sizeof(AMP_VIDEOENC_BITSTREAM_CFG_s));
        BitsCfg.Rotation = AMP_ROTATE_0;
        BitsCfg.TimeLapse = 0;
        BitsCfg.VideoThumbnail = 0;

        // Assign bitstream-specific configs
        if (EncPriSpecH264) {
            BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
            H264Cfg = &BitsCfg.Spec.H264Cfg;
            H264Cfg->GopM = VideoEncMgt[EncModeIdx].GopM;
            H264Cfg->GopN = GOP_N;
            H264Cfg->GopIDR = GOP_IDR;
            H264Cfg->GopHierarchical = 0;
            H264Cfg->QPControl.QpMinI = 14;
            H264Cfg->QPControl.QpMaxI = 51;
            H264Cfg->QPControl.QpMinP = 17;
            H264Cfg->QPControl.QpMaxP = 51;
            H264Cfg->QPControl.QpMinB = 21;
            H264Cfg->QPControl.QpMaxB = 51;
            H264Cfg->Cabac = 1;
            H264Cfg->QualityControl.LoopFilterEnable = 1;
            H264Cfg->QualityControl.LoopFilterAlpha = 0;
            H264Cfg->QualityControl.LoopFilterBeta = 0;
            H264Cfg->StartFromBFrame = (H264Cfg->GopM > 1);
            H264Cfg->Interlace = 0;
            H264Cfg->AuDelimiterType = 1;
            H264Cfg->QualityLevel = 0x94; // Suggested value: 1080P: 0x94, 1008i: 0x9B, 720P: 0x9A
            H264Cfg->StopMethod = AMP_VIDEOENC_STOP_NEXT_IP;
            HeaderInfo.GopM = H264Cfg->GopM;
            {
                UINT16 MaxEncWidth = 0, MaxEncHeight = 0;
                UINT8 i;
                for (i=0; i<ENCODE_FOV_NUM; i++) {
                    if (FovEncodeWindow[0][i].Enable) {
                        if (MaxEncWidth < FovEncodeWindow[0][i].DestWin.X + FovEncodeWindow[0][i].DestWin.Width)
                            MaxEncWidth = FovEncodeWindow[0][i].DestWin.X + FovEncodeWindow[0][i].DestWin.Width;
                        if (MaxEncHeight < FovEncodeWindow[0][i].DestWin.Y + FovEncodeWindow[0][i].DestWin.Height)
                            MaxEncHeight = FovEncodeWindow[0][i].DestWin.Y + FovEncodeWindow[0][i].DestWin.Height;
                    }
                }
                HeaderInfo.Width = MaxEncWidth;
                HeaderInfo.Height = MaxEncHeight;
            }
            HeaderInfo.Interlace = 0;
            HeaderInfo.Rotation = AMP_ROTATE_0;

            // Use default SPS/VUI
            AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
            H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

            // Bitrate control
            H264Cfg->BitRateControl.BrcMode = VideoEncMgt[EncModeIdx].BrcMode;
            H264Cfg->BitRateControl.AverageBitrate = (UINT32)(VideoEncMgt[EncModeIdx].AverageBitRate * 1E6);
            if (VideoEncMgt[EncModeIdx].BrcMode == VIDEOENC_SMART_VBR) {
                H264Cfg->BitRateControl.MaxBitrate = (UINT32)(VideoEncMgt[EncModeIdx].MaxBitRate * 1E6);
                H264Cfg->BitRateControl.MinBitrate = (UINT32)(VideoEncMgt[EncModeIdx].MinBitRate * 1E6);
            }
            H264Cfg->QualityControl.IBeatMode = 0;

        } else {
            BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_MJPEG;
            MjpegCfg = &BitsCfg.Spec.MjpgCfg;

            MjpegCfg->FrameRateDivisionFactor = 1;
            MjpegCfg->QuantMatrixAddr = MjpegQuantMatrix;

        }
        AmpVideoEnc_SetBitstreamConfig(MotorVuPri, &BitsCfg);

        if (MotorVuSec) {
            if (EncSecSpecH264) {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
                H264Cfg = &BitsCfg.Spec.H264Cfg;
                H264Cfg->GopM = 1;
                H264Cfg->GopN = 8;
                H264Cfg->GopIDR = 32;
                H264Cfg->GopHierarchical = 0;
                H264Cfg->QPControl.QpMinI = 14;
                H264Cfg->QPControl.QpMaxI = 51;
                H264Cfg->QPControl.QpMinP = 17;
                H264Cfg->QPControl.QpMaxP = 51;
                H264Cfg->QPControl.QpMinB = 21;
                H264Cfg->QPControl.QpMaxB = 51;
                H264Cfg->Cabac = 1;
                H264Cfg->QualityControl.LoopFilterEnable = 1;
                H264Cfg->QualityControl.LoopFilterAlpha = 0;
                H264Cfg->QualityControl.LoopFilterBeta = 0;
                H264Cfg->StartFromBFrame = 0;
                H264Cfg->Interlace = 0;
                H264Cfg->AuDelimiterType = 1;
                H264Cfg->QualityLevel = 0x9A; // Suggested value: 1080P: 0x94, 1008i: 0x9B, 720P: 0x9A
                H264Cfg->StopMethod = AMP_VIDEOENC_STOP_NEXT_IP;

                HeaderInfo.GopM = H264Cfg->GopM;
                HeaderInfo.Width = SEC_STREAM_WIDTH;
                HeaderInfo.Height = SEC_STREAM_HEIGHT;
                HeaderInfo.Interlace = 0;
                HeaderInfo.Rotation = AMP_ROTATE_0;

                // Get default SPS/VUI
                AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
                H264Cfg->SPS.level_idc = 42;
                H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

                // Bitrate control
                H264Cfg->BitRateControl.BrcMode = VIDEOENC_CBR;
                H264Cfg->BitRateControl.AverageBitrate = (UINT32)(1E6);
                H264Cfg->QualityControl.IBeatMode = 0;
            } else {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_MJPEG;
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;

                MjpegCfg->FrameRateDivisionFactor = 1;
                MjpegCfg->QuantMatrixAddr = MjpegQuantMatrix;
            }
            AmpVideoEnc_SetBitstreamConfig(MotorVuSec, &BitsCfg);

        }

        //
        // Setup bitstream buffer.
        //
        // Rule: H.264 and MJPEG can't use the same bitstream/descriptor buffer. Same Spec uses the same buffer. No matter it is primary or secondary
        // Note: Since buffer allocation depends on the above rule, it is better to assign bitstream buffer before encode start.
        //       Otherwise you have to know what you are going to encode when codec instance creation
        {
            AMP_ENC_BITSBUFFER_CFG_s  BitsBufCfg = {0};

            if (EncPriSpecH264) {
                BitsBufCfg.BitsBufAddr = H264BitsBuf;
                BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
                BitsBufCfg.DescBufAddr = H264DescBuf;
                BitsBufCfg.DescBufSize = DESC_SIZE;
                BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
            } else {
                BitsBufCfg.BitsBufAddr = MjpgBitsBuf;
                BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
                BitsBufCfg.DescBufAddr = MjpgDescBuf;
                BitsBufCfg.DescBufSize = DESC_SIZE;
                BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
            }
            AmpVideoEnc_SetBitstreamBuffer(MotorVuPri, &BitsBufCfg);

            if (MotorVuSec) {
                if (EncSecSpecH264) {
                    BitsBufCfg.BitsBufAddr = H264BitsBuf;
                    BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
                    BitsBufCfg.DescBufAddr = H264DescBuf;
                    BitsBufCfg.DescBufSize = DESC_SIZE;
                    BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
                } else {
                    BitsBufCfg.BitsBufAddr = MjpgBitsBuf;
                    BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
                    BitsBufCfg.DescBufAddr = MjpgDescBuf;
                    BitsBufCfg.DescBufSize = DESC_SIZE;
                    BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
                }
                AmpVideoEnc_SetBitstreamBuffer(MotorVuSec, &BitsBufCfg);
            }
            AmbaPrint("H.264 Bits 0x%x size %d Desc 0x%x size %d", H264BitsBuf, BITSFIFO_SIZE, H264DescBuf, DESC_SIZE);
            AmbaPrint("MJPEG Bits 0x%x size %d Desc 0x%x size %d", MjpgBitsBuf, BITSFIFO_SIZE, MjpgDescBuf, DESC_SIZE);

        }
    }

    AmpEnc_StartRecord(VideoEncPipe, 0);

    return 0;
}
int AmpUT_MotorVu360_EncStop()
{
    AmpEnc_StopRecord(VideoEncPipe, 0);
    return 0;
}

int AmpUT_MotorVu360_LiveviewStop()
{
    UINT8 i;
    Amba_Img_VIn_Invalid(MotorVuPri, (UINT32 *)NULL);
    for (i=0; i<TOTAL_FOV_NUM; i++) {
        if (ImgMultiChanSchdlr[i]) AmbaImgSchdlr_Enable(ImgMultiChanSchdlr[i], 0);
    }

    AmpEnc_StopLiveview(VideoEncPipe, AMP_ENC_FUNC_FLAG_WAIT);

#ifndef KEEP_ALL_INSTANCES
    AmpEnc_Delete(VideoEncPipe); VideoEncPipe = NULL;
    AmpVin_Delete(MotorVuVinA); MotorVuVinA = NULL;
    for (i=0; i<TOTAL_FOV_NUM; i++) {
        if (ImgMultiChanSchdlr[i]) AmbaImgSchdlr_Delete(ImgMultiChanSchdlr[i]);
    }
    AmpVideoEnc_Delete(MotorVuPri); MotorVuPri = NULL;
    AmpVideoEnc_Delete(MotorVuSec); MotorVuSec = NULL;

#endif

    return 0;
}


int AmpUT_MotorVu360_EncPause()
{

    if (Status == STATUS_ENCODE) {
        AmpEnc_PauseRecord(VideoEncPipe, 0);
        Status = STATUS_PAUSE;
    }
    return 0;
}

int AmpUT_MotorVu360_EncResume()
{

    if (Status == STATUS_PAUSE) {
        AmpEnc_ResumeRecord(VideoEncPipe, 0);
        Status = STATUS_ENCODE;
    }

    return 0;
}



int AmpUT_MotorVu360_ChangeResolution(UINT32 modeIdx)
{
    AMP_VIN_RUNTIME_CFG_s VinCfg = {0};
    AMP_VIDEOENC_MAIN_CFG_s MainCfg[2] = {0};
    //AMP_VIDEOENC_LAYER_DESC_s NewPriLayer = {0, 0, 0, AMP_ENC_SOURCE_VIN, 0, 0, {0,0,0,0},{0,0,0,0}};
    AMP_VIDEOENC_LAYER_DESC_s NewSecLayer = {0, 0, 0, AMP_ENC_SOURCE_VIN, 0, 0, {0,0,0,0},{0,0,0,0}};
    AMBA_SENSOR_MODE_INFO_s VinInfo;
    AMP_VIN_LAYOUT_CFG_s Layout[2]; // Dualstream from same vin/vcapwindow
    AMBA_SENSOR_MODE_ID_u Mode = {0};

    EncModeIdx = modeIdx;

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" AmbaUnitTest: Change Resolution to %s",VideoEncMgt[EncModeIdx].Name);
    AmbaPrint(" =========================================================");

    memset(&Layout, 0x0, 2*sizeof(AMP_VIN_LAYOUT_CFG_s));

    Mode.Data = VideoEncMgt[EncModeIdx].SensorMode;
    AmbaSensor_GetModeInfo(VinChannel, Mode, &VinInfo);

    if (strcmp(VideoEncMgt[EncModeIdx].Name, "2560x1440P60 16:9\0") == 0) {
        AmbaPrint("Turn OFF TV");
        TvLiveview = 0;
    } else {
        AmbaPrint("Turn %s Tv",TvLiveview?"ON":"OFF");
    }


    // Update VIN information
    VinCfg.Hdlr = MotorVuVinA;
    VinCfg.Mode = Mode;
    VinCfg.LayoutNumber = 2;

    {
        UINT8 i;
        UINT16 MaxCapWidth = 0, MaxCapHeight = 0;
        for (i=0; i<TOTAL_FOV_NUM; i++) {
            if (FovMainWindow[i].Enable) {
                if (MaxCapWidth < FovMainWindow[i].MainCapWin.X + FovMainWindow[i].MainCapWin.Width)
                    MaxCapWidth = FovMainWindow[i].MainCapWin.X + FovMainWindow[i].MainCapWin.Width;
                if (MaxCapHeight < FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].MainCapWin.Height)
                    MaxCapHeight = FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].MainCapWin.Height;
            }
        }
        VinCfg.HwCaptureWindow.Width = MaxCapWidth;
        VinCfg.HwCaptureWindow.Height = MaxCapHeight;
        VinCfg.HwCaptureWindow.X = VinCfg.HwCaptureWindow.Y = 0;
        Layout[0].Width = FovMainWindow[0].MainWidth;
        Layout[0].Height = FovMainWindow[0].MainHeight;
    }

    Layout[0].EnableSourceArea = 0; // Get all capture window to main
    Layout[0].DzoomFactorX = 1<<16;
    Layout[0].DzoomFactorY = 1<<16;
    Layout[0].DzoomOffsetX = 0;
    Layout[0].DzoomOffsetY = 0;
    Layout[1].Width = SEC_STREAM_WIDTH;
    Layout[1].Height = SEC_STREAM_HEIGHT;
    Layout[1].EnableSourceArea = 0; // Get all capture window to main
    Layout[1].DzoomFactorX = 1<<16;
    Layout[1].DzoomFactorY = 1<<16;
    Layout[1].DzoomOffsetX = 0;
    Layout[1].DzoomOffsetY = 0;
    VinCfg.Layout = &Layout[0];


    // Make sure Img scheduler has been created
    {
        AMBA_IMG_SCHDLR_CFG_s ISCfg;
        UINT8 i, j;
        AmbaImgSchdlr_GetDefaultCfg(&ISCfg);

        for(i=0; i<TOTAL_FOV_NUM; i++) {
            if (FovMainWindow[i].Enable && ImgMultiChanSchdlr[i] == NULL) {
                UINT8 sensorBitmap = 0;
                for(j=0; j<FovMainWindow[i].ChannelNumber; j++) {
                    sensorBitmap |= FovMainWindow[i].ChannelID[j].Bits.SensorID;
                }
                ISCfg.MainViewID = i;
                ISCfg.Channel.Bits.VinID = VinChannel.Bits.VinID;
                ISCfg.Channel.Bits.SensorID = sensorBitmap;
                ISCfg.Vin = MotorVuVinA;
                ISCfg.cbEvent = AmpUT_MotorVu360ImgSchdlrCallback;
                AmbaImgSchdlr_Create(&ISCfg, &ImgMultiChanSchdlr[i]);  // One MainViewID (not vin) need one scheduler.
            }
        }
    }

    // Update VideoEnc information
    { //physical
        AMP_VIDEOENC_LAYER_DESC_s MultChanEncLayer[4];
        UINT16 MaxCapWidth = 0, MaxCapHeight = 0;
        UINT8 i;
        memset(MultChanEncLayer, 0x0, sizeof(AMP_VIDEOENC_LAYER_DESC_s)*4);
        MainCfg[0].MainLayout.Layer = MultChanEncLayer;

        MainCfg[0].Hdlr = MotorVuPri;
        MainCfg[0].MainLayout.Width = 0;
        MainCfg[0].MainLayout.Height = 0;
        MainCfg[0].MainLayout.LayerNumber = 0;
        MainCfg[0].MainTickPerPicture = VideoEncMgt[EncModeIdx].TickPerPicture;
        MainCfg[0].MainTimeScale = VideoEncMgt[EncModeIdx].TimeScale;

        for(i=0; i<TOTAL_FOV_NUM; i++) {
            if (FovMainWindow[i].Enable) {
                MultChanEncLayer[i].SourceType = AMP_ENC_SOURCE_VIN;
                MultChanEncLayer[i].Source = MotorVuVinA; //TBD, find VinID
                MultChanEncLayer[i].SourceLayoutId = FovMainWindow[i].MainViewID;
                MultChanEncLayer[i].EnableSourceArea = 1;
                MultChanEncLayer[i].EnableTargetArea = 1;
                MultChanEncLayer[i].SourceArea = FovMainWindow[i].MainCapWin;
                MultChanEncLayer[i].TargetArea.Width = FovMainWindow[i].MainWidth;
                MultChanEncLayer[i].TargetArea.Height = FovMainWindow[i].MainHeight;
                if (MaxCapWidth < FovMainWindow[i].MainCapWin.X + FovMainWindow[i].MainCapWin.Width)
                    MaxCapWidth = FovMainWindow[i].MainCapWin.X + FovMainWindow[i].MainCapWin.Width;
                if (MaxCapHeight < FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].MainCapWin.Height)
                    MaxCapHeight = FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].MainCapWin.Height;
                MainCfg[0].MainLayout.LayerNumber++;
            }
        }
        MainCfg[0].MainLayout.Width = MaxCapWidth;
        MainCfg[0].MainLayout.Height = MaxCapHeight;
    }

    MainCfg[0].SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[0].SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[0].SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;

    {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        AmpUT_MotorVu360_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
        MainCfg[0].DspWorkBufAddr = dspWorkAddr;
        MainCfg[0].DspWorkBufSize = dspWorkSize;
    }

    MainCfg[1].Hdlr = MotorVuSec;
    MainCfg[1].MainLayout.LayerNumber = 1;
    MainCfg[1].MainLayout.Layer = &NewSecLayer;
    MainCfg[1].MainLayout.Width = SEC_STREAM_WIDTH;
    MainCfg[1].MainLayout.Height = SEC_STREAM_HEIGHT;
    MainCfg[1].MainTickPerPicture = SEC_STREAM_TICK;
    MainCfg[1].MainTimeScale = SEC_STREAM_TIMESCALE;
    MainCfg[1].SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[1].SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[1].SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;


    if (VideoEncMgt[EncModeIdx].DualValid == 0) {
        MainCfg[1].MainLayout.Width = 0;
        MainCfg[1].MainLayout.Height = 0;
        EncDualStream = 0;
        AmbaPrint(" ==> Disable dual stream");
    }

    MainCfg[1].DspWorkBufAddr = 0;  // Don't want to change dsp buffer
    MainCfg[1].DspWorkBufSize = 0;  // Don't want to change dsp buffer
    NewSecLayer.EnableSourceArea = 0;
    NewSecLayer.EnableTargetArea = 0;
    NewSecLayer.LayerId = 0;
    NewSecLayer.SourceType = AMP_ENC_SOURCE_VIN;
    NewSecLayer.Source = MotorVuVinA;
    NewSecLayer.SourceLayoutId = 1;

    AmpVideoEnc_ConfigVinMain(1, &VinCfg, 2, &MainCfg[0]);

    // Remember new frame/field rate for muxer
    EncFrameRate = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;

    return 0;
}

/**
 * UnitTest: Calculate dsp working address and size base on current capture mode
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_MotorVu360_DspWork_Calculate(UINT8 **addr, UINT32 *size)
{
    extern UINT8 *DspWorkAreaResvStart;
    extern UINT32 DspWorkAreaResvSize;

    (*addr) = DspWorkAreaResvStart;
    (*size) = DspWorkAreaResvSize - 15*1024*1024;

    AmbaPrint("[DspWork_Calculate] Addr 0x%X, Sz %u", *addr, *size);
    return 0;
}

int AmpUT_MotorVu360Test(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_MotorVu360Test cmd: %s", argv[1]);
    if (strcmp(argv[1],"init") == 0) {
        int ss = 0;

        if (Status != STATUS_IDLE) {
            return -1;
        }

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc init [sensor_id]");
            AmbaPrint("               sensor_id: 0 -- IMX122@B5");
            return 0;
        }
        ss = atoi(argv[2]);
        AmpUT_MotorVu360_Init(ss);
        Status = STATUS_INIT;
    } else if (strcmp(argv[1],"tv") == 0) {
        if (Status != STATUS_INIT) {
            AmbaPrint("Please set it before liveviewstart");
        }

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc tv [enable]");
        }

        TvLiveview = atoi(argv[2]);

        AmbaPrint("TV: %s",TvLiveview?"Enable":"Disable");

    } else if ((strcmp(argv[1],"liveviewstart") == 0) || (strcmp(argv[1],"lvst") == 0)) {
        UINT32 tt;

        if (Status != STATUS_INIT)
            return -1;

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc liveviewstart [modeId][viewId]");
            AmbaPrint("IMX122@B5: ");
            AmbaPrint("          mode_id:");
            AmbaPrint("             0 -- 1920x1080  30fps");
            AmbaPrint("          view_id:");
            AmbaPrint("             0 -- default value, set FrontView as 5th VoutWindow");
            AmbaPrint("             1 -- set RearView as 5th VoutWindow");
            AmbaPrint("             2 -- set LeftView as 5th VoutWindow");
            AmbaPrint("             3 -- set RightView as 5th VoutWindow");
            return -1;
        }
        tt = atoi(argv[2]);

        AmpUT_MotorVu360_LiveviewStart(tt);
        Status = STATUS_LIVEVIEW;
    } else if ((strcmp(argv[1],"encstart") == 0) || (strcmp(argv[1],"enst") == 0)) {
        UINT32 tt = atoi(argv[2]);

        if (Status != STATUS_LIVEVIEW)
            return -1;

        AmbaPrint("Encode for %d milliseconds", tt);
        AmpUT_MotorVu360_EncodeStart();
        Status = STATUS_ENCODE;

        if (tt != 0) {
            while (tt) {
                AmbaKAL_TaskSleep(1);
                tt--;
                if (tt == 0) {
                    AmpUT_MotorVu360_EncStop();
                    Status = STATUS_LIVEVIEW;
                }
                if (Status == STATUS_LIVEVIEW)
                    tt = 0;
            }
        }

    } else if ((strcmp(argv[1],"encstop") == 0) || (strcmp(argv[1],"ensp") == 0)) {
        if (Status != STATUS_ENCODE && Status != STATUS_PAUSE)
            return -1;

        AmpUT_MotorVu360_EncStop();
        Status = STATUS_LIVEVIEW;
    } else if ((strcmp(argv[1],"pause") == 0)) {
        AmpUT_MotorVu360_EncPause();
    } else if ((strcmp(argv[1],"resume") == 0)) {
        AmpUT_MotorVu360_EncResume();
    } else if ((strcmp(argv[1],"liveviewstop") == 0) || (strcmp(argv[1],"lvsp") == 0)) {
        if (Status != STATUS_LIVEVIEW)
            return -1;
        AmpUT_MotorVu360_LiveviewStop();
        Status = STATUS_INIT;
    } else if ((strcmp(argv[1],"dual") == 0)) {
        UINT32 tt = atoi(argv[2]);

        if (Status != STATUS_INIT) {
            AmbaPrint("Please set dualstream before liveview start");
            return -1;
        }

        AmbaPrint("DualStream: %s", tt?"ENABLE":"DISABLE");
        EncDualStream = tt;
    } else if ((strcmp(argv[1],"sc") == 0)) {
        AMBA_IMG_SCHDLR_EXP_s Exp = {0};

        if (argc < 10) {
            AmbaPrint("Usage: t videnenc sc [Type] [AGC] [Shutter] [RGain] [GGain] [BGain] [AEGain] [GlobalGain]");
            AmbaPrint("                     Type: 2 - Direct, 3 - By scheduler");
            AmbaPrint("                     Shutter: fps. Eg, 60 = 60fps");
            return -1;
        }

        Exp.Type = (AMBA_IMG_SCHDLR_SET_TYPE_e)atoi(argv[2]);
        Exp.Info.SubChannelNum = 0;
        Exp.Info.AGC[0] = 1.0*atoi(argv[3]);
        Exp.Info.Shutter[0] = 1.0/atoi(argv[4]);
        Exp.Info.DGain.GainR = atoi(argv[5]);
        Exp.Info.DGain.GainG = atoi(argv[6]);
        Exp.Info.DGain.GainB = atoi(argv[7]);
        Exp.Info.DGain.AeGain = atoi(argv[8]);
        Exp.Info.DGain.GlobalDGain = atoi(argv[9]);

        AmbaPrint("Type %d AGC %f Shutter %f Gain R:%d G:%d B:%d AE:%d GLOBAL:%d",Exp.Type, Exp.Info.AGC,Exp.Info.Shutter,
        Exp.Info.DGain.GainR ,Exp.Info.DGain.GainG, Exp.Info.DGain.GainB, Exp.Info.DGain.AeGain, Exp.Info.DGain.GlobalDGain);

        AmbaImgSchdlr_SetExposure((UINT32)0, &Exp);

    } else if ((strcmp(argv[1],"3a") == 0)) {
        UINT8 Is3aEnable = atoi(argv[2]);
        UINT32 ChNo = 0;
        AMBA_3A_OP_INFO_s AaaOpInfo = {0};
        if (Is3aEnable == 0) {
            MVu3AEnable = 0;
            AaaOpInfo.AeOp = DISABLE;
            AaaOpInfo.AfOp = DISABLE;
            AaaOpInfo.AwbOp = DISABLE;
            AaaOpInfo.AdjOp = DISABLE;
            AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
        } else if (Is3aEnable == 1) {
            MVu3AEnable = 1;
            AaaOpInfo.AeOp = ENABLE;
            AaaOpInfo.AfOp = ENABLE;
            AaaOpInfo.AwbOp = ENABLE;
            AaaOpInfo.AdjOp = ENABLE;
            AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
        }
        AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
        AmbaPrint("3A %s: ae:%u af:%u awb:%u adj:%u", MVu3AEnable?"Enable":"Disable", AaaOpInfo.AeOp, \
            AaaOpInfo.AfOp, AaaOpInfo.AwbOp, AaaOpInfo.AdjOp);
    } else if ((strcmp(argv[1],"spec") == 0)) {
        UINT32 v1 = atoi(argv[2]);
        UINT32 v2 = atoi(argv[3]);

        if (Status == STATUS_ENCODE) {
            AmbaPrint("Wrong Status");
            return 0;
        }
        if (argc < 4) {
            AmbaPrint("Usage: t videoenc spec [PriSpec] [SecSpec]");
            AmbaPrint("                  PriSpec: Primary stream spec. 1 - H.264 (default), 0 - MotionJPEG");
            AmbaPrint("                  SecSpec: Secondary stream spec. 1 - H.264 (default), 0 - MotionJPEG");
            return 0;
        }
        AmbaPrint("Primary Stream: %s",v1?"H.264":"MJPEG");
        AmbaPrint("Secondary Stream (if enabled): %s",v2?"H.264":"MJPEG");
        EncPriSpecH264 = v1;
        EncSecSpecH264 = v2;
        if (Status == STATUS_LIVEVIEW) {
            AmpUT_MotorVu360_ChangeResolution(EncModeIdx);
        }


    }  else if ((strcmp(argv[1],"vinencchg") == 0) || (strcmp(argv[1],"chg") == 0)) {
        if (argc < 3) {
            AmbaPrint("Usage: t videoenc chg [modeId][viewId]");
            AmbaPrint("IMX122@B5: ");
            AmbaPrint("          mode_id:");
            AmbaPrint("             0 -- 1920x1080  30fps");
            return 0;
        } else {
            UINT32 md;

            if (Status != STATUS_LIVEVIEW)
                return 0;

            md = atoi(argv[2]);

            AmpUT_MotorVu360_ChangeResolution(md);
        }
    } else if (strcmp(argv[1],"mpl")==0) {
        AmbaPrint("MMPL remain %d frag %d ",G_MMPL.tx_byte_pool_available,G_MMPL.tx_byte_pool_fragments);
    } else if (strcmp(argv[1],"mpl2")==0) {
     static void *tz;
        AmbaKAL_BytePoolAllocate(&G_MMPL, &tz, 100, 10000);
        AmbaPrint("MMPL remain %d frag %d ",G_MMPL.tx_byte_pool_available,G_MMPL.tx_byte_pool_fragments);
        AmbaKAL_BytePoolFree(tz);
    } else if (strcmp(argv[1],"vdsp")==0) {
            VdspPrint = atoi(argv[2]);
            AmbaPrint("VdspPrint: %s",VdspPrint?"ON":"OFF");
    } else if (strcmp(argv[1],"logmuxer")==0) {
        LogMuxer = atoi(argv[2]);
        AmbaPrint("Log muxer: %s",LogMuxer?"ON":"OFF");
    } else if (strcmp(argv[1],"osd")==0) {

        if (atoi(argv[2]) == 1) {
            AmpOsd_ActivateOsdWindow(OsdTHdlr, OsdWinTvHdlr);
        } else {
            AmpOsd_DeActivateOsdWindow(OsdTHdlr, OsdWinTvHdlr);
        }
    } else if (strcmp(argv[1],"freq")==0) {
        AmbaPrint("---------------------------------------------");
        AmbaPrint("Cortex freq:\t\t%d", AmbaPLL_GetCortexClk());
        AmbaPrint("ARM freq:\t\t%d", AmbaPLL_GetArmClk());
        AmbaPrint("DDR freq:\t\t%d", AmbaPLL_GetDdrClk());
        AmbaPrint("IDSP freq:\t\t%d", AmbaPLL_GetIdspClk());
        AmbaPrint("Core freq:\t\t%d", AmbaPLL_GetCoreClk());
        AmbaPrint("AXI freq:\t\t%d", AmbaPLL_GetAxiClk());
        AmbaPrint("AHB freq:\t\t%d", AmbaPLL_GetAhbClk());
        AmbaPrint("APB freq:\t\t%d", AmbaPLL_GetApbClk());
        AmbaPrint("---------------------------------------------");

        AmbaPrint("----- Version info --------------------------");
        AmbaPrint("FlowPipeVer  : %5d", AmpMW_GetFlowPipeVer());
        AmbaPrint("DataPipeVer  : %5d", AmpMW_GetDataPipeVer());
        AmbaPrint("SchedulerVer : %5d", AmbaImgSchdlr_GetVer());
        AmbaPrint("---------------------------------------------");

    } else {
        AmbaPrint("Usage: t videoenc cmd ...");
        AmbaPrint("    cmd:");
        AmbaPrint("       init: init all");
        AmbaPrint("       liveviewstart: start liveview");
        AmbaPrint("       encstart [millisecond]: recording for N milliseconds, N = 0 will do continuous encoding");
        AmbaPrint("       encstop: stop recording");
        AmbaPrint("       pause: pause recording");
        AmbaPrint("       resume: resume recording");
        AmbaPrint("       chg: change sensor resolution (when liveview)");
        AmbaPrint("       spec: Select bitstream specification");
    }
    return 0;
}

int AmpUT_MotorVu360TestAdd(void)
{
    AmbaPrint("Adding AmpUT_MotorVu360 %s %d",__FUNCTION__, __LINE__);
    // hook command
    AmbaTest_RegisterCommand("mv", AmpUT_MotorVu360Test);

   // AmpUT_MotorVu360_Init(0);
   // AmpUT_MotorVu360_LiveviewStart();

    return AMP_OK;
}
