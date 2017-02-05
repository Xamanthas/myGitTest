/**
 *  @file AmpUT_StlDec.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/5/3 |cyweng       |Created        |
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include <player/Decode.h>
#include <player/StillDec.h>
#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaUtility.h>
#include <util.h>
#include <AmbaLCD.h>
#include "AmbaCache.h"
#include "DecCFSWrapper.h"
#include "AmpUT_Display.h"

#define STLDEC_RAW_SIZE     (16<<20)///< 16MB raw buffer. This limits the size of image file. Support resolution 7680x5760.
#define STLDEC_RAW_DESC_NUM (64)    ///< descriptor number = 64
#define CACHE_NUM           (32)    ///< Number of caches in cache buffer
#define CACHE_WIDTH         (128)   ///< Width of a cache in cache buffer
#define CACHE_PITCH         (128)   ///< Pitch of a cache in cache buffer
#define CACHE_HEIGHT        (128)   ///< Height of a cache in cache buffer
#define CACHE_SIZE          (CACHE_PITCH*CACHE_HEIGHT) ///< Size of a cache in cache buffer
#define IMAGE_MAX_SIZE      (64<<20)    ///< 64MB main buffer. Max image supported in pixel. This limits the size of (image width * image height).
#define STLDEC_MAIN_SIZE    (IMAGE_MAX_SIZE*2) ///< Size of the main buffer
#define STLDEC_BORROW_DSP ///< If defined, borrow DSP working area for main buffer.
#define ALIGN_TO(x,align)   ((((x)+(align)-1)/(align))*(align)) ///< Align x

static UINT8 AlphaMap[CACHE_SIZE];

extern UINT8 *DspWorkAreaResvStart; ///< dsp work area
extern UINT32 DspWorkAreaResvSize;  ///< dsp work area size

static int StillDecodeStartFlag = 0; ///< 0: Not started, 1: Started

void* StlCodecBufOri = NULL; ///< Original buffer address of still codec module
void* StlRawBuf = NULL;      ///< still raw file aligned buffer
void* StlRawBufOri = NULL;   ///< still raw file buffer
static AMP_STLDEC_HDLR_s *StlDecHdlr = NULL;    ///< still decode codec
static AMP_DEC_PIPE_HDLR_s *DecPipeHdlr = NULL; ///< decoder mgr pipe handler
static AMP_FIFO_HDLR_s *StlDecFifoHdlr = NULL;  ///< decoder fifo handler

#define VOUT_INVALID_ID (0xFF)      ///< Invalid index of Vout buffer (Defined by uCode)
#define LCD_VOUT_BUF_MAX_NUM (16)   ///< Maximum number of elements in LCD Vout buffer
#define LCD_VOUT_BUF_MIN_NUM (1)    ///< Minimum number of elements in LCD Vout buffer
static UINT8   LcdVoutIsAlloc = 0;                              ///< LCD Vout buffer is allocated or is using a shared buffer that allocated by others.
static UINT8*  LcdVoutAddrOri = NULL;                           ///< LCD Vout buffer array
static UINT8*  LcdVoutLumaAddr[LCD_VOUT_BUF_MAX_NUM] = {0};     ///< LCD Vout buffer (luma)
static UINT8*  LcdVoutChromaAddr[LCD_VOUT_BUF_MAX_NUM] = {0};   ///< LCD Vout buffer (chroma)
static UINT8   LcdVoutIdx = VOUT_INVALID_ID;                    ///< Index of currently written LCD Vout buffer
static UINT8   UpdateLcdVoutIdx = 1;                            ///< If update LcdIndex
static UINT32  LcdVoutBufferNumber = 5;                         ///< Number of LCD Vout buffers. (<= LCD_VOUT_BUF_MAX_NUM)
static UINT16  LcdVoutPitch = 1024;                             ///< LCD Vout buffer pitch. LcdVoutWidth aligned to 960 or 1024. (depending on uCode setting)
static UINT16  LcdVoutWidth = 960;                              ///< LCD Vout buffer width
static UINT16  LcdVoutHeight = 480;                             ///< LCD Vout buffer height
static UINT8   LcdSharedBufferIsValid = 0;                      ///< Whether common buffer is valid or not

#define TV_VOUT_BUF_MAX_NUM (8)     ///< Maximum number of elements in TV Vout buffer
#define TV_VOUT_BUF_MIN_NUM (1)     ///< Minimum number of elements in TV Vout buffer
#define TV_VOUT_BUF_PITCH_ALIGN (64)        ///< Alignment of TV Vout buffer pitch. If it's not aligned to 32, "display" would fail. If it's not aligned to 64, "rescale" would fail. TODO: No hardcode
#define TV_VOUT_BUF_MAX_WIDTH (1920)        ///< Maximum width of elements in TV Vout buffer
#define TV_VOUT_BUF_MAX_HEIGHT (1080)       ///< Maximum height of elements in TV Vout buffer
#define TV_VOUT_BUF_MAX_PITCH (1920)        ///< Maximum pitch of elements in TV Vout buffer. TV_VOUT_BUF_MAX_WIDTH aligned to 64.
static UINT8   TvVoutIsAlloc = 0;                               ///< TV Vout buffer is allocated. (There's no shared buffer of TV.)
static UINT8*  TvVoutAddrOri = NULL;                            ///< TV Vout buffer array
static UINT8*  TvVoutLumaAddr[TV_VOUT_BUF_MAX_NUM] = {0};       ///< TV Vout buffer (luma)
static UINT8*  TvVoutChromaAddr[TV_VOUT_BUF_MAX_NUM] = {0};     ///< TV Vout buffer (chroma)
static UINT8   TvVoutIdx = VOUT_INVALID_ID;                     ///< Index of current TV Vout buffer
static UINT32  TvVoutBufferNumber = 5;                          ///< Number of TV Vout buffers. (<= TV_VOUT_BUF_MAX_NUM)
static UINT16  TvVoutPitch = 1920;                              ///< TV Vout buffer pitch. TvVoutWidth is aligned to TV_VOUT_BUF_PITCH_ALIGN.
static UINT16  TvVoutWidth = 1920;                              ///< TV Vout buffer width
static UINT16  TvVoutHeight = 1080;                             ///< TV Vout buffer height

UINT8* DecCacheOri = NULL;                  ///< thumbnail cache
UINT8* DecCache = NULL;                     ///< thumbnail cache aligned
UINT8* DecMainBufOri = NULL;                ///< decoder buffer
UINT8* DecMainBuf = NULL;                   ///< decoder buffer aligned

/**
 * Release LCD Vout buffer
 *
 * @return 0: OK, -1: NG
 */
static int AmpUT_StlDec_ReleaseVoutBuffer_LCD(void)
{
    int Er = 0;
    int T = 0;

    if (LcdVoutIsAlloc == 0) {
        return 0; // Success
    }

    if (LcdVoutAddrOri != NULL) {
        if (AmbaKAL_BytePoolFree(LcdVoutAddrOri) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the LCD Vout buffer.", __FUNCTION__, __LINE__);
            Er = -1;
        } else {
            AmbaPrint("%s: Free LCD Vout buffer. Address = 0x%08X", __FUNCTION__, LcdVoutAddrOri);
        }
        LcdVoutAddrOri = NULL;
    }

    for (T = 0; T < LcdVoutBufferNumber; T++) {
        LcdVoutLumaAddr[T] = NULL;
        LcdVoutChromaAddr[T] = NULL;
    }
    LcdVoutIsAlloc = 0;

    return Er;
}

/**
 * Release TV Vout buffer
 *
 * @return 0: OK, -1: NG
 */
static int AmpUT_StlDec_ReleaseVoutBuffer_TV(void)
{
    int Er = 0;
    int T = 0;

    if (TvVoutIsAlloc == 0) {
        return 0; // Success
    }

    if (TvVoutAddrOri != NULL) {
        if (AmbaKAL_BytePoolFree(TvVoutAddrOri) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the TV Vout buffer.", __FUNCTION__, __LINE__);
            Er = -1;
        } else {
            AmbaPrint("%s: Free TV Vout buffer. Address = 0x%08X", __FUNCTION__, TvVoutAddrOri);
        }
        TvVoutAddrOri = NULL;
    }

    for (T = 0; T < TvVoutBufferNumber; T++) {
        TvVoutLumaAddr[T] = NULL;
        TvVoutChromaAddr[T] = NULL;
    }
    TvVoutIsAlloc = 0;

    return Er;
}

/**
 * Allocate LCD Vout buffer
 *
 * @return 0: OK, -1: NG
 */
static int AmpUT_StlDec_AllocVoutBuffer_LCD(void)
{
    int T = 0;
    AMP_DISP_INFO_s DispInfo = {0};
    UINT32 VoutSize = 0;
    UINT32 LcdPitchAlign = 1024; // Alignment of LCD buffer pitch.
    int Er = 0;

    if (LcdVoutIsAlloc) {
        AmbaPrint("%s: Already allocated", __FUNCTION__);
        return 0; // Success
    }
    if (LcdSharedBufferIsValid) {
        AmbaPrint("%s: No need to allocate. Using shared buffer.", __FUNCTION__);
        return 0; // Success
    }

    if (LcdVoutBufferNumber > LCD_VOUT_BUF_MAX_NUM || LcdVoutBufferNumber < LCD_VOUT_BUF_MIN_NUM) {
        AmbaPrint("%s:%u Illegal buffer number %u", __FUNCTION__, __LINE__, LcdVoutBufferNumber);
        return -1; // Error
    }

    // Get display info
    Er = AmpUT_Display_GetInfo(0, &DispInfo);
    if (Er != 0) {
        AmbaPrint("%s:%u AmpUT_Display_GetInfo failed", __FUNCTION__, __LINE__);
        return -1;
    }
    if (DispInfo.DeviceInfo.Enable == 0) {
        AmbaPrint("%s:%u LCD not enabled", __FUNCTION__, __LINE__);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutWidth == 0) {
        AmbaPrint("%s:%u Illegal Vout width %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout height %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }
    if (AmpResource_GetVout0SeamlessBufferPitchAlign(&LcdPitchAlign) != 0) {
        AmbaPrint("%s: Failed to get LCD buffer pitch align", __FUNCTION__);
        return -1; // Error
    }
    LcdVoutPitch = ALIGN_TO(DispInfo.DeviceInfo.VoutWidth, LcdPitchAlign);
    LcdVoutWidth = DispInfo.DeviceInfo.VoutWidth;
    LcdVoutHeight = DispInfo.DeviceInfo.VoutHeight;
    VoutSize = LcdVoutPitch * LcdVoutHeight;

    // Allocate memory for LCD buffer array
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &LcdVoutLumaAddr[0], (void**) &LcdVoutAddrOri, LcdVoutBufferNumber * VoutSize * 2, 1 << 6) != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        return -1; // Error
    }
    LcdVoutChromaAddr[0] = LcdVoutLumaAddr[0] + VoutSize;
    AmbaPrint("%s: Allocate LCD Vout buffer. Address = 0x%08X, Size = 0x%08X", __FUNCTION__, LcdVoutAddrOri, LcdVoutBufferNumber * VoutSize * 2);

    // Get address for each buffer
    // ID = 0 is done. Start from ID = 1.
    for (T = 1; T < LcdVoutBufferNumber; T++) {
        // LCD buffer
        LcdVoutLumaAddr[T] = LcdVoutChromaAddr[T - 1] + VoutSize;
        LcdVoutChromaAddr[T] = LcdVoutLumaAddr[T] + VoutSize;
    }

    // Set buffer to black color
    for (T = 0; T < LcdVoutBufferNumber; T++) {
        // Set buffers to black
        memset (LcdVoutLumaAddr[T], 0, VoutSize);
        memset (LcdVoutChromaAddr[T], 128, VoutSize);
        // Since the buffers are cached, clean the cache so that DSP can get currect data.
        AmbaCache_Clean(LcdVoutLumaAddr[T], VoutSize);
        AmbaCache_Clean(LcdVoutChromaAddr[T], VoutSize);
        AmbaPrint("%s: LCD[%d]: Luma = 0x%08X , Chroma = 0x%08X", __FUNCTION__, T, LcdVoutLumaAddr[T], LcdVoutChromaAddr[T]);
    }

    LcdVoutIdx = VOUT_INVALID_ID;
    LcdVoutIsAlloc = 1;

    return 0;
}

/**
 * Allocate TV Vout buffer
 *
 * @return 0: OK, -1: NG
 */
static int AmpUT_StlDec_AllocVoutBuffer_TV(void)
{
    int T = 0;
    AMP_DISP_INFO_s DispInfo = {0};
    UINT32 VoutSize = TV_VOUT_BUF_MAX_PITCH * TV_VOUT_BUF_MAX_HEIGHT; // Allocate max size so that we can runtime change TV config.
    UINT32 TvPitchAlign = TV_VOUT_BUF_PITCH_ALIGN; // Alignment of TV buffer pitch. TODO: No hardcode
    int Er = 0;

    if (TvVoutIsAlloc) {
        AmbaPrint("%s: Already allocated", __FUNCTION__);
        return 0; // Success
    }
    if (VoutSize == 0) {
        AmbaPrint("%s:%u Illegal Vout buffer size %u", __FUNCTION__, __LINE__, VoutSize);
        return -1; // Error
    }

    if (TvVoutBufferNumber > TV_VOUT_BUF_MAX_NUM || TvVoutBufferNumber < TV_VOUT_BUF_MIN_NUM) {
        AmbaPrint("%s:%u Illegal buffer number %u", __FUNCTION__, __LINE__, TvVoutBufferNumber);
        return -1; // Error
    }

    // Get display info
    Er = AmpUT_Display_GetInfo(1, &DispInfo);
    if (Er != 0) {
        AmbaPrint("%s:%u AmpUT_Display_GetInfo failed", __FUNCTION__, __LINE__);
        return -1;
    }
    if (DispInfo.DeviceInfo.Enable == 0) {
        AmbaPrint("%s:%u TV not enabled", __FUNCTION__, __LINE__);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutWidth == 0) {
        AmbaPrint("%s:%u Illegal Vout width %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout height %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }
    TvVoutPitch = ALIGN_TO(DispInfo.DeviceInfo.VoutWidth, TvPitchAlign);
    TvVoutWidth = DispInfo.DeviceInfo.VoutWidth;
    TvVoutHeight = DispInfo.DeviceInfo.VoutHeight;
    // Don't calculate VoutSize by TvVoutPitch and TvVoutHeight. We need max size.

    // Allocate memory for TV buffer array
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &TvVoutLumaAddr[0], (void**) &TvVoutAddrOri, TvVoutBufferNumber * VoutSize * 2, 1 << 6) != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        return -1; // Error
    }
    TvVoutChromaAddr[0] = TvVoutLumaAddr[0] + VoutSize;
    AmbaPrint("%s: Allocate TV Vout buffer. Address = 0x%08X, Size = 0x%08X", __FUNCTION__, TvVoutAddrOri, TvVoutBufferNumber * VoutSize * 2);

    // Get address for each buffer
    // ID = 0 is done. Start from ID = 1.
    for (T = 1; T < TvVoutBufferNumber; T++) {
        // TV buffer
        TvVoutLumaAddr[T] = TvVoutChromaAddr[T - 1] + VoutSize;
        TvVoutChromaAddr[T] = TvVoutLumaAddr[T] + VoutSize;
    }

    // Set buffer to black color
    for (T = 0; T < TvVoutBufferNumber; T++) {
        // Set buffers to black
        memset (TvVoutLumaAddr[T], 0, VoutSize);
        memset (TvVoutChromaAddr[T], 128, VoutSize);
        // Since the buffers are cached, clean the cache so that DSP can get currect data.
        AmbaCache_Clean(TvVoutLumaAddr[T], VoutSize);
        AmbaCache_Clean(TvVoutChromaAddr[T], VoutSize);
        AmbaPrint("%s: TV[%d]: Luma = 0x%08X , Chroma = 0x%08X", __FUNCTION__, T, TvVoutLumaAddr[T], TvVoutChromaAddr[T]);
    }

    TvVoutIdx = VOUT_INVALID_ID;
    TvVoutIsAlloc = 1;

    return 0;
}

/**
 * Get luma and chroma addresses of the next LCD Vout buffer.\n
 * The buffer will be cleaned.\n
 * The buffer index will move ahead
 *
 * @param lumaAddr [out]    luma addresse
 * @param chromaAddr [out]  chroma addresse
 *
 * @return 0: OK
 */
static int AmpUT_StlDec_GetNextVoutBuffer_LCD(UINT8 **lumaAddr, UINT8 **chromaAddr)
{
    // Allocate buffer if needed
    if (LcdVoutIsAlloc == 0 && LcdSharedBufferIsValid == 0) {
        if (AmpUT_StlDec_AllocVoutBuffer_LCD() != AMP_OK) {
            AmbaPrint("%s: AmpUT_StlDec_AllocVoutBuffer_LCD failed", __FUNCTION__);
            return -1;
        }
    }

    // Get next ID
    LcdVoutIdx++;

    // There're 2 cases that LcdVoutIdx changes to 0.
    // Case 1: Original LcdVoutIdx is invalid ID (255). That means no picture has been displayed before. Start from ID = 0.
    //         In this case, new ID = (255 + 1) % Number = 0.
    // Case 2: Original LcdVoutIdx is the last buffer. Should wrap around. Next ID = 0.
    LcdVoutIdx = LcdVoutIdx % LcdVoutBufferNumber;

    // Set buffers to black
    memset (LcdVoutLumaAddr[LcdVoutIdx], 0, LcdVoutPitch*LcdVoutHeight);
    memset (LcdVoutChromaAddr[LcdVoutIdx], 128, LcdVoutPitch*LcdVoutHeight);
    // Since the buffers are cached, clean the cache so that DSP can get currect data.
    AmbaCache_Clean(LcdVoutLumaAddr[LcdVoutIdx], LcdVoutPitch*LcdVoutHeight);
    AmbaCache_Clean(LcdVoutChromaAddr[LcdVoutIdx], LcdVoutPitch*LcdVoutHeight);

    *lumaAddr = LcdVoutLumaAddr[LcdVoutIdx];
    *chromaAddr = LcdVoutChromaAddr[LcdVoutIdx];

    return 0;
}

/**
 * Get luma and chroma addresses of the next TV Vout buffer.\n
 * The buffer will be cleaned.\n
 * The buffer index will move ahead
 *
 * @param lumaAddr [out]    luma addresse
 * @param chromaAddr [out]  chroma addresse
 *
 * @return 0: OK
 */
static int AmpUT_StlDec_GetNextVoutBuffer_TV(UINT8 **lumaAddr, UINT8 **chromaAddr)
{
    UINT32 VoutSize = TV_VOUT_BUF_MAX_PITCH * TV_VOUT_BUF_MAX_HEIGHT; // Make it consistent to the size in AmpUT_StlDec_AllocVoutBuffer_TV

    // Allocate buffer if needed
    if (TvVoutIsAlloc == 0) {
        if (AmpUT_StlDec_AllocVoutBuffer_TV() != AMP_OK) {
            AmbaPrint("%s: AmpUT_StlDec_AllocVoutBuffer_TV failed", __FUNCTION__);
            return -1;
        }
    }

    // Get next ID
    TvVoutIdx++;
    TvVoutIdx = TvVoutIdx % TvVoutBufferNumber;

    // Set buffers to black
    memset (TvVoutLumaAddr[TvVoutIdx], 0, VoutSize);
    memset (TvVoutChromaAddr[TvVoutIdx], 128, VoutSize);
    // Since the buffers are cached, clean the cache so that DSP can get currect data.
    AmbaCache_Clean(TvVoutLumaAddr[TvVoutIdx], VoutSize);
    AmbaCache_Clean(TvVoutChromaAddr[TvVoutIdx], VoutSize);

    *lumaAddr = TvVoutLumaAddr[TvVoutIdx];
    *chromaAddr = TvVoutChromaAddr[TvVoutIdx];

    return 0;
}

/**
 * Get luma and chroma addresses of the current LCD Vout buffer.\n
 * The data in buffer will not be changed.
 *
 * @param lumaAddr [out]    luma addresse
 * @param chromaAddr [out]  chroma addresse
 *
 * @return 0: OK
 */
#if 0
static int AmpUT_StlDec_GetCurVoutBuffer_LCD(UINT8 **lumaAddr, UINT8 **chromaAddr)
{
    *lumaAddr = LcdVoutLumaAddr[LcdVoutIdx];
    *chromaAddr = LcdVoutChromaAddr[LcdVoutIdx];

    return 0;
}
#endif

/**
 * Get luma and chroma addresses of the current TV Vout buffer.\n
 * The data in buffer will not be changed.
 *
 * @param lumaAddr [out]    luma addresse
 * @param chromaAddr [out]  chroma addresse
 *
 * @return 0: OK
 */
#if 0
static int AmpUT_StlDec_GetCurVoutBuffer_TV(UINT8 **lumaAddr, UINT8 **chromaAddr)
{
    *lumaAddr = TvVoutLumaAddr[TvVoutIdx];
    *chromaAddr = TvVoutChromaAddr[TvVoutIdx];

    return 0;
}
#endif

/**
 * fifo callback function for get info from fifo.
 *
 * @param hdlr[in] : co-related coded
 * @param event[in] : callback event
 * @param info[in] : callback info
 * @return 0: OK
 */
int AmpUT_StlDec_FifoCB(void *hdlr, UINT32 event, void* info)
{
    return 0;
}

/**
 * Start still decoder.
 *
 * @return 0: OK
 */
static int AmpUT_StlDec_StillDecodeStart(void)
{
    // Start decoder
    if (StillDecodeStartFlag == 0) {
        if (AmpDec_Start(DecPipeHdlr, NULL, NULL) != AMP_OK) {
            AmbaPrint("%s:%u Failed to start the decoder.", __FUNCTION__, __LINE__);
            return -1;
        }
        StillDecodeStartFlag = 1;
        // Wait StillDecodeStart done
        AmbaKAL_TaskSleep(100); // TODO: No hardcode
    }
    return 0;
}

/**
 * codec callback function for get info from codec.
 *
 * @param hdlr[in] : co-related coded
 * @param event[in] : callback event
 * @param info[in] : callback info
 * @return 0: OK
 */
int AmpUT_StlDec_CodecCB(void *hdlr, UINT32 event, void* info)
{
    //AmbaPrint("%s on Event: 0x%x ", __FUNCTION__, event); // Mark this line because it will be printed constantly after uCode update on 2015/03/04
    switch (event) {
    case AMP_DEC_EVENT_JPEG_DEC_COMMON_BUFFER_REPORT:
        {
            AMP_DEC_EVENT_JPEG_DEC_COMMON_BUFFER_REPORT_s* EventData = info;
            UINT32 T = 0;
            UINT32 LcdPitchAlign = 1024; // Alignment of LCD buffer pitch.
            AMP_DISP_INFO_s DispInfo = {0};
            static char *  PrevAddress = (char *) 0xDEADBEEF; // The last Address from common buffer message
            static UINT32  PrevBufferNumber = 0;              // The last BufferNumber from common buffer message
            static UINT32  PrevBufferSize = 0;                // The last BufferSize from common buffer message
            if (info == NULL) {
                AmbaPrintColor(RED, "%s: info is NULL", __FUNCTION__);
                break;
            }
            // Update current index
            if (UpdateLcdVoutIdx == 1) {
                if (LcdVoutIdx != EventData->CurrentId) {
                    AmbaPrint("%s: Change Vout buffer ID from %u to %u", __FUNCTION__, LcdVoutIdx, EventData->CurrentId);
                    LcdVoutIdx = EventData->CurrentId; // Invalid ID will be handled in AmpUT_StlDec_ShowBufOnLCD
                }
                UpdateLcdVoutIdx = 0;
            }
            // Skip the remained process if the buffer is not changed
            if ((EventData->Address == PrevAddress) &&
                (EventData->BufferNumber == PrevBufferNumber) &&
                (EventData->BufferSize == PrevBufferSize) ) {
                PrevAddress = EventData->Address;
                PrevBufferNumber = EventData->BufferNumber;
                PrevBufferSize = EventData->BufferSize;
                break;
            }
            PrevAddress = EventData->Address;
            PrevBufferNumber = EventData->BufferNumber;
            PrevBufferSize = EventData->BufferSize;
            if (EventData->BufferNumber > LCD_VOUT_BUF_MAX_NUM) {
                AmbaPrintColor(RED, "%s: Illegal buffer number (%u)", __FUNCTION__, EventData->BufferNumber);
                break;
            }

            // Get LCD info
            if (AmpUT_Display_GetInfo(0, &DispInfo) != 0) {
                AmbaPrintColor(RED, "%s AmpUT_Display_GetInfo failed", __FUNCTION__);
                break;
            }
            if (DispInfo.DeviceInfo.Enable == 0) {
                AmbaPrintColor(RED, "%s LCD not enabled", __FUNCTION__);
                break;
            }
            if (DispInfo.DeviceInfo.VoutWidth == 0) {
                AmbaPrintColor(RED, "%s Illegal Vout width %u", __FUNCTION__, DispInfo.DeviceInfo.VoutWidth);
                break;
            }
            if (DispInfo.DeviceInfo.VoutHeight == 0) {
                AmbaPrintColor(RED, "%s Illegal Vout height %u", __FUNCTION__, DispInfo.DeviceInfo.VoutHeight);
                break;
            }
            if (AmpResource_GetVout0SeamlessBufferPitchAlign(&LcdPitchAlign) != 0) {
                AmbaPrintColor(RED, "%s Failed to get LCD buffer pitch align", __FUNCTION__);
                break;
            }
            LcdVoutPitch = ALIGN_TO(DispInfo.DeviceInfo.VoutWidth, LcdPitchAlign);
            LcdVoutWidth = DispInfo.DeviceInfo.VoutWidth;
            LcdVoutHeight = DispInfo.DeviceInfo.VoutHeight;

            // Check buffer size
            {
                UINT64 BufferSize = LcdVoutPitch * LcdVoutHeight * 2;
                if ((UINT64) EventData->BufferSize <  BufferSize) {
                    AmbaPrintColor(RED, "%s: Buffer size is too small (%u. Should larger than %lld)", __FUNCTION__, EventData->BufferSize, BufferSize);
                }
            }

            LcdVoutBufferNumber = EventData->BufferNumber;
            // Release LCD buffer if it's been allocated.
            if (AmpUT_StlDec_ReleaseVoutBuffer_LCD() != 0) {
                AmbaPrintColor(RED, "%s Failed to release LCD buffer", __FUNCTION__);
                break;
            }
            // Assign Vout buffer
            AmbaPrint("%s: Use shared LCD Vout buffer. Address = 0x%08X, BufferNumber = %u, Size = 0x%08X, CurrentID = %u",
                    __FUNCTION__, EventData->Address, EventData->BufferNumber, EventData->BufferNumber * EventData->BufferSize, EventData->CurrentId);
            for (T = 0; T < EventData->BufferNumber; ++T) {
                LcdVoutLumaAddr[T] = (UINT8 *) EventData->Address + (T * EventData->BufferSize);
                LcdVoutChromaAddr[T] = LcdVoutLumaAddr[T] + ((UINT32) EventData->BufferSize / 2);
                AmbaPrint("%s: LCD[%d]: Luma = 0x%08X , Chroma = 0x%08X", __FUNCTION__, T, LcdVoutLumaAddr[T], LcdVoutChromaAddr[T]);
            }
            LcdSharedBufferIsValid = 1;
        }
        break;
    default:
        break;
    }
    return 0;
}

/**
 * To config vout device and create windows for decode output
 *
 * @return 0: OK
 */
int AmpUT_StlDec_ConfigDisplay(void)
{
    AMP_DISP_WINDOW_CFG_s Window;
    int Er = NG;

    if (AmpUT_Display_Init() == NG) {
        return Er;
    }

    memset(&Window, 0, sizeof(Window));

    // Creat LCD Window
    Window.Source = AMP_DISP_DEC;
    Window.CropArea.Width = 0;
    Window.CropArea.Height = 0;
    Window.CropArea.X = 0;
    Window.CropArea.Y = 0;
    Window.TargetAreaOnPlane.Width = 960;
    Window.TargetAreaOnPlane.Height = 480;
    Window.TargetAreaOnPlane.X = 0;
    Window.TargetAreaOnPlane.Y = 0;
    if (AmpUT_Display_Window_Create(0, &Window) == NG) {
        return Er;
    }

    // Creat TV Window
    Window.Source = AMP_DISP_DEC;
    Window.CropArea.Width = 0;
    Window.CropArea.Height = 0;
    Window.CropArea.X = 0;
    Window.CropArea.Y = 0;
    Window.TargetAreaOnPlane.Width = 1920;
    Window.TargetAreaOnPlane.Height = 1080;
    Window.TargetAreaOnPlane.X = 0;
    Window.TargetAreaOnPlane.Y = 0;
    if (AmpUT_Display_Window_Create(1, &Window) == NG) {
        return Er;
    }

    return 0;
}

/**
 * Enable selected channel
 *
 * @param argv[2] - channel
 * @return 0: OK
 */
int AmpUT_StlDec_Ch(char** argv)
{
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    if (atoi(argv[2]) == 1) {
        AmpUT_StlDec_AllocVoutBuffer_TV();
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
        AmpUT_StlDec_AllocVoutBuffer_LCD();
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
 * Start still decode codec
 *
 * @return 0: OK
 */
int AmpUT_StlDec_Start(void)
{
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    if (StillDecodeStartFlag) {
        AmbaPrint("%s:%u Already started.", __FUNCTION__, __LINE__);
        goto ReturnSuccess;
    }

    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        ErrMsg = "AmpUT_StlDec_StillDecodeStart failed";
        goto ReturnError;
    }

ReturnSuccess:
    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * stop still decode codec
 *
 * @return 0: OK
 */
int AmpUT_StlDec_Stop(void)
{
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    // Clear flag so that AmpDec_Start will be called.
    StillDecodeStartFlag = 0;

    // Deinit still decoder manager
    if (DecPipeHdlr != NULL) {
        if (AmpDec_Remove(DecPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to remove the still decoder manager.", __FUNCTION__, __LINE__);
            Er = -1;
        }
        if (AmpDec_Delete(DecPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to delete the still decoder manager.", __FUNCTION__, __LINE__);
            Er = -1;
        }
        DecPipeHdlr = NULL;
    }

    // Deinit fifo handler
    if (StlDecFifoHdlr != NULL) {
        if (AmpFifo_Delete(StlDecFifoHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to delete fifo.", __FUNCTION__, __LINE__);
            Er = -1;
        }
        StlDecFifoHdlr = NULL;
    }

    // Deinit still decoder
    if (StlDecHdlr != NULL) {
        if (AmpStillDec_Delete(StlDecHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to deinit the still decoder.", __FUNCTION__, __LINE__);
            Er = -1;
        }
        StlDecHdlr = NULL;
    }

    // Deinit LCD and TV windows
    {
        // Delete LCD Window
        if (AmpUT_Display_Window_Delete(0) != AMP_OK) {
            AmbaPrint("%s:%u Failed to delete LCD window handler.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpUT_Display_Window_Delete failed";
            goto ReturnError;
        }

        // Delete TV Window
        if (AmpUT_Display_Window_Delete(1) != AMP_OK) {
            AmbaPrint("%s:%u Failed to delete TV window handler.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpUT_Display_Window_Delete failed";
            goto ReturnError;
        }
    }

    // Release raw buffer
    if (StlRawBufOri != NULL) {
        if (AmbaKAL_BytePoolFree(StlRawBufOri) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the raw buffer.", __FUNCTION__, __LINE__);
            Er = -1;
        }
        StlRawBuf = NULL;
        StlRawBufOri = NULL;
    }

    // Release Vout buffer
    if (AmpUT_StlDec_ReleaseVoutBuffer_LCD() != AMP_OK) {
        AmbaPrint("%s:%u Failed to release the LCD Vout buffer.", __FUNCTION__, __LINE__);
        Er = -1;
    }
    if (AmpUT_StlDec_ReleaseVoutBuffer_TV() != AMP_OK) {
        AmbaPrint("%s:%u Failed to release the TV Vout buffer.", __FUNCTION__, __LINE__);
        Er = -1;
    }

    // Release cache buffer
    if (DecCacheOri != NULL) {
        if (AmbaKAL_BytePoolFree(DecCacheOri) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the cache buffer.", __FUNCTION__, __LINE__);
            Er = -1;
        }
        DecCache = NULL;
        DecCacheOri = NULL;
    }

    // Release main buffer
#ifdef STLDEC_BORROW_DSP
    if (DecMainBufOri != NULL) {
        // Don't have to release memory
        DecMainBuf = NULL;
        DecMainBufOri = NULL;
    }
#else
    if (DecMainBufOri != NULL) {
        if (AmbaKAL_BytePoolFree(DecMainBufOri) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the cache buffer.", __FUNCTION__, __LINE__);
        }
        DecMainBuf = NULL;
        DecMainBufOri = NULL;
    }
#endif

    UpdateLcdVoutIdx = 1;

    if (Er != 0) {
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Init stilldecode unit test.\n
 * In the function we config display, create codec and create fifo.
 *
 * @return 0: OK
 */
int AmpUT_StlDec_Init(UINT32 disableVoutAtInit)
{
    AMP_STILLDEC_CFG_s CodecCfg;
    AMP_STILLDEC_INIT_CFG_s CodecInitCfg;
    AMP_DEC_PIPE_CFG_s PipeCfg;
    AMP_FIFO_CFG_s FifoDefCfg;

    int Er;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    {
        extern int AmbaLoadDSPuCode(void);
        Er = AmbaLoadDSPuCode(); /* Load uCodes from NAND */
        if (Er != 0) {
            ErrMsg = "AmbaLoadDSPuCode failed";
            goto ReturnError;
        }
    }

    // Clear flag so that AmpDec_Start will be called.
    StillDecodeStartFlag = 0;

    /** Create LCD handler, TV handler, LCD window, TV window */
    if (AmpUT_StlDec_ConfigDisplay() != AMP_OK) {
        AmbaPrint("%s:%u Failed to configure display handler.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpUT_StlDec_configDisplay failed";
        goto ReturnError;
    }

    /** Initialize codec module */
    // Get the default codec module settings
    AmpStillDec_GetInitDefaultCfg(&CodecInitCfg);
    //Customize the module settings
        // You can add some codes here...
    // Allocate memory for codec module
    if (StlCodecBufOri == NULL){
    Er = AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(CodecInitCfg.Buf), &StlCodecBufOri, CodecInitCfg.BufSize, 1 << 5);
        AmbaPrint("%s: malloc :%d", __FUNCTION__, Er);
        if (Er != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            AmpUT_StlDec_Stop();
            ErrMsg = "AmpUtil_GetAlignedPool failed";
            goto ReturnError;
        }
        // Configure the initial settings
        AmpStillDec_Init(&CodecInitCfg);
    }

    /** Create codec handler */
    // Get the default codec handler settings
    AmpStillDec_GetDefaultCfg(&CodecCfg);
    // Allocate memory for codec raw buffer
    Er = AmpUtil_GetAlignedPool(&G_MMPL, &StlRawBuf, &StlRawBufOri, STLDEC_RAW_SIZE, 1 << 6);
    CodecCfg.RawBuf = (UINT8*) StlRawBuf;
    AmbaPrint("%s: malloc :%d", __FUNCTION__, Er);
    if (Er != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        AmpUT_StlDec_Stop();
        ErrMsg = "AmpUtil_GetAlignedPool failed";
        goto ReturnError;
    }
    // Customize the handler settings
    AmbaPrint("%x -> %x", StlRawBufOri, CodecCfg.RawBuf);
    CodecCfg.RawBufSize = STLDEC_RAW_SIZE;
    CodecCfg.CbCodecEvent = AmpUT_StlDec_CodecCB;
#ifdef STLDEC_BORROW_DSP
    // Assign DSP working memory address.
    CodecCfg.DspWorkBufAddr = DspWorkAreaResvStart;
    CodecCfg.DspWorkBufSize = DspWorkAreaResvSize - (STLDEC_MAIN_SIZE + 64);
#else
    // Assign DSP working memory address.
    CodecCfg.DspWorkBufAddr = DspWorkAreaResvStart;   // Set to original address
    CodecCfg.DspWorkBufSize = DspWorkAreaResvSize;    // Set to original size in case that other modules have changed it.
#endif
    // Create a codec handler, and configure the initial settings
    Er = AmpStillDec_Create(&CodecCfg, &StlDecHdlr);
    if (Er != 0) {
        ErrMsg = "AmpStillDec_Create failed";
        goto ReturnError;
    }

    /** Create decoder manager */
    // Get the default decoder manager settings
    AmpDec_GetDefaultCfg(&PipeCfg);
    // Customize the manager settings
    PipeCfg.Decoder[0] = StlDecHdlr;
    PipeCfg.NumDecoder = 1;
    PipeCfg.Type = AMP_DEC_STL_PIPE;
    // Create a decoder manager, and insert the codec handler into the  manager
    Er = AmpDec_Create(&PipeCfg, &DecPipeHdlr);
    if (Er != 0) {
        ErrMsg = "AmpDec_Create failed";
        goto ReturnError;
    }

    /** Activate decoder manager */
    // Activate the decoder manager and all the codec handlers in the manager
    Er = AmpDec_Add(DecPipeHdlr);
    if (Er != 0) {
        ErrMsg = "AmpDec_Add failed";
        goto ReturnError;
    }

    /** Create fifo */
    // Get the default fifo settings
    AmpFifo_GetDefaultCfg(&FifoDefCfg);
    // Customize the fifo settings
    FifoDefCfg.hCodec = StlDecHdlr;
    FifoDefCfg.IsVirtual = 1;
    FifoDefCfg.NumEntries = 64;
    FifoDefCfg.cbEvent = AmpUT_StlDec_FifoCB;
    // Create a fifo handler, and configure the initial settings
    Er = AmpFifo_Create(&FifoDefCfg, &StlDecFifoHdlr);
    if (Er != 0) {
        ErrMsg = "AmpFifo_Create failed";
        goto ReturnError;
    }

    /** Enable vout */
    if ((disableVoutAtInit&1) == 0) {
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
        Er = AmpUT_Display_Update(0);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Update failed";
            goto ReturnError;
        }

        /** Allocate vout buffer */
        // Allocate when Vout width and height are available (after AmpUT_Display_Start()).
        if (AmpUT_StlDec_AllocVoutBuffer_LCD() != AMP_OK) {
            ErrMsg = "AmpUT_StlDec_AllocVoutBuffer_LCD failed";
            goto ReturnError;
        }
    }
    if ((disableVoutAtInit&2) == 0) {
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
        Er = AmpUT_Display_Update(1);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Update failed";
            goto ReturnError;
        }

        /** Allocate vout buffer */
        // Allocate when Vout width and height are available (after AmpUT_Display_Start()).
        if (AmpUT_StlDec_AllocVoutBuffer_TV() != AMP_OK) {
            ErrMsg = "AmpUT_StlDec_AllocVoutBuffer_TV failed";
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
 * Feed frame to fifo
 *
 * @param argc
 * @param argv - argv[2]: file name argv[3]: file type
 * @return
 */
int AmpUT_StlDec_Feed(int argc, char **argv)
{
    AMP_BITS_DESC_s Desc;
    AMP_BITS_DESC_s DescToWrite;
    AMP_CFS_FILE_s *Fp;
    /** Configure file access mode */
    char mode[3] = "rb";
    char* fn = argv[2];
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    // Since the raw buffer of still decoder is not circular, erase FIFO in order to reset read/write pointer of the buffer.
    Er = AmpFifo_EraseAll(StlDecFifoHdlr);
    if (Er != 0) {
        ErrMsg = "AmpFifo_EraseAll failed";
        goto ReturnError;
    }

    /** Prepare entry */
    // Prepare space in raw buffer to write undecoded data, and get an entry (a descriptor) to the space
    Er = AmpFifo_PrepareEntry(StlDecFifoHdlr, &Desc);
    if (Er != 0) {
        ErrMsg = "AmpFifo_PrepareEntry failed";
        goto ReturnError;
    }

    /** Read out frame */
    AmbaPrint("open %s", fn);

    // Open target file
    Fp = AmpDecCFS_fopen(fn, mode);
    if (Fp == NULL) {
        AmbaPrint("Feed open failed");
        ErrMsg = "AmpDecCFS_fopen failed";
        goto ReturnError;
    }
    // Configure descriptor settings
    DescToWrite.Align = 0;
    DescToWrite.StartAddr = Desc.StartAddr;
    if (atoi(argv[3]) == 0) {
        AmbaPrint("Feed image");
        Er = AmpCFS_fseek(Fp, 0, AMBA_FS_SEEK_END); // Move to the end of file to get file size
        if (Er != 0) {
            ErrMsg = "AmpCFS_fseek failed";
            goto ReturnError;
        }
        DescToWrite.Size = AmpCFS_ftell(Fp);
        DescToWrite.Type = AMP_FIFO_TYPE_JPEG_FRAME;
        AmbaPrint("Feed file %s size: %d", fn, DescToWrite.Size);
        Er = AmpCFS_fseek(Fp, 0, AMBA_FS_SEEK_START);
        if (Er != 0) {
            ErrMsg = "AmpCFS_fseek failed";
            goto ReturnError;
        }
        // Read target file into raw buffer
        AmpCFS_fread(DescToWrite.StartAddr, DescToWrite.Size, 1, Fp);
        // Since the buffer is cached, clean the cache after writing data so that DSP can get the currect data.
        AmbaCache_Clean(DescToWrite.StartAddr, DescToWrite.Size);
        // Close file
        Er = AmpCFS_fclose(Fp);
        if (Er != 0) {
            ErrMsg = "AmpCFS_fclose failed";
            goto ReturnError;
        }
        // Insert the descriptor into fifo
        Er = AmpFifo_WriteEntry(StlDecFifoHdlr, &DescToWrite);
        if (Er != 0) {
            ErrMsg = "AmpFifo_WriteEntry failed";
            goto ReturnError;
        }
    }else{
        AmbaPrint("Feed video");
        Er = AmpCFS_fseek(Fp, 0, AMBA_FS_SEEK_END); // Move to the end of file to get file size
        if (Er != 0) {
            ErrMsg = "AmpCFS_fseek failed";
            goto ReturnError;
        }
        DescToWrite.Size = AmpCFS_ftell(Fp);
        DescToWrite.Type = AMP_FIFO_TYPE_IDR_FRAME;
        AmbaPrint("Feed file %s size: %d", fn, DescToWrite.Size);
        Er = AmpCFS_fseek(Fp, 0, AMBA_FS_SEEK_START);
        if (Er != 0) {
            ErrMsg = "AmpCFS_fseek failed";
            goto ReturnError;
        }
        // Read target file into raw buffer
        AmpCFS_fread(DescToWrite.StartAddr, DescToWrite.Size, 1, Fp);
        // Since the buffer is cached, clean the cache after writing data so that DSP can get the currect data.
        AmbaCache_Clean(DescToWrite.StartAddr, DescToWrite.Size);
        // Close file
        Er = AmpCFS_fclose(Fp);
        if (Er != 0) {
            ErrMsg = "AmpCFS_fclose failed";
            goto ReturnError;
        }
        // Insert the descriptor into fifo
        Er = AmpFifo_WriteEntry(StlDecFifoHdlr, &DescToWrite);
        if (Er != 0) {
            ErrMsg = "AmpFifo_WriteEntry failed";
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
 * Decode frame to certain buffer
 *
 * @param argc
 * @param argv [in] - argv[2]: input width, argv[3]: input height argv[4]: target image cache index
 * @return 0: OK
 */
int AmpUT_StlDec_Dec(int argc, char **argv)
{
    static UINT8 DecTarCur = 0;     // Start cache id to store decoded data. Files will be stored seperately in different caches.

    int T;                          // Counter
    int Er;                         // Function call return
    char *ErrMsg = "";
    int InputValue;

    int DecNum;                     // How many files to decode
    AMP_STILLDEC_DECODE_s Decode;   // Decode settings
    UINT32 Pitch;                   // The distance, in bytes, between two beginning memory addresses of adjacent bitmap lines
    UINT32 State[16];
    AMP_AREA_s Crop[16];            // Cropping settings for each file
    AMP_YUV_BUFFER_s Tar[16];       // Decoded image settings for each file
    AMP_ROTATION_e Rotate[16] = {AMP_ROTATE_0}; // Rotate settings for each file

    AmbaPrint("%s", __FUNCTION__);

    InputValue = atoi(argv[4]);  // Index of the cache that stores the rescaled data from main buffer
    if (InputValue < 0 || InputValue >= CACHE_NUM) {
        AmbaPrint("%s Cache ID (%u) out of range (0 ~ %u).", __FUNCTION__, InputValue, CACHE_NUM - 1);
        ErrMsg = "Illegal cache ID";
        goto ReturnError;
    }
    DecTarCur = (UINT8) InputValue % CACHE_NUM; // Prevent cache from overflow
    DecNum = 1;
    AmbaPrint("decode %d files (crop 160*120 area)", DecNum);

    // Start decoder
    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        ErrMsg = "AmpUT_StlDec_StillDecodeStart failed";
        goto ReturnError;
    }

    /** Allocate memory for cache buffer */
    if (DecCacheOri == NULL) {
        // Cache buffer
        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void**) &DecCache, (void**) &DecCacheOri, (CACHE_SIZE*2*CACHE_NUM), 1 << 6);
        if (Er != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory (%u).", __FUNCTION__, __LINE__, Er);
            ErrMsg = "AmpUtil_GetAlignedPool failed";
            goto ReturnError;
        }
    }

    /** Allocate memory for main buffer */
#ifdef STLDEC_BORROW_DSP
    DecMainBufOri = DspWorkAreaResvStart + DspWorkAreaResvSize - (STLDEC_MAIN_SIZE + 64);
    DecMainBuf = (void *) ALIGN_64((UINT32)DecMainBufOri);
#else
    if (DecMainBufOri == NULL) {
        // Main buffer
        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void**) &DecMainBuf, (void**) &DecMainBufOri, STLDEC_MAIN_SIZE, 1 << 6);
        if (Er != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory (%u).", __FUNCTION__, __LINE__, Er);
            ErrMsg = "AmpUtil_GetAlignedPool failed";
            goto ReturnError;
        }
    }
#endif

    /** Configure decoder settings */
    Decode.NumFile = DecNum;
    Decode.DecodeBuf = DecMainBuf;
    Decode.SizeDecodeBuf = STLDEC_MAIN_SIZE;
    Decode.DecodedImgPitch = &Pitch;
    Decode.DecodeState = State;
    Decode.CropFromDecodedBuf = Crop;
    Decode.RescaleDest = Tar;
    Decode.Rotate = Rotate;
    for (T=0 ; T<DecNum ; T++) {
        Decode.CropFromDecodedBuf[T].X = 0;
        Decode.CropFromDecodedBuf[T].Y = 0;
        Decode.CropFromDecodedBuf[T].Width = atoi(argv[2]);;
        Decode.CropFromDecodedBuf[T].Height = atoi(argv[3]);;

        Decode.RescaleDest[T].LumaAddr = DecCache + (DecTarCur*CACHE_SIZE*2);
        Decode.RescaleDest[T].ChromaAddr = DecCache + (DecTarCur*CACHE_SIZE*2)+CACHE_SIZE;
        Decode.RescaleDest[T].ColorFmt = AMP_YUV_422;
        Decode.RescaleDest[T].Width = CACHE_WIDTH;
        Decode.RescaleDest[T].Height = CACHE_HEIGHT;
        Decode.RescaleDest[T].Pitch = CACHE_PITCH;
        Decode.RescaleDest[T].AOI.X = 0;
        Decode.RescaleDest[T].AOI.Y = 0;
        Decode.RescaleDest[T].AOI.Width = CACHE_WIDTH;
        Decode.RescaleDest[T].AOI.Height = CACHE_HEIGHT;

        AmbaPrint("img(%d) luma:0x%08X chroma:0x%08X", DecTarCur, Decode.RescaleDest[T].LumaAddr,
                  Decode.RescaleDest[T].ChromaAddr);

        DecTarCur = (DecTarCur+1)%CACHE_NUM;
    }

    /** Start decoding */
    // Decode the JPEG image in raw buffer, and store the YUV data in YUV buffer.
    // After that, rescale the YUV data, and store it in cache buffer.
    if (AmpStillDec_Decode(StlDecHdlr, &Decode) != AMP_OK) {
        AmbaPrint("%s:%u Failed to decode the image.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpStillDec_Decode failed";
        goto ReturnError;
    }

    // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

    AmbaPrint("%s: dec done  luma:0x%08X chroma:0x%08X pitch:0x%08X format:%u", __FUNCTION__, Decode.OutAddrY, Decode.OutAddrUV, *Decode.DecodedImgPitch, Decode.OutColorFmt);
    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * rescale yuv to yuv
 * @return
 */
int AmpUT_StlDec_Rescale(int argc, char **argv)
{
    AMP_STILLDEC_RESCALE_s Rescale;
    AMP_YUV_BUFFER_s Src;
    AMP_YUV_BUFFER_s Dest;
    AMP_ROTATION_e Rotate = AMP_ROTATE_0;
    UINT8 LumaGain = 128; // 128: original luma
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    // Start decoder
    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        ErrMsg = "AmpUT_StlDec_StillDecodeStart failed";
        goto ReturnError;
    }

    Rescale.NumBuf = 1;
    Rescale.Src = &Src;
    Rescale.Dest = &Dest;
    Rescale.Rotate = &Rotate;
    Rescale.LumaGain = &LumaGain;
    if (argc < 15) {
        AmbaPrint("%s: Not enough parameters", __FUNCTION__);
        goto ReturnError;
    }

    Src.LumaAddr = (UINT8 *) strtoul(argv[2], NULL, 0);
    Src.ChromaAddr = (UINT8 *) strtoul(argv[3], NULL, 0);
    Src.Width = atoi(argv[4]);
    Src.Height = atoi(argv[5]);
    Src.Pitch = Src.Width;
    Src.ColorFmt = atoi(argv[6]);
    Src.AOI.X = 0;
    Src.AOI.Y = 0;
    Src.AOI.Width = Src.Width;
    Src.AOI.Height = Src.Height;

    Dest.LumaAddr = (UINT8 *) strtoul(argv[7], NULL, 0);
    Dest.ChromaAddr = (UINT8 *) strtoul(argv[8], NULL, 0);
    Dest.Width = atoi(argv[9]);
    Dest.Height = atoi(argv[10]);
    Dest.Pitch = Dest.Width;
    Dest.ColorFmt = AMP_YUV_422;
    Dest.AOI.X = atoi(argv[11]);
    Dest.AOI.Y = atoi(argv[12]);
    Dest.AOI.Width = atoi(argv[13]);
    Dest.AOI.Height = atoi(argv[14]);

    Er = AmpStillDec_Rescale(StlDecHdlr, &Rescale);
    if (Er != 0) {
        ErrMsg = "AmpStillDec_Rescale failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * show image in buffer on LCD
 *
 * @param idx
 * @param x
 * @param y
 * @param w
 * @param h
 */
static int AmpUT_StlDec_ShowBufOnLCD(UINT8 *lumaAddr, UINT8 *chromaAddr)
{
    AMP_STILLDEC_DISPLAY_s Display;
    AMP_YUV_BUFFER_s Buf;
    AMP_DISP_INFO_s DispInfo = {0};

    // Start decoder
    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        AmbaPrint("%s:%u AmpUT_StlDec_StillDecodeStart failed", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    AmpUT_Display_GetInfo(0, &DispInfo);
    if (DispInfo.DeviceInfo.VoutWidth == 0 || DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout width(%u) or height(%u).", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.Enable == 0) {
        AmbaPrint("%s:%u LCD not enabled", __FUNCTION__, __LINE__);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutWidth == 0) {
        AmbaPrint("%s:%u Illegal Vout width %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout height %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }

    Display.Vout = AMP_DISP_CHANNEL_DCHAN; // LCD
    Display.Buf = &Buf;
    Buf.ColorFmt = AMP_YUV_422;
    Buf.AOI.X = 0;
    Buf.AOI.Y = 0;
    Buf.AOI.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.AOI.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.Pitch = LcdVoutPitch;
    Buf.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.LumaAddr = lumaAddr;
    Buf.ChromaAddr = chromaAddr;

    // Still decode on LCD is always full screen. No need to update window.

    if (AmpStillDec_Display(StlDecHdlr, &Display) != AMP_OK) {
        AmbaPrint("%s:%u Failed to display the image.", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

    return 0; // Success
}

/**
 * show image in buffer on TV
 *
 * @param idx
 * @param x
 * @param y
 * @param w
 * @param h
 */
static int AmpUT_StlDec_ShowBufOnTV(UINT8 *lumaAddr, UINT8 *chromaAddr)
{
    AMP_STILLDEC_DISPLAY_s Display;
    AMP_YUV_BUFFER_s Buf;
    AMP_DISP_INFO_s DispInfo = {0};
    UINT32 TvPitchAlign = TV_VOUT_BUF_PITCH_ALIGN; // Alignment of TV buffer pitch. TODO: No hardcode

    // Start decoder
    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        AmbaPrint("%s:%u AmpUT_StlDec_StillDecodeStart failed", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    AmpUT_Display_GetInfo(1, &DispInfo);
    if (DispInfo.DeviceInfo.VoutWidth == 0 || DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout width(%u) or height(%u).", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.Enable == 0) {
        AmbaPrint("%s:%u TV not enabled", __FUNCTION__, __LINE__);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutWidth == 0) {
        AmbaPrint("%s:%u Illegal Vout width %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout height %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }

    Display.Vout = AMP_DISP_CHANNEL_FCHAN; // TV
    Display.Buf = &Buf;
    Buf.ColorFmt = AMP_YUV_422;
    Buf.AOI.X = 0;
    Buf.AOI.Y = 0;
    Buf.AOI.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.AOI.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.Pitch = ALIGN_TO(DispInfo.DeviceInfo.VoutWidth, TvPitchAlign);
    Buf.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.LumaAddr = lumaAddr;
    Buf.ChromaAddr = chromaAddr;

    // Still decode on TV is always full screen. No need to update window.

    if (AmpStillDec_Display(StlDecHdlr, &Display) != AMP_OK) {
        AmbaPrint("%s:%u Failed to display the image.", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

    return 0; // Success
}

/**
 * show image cache on LCD
 *
 * @param idx
 * @param x
 * @param y
 * @param w
 * @param h
 */
static int AmpUT_StlDec_ShowCacheOnLCD(UINT8 idx, UINT32 x, UINT32 y, UINT32 w, UINT32 h)
{
    AMP_STILLDEC_RESCALE_s Rescale;
    AMP_YUV_BUFFER_s Src;
    AMP_YUV_BUFFER_s Dest;
    AMP_ROTATION_e Rotate = AMP_ROTATE_0;
    UINT8 LumaGain = 128; // 128: original luma
    AMP_STILLDEC_DISPLAY_s Display;
    AMP_YUV_BUFFER_s Buf;
    UINT8 *LumaAddr = NULL;
    UINT8 *ChromaAddr = NULL;
    AMP_DISP_INFO_s DispInfo = {0};

    // Start decoder
    // If seamless is enabled and the decoder is not started yet, LCD buffer will be updated by DSP command.
    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        AmbaPrint("%s:%u AmpUT_StlDec_StillDecodeStart failed", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    AmpUT_Display_GetInfo(0, &DispInfo);
    if (DispInfo.DeviceInfo.VoutWidth == 0 || DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout width(%u) or height(%u).", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.Enable == 0) {
        AmbaPrint("%s:%u LCD not enabled", __FUNCTION__, __LINE__);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutWidth == 0) {
        AmbaPrint("%s:%u Illegal Vout width %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout height %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }

    // Get Vout buffer address
    AmpUT_StlDec_GetNextVoutBuffer_LCD(&LumaAddr, &ChromaAddr);

    Rescale.NumBuf = 1;
    Rescale.Src = &Src;
    Rescale.Dest = &Dest;
    Rescale.Rotate = &Rotate;
    Rescale.LumaGain = &LumaGain;

    Src.ColorFmt = AMP_YUV_422;
    Src.LumaAddr = DecCache + (idx*CACHE_SIZE*2);
    Src.ChromaAddr = DecCache + (idx*CACHE_SIZE*2) + CACHE_SIZE;
    Src.Width = CACHE_WIDTH;
    Src.Height = CACHE_HEIGHT;
    Src.Pitch = CACHE_PITCH;
    Src.AOI.X = 0;
    Src.AOI.Y = 0;
    Src.AOI.Width = CACHE_WIDTH;
    Src.AOI.Height = CACHE_HEIGHT;


    Dest.ColorFmt = AMP_YUV_422;
    Dest.LumaAddr = LumaAddr;
    Dest.ChromaAddr = ChromaAddr;
    Dest.Width = DispInfo.DeviceInfo.VoutWidth;
    Dest.Height = DispInfo.DeviceInfo.VoutHeight;
    Dest.Pitch = LcdVoutPitch;
    Dest.AOI.X = x;
    Dest.AOI.Y = y;
    Dest.AOI.Width = w;
    Dest.AOI.Height = h;

    if (AmpStillDec_Rescale(StlDecHdlr, &Rescale) != AMP_OK) {
        AmbaPrint("%s:%u Failed to rescale the image.", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

    Display.Vout = AMP_DISP_CHANNEL_DCHAN; // LCD
    Display.Buf = &Buf;
    Buf.ColorFmt = AMP_YUV_422;
    Buf.AOI.X = 0;
    Buf.AOI.Y = 0;
    Buf.AOI.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.AOI.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.Pitch = LcdVoutPitch;
    Buf.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.LumaAddr = LumaAddr;
    Buf.ChromaAddr = ChromaAddr;

    // Still decode on LCD is always full screen. No need to update window.

    if (AmpStillDec_Display(StlDecHdlr, &Display) != AMP_OK) {
        AmbaPrint("%s:%u Failed to display the image.", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

    return 0; // Success
}

/**
 * show image cache on TV
 *
 * @param idx
 * @param x
 * @param y
 * @param w
 * @param h
 */
static int AmpUT_StlDec_ShowCacheOnTV(UINT8 idx, UINT32 x, UINT32 y)
{
    AMP_STILLDEC_RESCALE_s Rescale;
    AMP_YUV_BUFFER_s Src;
    AMP_YUV_BUFFER_s Dest;
    AMP_ROTATION_e Rotate = AMP_ROTATE_0;
    UINT8 LumaGain = 128; // 128: original luma
    AMP_STILLDEC_DISPLAY_s Display;
    AMP_YUV_BUFFER_s Buf;
    UINT8 *LumaAddr = NULL;
    UINT8 *ChromaAddr = NULL;
    AMP_DISP_INFO_s DispInfo = {0};

    // Start decoder
    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        AmbaPrint("%s:%u AmpUT_StlDec_StillDecodeStart failed", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    AmpUT_Display_GetInfo(1, &DispInfo);
    if (DispInfo.DeviceInfo.VoutWidth == 0 || DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout width(%u) or height(%u).", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.Enable == 0) {
        AmbaPrint("%s:%u TV not enabled", __FUNCTION__, __LINE__);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutWidth == 0) {
        AmbaPrint("%s:%u Illegal Vout width %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth);
        return -1; // Error
    }
    if (DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout height %u", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }

    // Get Vout buffer address
    AmpUT_StlDec_GetNextVoutBuffer_TV(&LumaAddr, &ChromaAddr);

    Rescale.NumBuf = 1;
    Rescale.Src = &Src;
    Rescale.Dest = &Dest;
    Rescale.Rotate = &Rotate;
    Rescale.LumaGain = &LumaGain;

    Src.ColorFmt = AMP_YUV_422;
    Src.LumaAddr = DecCache + (idx*CACHE_SIZE*2);
    Src.ChromaAddr = DecCache + (idx*CACHE_SIZE*2) + CACHE_SIZE;
    Src.Width = CACHE_WIDTH;
    Src.Height = CACHE_HEIGHT;
    Src.Pitch = CACHE_PITCH;
    Src.AOI.X = 0;
    Src.AOI.Y = 0;
    Src.AOI.Width = CACHE_WIDTH;
    Src.AOI.Height = CACHE_HEIGHT;


    Dest.ColorFmt = AMP_YUV_422;
    Dest.LumaAddr = LumaAddr;
    Dest.ChromaAddr = ChromaAddr;
    Dest.Width = DispInfo.DeviceInfo.VoutWidth;
    Dest.Height = DispInfo.DeviceInfo.VoutHeight;
    Dest.Pitch = TvVoutPitch;
    Dest.AOI.X = x;
    Dest.AOI.Y = y;
    Dest.AOI.Width = DispInfo.DeviceInfo.VoutWidth;
    Dest.AOI.Height = DispInfo.DeviceInfo.VoutHeight;

    if (AmpStillDec_Rescale(StlDecHdlr, &Rescale) != AMP_OK) {
        AmbaPrint("%s:%u Failed to rescale the image.", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

    Display.Vout = AMP_DISP_CHANNEL_FCHAN; // TV
    Display.Buf = &Buf;
    Buf.ColorFmt = AMP_YUV_422;
    Buf.AOI.X = 0;
    Buf.AOI.Y = 0;
    Buf.AOI.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.AOI.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.Pitch = TvVoutPitch;
    Buf.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.LumaAddr = LumaAddr;
    Buf.ChromaAddr = ChromaAddr;

    // Still decode on TV is always full screen. No need to update window.

    if (AmpStillDec_Display(StlDecHdlr, &Display) != AMP_OK) {
        AmbaPrint("%s:%u Failed to display the image.", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

    return 0; // Success
}

/**
 * Show black screen on LCD
 */
static int AmpUT_StlDec_ShowBlackOnLCD(void)
{
    AMP_STILLDEC_DISPLAY_s Display;
    AMP_YUV_BUFFER_s Buf;
    UINT8 *LumaAddr = NULL;
    UINT8 *ChromaAddr = NULL;
    AMP_DISP_INFO_s DispInfo = {0};

    AmpUT_Display_GetInfo(0, &DispInfo);
    if (DispInfo.DeviceInfo.VoutWidth == 0 || DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout width(%u) or height(%u).", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }

    // Start decoder
    // If seamless is enabled and the decoder is not started yet, LCD buffer will be updated by DSP command.
    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        AmbaPrint("%s:%u AmpUT_StlDec_StillDecodeStart failed", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    // Get Vout buffer address
    AmpUT_StlDec_GetNextVoutBuffer_LCD(&LumaAddr, &ChromaAddr);

    Display.Vout = AMP_DISP_CHANNEL_DCHAN; // LCD
    Display.Buf = &Buf;
    Buf.ColorFmt = AMP_YUV_422;
    Buf.AOI.X = 0;
    Buf.AOI.Y = 0;
    Buf.AOI.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.AOI.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.Pitch = LcdVoutPitch;
    Buf.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.LumaAddr = LumaAddr;
    Buf.ChromaAddr = ChromaAddr;

    // Still decode on LCD is always full screen. No need to update window.

    if (AmpStillDec_Display(StlDecHdlr, &Display) != AMP_OK) {
        AmbaPrint("%s:%u Failed to display the image.", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

    return 0; // Success
}

/**
 * Show black screen on TV
 */
static int AmpUT_StlDec_ShowBlackOnTV(void)
{
    AMP_STILLDEC_DISPLAY_s Display;
    AMP_YUV_BUFFER_s Buf;
    UINT8 *LumaAddr = NULL;
    UINT8 *ChromaAddr = NULL;
    AMP_DISP_INFO_s DispInfo = {0};

    AmpUT_Display_GetInfo(1, &DispInfo);
    if (DispInfo.DeviceInfo.VoutWidth == 0 || DispInfo.DeviceInfo.VoutHeight == 0) {
        AmbaPrint("%s:%u Illegal Vout width(%u) or height(%u).", __FUNCTION__, __LINE__, DispInfo.DeviceInfo.VoutWidth, DispInfo.DeviceInfo.VoutHeight);
        return -1; // Error
    }

    // Start decoder
    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        AmbaPrint("%s:%u AmpUT_StlDec_StillDecodeStart failed", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    // Get Vout buffer address
    AmpUT_StlDec_GetNextVoutBuffer_TV(&LumaAddr, &ChromaAddr);

    Display.Vout = AMP_DISP_CHANNEL_FCHAN; // TV
    Display.Buf = &Buf;
    Buf.ColorFmt = AMP_YUV_422;
    Buf.AOI.X = 0;
    Buf.AOI.Y = 0;
    Buf.AOI.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.AOI.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.Width = DispInfo.DeviceInfo.VoutWidth;
    Buf.Pitch = TvVoutPitch;
    Buf.Height = DispInfo.DeviceInfo.VoutHeight;
    Buf.LumaAddr = LumaAddr;
    Buf.ChromaAddr = ChromaAddr;

    // Still decode on TV is always full screen. No need to update window.

    if (AmpStillDec_Display(StlDecHdlr, &Display) != AMP_OK) {
        AmbaPrint("%s:%u Failed to display the image.", __FUNCTION__, __LINE__);
        return -1; // Error
    }

    // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

    return 0; // Success
}

/**
 * Show image on Vout
 * @return 0: OK
 */
int AmpUT_StlDec_Disp(int argc, char **argv)
{
    UINT32 Idx;
    UINT8 Ch = 0;
    int InputValue;
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    if (argc < 3) {
       AmbaPrint("%s: Not enough parameters", __FUNCTION__);
       ErrMsg = "Not enough parameters";
       goto ReturnError;
   } else if (argc > 4) {
       AmbaPrint("%s: Too much parameters", __FUNCTION__);
       ErrMsg = "Too much parameters";
       goto ReturnError;
   }

    if (argc >= 3) {
        InputValue = atoi(argv[2]);
        if (InputValue < 0 || InputValue >= CACHE_NUM) {
            AmbaPrint("%s Cache ID (%u) out of range (0 ~ %u).", __FUNCTION__, InputValue, CACHE_NUM - 1);
            ErrMsg = "Illegal cache ID";
            goto ReturnError;
        }
        Idx = (UINT32) InputValue;
    }

    if (argc >= 4) {
        InputValue = atoi(argv[3]);
        if (InputValue != 0 && InputValue != 1) {
            AmbaPrint("%s: Channel cannot be %d", __FUNCTION__, InputValue);
            ErrMsg = "Illegal channel";
            goto ReturnError;
        }
        Ch = (UINT8) InputValue;
    }

    if (Ch == 0) {
        Er = AmpUT_StlDec_ShowCacheOnLCD(Idx, 0, 0, LcdVoutWidth, LcdVoutHeight);
        if (Er != 0) {
            ErrMsg = "AmpUT_StlDec_ShowCacheOnLCD failed";
            goto ReturnError;
        }
    } else if (Ch == 1) {
        Er = AmpUT_StlDec_ShowCacheOnTV(Idx, 0, 0);
        if (Er != 0) {
            ErrMsg = "AmpUT_StlDec_ShowCacheOnTV failed";
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
 * Show image in specified buffer on Vout
 * @return 0: OK
 */
int AmpUT_StlDec_DispBuf(int argc, char **argv)
{
    UINT8 Ch = 0;
    UINT8 *LumaAddr;
    UINT8 *ChromaAddr;
    int InputValue;
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    if (argc < 5) {
       AmbaPrint("%s: Not enough parameters", __FUNCTION__);
       ErrMsg = "Not enough parameters";
       goto ReturnError;
   } else if (argc > 5) {
       AmbaPrint("%s: Too much parameters", __FUNCTION__);
       ErrMsg = "Too much parameters";
       goto ReturnError;
   }

    if (argc >= 3) {
        InputValue = atoi(argv[2]);
        if (InputValue != 0 && InputValue != 1) {
            AmbaPrint("%s: Channel cannot be %d", __FUNCTION__, InputValue);
            ErrMsg = "Illegal channel";
            goto ReturnError;
        }
        Ch = (UINT8) InputValue;
    }

    if (argc >= 4) {
        LumaAddr = (UINT8 *) strtoul(argv[3], NULL, 0);
        if (LumaAddr == NULL) {
            AmbaPrint("%s: Illegal luma adderss 0x%08X", __FUNCTION__, LumaAddr);
            ErrMsg = "Illegal luma adderss";
            goto ReturnError;
        }
    }

    if (argc >= 5) {
        ChromaAddr = (UINT8 *) strtoul(argv[4], NULL, 0);
        if (ChromaAddr == NULL) {
            AmbaPrint("%s: Illegal chroma adderss 0x%08X", __FUNCTION__, ChromaAddr);
            ErrMsg = "Illegal chroma adderss";
            goto ReturnError;
        }
    }

    if (Ch == 0) {
        Er = AmpUT_StlDec_ShowBufOnLCD(LumaAddr, ChromaAddr);
        if (Er != 0) {
            ErrMsg = "AmpUT_StlDec_ShowBufOnLCD failed";
            goto ReturnError;
        }
    } else if (Ch == 1) {
        Er = AmpUT_StlDec_ShowBufOnTV(LumaAddr, ChromaAddr);
        if (Er != 0) {
            ErrMsg = "AmpUT_StlDec_ShowBufOnTV failed";
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
 * Show black screen
 * @return 0: OK
 */
int AmpUT_StlDec_DispBlack(int argc, char **argv)
{
    UINT8 Ch = 0;
    int InputValue;
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    if (argc < 3) {
       AmbaPrint("%s: Not enough parameters", __FUNCTION__);
       ErrMsg = "Not enough parameters";
       goto ReturnError;
   } else if (argc > 3) {
       AmbaPrint("%s: Too much parameters", __FUNCTION__);
       ErrMsg = "Too much parameters";
       goto ReturnError;
   }

    if (argc >= 3) {
        InputValue = atoi(argv[2]);
        if (InputValue != 0 && InputValue != 1) {
            AmbaPrint("%s: Channel cannot be %d", __FUNCTION__, InputValue);
            ErrMsg = "Illegal channel";
            goto ReturnError;
        }
        Ch = (UINT8) InputValue;
    }

    if (Ch == 0) {
        Er = AmpUT_StlDec_ShowBlackOnLCD();
        if (Er != 0) {
            ErrMsg = "AmpUT_StlDec_ShowBlackOnLCD failed";
            goto ReturnError;
        }
    } else { // Ch == 1
        Er = AmpUT_StlDec_ShowBlackOnTV();
        if (Er != 0) {
            ErrMsg = "AmpUT_StlDec_ShowBlackOnTV failed";
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
 * Motion test function, It will show object with given image cache index to lcd screen.
 * @return
 */
int AmpUT_StlDec_Motion(int argc, char **argv)
{
    AMP_STILLDEC_RESCALE_s Rescale;
    AMP_YUV_BUFFER_s Src;
    AMP_YUV_BUFFER_s Dest;
    AMP_ROTATION_e Rotate = AMP_ROTATE_0;
    UINT8 LumaGain = 128; // 128: original luma
    UINT8 Idx;
    UINT8 *LumaAddr = NULL;
    UINT8 *ChromaAddr = NULL;
    int Er = 0;
    char *ErrMsg = "";
    int T = 0;
    int InputValue;

    AMP_STILLDEC_DISPLAY_s Display;
    AMP_YUV_BUFFER_s Buf;

    AmbaPrint("%s", __FUNCTION__);

    InputValue = atoi(argv[2]);
    if (InputValue < 0 || InputValue >= CACHE_NUM) {
        AmbaPrint("%s Cache ID (%u) out of range (0 ~ %u).", __FUNCTION__, InputValue, CACHE_NUM - 1);
        ErrMsg = "Illegal cache ID";
        goto ReturnError;
    }
    Idx = InputValue;

    // Start decoder
    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        ErrMsg = "AmpUT_StlDec_StillDecodeStart failed";
        goto ReturnError;
    }

    // For each frame
    for (T = 0 ; T < 120 ; T++) {
        /** Rescale the image in cache to LCD buffer */
        // Prepare vout buffer. Apply multiple buffering.

        // Get Vout buffer address
        Er = AmpUT_StlDec_GetNextVoutBuffer_LCD(&LumaAddr, &ChromaAddr);
        if (Er != 0) {
            ErrMsg = "AmpUT_StlDec_GetNextVoutBuffer_LCD failed";
            goto ReturnError;
        }
        // Configure Rescale settings
        Rescale.NumBuf = 1;
        Rescale.Src = &Src;
        Rescale.Dest = &Dest;
        Rescale.Rotate = &Rotate;
        Rescale.LumaGain = &LumaGain;
        // Configure source image settings
        Src.ColorFmt = AMP_YUV_422;
        Src.LumaAddr = DecCache + (Idx*CACHE_SIZE*2);
        Src.ChromaAddr = DecCache + (Idx*CACHE_SIZE*2) + CACHE_SIZE;
        Src.Width = CACHE_WIDTH;
        Src.Height = CACHE_HEIGHT;
        Src.Pitch = CACHE_PITCH;
        Src.AOI.X = 0;
        Src.AOI.Y = 0;
        Src.AOI.Width = CACHE_WIDTH;
        Src.AOI.Height = CACHE_HEIGHT;
        // Configure target image settings
        Dest.ColorFmt = AMP_YUV_422;
        Dest.LumaAddr = LumaAddr;
        Dest.ChromaAddr = ChromaAddr;
        Dest.Width = LcdVoutWidth;
        Dest.Height = LcdVoutHeight;
        Dest.Pitch = LcdVoutPitch;
        Dest.AOI.X = 160 + abs(T-60)*10;
        Dest.AOI.Y = 10 + abs(T-60)*3;
        Dest.AOI.Width = 128;
        Dest.AOI.Height = 128;
        // Rescale souece image to target image
        if (AmpStillDec_Rescale(StlDecHdlr, &Rescale) != AMP_OK) {
            AmbaPrint("%s:%u Failed to Rescale the image.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpStillDec_Rescale failed";
            goto ReturnError;
        }

        // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

        /** Display the image in LCD buffer */
        // Configure Display settings
        Display.Vout = AMP_DISP_CHANNEL_DCHAN; // LCD
        Display.Buf = &Buf;
        Buf.ColorFmt = AMP_YUV_422;
        Buf.LumaAddr = LumaAddr;
        Buf.ChromaAddr = ChromaAddr;
        Buf.Width = Dest.Width;
        Buf.Height = Dest.Height;
        Buf.Pitch = Dest.Pitch;
        Buf.AOI.X = 0;
        Buf.AOI.Y = 0;
        Buf.AOI.Width = Dest.Width;
        Buf.AOI.Height = Dest.Height;
        // Start displaying image
        if (AmpStillDec_Display(StlDecHdlr, &Display) != AMP_OK) {
            AmbaPrint("%s:%u Failed to Display the image.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpStillDec_Display failed";
            goto ReturnError;
        }

        // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

        AmbaKAL_TaskSleep(15);
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Blend test function\n
 * It will blend two given image cache to the target cache.
 */
int AmpUT_StlDec_Blend(int argc, char **argv)
{
    UINT8 Idx1, Idx2, Idx3, Alpha;
    AMP_YUV_BUFFER_s Src1, Src2;
    AMP_YUV_BUFFER_s Dest;
    AMP_STILLDEC_BLEND_s Blend = {0};
    int Er = 0;
    char *ErrMsg = "";
    int InputValue;

    AmbaPrint("%s", __FUNCTION__);

    InputValue = atoi(argv[2]);
    if (InputValue < 0 || InputValue >= CACHE_NUM) {
        AmbaPrint("%s Cache ID (%u) out of range (0 ~ %u).", __FUNCTION__, InputValue, CACHE_NUM - 1);
        ErrMsg = "Illegal cache ID";
        goto ReturnError;
    }
    Idx1 = InputValue;

    InputValue = atoi(argv[3]);
    if (InputValue < 0 || InputValue >= CACHE_NUM) {
        AmbaPrint("%s Cache ID (%u) out of range (0 ~ %u).", __FUNCTION__, InputValue, CACHE_NUM - 1);
        ErrMsg = "Illegal cache ID";
        goto ReturnError;
    }
    Idx2 = InputValue;

    InputValue = atoi(argv[4]);
    if (InputValue < 0 || InputValue >= CACHE_NUM) {
        AmbaPrint("%s Cache ID (%u) out of range (0 ~ %u).", __FUNCTION__, InputValue, CACHE_NUM - 1);
        ErrMsg = "Illegal cache ID";
        goto ReturnError;
    }
    Idx3 = InputValue;

    Alpha = atoi(argv[5]);
    memset(AlphaMap, Alpha, sizeof(AlphaMap));
    // Since the buffer is cached, it needs to be cleaned so that DSP can get the currect data.
    AmbaCache_Clean((void *)AlphaMap, sizeof(AlphaMap));

    // Start decoder
    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        ErrMsg = "AmpUT_StlDec_StillDecodeStart failed";
        goto ReturnError;
    }

    /** Configure blend settings */
    Blend.AlphaMap = AlphaMap;
    Blend.GlobalAlpha = 0;
    Blend.Src1 = &Src1;
    Blend.Src2 = &Src2;
    Blend.Dest = &Dest;

    /** Configure the first source image settings */
    Src1.ColorFmt = AMP_YUV_422;
    Src1.LumaAddr = DecCache + (Idx1*CACHE_SIZE*2);
    Src1.ChromaAddr = DecCache + (Idx1*CACHE_SIZE*2) + CACHE_SIZE;
    Src1.Width = CACHE_WIDTH;
    Src1.Height = CACHE_HEIGHT;
    Src1.Pitch = CACHE_PITCH;
    Src1.AOI.X = 0;
    Src1.AOI.Y = 0;
    Src1.AOI.Width = CACHE_WIDTH;
    Src1.AOI.Height = CACHE_HEIGHT;

    /** Configure the second source image settings */
    Src2.ColorFmt = AMP_YUV_422;
    Src2.LumaAddr = DecCache + (Idx2*CACHE_SIZE*2);
    Src2.ChromaAddr = DecCache + (Idx2*CACHE_SIZE*2) + CACHE_SIZE;
    Src2.Width = CACHE_WIDTH;
    Src2.Height = CACHE_HEIGHT;
    Src2.Pitch = CACHE_PITCH;
    Src2.AOI.X = 0;
    Src2.AOI.Y = 0;
    Src2.AOI.Width = CACHE_WIDTH;
    Src2.AOI.Height = CACHE_HEIGHT;

    /** Configure target image settings */
    Dest.ColorFmt = AMP_YUV_422;
    Dest.LumaAddr = DecCache + (Idx3*CACHE_SIZE*2);
    Dest.ChromaAddr = DecCache + (Idx3*CACHE_SIZE*2) + CACHE_SIZE;
    Dest.Width = CACHE_WIDTH;
    Dest.Height = CACHE_HEIGHT;
    Dest.Pitch = CACHE_PITCH;
    Dest.AOI.X = 0;
    Dest.AOI.Y = 0;
    Dest.AOI.Width = CACHE_WIDTH;
    Dest.AOI.Height = CACHE_HEIGHT;

    /** Start alpha blending */
    if (AmpStillDec_Blend(StlDecHdlr, &Blend) != AMP_OK) {
        AmbaPrint("%s:%u Failed to blend the image.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpStillDec_Blend failed";
        goto ReturnError;
    }

    // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

    /** Start displaying image */
    Er = AmpUT_StlDec_ShowCacheOnLCD(Idx3, 0, 0, LcdVoutWidth, LcdVoutHeight);
    if (Er != 0) {
        ErrMsg = "AmpUT_StlDec_ShowCacheOnLCD failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Blend test function\n
 * It will blend two given image cache with alpha map to the target cache.
 */
int AmpUT_StlDec_MapBlend(int argc, char **argv)
{
    UINT8 Idx1, Idx2, Idx3, AlphaMapIdx;
    AMP_YUV_BUFFER_s Src1, Src2;
    AMP_YUV_BUFFER_s Dest;
    AMP_STILLDEC_BLEND_s Blend = {0};
    UINT8* AlphaMap;
    int T1, T2;
    int Er = 0;
    char *ErrMsg = "";
    int InputValue;

    AmbaPrint("%s", __FUNCTION__);

    InputValue = atoi(argv[2]);
    if (InputValue < 0 || InputValue >= CACHE_NUM) {
        AmbaPrint("%s Cache ID (%u) out of range (0 ~ %u).", __FUNCTION__, InputValue, CACHE_NUM - 1);
        ErrMsg = "Illegal cache ID";
        goto ReturnError;
    }
    Idx1 = InputValue;

    InputValue = atoi(argv[3]);
    if (InputValue < 0 || InputValue >= CACHE_NUM) {
        AmbaPrint("%s Cache ID (%u) out of range (0 ~ %u).", __FUNCTION__, InputValue, CACHE_NUM - 1);
        ErrMsg = "Illegal cache ID";
        goto ReturnError;
    }
    Idx2 = InputValue;

    InputValue = atoi(argv[4]);
    if (InputValue < 0 || InputValue >= CACHE_NUM) {
        AmbaPrint("%s Cache ID (%u) out of range (0 ~ %u).", __FUNCTION__, InputValue, CACHE_NUM - 1);
        ErrMsg = "Illegal cache ID";
        goto ReturnError;
    }
    Idx3 = InputValue;

    InputValue = atoi(argv[5]);
    if (InputValue < 0 || InputValue >= CACHE_NUM) {
        AmbaPrint("%s Cache ID (%u) out of range (0 ~ %u).", __FUNCTION__, InputValue, CACHE_NUM - 1);
        ErrMsg = "Illegal cache ID";
        goto ReturnError;
    }
    AlphaMapIdx = InputValue;

    AmbaPrint("%s: Idx1 = %u, Idx2 = %u, Idx3 = %u, AlphaMapIdx = %u", __FUNCTION__, Idx1, Idx2, Idx3, AlphaMapIdx);

    AlphaMap = DecCache + (AlphaMapIdx*CACHE_SIZE*2);

    for (T1 = 0 ; T1 < CACHE_HEIGHT; T1++) {
        int k = T1 * CACHE_PITCH;
        for (T2 = 0 ; T2 < CACHE_PITCH; T2++) {
            AlphaMap[k + T2] = T1 + T2;
        }
    }

    // Since the buffer is cached, it needs to be cleaned so that DSP can get the currect data.
    AmbaCache_Clean((void *)AlphaMap, CACHE_SIZE);

    // Start decoder
    if (AmpUT_StlDec_StillDecodeStart() != AMP_OK) {
        ErrMsg = "AmpUT_StlDec_StillDecodeStart failed";
        goto ReturnError;
    }

    Blend.AlphaMap = AlphaMap;
    Blend.GlobalAlpha = 0;
    Blend.Src1 = &Src1;
    Blend.Src2 = &Src2;
    Blend.Dest = &Dest;

    Src1.ColorFmt = AMP_YUV_422;
    Src1.LumaAddr = DecCache + (Idx1*CACHE_SIZE*2);
    Src1.ChromaAddr = DecCache + (Idx1*CACHE_SIZE*2) + CACHE_SIZE;
    Src1.Width = CACHE_WIDTH;
    Src1.Height = CACHE_HEIGHT;
    Src1.Pitch = CACHE_PITCH;
    Src1.AOI.X = 0;
    Src1.AOI.Y = 0;
    Src1.AOI.Width = CACHE_WIDTH;
    Src1.AOI.Height = CACHE_HEIGHT;

    Src2.ColorFmt = AMP_YUV_422;
    Src2.LumaAddr = DecCache + (Idx2*CACHE_SIZE*2);
    Src2.ChromaAddr = DecCache + (Idx2*CACHE_SIZE*2) + CACHE_SIZE;
    Src2.Width = CACHE_WIDTH;
    Src2.Height = CACHE_HEIGHT;
    Src2.Pitch = CACHE_PITCH;
    Src2.AOI.X = 0;
    Src2.AOI.Y = 0;
    Src2.AOI.Width = CACHE_WIDTH;
    Src2.AOI.Height = CACHE_HEIGHT;

    Dest.ColorFmt = AMP_YUV_422;
    Dest.LumaAddr = DecCache + (Idx3*CACHE_SIZE*2);
    Dest.ChromaAddr = DecCache + (Idx3*CACHE_SIZE*2) + CACHE_SIZE;
    Dest.Width = CACHE_WIDTH;
    Dest.Height = CACHE_HEIGHT;
    Dest.Pitch = CACHE_PITCH;
    Dest.AOI.X = 0;
    Dest.AOI.Y = 0;
    Dest.AOI.Width = CACHE_WIDTH;
    Dest.AOI.Height = CACHE_HEIGHT;

    if (AmpStillDec_Blend(StlDecHdlr, &Blend) != AMP_OK) {
        AmbaPrint("%s:%u Failed to blend the image.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpStillDec_Blend failed";
        goto ReturnError;
    }

    // Don't have to clean or flush cache and main buffer since they're only accessed by DSP

    Er = AmpUT_StlDec_ShowCacheOnLCD(Idx3, 0, 0, LcdVoutWidth, LcdVoutHeight);
    if (Er != 0) {
        ErrMsg = "AmpUT_StlDec_ShowCacheOnLCD failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

int AmpUT_StlDec_Slide(int argc, char **argv)
{
    return 0;
}

int AmpUT_StlDec_Usage(void)
{
    AmbaPrint("AmpUT_StillDec");
    AmbaPrint("\t init - initialize still decoder");
    AmbaPrint("\t feed [fileName] [mediaType] - mediaType: 0 - JPEG, 1 - H264");
    AmbaPrint("\t dec [mediaWidth] [mediaHeight] [cacheId] - cacheId: target cache ID, 0 ~ 31");
    AmbaPrint("\t motion_test [cacheId] - cache ID to demonstrate motion, 0 ~ 31");
    AmbaPrint("\t blend_test [srcCacheId1] [srcCacheId2] [destCacheId1] [alpha] - alpha: 0~255");
    return 0;
}

int AmpUT_StlDecTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if ( strcmp(argv[1],"init") == 0) {
        AmpUT_StlDec_Init(atoi(argv[2]));
    } else if ( strcmp(argv[1],"feed") == 0) {
        AmpUT_StlDec_Feed(argc, argv);
    } else if ( strcmp(argv[1],"dec") == 0) {
        AmpUT_StlDec_Dec(argc, argv);
    } else if ( strcmp(argv[1],"resc") == 0) {
        AmpUT_StlDec_Rescale(argc, argv);
    } else if ( strcmp(argv[1],"disp") == 0) {
        AmpUT_StlDec_Disp(argc, argv);
    } else if ( strcmp(argv[1],"dispbuf") == 0) {
        AmpUT_StlDec_DispBuf(argc, argv);
    } else if ( strcmp(argv[1],"clearscreen") == 0) {
        AmpUT_StlDec_DispBlack(argc, argv);
    } else if ( strcmp(argv[1],"start") == 0) {
        AmpUT_StlDec_Start();
    } else if ( strcmp(argv[1],"stop") == 0) {
        AmpUT_StlDec_Stop();
    } else if ( strcmp(argv[1],"motion_test") == 0) {
        AmpUT_StlDec_Motion(argc, argv);
    } else if ( strcmp(argv[1],"blend_test") == 0) {
        AmpUT_StlDec_Blend(argc, argv);
    } else if ( strcmp(argv[1],"map_blend_test") == 0) {
        AmpUT_StlDec_MapBlend(argc, argv);
    } else if ( strcmp(argv[1],"ch") == 0) {
        AmpUT_StlDec_Ch(argv);
    } else if (strcmp(argv[1], "cvbs") == 0) {
        AmpUT_Display_CvbsCtrl(atoi(argv[2]));
    } else {
        AmpUT_StlDec_Usage();
    }
    return 0;
}

int AmpUT_StlDecInit(void)
{
    // hook command
    AmbaTest_RegisterCommand("stilldec", AmpUT_StlDecTest);
    AmbaTest_RegisterCommand("stldec", AmpUT_StlDecTest);

    return AMP_OK;
}
