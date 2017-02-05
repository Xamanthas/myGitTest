/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaBuffers.c
 *
 *  @Copyright      :: Copyright (C) 2011 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions for all of the Data Buffers
 *
 *  @History        ::
 *      Date        Name        Comments
 *      12/01/2011  W.Shi       Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>

#include "AmbaDataType.h"
#include "AmbaKAL.h"
#include "AmbaPrintk.h"
#include "AmbaPrint.h"

#include "AmbaBuffers.h"
#include "AmbaAudio.h"

AMBA_KAL_BYTE_POOL_t  AmbaBytePool_Cached;      /* Cacheable Memory Pool */
AMBA_KAL_BYTE_POOL_t  AmbaBytePool_NonCached;   /* Non-Cacheable Memory Pool */

AMBA_MEM_CTRL_s AudioInputCached;               /* 4-byte alignment  */
AMBA_MEM_CTRL_s AudioInputNonCached;            /* 8-byte alignment  */
AMBA_MEM_CTRL_s AudioOutputCached;              /* 4-byte alignment  */
AMBA_MEM_CTRL_s AudioOutputNonCached;           /* 8-byte alignment  */
UINT32 AmbaDSP_TotalCacheMemSize;
UINT32 AmbaDSP_TotalNonCachedMemSize;
UINT32 AmbaDSP_TotalUsedNonCachedMem;
AMBA_AUDIO_IO_CREATE_INFO_s AudioInputInfo, AudioOutputInfo;
UINT32 AudioInputCachedSize, AudioInputNonCachedSize;
UINT32 AudioOutputCachedSize, AudioOutputNonCachedSize;

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaBufferInit
 *
 *  @Description:: Initialize all data buffers
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK/NG
\*-----------------------------------------------------------------------------------------------*/
int AmbaBufferInit(void)
{
    extern void *__non_cache_heap_start, *__non_cache_heap_end;
    extern void *__cache_heap_start, *__cache_heap_end;
    int    RetStatus = OK;

    AmbaDSP_TotalCacheMemSize = ((UINT32) &__cache_heap_end) - ((UINT32) &__cache_heap_start) - DSP_REV;
    RetStatus |= AmbaKAL_BytePoolCreate(&AmbaBytePool_Cached,
                                        (void*)((UINT32) &__cache_heap_start + DSP_REV),
                                        AmbaDSP_TotalCacheMemSize);

    AmbaDSP_TotalNonCachedMemSize = ((UINT32) &__non_cache_heap_end) - ((UINT32) &__non_cache_heap_start);
    RetStatus |= AmbaKAL_BytePoolCreate(&AmbaBytePool_NonCached,
                                        (void *) &__non_cache_heap_start,
                                        AmbaDSP_TotalNonCachedMemSize);

    AmbaDSP_TotalUsedNonCachedMem = 0;
    /*---------------------------------------------------------------------------*\
     * Buffer for Audio Input/Output
    \*---------------------------------------------------------------------------*/
#if 0
    AudioInputInfo.I2sIndex = 0;
    AudioInputInfo.MaxChNum = 2;
    AudioInputInfo.MaxDmaDescNum = 16;
    AudioInputInfo.MaxDmaSize = 1024;
    AudioInputInfo.MaxSampleFreq = 48000;
    
    AudioInputCachedSize = AmbaAudio_InputCachedSizeQuery(&AudioInputInfo);
    AudioInputNonCachedSize = AmbaAudio_InputNonCachedSizeQuery(&AudioInputInfo);
    AmbaKAL_MemAllocate(&AmbaBytePool_Cached, &AudioInputCached, AudioInputCachedSize, 4);
    AmbaKAL_MemAllocate(&AmbaBytePool_NonCached, &AudioInputNonCached, AudioInputNonCachedSize, 8);    

    AudioOutputInfo.I2sIndex = 0;
    AudioOutputInfo.MaxChNum = 2;
    AudioOutputInfo.MaxDmaDescNum = 16;
    AudioOutputInfo.MaxDmaSize = 256;
    AudioOutputInfo.MaxSampleFreq = 48000;
    
    AudioOutputCachedSize = AmbaAudio_OutputCachedSizeQuery(&AudioOutputInfo);
    AudioOutputNonCachedSize = AmbaAudio_OutputNonCachedSizeQuery(&AudioOutputInfo);
    AmbaKAL_MemAllocate(&AmbaBytePool_Cached, &AudioOutputCached, AudioOutputCachedSize, 4);
    AmbaKAL_MemAllocate(&AmbaBytePool_NonCached, &AudioOutputNonCached, AudioOutputNonCachedSize, 8);
#endif

    return RetStatus;
}

