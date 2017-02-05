/**
 * @file mw/unittest/extSrc/sample_calib/adjust/AmbaCalibParams.c
 *
 * calibration parameters to control the calibration in running time
 *
 * History:
 *    02/16/2015  Allen Chiu Created
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */


#include "AmbaDataType.h"

#if defined(CONFIG_SENSOR_OV4689)
    #include <adjust/AmbaUTCalibParams_ov4689.h>
#elif defined(CONFIG_SENSOR_AR0230)
    #include <adjust/AmbaUTCalibParams_ar0230.h>    
#elif defined(CONFIG_SENSOR_AR0330_PARALLEL)
    #include <adjust/AmbaUTCalibParams_ar0330_parallel.h>    
#elif defined(CONFIG_SENSOR_SONY_IMX117)
    #include <adjust/AmbaUTCalibParams_imx117.h>    
#elif defined(CONFIG_SENSOR_SONY_IMX206)
    #include <adjust/AmbaUTCalibParams_imx206.h>    
#elif defined(CONFIG_SENSOR_MN34120)
    #include <adjust/AmbaUTCalibParams_mn34120.h>   
#elif defined(CONFIG_SENSOR_OV9750)
    #include <adjust/AmbaUTCalibParams_ov9750.h>    
#elif defined(CONFIG_SENSOR_OV10823)
    #include <adjust/AmbaUTCalibParams_ov10823.h>    
#else
    #include <adjust/AmbaUTCalibParams_Default.h>
#endif


CALIBRATION_ADJUST_PARAM_s* AmpUTCalibAdjustGetCalibrationParam(void)
{
#if defined(CONFIG_SENSOR_OV4689)
	extern CALIBRATION_ADJUST_PARAM_s AmpUTCalibParamsOv4689;
	return &AmpUTCalibParamsOv4689;
#elif defined(CONFIG_SENSOR_AR0230)
	extern CALIBRATION_ADJUST_PARAM_s AmpUTCalibParamsAr0230;
	return &AmpUTCalibParamsAr0230;    
#elif defined(CONFIG_SENSOR_AR0330_PARALLEL)
	extern CALIBRATION_ADJUST_PARAM_s AmpUTCalibParamsAr0330Parallel;
	return &AmpUTCalibParamsAr0330Parallel;    
#elif defined(CONFIG_SENSOR_SONY_IMX117)
	extern CALIBRATION_ADJUST_PARAM_s AmpUTCalibParamsImx117;
	return &AmpUTCalibParamsImx117;    
#elif defined(CONFIG_SENSOR_SONY_IMX206)
	extern CALIBRATION_ADJUST_PARAM_s AmpUTCalibParamsImx206;
	return &AmpUTCalibParamsImx206;    
#elif defined(CONFIG_SENSOR_MN34120)
	extern CALIBRATION_ADJUST_PARAM_s AmpUTCalibParamsMn34120;
	return &AmpUTCalibParamsMn34120;       
#elif defined(CONFIG_SENSOR_OV9750)
	extern CALIBRATION_ADJUST_PARAM_s AmpUTCalibParamsOv9750;
	return &AmpUTCalibParamsOv9750;
#elif defined(CONFIG_SENSOR_OV10823)
	extern CALIBRATION_ADJUST_PARAM_s AmpUTCalibParamsOv10823;
	return &AmpUTCalibParamsOv10823;    
#else
    extern CALIBRATION_ADJUST_PARAM_s AmpUTCalibParamsDefault;
    return &AmpUTCalibParamsDefault;
#endif
}



