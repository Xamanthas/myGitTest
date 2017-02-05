/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_AR1335.h
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of Aptina AR1335 CMOS sensor with MIPI interface
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_SENSOR_AR1335_H_
#define _AMBA_SENSOR_AR1335_H_

/*-----------------------------------------------------------------------------------------------*\
 * Control registers
\*-----------------------------------------------------------------------------------------------*/
#define AR1335_NUM_INIT_REG            275
#define AR1335_NUM_READOUT_MODE_REG    31
#define AR1335_ANA_DIG_GAIN_DIRECT     1
#ifdef AR1335_ANA_DIG_GAIN_DIRECT
#define AR1335_ANA_DIG_GAIN_GLOBAL     1
#endif

typedef enum _AR1335_CTRL_REG_e_ {
    AR1335_MODE_SELECT_REG       = 0x0100,
    AR1335_IMG_ORIENTATION_REG   = 0x0101,
    AR1335_FRM_LENGTH_REG        = 0x0340,
    AR1335_COARSE_INTEG_TIME_REG = 0x3012,
    AR1335_ANA_GAIN_GLOBAL_REG   = 0x3028,
    AR1335_DIG_GAIN_GR_REG       = 0x3032,
    AR1335_DIG_GAIN_R_REG        = 0x3034,
    AR1335_DIG_GAIN_B_REG        = 0x3036,
    AR1335_DIG_GAIN_GB_REG       = 0x3038,
    AR1335_GLOBAL_GAIN_REG       = 0x305E,
    AR1335_GAIN_GB_REG           = 0x3056,
    AR1335_GAIN_B_REG            = 0x3058,
    AR1335_GAIN_R_REG            = 0x305A,
    AR1335_GAIN_GR_REG           = 0x305C,
    AR1335_FRAME_LENGTH_REG      = 0x300A,
    AR1335_LINE_LENGTH_REG       = 0x300C,
} AR1335_CTRL_REG_e;

typedef enum _AR1335_READOUT_MODE_e_ {
    /* wide fov */
    AR1335_READOUT_MODE_4208_3120_0 = 0,  /*   13Mp30: native, native, 1104Mbps */ /* 4:3 */
    AR1335_READOUT_MODE_4208_2368_0,      /*   4K+p30: native, native, 1104Mbps */
    AR1335_READOUT_MODE_4208_1560_0,      /*  1560p30: native, binned, 1104Mbps */ /* 4:3 */
    AR1335_READOUT_MODE_4096_2880_0,      /*    4Kp30: native, native, 1104Mbps */ /* 4:3 */
    AR1335_READOUT_MODE_4096_2160_0,      /*    4Kp30: native, native, 1104Mbps */
    AR1335_READOUT_MODE_4096_1440_0,      /*  1080p60: native, binned, 1104Mbps */ /* 4:3 */
    AR1335_READOUT_MODE_4096_1080_0,      /*  1080p60: native, binned, 1104Mbps */
    AR1335_READOUT_MODE_4096_1080_1,      /*  1080P90: native, binned, 1134Mbps */
    AR1335_READOUT_MODE_2104_1560_0,      /*  1560p30:  16/32, binned, 1104Mbps */ /* 4:3 */

    /* medium fov */
    AR1335_READOUT_MODE_3168_2376_0,      /* 2.7K+p30: native, native, 1104Mbps */ /* 4:3 */
    AR1335_READOUT_MODE_3168_1782_0,      /* 2.7K+p30: native, native, 1104Mbps */
    AR1335_READOUT_MODE_2880_1620_1,      /*  2.7Kp60: native, native, 1134Mbps */

    /* narrow fov */
    AR1335_READOUT_MODE_2080_1560_0,      /*  1080p60: native, native, 1104Mbps */ /* 4:3 */
    AR1335_READOUT_MODE_2104_1284_0,      /*  1080p60: natibe, native, 1104Mbps */

    AR1335_NUM_READOUT_MODE,
} AR1335_READOUT_MODE_e;

typedef struct _AR1335_INIT_REG_s_ {
    UINT16  Addr;
    UINT16  Data;
} AR1335_INIT_REG_s;

typedef struct _AR1335_MODE_REG_s_ {
    UINT16  Addr;
    UINT16  Data[AR1335_NUM_READOUT_MODE];
} AR1335_MODE_REG_s;

typedef struct _AR1335_FRAME_TIMING_s_ {
    UINT32  InputClk;                       /* Sensor side input clock frequency */
    UINT32  LineLengthPck;                  /* number of pixel clock cycles per line of frame */
    UINT32  FrameLengthLines;               /* number of lines per frame */
    AMBA_DSP_FRAME_RATE_s   FrameRate;      /* framerate value of this sensor mode */
} AR1335_FRAME_TIMING_s;

typedef struct _AR1335_MODE_INFO_s_ {
    AR1335_READOUT_MODE_e   ReadoutMode;
    AR1335_FRAME_TIMING_s   FrameTiming;
} AR1335_MODE_INFO_s;

typedef struct _AR1335_CTRL_s_ {
    AMBA_SENSOR_STATUS_INFO_s   Status;
    AR1335_FRAME_TIMING_s       FrameTime;
    UINT32                      ShutterCtrl;
    UINT32                      CurrentAgc;
    UINT32                      CurrentDgc;
} AR1335_CTRL_s;

/*-----------------------------------------------------------------------------------------------*\
 *  Application Mode Definition
\*-----------------------------------------------------------------------------------------------*/
typedef enum _AMBA_SENSOR_AR1335_MODE_ID_e_ {
    /* wide fov */
    AMBA_SENSOR_AR1335_4208_3120_30P = 0,  /*   13Mp30: native, native */ /* 4:3 */
    AMBA_SENSOR_AR1335_4208_2368_30P,      /*   4K+p30: native, native */
    AMBA_SENSOR_AR1335_4208_1560_30P,      /*  1560p30: native, binned */ /* 4:3 */
    AMBA_SENSOR_AR1335_4096_2880_30P,      /*    4kp30: native, native */ /* 4:3 */
    AMBA_SENSOR_AR1335_4096_2160_30P,      /*    4Kp30: native, native */
    AMBA_SENSOR_AR1335_4096_1440_60P,      /*  1080p60: native, binned */ /* 4:3 */
    AMBA_SENSOR_AR1335_4096_1080_60P,      /*  1080p60: native, binned */
    AMBA_SENSOR_AR1335_4096_1080_90P,      /*  1080P90: native, binned */
    AMBA_SENSOR_AR1335_2104_1560_30P,      /*  1560p30:  16/32, binned */ /* 4:3 */

    /* medium fov */
    AMBA_SENSOR_AR1335_3168_2376_30P,      /* 2.7K+p30: native, native */ /* 4:3 */
    AMBA_SENSOR_AR1335_3168_1782_30P,      /* 2.7K+p30: native, native */
    AMBA_SENSOR_AR1335_2880_1620_30P,      /*  2.7kp30: native, native */
    AMBA_SENSOR_AR1335_2880_1620_60P,      /*  2.7Kp60: native, native */

    /* narrow fov */
    AMBA_SENSOR_AR1335_2080_1560_30P,      /*  1080p30: native, native */ /* 4:3 */
    AMBA_SENSOR_AR1335_2080_1560_60P,      /*  1080p60: native, native */ /* 4:3 */
    AMBA_SENSOR_AR1335_2104_1284_30P,      /*  1080p30: native, native */
    AMBA_SENSOR_AR1335_2104_1284_60P,      /*  1080p60: native, native */

    AMBA_SENSOR_AR1335_NUM_MODE,
} AMBA_SENSOR_AR1335_MODE_ID_e;

/*-----------------------------------------------------------------------------------------------*\
 *  Defined in AmbaSensor_AR1335.c
\*-----------------------------------------------------------------------------------------------*/
extern const AMBA_SENSOR_DEVICE_INFO_s AR1335DeviceInfo;
extern const AMBA_SENSOR_INPUT_INFO_s AR1335InputInfoNormalReadout[AR1335_NUM_READOUT_MODE];
extern const AMBA_SENSOR_INPUT_INFO_s AR1335InputInfoInversionReadout[AR1335_NUM_READOUT_MODE];
extern const AMBA_SENSOR_OUTPUT_INFO_s AR1335OutputInfo[AR1335_NUM_READOUT_MODE];
extern const AR1335_MODE_INFO_s AR1335ModeInfoList[AMBA_SENSOR_AR1335_NUM_MODE];

extern AR1335_MODE_REG_s AR1335ModeRegTable[AR1335_NUM_READOUT_MODE_REG];
extern AR1335_INIT_REG_s AR1335InitRegTable[];

#endif /* _AMBA_SENSOR_AR1335_H_ */
