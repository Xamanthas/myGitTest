/**
 *  @file AmpUT_TranscoderDec.c
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
#include <transcoder/DecTranscoder.h>
#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaUtility.h>
#include <util.h>
#include "AmbaCache.h"
#include "DecCFSWrapper.h"
#include "AmpUT_Display.h"

#define VIDEODEC_RAW_SIZE (10<<20) ///< 10MB raw buffer
#define VIDEODEC_RAW_DESC_NUM (128) ///< descriptor number = 128
// global var
static void* avcCodecBufOri = NULL; ///< Original buffer address of video codec
static void* avcRawBufOri = NULL;   ///< Original buffer address of video decode raw file
static void* avcRawBuf = NULL;      ///< Aligned buffer address of video decode raw file
static void* avcDescBufOri = NULL;  ///< Original buffer address of video descripter
static AMP_AVDEC_HDLR_s *avcDecHdlr = NULL;
static AMP_DEC_PIPE_HDLR_s *decPipeHdlr = NULL;
static AMP_FIFO_HDLR_s *avcDecFifoHdlr = NULL;

static AMBA_KAL_TASK_t FakeCodecTask = {0};
static AMP_FIFO_HDLR_s *FakeCodecFifoHdlr = NULL;
#define AMP_UT_FAKECODEC_STACK_SIZE (16<<10)
static char AmpUT_FakeCodecStack[AMP_UT_FAKECODEC_STACK_SIZE];
AMBA_KAL_SEM_t FakeSem = {0};

static AMP_AVDEC_HDLR_s *DecTransCodecHdlr = NULL;
static void* decTransCodecBufOri = NULL; ///< Original buffer address of video codec
static void* DecTransRawBufOri = NULL;   ///< Original buffer address of video decode raw file
static void* DecTransRawBuf = NULL;      ///< Aligned buffer address of video decode raw file

// feature ctrl
static UINT8 fadeEnable = 0;

// dmy dmx
static AMBA_KAL_TASK_t VideoDecDemuxTask = {
        0 };
static AMBA_KAL_MUTEX_t VideoDecFrmMutex = {
        0 };
static UINT32 VideoDecFrmCnt = 0;
static AMP_CFS_FILE_s *avcIdxFile = NULL;
static AMP_CFS_FILE_s *avcRawFile = NULL;

#define AMP_UT_VIDEODEC_STACK_SIZE (16<<10)
static char AmpUT_TranscoderDecStack[AMP_UT_VIDEODEC_STACK_SIZE];
typedef enum {
    FEED_SPEED_NOR,
    FEED_SPEED_IDR
} FEED_SPEED;
static UINT32 feedingSpeed = FEED_SPEED_NOR;

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
/*
static UINT8 *fmt_read_stream_mem(UINT8 *buf,
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
*/
/**
 * Put GOP header
 *
 * @param [in] ptr          Address to place GOP header
 * @param [in] rate         output rate
 * @param [in] scale        output scale fps = rate/scale
 * @param [in] skipFirstI   if skip first I
 * @param [in] skipLastI    if skip last I
 * @param [in] m            The distance between two anchor frames (I or P)
 * @param [in] n            The distance between two full images (I-frames)
 * @param [in] pts          time stamp
 *
 * @return 0 - OK, others - Error
 */
#define GOP_NALU_SIZE       22
static int fmt_put_gop_header(UINT8* ptr,
                              UINT32 rate,
                              UINT32 scale,
                              UINT8 skip_first_I,
                              UINT8 skip_last_I,
                              UINT8 M,
                              UINT8 N,
                              UINT pts)
{
    UINT8 reserved = 1;
    UINT16 high, low;
    UINT8 header[GOP_NALU_SIZE];
#define NORMALIZE_CLOCK(X, Y)   ((X * 90000) / Y)
    pts = NORMALIZE_CLOCK(pts, rate);

    // Steve 0927
    /* Fine tune pts start
     tmp_pts = pts;
     K = (u32)((tmp_pts/delta) + 0.49);
     pts_t = (u64)(K * delta);
     //printk("modify pts from %lld to %lld", pts, pts_t);
     pts = pts_t;*/
    // Fine tune pts end
    //printk("fmt_put_gop_header vno=%d , vpts=%lld pktcnt=%d skip_last_I=%d",p_movie->vno,pts,p_movie->pktcnt,skip_last_I);
    //pts &= VPTS_LSB_MASK;
#define HIGH_WORD(x) (UINT16)((x >> 16) & 0x0000ffff)
#define LOW_WORD(x) (UINT16)(x & 0x0000ffff)

    header[0] = (UINT8) 0x00;
    header[1] = (UINT8) 0x00;
    header[2] = (UINT8) 0x00;
    header[3] = (UINT8) 0x01;
    /* NAL header */
    header[4] = (UINT8) 0x7a;
    /* Main version */
    header[5] = (UINT8) 0x01;
    /* Sub version */
    header[6] = (UINT8) 0x01;

    high = HIGH_WORD(scale);
    low = LOW_WORD(scale);
    header[7] = ((UINT8) skip_first_I << (8 - 1)) | ((UINT8) skip_last_I << (7 - 1)) | ((UINT8) (high >> (16 - 6)));

    header[8] = ((UINT8) (high >> (10 - 8)));

    header[9] = ((UINT8) (high << (8 - 2))) | ((UINT8) reserved << (6 - 1)) | ((UINT8) (low >> (16 - 5)));

    header[10] = ((UINT8) (low >> (11 - 8)));

    high = HIGH_WORD(rate);
    header[11] = ((UINT8) (low << (8 - 3))) | ((UINT8) reserved << (5 - 1)) | ((UINT8) (high >> (16 - 4)));

    header[12] = ((UINT8) (high >> (12 - 8)));

    low = LOW_WORD(rate);
    header[13] = ((UINT8) (high << (8 - 4))) | ((UINT8) reserved << (4 - 1)) | ((UINT8) (low >> (16 - 3)));

    header[14] = ((UINT8) (low >> (13 - 8)));

    high = HIGH_WORD(pts);
    header[15] = ((UINT8) (low << (8 - 5))) | ((UINT8) reserved << (3 - 1)) | ((UINT8) (high >> (16 - 2)));

    header[16] = ((UINT8) (high >> (14 - 8)));

    low = LOW_WORD(pts);
    header[17] = ((UINT8) (high << (8 - 6))) | ((UINT8) reserved << (2 - 1)) | ((UINT8) (low >> (16 - 1)));

    header[18] = ((UINT8) (low >> (15 - 8)));

    header[19] = ((UINT8) (low << (8 - 7))) | ((UINT8) reserved >> (1 - 1));

    header[20] = ((UINT8) N << (8 - 8));

    header[21] = ((UINT8) M << (8 - 4)) & 0xf0;

    memcpy(ptr, header, GOP_NALU_SIZE);
    /*    ptr = iavobj_get_bitsbuffer_wp(p_movie->vbs);
     fmt_read_stream_mem(header, GOP_NALU_SIZE,
     ptr, p_movie->vbs_base, p_movie->vbs_limit);
     */
//  iavobj_update_bits_buffer_wp(p_movie->vbs, GOP_NALU_SIZE);
    return 0;
}

static UINT8 putHeader = 1;
static NHNT_HEADER_s nhntHeader = {
        0 };
/**
 * Dummy demux task.
 * Feed frames to raw buffer.
 *
 * @param [in] info         EntryArg
 *
 * @return 0 - OK, others - Error
 */
void AmpUT_TranscoderDec_DemuxTask(UINT32 info)
{
    AMP_BITS_DESC_s desc;
    AMP_BITS_DESC_s tmpDesc = {
            0 };
    NHNT_SAMPLE_HEADER_s nhntDesc = {
            0 };
    UINT32 freeToEnd;
    UINT32 remain;
    UINT32 prePts = 0xFFFFFFFF;
    UINT32 totalSz = 0;
    UINT8 waitingSpace = 0;
    int Er;

    UINT8* rawBase = DecTransRawBuf;
    UINT8* rawLimit = (UINT8*) ((UINT32) DecTransRawBuf + VIDEODEC_RAW_SIZE - 1);

    AmbaPrint("AmpUT_TranscoderDec_DemuxTask Start!");
    // raw file used format nhnt
    // check http://gpac.wp.mines-telecom.fr/mp4box/media-import/nhnt-format/ for more info
    // we could get it by using MP4box ( -nhnl )
    while (1) {
//AmbaPrint("AmpUT_TranscoderDec_DemuxTask Start  1");
        AmbaKAL_TaskSleep(1);
        AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
        if (VideoDecFrmCnt == 0 || VideoDecFrmCnt == 0xFFFFFFFF || avcRawFile == NULL ) {
//AmbaPrint("AmpUT_TranscoderDec_DemuxTask Start  2 - %d 0x%x", VideoDecFrmCnt, avcRawFile);
            AmbaKAL_TaskSleep(10);
            AmbaKAL_MutexGive(&VideoDecFrmMutex);
            continue;
        }

        // prepare entry
        Er = AmpFifo_PrepareEntry(avcDecFifoHdlr, &desc);
        if (Er != AMP_OK) {
            AmbaKAL_TaskSleep(10);
            AmbaKAL_MutexGive(&VideoDecFrmMutex);
            continue;
        }

        // read frame
        // read out frame header
        //DummyEncDbg("AmpDummyEnc_CodecTask: readout frame");
        if (waitingSpace == 0) {
            AmpCFS_fread(&nhntDesc, sizeof(nhntDesc), 1, avcIdxFile);
        } else {
            waitingSpace = 0;
        }
        if (putHeader == 1) {
            putHeader = 0;
            fmt_put_gop_header(desc.StartAddr, nhntHeader.TimeStampResolution, nhntDesc.DecodingTimeStamp, 0, 0, 1,
                    8, nhntDesc.DecodingTimeStamp);
            desc.StartAddr += 22;
        }
        tmpDesc.Pts = nhntDesc.CompositionTimeStamp;
        // EOS handling
        if (prePts == (UINT32) tmpDesc.Pts) {
            // EOS
            memset(&tmpDesc, 0, sizeof(tmpDesc));
            tmpDesc.Size = AMP_FIFO_MARK_EOS;       // EOS magic num
            tmpDesc.Type = AMP_FIFO_TYPE_EOS;       // Indicating EOS
            tmpDesc.Pts = prePts;
            AmpFifo_WriteEntry(avcDecFifoHdlr, &tmpDesc);
            VideoDecFrmCnt = 0xFFFFFFFF; // EOS
            AmbaPrint("Feeding EOS");
            AmbaKAL_MutexGive(&VideoDecFrmMutex);
            continue;
        }
        tmpDesc.Completed = 1;
        tmpDesc.Size = nhntDesc.DataSize;
        tmpDesc.Type = (AMP_FIFO_FRMAE_TYPE_e) nhntDesc.FrameType;
        if (feedingSpeed == FEED_SPEED_IDR) {
            // feed idr only
            if (tmpDesc.Type != AMP_FIFO_TYPE_IDR_FRAME && tmpDesc.Type != AMP_FIFO_TYPE_I_FRAME) {
                AmbaKAL_MutexGive(&VideoDecFrmMutex);
                continue;
            }
        }
        totalSz += tmpDesc.Size;

        if (tmpDesc.Size > desc.Size) {
            //AmbaPrint("Feeding no space!!!!!!!");
            AmbaKAL_MutexGive(&VideoDecFrmMutex);
            AmbaKAL_TaskSleep(5);
            waitingSpace = 1;
            continue;
        }
        AmbaPrint("read out : pts:%8d size:%8d ftype:%8d write to:0x%x, desc sz:%d", nhntDesc.CompositionTimeStamp,
         nhntDesc.DataSize,
         nhntDesc.FrameType,
         desc.StartAddr,
         tmpDesc.Size);

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
        tmpDesc.SeqNum++;
        VideoDecFrmCnt--;
        AmbaKAL_MutexGive(&VideoDecFrmMutex);
        AmbaKAL_TaskSleep(10);    // TODO: wait FIFO fix....
    }
}

/**
 * Callback handler of FIFO.
 *
 * @param [in] hdlr         FIFO handler
 * @param [in] event        Event ID
 * @param [in] info         Event info
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_TranscoderDec_DummyDmx_FifoCB(void *hdlr,
                                        UINT32 event,
                                        void* info)
{
    /*
     AmbaPrint("AmpUT_TranscoderDec_FifoCB on Event: 0x%x 0x%x", event, AMP_FIFO_EVENT_DATA_READY);
     if (event == AMP_FIFO_EVENT_DATA_CONSUMED){
     AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER);
     //VideoDecFrmCnt -= (*((int*)info));
     //AmbaPrint("AmpUT_TranscoderDec_FifoCB VideoDecFrmCnt: %d", VideoDecFrmCnt);
     AmbaKAL_MutexGive(&VideoDecFrmMutex);
     }
     */
    return 0;
}

/**
 * Create display handlers and windows on Vout.
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_TranscoderDec_configDisplay(void)
{
    AMP_DISP_WINDOW_CFG_s window;
    int Er = NG;

    if (AmpUT_Display_Init() == NG) {
        return Er;
    }

    memset(&window, 0, sizeof(AMP_DISP_WINDOW_CFG_s));


    // Creat LCD Window
    window.Source = AMP_DISP_DEC;
    window.CropArea.Width = 0;
    window.CropArea.Height = 0;
    window.CropArea.X = 0;
    window.CropArea.Y = 0;
    window.TargetAreaOnPlane.Width = 960;
    window.TargetAreaOnPlane.Height = 360;
    window.TargetAreaOnPlane.X = 0;
    window.TargetAreaOnPlane.Y = 60;
    if (AmpUT_Display_Window_Create(0, &window) == NG) {
        return Er;
    }

    // Creat TV Window
    window.Source = AMP_DISP_DEC;
    window.CropArea.Width = 0;
    window.CropArea.Height = 0;
    window.CropArea.X = 0;
    window.CropArea.Y = 0;
#ifdef     DEC_1080
    window.TargetAreaOnPlane.Width = 1920;
    window.TargetAreaOnPlane.Height = 1080;
#else
    window.TargetAreaOnPlane.Width = 3840;
    window.TargetAreaOnPlane.Height = 2160;
#endif    
    window.TargetAreaOnPlane.Width = 720;
    window.TargetAreaOnPlane.Height = 480;
    window.TargetAreaOnPlane.X = 0;
    window.TargetAreaOnPlane.Y = 0;
    if (AmpUT_Display_Window_Create(1, &window) == NG) {
        return Er;
    }

    /** Step 2: Setup device */
    // Setup LCD & TV
    //AmpDisplay_Start(LCDHdlr);
    //AmpDisplay_Start(TVHdlr);
    // Active Window
    //AmpDisplay_ActivateVideoWindow(vidDecLcdWinHdlr);
    //AmpDisplay_ActivateVideoWindow(vidDecTvWinHdlr);
    return 0;
}

/**
 * Set display channel configuration.
 *
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_TranscoderDec_configDisplayCh(char **argv)
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
 * Initialize dummy demux.
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_TranscoderDec_DummyDmx_init(void)
{
    AMP_FIFO_CFG_s fifoDefCfg = {
            0 };
    static UINT8 init = 0;
    int er = 0;

    // create fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
//    fifoDefCfg.hCodec = (void*)avcDecHdlr;
    fifoDefCfg.hCodec = (void*) DecTransCodecHdlr;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = 32;
    fifoDefCfg.cbEvent = AmpUT_TranscoderDec_DummyDmx_FifoCB;
    AmpFifo_Create(&fifoDefCfg, &avcDecFifoHdlr);
    AmbaPrint("avcDecFifoHdlr == %x @ %x", avcDecFifoHdlr, &avcDecFifoHdlr);

    if (init != 0) {
        return 0;
    }
    init = 1;

    /* Create task */
    er = AmbaKAL_TaskCreate(&VideoDecDemuxTask, /* pTask */
    "UT_VideoDec_FWriteTask", /* pTaskName */
    70, /* Priority */
    AmpUT_TranscoderDec_DemuxTask, /* void (*EntryFunction)(UINT32) */
    0x0, /* EntryArg */
    (void *) AmpUT_TranscoderDecStack, /* pStackBase */
    AMP_UT_VIDEODEC_STACK_SIZE, /* StackByteSize */
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

void AmpUT_FakeCodec_Task(UINT32 info)
{
    AMP_BITS_DESC_s *Desc;
    while (1) {
        AmbaKAL_SemTake(&FakeSem, AMBA_KAL_WAIT_FOREVER);
        if (AmpFifo_PeekEntry(FakeCodecFifoHdlr, &Desc, 0) == AMP_OK) {
            AmbaPrint("NewFrm incoming: @0x%x size:%d", Desc->StartAddr, Desc->Size);
            AmpFifo_RemoveEntry(FakeCodecFifoHdlr, 1);
        }
    }
}

int Amp_FakeCodec_FifoCB(void *hdlr,
                         UINT32 event,
                         void* info)
{
    AmbaPrint("Amp_FakeCodec_FifoCB on Event: 0x%x", event);
    if (event == AMP_FIFO_EVENT_DATA_READY){
        AmbaKAL_SemGive(&FakeSem);
    }
    return 0;
}

int AmpUT_FakeCodec_Init(void)
{
    AMP_FIFO_CFG_s fifoDefCfg = {
            0 };
    static UINT8 init = 0;
    int er = 0;

    // create fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
//    fifoDefCfg.hCodec = (void*)avcDecHdlr;
    fifoDefCfg.hCodec = (void*) AmpUT_FakeCodec_Task;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = 32;
    fifoDefCfg.cbEvent = Amp_FakeCodec_FifoCB;
    AmpFifo_Create(&fifoDefCfg, &FakeCodecFifoHdlr);
    AmbaPrint("FakeCodecFifoHdlr == %x", FakeCodecFifoHdlr);

    if (init != 0) {
        return 0;
    }
    init = 1;

    /* create sem  for frame read and write to fifo*/
    er = AmbaKAL_SemCreate(&FakeSem, 0);
    if (er != OK) {
        AmbaPrint("Sem create failed: %d", er);
    }

    /* Create task */
    er = AmbaKAL_TaskCreate(&FakeCodecTask, /* pTask */
    "UT_FakeCodec_FifoTask", /* pTaskName */
    71, /* Priority */
    AmpUT_FakeCodec_Task, /* void (*EntryFunction)(UINT32) */
    0x0, /* EntryArg */
    (void *) AmpUT_FakeCodecStack, /* pStackBase */
    AMP_UT_FAKECODEC_STACK_SIZE, /* StackByteSize */
    AMBA_KAL_AUTO_START); /* AutoStart */

    if (er != OK) {
        AmbaPrint("Task create failed: %d", er);
    }
    return 0;
}

/**
 * Open files of dummy demux.
 *
 * @param [in] rawFn        Full path of raw file (.h264)
 * @param [in] idxFn        Full path of idx file (.nhnt)
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_TranscoderDec_DummyDmx_open(char* rawFn,
                                      char* idxFn)
{
    char mode[3] = "rb";

    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
    AmbaPrint("AmpUT_TranscoderDec_DummyDmx_open");
    // open file
    avcRawFile = AmpDecCFS_fopen(rawFn, mode);
    if (avcRawFile == NULL ) {
        AmbaPrint("AmpUT_TranscoderDec_init: raw open failed");
    }
    avcIdxFile = AmpDecCFS_fopen( idxFn, mode);
    if (avcIdxFile == NULL ) {
        AmbaPrint("AmpUT_TranscoderDec_init: idx open failed");
    }
    AmpCFS_fread(&nhntHeader, sizeof(nhntHeader), 1, avcIdxFile);
    VideoDecFrmCnt = 0;
    putHeader = 1;
    AmbaKAL_MutexGive(&VideoDecFrmMutex);
    return 0;
}

/**
 * Close files of dummy demux.
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_TranscoderDec_DummyDmx_close(void)
{
    AmbaPrint("AmpUT_TranscoderDec_DummyDmx_close");
    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
    AmbaPrint("AmpUT_TranscoderDec_DummyDmx_close 1");
    VideoDecFrmCnt = 0xFFFFFFFF;
    AmpCFS_fclose(avcRawFile);
    AmpCFS_fclose(avcIdxFile);
    avcRawFile = NULL;
    avcIdxFile = NULL;
    AmbaKAL_MutexGive(&VideoDecFrmMutex);
    return 0;
}

/**
 * Time search.
 *
 * @param [in] timeStart    Start time in ms
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_TranscoderDec_DummyDmx_TimeSearch(UINT32 timeStart)
{
#if 0
    NHNT_HEADER_s nhntHeader;
    UINT32 resolution = 90000;

    AmbaPrint("AmpUT_TranscoderDec_DummyDmx_TimeSearch");
    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER );

    VideoDecFrmCnt = 0;

    // reset index file
    AmpCFS_fseek(avcIdxFile, 0, AMBA_FS_SEEK_START);

    // read index header
    AmpCFS_fread(&nhntHeader, sizeof(nhntHeader), 1, avcIdxFile);
    resolution = nhntHeader.TimeStampResolution;
    timeStart = (timeStart * resolution) / 1000;

    // search for idx which's time is larger then time timeStart

    // reset the read point to the idr

    AmbaKAL_MutexGive(&VideoDecFrmMutex);
#endif
    return 0;
}

/**
 * Request dummy demux to feed frames.
 *
 * @param [in] req          Number of frames requested to feed
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_TranscoderDec_DummyDmx_feed(int req)
{
    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
    if (VideoDecFrmCnt != 0xFFFFFFFF) {
        VideoDecFrmCnt += req;
    }
    AmbaKAL_MutexGive(&VideoDecFrmMutex);
    return 0;
}

/**
 * Wait for dummy demux to finish feeding frames.
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_TranscoderDec_DummyDmx_waitFeedDone(void)
{
    while (VideoDecFrmCnt != 0 && VideoDecFrmCnt!=0xFFFFFFFF) {
        AmbaKAL_TaskSleep(10);
        AmbaPrint("AmpUT_TranscoderDec_DummyDmx_waitFeedDone: %d", VideoDecFrmCnt);
    }
    return 0;
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
int AmpUT_TranscoderDec_CodecCB(void *hdlr,
                                UINT32 event,
                                void* info)
{
    //AmbaPrint("AmpUT_TranscoderDec_CodecCB on Event: 0x%x ", event);

    if (event == AMP_DEC_EVENT_DATA_UNDERTHRSHOLDD && VideoDecFrmCnt != 0xFFFFFFFF) {
        AmpUT_TranscoderDec_DummyDmx_feed(4); // feed 4 frames;
    }
    return 0;
}

/**
 * Set fading effect.
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_TranscoderDec_SetFading(void)
{
    AMP_VIDEODEC_FADING_EFFECT_s fadingCfg[2] = {
            0 };
    fadingCfg[0].Duration = 1000;
    fadingCfg[0].StartTime = 0;
    fadingCfg[0].StartMatrix[0] = 0;
    fadingCfg[0].StartMatrix[1] = 0;
    fadingCfg[0].StartMatrix[2] = 0;
    fadingCfg[0].StartMatrix[3] = 0;
    fadingCfg[0].StartMatrix[4] = 0;
    fadingCfg[0].StartMatrix[5] = 0;
    fadingCfg[0].StartMatrix[6] = 0;
    fadingCfg[0].StartMatrix[7] = 0;
    fadingCfg[0].StartMatrix[8] = 0;
    fadingCfg[0].StartYOffset = 128;
    fadingCfg[0].StartUOffset = 255;
    fadingCfg[0].StartVOffset = 255;

    fadingCfg[0].EndMatrix[0] = 1024;
    fadingCfg[0].EndMatrix[1] = 0;
    fadingCfg[0].EndMatrix[2] = 0;
    fadingCfg[0].EndMatrix[3] = 0;
    fadingCfg[0].EndMatrix[4] = 1024;
    fadingCfg[0].EndMatrix[5] = 0;
    fadingCfg[0].EndMatrix[6] = 0;
    fadingCfg[0].EndMatrix[7] = 0;
    fadingCfg[0].EndMatrix[8] = 1024;
    fadingCfg[0].EndYOffset = 0;
    fadingCfg[0].EndUOffset = 128;
    fadingCfg[0].EndVOffset = 128;
    AmpVideoDec_SetFadingEffect(avcDecHdlr, 1, // only fade in
            fadingCfg);
    return 0;
}

/**
 * call back for TRANSCODER CODEC
 *
 * @param hdlr  transcoder handler
 * @param event event id
 * @param info  callback info
 * @return
 */
int AmpUT_TranscodeCodecCB(void* hdlr,
                           UINT32 event,
                           void* info)
{
    if (event == AMP_DEC_EVENT_DATA_UNDERTHRSHOLDD && VideoDecFrmCnt != 0xFFFFFFFF) {
        AmpUT_TranscoderDec_DummyDmx_feed(4); // feed 4 frames;
    }
    return 0;
}

/**
 * init transcoder
 *
 * @return
 */
int AmpUT_TranscoderDec_init(void)
{
    AMP_VIDEODEC_CFG_s codecCfg;
    AMP_VIDEODEC_INIT_CFG_s codecInitCfg;
    AMP_DEC_PIPE_CFG_s pipeCfg;

    /** Initialize codec module */
    if (avcCodecBufOri == NULL ) {
        // Get the default codec module settings
        AmpVideoDec_GetInitDefaultCfg(&codecInitCfg);
        // Customize the module settings
        // You can add some codes here ...
        // Allocate memory for codec module
        if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(codecInitCfg.Buf), &avcCodecBufOri, codecInitCfg.BufSize, 1 << 5)
                != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            return -1;
        }
        // Configure the initial settings
        AmpVideoDec_Init(&codecInitCfg);
    }

    {
        AMP_DEC_TRANSCODER_INIT_CFG_s TransInitCfg;
        AmpDecTranscoder_GetInitDefaultCfg(&TransInitCfg);
        if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(TransInitCfg.WorkingBuf), &decTransCodecBufOri,
                TransInitCfg.WorkingBufSize, 1 << 5) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            return -1;
        }
        AmpDecTranscoder_Init(&TransInitCfg);
    }

    /** Create codec handler */
    // Get the default codec handler settings
    AmpVideoDec_GetDefaultCfg(&codecCfg);
    // Allocate memory for codec raw buffer
    if (AmpUtil_GetAlignedPool(&G_MMPL, &avcRawBuf, &avcRawBufOri, VIDEODEC_RAW_SIZE, 1 << 6) != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        return -1;
    }
    codecCfg.RawBuffer = (char*) avcRawBuf;
    AmbaPrint("%x -> %x", avcRawBufOri, codecCfg.RawBuffer);
    codecCfg.RawBufferSize = VIDEODEC_RAW_SIZE;
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(codecCfg.DescBuffer), &avcDescBufOri,
            VIDEODEC_RAW_DESC_NUM * sizeof(AMP_BITS_DESC_s), 1 << 5) != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        return -1;
    }
    codecCfg.NumDescBuffer = VIDEODEC_RAW_DESC_NUM;
    // Customize the handler settings
    codecCfg.CbCodecEvent = AmpUT_TranscoderDec_CodecCB;
    // TDB feature config
    codecCfg.Feature.MaxVoutWidth = 3840;
    codecCfg.Feature.MaxVoutHeight = 2160;
    // Create a codec handler, and configure the initial settings
    if (AmpVideoDec_Create(&codecCfg, &avcDecHdlr) != AMP_OK) {
        AmbaPrint("%s:%u Cannot create video codec.", __FUNCTION__, __LINE__);
        return -1;
    }
    AmbaPrint("avcDecHdlr = 0x%x ", avcDecHdlr);

    {
        AMP_DEC_TRANSCODER_CFG_s TransCfg;
        AmpDecTranscoder_GetDefaultCfg(&TransCfg);
        TransCfg.RawBufferSize = VIDEODEC_RAW_SIZE;
        if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &DecTransRawBuf, &DecTransRawBufOri, TransCfg.RawBufferSize,
                1 << 5) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        }
        TransCfg.RawBuffer = DecTransRawBuf;
        TransCfg.CbTranscode = AmpUT_TranscodeCodecCB;
        TransCfg.DstCodec = (AMP_AVDEC_HDLR_s*)AmpUT_FakeCodec_Task;
        TransCfg.DstCodecType = 1;
        AmpDecTranscoder_Create(&TransCfg, &DecTransCodecHdlr);
        AmbaPrint("DecTransCodecHdlr = 0x%x Raw@ 0x%x - 0x%x", DecTransCodecHdlr, DecTransRawBuf, TransCfg.RawBufferSize + DecTransRawBuf - 1);
    }

    /** Create decoder manager */
    // Get the default decoder manager settings
    AmpDec_GetDefaultCfg(&pipeCfg);
    // Customize the manager settings
    //pipeCfg.cbEvent
//    pipeCfg.Decoder[0] = avcDecHdlr;
    pipeCfg.Decoder[0] = DecTransCodecHdlr;
    pipeCfg.NumDecoder = 1;
    pipeCfg.Type = AMP_DEC_VID_PIPE;
    // Create a decoder manager, and insert the codec handler into the manager
    AmpDec_Create(&pipeCfg, &decPipeHdlr);

    /** Create LCD handler, TV handler, LCD window, TV window */
    AmpUT_TranscoderDec_configDisplay();

    /** Activate decoder manager */
    // Activate the decoder manager and all the codec handlers in the manager
    AmpDec_Add(decPipeHdlr);

    /** Dummy mux */
    // Create a dummy demuxer
    AmpUT_TranscoderDec_DummyDmx_init();

    AmpUT_FakeCodec_Init();

    /** Enable vout */
    // Start up TV handler
    /*
    AmpDisplay_Start(TVHdlr);
    AmpDisplay_SetWindowActivateFlag(vidDecTvWinHdlr, 1);
    AmpDisplay_Update(TVHdlr);
    // Start up LCD handler
    AmpDisplay_Start(LCDHdlr);
    AmpDisplay_SetWindowActivateFlag(vidDecLcdWinHdlr, 1);
    AmpDisplay_Update(LCDHdlr);
    */
    return 0;
}

/**
 * start decode
 *
 * @param argv
 * @return
 */
int AmpUT_TranscoderDec_start(char** argv)
{
    AMP_AVDEC_TRICKPLAY_s trick = {
            0 };
    AMP_VIDEODEC_DISPLAY_s display = {
            0 };
    char avcFn[80];
    char idxFn[80];
    UINT32 startTime = atoi(argv[5]);
    UINT32 speed = atoi(argv[6]);

    if (speed == 0) {
        speed = 0x100;
    }

    /** Get target file name (XXX.h264¡BXXX.nhnt) */
    if (strcmp(argv[2], "") == 0) {
        AmbaPrint(
                "AmpUT_TranscoderDec_start: please enter index of video file which named 0UT_XXXX.h264 and located in SD root.");
        return 0;
    }
    if (strcmp(argv[3], "") == 0 || strcmp(argv[4], "") == 0) {
        AmbaPrint("AmpUT_TranscoderDec_start: width and height could not be zero");
        return 0;
    }
    AmbaPrint("AmpUT_TranscoderDec_start: target file - %s", argv[2]);
    sprintf(avcFn, "C:\\OUT_%s.h264", argv[2]);
    sprintf(idxFn, "C:\\OUT_%s.nhnt", argv[2]);

    /** Open target file */
    // Open file by dummy demuxer
    AmpUT_TranscoderDec_DummyDmx_open(avcFn, idxFn);

    /** Pre-feed */
    VideoDecFrmCnt = 0x0;
    AmbaPrint("AmpUT_TranscoderDec_start: %d 0x%3x", startTime, speed);
    // Configure feeding speed
    if (speed > 0x100) {
        feedingSpeed = FEED_SPEED_IDR;
        AmpUT_TranscoderDec_DummyDmx_feed(16); // prefeed 16 frames;
    } else {
        feedingSpeed = FEED_SPEED_NOR;
        AmpUT_TranscoderDec_DummyDmx_feed(32); // prefeed 32 frames;
    }
    // Wait until feeding complete
    AmpUT_TranscoderDec_DummyDmx_waitFeedDone();
    AmbaPrint("AmpUT_TranscoderDec_start: feed done");

    /** Configure fading settings */
    if (fadeEnable) {
        AmpUT_TranscoderDec_SetFading();
    }

#if 0
    /** Configure display settings */
    display.SrcWidth = atoi(argv[3]);
    display.SrcHeight = atoi(argv[4]);
    display.AOI.X = 0;
    display.AOI.Y = 0;
    display.AOI.Width = display.SrcWidth;
    display.AOI.Height = display.SrcHeight;

    /** Re-config window */
    // Get display info
    AmpUT_Display_GetInfo(0, &DispInfo);
    // Get window settings
    AmpDisplay_GetDefaultWindowCfg(&WinCfg);
    // Center align the image
    {
        float WidthPxlRatioOnDev, HeightPxlRatioOnDev;
        WidthPxlRatioOnDev = display.SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
        HeightPxlRatioOnDev = display.SrcHeight;
        if (WidthPxlRatioOnDev / HeightPxlRatioOnDev
                > (float) DispInfo.DeviceInfo.VoutWidth / ((float) DispInfo.DeviceInfo.VoutHeight)) {
            // black bar at top and bottom
            winCfg.TargetAreaOnPlane.Width = DispInfo.DeviceInfo.VoutWidth;
            winCfg.TargetAreaOnPlane.Height = (UINT32) ((winCfg.TargetAreaOnPlane.Width * HeightPxlRatioOnDev)
                    / WidthPxlRatioOnDev);
            winCfg.TargetAreaOnPlane.X = 0;
            winCfg.TargetAreaOnPlane.Y = (DispInfo.DeviceInfo.VoutHeight - winCfg.TargetAreaOnPlane.Height) >> 1;
        } else {
            // black bar at left and right
            winCfg.TargetAreaOnPlane.Height = DispInfo.DeviceInfo.VoutHeight;
            winCfg.TargetAreaOnPlane.Width = (UINT32) ((winCfg.TargetAreaOnPlane.Height * WidthPxlRatioOnDev)
                    / HeightPxlRatioOnDev);
            winCfg.TargetAreaOnPlane.Y = 0;
            winCfg.TargetAreaOnPlane.X = (DispInfo.DeviceInfo.VoutWidth - winCfg.TargetAreaOnPlane.Width) >> 1;
        }
    }
    // Configure window settings
    AmpUT_Display_Window_Create(0, &WinCfg);
    // Update window
    AmpUT_Display_Act_Window(0);

    // Get display info
    AmpUT_Display_GetInfo(0, &DispInfo);
    // Get window settings
    AmpDisplay_GetDefaultWindowCfg(&WinCfg);
    // Center align the image
    {
        float WidthPxlRatioOnDev, HeightPxlRatioOnDev;
        WidthPxlRatioOnDev = display.SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
        HeightPxlRatioOnDev = display.SrcHeight;
        if (WidthPxlRatioOnDev / HeightPxlRatioOnDev
                > (float) DispInfo.DeviceInfo.VoutWidth / ((float) DispInfo.DeviceInfo.VoutHeight)) {
            // black bar at top and bottom
            winCfg.TargetAreaOnPlane.Width = DispInfo.DeviceInfo.VoutWidth;
            winCfg.TargetAreaOnPlane.Height = (UINT32) ((winCfg.TargetAreaOnPlane.Width * HeightPxlRatioOnDev)
                    / WidthPxlRatioOnDev);
            winCfg.TargetAreaOnPlane.X = 0;
            winCfg.TargetAreaOnPlane.Y = (DispInfo.DeviceInfo.VoutHeight - winCfg.TargetAreaOnPlane.Height) >> 1;
        } else {
            // black bar at left and right
            winCfg.TargetAreaOnPlane.Height = DispInfo.DeviceInfo.VoutHeight;
            winCfg.TargetAreaOnPlane.Width = (UINT32) ((winCfg.TargetAreaOnPlane.Height * WidthPxlRatioOnDev)
                    / HeightPxlRatioOnDev);
            winCfg.TargetAreaOnPlane.Y = 0;
            winCfg.TargetAreaOnPlane.X = (DispInfo.DeviceInfo.VoutWidth - winCfg.TargetAreaOnPlane.Width) >> 1;
        }
    }
    // Configure window settings
    AmpUT_Display_Window_Create(1, &WinCfg);
    // Update window
    AmpUT_Display_Act_Window(1);
#endif
    /** Configure play settings */
    trick.Speed = speed;
    trick.TimeOffsetOfFirstFrame = startTime;
    if (atoi(argv[7]) == 0) {
        trick.Direction = AMP_VIDEO_PLAY_FW;
    } else {
        trick.Direction = AMP_VIDEO_PLAY_BW;
    }

    /** Play video */
    AmbaPrint("AmpUT_TranscoderDec_start: start start");
    AmpDec_Start(decPipeHdlr, &trick, &display);
    AmbaPrint("AmpUT_TranscoderDec_start: start end");
    return 0;
}

int AmpUT_TranscoderDec_zoom(char** argv)
{
    /** Configure display settings */
    AMP_VIDEODEC_DISPLAY_s display = {
            0 };

    display.SrcWidth = atoi(argv[2]);
    display.SrcHeight = atoi(argv[3]);
    display.AOI.X = atoi(argv[4]);
    display.AOI.Y = atoi(argv[5]);
    display.AOI.Width = atoi(argv[6]);
    display.AOI.Height = atoi(argv[7]);

    /** Play video */
    AmpDec_Start(decPipeHdlr, NULL, &display);
    return 0;
}

int AmpUT_TranscoderDec_trick(void)
{
    AMP_AVDEC_TRICKPLAY_s trick = {
            0 };

    trick.Speed = 0x100;
    trick.TimeOffsetOfFirstFrame = 0;
    trick.Direction = AMP_VIDEO_PLAY_FW;
    AmpDec_Start(decPipeHdlr, &trick, NULL );
    return 0;
}

int AmpUT_TranscoderDec_pause(void)
{
    /** Pause decoding */
    AmpDec_Pause(decPipeHdlr);
    return 0;
}

int AmpUT_TranscoderDec_resume(void)
{
    /** Resume decoding */
    AmpDec_Resume(decPipeHdlr);
    return 0;
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
    0x63};
#endif
int AmpUT_TranscoderDec_cap(int argc,
                            char **argv)
{
    AMP_YUV_BUFFER_s buf;
    UINT8* rawY;
    UINT8* rawUV;
    char mode[] = {'w','b','\0'};

    /** Set target file name (dump.y, dump.uv) */
    char yFn[] = "C:\\dump.y";
    char uvFn[] = "C:\\dump.uv";
    AMP_CFS_FILE_s *yFp, *uvFp;
//AMBA_JPEG_ENC_CONFIG_s jpegEncConfig;
//AMBA_DSP_JPEG_ENC_CTRL_s jpgEncCtrl;

    /** Allocate memory for YUV buffer */
    AmbaPrint("AmpUT_TranscoderDec_cap");
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(buf.LumaAddr), (void**) &rawY, 4608 * 3456, 1 << 6) != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        return -1;
    }
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(buf.ChromaAddr), (void**) &rawUV, 4608 * 3456 >> 1, 1 << 6)
            != AMP_OK) { // Half size in 4:2:0 format
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        return -1;
    }

    /** Configure buffer settings */
    buf.ColorFmt = AMP_YUV_420;
    if (atoi(argv[2]) == 0) {
        buf.Pitch = 1920;
        buf.Width = 1920;
        buf.Height = 1080;
        buf.AOI.X = 0;
        buf.AOI.Y = 0;
        buf.AOI.Width = 1920;
        buf.AOI.Height = 1080;
    } else {
        buf.Pitch = atoi(argv[2]);
        buf.Width = atoi(argv[2]);
        buf.Height = atoi(argv[3]);
        buf.AOI.X = 0;
        buf.AOI.Y = 0;
        buf.AOI.Width = buf.Width;
        buf.AOI.Height = buf.Height;
    }
    /** Capture image */
    AmbaPrint("Cap: %d %d", buf.Width, buf.Height);
    // Dump current frame to the buffer
    AmpVideoDec_DumpFrame(avcDecHdlr, &buf);

    // Since the buffer is cached, the cache needs to be flushed in order to get currect data.
    AmbaCache_Flush(buf.LumaAddr, 4608 * 3456);
    AmbaCache_Flush(buf.ChromaAddr, 4608 * 3456 >> 1);

    /** Save image */
    // Open file
    yFp = AmpDecCFS_fopen(yFn, mode);
    uvFp = AmpDecCFS_fopen(uvFn, mode);
    // Write buffer to the file
    AmpCFS_fwrite(buf.LumaAddr, buf.Height * buf.Pitch, 1, yFp);
    AmpCFS_fwrite(buf.ChromaAddr, (buf.Height * buf.Pitch) >> 1, 1, uvFp); // Half size in 4:2:0 format
    // Close file
    AmpCFS_fclose(yFp);
    AmpCFS_fclose(uvFp);
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
    AmbaKAL_BytePoolFree(rawY);
    AmbaKAL_BytePoolFree(rawUV);
    return 0;
}

int AmpUT_TranscoderDec_stop(void)
{
    AmbaPrint("AmpUT_TranscoderDec_stop");

    /** Close dummy demuxer */
    AmpUT_TranscoderDec_DummyDmx_close();
    AmbaPrint("AmpUT_TranscoderDec_stop 1");
//AmpFifo_EraseAll(avcDecFifoHdlr);
    AmbaPrint("AmpUT_TranscoderDec_stop 2");
    AmpDec_Stop(decPipeHdlr);
    AmbaPrint("AmpUT_TranscoderDec_stop 3");

    return 0;
}

int AmpUT_TranscoderDec_exit(void)
{
    AmbaPrint("AmpUT_TranscoderDec_exit");

    // Deinit fifo handler
    if (avcDecFifoHdlr != NULL ) {
        // Erase data in fifo
        if (AmpFifo_EraseAll(avcDecFifoHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to erase fifo.", __FUNCTION__, __LINE__);
        }

        // Delete fifo
        if (AmpFifo_Delete(avcDecFifoHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to delete fifo.", __FUNCTION__, __LINE__);
        }
        avcDecFifoHdlr = NULL;
    }

    // Deinit video decoder manager
    if (decPipeHdlr != NULL ) {
        if (AmpDec_Stop(decPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to stop the video decoder manager.", __FUNCTION__, __LINE__);
        }
        if (AmpDec_Remove(decPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to remove the video decoder manager.", __FUNCTION__, __LINE__);
        }
        if (AmpDec_Delete(decPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to delete the video decoder manager.", __FUNCTION__, __LINE__);
        }
        decPipeHdlr = NULL;
    }

    // Deinit video decoder
    if (avcDecHdlr != NULL ) {
        if (AmpVideoDec_Delete(avcDecHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to deinit the video decoder.", __FUNCTION__, __LINE__);
        }
        avcDecHdlr = NULL;
    }

    // Release descriptor buffer
    if (avcDescBufOri != NULL ) {
        if (AmbaKAL_BytePoolFree(avcDescBufOri) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the descriptor buffer.", __FUNCTION__, __LINE__);
        }
        avcDescBufOri = NULL;
    }

    // Release raw buffer
    if (avcRawBufOri != NULL ) {
        if (AmbaKAL_BytePoolFree(avcRawBufOri) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the raw buffer.", __FUNCTION__, __LINE__);
        }
        avcRawBuf = NULL;
        avcRawBufOri = NULL;
    }

    //AmbaKAL_BytePoolFree(avcCodecBufOri);
    //avcCodecBufOri = NULL;

    //AmpUT_TranscoderDec_DummyDmx_close();

    return 0;
}

int AmpUT_TranscoderDec_fade(char** argv)
{
    /** Configure fading settings */
    AmbaPrint("AmpUT_TranscoderDec_fade");

    fadeEnable = atoi(argv[2]);
    if (fadeEnable == 0) {
        AmbaPrint("AmpUT_TranscoderDec_fade: disable");
    } else {
        AmbaPrint("AmpUT_TranscoderDec_fade: enable");
    }

    return 0;
}

int AmpUT_TranscoderDec_usage(void)
{
    AmbaPrint("AmpUT_TranscoderDec");
    AmbaPrint("\t init");
    AmbaPrint("\t start [fileIndex] [videoWidth] [videoHeight] [startPTS] [Speed] [Dir] - start decode");
    AmbaPrint(
            "\t zoom [videoWidth] [videoHeight] [cropOffsetX] [cropOffsetY] [cropWidth] [cropHeight] - crop part of video");
    AmbaPrint("\t pause - pause decode");
    AmbaPrint("\t resume - resume decode");
    AmbaPrint("\t stop - stop decode");
    AmbaPrint("\t cap [width] [height] - capture YUV during pause");
    AmbaPrint("\t fade [enable] - 1: enable 0: disable");
    return 0;
}

int AmpUT_TranscoderDecTest(struct _AMBA_SHELL_ENV_s_ *env,
                            int argc,
                            char **argv)
{
    AmbaPrint("AmpUT_TranscoderDecTest cmd: %s", argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_TranscoderDec_init();
    } else if (strcmp(argv[1], "start") == 0) {
        AmpUT_TranscoderDec_start(argv);
    } else if (strcmp(argv[1], "pause") == 0) {
        AmpUT_TranscoderDec_pause();
    } else if (strcmp(argv[1], "resume") == 0) {
        AmpUT_TranscoderDec_resume();
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_TranscoderDec_stop();
    } else if (strcmp(argv[1], "cap") == 0) {
        AmpUT_TranscoderDec_cap(argc, argv);
    } else if (strcmp(argv[1], "exit") == 0) {
        AmpUT_TranscoderDec_exit();
    } else if (strcmp(argv[1], "zoom") == 0) {
        AmpUT_TranscoderDec_zoom(argv);
    } else if (strcmp(argv[1], "fade") == 0) {
        AmpUT_TranscoderDec_fade(argv);
    } else if (strcmp(argv[1], "ch") == 0) {
        AmpUT_TranscoderDec_configDisplayCh(argv);
    } else {
        AmpUT_TranscoderDec_usage();
    }
    return 0;
}

int AmpUT_TranscoderDecInit(void)
{
// hook command
    AmbaTest_RegisterCommand("vtrn", AmpUT_TranscoderDecTest);

    return AMP_OK;
}
