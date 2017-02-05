 /**
  * @file src/app/sample/unittest/AmpUT_MotorVu2CH.c
  *
  * MotorVu Dual Channel Encode/Liveview unit test (Use one IMX117 to simulate)
  *
  * History:
  *    2014/08/18 - [Jenghung Luo] created file
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
#include <AmbaSensor_IMX117.h>
#include <AmbaDSP_WarpCore.h>
#include "AmbaImg_VDsp_Handler.h"
#include "AmbaImg_VIn_Handler.h"
#include <recorder/StillEnc.h>
#include <scheduler.h>
#include "AmbaUtility.h"
#include "AmbaPLL.h"

#include "AmbaImg_Proc.h"
#include "AmbaImg_Impl_Cmd.h"
#define FULLHD_VOUT

static char DefaultSlot[] = "C";

void *UT_MotorVu2CHfopen(const char *pFileName, const char *pMode, BOOL8 AsyncMode)
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

int UT_MotorVu2CHfclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_MotorVu2CHfwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_MotorVu2CHfsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
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

#define ENCODE_MAIN_FOV_NUM          2              // Main window number for encode
#define VOUT_MAIN_FOV_NUM            2              // Main window number for vout
#define TOTAL_MAIN_FOV_NUM           (ENCODE_MAIN_FOV_NUM + VOUT_MAIN_FOV_NUM)
#define ENCODE_FOV_PER_STREAM_NUM    2
#define TOTAL_ENCODE_STREAM_NUM      1

#define STATUS_IDLE     1
#define STATUS_INIT     2
#define STATUS_LIVEVIEW 3
#define STATUS_ENCODE   4
#define STATUS_PAUSE    5
static UINT8 Status = STATUS_IDLE;                  // Simple state machine to avoid wrong command issuing

extern __weak AMBA_SENSOR_OBJ_s AmbaSensor_IMX117Obj;
extern AMBA_LCD_OBJECT_s AmbaLCD_WdF9648wObj;

static UINT8 TvLiveview = 1;                        // Enable TV liveview or not
static UINT8 LCDLiveview = 1;                       // Enable TV liveview or not
static UINT8 MVu2CH3AEnable = 0;                    // Enable AE/AWB/ADJ algo or not
static UINT8 EncPriSpecH264 = 1;                    // Primary codec instance output spec. 1 = H264, 0 = MJPEG
static UINT8 EncSecSpecH264 = 1;                    // Secondary codec instance output spec. 1 = H264, 0 = MJPEG
static UINT8 EncDualStream = 0;                     // Enable dual stream or not

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

UINT32 AmpUT_MotorVu2CH_DspWork_Calculate(UINT8 **addr, UINT32 *size);


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

#define FENC_WIDTH 1920


static encmgt VideoEncMgt117[] = {
    [0] = {
        .SensorMode = AMBA_SENSOR_IMX117_TYPE_2_5_MODE_0_30P,
        .CaptureWidth = 3840,
        .CaptureHeight = 2160,
        .MainWidth = FENC_WIDTH,
        .MainHeight = 2880,

        .TimeScale = 30000,
        .TickPerPicture = 1001,
        .AspectRatio = VAR_16x9,
        .GopM = 1,
        .DualValid = 1,
        .BrcMode = VIDEOENC_CBR,
        .MaxBitRate = 18,
        .MinBitRate = 6,
        .AverageBitRate = 12,
        .Name = "1920x1440P30 16:9\0",
    },
};

static encmgt *VideoEncMgt;         // Pointer to above tables
static UINT8 EncModeIdx = 1;        // Current mode index

#define GOP_N    8                  // I-frame distance
#define GOP_IDR  8                  // IDR-frame distance

#if 0
#define SEC_STREAM_WIDTH   432      // Secondary codec output width
#define SEC_STREAM_HEIGHT  240      // Secondary codec output height
#define SEC_STREAM_TIMESCALE 30000  // Secondary codec frame rate time scale
#define SEC_STREAM_TICK 1001        // Secondary codec frame rate tick per frame
#endif
static AMP_DISP_WINDOW_HDLR_s *LCDWinHdlr[VOUT_MAIN_FOV_NUM] = {0}; // LCD video plane window handler
static AMP_DISP_WINDOW_HDLR_s *TVWinHdlr[VOUT_MAIN_FOV_NUM] = {0}; // LCD video plane window handler

/** Description of the real sensors mounted on B5F */
 static AMBA_DSP_CHANNEL_ID_u SensorChannel[1] = {
    [0] = {
        .Bits.VinID = 0,
        .Bits.SensorID = 1,
    },
};
static AMP_AREA_s SensorCaptureWindow[1] = {
    [0] = {
        .X = 0, .Y = 0,
        .Width = 3840, .Height = 2160,
    },
};

#define SUR_MAIN_WIDTH 640
#define SUR_MAIN_HEIGHT 480

/** Every FOV comes from a main window */
static AMP_MULTI_CHAN_MAIN_WINDOW_CFG_s FovMainWindow[TOTAL_MAIN_FOV_NUM] = {
    /** Liveview FOV */
    [0] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[0],
        .SensorCapWin = &SensorCaptureWindow[0],
        .MainViewID = 0,
        .MainCapWin = {.X = 0, .Y = 0, .Width = 1920, .Height = 1440},
        .MainWidth = SUR_MAIN_WIDTH,
        .MainHeight = SUR_MAIN_HEIGHT,
    },
    [1] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[0],
        .SensorCapWin = &SensorCaptureWindow[0],
        .MainViewID = 1,
        .MainCapWin = {.X = 1280, .Y = 0, .Width = 1920, .Height = 1440},
        .MainWidth = SUR_MAIN_WIDTH,
        .MainHeight = SUR_MAIN_HEIGHT,
    },
    /** Encode FOV */
    [2] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[0],
        .SensorCapWin = &SensorCaptureWindow[0],
        .MainViewID = 2,
        .MainCapWin = {.X = 0, .Y = 0, .Width = 1920, .Height = 1440},
        .MainWidth = FENC_WIDTH,
        .MainHeight = 1440,
    },
    [3] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = &SensorChannel[0],
        .SensorCapWin = &SensorCaptureWindow[0],
        .MainViewID = 3,
        .MainCapWin = {.X = 1280, .Y = 0, .Width = 1920, .Height = 1440},
        .MainWidth = FENC_WIDTH,
        .MainHeight = 1440,
    },
};

/** Definition for SD resolution */
#define SD_PLANE_TOTAL_WIDTH  720
#define SD_PLANE_TOTAL_HEIGHT 480
#define SD_FOCUS_WIDTH        352
#define SD_FOCUS_HEIGHT       264
#define SD_FOCUS_GAP          (SD_PLANE_TOTAL_WIDTH - (SD_FOCUS_WIDTH << 1))
/** Definition for FHD resolution */
#define FHD_PLANE_TOTAL_WIDTH  1920
#define FHD_PLANE_TOTAL_HEIGHT 1080
#define FHD_FOCUS_WIDTH         952
#define FHD_FOCUS_HEIGHT        714
#define FHD_FOCUS_GAP          (FHD_PLANE_TOTAL_WIDTH - (FHD_FOCUS_WIDTH << 1))

/**
 *  Liveview FOVs
 *  +-------+-------+
 *  |       |       |
 *  | FOV_0 | FOV_1 |
 *  |       |       |
 *  +-------+-------+
 */
static AMP_MULTI_CHAN_VOUT_WINDOW_CFG_s FovVoutWindow[2][VOUT_MAIN_FOV_NUM] = {
    /* LCD */
    [0] = {
        [0] = {
            .Enable = 1,
            .MainWindowView = 0,
            .LayerID = 0,
            .DisplayWin = {.X = 0, .Y = (SD_PLANE_TOTAL_HEIGHT - SD_FOCUS_HEIGHT)>>1, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT, .Pitch = ALIGN_32(SD_FOCUS_WIDTH),},
        },
        [1] = {
            .Enable = 1,
            .MainWindowView = 1,
            .LayerID = 1,
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
            .DisplayWin = {.X = 0, .Y = (FHD_PLANE_TOTAL_HEIGHT - FHD_FOCUS_HEIGHT)>>1, .Width = FHD_FOCUS_WIDTH, .Height = FHD_FOCUS_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = FHD_FOCUS_WIDTH, .Height = FHD_FOCUS_HEIGHT, .Pitch = ALIGN_32(FHD_FOCUS_WIDTH),},
        },
        [1] = {
            .Enable = 1,
            .MainWindowView = 1,
            .LayerID = 1,
            .DisplayWin = {.X = FHD_PLANE_TOTAL_WIDTH - FHD_FOCUS_WIDTH, .Y = (FHD_PLANE_TOTAL_HEIGHT - FHD_FOCUS_HEIGHT)>>1, .Width = FHD_FOCUS_WIDTH, .Height = FHD_FOCUS_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = FHD_FOCUS_WIDTH, .Height = FHD_FOCUS_HEIGHT, .Pitch = ALIGN_32(FHD_FOCUS_WIDTH),},
        },
#else
        [0] = {
            .Enable = 1,
            .MainWindowView = 0,
            .LayerID = 0,
            .DisplayWin = {.X = 0, .Y = (SD_PLANE_TOTAL_HEIGHT - SD_FOCUS_HEIGHT)>>1, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT, .Pitch = ALIGN_32(SD_FOCUS_WIDTH),},
        },
        [1] = {
            .Enable = 1,
            .MainWindowView = 1,
            .LayerID = 1,
            .DisplayWin = {.X = SD_PLANE_TOTAL_WIDTH - SD_FOCUS_WIDTH, .Y = (SD_PLANE_TOTAL_HEIGHT - SD_FOCUS_HEIGHT)>>1, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = SD_FOCUS_WIDTH, .Height = SD_FOCUS_HEIGHT, .Pitch = ALIGN_32(SD_FOCUS_WIDTH),},
        },
#endif
    },
};

/**
 *     Encode FOVs (1 2560x1440 stream, each stream consume 2 FOV main)
 *     +-------+ +-------+
 *     |       | |       |
 *     | FOV_2 | | FOV_3 |
 *     |       | |       |
 *     +-------+ +-------+
 */
static AMP_MULTI_CHAN_ENCODE_WINDOW_CFG_s FovEncodeWindow[TOTAL_ENCODE_STREAM_NUM][ENCODE_FOV_PER_STREAM_NUM] = {
    /* Primary */
    [0] = {
        [0] = {
            .Enable = 1,
            .MainWindowView = 2,
            .LayerID = 0,
            .SrcWin = {.X = 0, .Y = 0, .Width = FENC_WIDTH, .Height = 1440},  // Use whole FOV main #2
            .DestWin = {.X = 0, .Y = 0, .Width = FENC_WIDTH, .Height = 1440},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
#if 1
        [1] = {
            .Enable = 1,
            .MainWindowView = 3,
            .LayerID = 1,
            .SrcWin = {.X = 0, .Y = 0, .Width = FENC_WIDTH, .Height = 1440},  // Use whole FOV main #2
            .DestWin = {.X = 0, .Y = 1440, .Width = FENC_WIDTH, .Height = 1440},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
#endif
    },
#if 0
    /* Secondary */
    [1] = {
        [0] = {
            .Enable = 1,
            .MainWindowView = 3,
            .LayerID = 0,
            .SrcWin = {.X = 0, .Y = 0, .Width = 2560, .Height = 1440}, // Use whole FOV main #3
            .DestWin = {.X = 0, .Y = 0, .Width = 2560, .Height = 1440},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
#if 0
        [1] = {
            .Enable = 0,
            .MainWindowView = 3,
            .LayerID = 0,
            .SrcWin = {.X = 0, .Y = 0, .Width = 432, .Height = 240}, // Use whole FOV main #3
            .DestWin = {.X = 0, .Y = 0, .Width = 432, .Height = 240},
            .Rotate = AMP_ROTATE_0,
            .BlendTable = {.Buf = NULL, .Width = 0, .Height = 0, .Pitch = 0,},
        },
#endif
    },
#endif
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

static int fnoPri = 0;                  // Frame number counter for primary stream muxer
static int fnoSec = 0;                  // Frame number counter for secondary stream muxer
static UINT32 EncFrameRate = 0;         // Remember current framerate for primary muxer to calculate actual bitrate
static UINT64 encPriBytesTotal = 0;     // total bytes primary stream muxer received
static UINT32 encPriTotalFrames = 0;    // total frames primary stream muxer received
static UINT64 encSecBytesTotal = 0;     // total bytes secondary stream muxer received
static UINT32 encSecTotalFrames = 0;    // total frames secondary stream muxer received
static AMP_FIFO_HDLR_s *VirtualPriFifoHdlr = NULL;  // Primary stream vitrual fifo
static AMP_FIFO_HDLR_s *VirtualSecFifoHdlr = NULL;  // Secondary stream vitrual fifo
static UINT8 LogMuxer = 1;              // Log muxing info
static UINT8 VdspPrint = 0;             // Print vdsp ack
static UINT32 VdspCount = 0;            // vdsp counter


/** UT function prototype */
int AmpUT_MotorVu2CH_EncStop(void);
int AmpUT_MotorVu2CH_LiveviewStart(UINT32 modeIdx);
int AmpUT_MotorVu2CH_ChangeMainView(UINT32 MainViewID);

/*************************************** Muxer (Data flow) implementation start ********************************************************/

/**
 * Primary muxer task
 *
 * @param [in] info initial value
 *
 */

static UINT32 BrcFrameCount = 0;
static UINT32 BrcByteCount = 0;

void AmpUT_MotorVu2CH_PriMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc;
    int Er;
    UINT8 *BitsLimit;

    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_MotorVu2CH_PriMuxTask Start");

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
            outputPriFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII, 1);
            while (outputPriFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                outputPriFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII, 1);
            }
            AmbaPrint("%s opened", Fn);

            sprintf(Fn,"%s:\\OUT_%04d.nhnt", DefaultSlot, fnoPri);
            outputPriIdxFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII, 1);
            while (outputPriIdxFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                outputPriIdxFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII, 1);
            }
            AmbaPrint("%s opened", Fn);

            sprintf(Fn,"%s:\\OUT_%04d.udta", DefaultSlot, fnoPri);
            UdtaPriFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII, 1);
            while (UdtaPriFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                UdtaPriFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII, 1);
            }
            AmbaPrint("%s opened", Fn);

            Udta.nIdrInterval = GOP_IDR/GOP_N;
            Udta.nTimeScale = VideoEncMgt[EncModeIdx].TimeScale;
            Udta.nTickPerPicture = VideoEncMgt[EncModeIdx].TickPerPicture;
            Udta.nN = GOP_N;
            Udta.nM = VideoEncMgt[EncModeIdx].GopM;
            Udta.nVideoWidth = VideoEncMgt[EncModeIdx].MainWidth;
            Udta.nVideoHeight = VideoEncMgt[EncModeIdx].MainHeight;

            UT_MotorVu2CHfwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)UdtaPriFile);
            UT_MotorVu2CHfclose((void *)UdtaPriFile);

            NhntHeader.Signature[0]='n';
            NhntHeader.Signature[1]='h';
            NhntHeader.Signature[2]='n';
            NhntHeader.Signature[3]='t';
            NhntHeader.TimeStampResolution = 90000;
            UT_MotorVu2CHfwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)outputPriIdxFile);

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
                    UT_MotorVu2CHfsync((void *)outputPriFile);
                    UT_MotorVu2CHfclose((void *)outputPriFile);
                    fnoPri++;
                    UT_MotorVu2CHfsync((void *)outputPriIdxFile);
                    UT_MotorVu2CHfclose((void *)outputPriIdxFile);
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

        } else {
        #ifdef AMPUT_FILE_DUMP
                if (outputPriFile) {
                    NhntSample.CompositionTimeStamp = Desc->Pts;
                    NhntSample.DecodingTimeStamp = Desc->Pts;
                    NhntSample.DataSize = Desc->Size;
                    NhntSample.FileOffset = FileOffset;
                    FileOffset += Desc->Size;
                    NhntSample.FrameType = Desc->Type;

                    UT_MotorVu2CHfwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)outputPriIdxFile);
             //       AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                    if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                        UT_MotorVu2CHfwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputPriFile);
                    } else {
                        UT_MotorVu2CHfwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputPriFile);
                        if (EncPriSpecH264) {
                            UT_MotorVu2CHfwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
                        } else {
                            UT_MotorVu2CHfwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
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

void AmpUT_MotorVu2CH_SecMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc;
    int Er;
    UINT8 *BitsLimit;

    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_MotorVu2CH_SecMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&MotorVuSecSem, 10000);
        if (Er!= OK) {
          //  AmbaPrint(" no sem fff");
            continue;
        }

#define AMPUT_FILE_DUMP
        #ifdef AMPUT_FILE_DUMP
        if (outputSecFile == NULL) {
            char Fn[80];
            char mdASCII[3] = {'w','+','\0'};
            FORMAT_USER_DATA_s Udta;

            sprintf(Fn,"%s:\\OUT_%04d_s.%s", DefaultSlot, fnoSec, EncSecSpecH264? "h264": "mjpg");
            outputSecFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII,1);
            while (outputSecFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                outputSecFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII,1);
            }
            AmbaPrint("%s opened", Fn);

            sprintf(Fn,"%s:\\OUT_%04d_s.nhnt", DefaultSlot, fnoSec);
            outputSecIdxFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII,1);
            while (outputSecIdxFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                outputSecIdxFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII,1);
            }
            AmbaPrint("%s opened", Fn);

            sprintf(Fn,"%s:\\OUT_%04d_s.udta", DefaultSlot, fnoSec);
            UdtaSecFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII,1);
            while (UdtaSecFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                UdtaSecFile = UT_MotorVu2CHfopen((const char *)Fn, (const char *)mdASCII,1);
            }
            AmbaPrint("%s opened", Fn);

            Udta.nIdrInterval = GOP_IDR/GOP_N;
            Udta.nTimeScale = VideoEncMgt[EncModeIdx].TimeScale;
            Udta.nTickPerPicture = VideoEncMgt[EncModeIdx].TickPerPicture;
            Udta.nN = GOP_N;
            Udta.nM = VideoEncMgt[EncModeIdx].GopM;
            Udta.nVideoWidth = VideoEncMgt[EncModeIdx].MainWidth;
            Udta.nVideoHeight = VideoEncMgt[EncModeIdx].MainHeight;

            UT_MotorVu2CHfwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)UdtaSecFile);
            UT_MotorVu2CHfclose((void *)UdtaSecFile);

            NhntHeader.Signature[0]='n';
            NhntHeader.Signature[1]='h';
            NhntHeader.Signature[2]='n';
            NhntHeader.Signature[3]='t';
            NhntHeader.TimeStampResolution = 90000;
            UT_MotorVu2CHfwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)outputSecIdxFile);
            fnoSec++;

            if (EncSecSpecH264) {
                BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
            } else {
                BitsLimit = MjpgBitsBuf + BITSFIFO_SIZE - 1;
            }
        }
        #endif

        Er = AmpFifo_PeekEntry(VirtualSecFifoHdlr, &Desc, 0);
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
        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 Avg;
            // EOS
            #ifdef AMPUT_FILE_DUMP
                if (outputSecFile) {
                    UT_MotorVu2CHfsync((void *)outputSecFile);
                    UT_MotorVu2CHfclose((void *)outputSecFile);
                    UT_MotorVu2CHfsync((void *)outputSecIdxFile);
                    UT_MotorVu2CHfclose((void *)outputSecIdxFile);
                    outputSecFile = NULL;
                    outputSecIdxFile = NULL;
                    FileOffset = 0;
                }
            #endif

            Avg = (UINT32)(encPriBytesTotal*8.0*EncFrameRate/encPriTotalFrames/1E6);

            AmbaPrint("Secondary Muxer met EOS, total %d frames/fields", encSecTotalFrames);
            AmbaPrint("Secondary Bitrate Average: %d Mbps\n", Avg);

            encSecBytesTotal = 0;
            encSecTotalFrames = 0;
        } else {
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

                    UT_MotorVu2CHfwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)outputSecIdxFile);
              //      AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                    if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                        UT_MotorVu2CHfwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputSecFile);
                    } else {
                        UT_MotorVu2CHfwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputSecFile);
                        if (EncSecSpecH264) {
                            UT_MotorVu2CHfwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputSecFile);
                        } else {
                            UT_MotorVu2CHfwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputSecFile);
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

static int AmpUT_MotorVu2CH_FifoCB(void *hdlr, UINT32 event, void* info)
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
 * Generic MotorVu2CH ImgSchdlr callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_MotorVu2CHImgSchdlrCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMBA_IMG_SCHDLR_EVENT_CFA_STAT_READY:
            if (VdspPrint) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                 AMBA_DSP_EVENT_CFA_3A_DATA_s *cfa = (AMBA_DSP_EVENT_CFA_3A_DATA_s *)Meta->AAAStatAddress;
                 static UINT32 LastVdspTime = 0;

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

            if (MVu2CH3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                AMBA_DSP_EVENT_CFA_3A_DATA_s *cfa = (AMBA_DSP_EVENT_CFA_3A_DATA_s *)Meta->AAAStatAddress;
                if (cfa->Header.ChannelId == 0) {
                    Amba_Img_VDspCfa_Handler(hdlr, (UINT32 *)Meta);
                }
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_RGB_STAT_READY:
           if (MVu2CH3AEnable) {
               AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
               Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
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
static int AmpUT_MotorVu2CHVinSwitchCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {

        case AMP_VIN_EVENT_INVALID:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_INVALID info: %X %X", info, ImgSchdlr);

            if (ImgSchdlr) AmbaImgSchdlr_Enable(ImgSchdlr, 0);
            Amba_Img_VIn_Invalid(hdlr, (UINT32 *)NULL);
            break;
        case AMP_VIN_EVENT_VALID:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_VALID info: %X %X", info, ImgSchdlr);
            if (MVu2CH3AEnable) {
                Amba_Img_VIn_Valid(hdlr, (UINT32 *)NULL);
            }
            if (ImgSchdlr) {
                AmbaImgSchdlr_Enable(ImgSchdlr, 1);
            }
            break;
        case AMP_VIN_EVENT_CHANGED_PRIOR:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_PRIOR info: %X", info);
            if (MVu2CH3AEnable)
                Amba_Img_VIn_Changed_Prior(hdlr, (UINT32 *)NULL);
            break;
        case AMP_VIN_EVENT_CHANGED_POST:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_POST info: %X", info);

            if (MVu2CH3AEnable) {
                UINT8 IsPhotoLiveView = 0;
                //inform 3A LiveView type
                AmbaImg_Proc_Cmd(MW_IP_SET_PHOTO_PREVIEW, &IsPhotoLiveView, 0, 0);
                Amba_Img_VIn_Changed_Post(hdlr, (UINT32 *)NULL);
            }
            if (Status == STATUS_LIVEVIEW && LCDHdlr) { // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window;
                memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));

                if (LCDWinHdlr[0]) {
                    UINT8 i;
                    for (i=0; i<VOUT_MAIN_FOV_NUM; i++) {
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
                    for (i=0; i<VOUT_MAIN_FOV_NUM; i++) {
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
                //disable ImgSchdlr
                AmbaPrint("VinSWCB: AMP_VIN_EVENT_MAINVIEW_CHANGED_PRIOR");
                if (ImgSchdlr) {
                    AmbaImgSchdlr_Enable(ImgSchdlr, 0);
                }
            }
            break;
        case AMP_VIN_EVENT_MAINVIEW_CHANGED_POST:
            {
                //enable ImgSchdlr
                AmbaPrint("VinSWCB: AMP_VIN_EVENT_MAINVIEW_CHANGED_POST");
                if (ImgSchdlr) {
                    AmbaImgSchdlr_Enable(ImgSchdlr, 1);
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
static int AmpUT_MotorVu2CHVinEventCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_VIN_EVENT_FRAME_READY:
            //if (xcnt % 30 == 0)
           //  AmbaPrint("VinEVCB: AMP_VIN_EVENT_FRAME_READY info: %X", info);
           // xcnt++;
            break;
        case AMP_VIN_EVENT_FRAME_DROPPED:
         //   AmbaPrint("VinEVCB: AMP_VIN_EVENT_FRAME_DROPPED info: %X", info);
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
static int AmpUT_MotorVu2CHCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        #if 0 //TBM
        case AMP_ENC_EVENT_CFA_STAT_READY:
        //    if (ycnt % 30 == 0 && Status == STATUS_LIVEVIEW)
        //    AmbaPrint("EncCB: AMP_ENC_EVENT_CFA_STAT_READY info: %X", info);
        //    ycnt++;
            if (VdspPrint) {
                 AMBA_DSP_EVENT_CFA_3A_DATA_s *cfa = (AMBA_DSP_EVENT_CFA_3A_DATA_s *)info;
                 static UINT32 LastVdspTime = 0;

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

            if (MVu2CH3AEnable) {
                AMBA_DSP_EVENT_CFA_3A_DATA_s *cfa = (AMBA_DSP_EVENT_CFA_3A_DATA_s *)info;
                if (cfa->Header.ChannelId == 0) {
                    Amba_Img_VDspCfa_Handler(hdlr, (UINT32 *)info);
                }
            }
            break;
        case AMP_ENC_EVENT_RGB_STAT_READY:
         //   AmbaPrint("EncCB: AMP_ENC_EVENT_RGB_STAT_READY info: %X", info);
            if (MVu2CH3AEnable)
               Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)info);
            break;
        #endif
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
            //AmbaPrint("AmpUT_MotorVu2CH: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("AmpUT_MotorVu2CH: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD !!!!!!!!!!");
            AmpUT_MotorVu2CH_EncStop();
            Status = STATUS_LIVEVIEW;
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("AmpUT_MotorVu2CH: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVERRUN !!!!!!!!!!");
            break;
        default:
            AmbaPrint("AmpUT_MotorVu2CH: Unknown %X info: %x", event, info);
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
static int AmpUT_MotorVu2CHPipeCallback(void *hdlr,UINT32 event, void *info)
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

/**
 * Display window initialization
 */
static int AmpUT_MotorVu2CH_DisplayInit(void)
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
            for (i=0; i<VOUT_MAIN_FOV_NUM; i++) {
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

            for (i=0; i<VOUT_MAIN_FOV_NUM; i++) {
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
        UINT8 i;
        if (LCDLiveview) {
            for (i=0; i<VOUT_MAIN_FOV_NUM; i++) {
                AmpDisplay_SetWindowActivateFlag(LCDWinHdlr[i], 1);
            }
            AmpDisplay_Update(LCDHdlr);
        } else {
            for (i=0; i<VOUT_MAIN_FOV_NUM; i++) {
                AmpDisplay_SetWindowActivateFlag(LCDWinHdlr[i], 0);
            }
            AmpDisplay_Update(LCDHdlr);
        }
    }
    if (TvLiveview) {
        UINT8 i;
        for (i=0; i<VOUT_MAIN_FOV_NUM; i++) {
            AmpDisplay_SetWindowActivateFlag(TVWinHdlr[i], 1);
        }

        AmpDisplay_Update(TVHdlr);
    } else {
        UINT8 i;
        for (i=0; i<VOUT_MAIN_FOV_NUM; i++) {
            AmpDisplay_SetWindowActivateFlag(TVWinHdlr[i], 0);
        }

        AmpDisplay_Update(TVHdlr);
    }
    DispStarted = 1;

    return 0;
}


int AmpUT_MotorVu2CH_CfaOut_Callback(AMBA_DSP_IMG_WARP_CALC_CFA_INFO_s *pCfaInfo)
{
    /* Read Only */
    AmbaPrint("Input window width       : %d", pCfaInfo->pInputWinDim->Width);
    AmbaPrint("Input window height      : %d", pCfaInfo->pInputWinDim->Height);
    AmbaPrint("Main window height       : %d", pCfaInfo->pMainWinDim->Width);
    AmbaPrint("Main window height       : %d", pCfaInfo->pMainWinDim->Height);

    pCfaInfo->pOutCfaWinDim->Width  = 1920; //TBD
    pCfaInfo->pOutCfaWinDim->Height = 1080; //TBD

    AmbaPrint("Cfa output window width  : %d", pCfaInfo->pOutCfaWinDim->Width);
    AmbaPrint("Cfa output window height : %d", pCfaInfo->pOutCfaWinDim->Height);

    return OK;
}

/**
 *  Unit Test Initialization
 */
int AmpUT_MotorVu2CH_Init(int sensorID, int LcdID)
{
    int Er;

    AmpCFS_GetFileParam(&cfsParam); // Get initial
    //imgproc initialization

    // Register LCD driver
    if (LcdID >= 0) {
        if (LcdID == 0) {
            AmbaPrint("Hook Wdf9648w LCD");
            AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_WdF9648wObj);
        } else {
            AmbaPrint("Hook Wdf9648w LCD");
            AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_WdF9648wObj);
        }
    }

    // Register sensor driver
    if (sensorID >= 0) {
        VinChannel.Bits.VinID = 0;
        VinChannel.Bits.SensorID = 0x1;

        if (sensorID == 0) {
            AmbaPrint("Hook IMX117 Sensor");

            AmbaSensor_Hook(VinChannel, &AmbaSensor_IMX117Obj);

            VideoEncMgt = VideoEncMgt117;
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
         AmpUT_MotorVu2CH_PriMuxTask, 0x0, MotorVuPriMuxStack, 6400, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("VideoEnc UnitTest: Muxer task creation failed");
    }
    if (AmbaKAL_TaskCreate(&MotorVuSecMuxTask, "Video Encoder UnitTest Secondary stream Muxing Task", 11, \
         AmpUT_MotorVu2CH_SecMuxTask, 0x0, MotorVuSecMuxStack, 6400, AMBA_KAL_AUTO_START) != OK) {
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
        AMBA_DSP_IMG_PIPE_e Pipe = AMBA_DSP_IMG_PIPE_VIDEO;

        AmbaImgSchdlr_GetInitDefaultCfg(&ISInitCfg);
        // For runtime enable/disable mainViewID, allocate MAX possible memory
        ISInitCfg.MainViewNum = 1;
        AmbaImgSchdlr_QueryMemsize(1, &ISPoolSize);
        ISInitCfg.MemoryPoolSize = ISPoolSize + ISInitCfg.MsgTaskStackSize;

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
 * Liveview start
 *
 * @param [in]
 *
 */
int AmpUT_MotorVu2CH_LiveviewStart(UINT32 modeIdx)
{
    EncModeIdx = modeIdx;


//#define KEEP_ALL_INSTANCES    // When defined, LiveviewStop won't delete all instances, so the next LiveviewStart is equal to change resolution.
#ifdef KEEP_ALL_INSTANCES
    if (MotorVuVinA) {
        AmpUT_MotorVu2CH_ChangeResolution(modeIdx);
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
        VinCfg.HwCaptureWindow.Width = SensorCaptureWindow[0].Width;
        VinCfg.HwCaptureWindow.Height = SensorCaptureWindow[0].Height;
        VinCfg.HwCaptureWindow.X = SensorCaptureWindow[0].X;
        VinCfg.HwCaptureWindow.Y = SensorCaptureWindow[0].Y;
        VinCfg.HwCaptureWindow.X = VinInfo.OutputInfo.RecordingPixels.StartX +
            (((VinInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
        VinCfg.HwCaptureWindow.Y = (VinInfo.OutputInfo.RecordingPixels.StartY +
            ((VinInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)) & 0xFFFE;

        Layout[0].Width = FovMainWindow[0].MainWidth;
        Layout[0].Height = FovMainWindow[0].MainHeight;
        Layout[0].EnableSourceArea = 0; // Get all capture window to main
        Layout[0].DzoomFactorX = 1<<16; // 16.16 format
        Layout[0].DzoomFactorY = 1<<16;
        Layout[0].DzoomOffsetX = 0;
        Layout[0].DzoomOffsetY = 0;
        Layout[1].Width = FovMainWindow[0].MainWidth;
        Layout[1].Height = FovMainWindow[0].MainHeight;
        Layout[1].EnableSourceArea = 0; // Get all capture window to main
        Layout[1].DzoomFactorX = 1<<16;
        Layout[1].DzoomFactorY = 1<<16;
        Layout[1].DzoomOffsetX = 0;
        Layout[1].DzoomOffsetY = 0;
        VinCfg.Layout = Layout;

        VinCfg.cbEvent = AmpUT_MotorVu2CHVinEventCallback;
        VinCfg.cbSwitch= AmpUT_MotorVu2CHVinSwitchCallback;

        AmpVin_Create(&VinCfg, &MotorVuVinA);
    }

    // Remember frame/field rate for muxers storing frame rate info
    EncFrameRate = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;


    // Create ImgSchdlr instance
    {
        AMBA_IMG_SCHDLR_CFG_s ISCfg;

        AmbaImgSchdlr_GetDefaultCfg(&ISCfg);

        ISCfg.MainViewID = 0;
        ISCfg.Channel.Bits.VinID = VinChannel.Bits.VinID;
        ISCfg.Channel.Bits.SensorID = VinChannel.Bits.SensorID;
        ISCfg.Vin = MotorVuVinA;
        ISCfg.cbEvent = AmpUT_MotorVu2CHImgSchdlrCallback;
        AmbaImgSchdlr_Create(&ISCfg, &ImgSchdlr);  // One MainViewID (not vin) need one scheduler.
    }

    // Create video encoder instances
    if (MotorVuPri == 0 && MotorVuSec == 0) {
        AMP_VIDEOENC_HDLR_CFG_s EncCfg;
        AMP_VIDEOENC_LAYER_DESC_s MultChanLayerCfg[TOTAL_MAIN_FOV_NUM];
#if 0
        AMP_VIDEOENC_H264_CFG_s *H264Cfg;
        AMP_VIDEOENC_MJPEG_CFG_s *MjpegCfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s HeaderInfo;
#endif
        memset(&EncCfg, 0x0, sizeof(AMP_VIDEOENC_HDLR_CFG_s));
        memset(MultChanLayerCfg, 0x0, sizeof(AMP_VIDEOENC_LAYER_DESC_s)*TOTAL_MAIN_FOV_NUM);
        EncCfg.MainLayout.Layer = MultChanLayerCfg;
        AmpVideoEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_MotorVu2CHCallback;

        if (1) {
            // Assign layout in multi chan
            EncCfg.EnableMultiView = 1;
            EncCfg.MainTimeScale = VideoEncMgt[EncModeIdx].TimeScale;
            EncCfg.MainTickPerPicture = VideoEncMgt[EncModeIdx].TickPerPicture;
            EncCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
            EncCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
            EncCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
            {
                UINT8 i, k, j;
                UINT16 MaxCapWidth = 0, MaxCapHeight = 0;
                EncCfg.MainLayout.Width = 0;
                EncCfg.MainLayout.Height = 0;
                EncCfg.MainLayout.LayerNumber = 0;

                for (i=0; i<TOTAL_MAIN_FOV_NUM; i++) {
                    if (FovMainWindow[i].Enable) {
                        MultChanLayerCfg[i].SourceType = AMP_ENC_SOURCE_VIN;
                        MultChanLayerCfg[i].Source = MotorVuVinA; //TBD, find VinID
                        MultChanLayerCfg[i].SourceLayoutId = FovMainWindow[i].MainViewID;
                        MultChanLayerCfg[i].EnableSourceArea = 1;
                        MultChanLayerCfg[i].EnableTargetArea = 1;
                        MultChanLayerCfg[i].SourceArea = FovMainWindow[i].MainCapWin;
                        MultChanLayerCfg[i].TargetArea.Width = FovMainWindow[i].MainWidth;
                        MultChanLayerCfg[i].TargetArea.Height = FovMainWindow[i].MainHeight;
                        if (MaxCapWidth < FovMainWindow[i].MainCapWin.X + FovMainWindow[i].MainCapWin.Width) {
                            MaxCapWidth = FovMainWindow[i].MainCapWin.X + FovMainWindow[i].MainCapWin.Width;
                        }
                        if (MaxCapHeight < FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].MainCapWin.Height) {
                            MaxCapHeight = FovMainWindow[i].MainCapWin.Y + FovMainWindow[i].MainCapWin.Height;
                        }
                        EncCfg.MainLayout.LayerNumber++;
                        // Loop to see if the main window matches any encode stream
                        for(k = 0; k<TOTAL_ENCODE_STREAM_NUM; k++ ) {
                            for (j = 0; j<ENCODE_FOV_PER_STREAM_NUM; j++) {
                                if (FovEncodeWindow[k][j].Enable && FovEncodeWindow[k][j].MainWindowView == FovMainWindow[i].MainViewID) {
                                    memcpy(&MultChanLayerCfg[i].MultiChanEncSrcArea, &FovEncodeWindow[k][j].SrcWin, sizeof(AMP_AREA_s));
                                    memcpy(&MultChanLayerCfg[i].MultiChanEncDestArea, &FovEncodeWindow[k][j].DestWin, sizeof(AMP_AREA_s));
                                    MultChanLayerCfg[i].SourceRotate = FovEncodeWindow[k][j].Rotate;
                                    MultChanLayerCfg[i].MultiChanEncBlendTable = FovEncodeWindow[k][j].BlendTable;
                                }
                            }
                        }
                    }
                }
                EncCfg.MainLayout.Width = MaxCapWidth;
                EncCfg.MainLayout.Height = MaxCapHeight;
            }


            EncCfg.EventDataReadySkipNum = 0;  // File data ready every frame
            EncCfg.StreamId = AMP_VIDEOENC_STREAM_PRIMARY;
        }

        {
            UINT8 *dspWorkAddr;
            UINT32 dspWorkSize;
            AmpUT_MotorVu2CH_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
            EncCfg.DspWorkBufAddr = dspWorkAddr;
            EncCfg.DspWorkBufSize = dspWorkSize;
        }

       // Create primary stream handler
        AmpVideoEnc_Create(&EncCfg, &MotorVuPri); // Don't have to worry about h.264 spec settings when liveview

        // Assign Secondary main layout
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
    AmpUT_MotorVu2CH_DisplayInit();

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
        pipeCfg.cbEvent = AmpUT_MotorVu2CHPipeCallback;
        pipeCfg.type = AMP_ENC_AV_PIPE;
        AmpEnc_Create(&pipeCfg, &VideoEncPipe);

        AmpEnc_Add(VideoEncPipe);
    }

    AmpEnc_StartLiveview(VideoEncPipe, AMP_ENC_FUNC_FLAG_WAIT);

    return 0;
}

int AmpUT_MotorVu2CH_EncodeStart()
{
    AMP_FIFO_CFG_s fifoDefCfg;

    if (VirtualPriFifoHdlr == NULL) {
        // create a virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = MotorVuPri;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = 1024;
        fifoDefCfg.cbEvent = AmpUT_MotorVu2CH_FifoCB;
        AmpFifo_Create(&fifoDefCfg, &VirtualPriFifoHdlr);
    }
    if (VirtualSecFifoHdlr == NULL) {
        // create a virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = MotorVuSec;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = 1024;
        fifoDefCfg.cbEvent = AmpUT_MotorVu2CH_FifoCB;
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
                UINT8 i;
                UINT16 MaxEncWidth = 0, MaxEncHeight = 0;

                for (i=0; i<ENCODE_MAIN_FOV_NUM; i++) {
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
                // Get default SPS/VUI
                AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
                H264Cfg->SPS.level_idc = 42;
                H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.
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
int AmpUT_MotorVu2CH_EncStop()
{
    AmpEnc_StopRecord(VideoEncPipe, 0);
    return 0;
}

int AmpUT_MotorVu2CH_LiveviewStop()
{

    Amba_Img_VIn_Invalid(MotorVuPri, (UINT32 *)NULL);
    if (ImgSchdlr) {
        AmbaImgSchdlr_Enable(ImgSchdlr, 0);
    }

    AmpEnc_StopLiveview(VideoEncPipe, AMP_ENC_FUNC_FLAG_WAIT);

#ifndef KEEP_ALL_INSTANCES
    AmpEnc_Delete(VideoEncPipe); VideoEncPipe = NULL;
    AmpVin_Delete(MotorVuVinA); MotorVuVinA = NULL;
    if (ImgSchdlr) {
        AmbaImgSchdlr_Delete(ImgSchdlr);
    }
    AmpVideoEnc_Delete(MotorVuPri); MotorVuPri = NULL;
    AmpVideoEnc_Delete(MotorVuSec); MotorVuSec = NULL;

#endif

    return 0;
}


int AmpUT_MotorVu2CH_EncPause()
{

    if (Status == STATUS_ENCODE) {
        AmpEnc_PauseRecord(VideoEncPipe, 0);
        Status = STATUS_PAUSE;
    }
    return 0;
}

int AmpUT_MotorVu2CH_EncResume()
{

    if (Status == STATUS_PAUSE) {
        AmpEnc_ResumeRecord(VideoEncPipe, 0);
        Status = STATUS_ENCODE;
    }

    return 0;
}


/**
 * UnitTest: Calculate dsp working address and size base on current capture mode
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_MotorVu2CH_DspWork_Calculate(UINT8 **addr, UINT32 *size)
{
    extern UINT8 *DspWorkAreaResvStart;
    extern UINT32 DspWorkAreaResvSize;

    (*addr) = DspWorkAreaResvStart;
    (*size) = DspWorkAreaResvSize - 15*1024*1024;

    AmbaPrint("[DspWork_Calculate] Addr 0x%X, Sz %u", *addr, *size);
    return 0;
}

int AmpUT_MotorVu2CHTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_MotorVu2CHTest cmd: %s", argv[1]);
    if (strcmp(argv[1],"init") == 0) {
        int ss = 0, dd= 0;

        if (Status != STATUS_IDLE) {
            return -1;
        }

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc init [sensorID][LcdID]");
            AmbaPrint("               sensorID: 0 -- IMX117");
            AmbaPrint("               LcdID: 0 -- WDF4648W");
            return 0;
        }
        ss = atoi(argv[2]);
        if (argc == 4) {
            dd = atoi(argv[3]);
        }
        AmpUT_MotorVu2CH_Init(ss, dd);
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
            AmbaPrint("IMX117: ");
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

        AmpUT_MotorVu2CH_LiveviewStart(tt);
        Status = STATUS_LIVEVIEW;
    } else if ((strcmp(argv[1],"encstart") == 0) || (strcmp(argv[1],"enst") == 0)) {
        UINT32 tt = atoi(argv[2]);

        if (Status != STATUS_LIVEVIEW)
            return -1;

        AmbaPrint("Encode for %d milliseconds", tt);
        AmpUT_MotorVu2CH_EncodeStart();
        Status = STATUS_ENCODE;

        if (tt != 0) {
            while (tt) {
                AmbaKAL_TaskSleep(1);
                tt--;
                if (tt == 0) {
                    AmpUT_MotorVu2CH_EncStop();
                    Status = STATUS_LIVEVIEW;
                }
                if (Status == STATUS_LIVEVIEW)
                    tt = 0;
            }
        }

    } else if ((strcmp(argv[1],"encstop") == 0) || (strcmp(argv[1],"ensp") == 0)) {
        if (Status != STATUS_ENCODE && Status != STATUS_PAUSE)
            return -1;

        AmpUT_MotorVu2CH_EncStop();
        Status = STATUS_LIVEVIEW;
    } else if ((strcmp(argv[1],"pause") == 0)) {
        AmpUT_MotorVu2CH_EncPause();
    } else if ((strcmp(argv[1],"resume") == 0)) {
        AmpUT_MotorVu2CH_EncResume();
    } else if ((strcmp(argv[1],"liveviewstop") == 0) || (strcmp(argv[1],"lvsp") == 0)) {
        if (Status != STATUS_LIVEVIEW)
            return -1;
        AmpUT_MotorVu2CH_LiveviewStop();
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
            MVu2CH3AEnable = 0;
            AaaOpInfo.AeOp = DISABLE;
            AaaOpInfo.AfOp = DISABLE;
            AaaOpInfo.AwbOp = DISABLE;
            AaaOpInfo.AdjOp = DISABLE;
            AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
        } else if (Is3aEnable == 1) {
            MVu2CH3AEnable = 1;
            AaaOpInfo.AeOp = ENABLE;
            AaaOpInfo.AfOp = ENABLE;
            AaaOpInfo.AwbOp = ENABLE;
            AaaOpInfo.AdjOp = ENABLE;
            AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
        }
        AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
        AmbaPrint("3A %s: ae:%u af:%u awb:%u adj:%u", MVu2CH3AEnable?"Enable":"Disable", AaaOpInfo.AeOp, AaaOpInfo.AfOp, \
            AaaOpInfo.AwbOp, AaaOpInfo.AdjOp);
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
        AmbaPrint("       spec: Select bitstream specification");
    }
    return 0;
}

int AmpUT_MotorVu2CHTestAdd(void)
{
    AmbaPrint("Adding AmpUT_MotorVu2CH %s %d",__FUNCTION__, __LINE__);
    // hook command
    AmbaTest_RegisterCommand("m2", AmpUT_MotorVu2CHTest);

   // AmpUT_MotorVu2CH_Init(0);
   // AmpUT_MotorVu2CH_LiveviewStart();

    return AMP_OK;
}
