 /**
  * @file src/unittest/MWUnitTest/VideoDummyDemux.h
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

#include "AmpUnitTest.h"
#include <player/Decode.h>
#include <player/VideoDec.h>
#include "DecCFSWrapper.h"
//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <AmbaUtility.h>
//#include <util.h>
//#include "AmbaCache.h"
//#include "VideoDummyDemux.h"

#ifndef VIDEO_DUMMY_DEMUX_H_
#define VIDEO_DUMMY_DEMUX_H_

#define VIDEODEC_FRAME_COUNT_EOS    (0xFFFFFFFF)        ///< VideoDecFrmCnt of 0xFFFFFFFF indicates EOS
#define AMP_UT_DUMMY_DEMUX_STACK_SIZE (4096)            ///< Size of the stack for dummy demux task
#define DUMMY_DEMUX_MAX_NUM (4)                         ///< Maximum number of dummy demux handler
#define VIDEODEC_PTS_ARRAY_SIZE_MAX (32)                ///< Maximum number of elements in a PTS array. Larger than number of frames in a GOP.
#define VIDEODEC_PTS_ARRAY_SORT (0)                     ///< Sorting option of PTS array (0: Ascending  1: Descending)
#define VIDEODEC_NHNT_DESC_ARRAY_SIZE (144000)          ///< Size of NHNT array in a dummy demuxer.

typedef struct _AMP_UT_DUMMY_DEMUX_HDLR_s_ {
    UINT8 IsInit;                       ///< Whether the dummy demux is initialized
    UINT8 IsOpened;                     ///< Whether the file is opened
    AMBA_KAL_MUTEX_t VideoDecFrmMutex;  ///< Mutex for reading and writing frames to fifo
    AMP_AVDEC_HDLR_s *AvcDecHdlr;       ///< Video Codec Handler
    AMP_FIFO_HDLR_s *AvcDecFifoHdlr;    ///< Fifo Handler
    UINT8* AvcRawBuffer;                ///< Aligned buffer address of video raw data
    UINT8* AvcRawLimit;                 ///< Maximum address in raw buffer
    UINT32 VideoDecFrmCnt;              ///< How many frames to feed. A value of 0xFFFFFFFF indicates EOS.
    AMP_CFS_FILE_s *AvcIdxFile;         ///< File system of h264 index file (.nhnt)
    AMP_CFS_FILE_s *AvcRawFile;         ///< File system of h264 raw file (.h264)
    AMP_CFS_FILE_s *AvcUdtFile;         ///< File system of user data file (.udta)
    NHNT_HEADER_s NhntHeader;           ///< Data from ".nhnt" file
    NHNT_SAMPLE_HEADER_s NhntDescArray[VIDEODEC_NHNT_DESC_ARRAY_SIZE]; ///< Nhnt descriptor from ".nhnt" file
    UINT32 NhntDescCount;               ///< Number of nhnt descriptor from ".nhnt" file
    UINT32 CurrentNhntID;               ///< Current ID in NhntDescArray
    FORMAT_USER_DATA_s AvcUserData;     ///< Data from ".udta" file
    UINT32 AvcFileTimeScale;            ///< How many units of "file time" per second (ex. 60000)
    UINT32 AvcFileTimePerFrame;         ///< How many units of "file time" per frame  (ex. 1001)
    UINT8 AvcFileM;                     ///< The distance between two anchor frames (I or P)
    UINT8 AvcFileN;                     ///< The distance between two full images (I-frames)
    UINT32 AvcFileWidth;                ///< The width of frame
    UINT32 AvcFileHeight;               ///< The height of frame
    UINT8 AvcFileInterlaced;            ///< 1: Interlaced video. 0: Progressive video.
    UINT8 PutHeader;                    ///< If 1, notify demux to put GOP header in raw buffer.
    UINT8 IsWaitingSpace;               ///< Whether the dummy demux is waiting for enough space in raw buffer to feed data
    UINT32 PreviousPts;                 ///< The last fed PTS
    UINT32 FrameSeqNum;                 ///< Sequential number of the next frame
    UINT8 IsFirstGop;                   ///< 1: No GOP header has been put. 0: More than 1 GOP header has been put.
} AMP_UT_DUMMY_DEMUX_HDLR_s;

typedef struct _AMP_UT_DUMMY_DEMUX_CREATE_CFG_s_ {
    AMP_AVDEC_HDLR_s *AvcDecHdlr;       ///< Video Codec Handler
    UINT8* AvcRawBuffer;                ///< Aligned buffer address of video raw data
    UINT8* AvcRawLimit;                 ///< Maximum address in raw buffer
} AMP_UT_DUMMY_DEMUX_CREATE_CFG_s;

/**
 * Lock dummy demux handler.\n
 * Enter critical section of the dummy demux.
 *
 * @param [in] dmxHdlr      Dummy demux handler
 * @param [in] Timeout      Timeout in ms
 *
 * @return 0 - OK, others - Error
 */
extern int AmpUT_Video_DummyDmx_Lock(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr, UINT32 timeout);

/**
 * Unlock dummy demux handler.\n
 * Exit critical section of the dummy demux.
 *
 * @param [in] dmxHdlr      Dummy demux handler
 *
 * @return 0 - OK, others - Error
 */
extern int AmpUT_Video_DummyDmx_UnLock(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr);

/**
 * Initialize all dummy demux and create task.
 *
 * @return 0 - OK, others - Error
 */
extern int AmpUT_Video_DummyDmx_Init(void);

extern int AmpUT_Video_DummyDmx_Create(const AMP_UT_DUMMY_DEMUX_CREATE_CFG_s* dummyDemuxCfg, AMP_UT_DUMMY_DEMUX_HDLR_s** outDmxHdlr);

/**
 * Open files of dummy demux.
 *
 * @param [in] dmxHdlr      Dummy demux handler
 * @param [in] rawFn        Full path of raw file (.h264) in Unicode format
 * @param [in] idxFn        Full path of idx file (.nhnt) in Unicode format
 * @param [in] udtFn        Full path of user data file (.udta) in Unicode format
 *
 * @return 0 - OK, others - Error
 */
extern int AmpUT_Video_DummyDmx_Open(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr,
                                     char* rawFn,
                                     char* idxFn,
                                     char* udtFn);

extern int AmpUT_Video_DummyDmx_Start(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr,
                                      UINT32 startTime,
                                      AMP_AVDEC_PLAY_DIRECTION_e direction,
                                      UINT32 speed,
                                      UINT32 preFeed);

/**
 * Close files of dummy demux.
 *
 * @param [in] dmxHdlr      Dummy demux handler
 *
 * @return 0 - OK, others - Error
 */
extern int AmpUT_Video_DummyDmx_Close(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr);

/**
 * Delete dummy demux.
 *
 * @param [in] dmxHdlr      Dummy demux handler
 *
 * @return 0 - OK, others - Error
 */
extern int AmpUT_Video_DummyDmx_Delete(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr);

/**
 * Deinit all dummy demux and delete task.
 *
 * @return 0 - OK, others - Error
 */
extern int AmpUT_Video_DummyDmx_Exit(void);

/**
 * Request dummy demux to feed frames.
 *
 * @param [in] dmxHdlr      Dummy demux handler
 * @param [in] req          Number of frames requested to feed
 *
 * @return 0 - OK, others - Error
 */
extern int AmpUT_Video_DummyDmx_Feed(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr,
                                     int req);

/**
 * Wait for dummy demux to finish feeding frames.
 *
 * @param [in] dmxHdlr      Dummy demux handler
 *
 * @return 0 - OK, others - Error
 */
extern int AmpUT_Video_DummyDmx_WaitFeedDone(AMP_UT_DUMMY_DEMUX_HDLR_s* dmxHdlr);

#endif /* VIDEO_DUMMY_DEMUX_H_ */
