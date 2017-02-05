/**
 * @file src/app/connected/applib/inc/calibration/ApplibCalibAdjust.h
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


#include <calibration/vig/ApplibCalibVig.h>
#include <calibration/ApplibCalibMgr.h>

extern int AmbaUTVigControllerInit (void);
extern int AmbaUTVigControllerVigMain(void);
extern CALIBRATION_ADJUST_PARAM_s* AppLibCalibAdjustGetCalibrationParam(void);
extern Vignette_Control_s* AppLibCalibVignette_GetVignetteCalData(void);
extern int AppLibCalibAdjust_Init(void);
extern void AppLibCalibAdjust_Func(void);
extern int AppLibCalibAdjust_StillVignetteStrength(UINT8 Channel);


