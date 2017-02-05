/**
 *  @file AmpUT_Xcode.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/03/25 |cyweng       |Created        |
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
#include "DecCFSWrapper.h"

#define VIDEODEC_RAW_SIZE (10<<20) // 10MB raw buffer
#define VIDEODEC_RAW_DESC_NUM (128) // descriptor number = 128

// global var
static void* avcCodecBuf = NULL;
static void* avcRawBuf = NULL;
static void* avcRawBufOri = NULL;
static void* avcDescBuf = NULL;
static AMP_AVDEC_HDLR_s *avcDecHdlr = NULL;
static AMP_DEC_PIPE_HDLR_s *decPipeHdlr = NULL;
static AMP_FIFO_HDLR_s *avcDecFifoHdlr = NULL;

// Window handler
static AMP_DISP_WINDOW_HDLR_s *vidDecLcdWinHdlr = NULL;
static AMP_DISP_WINDOW_HDLR_s *vidDecTvWinHdlr = NULL;

// dmy dmx
static AMBA_KAL_TASK_t VideoDecDemuxTask = {0};
static AMBA_KAL_MUTEX_t VideoDecFrmMutex = {0};
static UINT32 VideoDecFrmCnt = 0;
static AMP_CFS_FILE_s *avcIdxFile = NULL;
static AMP_CFS_FILE_s *avcRawFile = NULL;

static char AmpUT_XcodeStack[1024];
typedef enum {
    FEED_SPEED_NOR,
    FEED_SPEED_IDR
}FEED_SPEED;
static UINT32 feedingSpeed = FEED_SPEED_NOR;

static UINT8 enableTranscode = 1;
static UINT8 transcodeTargetFmt = 0;

/**
 * Read elementary stream data from memory
 */
#if 0
static UINT8 *fmt_read_stream_mem(UINT8 *buf, int size, UINT8 *ptr,
                           UINT8 *base, UINT8 *limit)
{
    UINT32 rear;

    if (!((ptr >= base) && (ptr < limit))) {
        K_ASSERT(0);
    }

    if ((ptr+size) <= limit) {
        memcpy(ptr, buf, size);
        ptr += size;
    } else {
        rear = limit - ptr;
        memcpy(ptr, buf, rear); buf += rear;
        memcpy(base, buf, size-rear);
        ptr = base + size-rear;
    }

    if (ptr == limit) {
        ptr = base;
    }

    return ptr;
}
#endif

static void AmpUT_Xcode_DemuxTask(UINT32 info)
{
    AMP_BITS_DESC_s desc;
    AMP_BITS_DESC_s tmpDesc = { 0 };
    NHNT_SAMPLE_HEADER_s nhntDesc = {0};
    UINT32 freeToEnd;
    UINT32 remain;
    UINT32 prePts =0xFFFFFFFF;
    UINT32 totalSz = 0;
    UINT8 waitingSpace = 0;

    UINT8* rawBase = avcRawBuf;
    UINT8* rawLimit = (UINT8*)((UINT32)avcRawBuf + VIDEODEC_RAW_SIZE - 1);

    AmbaPrint("AmpUT_Xcode_DemuxTask Start!");
    // raw file used format nhnt
    // check http://gpac.wp.mines-telecom.fr/mp4box/media-import/nhnt-format/ for more info
    // we could get it by using MP4box ( -nhnl )
    while (1) {
//AmbaPrint("AmpUT_Xcode_DemuxTask Start  1");
        AmbaKAL_TaskSleep(1);
        AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER);
        if (VideoDecFrmCnt == 0 || VideoDecFrmCnt == 0xFFFFFFFF || avcRawFile == NULL) {
//AmbaPrint("AmpUT_Xcode_DemuxTask Start  2 - %d 0x%x", VideoDecFrmCnt, avcRawFile);
            AmbaKAL_MutexGive(&VideoDecFrmMutex);
            AmbaKAL_TaskSleep(10);
            continue;
        }

        // prepare entry
        AmpFifo_PrepareEntry(avcDecFifoHdlr, &desc);

        // read frame
        // read out frame header
        //DummyEncDbg("AmpDummyEnc_CodecTask: readout frame");
        if (waitingSpace == 0) {
            AmpCFS_fread(&nhntDesc, sizeof(nhntDesc), 1, avcIdxFile);
        }else{
            waitingSpace = 0;
        }
        tmpDesc.Pts = nhntDesc.CompositionTimeStamp;
        // EOS handling
        if (prePts == (UINT32)tmpDesc.Pts) {
            // EOS
            memset (&tmpDesc, 0, sizeof(tmpDesc));
            tmpDesc.Size = AMP_FIFO_MARK_EOS;// EOS magic num // TBD
            tmpDesc.Pts = prePts;
            AmpFifo_WriteEntry(avcDecFifoHdlr, &tmpDesc);
            VideoDecFrmCnt = 0xFFFFFFFF; // EOS
            AmbaPrint("Feeding EOS");
            AmbaKAL_MutexGive(&VideoDecFrmMutex);
            continue;
        }
        tmpDesc.Completed = 1;
        tmpDesc.Size = nhntDesc.DataSize;
        tmpDesc.Type = nhntDesc.FrameType;
        if (feedingSpeed == FEED_SPEED_IDR) {
            // feed idr only
            if (tmpDesc.Type != AMP_FIFO_TYPE_IDR_FRAME &&
                tmpDesc.Type != AMP_FIFO_TYPE_I_FRAME) {
                AmbaKAL_MutexGive(&VideoDecFrmMutex);
                continue;
            }
        }
        totalSz += tmpDesc.Size;

        if (tmpDesc.Size>desc.Size) {
            //AmbaPrint("Feeding no space!!!!!!!");
            AmbaKAL_MutexGive(&VideoDecFrmMutex);
            AmbaKAL_TaskSleep(5);
            waitingSpace = 1;
            continue;
        }
        /*AmbaPrint("read out : pts:%8d size:%8d ftype:%8d write to:0x%x, desc sz:%d", nhntDesc.compositionTimeStamp,
                  nhntDesc.dataSize,
                  nhntDesc.frameType,
                  desc.StartAddr,
                  tmpDesc.Size);*/

        prePts = tmpDesc.Pts;
        // read out frame
        tmpDesc.Align = 0;
        tmpDesc.StartAddr = desc.StartAddr;
        AmpCFS_fseek(avcRawFile, nhntDesc.FileOffset, AMBA_FS_SEEK_START);
        if ((tmpDesc.StartAddr + tmpDesc.Size - 1) <= rawLimit) {
            AmpCFS_fread(tmpDesc.StartAddr, tmpDesc.Size, 1, avcRawFile);
        } else {
            freeToEnd = rawLimit - tmpDesc.StartAddr + 1;
            remain = tmpDesc.Size - freeToEnd;
            AmpCFS_fread(tmpDesc.StartAddr, freeToEnd, 1, avcRawFile);
            AmpCFS_fread(rawBase, remain, 1, avcRawFile);
        }
        // write to fifo
        AmpFifo_WriteEntry(avcDecFifoHdlr, &tmpDesc);
        tmpDesc.SeqNum ++;
        VideoDecFrmCnt --;
        AmbaKAL_MutexGive(&VideoDecFrmMutex);
        AmbaKAL_TaskSleep(10);    // TODO: wait FIFO fix....
    }
}

static int AmpUT_Xcode_DummyDmx_FifoCB(void *hdlr, UINT32 event, void* info)
{
    /*
    AmbaPrint("AmpUT_Xcode_FifoCB on Event: 0x%x 0x%x", event, AMP_FIFO_EVENT_DATA_READY);
    if (event == AMP_FIFO_EVENT_DATA_CONSUMED) {
        AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER);
        //VideoDecFrmCnt -= (*((int*)info));
        //AmbaPrint("AmpUT_Xcode_FifoCB VideoDecFrmCnt: %d", VideoDecFrmCnt);
        AmbaKAL_MutexGive(&VideoDecFrmMutex);
    }
    */
    return 0;
}

static int AmpUT_Xcode_configDisplay(void)
{
    AMP_DISP_WINDOW_CFG_s window = {0};
    AMP_DISP_INIT_CFG_s dispInitCfg;
    AMP_DISP_CFG_s dispCfg;
    AMP_DISP_VOUT_SYSTEM_e LcdSystem = AMP_DISP_NTSC;  // NTSC
    AMP_DISP_VOUT_SYSTEM_e TvSystem = AMP_DISP_NTSC;  // NTSC

    if (dispModuleMemPool == NULL) {
        AmpDisplay_GetDefaultInitCfg(&dispInitCfg);
        AmbaKAL_BytePoolAllocate(&G_MMPL, &dispModuleMemPool, dispInitCfg.MemoryPoolSize, 100);
        dispInitCfg.MemoryPoolAddr = dispModuleMemPool;
        AmpDisplay_Init(&dispInitCfg);
    }

    /// create display hdlr
    if (LCDHdlr == NULL) {
        // Create DCHAN display handler
        AmpDisplay_GetDefaultCfg(&dispCfg);
        dispCfg.Device.Channel = AMP_DISP_CHANNEL_DCHAN;
        dispCfg.Device.DeviceId = AMP_DISP_LCD;
        dispCfg.Device.DeviceMode = 0xFFFF;
        dispCfg.ScreenRotate = 0;
        dispCfg.SystemType = LcdSystem;
        AmpDisplay_Create(&dispCfg, &LCDHdlr);
    }

    if (TVHdlr == NULL) {
        // Create FCHAN display handler
        AmpDisplay_GetDefaultCfg(&dispCfg);
        dispCfg.Device.Channel = AMP_DISP_CHANNEL_FCHAN;
        dispCfg.Device.DeviceId = AMP_DISP_HDMI;
        dispCfg.Device.DeviceMode = 0xFFFF;
        dispCfg.ScreenRotate = 0;
        dispCfg.SystemType = TvSystem;
        AmpDisplay_Create(&dispCfg, &TVHdlr);
    }

    // Creat LCD Window
    window.Source = AMP_DISP_DEC;
    window.CropArea.Width = 0;
    window.CropArea.Height = 0;
    window.CropArea.X = 0;
    window.CropArea.Y = 0;
    window.TargetAreaOnPlane.Width = 960;
    window.TargetAreaOnPlane.Height = 480;
    window.TargetAreaOnPlane.X = 0;
    window.TargetAreaOnPlane.Y = 0;
    AmpDisplay_CreateWindow(LCDHdlr, &window, &vidDecLcdWinHdlr);

    // Creat TV Window
    window.Source = AMP_DISP_DEC;
    window.CropArea.Width = 0;
    window.CropArea.Height = 0;
    window.CropArea.X = 0;
    window.CropArea.Y = 0;
    window.TargetAreaOnPlane.Width = 1920;
    window.TargetAreaOnPlane.Height = 1080;
    window.TargetAreaOnPlane.X = 0;
    window.TargetAreaOnPlane.Y = 0;
    AmpDisplay_CreateWindow(TVHdlr, &window, &vidDecTvWinHdlr);

    /** Step 2: Setup device */
    // Setup LCD & TV
    //AmpDisplay_Start(LCDHdlr);
    //AmpDisplay_Start(TVHdlr);

    // Active Window
    //AmpDisplay_ActivateVideoWindow(vidDecLcdWinHdlr);
    //AmpDisplay_ActivateVideoWindow(vidDecTvWinHdlr);
    //AmpDisplay_SetWindowActivateFlag(vidDecLcdWinHdlr, 1);
    //AmpDisplay_Update(LCDHdlr);

    return 0;
}

static int AmpUT_Xcode_DummyDmx_init(void)
{
    AMP_FIFO_CFG_s fifoDefCfg = {0};
    static UINT8 init = 0;
    int er = 0;

    if (init != 0) {
        return 0;
    }
    init = 1;

    // create fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = avcDecHdlr;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = 128;
    fifoDefCfg.cbEvent = AmpUT_Xcode_DummyDmx_FifoCB;
    AmpFifo_Create(&fifoDefCfg, &avcDecFifoHdlr);

    /* Create task */
    er = AmbaKAL_TaskCreate(&VideoDecDemuxTask, /* pTask */
    "UT_VideoDec_FWriteTask", /* pTaskName */
    7, /* Priority */
    AmpUT_Xcode_DemuxTask, /* void (*EntryFunction)(UINT32) */
    0x0, /* EntryArg */
    (void *) AmpUT_XcodeStack, /* pStackBase */
    1024, /* StackByteSize */
    AMBA_KAL_AUTO_START); /* AutoStart */

    if (er != OK) {
        AmbaPrint("Task create failed: %d", er);
    }


    /* create mutex  for frame read and write to fifo*/
    er = AmbaKAL_MutexCreate(&VideoDecFrmMutex);
    if (er != OK) {
        AmbaPrint("Mutex create failed: %d", er);
    }

    return 0;
}

static int AmpUT_Xcode_DummyDmx_open(char* rawFn, char* idxFn)
{
    NHNT_HEADER_s nhntHeader = {0};
    char mode[3] = "rb";
    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER);
    AmbaPrint("AmpUT_Xcode_DummyDmx_open");
    // open file
    avcRawFile = AmpDecCFS_fopen(rawFn, mode);
    if (avcRawFile == NULL ) {
        AmbaPrint("AmpUT_Xcode_init: raw open failed");
    }
    avcIdxFile = AmpDecCFS_fopen(idxFn, mode);
    if (avcIdxFile == NULL ) {
        AmbaPrint("AmpUT_Xcode_init: idx open failed");
    }
    AmpCFS_fread(&nhntHeader, sizeof(nhntHeader), 1, avcIdxFile);
    VideoDecFrmCnt = 0;
    AmbaKAL_MutexGive(&VideoDecFrmMutex);
    return 0;
}

static int AmpUT_Xcode_DummyDmx_close(void)
{
    AmbaPrint("AmpUT_Xcode_DummyDmx_close");
    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER);
    AmbaPrint("AmpUT_Xcode_DummyDmx_close 1");
    VideoDecFrmCnt = 0xFFFFFFFF;
    AmpCFS_fclose(avcRawFile);
    AmpCFS_fclose(avcIdxFile);
    avcRawFile = NULL;
    avcIdxFile = NULL;
    AmbaKAL_MutexGive(&VideoDecFrmMutex);
    return 0;
}

#if 0
static int AmpUT_Xcode_DummyDmx_TimeSearch(UINT32 timeStart)
{
    NHNT_HEADER_s nhntHeader = {0};
    UINT32 resolution = 90000;

    AmbaPrint("AmpUT_Xcode_DummyDmx_TimeSearch");
    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER);

    VideoDecFrmCnt = 0;

    // reset index file
    AmpCFS_fseek(avcIdxFile, 0, AMBA_FS_SEEK_START);

    // read index header
    AmpCFS_fread(&nhntHeader, sizeof(nhntHeader), 1, avcIdxFile);
    resolution = nhntHeader.TimeStampResolution;
    timeStart = (timeStart*resolution)/1000;

    // search for idx which's time is larger then time timeStart

    // reset the read point to the idr

    AmbaKAL_MutexGive(&VideoDecFrmMutex);
    return 0;
}
#endif

static int AmpUT_Xcode_DummyDmx_feed(int req)
{
    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER);
    VideoDecFrmCnt += req;
    AmbaKAL_MutexGive(&VideoDecFrmMutex);
    return 0;
}

static int AmpUT_Xcode_DummyDmx_waitFeedDone(void)
{
    while (VideoDecFrmCnt != 0) {
        AmbaKAL_TaskSleep(10);
        AmbaPrint("AmpUT_Xcode_DummyDmx_waitFeedDone: %d", VideoDecFrmCnt);
    }
    return 0;
}

static AMP_CFS_FILE_s *outputPriFile = NULL;
static AMP_CFS_FILE_s *outputPriIdxFile = NULL;
static UINT32 transcodeBufSize = 5<<20;
static UINT8* transcodeBuf = NULL;
static UINT8 DummyMuxTaskStack[3200];
static AMBA_KAL_TASK_t VideoDecDmyMuxTask = {0};
static AMP_FIFO_HDLR_s *avcTranscodeFifoHdlr = NULL;
static AMBA_KAL_SEM_t transcodeSem = {0};

static int AmpUT_Xcode_DummyMux_FifoCB(void *hdlr, UINT32 event, void* info)
{
    UINT32 *numFrames = info;
    //AmbaPrint("AmpUT_Xcode_DummyMux_FifoCB : %d", event);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        int i;

        for(i = 0; i < *numFrames; i++) {
            AmbaKAL_SemGive(&transcodeSem);
        }
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
            AmbaKAL_SemGive(&transcodeSem);
    }
    return 0;
}

static void AmpUT_Xcode_DummyMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *desc;
    int er;
    UINT8 *bitsLimit;

    NHNT_HEADER_s nhntHeader = {0};
    NHNT_SAMPLE_HEADER_s nhntSample = {0};
    UINT64 fileOffset = 0;

    AmbaPrint("AmpUT_Xcode_DummyMuxTask Start");

    if (0) {  // force to run at core 1.
        AMBA_KAL_TASK_t *myTask = AmbaKAL_TaskIdentify();

        AmbaKAL_TaskSmpCoreExclusionSet(myTask, 2);
    }

    while (1) {
        er = AmbaKAL_SemTake(&transcodeSem, 10000);
        if (er!= OK) {
          //  AmbaPrint(" no sem fff");
            AmbaKAL_TaskSleep(1);
            continue;
        }

#define AMPUT_FILE_DUMP
        #ifdef AMPUT_FILE_DUMP
        if (outputPriFile == NULL) {
            char fn[80];
            char mode[3] = "wb";
            AMP_CFS_FILE_s *UdtaFile;
            FORMAT_USER_DATA_s Udta;

            sprintf(fn,"C:\\out_%04d.%s",1000,(transcodeTargetFmt==0)?"h264":"mjpg");
            outputPriFile = AmpDecCFS_fopen(fn, mode);
            AmbaPrint("%s opened", fn);

            sprintf(fn,"C:\\OUT_%04d.nhnt",1000);
            outputPriIdxFile = AmpDecCFS_fopen(fn, mode);
            AmbaPrint("%s opened", fn);

            sprintf(fn,"C:\\OUT_%04d.udta",1000);
            UdtaFile = AmpDecCFS_fopen(fn, mode);
            AmbaPrint("%s opened", fn);

            Udta.nIdrInterval = 4;
            Udta.nTimeScale = 0;
            Udta.nTickPerPicture = 0;
            Udta.nN = 15;
            Udta.nM = 0;
            Udta.nVideoWidth = 0;
            Udta.nVideoHeight = 0;

            AmpCFS_fwrite(&Udta, sizeof(FORMAT_USER_DATA_s), 1, UdtaFile);
            AmpCFS_fclose(UdtaFile);

            nhntHeader.Signature[0]='n';
            nhntHeader.Signature[1]='h';
            nhntHeader.Signature[2]='n';
            nhntHeader.Signature[3]='t';
            nhntHeader.TimeStampResolution = 90000;
            AmpCFS_fwrite(&nhntHeader, sizeof(nhntHeader), 1, outputPriIdxFile);

            bitsLimit = transcodeBuf + transcodeBufSize - 1;
        }
        #endif

        er = AmpFifo_PeekEntry(avcTranscodeFifoHdlr, &desc, 0);
        if (er == 0) {
            AmbaPrint("TransCode pts:%8lld size:%5d@0x%08x", desc->Pts, desc->Size, desc->StartAddr);
        } else {
            while (er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                er = AmpFifo_PeekEntry(avcTranscodeFifoHdlr, &desc, 0);
            }
        }
        if (desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 avg;
            // EOS
            AmbaPrint("AMP_FIFO_MARK_EOS");
            #ifdef AMPUT_FILE_DUMP
                if (outputPriFile) {
                    AmpCFS_FSync(outputPriFile);
                    AmpCFS_fclose(outputPriFile);
                    //fnoPri++;
                    AmpCFS_FSync(outputPriIdxFile);
                    AmpCFS_fclose(outputPriIdxFile);
                    outputPriFile = NULL;
                    outputPriIdxFile = NULL;
                    fileOffset = 0;
                }
            #endif

            //encPriBytesTotal = 0;
            //encPriTotalFrames = 0;
        } else {
            #ifdef AMPUT_FILE_DUMP
                if (outputPriFile) {
                    nhntSample.CompositionTimeStamp = desc->Pts;
                    nhntSample.DecodingTimeStamp = desc->Pts;
                    nhntSample.DataSize = desc->Size;
                    nhntSample.FileOffset = fileOffset;
                    fileOffset += desc->Size;
                    nhntSample.FrameType = desc->Type;

                    AmpCFS_fwrite(&nhntSample, sizeof(nhntSample), 1, outputPriIdxFile);
                    AmbaPrint("Write: 0x%x sz %d limit %X",desc->StartAddr,desc->Size, bitsLimit);
                    if (desc->StartAddr + desc->Size <= bitsLimit) {
                       // AmbaCache_Flush(desc->StartAddr, desc->Size); // Need to flush when using cacheable memory
                        AmpCFS_fwrite(desc->StartAddr, desc->Size, 1, outputPriFile);
                    } else {
                        AmpCFS_fwrite(desc->StartAddr, bitsLimit - desc->StartAddr + 1, 1, outputPriFile);
                        AmpCFS_fwrite(transcodeBuf, desc->Size - (bitsLimit - desc->StartAddr + 1), 1, outputPriFile);
                    }
                }
            #else
                AmbaKAL_TaskSleep(1);
            #endif
            //encPriBytesTotal += desc->Size;
            //encPriTotalFrames ++;
        }
        AmpFifo_RemoveEntry(avcTranscodeFifoHdlr, 1);
    }
}

static int AmpUT_Xcode_DummyMux_init(void)
{
    AMP_AVDEC_HDLR_s *xCodec;
    AMP_FIFO_CFG_s fifoDefCfg = {0};
    static UINT8 init = 0;
    int er = 0;

    if (init != 0) {
        return 0;
    }
    init = 1;

    AmpVideoDec_GetXCodec(avcDecHdlr, &xCodec);
    AmbaPrint("Dmy xCodec 0x%x", xCodec);

    // create fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = xCodec;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = 128;
    fifoDefCfg.cbEvent = AmpUT_Xcode_DummyMux_FifoCB;
    AmpFifo_Create(&fifoDefCfg, &avcTranscodeFifoHdlr);

    AmbaKAL_SemCreate(&transcodeSem, 0) ;

    if (AmbaKAL_TaskCreate(&VideoDecDmyMuxTask, "AmpUT_Xcode_DummyMuxTask Muxing Task", 11, \
            AmpUT_Xcode_DummyMuxTask, 0x0, DummyMuxTaskStack, 3200, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("AmpUT_Xcode_DummyMuxTask task creation failed");
    }

    return 0;
}

static int AmpUT_Xcode_CodecCB(void *hdlr, UINT32 event, void* info)
{
    //AmbaPrint("AmpUT_Xcode_CodecCB on Event: 0x%x ", event);
    if (event == AMP_DEC_EVENT_DATA_UNDERTHRSHOLDD &&
            VideoDecFrmCnt != 0xFFFFFFFF) {
        AmpUT_Xcode_DummyDmx_feed(4); // feed 4 frames;
    }
    return 0;
}

/**
 *
 * @param transCodeFmt - 0: h264 output, 1:jpg output
 * @return
 */
int AmpUT_Xcode_init(int transCodeFmt)
{
    AMP_VIDEODEC_CFG_s codecCfg;
    AMP_VIDEODEC_INIT_CFG_s codecInitCfg;
    AMP_DEC_PIPE_CFG_s pipeCfg;

    /// init codec module
    AmpVideoDec_GetInitDefaultCfg(&codecInitCfg);
    AmbaKAL_BytePoolAllocate(&G_MMPL, &avcCodecBuf, codecInitCfg.BufSize, 100);
    codecInitCfg.Buf = (UINT8*)avcCodecBuf;
    AmpVideoDec_Init(&codecInitCfg);

    /// create codec hdlr
    AmpVideoDec_GetDefaultCfg(&codecCfg);
    // alloc memory
    AmbaKAL_BytePoolAllocate(&G_MMPL, &avcRawBufOri, VIDEODEC_RAW_SIZE + 64, 100);
    codecCfg.RawBuffer = (char*)ALIGN_64((UINT32)avcRawBufOri);
    avcRawBuf = (void*)ALIGN_64((UINT32)avcRawBufOri);
    AmbaPrint("%x -> %x", avcRawBufOri, codecCfg.RawBuffer);
    codecCfg.RawBufferSize = VIDEODEC_RAW_SIZE;
    AmbaKAL_BytePoolAllocate(&G_MMPL, &avcDescBuf, VIDEODEC_RAW_DESC_NUM*sizeof(AMP_BITS_DESC_s), 100);
    codecCfg.DescBuffer = avcDescBuf;
    codecCfg.NumDescBuffer = VIDEODEC_RAW_DESC_NUM;
    codecCfg.CbCodecEvent = AmpUT_Xcode_CodecCB;

    // TDB feature config
    codecCfg.Feature.MaxVoutWidth = 3840;
    codecCfg.Feature.MaxVoutHeight = 2160;

    // transcode setting
    if (enableTranscode) {
        codecCfg.Feature.Transcode.DataFmt = transCodeFmt; // H264
        codecCfg.Feature.Transcode.Enable = 1;
        codecCfg.Feature.Transcode.FrameRateConvert = 0;
        codecCfg.Feature.Transcode.GopM = 1;
        codecCfg.Feature.Transcode.GopN = 8;
        codecCfg.Feature.Transcode.GopIdrInt = 4;
        AmbaKAL_BytePoolAllocate(&G_MMPL,
                                 &transcodeBuf,
                                 transcodeBufSize,
                                 100);
        codecCfg.Feature.Transcode.RawBuffer = (char*)transcodeBuf;
        codecCfg.Feature.Transcode.RawBufferSize = transcodeBufSize;
        codecCfg.Feature.Transcode.TargetWidth = 432;
        codecCfg.Feature.Transcode.TargetHeight = 240;
        codecCfg.Feature.Transcode.TimeScale = 180000;
        codecCfg.Feature.Transcode.TickPerPicture = 3003;
        codecCfg.Feature.Transcode.Interlaced = 0;
    }

    AmpVideoDec_Create(&codecCfg, &avcDecHdlr);

    /// create dec mgr
    AmpDec_GetDefaultCfg(&pipeCfg);
    //pipeCfg.cbEvent
    pipeCfg.Decoder[0] = avcDecHdlr;
    pipeCfg.NumDecoder = 1;
    pipeCfg.Type = AMP_DEC_VID_PIPE;
    AmpDec_Create(&pipeCfg, &decPipeHdlr);

    AmpUT_Xcode_configDisplay();

    // active pipe
    AmpDec_Add(decPipeHdlr);
    // dummy mux
    AmpUT_Xcode_DummyDmx_init();

    if (enableTranscode) {
        AmpUT_Xcode_DummyMux_init();
    }

    AmpDisplay_Start(LCDHdlr);
    AmpDisplay_Start(TVHdlr);

    // Active Window
    AmpDisplay_SetWindowActivateFlag(vidDecLcdWinHdlr, 1);
    AmpDisplay_Update(LCDHdlr);
    AmpDisplay_SetWindowActivateFlag(vidDecTvWinHdlr, 1);
    AmpDisplay_Update(TVHdlr);

    //TBD remove
#if 0
    {
        UINT8 Module;
        UINT32 DebugMask;
        UINT8 ThreadMask;
        /// cmd code 0x01000004
        typedef struct DSP_SET_DEBUG_LEVEL_CMDtag
        {
            UINT32 cmd_code;
            UINT8  module;
            UINT8  add_or_set;
            UINT16 reserved;
            UINT32 debug_mask;
        }DSP_SET_DEBUG_LEVEL_CMD;
        DSP_SET_DEBUG_LEVEL_CMD DebugLevelConfig;
/*
        Module = (UINT8)atoi(Argv[3]);
        DebugMask = (UINT32)atoi(Argv[4]);
        ThreadMask = (UINT32)atoi(Argv[5]);
*/
        memset(&DebugLevelConfig , 0x0, sizeof(DSP_SET_DEBUG_LEVEL_CMD));
        DebugLevelConfig.module = Module;
        DebugLevelConfig.debug_mask = DebugMask;
        DebugLevelConfig.add_or_set = 0;

        //AmbaDSP_CmdSetDebugLevel(0, &DebugLevelConfig);
        AmbaDSP_CmdSetDebugThread(0, 0/*ThreadMask*/);
    }
#endif

    return 0;
}

int AmpUT_Xcode_start(char** argv)
{
    AMP_AVDEC_TRICKPLAY_s trick = {0};
    AMP_VIDEODEC_DISPLAY_s display = {0};
    char avcFn[80];
    char idxFn[80];
    UINT32 startTime = atoi(argv[5]);
    UINT32 speed = atoi(argv[6]);

    if (speed == 0) {
        speed = 0x100;
    }
    if (strcmp(argv[2], "") == 0) {
        AmbaPrint("AmpUT_Xcode_start: please enter index");
        return 0;
    }
    AmbaPrint("AmpUT_Xcode_start: target file - %s", argv[2]);
    sprintf(avcFn, "C:\\OUT_%s.h264",argv[2]);
    sprintf(idxFn, "C:\\OUT_%s.nhnt",argv[2]);
    AmpUT_Xcode_DummyDmx_open(avcFn, idxFn);

    // pre-feed
    VideoDecFrmCnt = 0x0;
    AmbaPrint("AmpUT_Xcode_start: %d 0x%3x", startTime, speed);
    if (speed > 0x100) {
        feedingSpeed = FEED_SPEED_IDR;
        AmpUT_Xcode_DummyDmx_feed(16); // prefeed 32 frames;
    }else{
        feedingSpeed = FEED_SPEED_NOR;
        AmpUT_Xcode_DummyDmx_feed(32); // prefeed 32 frames;
    }
    AmpUT_Xcode_DummyDmx_waitFeedDone();
    AmbaPrint("AmpUT_Xcode_start: feed done");

    display.SrcWidth = atoi(argv[3]);
    display.SrcHeight = atoi(argv[4]);
    display.AOI.X = 0;
    display.AOI.Y = 0;
    display.AOI.Width = display.SrcWidth;
    display.AOI.Height = display.SrcHeight;

    trick.Speed = speed;
    trick.TimeOffsetOfFirstFrame = startTime;
    if (atoi(argv[7]) == 0) {
        trick.Direction = AMP_VIDEO_PLAY_FW;
    }else{
        trick.Direction = AMP_VIDEO_PLAY_BW;
    }
    AmbaPrint("AmpUT_Xcode_start: start start");
    AmpDec_Start(decPipeHdlr, &trick, &display);
    AmbaPrint("AmpUT_Xcode_start: start end");
    return 0;
}

int AmpUT_Xcode_zoom(char** argv)
{
    AMP_VIDEODEC_DISPLAY_s display = {0};

    display.SrcWidth = atoi(argv[2]);
    display.SrcHeight = atoi(argv[3]);
    display.AOI.X = atoi(argv[4]);
    display.AOI.Y = atoi(argv[5]);
    display.AOI.Width = atoi(argv[6]);
    display.AOI.Height = atoi(argv[7]);

    AmpDec_Start(decPipeHdlr, NULL, &display);
    return 0;
}

int AmpUT_Xcode_trick(void)
{
    AMP_AVDEC_TRICKPLAY_s trick = {0};

    trick.Speed = 0x100;
    trick.TimeOffsetOfFirstFrame = 0;
    trick.Direction = AMP_VIDEO_PLAY_FW;
    AmpDec_Start(decPipeHdlr, &trick, NULL);
    return 0;
}

int AmpUT_Xcode_pause(void)
{
    AmpDec_Pause(decPipeHdlr);
    return 0;
}

int AmpUT_Xcode_resume(void)
{
    AmpDec_Resume(decPipeHdlr);
    return 0;
}

/*
 * JPEG QTable
 */
#if 0
static UINT8 __attribute__((aligned(32))) StdJpegQTable[128] = {
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
#endif

int AmpUT_Xcode_cap(int argc, char **argv)
{
    AMP_YUV_BUFFER_s buf;
    UINT8* rawY;
    UINT8* rawUV;
    char mode[] = "wb";
    char yFn[] = "C:\\dump.y";
    char uvFn[] = "C:\\dump.uv";
    AMP_CFS_FILE_s *yFp, *uvFp;
    //AMBA_JPEG_ENC_CONFIG_s jpegEncConfig;
    //AMBA_DSP_JPEG_ENC_CTRL_s jpgEncCtrl;


    AmbaPrint("AmpUT_Xcode_cap");
    AmbaKAL_BytePoolAllocate(&G_MMPL, (void**)&rawY, 4608*3456, 100);
    AmbaKAL_BytePoolAllocate(&G_MMPL, (void**)&rawUV, 4608*3456, 100);

    buf.LumaAddr = (UINT8*)ALIGN_64((UINT32)rawY);
    buf.ChromaAddr = (UINT8*)ALIGN_64((UINT32)rawUV);
    buf.ColorFmt = AMP_YUV_420;
    if (atoi(argv[2]) == 0) {
        buf.Pitch = 1920;
        buf.Width = 1920;
        buf.Height = 1080;
        buf.AOI.X = 0;
        buf.AOI.Y = 0;
        buf.AOI.Width = 1920;
        buf.AOI.Height = 1080;
    }else{
        buf.Pitch = atoi(argv[2]);
        buf.Width = atoi(argv[2]);
        buf.Height = atoi(argv[3]);
        buf.AOI.X = 0;
        buf.AOI.Y = 0;
        buf.AOI.Width = buf.Width;
        buf.AOI.Height = buf.Height;
    }
    AmbaPrint("Cap: %d %d", buf.Width, buf.Height);
    AmpVideoDec_DumpFrame(avcDecHdlr, &buf);

    yFp = AmpDecCFS_fopen(yFn, mode);
    uvFp = AmpDecCFS_fopen(uvFn, mode);

    AmpCFS_fwrite(buf.LumaAddr, buf.Height*buf.Pitch, 1, yFp);
    AmpCFS_fwrite(buf.ChromaAddr, buf.Height*buf.Pitch, 1, uvFp);

    AmpCFS_fclose(yFp);
    AmpCFS_fclose(uvFp);

    AmbaKAL_BytePoolFree(rawY);
    AmbaKAL_BytePoolFree(rawUV);
    return 0;
}

int AmpUT_Xcode_stop(void)
{
    AmbaPrint("AmpUT_Xcode_stop");
    AmpUT_Xcode_DummyDmx_close();

    AmpDec_Stop(decPipeHdlr);

    // TDB HACK code to close file without encode eos
    if (0) {
        AmpCFS_FSync(outputPriFile);
        AmpCFS_fclose(outputPriFile);
        AmpCFS_FSync(outputPriIdxFile);
        AmpCFS_fclose(outputPriIdxFile);
        outputPriFile = NULL;
        outputPriIdxFile = NULL;
    }
    return 0;
}

int AmpUT_Xcode_exit(void)
{
    AmbaPrint("AmpUT_Xcode_exit");
    AmpDec_Stop(decPipeHdlr);

    // deinit
    AmpDec_Remove(decPipeHdlr);
    AmpDec_Delete(decPipeHdlr);
    decPipeHdlr = NULL;

    AmpFifo_Delete(avcDecFifoHdlr);
    avcDecFifoHdlr = NULL;

    AmpVideoDec_Delete(avcDecHdlr);
    avcDecHdlr = NULL;
    AmbaKAL_BytePoolFree(avcRawBufOri);
    AmbaKAL_BytePoolFree(avcDescBuf);
    avcRawBuf = NULL;
    avcRawBufOri = NULL;
    avcDescBuf = NULL;

    AmbaKAL_BytePoolFree(avcCodecBuf);
    avcCodecBuf = NULL;

    AmpUT_Xcode_DummyDmx_close();

    return 0;
}

int AmpUT_Xcode_usage(void)
{
    AmbaPrint("AmpUT_Xcode");
    AmbaPrint("\t init [idx] - raw file name: OUT_[idx].h264 idx file name : OUT_[idx].nhnt");
    AmbaPrint("\t start - start decode");
    AmbaPrint("\t pause - pause decode");
    AmbaPrint("\t zoom [src width] [src height] [x] [y] [w] [h] - crop part of video");
    AmbaPrint("\t resume - resume decode");
    AmbaPrint("\t stop - stop decode");
    return 0;
}

int AmpUT_XcodeTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_XcodeTest cmd: %s", argv[1]);
    if ( strcmp(argv[1],"init") == 0) {
        AmpUT_Xcode_init(atoi(argv[2]));
        transcodeTargetFmt = atoi(argv[2]);
    }else if ( strcmp(argv[1],"start") == 0) {
        AmpUT_Xcode_start(argv);
    }else if ( strcmp(argv[1],"pause") == 0) {
        //AmpUT_Xcode_pause();
    }else if ( strcmp(argv[1],"resume") == 0) {
        //AmpUT_Xcode_resume();
    }else if ( strcmp(argv[1],"stop") == 0) {
        AmpUT_Xcode_stop();
    }else if ( strcmp(argv[1],"cap") == 0) {
        //AmpUT_Xcode_cap(argc, argv);
    }else if ( strcmp(argv[1],"exit") == 0) {
        AmpUT_Xcode_exit();
    }else if ( strcmp(argv[1],"zoom") == 0) {
        //AmpUT_Xcode_zoom(argv);
    }else{
        AmpUT_Xcode_usage();
    }
    return 0;
}

int AmpUT_XcodeInit(void)
{
    // hook command
    AmbaTest_RegisterCommand("xcode", AmpUT_XcodeTest);
    AmbaTest_RegisterCommand("xc", AmpUT_XcodeTest);

    return AMP_OK;
}
