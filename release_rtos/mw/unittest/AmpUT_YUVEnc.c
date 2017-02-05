/**
  * @file src/app/sample/unittest/AmpUT_YUVEnc.c
  *
  * transcoder encode unit test
  *
  * History:
  *    2014/10/29 - [Wisdom Hung] created file
  *
  * Copyright (C) 2014, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include "AmpUnitTest.h"
#include "AmpUT_Input.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vin/vin.h>
#include <recorder/Encode.h>
#include <recorder/VideoEnc.h>
#include <display/Osd.h>
#include <display/Display.h>
#include <util.h>
#include "AmbaPLL.h"
#include "AmbaUtility.h"
#include <cfs/AmpCfs.h>
#include <AmbaLCD.h>
#include <AmbaHDMI.h>
#include "AmpUT_Display.h"
#include "AmbaDSP_EventInfo.h"


static char DefaultSlot[] = "C";


static AMP_CFS_FILE_PARAM_s cfsParam;

void *UT_YUVEncodefopen(const char *pFileName, const char *pMode, BOOL8 AsyncMode)
{
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
        //strcpy(cfsParam.Filename, pFileName);
        //return (void *)AmpCFS_fopen(&cfsParam);
        AmbaPrint("[%s]: Unicode should be abandoned", __func__);
        return NULL;
    } else {
        strcpy((char *)cfsParam.Filename, pFileName);
        return (void *)AmpCFS_fopen(&cfsParam);
    }
}

int UT_YUVEncodefclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_YUVEncodefwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_YUVEncodefsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
}



static AMP_ENC_PIPE_HDLR_s *EncPipe = NULL;  // Encode pipeline  instance

#define STATUS_YUV_NONE     1
#define STATUS_YUV_IDLE     2
#define STATUS_YUV_LIVEVIEW 3
#define STATUS_YUV_ENCODE   4
#define STATUS_YUV_PAUSE    5

static UINT8 Status = STATUS_YUV_NONE;

static AMP_CFS_FILE_PARAM_s cfsParam;
static AMP_CFS_FILE_s *outputPriFile = NULL;        // Output file pointer for primary stream
static AMP_CFS_FILE_s *outputPriIdxFile = NULL;     // Output file pointer for primary stream index (frame offset/size...etc)
static AMP_CFS_FILE_s *UdtaPriFile = NULL;          // Output file pointer for primary stream UserData

// Global var for VideoEnc codec
static AMP_VIN_HDLR_s *VideoEncVinA = NULL;       // Vin instance
static AMP_AVENC_HDLR_s *VideoEncPri = NULL;      // Primary VideoEnc codec instance
//static AMBA_IMG_SCHDLR_HDLR_s *ImgSchdlr = NULL;    // Image scheduler instance
static UINT8 *VinWorkBuffer = NULL;               // Vin module working buffer
static UINT8 *VEncWorkBuffer = NULL;              // VideoEnc working buffer
//static UINT8 *ImgSchdlrWorkBuffer = NULL;         // Img scheduler working buffer
static AMBA_DSP_CHANNEL_ID_u VinChannel;          // Vin channel

#ifdef CONFIG_SOC_A12
extern AMBA_YUV_OBJ_s AmbaYuv_TI5150Obj;
#endif
extern AMBA_LCD_OBJECT_s AmbaLCD_WdF9648wObj;

static UINT8 TvLiveview = 1;                        // Enable TV liveview or not
static UINT8 LCDLiveview = 1;                       // Enable LCD liveview or not
static UINT8 EncPriSpecH264 = 1;                    // Primary codec instance output spec. 1 = H264, 0 = MJPEG
static UINT8 EncPriInteralce = 0;                   // Primary codec instance output type. 0 = Progressive, 1 = InterlYUVe
//static UINT8 LiveViewProcMode = 0;                  // LiveView Process Mode, 0: Express 1:Hybrid
//static UINT8 LiveViewAlgoMode = 0;                  // LiveView Algo Mode in HybridMode, 0: Fast 1:Liso 2: Hiso
//static UINT8 LiveViewOSMode = 1;                    // LiveView OverSampling Mode in ExpressMode, 0: Disable 1:Enable
static UINT8 DspSuspendEnable = 0;
static UINT8 WirelessModeEnable = 0;

#if (CONFIG_SOC_A12 && CONFIG_YUVINPUT_TI5150)
extern INPUT_ENC_MGT_s VideoEncMgtTI5150[];
#endif

static INPUT_ENC_MGT_s *YuvEncMgt;         // Pointer to above tables
static UINT8 YuvEncModeIdx = 1;        // Current mode index

#define GOP_N   8                   // I-frame distance
#define GOP_IDR 8                   // IDR-frame distance


/* Simple muxer to communicate with FIFO */
#define BITSFIFO_SIZE 32*1024*1024
static UINT8 *H264BitsBuf;          // H.264 bitstream buffer
static UINT8 *MjpgBitsBuf;          // MJPEG bitstream buffer
#ifdef CONFIG_SOC_A9
#define BISFIFO_STILL_SIZE 10*1024*1024
static UINT8 *JpegBitsBuf;          // JPEG bitstream buffer
static UINT8 *JpegDescBuf;          // JPEG descriptor buffer (physical)
#endif
#define DESC_SIZE 40*3000
static UINT8 *H264DescBuf;          // H.264 descriptor buffer (physical)
static UINT8 *MjpgDescBuf;          // MJPEG descriptor buffer (physical)
static AMBA_KAL_TASK_t VideoEncPriMuxTask = {0};    // Primary stream muxer task
static UINT8 *VideoEncPriMuxStack = NULL;           // Stack for primary stream muxer task
#define YUV_ENC_PRI_MUX_TASK_STACK_SIZE (8192)
static AMBA_KAL_SEM_t VideoEncPriSem = {0};         // Counting semaphore for primary stream muxer task and fifo callback

static int fnoPri = 0;                  // Frame number counter for primary stream muxer
static UINT32 EncFrameRate = 0;         // Remember current framerate for primary muxer to calculate actual bitrate
static UINT64 encPriBytesTotal = 0;     // total bytes primary stream muxer received
static UINT32 encPriTotalFrames = 0;    // total frames primary stream muxer received
static AMP_FIFO_HDLR_s *VirtualPriFifoHdlr = NULL;  // Primary stream vitrual fifo
static UINT8 LogMuxer = 0;

static UINT32 BrcFrameCount = 0;
static UINT32 BrcByteCount = 0;

/** UT function prototype */
int AmpUT_YUVEnc_EncStop(void);
int AmpUT_YUVEnc_LiveviewStart(UINT32 modeIdx);
int AmpUT_YUVEnc_EncodeStart(void);
int AmpUT_YUVEnc_EncPause(void);
int AmpUT_YUVEnc_EncResume(void);
int AmpUT_YUVEnc_Delete(void);

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

UINT32 YuvEnc_find_jpeg_segment(UINT8* img, UINT32 size, UINT16 marker)
{
    UINT8* ptr = img;
    UINT8  marker_suffix = ((UINT8) marker & 0x00ff);
    UINT8* limit = img + size;
    //UINT32 length;
    while( ptr < limit){
        if( *ptr != 0xFF ){ // search for marker prefix
            ptr++;
        }
        else{
            ptr++;

            if(*ptr==marker_suffix)
                return (UINT32)(ptr-1);  // 1: marker prefix
            else if (*ptr == 0xD8){ // skip getting length since SOI has no length info.
                ptr++;
                continue;
            }
            /*
            else{
                length = (*(ptr+1)<<8) + *(ptr+2); // combine and get 16 bits length info.
                ptr += length ;
            }
            */
        }
    }
    AmbaPrint("MJPEG bitstream not found %x",marker);
    return 0xffffffff;
}

void AmpUT_YUVEnc_VPriMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc;
    int Er;
    UINT8 *BitsLimit=NULL;
    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_VideoEnc_PriMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&VideoEncPriSem, 10000);  // Check if there is any pending frame to be muxed
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

            sprintf(Fn,"%s:\\OUT_%04d.%s", DefaultSlot, fnoPri,EncPriSpecH264?"h264":"mjpg");
            outputPriFile = UT_YUVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
            while (outputPriFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                outputPriFile = UT_YUVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
            }
            AmbaPrint("%s opened", Fn);

            sprintf(Fn,"%s:\\OUT_%04d.nhnt", DefaultSlot, fnoPri);
            outputPriIdxFile = UT_YUVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
            while (outputPriIdxFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                outputPriIdxFile = UT_YUVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
            }
            AmbaPrint("%s opened", Fn);

            sprintf(Fn,"%s:\\OUT_%04d.udta", DefaultSlot, fnoPri);
            UdtaPriFile = UT_YUVEncodefopen((const char *)Fn, (const char *)mdASCII,0);
            while (UdtaPriFile==0) {
                AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                AmbaKAL_TaskSleep(10);
                UdtaPriFile = UT_YUVEncodefopen((const char *)Fn, (const char *)mdASCII,0);
            }

            AmbaPrint("%s opened", Fn);

            Udta.nIdrInterval = GOP_IDR/GOP_N;
            Udta.nTimeScale = YuvEncMgt[YuvEncModeIdx].TimeScale;
            Udta.nTickPerPicture = YuvEncMgt[YuvEncModeIdx].TickPerPicture;
            Udta.nN = GOP_N;
            Udta.nM = YuvEncMgt[YuvEncModeIdx].GopM;
            Udta.nVideoWidth = YuvEncMgt[YuvEncModeIdx].MainWidth;
            Udta.nVideoHeight = YuvEncMgt[YuvEncModeIdx].MainHeight;
            Udta.nInterlaced = YuvEncMgt[YuvEncModeIdx].Interlace;
            UT_YUVEncodefwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)UdtaPriFile);
            UT_YUVEncodefclose((void *)UdtaPriFile);

            NhntHeader.Signature[0]='n';
            NhntHeader.Signature[1]='h';
            NhntHeader.Signature[2]='n';
            NhntHeader.Signature[3]='t';
            NhntHeader.TimeStampResolution = YuvEncMgt[YuvEncModeIdx].TimeScale;
            UT_YUVEncodefwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)outputPriIdxFile);

            if (EncPriSpecH264) {
                BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
            } else {
                BitsLimit = MjpgBitsBuf + BITSFIFO_SIZE - 1;
            }
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
        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 Avg;
            // EOS
        #ifdef AMPUT_FILE_DUMP
                if (outputPriFile) {
                    UT_YUVEncodefsync(outputPriFile);
                    UT_YUVEncodefclose(outputPriFile);
                    fnoPri++;
                    UT_YUVEncodefsync(outputPriIdxFile);
                    UT_YUVEncodefclose(outputPriIdxFile);
                    outputPriFile = NULL;
                    outputPriIdxFile = NULL;
                    FileOffset = 0;
                }
        #endif

            Avg = (UINT32)(encPriBytesTotal*8.0*EncFrameRate/encPriTotalFrames/1E3);

            AmbaPrint("Video Primary Muxer met EOS, total %d frames/fields", encPriTotalFrames);
            AmbaPrint("Primary Bitrate Average: %d kbps\n", Avg);


            encPriBytesTotal = 0;
            encPriTotalFrames = 0;
        } else if (Desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
            //Do Nothing
        } else {
        #ifdef AMPUT_FILE_DUMP
                if (outputPriFile) {
                    NhntSample.CompositionTimeStamp = Desc->Pts;
                    NhntSample.DecodingTimeStamp = Desc->Pts;
                    NhntSample.DataSize = Desc->Size;
                    NhntSample.FileOffset = FileOffset;
                    FileOffset += Desc->Size;
                    NhntSample.FrameType = Desc->Type;

                    UT_YUVEncodefwrite(&NhntSample, 1, sizeof(NhntSample), outputPriIdxFile);

                    if (EncPriSpecH264==0 && YuvEncMgt[YuvEncModeIdx].MainHeight==1080) {
                        UINT8 *Mjpeg_bs=0;
                        UINT32 sof_addr=0;
                        UINT8 *sof_ptr=0;

                        if (EncPriSpecH264) {
                            BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
                        } else {
                            BitsLimit = MjpgBitsBuf + BITSFIFO_SIZE - 1;
                        }

                        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&Mjpeg_bs, ALIGN_32(Desc->Size), 100);
                        if (Er != OK) {
                            if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                               // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                                UT_YUVEncodefwrite(Desc->StartAddr, 1, Desc->Size, outputPriFile);
                            } else {
                                // AmbaCache_Invalidate(Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                UT_YUVEncodefwrite(Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, outputPriFile);
                                // AmbaCache_Invalidate(H264BitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                                UT_YUVEncodefwrite(MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), outputPriFile);
                            }
                            //AmbaPrint("Out of cached memory for MJPEG header change flow!!");
                        } else {
                            if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                                memcpy(Mjpeg_bs, Desc->StartAddr, Desc->Size);
                            } else {
                                UINT8 *Bs = Mjpeg_bs;
                                memcpy(Mjpeg_bs, Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                Bs += BitsLimit - Desc->StartAddr + 1;
                                memcpy(Bs, MjpgBitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                            }
                            sof_addr = YuvEnc_find_jpeg_segment(Mjpeg_bs,Desc->Size, 0xFFC0);
                            sof_ptr = (UINT8*)(sof_addr + 5);
                            (*sof_ptr)    =  (UINT8)(YuvEncMgt[YuvEncModeIdx].MainHeight >> 8);
                            *(sof_ptr+1)  = (UINT8)(YuvEncMgt[YuvEncModeIdx].MainHeight &  0xFF);

                            UT_YUVEncodefwrite(Mjpeg_bs, 1, Desc->Size, outputPriFile);

                            if (AmbaKAL_BytePoolFree((void *)Mjpeg_bs) != OK) {
                                AmbaPrint("cached memory release fail for MJPEG header change flow");
                            }
                        }

                    } else {
                 //       AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                        if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                           // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                           UT_YUVEncodefwrite(Desc->StartAddr, 1, Desc->Size, outputPriFile);
                        } else {
                            UT_YUVEncodefwrite(Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, outputPriFile);
                            if (EncPriSpecH264) {
                                UT_YUVEncodefwrite(H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), outputPriFile);
                            } else {
                                UT_YUVEncodefwrite(MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), outputPriFile);
                            }
                        }
                    }
                }
        #else
               AmbaKAL_TaskSleep(1);    // Do nothing
        #endif

// Bitrate statistics
//            if (((BrcFrameCount%EncFrameRate) == 0)&& ((BrcFrameCount/EncFrameRate)%3 == 0) && BrcFrameCount) {
//                AmbaPrint("Pri YUVgBitRate = %fMbps, Target %dMbps",(BrcByteCount*8.0*EncFrameRate/BrcFrameCount/1E6),YuvEncMgt[YuvEncModeIdx].YUVerageBitRate);
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
 * Display window initialization
 */
static int AmpUT_YUVEnc_DisplayStart(void)
{
    AMP_DISP_WINDOW_CFG_s window;

    /** Step 1: Display config & window config */
//    if (AmpUT_Display_Init() == NG) {
//        return NG;
//    }

    // Creat LCD Window
    if (1) {
        memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
        window.Source = AMP_DISP_ENC;
        window.CropArea.Width = 0;
        window.CropArea.Height = 0;
        window.CropArea.X = 0;
        window.CropArea.Y = 0;
        window.TargetAreaOnPlane.Width = 960;
        window.TargetAreaOnPlane.Height = (YuvEncMgt[YuvEncModeIdx].AspectRatio == VAR_16x9)? 360: 480;
        window.TargetAreaOnPlane.X = 0;
        window.TargetAreaOnPlane.Y = (480-window.TargetAreaOnPlane.Height)/2;
        window.SourceDesc.Enc.VinCh = VinChannel;
        if (AmpUT_Display_Window_Create(0, &window) == NG) {
            return NG;
        }
    }

    // Creat TV Window
    if (1) {
        memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
        window.Source = AMP_DISP_ENC;
        window.CropArea.Width = 0;
        window.CropArea.Height = 0;
        window.CropArea.X = 0;
        window.CropArea.Y = 0;
        window.TargetAreaOnPlane.Width = 1920;
        window.TargetAreaOnPlane.Height = 1080;//  interlance should be consider in MW
        window.TargetAreaOnPlane.X = 0;
        window.TargetAreaOnPlane.Y = 0;
        window.SourceDesc.Enc.VinCh = VinChannel;
        if (AmpUT_Display_Window_Create(1, &window) == NG) {
            return NG;
        }
    }

    if (1) {
        /** Step 2: Setup device */
        // Setup LCD & TV
        if (LCDLiveview) {
            AmpUT_Display_Start(0);
        } else {
            AmpUT_Display_Stop(0);
        }
        if (TvLiveview) {
            AmpUT_Display_Start(1);
        } else {
            AmpUT_Display_Stop(1);
        }
    }

    // Active Window 7002 cmd
    if (LCDLiveview) {
        AmpUT_Display_Act_Window(0);
    } else {
        AmpUT_Display_DeAct_Window(0);
    }
    if (TvLiveview) {
        AmpUT_Display_Act_Window(1);
    } else {
        AmpUT_Display_DeAct_Window(1);
    }

    return 0;
}


static int AmpUT_YUVEnc_FifoCB(void *hdlr, UINT32 event, void* info)
{
    UINT32 *numFrames = info;
    AMBA_KAL_SEM_t *pSem;

    pSem = &VideoEncPriSem;

    //AmbaPrint("AmpUT_AudioEnc_FifoCB on Event: 0x%x 0x%x", event, *numFrames);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        int i;

        for(i = 0; i < *numFrames; i++) {
            AmbaKAL_SemGive(pSem);
        }
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
            AmbaKAL_SemGive(pSem);
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
static int AmpUT_YUVEncPipeCallback(void *hdlr,UINT32 event, void *info)
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
 * VIN sensor mode switch callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_YUVEncVinSwitchCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {
        case AMP_VIN_EVENT_INVALID:
            break;
        case AMP_VIN_EVENT_VALID:
            break;
        case AMP_VIN_EVENT_CHANGED_PRIOR:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_PRIOR info: %X", info);
            break;
        case AMP_VIN_EVENT_CHANGED_POST:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_POST info: %X", info);
            if (Status == STATUS_YUV_LIVEVIEW) { // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window = {0}, DefWindow = {0};

                if (1) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;
                    window.TargetAreaOnPlane.Width = 960;
                    window.TargetAreaOnPlane.Height = (YuvEncMgt[YuvEncModeIdx].AspectRatio == VAR_16x9)? 360: 480;
                    window.TargetAreaOnPlane.X = 0;
                    window.TargetAreaOnPlane.Y = (480 - window.TargetAreaOnPlane.Height)/2;
                    if(AmpUT_Display_GetWindowCfg(0, &DefWindow) != OK) {
                        AmpUT_Display_Window_Create(0, &window);
                    } else {
                        AmpUT_Display_SetWindowCfg(0, &window);
                    }

                    if (LCDLiveview) {
                        AmpUT_Display_Act_Window(0);
                    } else {
                        AmpUT_Display_DeAct_Window(0);
                    }
                }
            }

            if (Status == STATUS_YUV_LIVEVIEW) {
                AMP_DISP_WINDOW_CFG_s window = {0}, DefWindow = {0};

                if (1) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;
                    window.TargetAreaOnPlane.Width = (YuvEncMgt[YuvEncModeIdx].AspectRatio == VAR_16x9)? 1920: 1440;
                    window.TargetAreaOnPlane.Height = 1080;
                    window.TargetAreaOnPlane.X = (1920 - window.TargetAreaOnPlane.Width)/2;
                    window.TargetAreaOnPlane.Y = 0;
                    if(AmpUT_Display_GetWindowCfg(1, &DefWindow) != OK) {
                        AmpUT_Display_Window_Create(1, &window);
                    } else {
                        AmpUT_Display_SetWindowCfg(1, &window);
                    }

                    if (TvLiveview) {
                        AmpUT_Display_Act_Window(1);
                    } else {
                        AmpUT_Display_DeAct_Window(1);
                    }
                }
            }
            break;
        case AMP_VIN_EVENT_MAINVIEW_CHANGED_PRIOR:
            {
                //disable ImgSchdlr
            }
            break;
        case AMP_VIN_EVENT_MAINVIEW_CHANGED_POST:
            {
                //enable ImgSchdlr
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
static int AmpUT_YUVEncVinEventCallback(void *hdlr,UINT32 event, void *info)
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
 * Generic YUVEnc codec callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_YUVEncCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
            break;
        case AMP_ENC_EVENT_RAW_CAPTURE_DONE:
            // Next raw capture is allowed from now on
            //StillRawCaptureRunning = 0;
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
        case AMP_ENC_EVENT_VCAP_2ND_YUV_READY:
            //AmbaPrint("AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD !!!!!!!!!!");
            AmpUT_YUVEnc_EncStop();
            //EncDateTimeStampPri = EncDateTimeStampSec = 0;
            Status = STATUS_YUV_LIVEVIEW;
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DESC_OVERRUN:
            AmbaPrint("AmpUT_YUVEnc: !!!!!!!!!!! AMP_ENC_EVENT_DESC_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_ILLEGAL_SIGNAL:
            AmbaPrint("AmpUT_YUVEnc: !!!!!!!!!!! AMP_ENC_EVENT_ILLEGAL_SIGNAL !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_FULLNESS_NOTIFY:
            if (0) {
                UINT32 *Percetage = (UINT32 *) info;
                AmbaPrint("AmpUT_YUVEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_FULLNESS_NOTIFY, Percetage=%d!!!!!!!!!!",Percetage);
            }
            break;
        default:
            AmbaPrint("AmpUT_VideoEnc: Unknown %X info: %x", event, info);
            break;
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
UINT32 AmpUT_YUVEnc_DspWork_Calculate(UINT8 **addr, UINT32 *size)
{
    extern UINT8 *DspWorkAreaResvStart;
    extern UINT32 DspWorkAreaResvSize;


    //only allocate MJPEG Bits
    (*addr) = DspWorkAreaResvStart;
    (*size) = DspWorkAreaResvSize - 15*1024*1024;

    AmbaPrint("[DspWork_Calculate] Addr 0x%X, Sz %u", *addr, *size);
    return 0;
}


/**
 *  Unit Test Initialization
 */
int AmpUT_YUVEnc_Init(int YUV_ID, int LcdID)
{
    int er;
    void *TmpbufRaw = NULL;
//    if (LcdID >= 0) {
//        if (LcdID == 0) {
//            AmbaPrint("Hook Wdf9648w LCD");
//            AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_WdF9648wObj);
//        } else {
//            AmbaPrint("Hook Wdf9648w LCD");
//            AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_WdF9648wObj);
//        }
//    }
    // Video side
    if (YUV_ID >= 0) {
        VinChannel.Bits.VinID = 0;
        VinChannel.Bits.SensorID = 0x1;
        YuvEncMgt = RegisterMWUT_YUV_Driver(VinChannel,YUV_ID);
    }

    // Create semaphores for muxers
    if (AmbaKAL_SemCreate(&VideoEncPriSem, 0) != OK) {
        AmbaPrint("VideoEnc UnitTest: Semaphore creation failed");
    }

    // Prepare stacks for muxer tasks
    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoEncPriMuxStack, (void **)&TmpbufRaw, YUV_ENC_PRI_MUX_TASK_STACK_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    // Create muxer tasks
    if (AmbaKAL_TaskCreate(&VideoEncPriMuxTask, "YUV Encoder UnitTest Primary Muxing Task", 11, \
         AmpUT_YUVEnc_VPriMuxTask, 0x0, VideoEncPriMuxStack, YUV_ENC_PRI_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("YUVEnc UnitTest: Muxer task creation failed");
    }

    // Initialize VIN module
    {
        AMP_VIN_INIT_CFG_s vinInitCfg;

        AmpVin_GetInitDefaultCfg(&vinInitCfg);

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VinWorkBuffer, (void **)&TmpbufRaw, vinInitCfg.MemoryPoolSize, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for vin!!");
        }
        vinInitCfg.MemoryPoolAddr = VinWorkBuffer;
        AmpVin_Init(&vinInitCfg);
    }

    // Initialize VIDEOENC module
    {
        AMP_VIDEOENC_INIT_CFG_s encInitCfg;

        AmpVideoEnc_GetInitDefaultCfg(&encInitCfg);

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VEncWorkBuffer, (void **)&TmpbufRaw, encInitCfg.MemoryPoolSize, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for enc!!");
        }
        encInitCfg.MemoryPoolAddr = VEncWorkBuffer;
        AmpVideoEnc_Init(&encInitCfg);
    }

    // Allocate bitstream buffers
    {
#ifdef CONFIG_SOC_A9
        extern UINT8 *DspWorkAreaResvLimit;
#endif


        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&H264BitsBuf, (void **)&TmpbufRaw, BITSFIFO_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&H264DescBuf, (void **)&TmpbufRaw, DESC_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }

#ifdef CONFIG_SOC_A12
        MjpgDescBuf = H264DescBuf;
        MjpgBitsBuf = H264BitsBuf;
#else
        // This is an example how to use DSP working memory when APP knows these memory area is not used.
        // We steal 15MB here
        MjpgDescBuf = DspWorkAreaResvLimit + 1 - 1*1024*1024;
        MjpgBitsBuf = MjpgDescBuf - BITSFIFO_SIZE;
#endif

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

    if (AmpUT_Display_Init() == NG) {
        return NG;
    }

    return 0;
}

/**
 * Liveview start
 *
 * @param [in]
 *
 */
int AmpUT_YUVEnc_LiveviewStart(UINT32 modeIdx)
{
    AMBA_YUV_MODE_INFO_s YuvModeInfo;
    YuvEncModeIdx = modeIdx;

//#define KEEP_ALL_INSTANCES    // When defined, LiveviewStop won't delete all instances, so the next LiveviewStart is equal to change resolution.

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" AmbaUnitTest: Liveview at %s", YuvEncMgt[YuvEncModeIdx].Name);
    AmbaPrint(" =========================================================");
    // Create Vin instance
    if (VideoEncVinA == 0) {
        AMP_VIN_HDLR_CFG_s VinCfg;
        AMP_VIN_LAYOUT_CFG_s Layout[2]; // Dualstream from same vin/vcapwindow
        AMBA_YUV_MODE_ID_u YuvMode;
        UINT16 Frate = YuvEncMgt[YuvEncModeIdx].TimeScale/YuvEncMgt[YuvEncModeIdx].TickPerPicture;

        memset(&VinCfg, 0x0, sizeof(AMP_VIN_HDLR_CFG_s));
        memset(&Layout, 0x0, sizeof(AMP_VIN_LAYOUT_CFG_s)*2);
        memset(&YuvMode, 0x0, sizeof(AMBA_YUV_MODE_ID_u));
        if ((Frate%25==0)) {
            YuvMode.Bits.VideoStandard = AMBA_YUV_PAL;
        }else {
            YuvMode.Bits.VideoStandard = AMBA_YUV_NTSC;
        }
        YuvMode.Bits.ScanMethod = AMBA_YUV_INTERLACED;
        YuvMode.Bits.InputSource = AMBA_YUV_CVBS;
        AmbaYuv_GetModeInfo(VinChannel, YuvMode, &YuvModeInfo);

        AmpVin_GetDefaultCfg(&VinCfg);
        VinCfg.VinSource = AMP_VIN_EXTERNAL_YUV;

        VinCfg.Channel = VinChannel;
        VinCfg.YuvMode = YuvMode;
        VinCfg.LayoutNumber = 1;
        VinCfg.HwCaptureWindow.Width = YuvEncMgt[YuvEncModeIdx].CaptureWidth;
        VinCfg.HwCaptureWindow.Height = YuvEncMgt[YuvEncModeIdx].CaptureHeight;
        EncPriInteralce = YuvModeInfo.OutputInfo.FrameRate.Interlace;
        if (YuvModeInfo.OutputInfo.FrameRate.Interlace) {
            VinCfg.HwCaptureWindow.Height = YuvEncMgt[YuvEncModeIdx].CaptureHeight>>1;
        } else {
            VinCfg.HwCaptureWindow.Height = YuvEncMgt[YuvEncModeIdx].CaptureHeight;
        }
        VinCfg.HwCaptureWindow.X = YuvModeInfo.OutputInfo.RecordingPixels.StartX +
            (((YuvModeInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);

        VinCfg.HwCaptureWindow.Y = (YuvModeInfo.OutputInfo.RecordingPixels.StartY +
            ((YuvModeInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)) & 0xFFFE;

        Layout[0].Width = YuvEncMgt[YuvEncModeIdx].MainWidth;
        Layout[0].Height = YuvEncMgt[YuvEncModeIdx].MainHeight;
        Layout[0].EnableSourceArea = 0; // Get all capture window to main
        Layout[0].DzoomFactorX = 1<<16;//InitZoomX; // 16.16 format
        Layout[0].DzoomFactorY = 1<<16;//InitZoomY;
        Layout[0].DzoomOffsetX = 0;
        Layout[0].DzoomOffsetY = 0;
        VinCfg.Layout = Layout;

        VinCfg.cbEvent = AmpUT_YUVEncVinEventCallback;
        VinCfg.cbSwitch= AmpUT_YUVEncVinSwitchCallback;
        AmpVin_Create(&VinCfg, &VideoEncVinA);
    }
    // Remember frame/field rate for muxers storing frame rate info
    EncFrameRate = YuvEncMgt[YuvEncModeIdx].TimeScale/YuvEncMgt[YuvEncModeIdx].TickPerPicture;

    // Create video encoder instances
    if (VideoEncPri == 0) {
        AMP_VIDEOENC_HDLR_CFG_s EncCfg;
        AMP_VIDEOENC_LAYER_DESC_s EncLayer;
#if 0
        AMP_VIDEOENC_H264_CFG_s *H264Cfg;
        AMP_VIDEOENC_MJPEG_CFG_s *MjpegCfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s HeaderInfo;
#endif
        memset(&EncCfg, 0x0, sizeof(AMP_VIDEOENC_HDLR_CFG_s));
        memset(&EncLayer, 0x0, sizeof(AMP_VIDEOENC_LAYER_DESC_s));
        EncCfg.MainLayout.Layer = &EncLayer;
        AmpVideoEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_YUVEncCallback;

        // Assign main layout in single chan
        EncCfg.MainLayout.Width = YuvEncMgt[YuvEncModeIdx].MainWidth;
        EncCfg.MainLayout.Height = YuvEncMgt[YuvEncModeIdx].MainHeight;
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = YuvModeInfo.OutputInfo.FrameRate.Interlace;
        EncCfg.MainTimeScale = YuvEncMgt[YuvEncModeIdx].TimeScale;
        EncCfg.MainTickPerPicture = YuvEncMgt[YuvEncModeIdx].TickPerPicture;
        EncCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    #ifdef CONFIG_SOC_A9
        EncCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    #else
        EncCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_POWERSAVING;
        EncCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
    #endif
        EncLayer.SourceType = AMP_ENC_SOURCE_VIN;
        EncLayer.Source = VideoEncVinA;
        EncLayer.SourceLayoutId = 0;
        EncLayer.EnableSourceArea = 0;  // No source cropping
        EncLayer.EnableTargetArea = 0;  // No target pip
        EncCfg.EventDataReadySkipNum = 0;  // File data ready every frame
        EncCfg.StreamId = AMP_VIDEOENC_STREAM_PRIMARY;

        {
            UINT8 *dspWorkAddr;
            UINT32 dspWorkSize;
            AmpUT_YUVEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
            EncCfg.DspWorkBufAddr = dspWorkAddr;
            EncCfg.DspWorkBufSize = dspWorkSize;
        }


        EncCfg.LiveViewProcMode = 0;
        EncCfg.LiveViewAlgoMode = 0;
        EncCfg.LiveViewOSMode = 0;
        EncCfg.VinSource = AMP_VIN_EXTERNAL_YUV;
        EncCfg.SysFreq.MaxArmCortexFreq = A12_CORTEX_PLL_MAX;
        EncCfg.SysFreq.MaxIdspFreq = A12_IDSP_PLL_MAX;
        EncCfg.SysFreq.MaxCoreFreq = A12_CORE_PLL_MAX;
       // Create primary stream handler
        AmpVideoEnc_Create(&EncCfg, &VideoEncPri); // Don't hYUVe to worry about h.264 spec settings when liveview
    }
#ifdef CONFIG_SOC_A9
    if (strcmp(YuvEncMgt[YuvEncModeIdx].Name, "2560x1440P60 16:9\0") == 0) {
        AmbaPrint("Turn OFF Tv");
        TvLiveview = 0;
    } else
#endif
    {
        AmbaPrint("Turn %s Tv",TvLiveview?"ON":"OFF");
    }

    {
        AmbaPrint("Turn ON LCD");
        LCDLiveview = 1;
    }

    // Setup display windows
    AmpUT_YUVEnc_DisplayStart();

    // Note: For A9, Bitstream-specific configs can be assigned here (before encode start), or when codec instance creation.
    //       But for A7L/A12 family, it shall be assigned when codec instance creation and before liveview start.
    //
#ifdef CONFIG_SOC_A12
    {
        AMP_VIDEOENC_H264_CFG_s *H264Cfg;
        AMP_VIDEOENC_MJPEG_CFG_s *MjpegCfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s HeaderInfo;
        AMP_VIDEOENC_BITSTREAM_CFG_s BitsCfg;

        memset(&BitsCfg, 0x0, sizeof(AMP_VIDEOENC_BITSTREAM_CFG_s));
        BitsCfg.Rotation = 0;//EncRotation;
        BitsCfg.TimeLapse = 0;//EncTimeLapse;
        BitsCfg.VideoThumbnail = 0;//EncThumbnail;

        // Assign bitstream-specific configs
        if (VideoEncPri) {
            if (EncPriSpecH264) {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
                H264Cfg = &BitsCfg.Spec.H264Cfg;
                H264Cfg->GopM = YuvEncMgt[YuvEncModeIdx].GopM;
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
                H264Cfg->Interlace = YuvModeInfo.OutputInfo.FrameRate.Interlace;
                H264Cfg->TimeScale = YuvEncMgt[YuvEncModeIdx].TimeScale;
                H264Cfg->TickPerPicture = YuvEncMgt[YuvEncModeIdx].TickPerPicture;
                H264Cfg->AuDelimiterType = 1;
                H264Cfg->QualityLevel = 0; // for Simple_GOP setting
                H264Cfg->StopMethod = AMP_VIDEOENC_STOP_NEXT_IP;
                HeaderInfo.GopM = H264Cfg->GopM;
                HeaderInfo.Width = YuvEncMgt[YuvEncModeIdx].MainWidth;
                HeaderInfo.Height = YuvEncMgt[YuvEncModeIdx].MainHeight;
                HeaderInfo.Interlace = YuvModeInfo.OutputInfo.FrameRate.Interlace;
                HeaderInfo.Rotation = 0;//EncRotation;

                // Use default SPS/VUI
                AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
                H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

                // Bitrate control
                H264Cfg->BitRateControl.BrcMode = YuvEncMgt[YuvEncModeIdx].BrcMode;
                H264Cfg->BitRateControl.AverageBitrate = (UINT32)(YuvEncMgt[YuvEncModeIdx].AverageBitRate * 1E6);
                if (YuvEncMgt[YuvEncModeIdx].BrcMode == VIDEOENC_SMART_VBR) {
                    H264Cfg->BitRateControl.MaxBitrate = (UINT32)(YuvEncMgt[YuvEncModeIdx].MaxBitRate * 1E6);
                    H264Cfg->BitRateControl.MinBitrate = (UINT32)(YuvEncMgt[YuvEncModeIdx].MinBitRate * 1E6);
                }
                H264Cfg->QualityControl.IBeatMode = 0;//EncIBeat;
            } else {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_MJPEG;
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;

                MjpegCfg->FrameRateDivisionFactor = 1;
                MjpegCfg->QuantMatrixAddr = MjpegQuantMatrix;
            }
            AmpVideoEnc_SetBitstreamConfig(VideoEncPri, &BitsCfg);
        }

    }

#endif

    return 0;
}

int AmpUT_YUVEnc_create(UINT32 modeIdx)
{
	AMP_ENC_PIPE_CFG_s pipeCfg;

    AmpUT_YUVEnc_LiveviewStart(modeIdx);
	// Register pipeline
	AmpEnc_GetDefaultCfg(&pipeCfg);
	//pipeCfg.cbEvent
	pipeCfg.encoder[0] = VideoEncPri;
	pipeCfg.numEncoder = 1;

	pipeCfg.cbEvent = AmpUT_YUVEncPipeCallback;
	pipeCfg.type = AMP_ENC_AV_PIPE;

	AmpEnc_Create(&pipeCfg, &EncPipe);

	AmpEnc_Add(EncPipe);

    AmpEnc_StartLiveview(EncPipe, 0);

    return 0;
}

int AmpUT_YUVEnc_EncodeStart()
{
    AMP_FIFO_CFG_s fifoDefCfg;

    if (Status != STATUS_YUV_IDLE)
        return -1;

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" VideoEncode : %s", YuvEncMgt[YuvEncModeIdx].Name);
    AmbaPrint(" VideoEncode : CaptureWin[%d, %d]", YuvEncMgt[YuvEncModeIdx].CaptureWidth, YuvEncMgt[YuvEncModeIdx].CaptureHeight);
    AmbaPrint(" VideoEncode : PriStream EncWin [%dx%d]", YuvEncMgt[YuvEncModeIdx].MainWidth, YuvEncMgt[YuvEncModeIdx].MainHeight);
    if (EncPriSpecH264) {
        AmbaPrint(" VideoEncode : AvgBrate %dMbyte", YuvEncMgt[YuvEncModeIdx].AverageBitRate);
        AmbaPrint(" VideoEncode : MaxBrate %dMbyte", YuvEncMgt[YuvEncModeIdx].MaxBitRate);
        AmbaPrint(" VideoEncode : MinBrate %dMbyte", YuvEncMgt[YuvEncModeIdx].MinBitRate);
        AmbaPrint(" VideoEncode : PriStream H264 GopM   %d", YuvEncMgt[YuvEncModeIdx].GopM);
        AmbaPrint(" VideoEncode : PriStream H264 GopN   %d", GOP_N);
        AmbaPrint(" VideoEncode : PriStream H264 GopIDR %d", GOP_IDR);
    } else {
        AmbaPrint(" VideoEncode : PriStream MJpeg");
    }

    AmbaPrint(" =========================================================");

    //
    // Note: For A9, Bitstream-specific configs can be assigned here (before encode start), or when codec instance creation.
    //       But for A7L/A12 family, it shall be assigned when codec instance creation and before liveview start.
    //

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
        AmpVideoEnc_SetBitstreamBuffer(VideoEncPri, &BitsBufCfg);

        AmbaPrint("H.264 Bits 0x%x size %d Desc 0x%x size %d", H264BitsBuf, BITSFIFO_SIZE, H264DescBuf, DESC_SIZE);
        AmbaPrint("MJPEG Bits 0x%x size %d Desc 0x%x size %d", MjpgBitsBuf, BITSFIFO_SIZE, MjpgDescBuf, DESC_SIZE);
    }

    if (VirtualPriFifoHdlr == NULL) {
        // create a virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = VideoEncPri;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = DESC_SIZE/sizeof(AMBA_DSP_EVENT_ENC_PIC_READY_s);
        fifoDefCfg.cbEvent = AmpUT_YUVEnc_FifoCB;
        if (EncPriSpecH264) {
            fifoDefCfg.RawBaseAddr = (UINT32)H264BitsBuf;
            fifoDefCfg.RawLimitAddr = (UINT32)H264BitsBuf + BITSFIFO_SIZE - 1;
        } else {
            fifoDefCfg.RawBaseAddr = (UINT32)MjpgBitsBuf;
            fifoDefCfg.RawLimitAddr = (UINT32)MjpgBitsBuf + BITSFIFO_SIZE - 1;
        }
        AmpFifo_Create(&fifoDefCfg, &VirtualPriFifoHdlr);
    } else {
        AmpFifo_EraseAll(VirtualPriFifoHdlr);
    }

    AmpEnc_StartRecord(EncPipe, 0);
    Status = STATUS_YUV_ENCODE;
    return 0;
}

int AmpUT_YUVEnc_EncStop()
{
    if (Status != STATUS_YUV_ENCODE)
        return -1;

    AmpEnc_StopRecord(EncPipe, 0);
    Status = STATUS_YUV_IDLE;

    return 0;
}


int AmpUT_YUVEnc_EncPause()
{
    if (Status != STATUS_YUV_ENCODE)
        return -1;

    AmpEnc_PauseRecord(EncPipe, 0);
    Status = STATUS_YUV_PAUSE;

    return 0;
}

int AmpUT_YUVEnc_EncResume()
{
    if (Status != STATUS_YUV_PAUSE)
        return -1;

    AmpEnc_ResumeRecord(EncPipe, 0);
    Status = STATUS_YUV_ENCODE;
    return 0;
}

/* delete pipe */
int AmpUT_YUVEnc_Delete()
{
    UINT32 Flag = AMP_ENC_FUNC_FLAG_WAIT;

    if (WirelessModeEnable) {
        Flag |= AMP_ENC_FUNC_FLAG_IDLE_DSP_ON_STOP;
        Flag &= ~AMP_ENC_FUNC_FLAG_SUS_DSP_ON_STOP;
        DspSuspendEnable = 0;
    } else if (DspSuspendEnable) {
        Flag |= AMP_ENC_FUNC_FLAG_SUS_DSP_ON_STOP;
        Flag &= ~AMP_ENC_FUNC_FLAG_IDLE_DSP_ON_STOP;
        WirelessModeEnable = 0;
    }
    AmpEnc_StopLiveview(EncPipe, Flag);

#ifndef KEEP_ALL_INSTANCES
    AmpEnc_Delete(EncPipe); EncPipe = NULL;
    AmpVin_Delete(VideoEncVinA); VideoEncVinA = NULL;
    //AmbaImgSchdlr_Delete(ImgSchdlr);
    AmpVideoEnc_Delete(VideoEncPri); VideoEncPri = NULL;

    if (VirtualPriFifoHdlr) {
        AmpFifo_Delete(VirtualPriFifoHdlr);
        VirtualPriFifoHdlr = NULL;
    }
#endif
    return 0;
}

int AmpUT_YUVEncTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_YUVEncTest cmd: %s", argv[1]);

    if (strcmp(argv[1],"init") == 0) {
        int ss = 0,dd = 0;
        if (argc < 3) {
            AmbaPrint("Usage: t videoenc init [sensor_id]");
            AmbaPrint("               sensor_id: 0 -- TI5150");
            AmbaPrint("               LcdID: 0 -- WDF4648W");
            return 0;
        }
        ss = atoi(argv[2]);
        if (argc == 4) {
            dd = atoi(argv[3]);
        }
        AmpUT_YUVEnc_Init(ss, dd);
        Status = STATUS_YUV_IDLE;
    } else if ((strcmp(argv[1],"liveviewstart") == 0) || (strcmp(argv[1],"lvst") == 0)) {
        if (Status != STATUS_YUV_IDLE && \
            Status != STATUS_YUV_ENCODE) {
            AmbaPrint("[Audio] Non Init");
            return -1;
        }
        if (argc < 3) {
            AmbaPrint("Usage: t YUVenc create [VideoModeID] [AudioEncType] ");
            AmbaPrint("             VideoModeID : Mode ID");
            AmbaPrint("TI5150 mode_id: ");
            AmbaPrint(" 0 -- 720x480 60fps interlace DualStrm: O MJPG O OSDBlend O DualHDStrm O");
            AmbaPrint(" 1 -- 720x576 50fps interlace DualStrm: O MJPG O OSDBlend O DualHDStrm O");
            return -1;
        } else {
            UINT32 tt = atoi(argv[2]);
            AmpUT_YUVEnc_create(tt);
        }
    } else if ((strcmp(argv[1],"encstart") == 0) || (strcmp(argv[1],"enst") == 0)) {
        UINT32 tt = atoi(argv[2]);

        if (Status == STATUS_YUV_ENCODE) {
            AmbaPrint("[Audio] encode start during Encoding");
            return -1;
        }

        AmbaPrint("Encode for %d milliseconds", tt);
        AmpUT_YUVEnc_EncodeStart();
        if (tt != 0) {
        while (tt) {
            AmbaKAL_TaskSleep(1);
            tt--;
            if (tt == 0) {
                AmpUT_YUVEnc_EncStop();
            }
            if (Status == STATUS_YUV_IDLE)
                tt = 0;
            }
        }
    } else if ((strcmp(argv[1],"encstop") == 0) || (strcmp(argv[1],"ensp") == 0)) {
        if (Status != STATUS_YUV_ENCODE) {
            AmbaPrint("[Audio] encode stop during IDLE");
            return -1;
        }

        AmpUT_YUVEnc_EncStop();
    } else if ((strcmp(argv[1],"encpause") == 0) || (strcmp(argv[1],"enps") == 0)) {
        if (Status != STATUS_YUV_ENCODE) {
            AmbaPrint("[Audio] encode Pause during IDLE");
            return -1;
        }

        AmpUT_YUVEnc_EncPause();
    } else if ((strcmp(argv[1],"encresume") == 0) || (strcmp(argv[1],"enrs") == 0)) {
        if (Status != STATUS_YUV_PAUSE) {
            AmbaPrint("[Audio] encode Resume during BUSY/IDLE");
            return -1;
        }

        AmpUT_YUVEnc_EncResume();
    } else if (strcmp(argv[1],"delete") == 0) {
        if (Status == STATUS_YUV_ENCODE) {
            AmbaPrint("[Audio] delete during Encodeing !!");
            return -1;
        }

        AmpUT_YUVEnc_Delete();
    } else if (strcmp(argv[1],"suspend") == 0) {
        DspSuspendEnable = atoi(argv[2]);
        AmbaPrint("%s Dsp suspend in LiveveiwStop !", DspSuspendEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"wirelessmode") == 0) {
        WirelessModeEnable = atoi(argv[2]);
        AmbaPrint("%s Enter Wireless mode in LiveveiwStop !", WirelessModeEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"logmuxer")==0) {
        LogMuxer = atoi(argv[2]);
        AmbaPrint("Log muxer: %s",LogMuxer?"ON":"OFF");
    }  else {
        AmbaPrint("Usage: t yuvenc init|create|encstart|encstop");
        AmbaPrint("       init: init all");
        AmbaPrint("       liveviewstart [VideoModeID] : create video instance");
        AmbaPrint("       encstart [millisecond]: recording for N milliseconds");
        AmbaPrint("                               N = 0 will do continuous encode");
        AmbaPrint("       encstop: stop recording");
        AmbaPrint("       encpause: pause recording");
        AmbaPrint("       encresume: resume recording");
        AmbaPrint("       delete: delete av instance");
}

return 0;
}

int AmpUT_YUVEncTestAdd(void)
{
    AmbaPrint("Adding AmpUT_YUVEnc");

    AmbaTest_RegisterCommand("yuvenc", AmpUT_YUVEncTest);

return AMP_OK;
}

