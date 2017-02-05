#include "DummyAudioDec.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DmyDec_Perror(level, color, fmt, ...) {\
    char Buffer[160];\
    snprintf(Buffer, 160, fmt, ##__VA_ARGS__);\
    Buffer[159] = '\0';\
    AmbaPrintColor(color, "[Error]%s:%u %s", __FUNCTION__, __LINE__, Buffer);\
}

typedef enum {
    DECODER_STATE_IDLE,
    DECODER_STATE_RUNNING,
    DECODER_STATE_STOPPING
} DECODER_STATE_e;

static int DummyAudioDec_RingBufCB(void *hdlr, UINT32 event, void* info)
{
//    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);

    return 0;
}

static int DummyAudioDec_OpenFile(DummyAudioDec_s *pDecoder)
{
    AMP_CFS_FILE_PARAM_s cfsParam;
    // Open Raw file (to make code clean, not to close the file)
    if (AmpCFS_GetFileParam(&cfsParam) != AMP_OK) {
        DmyDec_Perror(0, 0, "AmpCFS_GetFileParam() failed");
        return -1;
    }
    cfsParam.Mode = AMP_CFS_FILE_MODE_WRITE_ONLY;
    strncpy(cfsParam.Filename, pDecoder->szBsName, MAX_FILENAME_LENGTH);
    cfsParam.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
    pDecoder->pRawFile = AmpCFS_fopen(&cfsParam);
    if (pDecoder->pRawFile == NULL) {
        DmyDec_Perror(0, 0, "AmpCFS_fopen() failed");
        return -1;
    }
    return 0;
}

static int DummyAudioDec_CloseFile(DummyAudioDec_s *pDecoder)
{
    int rval = 0;
    if (pDecoder->pRawFile != NULL) {
        if (AmpCFS_fclose(pDecoder->pRawFile) != AMP_OK) {
            DmyDec_Perror(0, 0, "AmpCFS_fclose() failed");
            rval = -1;
        }
        pDecoder->pRawFile = NULL;
    }
    return rval;
}

static void DummyAudioDec_DecTask(UINT32 info)
{
    int rval = 0;
    DummyAudioDec_s * const pDecoder = (DummyAudioDec_s *)info;
    AMP_BITS_DESC_s *pTmpDesc = NULL;
    AMP_FIFO_INFO_s pTmpInfo = {0};
    UINT32 nPreAddr = 0;
    UINT32 nFrameCount = 0;
    UINT64 nTotalSize = 0;
    int r;
    while (TRUE) {
        if (AmpFifo_GetInfo(pDecoder->pFifo, &pTmpInfo) != AMP_OK) {
            DmyDec_Perror(0, 0, "AmpFifo_GetInfo() failed");
            goto Done;
        }
        if (pTmpInfo.AvailEntries < pTmpInfo.TotalEntries) {
            if (pDecoder->fnDataRequest(pDecoder) != AMP_OK) {
                DmyDec_Perror(0, 0, "fnDataRequest() failed");
                goto Done;
            }
        }
        r = AmpFifo_PeekEntry(pDecoder->pFifo, &pTmpDesc, 0);
        if (r != AMP_OK) {
            if (r != AMP_ERROR_FIFO_EMPTY) {
                DmyDec_Perror(0, 0, "AmpFifo_PeekEntry() failed");
                goto Done;
            }
        } else {
            if ((pTmpDesc->Type != AMP_FIFO_TYPE_EOS) && (pTmpDesc->Type != AMP_FIFO_TYPE_DECODE_MARK)) {
//                    AmbaPrint("Fifo[%p] Read %u: pts:%llu, ftype:%u, size:%u, total size:%llu", pDecoder->pFifo, nFrameCount, pTmpDesc->Pts, pTmpDesc->Type, pTmpDesc->Size, nTotalSize);
                if ((UINT32)pTmpDesc->StartAddr == nPreAddr) {
                    AmbaPrint("%s, same address", __FUNCTION__);
                } else {
                    nPreAddr = (UINT32)pTmpDesc->StartAddr;
                }
                while (TRUE) {
                    int r;
                    r = RingBuf_MoveFrameToFile(&pDecoder->RingBuf, pDecoder->pRawFile);
                    if (r == -1) { //  error
                        DmyDec_Perror(0, 0, "RingBuf_MoveFrameToFile() failed!");
                        goto Done;
                    } else if (r == 0) { // success
                        break;
                    }
                    // wait for updating wp
                    AmbaKAL_TaskSleep(1);
                }
                nTotalSize += pTmpDesc->Size;
                nFrameCount++;
            }
            if (AmpFifo_RemoveEntry(pDecoder->pFifo, 1) != AMP_OK) {
                DmyDec_Perror(0, 0, "AmpFifo_RemoveEntry() failed");
                goto Done;
            }
        }
        AmbaKAL_TaskSleep(10);
        if (pDecoder->nState == DECODER_STATE_STOPPING) {
            if (AmpFifo_GetInfo(pDecoder->pFifo, &pTmpInfo) != AMP_OK) {
                DmyDec_Perror(0, 0, "AmpFifo_GetInfo() failed");
                goto Done;
            }
            if (pTmpInfo.AvailEntries == 0) {
                pDecoder->nState = DECODER_STATE_IDLE;
                break;
            }
        }
    }
Done:
    DummyAudioDec_CloseFile(pDecoder);
    if (AmbaKAL_EventFlagGive(&pDecoder->DecStop, 0x00000001) != OK)
        DmyDec_Perror(0, 0, "AmbaKAL_EventFlagGive() failed");
    AmbaPrint("%s() is stopped", __FUNCTION__);
    if (rval == -1)
        AmbaPrint("[FAIL]: DeMuxing Failed!");
}

UINT32 DummyAudioDec_GetRequiredBufSize(UINT32 nRawBufSize, UINT32 nMaxQueuedFrame)
{
    return RingBuf_GetRequiredBufSize(nRawBufSize, nMaxQueuedFrame);
}

int DummyAudioDec_Init(DummyAudioDec_s *pDecoder, char *szBsName, UINT8 *pBuffer, UINT32 nBufferSize, UINT32 nMaxQueuedFrame, DUMMY_AUDIO_DEC_DATA_REQUEST_FP fnDataRequest)
{
    const UINT32 nRawBufSize = nBufferSize - nMaxQueuedFrame * sizeof(RING_BUFFER_DESC_s);
    K_ASSERT(nBufferSize == DummyAudioDec_GetRequiredBufSize(nRawBufSize, nMaxQueuedFrame));
    memset(pDecoder, 0, sizeof(DummyAudioDec_s));
    strncpy(pDecoder->szBsName, szBsName, MAX_FILENAME_LENGTH);
    pDecoder->szBsName[MAX_FILENAME_LENGTH - 1] = '\0';
    pDecoder->nState = DECODER_STATE_IDLE;
    pDecoder->pBuffer = pBuffer;
    pDecoder->fnDataRequest = fnDataRequest;
    if (RingBuf_Init(&pDecoder->RingBuf, (UINT8 *)pDecoder->pBuffer, nBufferSize, nMaxQueuedFrame, DummyAudioDec_RingBufCB) != 0) {
        DmyDec_Perror(0, 0, "RingBuf_Init() failed");
        return -1;
    }
    if (DummyAudioDec_OpenFile(pDecoder) < 0) {
        DmyDec_Perror(0, 0, "DummyAudioDec_OpenFile() failed");
        return -1;
    }
    if (AmbaKAL_EventFlagCreate(&pDecoder->DecStop) != OK) {
        DmyDec_Perror(0, 0, "AmbaKAL_EventFlagCreate() failed");
        return -1;
    }
    /* Create Encode task */
    if (AmbaKAL_TaskCreate(&pDecoder->DecTask, "DummyAudioDec", DUMMY_AUDIO_DEC_PRIORITY, DummyAudioDec_DecTask, (UINT32)pDecoder, (void *)pDecoder->DecStack, sizeof(pDecoder->DecStack), AMBA_KAL_DO_NOT_START) != OK) {
        DmyDec_Perror(0, 0, "AmbaKAL_TaskCreate() failed");
        return -1;
    }
    return 0;
}

int DummyAudioDec_Reset(DummyAudioDec_s *pDecoder, AMP_FIFO_HDLR_s *pFifo)
{
    if (DummyAudioDec_CloseFile(pDecoder) < 0) {
        DmyDec_Perror(0, 0, "DummyAudioDec_CloseFile() failed");
        return -1;
    }
    if (DummyAudioDec_OpenFile(pDecoder) < 0) {
        DmyDec_Perror(0, 0, "DummyAudioDec_OpenFile() failed");
        return -1;
    }
    RingBuf_Reset(&pDecoder->RingBuf);
    // reset tasks
    pDecoder->pFifo = pFifo;
    if (AmbaKAL_TaskReset(&pDecoder->DecTask) != OK) {
//        DmyDec_Perror(0, 0);
//        return -1;
    }
    return 0;
}

int DummyAudioDec_Start(DummyAudioDec_s *pDecoder)
{
    pDecoder->nState = DECODER_STATE_RUNNING;
    if (AmbaKAL_EventFlagClear(&pDecoder->DecStop, 0x00000001) != OK) {
        DmyDec_Perror(0, 0, "AmbaKAL_EventFlagClear() failed");
        return -1;
    }
    if (AmbaKAL_TaskResume(&pDecoder->DecTask) != OK) {
        DmyDec_Perror(0, 0, "AmbaKAL_TaskResume() failed");
        return -1;
    }
    return 0;
}

int DummyAudioDec_Stop(DummyAudioDec_s *pDecoder)
{
    UINT32 flag;
    pDecoder->nState = DECODER_STATE_STOPPING;
    if (AmbaKAL_EventFlagTake(&pDecoder->DecStop, 0x00000001, AMBA_KAL_AND_CLEAR, &flag, AMBA_KAL_WAIT_FOREVER) != OK) {
        DmyDec_Perror(0, 0, "AmbaKAL_EventFlagTake() failed");
        return -1;
    }
    return 0;
}

