/**
 * @file mw/unittest/extSrc/sample_calib/adjust/AmbaUTCalibParams_ov4689.h
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
#include <AmbaUTCalib_Adjustment.h>
/**
 * calibration control parameters for OV4689
 */
CALIBRATION_ADJUST_PARAM_s AmpUTCalibParamsOv4689 = {
    0x14031400,		/**< FW version number */
    0x14031400,		/**< parameter version number */
    0,              /**< vignette luma control flag */
    0,              /**< vignette chroma control flag */
    0,              /**< vignette OZ blend control flag */
    0,              /**< vignette WB blend control flag */
    10,             /**< frame period to control vignette luma strength */
    8,              /**< frame NO. to average the vignette strength (1*new_gain+(AverageNum-1)*old)/AverageNum */
    20,             /**< LV Threshold to change the luma strength*/ 
    20,             /**< LV Threshold to change the chroma strength*/
    
    //	   0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700 1800 1900 2000  // LV0 - LV20
    {{{  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}, //luma strength weight for video (0 ~ 256)
      {  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}}, //luma strength weight for still (0 ~ 256)
     {{  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}, //luma strength weight for video (0 ~ 256)
      {  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}}, //luma strength weight for still (0 ~ 256
     {{  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}, //luma strength weight for video (0 ~ 256)
      {  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}}, //luma strength weight for still (0 ~ 256
     {{  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}, //luma strength weight for video (0 ~ 256)
      {  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}}}, //luma strength weight for still (0 ~ 256
      
    {{{  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}, //chroma strength weight for video (0 ~ 256)
       {  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}}, //chroma strength weight for still (0 ~ 256)
     {{  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}, //chroma strength weight for video (0 ~ 256)
      {  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}}, //chroma strength weight for still (0 ~ 256
     {{  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}, //chroma strength weight for video (0 ~ 256)
      {  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}}, //chroma strength weight for still (0 ~ 256
     {{  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}, //chroma strength weight for video (0 ~ 256)
      {  256, 256, 256, 256, 256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256, 256,  256, 256, 256, 256, 256}}}, //chroma strength weight for still (0 ~ 256      
       // wb_idx = (b_gain * 4096)/r_gain, wb gain should be normalized.
    /* blend region weight table (0 ~ 256) */                                /* start */       /* end */
    { {0,0,{256, 224, 192, 160, 128,  96, 64, 32,  0}, {256, 224, 192, 160, 128,  96, 64, 32,  0}}, // Blend region #0 (6200K to 4000K)
      {0,0,{256, 224, 192, 160, 128,  96, 64, 32,  0}, {256, 224, 192, 160, 128,  96, 64, 32,  0}}, // Blend region #0 (6200K to 4000K)
      {0,0,{256, 224, 192, 160, 128,  96, 64, 32,  0}, {256, 224, 192, 160, 128,  96, 64, 32,  0}}, // Blend region #0 (6200K to 4000K)
      {0,0,{256, 224, 192, 160, 128,  96, 64, 32,  0}, {256, 224, 192, 160, 128,  96, 64, 32,  0}}}, // Blend region #0 (6200K to 4000K)
};



