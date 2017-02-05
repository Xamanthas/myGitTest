/**
 *  @file AmpUT_NetPbFifo.c
 *
 * Copyright (C) 2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <net/NetFifo.h>
#include <player/Decode.h>
#include <player/VideoDec.h>
#include <transcoder/DecTranscoder.h>
#include "AmpUnitTest.h"
#include <AmbaUtility.h>
#include <util.h>
#include "AmbaCache.h"
#include "DecCFSWrapper.h"
#include <format/FormatDef.h>

#define VIDEODEC_RAW_SIZE (10<<20) ///< 10MB raw buffer
#define VIDEODEC_RAW_DESC_NUM (128) ///< descriptor number = 128
#define BS_FN   "C:\\OUT_0000.h264" /**< H264 bitstream file name */
#define HDR_FN  "C:\\OUT_0000.nhnt" /**< nhnt file name */
#define UDTA_FN "C:\\OUT_0000.udta" /**< user define data file name */
#define AMPUT_NETPBFIFO_MEDIA_STREAMID (0xabcd)

static char bs_fn[32] = {0};
static char hdr_fn[32] = {0};
static char udta_fn[32] = {0};
// global var
//static void* avcCodecBufOri = NULL; ///< Original buffer address of video codec
static void* avcRawBufOri = NULL;   ///< Original buffer address of video decode raw file
static void* avcRawBuf = NULL;      ///< Aligned buffer address of video decode raw file
static void* avcDescBufOri = NULL;  ///< Original buffer address of video descripter
static AMP_AVDEC_HDLR_s *avcDecHdlr = NULL;
static AMP_DEC_PIPE_HDLR_s *decPipeHdlr = NULL;
static AMP_FIFO_HDLR_s *avcDecFifoHdlr = NULL;

//static AMP_FIFO_HDLR_s *FakeCodecFifoHdlr = NULL;

static AMP_AVDEC_HDLR_s *DecTransCodecHdlr = NULL;
static void* decTransCodecBufOri = NULL; ///< Original buffer address of video codec
static void* DecTransRawBufOri = NULL;   ///< Original buffer address of video decode raw file
static void* DecTransRawBuf = NULL;      ///< Aligned buffer address of video decode raw file

static FORMAT_USER_DATA_s g_Udta; /**< user define data */
static UINT8 pb_status = 0;

// dmy dmx
static AMBA_KAL_TASK_t VideoDecDemuxTask = {
        0 };
static AMBA_KAL_MUTEX_t VideoDecFrmMutex = {
        0 };
static UINT32 VideoDecFrmCnt = 0;
static AMP_CFS_FILE_s *avcIdxFile = NULL;
static AMP_CFS_FILE_s *avcRawFile = NULL;

#define AMP_UT_VIDEODEC_STACK_SIZE (16<<10)
static char AmpUT_NetPbFifoStack[AMP_UT_VIDEODEC_STACK_SIZE];
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
static UINT8 pauseFlag = 0;
void AmpUT_NetPbFifo_DemuxTask(UINT32 info)
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

    AmbaPrint("AmpUT_NetPbFifo_DemuxTask Start!");
    // raw file used format nhnt
    // check http://gpac.wp.mines-telecom.fr/mp4box/media-import/nhnt-format/ for more info
    // we could get it by using MP4box ( -nhnl )
    while (1) {
//AmbaPrint("AmpUT_NetPbFifo_DemuxTask Start  1");
        AmbaKAL_TaskSleep(1);
        AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
        if (VideoDecFrmCnt == 0 || VideoDecFrmCnt == 0xFFFFFFFF || avcRawFile == NULL) {
//AmbaPrint("AmpUT_NetPbFifo_DemuxTask Start  2 - %d 0x%x", VideoDecFrmCnt, avcRawFile);
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
int AmpUT_NetPbFifo_DummyDmx_FifoCB(void *hdlr,
                                        UINT32 event,
                                        void* info)
{

     AmbaPrint("AmpUT_NetPbFifo_FifoCB on Event: 0x%x 0x%x", event, AMP_FIFO_EVENT_DATA_READY);
    /* if (event == AMP_FIFO_EVENT_DATA_CONSUMED){
     AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER);
     //VideoDecFrmCnt -= (*((int*)info));
     //AmbaPrint("AmpUT_NetPbFifo_FifoCB VideoDecFrmCnt: %d", VideoDecFrmCnt);
     AmbaKAL_MutexGive(&VideoDecFrmMutex);
     }
     */
    return 0;
}

/**
 * Initialize dummy demux.
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_NetPbFifo_DummyDmx_init(void)
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
    fifoDefCfg.cbEvent = AmpUT_NetPbFifo_DummyDmx_FifoCB;
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
    AmpUT_NetPbFifo_DemuxTask, /* void (*EntryFunction)(UINT32) */
    0x0, /* EntryArg */
    (void *) AmpUT_NetPbFifoStack, /* pStackBase */
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


#if 1
int AmpUT_NetPbFifo_start(char *filename, int width, int height, UINT32 startTime, UINT32 speed, int direction);
int AmpUT_NetPbFifo_stop(void);
int AmpUT_NetPbFifo_pause(void);
int AmpUT_NetPbFifo_resume(void);

/**
 * callback function to handle NetFifo event.
 */
static int AmpUT_NetPbFifo_EventCB(void *hdlr, UINT32 event, void* info)
{
    int rval = 0;

    AmbaPrint("%s: Got NetFifo Event 0x%08x.", __FUNCTION__,event);

    return rval;
}

/**
 * Return Stream ID list, not used in file playback streaming
 */
static int AmpUT_NetPbFifo_GetStreamIDList(AMP_NETFIFO_MEDIA_STREAMID_LIST_s *slist)
{
    int rval = 0;

    if (slist == NULL) {
        AmbaPrint("%s: invalid StreamID List output buffer.(NULL)", __FUNCTION__);
        return -1;
    }

    slist->Amount = 0; //only 1 stream
    slist->StreamID_List[0] = 0;

    return rval;
}

/**
 * Return Media Ifno for dedicated Stream ID.
 */
static int AmpUT_NetPbFifo_GetMediaInfo(int StreamID, AMP_NETFIFO_MOVIE_INFO_CFG_s *media_info)
{
    int rval = -1;

    AmbaPrint("%s: request MediaInfo for Stream 0x%08x.", __FUNCTION__,StreamID);

    if(StreamID != AMPUT_NETPBFIFO_MEDIA_STREAMID) {
        AmbaPrint("%s: Only NetPBFifo StreamID is supported", __FUNCTION__);
        return -1;
    }

    // prepare media info output buffer
    if(media_info == NULL){
        AmbaPrint("%s: invalid MediaInfo output buffer.(NULL)", __FUNCTION__);
        return -1;
    }
    rval = AmpNetFifo_GetDefaultMovieInfoCfg(media_info);

    // fill media info
    if(rval == 0) {
        media_info->nTrack = 1;
        media_info->Track[0].hCodec = (void*) AmpUT_NetPbFifo_EventCB;
        media_info->Track[0].nTrackType = AMP_NETFIFO_MEDIA_TRACK_TYPE_VIDEO;
        media_info->Track[0].pBufferBase = (UINT8 *)DecTransRawBuf;
        media_info->Track[0].pBufferLimit = (UINT8 *)DecTransRawBuf + VIDEODEC_RAW_SIZE;
        media_info->Track[0].Info.Video.bDefault = TRUE;
        media_info->Track[0].nMediaId = AMP_FORMAT_MID_AVC;
        media_info->Track[0].nTimeScale = g_Udta.nTimeScale;//atoi(Rate);
        media_info->Track[0].nTimePerFrame = g_Udta.nTickPerPicture;//atoi(Scale);
        media_info->Track[0].Info.Video.nMode = AMP_VIDEO_MODE_P;
        media_info->Track[0].Info.Video.nM = g_Udta.nM;//atoi(M);
        media_info->Track[0].Info.Video.nN = g_Udta.nN;//atoi(N);
        media_info->Track[0].Info.Video.nGOPSize = (UINT32)media_info->Track[0].Info.Video.nN * g_Udta.nIdrInterval;//atoi(idrItvl);
        media_info->Track[0].Info.Video.nCodecTimeScale = g_Udta.nTimeScale;    // TODO: should remove nCodecTimeScale, but the sample clip uses 90K
        media_info->Track[0].Info.Video.nWidth = g_Udta.nVideoWidth;//atoi(Width);
        media_info->Track[0].Info.Video.nHeight = g_Udta.nVideoHeight;//atoi(Height);
        media_info->Track[0].Info.Video.nTrickRecDen = media_info->Track[0].Info.Video.nTrickRecNum = 1;
    }
    AmbaPrint("hCodec is %p\n\r", media_info->Track[0].hCodec);
    return rval;
}

/**
 * callback function to handle get MediaInfo request.
 */
static int AmpUT_NetPbFifo_MediaInfoCB(void *hdlr, UINT32 event, void* info)
{
    int rval = 0;
    int sid;
    unsigned int cmd;

    cmd = AMP_NETFIFO_GET_MEDIA_CMD(event);
    switch(cmd) {
    case AMP_NETFIFO_MEDIA_CMD_GET_STREAM_LIST:
        rval = AmpUT_NetPbFifo_GetStreamIDList((AMP_NETFIFO_MEDIA_STREAMID_LIST_s *)info);
        break;
    case AMP_NETFIFO_MEDIA_CMD_GET_INFO:
        sid = AMP_NETFIFO_GET_MEDIA_PARAM(event);
        rval = AmpUT_NetPbFifo_GetMediaInfo(sid,(AMP_NETFIFO_MOVIE_INFO_CFG_s *)info);
        break;
    default:
        AmbaPrint("%s: unsupported cmd 0x04%x, event:0x%08x", __FUNCTION__,
                cmd, event);
        break;
    }

    return rval;
}

/**
 * callback function to handle playback request.
 */
static int AmpUT_NetPbFifo_PlaybackCB(void *hdlr, UINT32 event, void* info)
{
    int rval = 0;
    char *fn;
    UINT32 *u_ptr;

    AmbaPrint("%s: Got NetFifo Playback request 0x%08x.", __FUNCTION__,event);

    switch(event){
    case AMP_NETFIFO_PLAYBACK_OPEN:
    /* rval: streamId > 0 for success, -1 for fail */
        fn = (char *)info;
        if(fn == NULL){
            AmbaPrint("%s[%d]: invalid parameter!",__FUNCTION__,__LINE__);
            rval = -1;
        } else {
            // Read UDTA file
            AMP_CFS_STAT stat = {0};
            AMP_CFS_FILE_PARAM_s cfsParam = {0};
            AMP_CFS_FILE_s *hUdtaFile = NULL;
            AmbaPrint("AMP_NETFIFO_PLAYBACK_OPEN filename %s",fn);
            sprintf(bs_fn, BS_FN);
            sprintf(hdr_fn, HDR_FN);
            sprintf(udta_fn, UDTA_FN);
            rval = AmpCFS_Stat(udta_fn, &stat);
            if (rval == AMP_OK) {
                AmpCFS_GetFileParam(&cfsParam);
                cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
                strcpy(cfsParam.Filename, udta_fn);
                hUdtaFile = AmpCFS_fopen(&cfsParam);
                if (hUdtaFile == NULL) {
                    AmbaPrint("Udta Open fail");
                    rval = -1;
                    break;
                } else {
                    AmpCFS_fread(&g_Udta, 1, sizeof(g_Udta), hUdtaFile);
                    AmpCFS_fclose(hUdtaFile);
                }
                /* In file-playback streaming, PB_OPEN should return the streamID of playback */
                rval = AMPUT_NETPBFIFO_MEDIA_STREAMID;
                pb_status = 1;
            } else {
                AmbaPrint("%s: fail to read UDTA file!(%d)", __FUNCTION__,rval);
                rval = -1;
            }
        }
        break;
    case AMP_NETFIFO_PLAYBACK_PLAY:
    /* rval: 0 for success, -1 for fail */
        u_ptr = (UINT32 *)info;

        if(u_ptr == NULL) {
            AmbaPrint("AMP_NETFIFO_PLAYBACK_PLAY from beginning!");
            rval = 0;
        } else {
            AmbaPrint("AMP_NETFIFO_PLAYBACK_PLAY from %d second",*u_ptr);
            rval = *u_ptr;
        }

        if(pb_status == 1) { //opened
            AmpUT_NetPbFifo_start(bs_fn, 0, 0, rval, 0, AMP_VIDEO_PLAY_FW);
            pb_status = 2;
            rval = 0;
        } else {
            AmbaPrint("file not opened!!(status=%u)",pb_status);
            rval = -1;
        }
        break;
    case AMP_NETFIFO_PLAYBACK_STOP:
    /* rval: 0 for success, -1 for fail */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_STOP!");
        if (pb_status == 2) { //playing
            AmpUT_NetPbFifo_stop();
            pb_status = 0;
        }
        rval = 0;
        break;
    case AMP_NETFIFO_PLAYBACK_RESET:
    /* rval: 0 for success, -1 for fail */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_RESET!");
        if (pb_status == 1) { //opened, but client leave before send PLAY command
            AmpUT_NetPbFifo_stop();
            pb_status = 0;
        }
        rval = 0;
        break;
    case AMP_NETFIFO_PLAYBACK_PAUSE:
    /* rval: 0 for success, -1 for fail */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_PAUSE!");
        AmpUT_NetPbFifo_pause();
        rval = 0;
        break;
    case AMP_NETFIFO_PLAYBACK_RESUME:
    /* rval: 0 for success, -1 for fail */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_RESUME!");
        AmpUT_NetPbFifo_resume();
        rval = 0;
        break;
    case AMP_NETFIFO_PLAYBACK_CONFIG:
    /* rval: 0 for success, -1 for fail */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_CONFIG!");
        rval = 0;
        break;
    case AMP_NETFIFO_PLAYBACK_GET_VID_FTIME:
    /* rval: Time per video frame in ms */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_GET_VID_FTIME!");
        rval = (1000 * g_Udta.nTickPerPicture)/g_Udta.nTimeScale;
        AmbaPrint("return %d!",rval);
        break;
    case AMP_NETFIFO_PLAYBACK_GET_VID_TICK:
    /* rval: Tick per video frame */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_GET_VID_TICK!");
        rval = (90000*g_Udta.nTickPerPicture)/g_Udta.nTimeScale;
        AmbaPrint("return %d!",rval);
        break;
    case AMP_NETFIFO_PLAYBACK_GET_AUD_FTIME:
    /* rval: Time per audio frame in ms */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_GET_AUD_FTIME!");
        rval = 21;
        break;
    case AMP_NETFIFO_PLAYBACK_GET_DURATION:
    /* rval: File duration in ms. 0 to disable seek function. */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_GET_DURATION!");
        rval = 0;
        break;
    case AMP_NETFIFO_PLAYBACK_SET_LIVE_BITRATE:
    /* rval: 0 for success, -1 for fail */
    /* param is bitrate in bps */
        u_ptr = (UINT32 *)info;

        if(u_ptr == NULL) {
            AmbaPrint("AMP_NETFIFO_PLAYBACK_SET_LIVE_BITRATE: invalid param!");
            rval = -1;
        } else {
            AmbaPrint("AMP_NETFIFO_PLAYBACK_SET_LIVE_BITRATE: wanted %d!",u_ptr[0]);
            rval = 0;
        }
        break;
    case AMP_NETFIFO_PLAYBACK_GET_LIVE_BITRATE:
    /* rval: runtime bitrate for Live streaming in bps*/
        AmbaPrint("AMP_NETFIFO_PLAYBACK_GET_LIVE_BITRATE!");
        rval = 12000000;
        break;
    case AMP_NETFIFO_PLAYBACK_GET_LIVE_AVG_BITRATE:
    /* rval: avg bitrate for Live streaming in bps*/
        AmbaPrint("AMP_NETFIFO_PLAYBACK_GET_LIVE_AVG_BITRATE!");
        rval = 12000000;
        break;
    case AMP_NETFIFO_PLAYBACK_SET_NET_BANDWIDTH:
    /* rval: 0 for success, -1 for fail */
    /* param is bandwidth in bps */
        u_ptr = (UINT32 *)info;

        if(u_ptr == NULL) {
            AmbaPrint("AMP_NETFIFO_PLAYBACK_SET_NET_BANDWIDTH: invalid param!");
            rval = -1;
        } else {
            AmbaPrint("AMP_NETFIFO_PLAYBACK_SET_NET_BANDWIDTH: wanted %d!",u_ptr[0]);
            rval = 0;
        }
        rval = 0;
        break;
    default:
        AmbaPrint("%s: Unsuoported event 0x%08x",__FUNCTION__,event);
        rval = -1;
        break;
    }

    return rval;
}
#endif


int AmpUT_NetFakeCodec_Init(void)
{
//    AMP_FIFO_CFG_s fifoDefCfg = {0};
    AMP_NETFIFO_INIT_CFG_s init_cfg;
    static UINT8 init = 0;
    int er = 0;

    if (init != 0) {
        return 0;
    }

    //init AmpNetFifo
    AmpNetFifo_GetInitDefaultCfg(&init_cfg);
    init_cfg.cbEvent = AmpUT_NetPbFifo_EventCB;
    init_cfg.cbMediaInfo = AmpUT_NetPbFifo_MediaInfoCB;
    init_cfg.cbPlayback = AmpUT_NetPbFifo_PlaybackCB;

    er = AmpNetFifo_init(&init_cfg);
    if(er != AMP_OK) {
        AmbaPrint("%s: Fail to AmpNetFifo_init(),%d",__FUNCTION__,er);
        return er;
    }

    init = 1;

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
int AmpUT_NetPbFifo_DummyDmx_open(char* rawFn,
                                      char* idxFn)
{
    char mode[3] = "rb";

    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
    // open file
    avcRawFile = AmpDecCFS_fopen(rawFn, mode);
    if (avcRawFile == NULL ) {
        AmbaPrint("AmpUT_NetPbFifo_init: raw open failed");
    }
    avcIdxFile = AmpDecCFS_fopen( idxFn, mode);
    if (avcIdxFile == NULL ) {
        AmbaPrint("AmpUT_NetPbFifo_init: idx open failed");
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
int AmpUT_NetPbFifo_DummyDmx_close(void)
{
    AmbaPrint("AmpUT_NetPbFifo_DummyDmx_close");
    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
    AmbaPrint("AmpUT_NetPbFifo_DummyDmx_close 1");
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
int AmpUT_NetPbFifo_DummyDmx_TimeSearch(UINT32 timeStart)
{
    NHNT_HEADER_s nhntHeader = {
            0 };
    UINT32 resolution = 90000;

    AmbaPrint("AmpUT_NetPbFifo_DummyDmx_TimeSearch");
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
    return 0;
}

/**
 * Request dummy demux to feed frames.
 *
 * @param [in] req          Number of frames requested to feed
 *
 * @return 0 - OK, others - Error
 */
int AmpUT_NetPbFifo_DummyDmx_feed(int req)
{
    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
    /* We don't feed any frame when pauseFlag is on
     * Due to that AmpUT_NetPbFifoCB(call back for TRANSCODER CODEC) is more frequent
     * than DemuxTask read frame, we need to restrict the number of frame we feed.
     * Otherwise, DemuxTask will keep read frame even we pause feeding frame.
     */
    if (VideoDecFrmCnt != 0xFFFFFFFF && pauseFlag == 0 && VideoDecFrmCnt < 20) {
        AmbaPrint("Feed %d frames\n", req);
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
int AmpUT_NetPbFifo_DummyDmx_waitFeedDone(void)
{
    while (VideoDecFrmCnt != 0 && VideoDecFrmCnt!=0xFFFFFFFF) {
        AmbaKAL_TaskSleep(10);
        AmbaPrint("AmpUT_NetPbFifo_DummyDmx_waitFeedDone: %d", VideoDecFrmCnt);
    }
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
int AmpUT_NetPbFifoCB(void* hdlr,
                           UINT32 event,
                           void* info)
{
    if (event == AMP_DEC_EVENT_DATA_UNDERTHRSHOLDD && VideoDecFrmCnt != 0xFFFFFFFF) {
        AmpUT_NetPbFifo_DummyDmx_feed(4); // feed 4 frames;
    }
    return 0;
}

/**
 * init transcoder
 *
 * @return
 */
int AmpUT_NetPbFifo_init(void)
{
    //AMP_VIDEODEC_CFG_s codecCfg;
    //AMP_VIDEODEC_INIT_CFG_s codecInitCfg;
    //AMP_DEC_PIPE_CFG_s pipeCfg;

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

    {
        AMP_DEC_TRANSCODER_CFG_s TransCfg;
        AmpDecTranscoder_GetDefaultCfg(&TransCfg);
        TransCfg.RawBufferSize = VIDEODEC_RAW_SIZE;
        if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &DecTransRawBuf, &DecTransRawBufOri, TransCfg.RawBufferSize,
                1 << 5) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        }
        TransCfg.RawBuffer = DecTransRawBuf;
        TransCfg.CbTranscode = AmpUT_NetPbFifoCB;
        TransCfg.DstCodec = (AMP_AVDEC_HDLR_s*)AmpUT_NetPbFifo_EventCB; //function pointer as codec.
        TransCfg.DstCodecType = 1;
        AmpDecTranscoder_Create(&TransCfg, &DecTransCodecHdlr);
        AmbaPrint("DecTransCodecHdlr = 0x%x Raw@ 0x%x - 0x%x", DecTransCodecHdlr, DecTransRawBuf, TransCfg.RawBufferSize + DecTransRawBuf - 1);
    }

    /** Dummy mux */
    // Create a dummy demuxer
    AmpUT_NetPbFifo_DummyDmx_init();
    AmpUT_NetFakeCodec_Init();

    return 0;
}

/**
 * start demux
 *
 */
int AmpUT_NetPbFifo_start(char *filename, int width, int height, UINT32 startTime, UINT32 speed, int direction)
{
    char avcFn[80];
    char idxFn[80];

    if (speed == 0) {
        speed = 0x100;
    }

    /** Get target file name (XXX.h264¡BXXX.nhnt) */
    if ((filename==NULL) || (strcmp(filename, "") == 0)) {
        AmbaPrint("%s: invalid filename.",__FUNCTION__);
        return 0;
    }

    sprintf(avcFn, bs_fn);
    sprintf(idxFn, hdr_fn);

    /** Open target file */
    // Open file by dummy demuxer
    AmpUT_NetPbFifo_DummyDmx_open(avcFn, idxFn);

    /** Pre-feed */

    AmbaPrint("AmpUT_NetPbFifo_start: %d 0x%3x", startTime, speed);
    // Configure feeding speed
    if (speed > 0x100) {
        feedingSpeed = FEED_SPEED_IDR;
        AmpUT_NetPbFifo_DummyDmx_feed(16); // prefeed 16 frames;
    } else {
        feedingSpeed = FEED_SPEED_NOR;
        AmpUT_NetPbFifo_DummyDmx_feed(32); // prefeed 32 frames;
    }

    return 0;
}

int AmpUT_NetPbFifo_trick(void)
{
    return 0;
}

int AmpUT_NetPbFifo_pause(void)
{
    /** Pause demuxing */
    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
    pauseFlag = 1;
    AmbaKAL_MutexGive(&VideoDecFrmMutex);
    return 0;
}

int AmpUT_NetPbFifo_resume(void)
{
    /** Resume demuxing */
    AmbaKAL_MutexTake(&VideoDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
    pauseFlag = 0;
    AmbaKAL_MutexGive(&VideoDecFrmMutex);
    /* The callback from TRANSCODER CODEC has already stopped. We need to manually feed to resume */
    AmpUT_NetPbFifo_DummyDmx_feed(4);
    return 0;
}

int AmpUT_NetPbFifo_stop(void)
{
    AmbaPrint("AmpUT_NetPbFifo_stop");

    /** Close dummy demuxer */
    AmpUT_NetPbFifo_DummyDmx_close();
    return 0;
}

int AmpUT_NetPbFifo_exit(void)
{
    AmbaPrint("AmpUT_NetPbFifo_exit");

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

    //AmpUT_NetPbFifo_DummyDmx_close();

    return 0;
}

int AmpUT_NetPbFifo_usage(void)
{
    AmbaPrint("AmpUT_NetPbFifo");
    AmbaPrint("\t init");
    return 0;
}

int AmpUT_NetPbFifoTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_NetPbFifo cmd: %s", argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_NetPbFifo_init();
    } else {
        AmpUT_NetPbFifo_usage();
    }
    return 0;
}

int AmpUT_NetPbFifo_TestAdd(void)
{
// hook command
    AmbaTest_RegisterCommand("netpbfifo", AmpUT_NetPbFifoTest);

    return AMP_OK;
}
