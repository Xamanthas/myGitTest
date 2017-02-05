#include "DummyVideoEnc_Net.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaTimer.h>

static UINT8 dmmvenc_inited = 0;
#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

static void DmyEnc_PerrorImpl(UINT32 nLevel, UINT32 nColor, UINT32 nLine)
{
    AmbaPrint("[Error]%s:%u", FILENAME, nLine);
}

#define DmyEnc_Perror(nLevel, nColor) {\
    DmyEnc_PerrorImpl(nLevel, nColor, __LINE__);\
}

static int DummyVideoEnc_Net_RingBufCB(void *hdlr, UINT32 event, void* info)
{
//    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);

    return 0;
}

static int DummyVideoEnc_Net_OpenFile(DummyVideoEnc_Net_s *pEncoder)
{
    AMP_CFS_FILE_PARAM_s cfsParam;
    if (AmpCFS_GetFileParam(&cfsParam) != AMP_OK) {
        DmyEnc_Perror(0, 0);
        return -1;
    }
    cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
    strncpy(cfsParam.Filename, pEncoder->szHdrName, MAX_FILENAME_LENGTH);
    cfsParam.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
    pEncoder->pIdxFile = AmpCFS_fopen(&cfsParam);
    if (pEncoder->pIdxFile == NULL) {
        DmyEnc_Perror(0, 0);
        return -1;
    }
    // Open Raw file (to make code clean, not to close the file)
    if (AmpCFS_GetFileParam(&cfsParam) != AMP_OK) {
        DmyEnc_Perror(0, 0);
        return -1;
    }
    cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
    strncpy(cfsParam.Filename, pEncoder->szBsName, MAX_FILENAME_LENGTH);
    cfsParam.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
    pEncoder->pRawFile = AmpCFS_fopen(&cfsParam);
    if (pEncoder->pRawFile == NULL) {
        DmyEnc_Perror(0, 0);
        return -1;
    }
    return 0;
}

static int DummyVideoEnc_Net_CloseFile(DummyVideoEnc_Net_s *pEncoder)
{
    int rval = 0;
    if (pEncoder->pIdxFile != NULL) {
        if (AmpCFS_fclose(pEncoder->pIdxFile) != AMP_OK) {
            DmyEnc_Perror(0, 0);
            rval = -1;
        }
        pEncoder->pIdxFile = NULL;
    }
    if (pEncoder->pRawFile != NULL) {
        if (AmpCFS_fclose(pEncoder->pRawFile) != AMP_OK) {
            DmyEnc_Perror(0, 0);
            rval = -1;
        }
        pEncoder->pRawFile = NULL;
    }
    return rval;
}

static void DummyVideoEnc_Net_EncTask(UINT32 info)
{
    DummyVideoEnc_Net_s * const pEncoder = (DummyVideoEnc_Net_s *)info;
    UINT32 nFrameCount = 0;
    UINT64 nTotalSize = 0;
    UINT32 nIdxLen = 0;
    UINT32 t_st;
    int t_diff, t_pf;
    RING_BUFFER_INFO_s RingBufInfo = {0};
    AMP_BITS_DESC_s tmpDesc = {0};
    AMP_FIFO_INFO_s tmpInfo = {0};

    MP4_NHNT_SAMPLE_HEADER_s nhntDesc = {0};
    MP4_NHNT_HEADER_s nhntHeader = {0};

    nIdxLen = AmpCFS_fread(&nhntHeader, 1, sizeof(nhntHeader), pEncoder->pIdxFile);
    if (nIdxLen < sizeof(nhntHeader)) {
        DmyEnc_Perror(0, 0);
        goto Done;
    }

    AmbaPrint("Raw File sizeof(MP4_NHNT_HEADER_s) = (%d):", sizeof(MP4_NHNT_HEADER_s));
    AmbaPrint("Symbol:%c%c%c%c", nhntHeader.Signature[0], nhntHeader.Signature[1], nhntHeader.Signature[2], nhntHeader.Signature[3]);
    AmbaPrint("avgBitRate:%d", nhntHeader.avgBitRate);
    AmbaPrint("maxBitRate:%d", nhntHeader.maxBitRate);
    AmbaPrint("timeStampResolution:%d", nhntHeader.timeStampResolution);
    t_pf = nhntHeader.timeStampResolution/1000;

    while (pEncoder->nState != ENCODER_STATE_IDLE) {
        t_st = AmbaTimer_GetSysTickCount(); //start_time
        if (AmpFifo_GetInfo(pEncoder->pFifo, &tmpInfo) != AMP_OK) {
            DmyEnc_Perror(0, 0);
            goto Done;
        }
        if (tmpInfo.AvailEntries < tmpInfo.TotalEntries) {
            if (AmpCFS_ftell(pEncoder->pIdxFile) == pEncoder->nHDRFlen) {
                AmbaPrint("Reach Video EOS");
                memset (&tmpDesc, 0, sizeof(tmpDesc));
                tmpDesc.Pts = 0xffffffff;
                tmpDesc.Completed = 1;
                tmpDesc.SeqNum = 0xffffffff;
                tmpDesc.Size = AMP_FIFO_MARK_EOS;// EOS magic num // TBD
                tmpDesc.Type = AMP_FIFO_TYPE_EOS;// EOS Frame
                if (AmpFifo_WriteEntry(pEncoder->pFifo, &tmpDesc) != AMP_OK) {
                    DmyEnc_Perror(0, 0);
                    goto Done;
                }
                break;
            } else {
                /**< Read frame header */
                nIdxLen = AmpCFS_fread(&nhntDesc, 1, sizeof(nhntDesc), pEncoder->pIdxFile);
                if (nIdxLen < sizeof(nhntDesc)) {
                    DmyEnc_Perror(0, 0);
                    goto Done;
                }
                tmpDesc.Pts = nhntDesc.compositionTimeStamp;
                tmpDesc.Completed = 1;
                tmpDesc.Size = nhntDesc.dataSize;
                tmpDesc.Type = (AMP_FIFO_FRMAE_TYPE_e)nhntDesc.frameType;
                tmpDesc.SeqNum = nFrameCount;
                nTotalSize += tmpDesc.Size;
                while (TRUE) {
                    if (pEncoder->nState == ENCODER_STATE_PAUSING) {
                        AMP_BITS_DESC_s eos = {0};
                        if (tmpDesc.Type != AMP_FIFO_TYPE_IDR_FRAME)
                            break;
                        eos.Size = AMP_FIFO_MARK_EOS_PAUSE;
						AmbaPrint("Reach Video PAUSE");
                        AmpFifo_WriteEntry(pEncoder->pFifo, &eos);
                        pEncoder->nState = ENCODER_STATE_PAUSED;
                        AmbaKAL_TaskSleep(200);
                    } else if (pEncoder->nState == ENCODER_STATE_RESUME) {
                        pEncoder->nState = ENCODER_STATE_RUNNING;
                        break;
                    } else if (pEncoder->nState == ENCODER_STATE_PAUSED) {
                        AmbaKAL_TaskSleep(200);
                    } else {
                        break;
                    }
                }

                //AmbaPrint("Fifo[%p] Read %u: pts:%llu, ftype:%u, size:%u, total size:%llu", pEncoder->pFifo, nFrameCount, tmpDesc.Pts, nhntDesc.frameType, tmpDesc.Size, nTotalSize);

                /**< Read frame from BitStream*/
                RingBuf_GetInfo(&pEncoder->RingBuf, &RingBufInfo);
                tmpDesc.Align = 0;
                tmpDesc.StartAddr = RingBufInfo.pRingBufWp;

                if (AmpCFS_fseek(pEncoder->pRawFile, nhntDesc.fileOffset, AMP_CFS_SEEK_START) != AMP_OK) {
                    DmyEnc_Perror(0, 0);
                    goto Done;
                }
                while (TRUE) {
                    const int r = RingBuf_CreateFrame(&pEncoder->RingBuf, pEncoder->pRawFile, tmpDesc.Size);
                    if (r == tmpDesc.Size)
                        break;
                    if (r == 0) {
                        AmbaKAL_TaskSleep(1000);
                    } else {
                        DmyEnc_Perror(0, 0);
                        goto Done;
                    }
                }

                // write to fifo
                if (AmpFifo_WriteEntry(pEncoder->pFifo, &tmpDesc) != AMP_OK) {
                    DmyEnc_Perror(0, 0);
                    goto Done;
                }
                nFrameCount++;
            }

            t_diff = AmbaTimer_GetSysTickCount() - t_st;
            if (t_diff < 0) {
                t_diff = t_pf + t_diff;
                AmbaPrint("t_pf=%u, sleep %d ms",t_pf, t_diff);
            } else {
                t_diff = t_pf - t_diff;
            }
            //AmbaPrint("t_pf=%u, sleep %d ms",t_pf, t_diff);
            AmbaKAL_TaskSleep(t_diff);
        } else {
            AmbaKAL_TaskSleep(1000);
        }
    }
Done:
    DummyVideoEnc_Net_CloseFile(pEncoder);
    if (AmbaKAL_EventFlagGive(&pEncoder->EncStop, 0x00000001) != OK) {
        DmyEnc_Perror(0, 0);
    }
    if (tmpDesc.Size == AMP_FIFO_MARK_EOS) { //exit due to EOS
        pEncoder->nState = ENCODER_STATE_IDLE;
    }
    AmbaPrint("%s() is stopped", __FUNCTION__);
}

UINT32 DummyVideoEnc_Net_GetRequiredBufSize(UINT32 nRawBufSize, UINT32 nMaxQueuedFrame)
{
    return RingBuf_GetRequiredBufSize(nRawBufSize, nMaxQueuedFrame);
}

static void DummyVideoEnc_Net_KillEncTask(AMBA_KAL_TASK_t *pTask, UINT32 Condition)
{
    if (Condition == TX_THREAD_EXIT) {
        if (AmbaKAL_TaskDelete(pTask) != OK) {
            AmbaPrint("%s: failed!", __func__);
        }
    }
}

int DummyVideoEnc_Net_Init(DummyVideoEnc_Net_s *pEncoder, char *szBsName, char *szHdrName, UINT8 *pBuffer, UINT32 nBufferSize, UINT32 nMaxQueuedFrame)
{
    AMP_CFS_STAT stat;
    UINT32 nRawBufSize;

    if(dmmvenc_inited == 1) {
        AmbaPrint("%s: aready inited!",__FUNCTION__);
        return 0;
    }

    nRawBufSize = nBufferSize - nMaxQueuedFrame * sizeof(RING_BUFFER_DESC_s);
    K_ASSERT(nBufferSize == DummyVideoEnc_Net_GetRequiredBufSize(nRawBufSize, nMaxQueuedFrame));

    memset(pEncoder, 0, sizeof(DummyVideoEnc_Net_s));
    strncpy(pEncoder->szBsName, szBsName, MAX_FILENAME_LENGTH);
    pEncoder->szBsName[MAX_FILENAME_LENGTH - 1] = '\0';
    strncpy(pEncoder->szHdrName, szHdrName, MAX_FILENAME_LENGTH);
    pEncoder->szHdrName[MAX_FILENAME_LENGTH - 1] = '\0';
    pEncoder->nState = ENCODER_STATE_IDLE;
    pEncoder->pBuffer = pBuffer;
    if (RingBuf_Init(&pEncoder->RingBuf, (UINT8 *)pEncoder->pBuffer, nBufferSize, nMaxQueuedFrame, DummyVideoEnc_Net_RingBufCB) != 0) {
        DmyEnc_Perror(0, 0);
        return -1;
    }
    // Open Index file (to make code clean, not to close the file)
    if (AmpCFS_Stat(pEncoder->szHdrName, &stat) != AMP_OK) {
        DmyEnc_Perror(0, 0);
        return -1;
    }
    pEncoder->nHDRFlen = stat.Size;
    if (DummyVideoEnc_Net_OpenFile(pEncoder) < 0) {
        DmyEnc_Perror(0, 0);
        return -1;
    }
    if (AmbaKAL_EventFlagCreate(&pEncoder->EncStop) != OK) {
        DmyEnc_Perror(0, 0);
        return -1;
    }

    dmmvenc_inited = 1;
    return 0;
}

int DummyVideoEnc_Net_Reset(DummyVideoEnc_Net_s *pEncoder, AMP_FIFO_HDLR_s *pFifo)
{
    if (DummyVideoEnc_Net_CloseFile(pEncoder) < 0) {
        DmyEnc_Perror(0, 0);
        return -1;
    }
    if (DummyVideoEnc_Net_OpenFile(pEncoder) < 0) {
        DmyEnc_Perror(0, 0);
        return -1;
    }
    RingBuf_Reset(&pEncoder->RingBuf);
    // reset tasks
    pEncoder->pFifo = pFifo;

    return 0;
}

int DummyVideoEnc_Net_Start(DummyVideoEnc_Net_s *pEncoder)
{
    if(pEncoder->nState == ENCODER_STATE_RUNNING) { //alread running
        return 0;
    }
    pEncoder->nState = ENCODER_STATE_RUNNING;
    if (AmbaKAL_EventFlagClear(&pEncoder->EncStop, 0x00000001) != OK) {
        DmyEnc_Perror(0, 0);
        return -1;
    }

    /* Create Encode task */
    if (AmbaKAL_TaskCreate(&pEncoder->EncTask,
                           "DummyVideoEnc_Net",
                           DUMMY_VIDEO_ENC_NET_PRIORITY,
                           DummyVideoEnc_Net_EncTask,
                           (UINT32)pEncoder,
                           (void *)pEncoder->EncStack,
                           sizeof(pEncoder->EncStack),
                           AMBA_KAL_AUTO_START) != OK) {
        DmyEnc_Perror(0, 0);
        return -1;
    }

    if (tx_thread_entry_exit_notify(&pEncoder->EncTask, DummyVideoEnc_Net_KillEncTask) != OK) {
        AmbaPrint("thread_entry_exit_notify registered failed.\n");
        DmyEnc_Perror(0, 0);
        return -1;
    }

    return 0;
}

int DummyVideoEnc_Net_Stop(DummyVideoEnc_Net_s *pEncoder)
{
    UINT32 flag;

    if(pEncoder->nState == ENCODER_STATE_IDLE) {
        return 0;
    }

    pEncoder->nState = ENCODER_STATE_IDLE;
    if (AmbaKAL_EventFlagTake(&pEncoder->EncStop, 0x00000001, AMBA_KAL_AND_CLEAR, &flag, AMBA_KAL_WAIT_FOREVER) != OK) {
        DmyEnc_Perror(0, 0);
        return -1;
    }
    return 0;
}

int DummyVideoEnc_Net_Pause(DummyVideoEnc_Net_s *pEncoder)
{
    pEncoder->nState = ENCODER_STATE_PAUSING;
    return 0;
}

int DummyVideoEnc_Net_Resume(DummyVideoEnc_Net_s *pEncoder)
{
    pEncoder->nState = ENCODER_STATE_RESUME;
    return 0;
}

int DummyVideoEnc_Net_QueryState(DummyVideoEnc_Net_s *pEncoder)
{
    if (pEncoder==NULL) {
        return -1;
    }
    return pEncoder->nState;
}
