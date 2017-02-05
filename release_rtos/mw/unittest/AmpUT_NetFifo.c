/**
 *  @file AmpUT_NetFifo.c
 *
 * Copyright (C) 2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <net/NetFifo.h>
#include <recorder/Encode.h>
#include "AmpUnitTest.h"
#include "AmbaUtility.h"
#include <AmbaCache_Def.h>
#include "DummyVideoEnc_Net.h"
#include <util.h>
#include <net/NetUtility.h>
#include <AmbaTimer.h>

#define BS_FN   "C:\\OUT_0000.h264" /**< H264 bitstream file name */
#define HDR_FN  "C:\\OUT_0000.nhnt" /**< nhnt file name */
#define UDTA_FN "C:\\OUT_0000.udta" /**< user define data file name */
#define AMPUT_NETFIFO_CODEC_HDLR ((void *)0x5678) /**< codec handler */
#define AMPUT_NETFIFO_ENC_RAW_SIZE (10 << 20) /**< 10MB raw buffer */
#define AMPUT_NETFIFO_MAX_FIFO_NUM (256) /**< number of entry of FIFO */

#define AMPUT_NETFIFO_MEDIA_STREAMID (0x0101)

static void *g_EncBuffer = NULL; /**< Encode buffer */
static AMP_FIFO_HDLR_s *g_EncFifoHdlr = NULL; /**< encode FIFO handler */
static AMP_FIFO_HDLR_s *g_DefFifoHdlr = NULL; /**< default AMP_NETFIFO handler */
static FORMAT_USER_DATA_s g_Udta; /**< user define data */
static DummyVideoEnc_Net_s g_Encoder; /**< video encoder */
static int g_EncStatus = 0;
static UINT8 g_RTSPStatus = 0;
static UINT8 AmpUT_NetFifoInited = 0;


/**
 * video encoder event callback function.
 */
static int DummyVideoEnc_Net_EventCB(void *pHdlr, UINT32 nEvent, void *pInfo)
{
    if (nEvent == AMP_FIFO_EVENT_DATA_CONSUMED) {
        if (RingBuf_DeleteFrame(&g_Encoder.RingBuf) == -1)
             AmbaPrint("RingBuf_DeleteFrame Fail");
    }

    return 0;
}

/**
 * Dummy encoder reset function.
 */
static int AmpUT_NetFifo_rec_Reset(void)
{
    int rval = -1;

    if(g_EncFifoHdlr != NULL) {
        rval = DummyVideoEnc_Net_Reset(&g_Encoder, g_EncFifoHdlr);
    }

    return rval;
}

/**
 * Dummy encoder start function.
 */
static int AmpUT_NetFifo_rec_start(void)
{
    int rval = -1;

    rval = DummyVideoEnc_Net_QueryState(&g_Encoder);
    if(rval < 0) {
        AmbaPrint("%s: Fail to do DummyVideoEnc_Net_QueryState. rval=%d",__FUNCTION__,rval);
        return rval;
    }

    if(rval == ENCODER_STATE_RUNNING) {
        AmbaPrint("Already Encoding...");
        return 0;
    }

    if(g_EncStatus == 1) { //previous stop due to EOS
        AmbaPrint("previous stop due to EOS...");
        rval = AmpNetFifo_SendNotify(AMP_NETFIFO_NOTIFY_STOPENC, 0, 0);
        if(rval != 0){
            AmbaPrint("%s: Fail to AmpNetFifo_SendNotify().%d", __FUNCTION__,rval);
        }
    }

    //Reset DummyVideoEnc_Net
    rval = AmpUT_NetFifo_rec_Reset();
    if(rval < 0){
        return rval;
    }

    rval = DummyVideoEnc_Net_Start(&g_Encoder);
    if(rval == 0){
        rval = AmpNetFifo_SendNotify(AMP_NETFIFO_NOTIFY_STARTENC, 0, 0);
        if(rval != 0){
            AmbaPrint("%s: Fail to AmpNetFifo_SendNotify().%d", __FUNCTION__,rval);
        }
        g_EncStatus = 1; //encoding
    }

    return rval;
}

/**
 * Dummy encoder stop function.
 */
static int AmpUT_NetFifo_rec_stop(void)
{
    int rval = -1;

    if (g_EncStatus == 0) {
        AmbaPrint("Already Stopped...");
        return 0;
    }

    rval = DummyVideoEnc_Net_Stop(&g_Encoder);
    if (rval == 0) {
        rval = AmpNetFifo_SendNotify(AMP_NETFIFO_NOTIFY_STOPENC, 0, 0);
        if(rval != 0){
            AmbaPrint("%s: Fail to AmpNetFifo_SendNotify().%d", __FUNCTION__,rval);
        }
        g_EncStatus = 0; //idle
    }

    return rval;
}

#if 0
/**
 * Dummy callback function for CFS. This move to SystemInitTaskEntry.
 */
static int AmpUT_NetFifo_CfsCB(int opCode, UINT32 param)
{
    return AMP_OK;
}

/**
 * Init CFS module(for Dummy encoder). This move to SystemInitTaskEntry.
 */
static int AmpUT_NetFifo_CfsInit(void)
{
    AMP_CFS_CFG_s cfsCfg;
    static void *pCfsBuf = NULL;
    int rval = AMP_ERROR_GENERAL_ERROR;

    if (AmpCFS_GetDefaultCfg(&cfsCfg) != AMP_OK) {
        AmbaPrint("%s: Can't get default config.", __FUNCTION__);
        return rval;
    }
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&cfsCfg.Buffer, &pCfsBuf, cfsCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s: Can't allocate memory.", __FUNCTION__);
        return rval;
    }
    cfsCfg.FileOperation = AmpUT_NetFifo_CfsCB;
    if (AmpCFS_Init(&cfsCfg) != AMP_OK) {
        AmbaPrint("%s: Can't initialize.", __FUNCTION__);
        return rval;
    }

    return AMP_OK;
}
#endif

/**
 * callback function to handle NetFifo event.
 */
static int AmpUT_NetFifo_EventCB(void *hdlr, UINT32 event, void* info)
{
    int rval = 0;

    AmbaPrint("%s: Got NetFifo Event 0x%08x.", __FUNCTION__,event);

    return rval;
}

/**
 * Return Stream ID list.
 */
static int AmpUT_NetFifo_GetStreamIDList(AMP_NETFIFO_MEDIA_STREAMID_LIST_s *slist)
{
    int rval = 0;

    if (slist == NULL) {
    	AmbaPrint("%s: invalid StreamID List output buffer.(NULL)", __FUNCTION__);
        return -1;
    }

    if (g_EncStatus == 1) { //encoding
        slist->Amount = 1; //only 1 stream
        slist->StreamID_List[0] = AMPUT_NETFIFO_MEDIA_STREAMID;
    } else { //idle
        slist->Amount = 0;
        slist->StreamID_List[0] = 0;
    }

    return rval;
}

/**
 * Return Media Ifno for dedicated Stream ID.
 */
static int AmpUT_NetFifo_GetMediaInfo(int StreamID, AMP_NETFIFO_MOVIE_INFO_CFG_s *media_info)
{
    int rval = -1;

    AmbaPrint("%s: request MediaInfo for Stream 0x%08x.", __FUNCTION__,StreamID);

    //Only CH0_Primary stream is supported for this test
    if(StreamID != AMPUT_NETFIFO_MEDIA_STREAMID) {
        AmbaPrint("%s: Only first StreamID is supported", __FUNCTION__);
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
        media_info->Track[0].hCodec = AMPUT_NETFIFO_CODEC_HDLR;
        media_info->Track[0].nTrackType = AMP_NETFIFO_MEDIA_TRACK_TYPE_VIDEO;
        media_info->Track[0].pBufferBase = (UINT8 *)g_EncBuffer;
        media_info->Track[0].pBufferLimit = (UINT8 *)g_EncBuffer + AMPUT_NETFIFO_ENC_RAW_SIZE;
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

    return rval;
}

/**
 * callback function to handle get MediaInfo request.
 */
static int AmpUT_NetFifo_MediaInfoCB(void *hdlr, UINT32 event, void* info)
{
    int rval = 0;
    int sid;
    unsigned int cmd;

    cmd = AMP_NETFIFO_GET_MEDIA_CMD(event);
    switch(cmd) {
    case AMP_NETFIFO_MEDIA_CMD_GET_STREAM_LIST:
        rval = AmpUT_NetFifo_GetStreamIDList((AMP_NETFIFO_MEDIA_STREAMID_LIST_s *)info);
        break;
    case AMP_NETFIFO_MEDIA_CMD_GET_INFO:
        sid = AMP_NETFIFO_GET_MEDIA_PARAM(event);
        rval = AmpUT_NetFifo_GetMediaInfo(sid,(AMP_NETFIFO_MOVIE_INFO_CFG_s *)info);
        break;
    default:
        AmbaPrint("%s: unsupported cmd 0x04%x, event:0x%08x", __FUNCTION__,
                cmd, event);
        break;
    }

    return rval;
}

/*
 *  Check the statistic of RR and change bitrate accordingly
 */
typedef struct rr_report_s
{
    unsigned int fr_lost;
    unsigned int jitter;
    double pg_delay;
} rr_report_t;

typedef struct rr_stat_s
{
    unsigned int curBitRate;    //indicate the current bitrate
    unsigned int maxStableBR;   //max stable bitrate
    unsigned int lastBitRate;   //indicate the bitrate when last RR come
    unsigned int maxBitRate;    //the max bitrate we can reach in this sensor_config
    unsigned int netBandwidth;  //network bandwidth
    unsigned int zeroLost;      //indicate how many consecutive fr_lost=0 we got
    unsigned int lastFrLost;    //fr_lost of last RR
    UINT8 inMiddleOfIncrement;
    UINT8 inMiddleOfDecrement;
    UINT8 inited;              //indicate that whether some value is inited or not
} rr_stat_t;

static rr_stat_t RRStat;
#define INCREMENT_STEP 20      //brate raise of each step when receive consecutive zero fr_lost
#define MAX_STATBLE_COUNT_RAISE 30 //max stable zero fr_lost count before we try to raise max stable bitrate
int rr_stat_handle(unsigned int fraction_lost, unsigned int jitter, double pg_delay)
{
    int nt_brate = 0;
    if(RRStat.inited == 0){
        RRStat.inited = 1;
        /*if(app_video_get_streaming_mode()==VIDEO_STREAMING_OUT_PRI){
            RRStat.maxBitRate = app_video_get_brc_avg_bitrate(REC_PRIMARY_STREAM)/1000;
        }else{
            RRStat.maxBitRate = app_video_get_brc_avg_bitrate(REC_SECONDARY_STREAM)/1000;
        }*/
        RRStat.maxBitRate = 2000000; // assume 2M bits/sec
        RRStat.netBandwidth = RRStat.maxBitRate;
        RRStat.maxStableBR =  RRStat.maxBitRate;
    }
    //RRStat.curBitRate = app_video_get_brc_cur_bitrate()/1000;
    RRStat.curBitRate = 2000000; // assume 2M bits/sec
    /* highest priority rule: if fr_lost > 50%, decrease the bitrate accordingly
     * The fraction of bitrate we decrease is empirical result which comes from testing with NetLimiter
     */
    if(fraction_lost > 192){ // > 75%
        nt_brate = RRStat.curBitRate >> 2;
        RRStat.zeroLost = 0;
        AmbaPrint("[dynamic bitrate] fraction lost over 75%, set bitrate=%d\n", nt_brate);
    }else if(fraction_lost > 180){ // > 70%
        nt_brate = RRStat.curBitRate >> 1;
        RRStat.zeroLost = 0;
        AmbaPrint("[dynamic bitrate] fraction lost over 70%, set bitrate=%d\n", nt_brate);
    }else if(fraction_lost > 160){ // > 62%
        nt_brate = RRStat.curBitRate*7/10;
        RRStat.zeroLost = 0;
        AmbaPrint("[dynamic bitrate] fraction lost over 62%, set bitrate=%d\n", nt_brate);
    }else if(fraction_lost > 128){ // > 50%
        nt_brate = RRStat.curBitRate*17/20;
        RRStat.zeroLost = 0;
        AmbaPrint("[dynamic bitrate] fraction lost over 50%, set bitrate=%d\n", nt_brate);
    }else{
        if(RRStat.inMiddleOfIncrement== 1){
            /* if fr_lost is caused by increment of bitrate, we set maxStableBR */
            if(fraction_lost > 64 && fraction_lost > RRStat.lastFrLost){ // > 25%
                RRStat.maxStableBR = RRStat.curBitRate - 2*INCREMENT_STEP;
                nt_brate = RRStat.maxStableBR - INCREMENT_STEP;
                RRStat.zeroLost = 0;
                AmbaPrint("[dynamic bitrate] increment to %d but fr_lost=%d, set max_stable_brate =%d and reset lost_count\n", RRStat.curBitRate, fraction_lost, RRStat.maxStableBR);
            }else if(fraction_lost > 25 && fraction_lost > RRStat.lastFrLost){ // > 10 %
                RRStat.maxStableBR = RRStat.curBitRate - 2*INCREMENT_STEP;
                nt_brate = RRStat.maxStableBR - INCREMENT_STEP;
                RRStat.zeroLost = 0;
                AmbaPrint("[dynamic bitrate] increment to %d but fr_lost=%d, set max_stable_brate =%d and reset lost_count\n", RRStat.curBitRate, fraction_lost, RRStat.maxStableBR);
            }else if(fraction_lost > 10){//increment result in fr_lost
                RRStat.maxStableBR = RRStat.curBitRate - INCREMENT_STEP;
                RRStat.zeroLost = 0;
                AmbaPrint("[dynamic bitrate] increment to %d but fr_lost=%d, set max_stable_brate =%d and reset lost_count\n", RRStat.curBitRate, fraction_lost, RRStat.maxStableBR);
            }else{
                RRStat.zeroLost++;
                AmbaPrint("[dynamic bitrate] increment to %d and fr_lost=0\n", RRStat.curBitRate);
            }
            RRStat.inMiddleOfIncrement = 0;
        }else{
            if(fraction_lost > 64 && fraction_lost > RRStat.lastFrLost){ // > 25%
                nt_brate = RRStat.curBitRate*6/7;
                RRStat.zeroLost = 0;
                AmbaPrint("[dynamic bitrate] fraction lost over 25%, set bitrate=%d\n", nt_brate);
            }else if(fraction_lost > 25 && fraction_lost > RRStat.lastFrLost){ // > 10 %
                nt_brate = RRStat.curBitRate*13/14;
                RRStat.zeroLost = 0;
                AmbaPrint("[dynamic bitrate] fraction lost over 10%, set bitrate=%d\n", nt_brate);
            }else if(fraction_lost > 10){ //if we are in bad channel fr_lost can be 1-10
                RRStat.zeroLost = 0;
                AmbaPrint("[dynamic bitrate] fr_lost=%d, reset lost_count\n", fraction_lost);
            }else{
                RRStat.zeroLost++;
                AmbaPrint("[dynamic bitrate] fr_lost=0, ++lost_count=%d\n", RRStat.zeroLost);
#if 1 //turn on to calculate maxStable bitrate accordring to (lastBR, curBR, lastFrLost)
                if(RRStat.inMiddleOfDecrement){
                    double delta = (double) RRStat.lastFrLost/ (double) 255;
                    RRStat.maxStableBR = (1-delta)*RRStat.lastBitRate + delta*RRStat.curBitRate;
                    AmbaPrint("[dynamic bitrate] delta=%lf , lastBitRate = %d, curBitRate=%d ,set max_stable bitrate=%d\n",delta,  RRStat.lastBitRate, RRStat.curBitRate, RRStat.maxStableBR);
                    RRStat.inMiddleOfDecrement = 0;
                }
#endif
                if(RRStat.zeroLost > 2 &&   (RRStat.netBandwidth + INCREMENT_STEP) <= RRStat.maxStableBR){
                    /* three consecutive fr_lost=0, try to raise net bandwith */
                    RRStat.netBandwidth += INCREMENT_STEP;
                    RRStat.zeroLost = 0;
                    RRStat.inMiddleOfIncrement = 1;
                    //app_video_set_brc_net_bandwidth(RRStat.netBandwidth*1000);
                    AmbaPrint("[dynamic bitrate] try to raise bandwidth to %d(max_stable=%d)\n", RRStat.netBandwidth, RRStat.maxStableBR);
                }
#if 0 //turn on to raise max_stable automatically, we suggest this should be raised by retmoe command API(or reset streaming session)
                else if(RRStat.zeroLost > MAX_STATBLE_COUNT_RAISE && (RRStat.maxStableBR + INCREMENT_STEP)<= RRStat.maxBitRate){
                    RRStat.maxStableBR+=INCREMENT_STEP;
                    RRStat.zeroLost = 0;
                    AmbaPrint("[dynamic bitrate] try to raise max_stable_bitrate to %d\n", RRStat.maxStableBR);
                }
#endif
            }
        }
    }

    /* Jitter might be helpful, we haven't integrate it with fr_lost
    */
    if(jitter > 10000){
        AmbaPrint("[dynamic bitrate] jitter = %d\n", jitter);
    }
    /* Round-trip propagation delay is not used because in our use case, udp packet
    *  won't go through lots of rounters/hubs before reach target
    * Note: rtcp needs to be enabled so that this value won't be zero
    */
    if(pg_delay > 1.00){
        AmbaPrint("[dynamic bitrate] Round-trip delay = %lf\n", pg_delay);
    }
    if(nt_brate < RRStat.curBitRate && nt_brate > 0){
        RRStat.inMiddleOfDecrement = 1;
        RRStat.netBandwidth = nt_brate;
        //app_video_set_brc_net_bandwidth(RRStat.netBandwidth*1000);
        AmbaPrint("[dynamic bitrate] try to decrease bandwidth to %d(now=%d)\n", RRStat.netBandwidth, RRStat.curBitRate);
        RRStat.lastBitRate = RRStat.curBitRate;
        RRStat.curBitRate = nt_brate;
    }
    RRStat.lastFrLost = fraction_lost;

    return 0;
}



/**
 * callback function to handle playback request.
 */
static int AmpUT_NetFifo_PlaybackCB(void *hdlr, UINT32 event, void* info)
{
    int rval = 0;
    char *fn;
    UINT32 *u_ptr;

    AmbaPrint("%s: Got NetFifo Playback request 0x%08x.", __FUNCTION__,event);

    switch(event){
    case AMP_NETFIFO_PLAYBACK_OPEN:
    /* rval: 0 for success, -1 for fail */
        fn = (char *)info;
        if(fn == NULL){
            AmbaPrint("%s[%d]: invalid parameter!",__FUNCTION__,__LINE__);
            rval = -1;
        } else {
            AmbaPrint("AMP_NETFIFO_PLAYBACK_OPEN filename %s",fn);
            rval = 0;
        }
        break;
    case AMP_NETFIFO_PLAYBACK_PLAY:
    /* rval: 0 for success, -1 for fail */
        u_ptr = (UINT32 *)info;

        if(u_ptr == NULL) {
            AmbaPrint("AMP_NETFIFO_PLAYBACK_PLAY from beginning!");
        } else {
            AmbaPrint("AMP_NETFIFO_PLAYBACK_PLAY from %d second",*u_ptr);
        }
        if(!g_EncStatus) {
            AmpUT_NetFifo_rec_start();
        }
        rval = 0;
        break;
    case AMP_NETFIFO_PLAYBACK_STOP:
    /* rval: 0 for success, -1 for fail */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_STOP!");
        //if(g_EncStatus) {
        //    AmpUT_NetFifo_rec_stop();
        //}
        rval = 0;
        break;
    case AMP_NETFIFO_PLAYBACK_RESET:
    /* rval: 0 for success, -1 for fail */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_RESET!");
        rval = 0;
        break;
    case AMP_NETFIFO_PLAYBACK_PAUSE:
    /* rval: 0 for success, -1 for fail */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_PAUSE!");
        rval = 0;
        break;
    case AMP_NETFIFO_PLAYBACK_RESUME:
    /* rval: 0 for success, -1 for fail */
        AmbaPrint("AMP_NETFIFO_PLAYBACK_RESUME!");
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
        rval = g_Udta.nTickPerPicture;
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
    case AMP_NETFIFO_PLAYBACK_SEND_RR_STAT:
    /* rval: don't care */
        do{
            rr_report_t* report = (rr_report_t*) info;
            AmbaPrint("Got Report: fr_lost=%d, jitter=%d, pg_delay=%g",
                    report->fr_lost, report->jitter, report->pg_delay);
            rval = rr_stat_handle(report->fr_lost, report->jitter, report->pg_delay);
        }while(0);
        break;
    default:
        AmbaPrint("%s: Unsuoported event 0x%08x",__FUNCTION__,event);
        rval = -1;
        break;
    }

    return rval;
}

/*
 * Unitest initiate function.
 */
static int AmpUT_NetFifo_init(void)
{
    AMP_CFS_STAT stat = {0};
    AMP_CFS_FILE_s *hUdtaFile = NULL;
    AMP_CFS_FILE_PARAM_s cfsParam = {0};
    int rval = 0;
    void *encRawBuf = NULL;
    UINT32 nBufferSize = 0;

    if(AmpUT_NetFifoInited){
        AmbaPrint("Already inited!!");
        return 0;
    }

    //init CFS. This is move to SystemInitTaskEntry
#if 0
    rval = AmpUT_NetFifo_CfsInit();
    if(rval != AMP_OK){
        AmbaPrint("Fail to do AmpUT_NetFifo_CfsInit");
        return rval;
    }
#endif

    //create encode buffer
    nBufferSize = DummyVideoEnc_Net_GetRequiredBufSize(AMPUT_NETFIFO_ENC_RAW_SIZE, AMPUT_NETFIFO_MAX_FIFO_NUM);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_EncBuffer, &encRawBuf, nBufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    }
    DummyVideoEnc_Net_Init(&g_Encoder, BS_FN, HDR_FN, g_EncBuffer, nBufferSize, AMPUT_NETFIFO_MAX_FIFO_NUM);

    // Read UDTA file
    rval = AmpCFS_Stat(UDTA_FN, &stat);
    if (rval == AMP_OK) {
        AmpCFS_GetFileParam(&cfsParam);
        cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
        strcpy(cfsParam.Filename, UDTA_FN);
        hUdtaFile = AmpCFS_fopen(&cfsParam);
        if (hUdtaFile == NULL) {
            AmbaPrint("Udta Open fail");
            rval = -1;
            goto done;
        } else {
            AmpCFS_fread(&g_Udta, 1, sizeof(g_Udta), hUdtaFile);
            AmpCFS_fclose(hUdtaFile);
        }
    } else {
        AmbaPrint("%s: fail to read UDTA file!(%d)", __FUNCTION__,rval);
        rval = -1;
        goto done;
    }

    //init AmpNetFifo
    do {
        AMP_NETFIFO_INIT_CFG_s init_cfg;

        AmpNetFifo_GetInitDefaultCfg(&init_cfg);
        init_cfg.cbEvent = AmpUT_NetFifo_EventCB;
        init_cfg.cbMediaInfo = AmpUT_NetFifo_MediaInfoCB;
        init_cfg.cbPlayback = AmpUT_NetFifo_PlaybackCB;

        AmpNetFifo_init(&init_cfg);
    } while (0);

    //create Physical FIFO for encoder
    do {
        AMP_FIFO_CFG_s fifoDefCfg;

        // create codec fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = AMPUT_NETFIFO_CODEC_HDLR;
        fifoDefCfg.IsVirtual = 0;
        fifoDefCfg.NumEntries = AMPUT_NETFIFO_MAX_FIFO_NUM;
        fifoDefCfg.cbEvent = DummyVideoEnc_Net_EventCB;
        AmpFifo_Create(&fifoDefCfg, &g_EncFifoHdlr);

        // hook default callback
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = AMPUT_NETFIFO_CODEC_HDLR;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = AMPUT_NETFIFO_MAX_FIFO_NUM;
        fifoDefCfg.cbEvent = AmpNetFifo_DefaultCB;
        AmpFifo_Create(&fifoDefCfg, &g_DefFifoHdlr);
    } while (0);

done:
    if (rval < 0) {
        if (AmbaKAL_BytePoolFree(encRawBuf) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the encRawBuf.", __FUNCTION__, __LINE__);
        }
    } else {
        AmpUT_NetFifoInited = 1;
    }

    return 0;
}

extern int Linux_resource_init(void);
extern int Linux_Playback_OP(unsigned int OP, void *param, int param_size);

static void AmpUT_NetFifo_usage(char *pName)
{
    AmbaPrint("usage: %s <CMD>",pName);
    AmbaPrint("\t init");
    AmbaPrint("\t rec_start");
    AmbaPrint("\t rec_stop");
    AmbaPrint("\t rtsp_start");
    AmbaPrint("\t rtsp_stop");
    AmbaPrint("\t end\n\n");
}

/**
 * NetFifo test function.
 */
static int AmpUT_NetFifo_Test(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int rval = -1;

    if (strcmp(argv[1], "init") == 0) { //init resource for video recorder and physical FIFO
        rval = AmpUT_NetFifo_init();
    } else if (strcmp(argv[1], "rec_start") == 0) { //start video record
        rval = AmpUT_NetFifo_rec_start();
    } else if (strcmp(argv[1], "rec_stop") == 0) { //stop video record
        rval = AmpUT_NetFifo_rec_stop();
    } else if (strcmp(argv[1], "rtsp_start") == 0) { //Start RTSP Server
        if(g_RTSPStatus == 0) {
            AmpNetUtility_luExecNoResponse("/usr/bin/AmbaRTSPServer");
            g_RTSPStatus = 1;
        } else {
            AmbaPrint("RTSPServer is running");
        }
    } else if (strcmp(argv[1], "rtsp_stop") == 0) { //Stop RTSP Server
        AmpNetUtility_luExecNoResponse("killall AmbaRTSPServer");
        g_RTSPStatus = 0;
    } else if (strcmp(argv[1], "time") == 0) { //Stop RTSP Server
        AmbaPrint("current time: %u ms",AmbaTimer_GetSysTickCount());
    } else if (strcmp(argv[1], "end") == 0) { //release all resource and exit
        AmbaPrint("%s: Not support yet!", argv[1]);
    } else { //show usage
        AmpUT_NetFifo_usage(argv[0]);
    }

    return rval;
}

/**
 * NetFifo test command hooking function.
 */
int AmpUT_NetFifo_TestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("netfifo", AmpUT_NetFifo_Test);

    return AMP_OK;
}

