#ifndef __DUMMYVIDEOENC_NET_H__

#include "RingBuffer.h"

#define DUMMY_VIDEO_ENC_NET_PRIORITY    40

typedef enum {
    ENCODER_STATE_IDLE,
    ENCODER_STATE_RUNNING,
    ENCODER_STATE_PAUSING,
    ENCODER_STATE_PAUSED,
    ENCODER_STATE_RESUME
} ENCODER_STATE_e;

typedef struct {
    AMBA_KAL_EVENT_FLAG_t EncStop;
    AMP_CFS_FILE_s *pIdxFile;
    AMP_CFS_FILE_s *pRawFile;
    UINT64 nHDRFlen;
    UINT32 nState;
    AMBA_KAL_TASK_t EncTask;
    char EncStack[0x8000];
    RingBuf_s RingBuf;
    UINT8 *pBuffer;
    AMP_FIFO_HDLR_s *pFifo;
    char szBsName[MAX_FILENAME_LENGTH];
    char szHdrName[MAX_FILENAME_LENGTH];
} DummyVideoEnc_Net_s;

extern UINT32 DummyVideoEnc_Net_GetRequiredBufSize(UINT32 nRawBufSize, UINT32 nMaxQueuedFrame);

extern int DummyVideoEnc_Net_Init(DummyVideoEnc_Net_s *pEncoder, char *szBsName, char *szHdrName, UINT8 *pBuffer, UINT32 nBufferSize, UINT32 nMaxQueuedFrame);

extern int DummyVideoEnc_Net_Reset(DummyVideoEnc_Net_s *pEncoder, AMP_FIFO_HDLR_s *pFifo);

extern int DummyVideoEnc_Net_Start(DummyVideoEnc_Net_s *pEncoder);

extern int DummyVideoEnc_Net_Stop(DummyVideoEnc_Net_s *pEncoder);

extern int DummyVideoEnc_Net_Pause(DummyVideoEnc_Net_s *pEncoder);

extern int DummyVideoEnc_Net_Resume(DummyVideoEnc_Net_s *pEncoder);

extern int DummyVideoEnc_Net_QueryState(DummyVideoEnc_Net_s *pEncoder);

#define __DUMMYVIDEOENC_NET_H__
#endif

