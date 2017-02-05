/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSample_AeAwbAdj.h
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions & Constants for Sample AE(Auto Exposure) ,AWB(Auto White Balance), ADJ(Auto Adjustment)) algorithms
 *
 *  @History        ::
 *      Date        Name        Comments
 *      03/12/2013  Jyh-Jiun Li Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_SAMPLE_AE_AWB_ADJ_
#define _AMBA_SAMPLE_AE_AWB_ADJ_

#include "AmbaImg_Proc.h"
#include "AmbaDSP_ImgDef.h"

/*-----------------------------------------------------------------------------------------------*\
 * Defined in AmbaSample_Ae.c
\*-----------------------------------------------------------------------------------------------*/
void AmbaSample_AeInit(UINT32 chNo);
void AmbaSample_AeControl(UINT32 chNo, AMBA_3A_STATUS_s *aaaVideoStatus, AMBA_3A_STATUS_s *aaaStillStatus);
int AeGetVideoExposureValue(UINT32 chNo, float *shutter, float *agc, UINT32 *dgain);

/*-----------------------------------------------------------------------------------------------*\
 * Defined in AmbaSample_Awc.c
\*-----------------------------------------------------------------------------------------------*/
void AmbaSample_AwbInit(UINT32 chNo);
void AmbaSample_AwbControl(UINT32 chNo, AMBA_3A_STATUS_s *aaaVideoStatus, AMBA_3A_STATUS_s *aaaStillStatus);

/*-----------------------------------------------------------------------------------------------*\
 * Defined in AmbaSample_Adj.c
\*-----------------------------------------------------------------------------------------------*/
void AmbaSample_AdjInit(UINT32 chNo);
void AmbaSample_AdjControl(UINT32 chNo);
void AmbaSample_AdjStillControl(UINT32 chNo, UINT32 aeIndx, AMBA_DSP_IMG_MODE_CFG_s *mode, UINT16 algoMode);

#endif  /* _AMBA_DSC_AE_AWB_ */
