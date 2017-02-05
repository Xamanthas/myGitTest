/**
 * @file mw/unittest/extSrc/sample_calib/blc/AmbaUTCalibBlc.h
 *
 * Header file for black level calibration
 *
 * History:
 *    07/10/2013  Allen Chiu Created
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "../AmbaUTCalibMgr.h"
#include "AmbaCalib_Blc.h"

extern int AmpCalib_BLCFunc(Cal_Blc_Info_s *CalBlcInfo, AMBA_DSP_IMG_BLACK_CORRECTION_s *BlResult, char *OutputStr);
extern void Amba_AeAwb_Get_AAA_OP_Info(AMBA_3A_OP_INFO_s *pAaaOpInfo);
extern void Amba_AeAwb_Set_AAA_OP_Info(AMBA_3A_OP_INFO_s *pAaaOpInfo);
extern int    AeSetVideoExposureValue(AMBA_AE_INFO_s *VideoInfo);
extern int Amba_AeAwb_Set_Ae_Info(UINT8 Mode, AMBA_AE_INFO_s *AeInfo);
extern int Amba_AeAwb_Set_Dgain(UINT32 Dgain);
extern int AmpUT_CalibSiteInit(void);


