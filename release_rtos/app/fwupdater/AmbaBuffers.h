/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaBuffers.h
 *
 *  @Copyright      :: Copyright (C) 2011 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions for all of various Data Buffers
 *
 *  @History        ::
 *      Date        Name        Comments
 *      12/01/2011  W.Shi       Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_BUFFERS_H_
#define _AMBA_BUFFERS_H_

#include "AmbaKAL.h"
#include "AmbaAudio.h"

/* It will be removed until uCode fix back2livew issue. */

#define ALIGN64(X)   ((X + 63) & 0xFFFFFFC0)
#define ALIGN32(X)   ((X + 31) & 0xFFFFFFE0)
#define ALIGN16(X)   ((X + 15) & 0xFFFFFFF0)
#define ALIGN8(X)    ((X + 7) & 0xFFFFFFF8)

extern AMBA_KAL_BYTE_POOL_t  AmbaBytePool_Cached;      /* Cacheable Memory Pool */
extern AMBA_KAL_BYTE_POOL_t  AmbaBytePool_NonCached;   /* Non-Cacheable Memory Pool */
extern AMBA_MEM_CTRL_s AudioInputCached;               /* 4-byte alignment  */
extern AMBA_MEM_CTRL_s AudioInputNonCached;            /* 8-byte alignment  */
extern AMBA_MEM_CTRL_s AudioOutputCached;              /* 4-byte alignment  */
extern AMBA_MEM_CTRL_s AudioOutputNonCached;           /* 8-byte alignment  */
extern UINT32 AmbaDSP_TotalCacheMemSize;
extern UINT32 AmbaDSP_TotalNonCachedMemSize;
extern UINT32 AmbaDSP_TotalUsedNonCachedMem;
extern AMBA_AUDIO_IO_CREATE_INFO_s AudioInputInfo, AudioOutputInfo;
extern UINT32 AudioInputCachedSize, AudioInputNonCachedSize;
extern UINT32 AudioOutputCachedSize, AudioOutputNonCachedSize;

#define DSP_REV     (256<<20)   //dsp max usage 256 MB

int AmbaBufferInit(void);

#endif  /* _AMBA_BUFFERS_H_ */
