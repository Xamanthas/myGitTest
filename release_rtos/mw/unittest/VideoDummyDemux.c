 /**
  * @file src/unittest/MWUnitTest/VideoDummyDemux.c
  *
  * Dummy demux for unit test of video decode and multiple channel decode
  *
  * History:
  *    2014/09/12 - [phcheng] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include "VideoDummyDemux.h"
#include "AmbaUtility.h"
#include "DecCFSWrapper.h"

//#define AMPVIDEODEC_DBG
#define AMPVIDEODEC_ERR

#ifdef AMPVIDEODEC_DBG
#define AmpVideoDecDbg AmbaPrintColor
#else
#define AmpVideoDecDbg(...)
#endif

#ifdef AMPVIDEODEC_ERR
#define AmpVideoDecErr AmbaPrintColor
#else
#define AmpVideoDecErr(...)
#endif

#define VIDEODEC_NORMALIZE_CLOCK(X, Y)   ( (UINT32) ((((UINT64)X) * 90000) / ((UINT64)Y)) )

typedef enum {
    FEED_SPEED_NOR,         ///< Feed all frames
    FEED_SPEED_IDR          ///< Feed Idr frames only
} FEED_SPEED;

static AMBA_KAL_TASK_t VideoDecDemuxTask = { 0 };       ///< Dummy demux task
static UINT8 VideoDecDemuxTaskIsInit = 0;               ///< Whether dummy demux task is initialized

static char AmpUT_VideoDecStack[AMP_UT_DUMMY_DEMUX_STACK_SIZE]; ///< Stack for dummy demux task
static FEED_SPEED FeedingSpeed = FEED_SPEED_NOR;    ///< Feeding speed of dummy demux

static AMP_AVDEC_PLAY_DIRECTION_e PlayDirection = AMP_VIDEO_PLAY_FW; ///< Play direction

static UINT32 PtsSortArray[VIDEODEC_PTS_ARRAY_SIZE_MAX] = { 0 };    ///< Array of sorted PTS used by dummy demux
static UINT32 PtsSortArraySize = 0;                                 ///< Number of elements in the PTS array

static AMP_UT_DUMMY_DEMUX_HDLR_s DummyDmxHdlr[DUMMY_DEMUX_MAX_NUM];  ///< Dummy demux handler (for each codec)

/**
 * Put GOP header
 *
 * @param [in] ptr          Address to place GOP header
 * @param [in] rate         Output rate
 * @param [in] scale        Output scale fps = rate/scale
 * @param [in] skipFirstI   Whether skip first I
 * @param [in] skipLastI    Whether skip last I
 * @param [in] m            The distance between two anchor frames (I or P)
 * @param [in] n            The distance between two full images (I-frames)
 * @param [in] pts          Time stamp
 *
 * @return 0 - OK, others - Error
 */
#define GOP_NALU_SIZE       (22)    ///< Size of GOP header
static int AmpUT_Video_DummyDmx_PutGopHeader(UINT8* ptr, UINT32 rate, UINT32 scale, UINT8 skipFirstI, UINT8 skipLastI,
    UINT8 m, UINT8 n, UINT32 pts)
{
    UINT8 Reserved = 1;
    UINT16 High, Low;
    UINT8 Header[GOP_NALU_SIZE];

#define HIGH_WORD(x) (UINT16)((x >> 16) & 0x0000ffff)
#define LOW_WORD(x) (UINT16)(x & 0x0000ffff)

    // Normalization
    // pts has already been normalized
    //pts = VIDEODEC_NORMALIZE_CLOCK(pts, rate);

    // Don't normalize TimeScale and NUIT.
    // If those values are normalzed, the ignored fractional number of time_scale can cause PTS drifted.
    //scale = VIDEODEC_NORMALIZE_CLOCK(scale, rate);
    //rate = VIDEODEC_NORMALIZE_CLOCK(rate, rate);

    AmpVideoDecDbg(GREEN, "%s  Pts = %u  Rate = %u  Scale = %u  SkipFirstI = %u  SkipLastI = %u  M = %u  N = %u",
            __FUNCTION__, pts, rate, scale, skipFirstI, skipLastI, m, n);

    Header[0] = (UINT8) 0x00;
    Header[1] = (UINT8) 0x00;
    Header[2] = (UINT8) 0x00;
    Header[3] = (UINT8) 0x01;
    /* NAL header */
    Header[4] = (UINT8) 0x7a;
    /* Main version */
    Header[5] = (UINT8) 0x01;
    /* Sub version */
    Header[6] = (UINT8) 0x01;

    High = HIGH_WORD(scale);
    Low = LOW_WORD(scale);
    Header[7] = ((UINT8) skipFirstI << (8 - 1)) |
        ((UINT8) skipLastI << (7 - 1)) |
        ((UINT8) (High >> (16 - 6)));

    Header[8] = ((UINT8) (High >> (10 - 8)));

    Header[9] = ((UINT8) (High << (8 - 2))) |
        ((UINT8) Reserved << (6 - 1)) |
        ((UINT8) (Low >> (16 - 5)));

    Header[10] = ((UINT8) (Low >> (11 - 8)));

    High = HIGH_WORD(rate);
    Header[11] = ((UINT8) (Low << (8 - 3))) |
        ((UINT8) Reserved << (5 - 1)) |
        ((UINT8) (High >> (16 - 4)));

    Header[12] = ((UINT8) (High >> (12 - 8)));

    Low = LOW_WORD(rate);
    Header[13] = ((UINT8) (High << (8 - 4))) |
        ((UINT8) Reserved << (4 - 1)) |
        ((UINT8) (Low >> (16 - 3)));

    Header[14] = ((UINT8) (Low >> (13 - 8)));

    High = HIGH_WORD(pts);
    Header[15] = ((UINT8) (Low << (8 - 5))) |
        ((UINT8) Reserved << (3 - 1)) |
        ((UINT8) (High >> (16 - 2)));

    Header[16] = ((UINT8) (High >> (14 - 8)));

    Low = LOW_WORD(pts);
    Header[17] = ((UINT8) (High << (8 - 6))) |
        ((UINT8) Reserved << (2 - 1)) |
        ((UINT8) (Low >> (16 - 1)));

    Header[18] = ((UINT8) (Low >> (15 - 8)));

    Header[19] = ((UINT8) (Low << (8 - 7))) |
        ((UINT8) Reserved >> (1 - 1));

    Header[20] = ((UINT8) n << (8 - 8));

    Header[21] = ((UINT8) m << (8 - 4)) & 0xf0;

    memcpy(ptr, Header, GOP_NALU_SIZE);

    return 0;
}

int AmpUT_Video_DummyDmx_Lock(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr, UINT32 timeout)
{
    int Er = 0;

    // Take mutex. Enter critical section.
    Er = AmbaKAL_MutexTake(&dmxHdlr->VideoDecFrmMutex, timeout);
    if (Er == TX_NOT_AVAILABLE) {
        AmpVideoDecErr(RED, "%s: Error code = %d  DemuxHdlr = 0x%08X  Take mutex timeout (%ums)", __FUNCTION__, Er, dmxHdlr, timeout);
    } else if (Er != 0) {
        AmpVideoDecErr(RED, "%s: Error code = %d  DemuxHdlr = 0x%08X  Failed to take mutex", __FUNCTION__, Er, dmxHdlr);
    }

    return Er;
}

int AmpUT_Video_DummyDmx_UnLock(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr)
{
    int Er = 0;

    if (dmxHdlr == NULL) {
        AmpVideoDecErr(RED, "%s: dmxHdlr is NULL", __FUNCTION__);
        return -1; // Error
    }

    // Give mutex. Exit critical section.
    Er = AmbaKAL_MutexGive(&dmxHdlr->VideoDecFrmMutex);
    if (Er != 0) {
        AmpVideoDecErr(RED, "%s: Error code = %d  DemuxHdlr = 0x%08X  Failed to give mutex", __FUNCTION__, Er, dmxHdlr);
    }

    return Er;
}

/**
 * Write data to Raw Buffer.\n
 * Do not clean cache, update WP, or send BitsFifoUpdate command.\n
 * This function should be in the critical section of the dummy demux.
 *
 * @param [in] dmxHdlr          Dummy demux handler
 * @param [in] startAddress     Address of data
 * @param [in] size             Size of data
 * @param [out] newWP           New write point
 *
 * @return 0 - OK, -1 - Error, -2 - Not enough space
 */
static int AmpUT_Video_DummyDmx_WriteData(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr, const UINT8* startAddress, UINT32 size, UINT8** newWP)
{
    AMP_BITS_DESC_s Desc = { 0 };           // Descriptor of remain space in Raw Buffer
    UINT8* OldWp = NULL;                    // Write point before updating
    UINT8* NewWp = NULL;                    // Write point after updating
    UINT8 IsWrapped = 0;                    // Whether the data in raw buffer is wrapped back to raw base.

    if (dmxHdlr == NULL) {
        AmpVideoDecErr(RED, "%s: dmxHdlr is NULL", __FUNCTION__);
        return -1; // Error
    }

    // Check size
    if (size == 0) {
        return 0; // No data. Do nothing.
    }

    // Get a descriptor of remaining space in Raw Buffer
    while (1) {
        if (AmpFifo_PrepareEntry(dmxHdlr->AvcDecFifoHdlr, &Desc) == 0) {
            break;
        }
        AmbaKAL_TaskSleep(1);
    }

    if (Desc.Size <= size) {
        return -2; // Not enough space
    }

    // Calculate new write point
    OldWp = Desc.StartAddr;
    IsWrapped = ((OldWp + size - 1) > dmxHdlr->AvcRawLimit) ? (1) : (0);
    if (IsWrapped) {
        // Get new WP
        NewWp = OldWp + size - dmxHdlr->AvcRawLimit + dmxHdlr->AvcRawBuffer - 1;
    } else {
        // Get new WP
        NewWp = OldWp + size;
    }

    // Write Data
    if (IsWrapped) {
        UINT32 DistanceToLimit = (UINT32)dmxHdlr->AvcRawLimit - (UINT32)OldWp + 1;

        // Write Data to raw buffer
        memcpy((void *)OldWp, (void *)startAddress, DistanceToLimit);
        memcpy((void *)dmxHdlr->AvcRawBuffer, (void *)(startAddress + DistanceToLimit), size - DistanceToLimit);
        AmpVideoDecDbg(GREEN, "Write Data:  Start = 0x%08X  Size = 0x%08X;  Start = 0x%08X  Size = 0x%08X",
                OldWp, DistanceToLimit, dmxHdlr->AvcRawBuffer, size - DistanceToLimit);
    } else {
        // Write Data to raw buffer
        memcpy((void *)OldWp, (void *)startAddress, size);
        AmpVideoDecDbg(GREEN, "Write Data:  Start = 0x%08X  Size = 0x%08X", OldWp, size);
    }


    *newWP = NewWp;

    return 0;
}

/**
 * Write descriptor of a frame to FIFO.\n
 * This function should be in the critical section of the dummy demux.
 *
 * @param [in] fifoHdlr     Fifo handler
 * @param [in] Desc         Descriptor of a frame
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_Video_DummyDmx_WriteEntry(AMP_FIFO_HDLR_s *fifoHdlr, AMP_BITS_DESC_s *Desc)
{
    int Rval = 0;

    if (fifoHdlr == NULL) {
        AmpVideoDecErr(RED, "%s: fifoHdlr is NULL", __FUNCTION__);
        return -1; // Error
    }

    while (1) {
        Rval = AmpFifo_WriteEntry(fifoHdlr, Desc);
        if (Rval == AMP_OK) {
            break; // Success
        } else if (Rval == AMP_ERROR_FIFO_FULL) {
            AmbaKAL_TaskSleep(1); // Wait for a while
            continue; // Try again
        } else {
            return Rval; // Error
        }
    }

    return AMP_OK; // Success
}

/**
 * Put a decode mark in FIFO.\n
 * This function should be in the critical section of the dummy demux.
 *
 * @param [in] dmxHdlr      Dummy demux handler
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_Video_DummyDmx_PutDecodeMark(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr)
{
    AMP_BITS_DESC_s Desc = { 0 }; // Descriptor to be stored in FIFO
    int Rval = 0;

    AmpVideoDecDbg(GREEN, "%s", __FUNCTION__);

    if (dmxHdlr == NULL) {
        AmpVideoDecErr(RED, "%s: dmxHdlr is NULL", __FUNCTION__);
        return -1; // Error
    }

    Desc.Type = AMP_FIFO_TYPE_DECODE_MARK;

    Rval = AmpUT_Video_DummyDmx_WriteEntry(dmxHdlr->AvcDecFifoHdlr, &Desc);
    if (Rval != AMP_OK) {
        AmpVideoDecErr(RED, "%s: Failed to write entry", __FUNCTION__);
    }

    return Rval;
}

/**
 * Put an EOS in FIFO.\n
 * This function should be in the critical section of the dummy demux.
 *
 * @param [in] dmxHdlr      Dummy demux handler
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_Video_DummyDmx_PutEos(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr)
{
    AMP_BITS_DESC_s Desc = { 0 }; // Descriptor to be stored in FIFO
    int Rval = 0;

    if (dmxHdlr == NULL) {
        AmpVideoDecErr(RED, "%s: dmxHdlr is NULL", __FUNCTION__);
        return -1; // Error
    }

    Desc.Size = AMP_FIFO_MARK_EOS;       // EOS magic num
    Desc.Type = AMP_FIFO_TYPE_EOS;       // Indicating EOS
    Desc.Pts = dmxHdlr->PreviousPts;

    Rval = AmpUT_Video_DummyDmx_WriteEntry(dmxHdlr->AvcDecFifoHdlr, &Desc);
    if (Rval != AMP_OK) {
        AmpVideoDecErr(RED, "%s: Failed to write entry", __FUNCTION__);
        return Rval;
    }

    dmxHdlr->VideoDecFrmCnt = VIDEODEC_FRAME_COUNT_EOS; // EOS
    AmbaPrint("Feeding EOS");
    return 0; // Success
}

static int AmpUT_Video_DummyDmx_FindNextFrame(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr, UINT32 currentID, AMP_AVDEC_PLAY_DIRECTION_e direction, FEED_SPEED feedingSpeed, UINT32 *nextID)
{
#define SLOW_SHUTTER_CLIP (1) ///< Whether to support slow shutter clip or interlaced clip. Less efficient approach. (TimePerFrame or N value may change in a single clip.
    INT32 ID = 0;
    const INT32 CurID = (INT32) currentID;
    const INT32 Count = (INT32) dmxHdlr->NhntDescCount;
    if (dmxHdlr == NULL) {
        return -1;
    }
    if (nextID == NULL) {
        return -1;
    }
    if (Count < 0) {
        return -1;
    }
    if (direction == AMP_VIDEO_PLAY_FW) {
        switch (feedingSpeed) {
        case FEED_SPEED_IDR:
            // Find the closest full images (I or Idr frame) ahead.
            // Usually, the NhntID of full images is divisible by N value.
            // However, that is not the case for slow shutter video.
#if SLOW_SHUTTER_CLIP
            for (ID = CurID + 1; ID < Count; ++ID) {
                if (dmxHdlr->NhntDescArray[ID].FrameType == AMP_FIFO_TYPE_IDR_FRAME ||
                    dmxHdlr->NhntDescArray[ID].FrameType == AMP_FIFO_TYPE_I_FRAME) {
                    break;
                }
            }
#else
            ID = ((currentID / dmxHdlr->AvcFileN) + 1) * dmxHdlr->AvcFileN; // ID is divisible by N value
#endif
            break;
        case FEED_SPEED_NOR:
            ID = CurID + 1;
            break;
        default:
            AmpVideoDecErr(RED, "%s: Unexpected feeding speed (%u)", __FUNCTION__, feedingSpeed);
            return -1;
        }
    } else { // Backwards
        switch (feedingSpeed) {
        case FEED_SPEED_IDR:
            // Find the closest full images (I or Idr frame) behind.
            // Usually, the NhntID of full images is divisible by N value.
            // However, that is not the case for slow shutter video.
            if (CurID > 0) {
#if SLOW_SHUTTER_CLIP
                for (ID = CurID - 1; ID >= 0; --ID) {
                    if (dmxHdlr->NhntDescArray[ID].FrameType == AMP_FIFO_TYPE_IDR_FRAME ||
                        dmxHdlr->NhntDescArray[ID].FrameType == AMP_FIFO_TYPE_I_FRAME) {
                        break;
                    }
                }
#else
                ID = ((currentID - 1) / dmxHdlr->AvcFileN) * dmxHdlr->AvcFileN; // ID is divisible by N value
#endif
            } else {
                ID = Count; // Set an illegal ID to stop feeding
            }
            break;
        case FEED_SPEED_NOR:
#if SLOW_SHUTTER_CLIP
            {
                UINT8 IsLastFrameInGOP = 0;
                // Check if this is the last frame in a GOP.
                if ((CurID + 1) >= Count) {
                    IsLastFrameInGOP = 1;
                } else {
                    if (dmxHdlr->NhntDescArray[CurID + 1].FrameType == AMP_FIFO_TYPE_IDR_FRAME ||
                        dmxHdlr->NhntDescArray[CurID + 1].FrameType == AMP_FIFO_TYPE_I_FRAME) {
                        IsLastFrameInGOP = 1;
                    }
                }
                if (IsLastFrameInGOP) {
                    UINT32 ID2 = 0; // The ID of the second I or Idr frame
                    UINT32 I_Count = 0; // Number of I or Idr frames
                    // TODO: Find ID2 in AmpUT_Video_DummyDmx_Open for better performance
                    for (ID2 = 0; ID2 < Count; ++ID2) {
                        if (dmxHdlr->NhntDescArray[ID2].FrameType == AMP_FIFO_TYPE_IDR_FRAME ||
                            dmxHdlr->NhntDescArray[ID2].FrameType == AMP_FIFO_TYPE_I_FRAME) {
                            I_Count++;
                            if (I_Count == 2) {
                                break;
                            }
                        }
                    }
                    // Check if this is the first GOP of entire video.
                    if (CurID < ID2) {
                        // If it is, there're no more frames to feed. Put decode mark and then EOS.
                        // Set an illegal ID. Demux task will receive an error (and hence put EOS) next time it runs AmpUT_Video_DummyDmx_GetNextDesc.
                        ID = Count; // Set an illegal ID to stop feeding
                    } else {
                        UINT32 I_Count = 0; // Number of I or Idr frames
                        // If it isn't, feed the first frame of the GOP behind.
                        for (ID = CurID; ID >= 0; --ID) { // Start from CurID in case that current frame is I or Idr
                            if (dmxHdlr->NhntDescArray[ID].FrameType == AMP_FIFO_TYPE_IDR_FRAME ||
                                dmxHdlr->NhntDescArray[ID].FrameType == AMP_FIFO_TYPE_I_FRAME) {
                                I_Count++;
                                if (I_Count == 2) {
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    // Not the last frame in a GOP. Same as feeding forwards.
                    ID = CurID + 1;
                }
            }
#else
            // Check if this is the last frame in a GOP.
            if (((CurID + 1) % dmxHdlr->AvcFileN) == 0 ||
                (CurID + 1) == Count) {
                // Check if this is the first GOP of entire video.
                if (CurID < dmxHdlr->AvcFileN) {
                    // If it is, there're no more frames to feed. Put decode mark and then EOS.
                    // Set an illegal ID. Demux task will receive an error (and hence put EOS) next time it runs AmpUT_Video_DummyDmx_GetNextDesc.
                    ID = Count; // Set an illegal ID to stop feeding
                } else {
                    // If it isn't, feed the first frame of the GOP behind.
                    ID = ((CurID / dmxHdlr->AvcFileN) - 1) * dmxHdlr->AvcFileN;
                }
            } else {
                // Not the last frame in a GOP. Same as feeding forwards.
                ID = CurID + 1;
            }
#endif
            break;
        default:
            AmpVideoDecErr(RED, "%s: Unexpected feeding speed (%u)", __FUNCTION__, feedingSpeed);
            return -1;
        }
    }

    *nextID = ID;
    if (ID < Count && ID >= 0) {
        return 0; // Success
    } else {
        return 1; // Seek to end. Not found.
    }
}

static int AmpUT_Video_DummyDmx_GetNextDesc(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr, NHNT_SAMPLE_HEADER_s* outNhntDesc)
{
    UINT32 NextID = -1; // Set an illegal ID by default
    if (PlayDirection == AMP_VIDEO_PLAY_FW) {
        if (dmxHdlr->CurrentNhntID >= dmxHdlr->NhntDescCount) {
            AmbaPrint("%s: Feed to end", __FUNCTION__);
            return -1; // Feed to end. Demux task will put a decode mark and EOS if it receives an error.
        }
    } else { // Backwards
        if (dmxHdlr->CurrentNhntID >= dmxHdlr->NhntDescCount) {
            AmbaPrint("%s: Feed to end", __FUNCTION__);
            return -1; // Feed to end. Demux task will put a decode mark and EOS if it receives an error.
        }
    }

    // Output value
    *outNhntDesc = dmxHdlr->NhntDescArray[dmxHdlr->CurrentNhntID];

    // Get next ID
    if (AmpUT_Video_DummyDmx_FindNextFrame(dmxHdlr, dmxHdlr->CurrentNhntID, PlayDirection, FeedingSpeed, &NextID) != 0) {
        // Set an illegal ID. Demux task will receive an error (and hence put EOS) next time it runs this function.
        dmxHdlr->CurrentNhntID = dmxHdlr->NhntDescCount;
    } else {
        dmxHdlr->CurrentNhntID = NextID;
    }

    return 0;
}

/**
 * Feed one frame into raw buffer if there're enough space.
 *
 * @param [in] dmxHdlr      Dummy demux handler
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_Video_DummyDmx_FeedFrame(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr)
{
    AMP_BITS_DESC_s Desc = { 0 };                   // Descriptor of remain space in Raw Buffer
    static NHNT_SAMPLE_HEADER_s NhntDesc = { 0 };   // Information of a frame got from ".nhnt" file
    UINT8* RawBase = NULL;                          // Minimum address within Raw Buffer
    UINT8* RawLimit = NULL;                         // Maximum address within Raw Buffer
    UINT8 GopHeader[GOP_NALU_SIZE];                 // Temporary storage of GOP header
    UINT8 *TempStartAddr = NULL;                    // Current address in Raw Buffer to write new data
    UINT32 TempSize = 0;                            // Total size of each frame including GOP header
    int Er = 0;
    UINT32 Rate = dmxHdlr->AvcFileTimeScale;        // "Rate" in GOP header settings. May change when "slow shutter" is enabled.
    UINT32 Scale = dmxHdlr->AvcFileTimePerFrame;    // "Scale" in GOP header settings.

    // Preliminary check
    if (dmxHdlr->IsInit == 0) {
        return -1; // Error
    }

    // Take mutex. Enter critical section.
    AmpUT_Video_DummyDmx_Lock(dmxHdlr, AMBA_KAL_WAIT_FOREVER);

    if (dmxHdlr->IsOpened == 0) {
        AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
        //AmbaKAL_TaskSleep(10);
        return -1; // Error
    }

    if (dmxHdlr->VideoDecFrmCnt == 0 || dmxHdlr->VideoDecFrmCnt == VIDEODEC_FRAME_COUNT_EOS || dmxHdlr->AvcRawFile == NULL) {
        AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
        //AmbaKAL_TaskSleep(10);
        return -1; // Error
    }

    RawBase = dmxHdlr->AvcRawBuffer;
    RawLimit = dmxHdlr->AvcRawLimit;

    // PTS delta may change because of "slow shutter". "Rate" may change. The "rate" in udta file is no longer reliable.
    // Calculate PTS change before AmpUT_Video_DummyDmx_GetNextDesc(), because it will change CurrentNhntID.
    // The calculation is based on the following assumption
    // 1. No B frame
    // 2. The original scale is defined in udta file.
    // 3. New scale is an integer multiple of the original scale.
    // 4. PTS delta does not change within a GOP.
    if (dmxHdlr->NhntDescCount >= 2 &&
        dmxHdlr->CurrentNhntID < (dmxHdlr->NhntDescCount - 1)) {
        UINT32 ID1 = dmxHdlr->CurrentNhntID;
        UINT32 ID2 = dmxHdlr->CurrentNhntID + 1;
        UINT32 Rate_Ori = dmxHdlr->AvcFileTimeScale;
        UINT32 Scale_Ori = dmxHdlr->AvcFileTimePerFrame;
        float Multiple = 0;
        UINT32 Multiple_Round = 0;
        UINT32 PtsDelta = dmxHdlr->NhntDescArray[ID2].CompositionTimeStamp - dmxHdlr->NhntDescArray[ID1].CompositionTimeStamp;
        if (PtsDelta != 0) {
            Multiple = (float) PtsDelta * dmxHdlr->AvcFileTimeScale / 90000 / Scale_Ori;
            Multiple += 0.5;
            Multiple_Round = (UINT32) Multiple;
            if (Multiple_Round >= 1) {
                Rate = Rate_Ori / Multiple_Round;
            }
        } else {
            // This is possible when video is interlaced.
            // For example, the PTS sequence of an interlaced video with Rate = 50, Scale = 1, M = 1 and N = 16 might be:
            // 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7
            Rate = dmxHdlr->AvcFileTimeScale;
        }
    }

    // Read frame
    // Read out frame header
    if (dmxHdlr->IsWaitingSpace == 0) {
        Er = AmpUT_Video_DummyDmx_GetNextDesc(dmxHdlr, &NhntDesc);
    } else {
        dmxHdlr->IsWaitingSpace = 0;
        Er = 0;
    }

    // EOS handling
    if (Er != 0) {
        // Put Decode Mark
        AmpUT_Video_DummyDmx_PutDecodeMark(dmxHdlr);
        // Put EOS
        AmpUT_Video_DummyDmx_PutEos(dmxHdlr);

        AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
        return 0; // Success
    }

    if (FeedingSpeed == FEED_SPEED_IDR) {
        // feed idr only
        if (NhntDesc.FrameType != AMP_FIFO_TYPE_IDR_FRAME && NhntDesc.FrameType != AMP_FIFO_TYPE_I_FRAME) {
            AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
            return 0; // Success
        }
    }

    // Decide whether to put a GOP header
    if (PlayDirection == AMP_VIDEO_PLAY_FW) {
        // Put GOP header before every Idr frames when playing forwards
        if (NhntDesc.FrameType == AMP_FIFO_TYPE_IDR_FRAME) {
            dmxHdlr->PutHeader = 1;
        }
    } else {
        // Put GOP header before every Idr and I frames when playing backwards
        if (NhntDesc.FrameType == AMP_FIFO_TYPE_IDR_FRAME || NhntDesc.FrameType == AMP_FIFO_TYPE_I_FRAME) {
            dmxHdlr->PutHeader = 1;
        }
    }

    // Store GOP header in a temporary space
    if (dmxHdlr->PutHeader == 1) {
        UINT8 SkipLastI; // Whether to skip the last I frame
        static UINT32 PrevRate = 0;

        // Put Decode Mark
        if (dmxHdlr->IsFirstGop == 1) {
            dmxHdlr->IsFirstGop = 0;
        } else if (PrevRate != Rate) { // Add decode marker when rate changes  TODO:REMOVE after uCode fix
            // Put Decode Mark at the end of a GOP
            AmpUT_Video_DummyDmx_PutDecodeMark(dmxHdlr);
        } else if (PlayDirection == AMP_VIDEO_PLAY_BW) {
            // Put Decode Mark at the end of a GOP
            AmpUT_Video_DummyDmx_PutDecodeMark(dmxHdlr);
        }
        PrevRate = Rate;

        // Determine whether to skip the last I frame
        if (PlayDirection == AMP_VIDEO_PLAY_BW && NhntDesc.FrameType == AMP_FIFO_TYPE_IDR_FRAME) {
            SkipLastI = 0; // We do not feed a redundant I frame at the end of every GOP, so no need to skip.
        } else {
            SkipLastI = 0;
        }

        SET_ZERO(GopHeader);
        AmpUT_Video_DummyDmx_PutGopHeader(GopHeader,
                Rate,
                (dmxHdlr->AvcFileInterlaced == 0) ? Scale : (Scale << 1), // Scale*2 if the video is interlaced (field per sample)
                0,
                SkipLastI,
                dmxHdlr->AvcFileM,
                dmxHdlr->AvcFileN,
                NhntDesc.DecodingTimeStamp);
        TempSize += GOP_NALU_SIZE;
    }

    while (1) {
        if (AmpFifo_PrepareEntry(dmxHdlr->AvcDecFifoHdlr, &Desc) == 0) {
            break;
        }
        AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
        AmbaKAL_TaskSleep(1);
        AmpUT_Video_DummyDmx_Lock(dmxHdlr, AMBA_KAL_WAIT_FOREVER);
    }

    // Check Raw Buffer size
    TempSize += NhntDesc.DataSize;
    if (TempSize > Desc.Size) {
        AmpVideoDecDbg(RED, "Feeding no space");
        dmxHdlr->VideoDecFrmCnt = 0; // Stop preloading if there's no space. Otherwise it causes system to wait forever.
        dmxHdlr->IsWaitingSpace = 1;
        AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
        //AmbaKAL_TaskSleep(5);
        return -1;
    }

    // Write GOP header into raw buffer
    TempStartAddr = Desc.StartAddr;
    if (dmxHdlr->PutHeader == 1) {
        dmxHdlr->PutHeader = 0;
        AmpVideoDecDbg(YELLOW, "Write GOP header: Addr = 0x%08X Size = 0x%08X", Desc.StartAddr, GOP_NALU_SIZE);
        Er = AmpUT_Video_DummyDmx_WriteData(dmxHdlr, GopHeader, GOP_NALU_SIZE, &TempStartAddr);
        if (Er == -2) {
            dmxHdlr->VideoDecFrmCnt = 0; // Stop preloading if there's no space. Otherwise it causes system to wait forever.
            dmxHdlr->IsWaitingSpace = 1;
            AmpVideoDecErr(RED, "Write GOP header: Feeding no space");
            AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
            return -1;
        } else if (Er != 0) {
            AmpVideoDecErr(RED, "Write GOP header: Failed to wrtie data");
            AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
            return -1;
        }
    }

    {
        AMP_BITS_DESC_s TempDesc = { 0 };               // Descriptor of the new frame to be stored in Raw Buffer

        TempDesc.StartAddr = Desc.StartAddr;
        TempDesc.Pts = NhntDesc.CompositionTimeStamp;
        TempDesc.Completed = 1;
        TempDesc.Size = TempSize;
        TempDesc.Type = (AMP_FIFO_FRMAE_TYPE_e) NhntDesc.FrameType;
        TempDesc.SeqNum = dmxHdlr->FrameSeqNum;

        dmxHdlr->PreviousPts = TempDesc.Pts;
        // Read out frame
        TempDesc.Align = 0;
        AmpCFS_fseek(dmxHdlr->AvcRawFile, NhntDesc.FileOffset, AMBA_FS_SEEK_START);
        AmpVideoDecDbg(YELLOW, "Write Video Frame: Addr = 0x%08X Size = 0x%08X", TempStartAddr, NhntDesc.DataSize);
        if ((TempStartAddr + NhntDesc.DataSize - 1) <= RawLimit) {
            AmpCFS_fread(TempStartAddr, NhntDesc.DataSize, 1, dmxHdlr->AvcRawFile);
        } else {
            UINT32 FreeToEnd = RawLimit - TempStartAddr + 1;
            UINT32 Remain = NhntDesc.DataSize - FreeToEnd;
            AmpCFS_fread(TempStartAddr, FreeToEnd, 1, dmxHdlr->AvcRawFile);
            AmpCFS_fread(RawBase, Remain, 1, dmxHdlr->AvcRawFile);
        }

        AmpVideoDecDbg(YELLOW, "Write Entry: Addr = 0x%08X Size = 0x%08X PTS = %lld", TempDesc.StartAddr, TempDesc.Size, TempDesc.Pts);
        // Write to fifo
        AmpUT_Video_DummyDmx_WriteEntry(dmxHdlr->AvcDecFifoHdlr, &TempDesc);
    }

    dmxHdlr->FrameSeqNum++;
    dmxHdlr->VideoDecFrmCnt--;

    AmpUT_Video_DummyDmx_UnLock(dmxHdlr);

    return 0; // Success
}

/**
 * Dummy demux task.\n
 * Feed frames to raw buffer.
 *
 * @param [in] info         EntryArg
 *
 * @return None
 */
static void AmpUT_Video_DemuxTask(UINT32 info)
{
    AmbaPrint("%s Start!", __FUNCTION__);

    // Raw file used format nhnt
    // check http://gpac.wp.mines-telecom.fr/mp4box/media-import/nhnt-format/ for more info
    // we could get it by using MP4box ( -nhnl )
    while (1) {
        UINT8 DmxID = 0;

        // Deal with each demuxer
        for (DmxID = 0; DmxID < DUMMY_DEMUX_MAX_NUM; ++DmxID) {
            AMP_UT_DUMMY_DEMUX_HDLR_s *DmxHdlr = NULL;

            // Get demux handler
            DmxHdlr = &DummyDmxHdlr[DmxID];

            // Feed one frame to raw buffer
            if (DmxHdlr != NULL) {
                UINT32 T = 0;
                for (T = 0; T < 4; ++T) {
                    if (AmpUT_Video_DummyDmx_FeedFrame(DmxHdlr) != 0) {
                        break;
                    }
                }
            }
        }

        AmbaKAL_TaskSleep(1);
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
static int AmpUT_Video_DummyDmx_FifoCB(void *hdlr,
                                   UINT32 event,
                                   void* info)
{
    // Do nothing

    return 0;
}

/**
 * Reset PTS array.
 *
 * @return None
 */
static void AmpUT_Video_PtsSortArray_Init(void)
{
    SET_ZERO(PtsSortArray);
    PtsSortArraySize = 0;
}

/**
 * Print PTS array.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_Video_PtsSortArray_Print(void)
{
    UINT32 T = 0;
    static char PtsStr[VIDEODEC_PTS_ARRAY_SIZE_MAX * 4];
    static char TempStr[16];
    SET_ZERO(PtsStr);
    for (T = 0; T < PtsSortArraySize; ++T) {
        if (T != 0) {
            strcat(PtsStr, ", ");
        }
        sprintf(TempStr, "%u", PtsSortArray[T]);
        strcat(PtsStr, TempStr);
    }
    AmbaPrint("PtsSortArray = { %s }", PtsStr);
    return 0; // Success
}

/**
 * Add an element and sort the array.
 *
 * @param [in] pts          PTS of a frame to be added into PTS array
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_Video_PtsSortArray_Add(UINT32 pts)
{
    static UINT32 TempArray[VIDEODEC_PTS_ARRAY_SIZE_MAX];
    UINT32 Position; // Position of the element in the array

    if (PtsSortArraySize == VIDEODEC_PTS_ARRAY_SIZE_MAX) {
        AmpVideoDecErr(RED, "%s: PTS array is full (max = %u)", __FUNCTION__, VIDEODEC_PTS_ARRAY_SIZE_MAX);
        return -1; // Error
    }

    // No element in the array. Simply add the element at the beginning.
    if (PtsSortArraySize == 0) {
        // Add element
        PtsSortArray[0] = pts;
        ++PtsSortArraySize;
        return 0; // Success
    }

    // Find the position of the element (after sorting)
    for (Position = 0; Position < PtsSortArraySize; ++Position) {
        if (VIDEODEC_PTS_ARRAY_SORT == 0) { // Ascending
            if (pts > PtsSortArray[Position]) {
                continue; // Keep looking
            } else {
                break; // Position found
            }
        } else { // Descending
            if (pts < PtsSortArray[Position]) {
                continue; // Keep looking
            } else {
                break; // Position found
            }
        }
    }

    if (Position == PtsSortArraySize) {
        // Add the element at the end.
        PtsSortArray[Position] = pts;
    } else {
        // Add the element in the middle of the array and shift all elements behind it.
        UINT32 Remain = PtsSortArraySize - Position;
        memcpy(TempArray, PtsSortArray + Position, Remain * sizeof(UINT32));
        PtsSortArray[Position] = pts;
        memcpy(PtsSortArray + Position + 1, TempArray, Remain * sizeof(UINT32));
    }

    ++PtsSortArraySize;
    return 0; // Success
}

/**
 * Get the difference between two elements in the PTS array.
 * It's expected that the differences between two adjacent elements are equal.
 * As a result, it will return error if the differences are not consistent.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_Video_PtsSortArray_GetDiff(UINT32 *diff)
{
    UINT32 T;
    UINT32 TempDiff;    // Difference of adjacent element
    UINT32 MinDiff;     // Minimum of TempDiff
    UINT32 MaxDiff;     // Maximum of TempDiff

    if (PtsSortArraySize <= 1) {
        AmpVideoDecErr(RED, "%s: Not enough elements to find the difference", __FUNCTION__);
        return -1;
    }

    for (T = 1; T < PtsSortArraySize; ++T) {
        if (VIDEODEC_PTS_ARRAY_SORT == 0) { // Ascending
            if (PtsSortArray[T - 1] > PtsSortArray[T]) {
                AmpVideoDecErr(RED, "%s: PTS array is not sorted", __FUNCTION__);
                return -1; // Error
            }
            if (T == 1) { // The first time in this loop
                // Get the difference between two elements
                TempDiff = PtsSortArray[T] - PtsSortArray[T - 1];
                MinDiff = TempDiff;
                MaxDiff = TempDiff;
            } else {
                TempDiff = PtsSortArray[T] - PtsSortArray[T - 1];
                MinDiff = MIN(MinDiff, TempDiff);
                MaxDiff = MAX(MaxDiff, TempDiff);
            }
        } else { // Descending
            if (PtsSortArray[T - 1] < PtsSortArray[T]) {
                AmpVideoDecErr(RED, "%s: PTS array is not sorted", __FUNCTION__);
                return -1; // Error
            }
            if (T == 1) { // The first time in this loop
                // Get the difference between two elements
                TempDiff = PtsSortArray[T - 1] - PtsSortArray[T];
                MinDiff = TempDiff;
                MaxDiff = TempDiff;
            } else {
                TempDiff = PtsSortArray[T] - PtsSortArray[T - 1];
                MinDiff = MIN(MinDiff, TempDiff);
                MaxDiff = MAX(MaxDiff, TempDiff);
            }
        }
    }

#define VIDEODEC_PTS_ARRAY_DIFF_THRES (10)      ///< Threshold of difference between adjacent frames to be regarded as consistent
    // Check consistency
    if ((MaxDiff - MinDiff) > VIDEODEC_PTS_ARRAY_DIFF_THRES) {
        AmpVideoDecErr(RED, "%s: Inconsistent difference (Min = %u, Max = %u)", __FUNCTION__, MinDiff, MaxDiff);
        return -1; // Error
    }

    // Output diff
    *diff = (MaxDiff + MinDiff) >> 1; // Average of max and min

    return 0; // Success
}

/**
 * Get file information.\n
 *
 * @param [in] dmxHdlr      Dummy demux handler
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_Video_DummyDmx_GetFileInfo_FromUdta(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr)
{
    // Initialize data
    dmxHdlr->AvcFileTimeScale = 0;
    dmxHdlr->AvcFileTimePerFrame = 0;
    dmxHdlr->AvcFileM = 0;
    dmxHdlr->AvcFileN = 0;

    // Get Width
    dmxHdlr->AvcFileWidth = dmxHdlr->AvcUserData.nVideoWidth;
    // Get Height
    dmxHdlr->AvcFileHeight = dmxHdlr->AvcUserData.nVideoHeight;

    // Get TimeScale
    dmxHdlr->AvcFileTimeScale = dmxHdlr->AvcUserData.nTimeScale;
    // Get TimePerFrame
    dmxHdlr->AvcFileTimePerFrame = dmxHdlr->AvcUserData.nTickPerPicture;
    // Get M value
    dmxHdlr->AvcFileM = dmxHdlr->AvcUserData.nM;
    // Get N value
    dmxHdlr->AvcFileN = dmxHdlr->AvcUserData.nN;
    // Get Interlaced
    dmxHdlr->AvcFileInterlaced = dmxHdlr->AvcUserData.nInterlaced;

    AmbaPrintColor(GREEN, "%s  Rate = %u  Scale = %u  M = %u  N = %u  Interlaced = %u", __FUNCTION__, dmxHdlr->AvcFileTimeScale, dmxHdlr->AvcFileTimePerFrame, dmxHdlr->AvcFileM, dmxHdlr->AvcFileN, dmxHdlr->AvcFileInterlaced);

    return 0; // Success
}

/**
 * Get file information.\n
 * NOTE: FS of nhnt file will seek to the fisrt descriptor in the end of this function
 *
 * @param [in] dmxHdlr      Dummy demux handler
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_Video_DummyDmx_GetFileInfo_FromNhnt(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr)
{
    int ReturnValue = 0; // Success
    NHNT_SAMPLE_HEADER_s NhntDesc = { 0 };      // Descriptor of a frame
    UINT64 DataSize = 0;                        // Size of the data get from file
    int T = 0;                                  // Counter
    int FirstI = -1;                            // Index of the first I frame. For calculating N value.
    int SecondI = -1;                           // Index of the second I frame. For calculating N value.
    int FirstIOrP = -1;                         // Index of the first I or P frame. For calculating M value.
    int SecondIOrP = -1;                        // Index of the second I or P frame. For calculating M value.

    // Initialize data
    dmxHdlr->AvcFileTimeScale = 0;
    dmxHdlr->AvcFileTimePerFrame = 0;
    dmxHdlr->AvcFileM = 0;
    dmxHdlr->AvcFileN = 0;

    // Get rate
    dmxHdlr->AvcFileTimeScale = dmxHdlr->NhntHeader.TimeStampResolution;

    // Initialize PtsSortArray
    AmpUT_Video_PtsSortArray_Init();

    // Get the first frame descriptor
    DataSize = AmpCFS_fread(&NhntDesc, sizeof(NhntDesc), 1, dmxHdlr->AvcIdxFile);
    if (DataSize == 0) {
        AmpVideoDecErr(RED, "%s: Failed to read nhnt descriptor (size = %lld)", __FUNCTION__, DataSize);
        ReturnValue = -1; // Error
        goto GetFileInfoEnd;
    }
    if (NhntDesc.FrameType != AMP_FIFO_TYPE_IDR_FRAME && NhntDesc.FrameType != AMP_FIFO_TYPE_I_FRAME) {
        // Usually the first frame is Idr.
        // There's a special case that the first frame is I instead of Idr.
        // To simplify dummy demux, we use a special clip to test playing backwards.
        // The frames in the clip are alrerady arranged in backward order.
        // The first frame could be I instead of Idr in this case.
        AmpVideoDecErr(RED, "%s: Type of the first frame (%u) is not expected (should be %u or %u)",
                __FUNCTION__,
                NhntDesc.FrameType,
                AMP_FIFO_TYPE_IDR_FRAME,
                AMP_FIFO_TYPE_I_FRAME);
        ReturnValue = -1; // Error
        goto GetFileInfoEnd;
    } else {
        FirstI = T;
        FirstIOrP = T;
    }
    // Add PTS to PtsSortArray
    if (AmpUT_Video_PtsSortArray_Add(NhntDesc.CompositionTimeStamp) != 0) {
        AmpVideoDecErr(RED, "%s: Failed to add PTS to PtsSortArray", __FUNCTION__);
        ReturnValue = -1; // Error
        goto GetFileInfoEnd;
    }
    ++T;

    // Search to the next I frame. Find M and N values.
    while (SecondI == -1 || SecondIOrP == -1) {
        DataSize = AmpCFS_fread(&NhntDesc, sizeof(NhntDesc), 1, dmxHdlr->AvcIdxFile);
        if (DataSize == 0) {
            AmpVideoDecErr(RED, "%s: Failed to read nhnt descriptor (size = %lld)", __FUNCTION__, DataSize);
            ReturnValue = -1; // Error
            goto GetFileInfoEnd;
        }
        // Check frame type
        if (NhntDesc.FrameType == AMP_FIFO_TYPE_IDR_FRAME || NhntDesc.FrameType == AMP_FIFO_TYPE_I_FRAME) {
            if (SecondI == -1) {
                SecondI = T;
            }
            if (SecondIOrP == -1) {
                SecondIOrP = T;
            }
            // End of search
            break;
        } else if (NhntDesc.FrameType == AMP_FIFO_TYPE_P_FRAME || NhntDesc.FrameType == AMP_FIFO_TYPE_B_FRAME) {
            // Add PTS to PtsSortArray
            if (AmpUT_Video_PtsSortArray_Add(NhntDesc.CompositionTimeStamp) != 0) {
                AmpVideoDecErr(RED, "%s: Failed to add PTS to PtsSortArray", __FUNCTION__);
                ReturnValue = -1; // Error
                goto GetFileInfoEnd;
            }
            // Find the second P frame
            if (SecondIOrP == -1 &&
                NhntDesc.FrameType == AMP_FIFO_TYPE_P_FRAME) {
                SecondIOrP = T;
            }
        }
        ++T;
    }

    if (AmpUT_Video_PtsSortArray_GetDiff(&dmxHdlr->AvcFileTimePerFrame) != 0) {
        AmpVideoDecErr(RED, "%s: Failed to get PTS diff", __FUNCTION__);
        AmpUT_Video_PtsSortArray_Print();
        ReturnValue = -1; // Error
        goto GetFileInfoEnd;
    }
    AmpUT_Video_PtsSortArray_Print();

    // Get M value
    if (SecondIOrP <= FirstIOrP ||
            FirstIOrP == -1 ||
            SecondIOrP == -1) {
        AmpVideoDecErr(RED, "%s: Unexpected: FirstIOrP = %d, SecondIOrP = %d", __FUNCTION__, FirstIOrP, SecondIOrP);
        ReturnValue = -1; // Error
        goto GetFileInfoEnd;
    }
    dmxHdlr->AvcFileM = SecondIOrP - FirstIOrP;

    // Get N value
    if (SecondI <= FirstI ||
            FirstI == -1 ||
            SecondI == -1) {
        AmpVideoDecErr(RED, "%s: Unexpected: FirstI = %d, SecondI = %d", __FUNCTION__, FirstI, SecondI);
        ReturnValue = -1; // Error
        goto GetFileInfoEnd;
    }
    dmxHdlr->AvcFileN = SecondI - FirstI;

    // TODO: Get Interlaced

GetFileInfoEnd:
    AmbaPrintColor(GREEN, "%s  Rate = %u  Scale = %u  M = %u  N = %u", __FUNCTION__, dmxHdlr->AvcFileTimeScale, dmxHdlr->AvcFileTimePerFrame, dmxHdlr->AvcFileM, dmxHdlr->AvcFileN);
    // Seek to the first frame
    AmpCFS_fseek(dmxHdlr->AvcIdxFile, 0, AMBA_FS_SEEK_START);
    AmpCFS_fread(&dmxHdlr->NhntHeader, sizeof(dmxHdlr->NhntHeader), 1, dmxHdlr->AvcIdxFile);

    return ReturnValue;
}

int AmpUT_Video_DummyDmx_Init(void)
{
    int Er = 0;
    int ReturnValue = 0; // 0: Success

    // Initialize all dummy demux handlers
    SET_ZERO(DummyDmxHdlr);

    // Initialize task
    if (VideoDecDemuxTaskIsInit == 0) {
        VideoDecDemuxTaskIsInit = 1;

        // Create demux task
        Er = AmbaKAL_TaskCreate(&VideoDecDemuxTask, // pTask
                "UT_DummyDemuxTask", // pTaskName
                7, // Priority
                AmpUT_Video_DemuxTask, // void (*EntryFunction)(UINT32)
                0x0, // EntryArg
                (void *) AmpUT_VideoDecStack, // pStackBase
                AMP_UT_DUMMY_DEMUX_STACK_SIZE, // StackByteSize
                AMBA_KAL_AUTO_START); // AutoStart
        if (Er != 0) {
            AmpVideoDecErr(RED, "%s:%u Failed to create task.", __FUNCTION__, __LINE__);
            ReturnValue = -1; // Error
        }
    }

    return ReturnValue;
}

int AmpUT_Video_DummyDmx_Create(const AMP_UT_DUMMY_DEMUX_CREATE_CFG_s* dummyDemuxCfg, AMP_UT_DUMMY_DEMUX_HDLR_s** outDmxHdlr)
{
    int T = 0;
    int Er = 0;
    AMP_UT_DUMMY_DEMUX_HDLR_s* CrtDmxHdlr = NULL;

    if (outDmxHdlr == NULL) {
        return -1;
    }

    // Search for free handler
    for (T = 0; T < DUMMY_DEMUX_MAX_NUM; ++T) {
        if (DummyDmxHdlr[T].IsInit == 0) {
            CrtDmxHdlr = &DummyDmxHdlr[T];
            break;
        }
    }
    if (CrtDmxHdlr == NULL) {
        AmpVideoDecErr(RED, "%s: No more handlers", __FUNCTION__);
        return -1;
    }

    // Setup config
    CrtDmxHdlr->AvcDecHdlr = dummyDemuxCfg->AvcDecHdlr;
    CrtDmxHdlr->AvcRawBuffer = dummyDemuxCfg->AvcRawBuffer;
    CrtDmxHdlr->AvcRawLimit = dummyDemuxCfg->AvcRawLimit;

    // Create fifo
    if (CrtDmxHdlr->AvcDecFifoHdlr == NULL) {
        AMP_FIFO_CFG_s FifoDefCfg = { 0 };

        AmpFifo_GetDefaultCfg(&FifoDefCfg);
        FifoDefCfg.hCodec = dummyDemuxCfg->AvcDecHdlr;
        FifoDefCfg.IsVirtual = 1;   // Virtual FIFO
        FifoDefCfg.NumEntries = 128;
        FifoDefCfg.cbEvent = AmpUT_Video_DummyDmx_FifoCB;
        Er = AmpFifo_Create(&FifoDefCfg, &CrtDmxHdlr->AvcDecFifoHdlr);
        if (Er != OK) {
            AmpVideoDecErr(RED, "Failed to create fifo: %d", Er);
            return -1;
        }
    }

    // Create mutex for reading and writing frames to fifo
    Er = AmbaKAL_MutexCreate(&CrtDmxHdlr->VideoDecFrmMutex);
    if (Er != OK) {
        AmpVideoDecErr(RED, "Failed to create mutex: %d", Er);
        return -1;
    }

    CrtDmxHdlr->IsInit = 1;

    // Return result
    *outDmxHdlr = CrtDmxHdlr;

    return 0;
}

int AmpUT_Video_DummyDmx_Open(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr,
                              char* rawFn,
                              char* idxFn,
                              char* udtFn)
{
    int ReturnValue = 0; // Success
    char Mode[3] = "rb";
    UINT8 NhntIsValid = 0;
    UINT8 UdtaIsValid = 0;
    char *FsFileName;       // Input to AmpCFS_fopen
    char *FsMode;           // Input to AmpCFS_fopen

    if (dmxHdlr->IsInit == 0) {
        return -1; // Error
    }

    AmpUT_Video_DummyDmx_Lock(dmxHdlr, AMBA_KAL_WAIT_FOREVER);

    // Open raw file (.h264)
    AmbaPrint("%s: Open file %s", __FUNCTION__, rawFn);
    if (dmxHdlr->AvcRawFile != NULL) {
        AmpCFS_fclose(dmxHdlr->AvcRawFile);
        dmxHdlr->AvcRawFile = NULL;
    }
    // Set input
    FsFileName = rawFn;
    FsMode = Mode;

    // Open file
    dmxHdlr->AvcRawFile = AmpDecCFS_fopen(FsFileName, FsMode);
    if (dmxHdlr->AvcRawFile == NULL) {
        AmpVideoDecErr(RED, "%s: Failed to open raw file", __FUNCTION__);
        ReturnValue = -1; // Error
        goto ReturnResult;
    }

    // Open index file (.nhnt)
    AmbaPrint("%s: Open file %s", __FUNCTION__, idxFn);
    if (dmxHdlr->AvcIdxFile != NULL) {
        AmpCFS_fclose(dmxHdlr->AvcIdxFile);
        dmxHdlr->AvcIdxFile = NULL;
    }
    // Set input
    FsFileName = idxFn;
    FsMode = Mode;
    // Open file
    dmxHdlr->AvcIdxFile = AmpDecCFS_fopen(FsFileName, FsMode);
    if (dmxHdlr->AvcIdxFile == NULL) {
        AmpVideoDecErr(RED, "%s: Failed to open index file", __FUNCTION__);
        ReturnValue = -1; // Error
        goto ReturnResult;
    } else {
        NhntIsValid = 1;
    }

    // Open user data file (.udta)
    AmbaPrint("%s: Open file %s", __FUNCTION__, udtFn);
    if (dmxHdlr->AvcUdtFile != NULL) {
        AmpCFS_fclose(dmxHdlr->AvcUdtFile);
        dmxHdlr->AvcUdtFile = NULL;
    }
    // Set input
    FsFileName = udtFn;
    FsMode = Mode;
    // Open file
    dmxHdlr->AvcUdtFile = AmpDecCFS_fopen(FsFileName, FsMode);
    if (dmxHdlr->AvcUdtFile == NULL) {
        AmpVideoDecErr(RED, "%s: Failed to open user data file", __FUNCTION__);
        // No need to return failure. Parse index file instead.
    } else {
        UINT64 DataSize = 0;
        UdtaIsValid = 1;
        SET_ZERO(dmxHdlr->AvcUserData);
        DataSize = AmpCFS_fread(&dmxHdlr->AvcUserData, sizeof(dmxHdlr->AvcUserData), 1, dmxHdlr->AvcUdtFile);
        if (DataSize == 0) {
            AmpVideoDecDbg(YELLOW, "%s: Failed to read user data", __FUNCTION__);
            //ReturnValue = -1; // Don't return error. Since the structure of FORMAT_USER_DATA_s has changed and we still want to play old clips
        }
        AmbaPrintColor(YELLOW, "Udta: nIdrInterval = %u  nTimeScale = %u  nTickPerPicture = %u  nM = %u  nN = %u  nVideoWidth = %u  nVideoHeight = %u  nInterlaced = %u",
                dmxHdlr->AvcUserData.nIdrInterval,
                dmxHdlr->AvcUserData.nTimeScale,
                dmxHdlr->AvcUserData.nTickPerPicture,
                dmxHdlr->AvcUserData.nM,
                dmxHdlr->AvcUserData.nN,
                dmxHdlr->AvcUserData.nVideoWidth,
                dmxHdlr->AvcUserData.nVideoHeight,
                dmxHdlr->AvcUserData.nInterlaced);
        AmpCFS_fclose(dmxHdlr->AvcUdtFile);
        dmxHdlr->AvcUdtFile = NULL;
    }

    dmxHdlr->VideoDecFrmCnt = 0;
    dmxHdlr->FrameSeqNum = 0;
    dmxHdlr->PutHeader = 1;
    dmxHdlr->IsFirstGop = 1;

    // Get Nhnt Header
    {
        UINT64 DataSize = 0;                        // Size of the data get from file
        DataSize = AmpCFS_fread(&dmxHdlr->NhntHeader, sizeof(dmxHdlr->NhntHeader), 1, dmxHdlr->AvcIdxFile);
        if (DataSize == 0) {
            AmpVideoDecErr(RED, "%s: Failed to read nhnt header (size = %lld)", __FUNCTION__, DataSize);
            ReturnValue = -1; // Error
        }
    }

    // Read Nhnt Desc
    {
        UINT64 DataSize = 0;                        // Size of the data get from file
        UINT32 T = 0;
        SET_ZERO(dmxHdlr->NhntDescArray);
        dmxHdlr->NhntDescCount = 0;
        for (T = 0; T < VIDEODEC_NHNT_DESC_ARRAY_SIZE; ++T) {
            DataSize = AmpCFS_fread(dmxHdlr->NhntDescArray + T, sizeof(NHNT_SAMPLE_HEADER_s), 1, dmxHdlr->AvcIdxFile);
            if (DataSize == 0) {
                dmxHdlr->NhntDescCount = T;
                break;
            }
        }
        if (T == VIDEODEC_NHNT_DESC_ARRAY_SIZE) {
            AmpVideoDecErr(RED, "%s: Video is too large. More than %u frames.", __FUNCTION__, VIDEODEC_NHNT_DESC_ARRAY_SIZE);
            ReturnValue = -1; // Error
            goto ReturnResult;
        }
    }

    // Get file information
    if (UdtaIsValid) {
        AmpUT_Video_DummyDmx_GetFileInfo_FromUdta(dmxHdlr);
    } else if (NhntIsValid) {
        if (AmpUT_Video_DummyDmx_GetFileInfo_FromNhnt(dmxHdlr) != 0) {
            AmpVideoDecErr(RED, "%s: Failed to get file information", __FUNCTION__);
            ReturnValue = -1; // Error
            goto ReturnResult;
        }
    } else {
        AmpVideoDecErr(RED, "%s: Failed to get file information", __FUNCTION__);
        ReturnValue = -1; // Error
        goto ReturnResult;
    }

    {
        UINT32 T = 0;
        const UINT32 TimeScale = dmxHdlr->AvcFileTimeScale;
        // Normalization
        for (T = 0; T < dmxHdlr->NhntDescCount; ++T) {
            dmxHdlr->NhntDescArray[T].CompositionTimeStamp = VIDEODEC_NORMALIZE_CLOCK(dmxHdlr->NhntDescArray[T].CompositionTimeStamp, TimeScale);
            dmxHdlr->NhntDescArray[T].DecodingTimeStamp = VIDEODEC_NORMALIZE_CLOCK(dmxHdlr->NhntDescArray[T].DecodingTimeStamp, TimeScale);
        }
        // Don't normalize TimeScale and TimePerFrame. They will be TimeScale and NUIT in GOP header.
        // If those values are normalzed, the ignored fractional number of time_scale can cause PTS drifted.
        //dmxHdlr->AvcFileTimePerFrame = VIDEODEC_NORMALIZE_CLOCK(dmxHdlr->AvcFileTimePerFrame, TimeScale);
        //dmxHdlr->AvcFileTimeScale = VIDEODEC_NORMALIZE_CLOCK(dmxHdlr->AvcFileTimeScale, TimeScale);

        // Set EOS after normalization
        // The inputs have to be normalized
        AmpVideoDec_SetEosPts(
                dmxHdlr->AvcDecHdlr,
                (UINT64)0, // This parameter is dummy
                VIDEODEC_NORMALIZE_CLOCK(dmxHdlr->AvcFileTimePerFrame, TimeScale),
                VIDEODEC_NORMALIZE_CLOCK(dmxHdlr->AvcFileTimeScale, TimeScale));
    }

    dmxHdlr->PreviousPts = VIDEODEC_FRAME_COUNT_EOS;
    dmxHdlr->IsOpened = 1;

ReturnResult:
    AmpUT_Video_DummyDmx_UnLock(dmxHdlr);

    return ReturnValue;
}

int AmpUT_Video_DummyDmx_Start(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr,
                               UINT32 startTime,
                               AMP_AVDEC_PLAY_DIRECTION_e direction,
                               UINT32 speed,
                               UINT32 preFeed)
{
    int T = 0;
    UINT32 TargetPTS = startTime * 90; // Convert to DSP 90000 base

    // TODO: 1. Whether erase previous frames or not?
    // TODO: 2. Change definition of speed, enable 2x, 4x, ...

    if (dmxHdlr->IsOpened == 0) {
        AmpVideoDecErr(RED, "%s: Open demux first", __FUNCTION__);
        return -1;
    }

    // Configure feeding speed
    if (speed > 0x100) {
        FeedingSpeed = FEED_SPEED_IDR;
    } else {
        FeedingSpeed = FEED_SPEED_NOR;
    }

    PlayDirection = direction;

    // 3. Time seek. Select CurrentNhntID based on startTime
    while (T < dmxHdlr->NhntDescCount) {
        if (TargetPTS < dmxHdlr->NhntDescArray[T].CompositionTimeStamp) {
            if (T < dmxHdlr->AvcFileN) {
                dmxHdlr->CurrentNhntID = 0;
            } else {
                dmxHdlr->CurrentNhntID = T - dmxHdlr->AvcFileN;
            }
            break;
        }
        // Find the full image (I or Idr frame) ahead. The NhntID of full images is divisible by N value.
        T += dmxHdlr->AvcFileN;
    }
    if (T >= dmxHdlr->NhntDescCount) {
        // Go to the last I or Idr frame
        dmxHdlr->CurrentNhntID = ((dmxHdlr->NhntDescCount - 1) / dmxHdlr->AvcFileN) * dmxHdlr->AvcFileN;
    }

    AmpUT_Video_DummyDmx_Feed(dmxHdlr, preFeed); // prefeed frames

    // Wait until feeding complete
    AmpUT_Video_DummyDmx_WaitFeedDone(dmxHdlr);

    AmbaPrint("%s: Prefeed done", __FUNCTION__);

    return 0;
}

int AmpUT_Video_DummyDmx_Close(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr)
{
    int Er = 0;
    if (dmxHdlr->IsInit == 0) {
        return -1; // Error
    }
    if (dmxHdlr->IsOpened == 0) {
        return 0; // Success. Already opened.
    }

    AmbaPrint("%s", __FUNCTION__);
    AmpUT_Video_DummyDmx_Lock(dmxHdlr, AMBA_KAL_WAIT_FOREVER);
    AmbaPrint("%s 1", __FUNCTION__);
    dmxHdlr->PreviousPts = VIDEODEC_FRAME_COUNT_EOS;
    dmxHdlr->PutHeader = 0;
    dmxHdlr->VideoDecFrmCnt = VIDEODEC_FRAME_COUNT_EOS;
    dmxHdlr->FrameSeqNum = 0;

    if (dmxHdlr->AvcRawFile != NULL) {
        Er = AmpCFS_fclose(dmxHdlr->AvcRawFile);
        if (Er != AMP_OK) {
            AmpVideoDecErr(RED, "%s Failed to close raw file (%d)", __FUNCTION__, Er);
            AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
            return -1; // Error
        }
        dmxHdlr->AvcRawFile = NULL;
    }

    if (dmxHdlr->AvcIdxFile != NULL) {
        Er = AmpCFS_fclose(dmxHdlr->AvcIdxFile);
        if (Er != AMP_OK) {
            AmpVideoDecErr(RED, "%s Failed to close idx file (%d)", __FUNCTION__, Er);
            AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
            return -1; // Error
        }
        dmxHdlr->AvcIdxFile = NULL;
    }

    if (dmxHdlr->AvcUdtFile != NULL) {
        Er = AmpCFS_fclose(dmxHdlr->AvcUdtFile);
        if (Er != AMP_OK) {
            AmpVideoDecErr(RED, "%s Failed to close user data file (%d)", __FUNCTION__, Er);
            AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
            return -1; // Error
        }
        dmxHdlr->AvcUdtFile = NULL;
    }

    dmxHdlr->IsOpened = 0;
    AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
    return 0;
}

int AmpUT_Video_DummyDmx_Delete(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr)
{
    int Er = 0;

    if (dmxHdlr->IsInit != 0) {
        if (dmxHdlr->IsOpened) {
            Er = AmpUT_Video_DummyDmx_Close(dmxHdlr);
            if (Er != 0) {
                return -1;
            }
        }

        // Delete mutex
        Er = AmbaKAL_MutexDelete(&dmxHdlr->VideoDecFrmMutex);
        if (Er != OK) {
            AmpVideoDecErr(RED, "Failed to delete mutex: %d", Er);
        }

        // Delete fifo
        if (dmxHdlr->AvcDecFifoHdlr != NULL) {
            // Erase data in fifo
            Er = AmpFifo_EraseAll(dmxHdlr->AvcDecFifoHdlr);
            if (Er != OK) {
                AmpVideoDecErr(RED, "Failed to erase fifo: %d", Er);
            }

            // Delete fifo
            Er = AmpFifo_Delete(dmxHdlr->AvcDecFifoHdlr);
            if (Er != OK) {
                AmpVideoDecErr(RED, "Failed to delete fifo: %d", Er);
            }
            dmxHdlr->AvcDecFifoHdlr = NULL;
        }

        dmxHdlr->IsInit = 0;
    }

    return 0;
}

int AmpUT_Video_DummyDmx_Exit(void)
{
    int Er = 0;
    UINT32 T = 0;
    int ReturnValue = 0; // 0: Success

    // Delete task
    if (VideoDecDemuxTaskIsInit != 0) {
        Er = AmbaKAL_TaskTerminate(&VideoDecDemuxTask);
        // TX_THREAD_ERROR: The task is not created.
        if ((Er != AMP_OK) && (Er != TX_THREAD_ERROR)) {
            AmpVideoDecErr(RED, "%s:%u Failed to terminate task (%d).", __FUNCTION__, __LINE__, Er);
        }
        Er = AmbaKAL_TaskDelete(&VideoDecDemuxTask);
        // TX_THREAD_ERROR: The task is not created.
        // TX_DELETE_ERROR: The task is not terminated.
        if ((Er != AMP_OK) && (Er != TX_THREAD_ERROR) && (Er != TX_DELETE_ERROR)) {
            AmpVideoDecErr(RED, "%s:%u Failed to delete task (%d).", __FUNCTION__, __LINE__, Er);
            ReturnValue = -1; // Error
        }
        VideoDecDemuxTaskIsInit = 0;
    }

    // Delete all dummy demux handlers
    for (T = 0; T < DUMMY_DEMUX_MAX_NUM; ++T) {
        Er = AmpUT_Video_DummyDmx_Delete(&DummyDmxHdlr[T]);
        if (Er != 0) {
            ReturnValue = -1; // Error
        }
    }

    return ReturnValue;
}

int AmpUT_Video_DummyDmx_Feed(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr,
                                      int req)
{
    if (dmxHdlr->IsInit == 0) {
        return -1; // Error
    }
    AmpUT_Video_DummyDmx_Lock(dmxHdlr, AMBA_KAL_WAIT_FOREVER);
    if (dmxHdlr->IsOpened == 0 || dmxHdlr->VideoDecFrmCnt == VIDEODEC_FRAME_COUNT_EOS) {
        AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
        return -1; // Error
    }
    dmxHdlr->VideoDecFrmCnt += req;
    AmpUT_Video_DummyDmx_UnLock(dmxHdlr);
    return 0;
}

int AmpUT_Video_DummyDmx_WaitFeedDone(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr)
{
    while (dmxHdlr->VideoDecFrmCnt != 0) {
        if (dmxHdlr->VideoDecFrmCnt == VIDEODEC_FRAME_COUNT_EOS) { // EOS
            return 0;
        }
        AmbaKAL_TaskSleep(10);
        AmpVideoDecDbg(GRAY, "%s: %d", __FUNCTION__, dmxHdlr->VideoDecFrmCnt);
    }
    return 0;
}

