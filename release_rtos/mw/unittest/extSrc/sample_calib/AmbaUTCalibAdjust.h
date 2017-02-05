/**
 * @file mw/unittest/extSrc/sample_calib/AmbaUTCalibAdjust.h
 *
 * sample code for vignette calibration
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


#include <vignette/AmbaUTCalibVig.h>
#include <AmbaUTCalibMgr.h>
#include <AmbaUTCalib_Adjustment.h>

extern CALIBRATION_ADJUST_PARAM_s* AmpUTCalibAdjustGetCalibrationParam(void);
extern Vignette_Control_s* AmpUTCalibVignette_GetVignetteCalData(void);
extern int AmpUTCalibAdjust_Init(void);
extern void AmpUTCalibAdjust_Func(void);
extern int AmpUTCalibAdjust_StillVignetteStrength(UINT8 Channel);
extern int AmpUTCalibVignette_GetParameter(Vignette_Select_Param_s *Param,UINT8 Channel, UINT8 Mode);
extern int AmpUTCalibVignette_SetParameter(Vignette_Select_Param_s *Param,UINT8 Channel, UINT8 Mode);


