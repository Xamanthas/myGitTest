/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_IMX377.h
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of SONY IMX377 CMOS sensor with MIPI interface
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_SENSOR_IMX377_H_
#define _AMBA_SENSOR_IMX377_H_

/*-----------------------------------------------------------------------------------------------*\
 * Control registers
\*-----------------------------------------------------------------------------------------------*/
#define IMX377_NUM_READOUT_PSTMG_REG   62
#define IMX377_NUM_READOUT_MODE_REG    62

typedef enum _IMX377_CTRL_REG_e_ {
    IMX377_OP_REG                = 0x3000,
    IMX377_CLPSQRST_REG          = 0x3001,
    IMX377_MDSEL1_REG            = 0x3004,
    IMX377_MDSEL2_REG            = 0x3005,
    IMX377_MDSEL3_REG            = 0x3006,
    IMX377_MDSEL4_REG            = 0x3007,
    IMX377_SMD_REG               = 0x3008,
    IMX377_PGC_LSB_REG           = 0x3009,
    IMX377_PGC_MSB_REG           = 0x300a,
    IMX377_SHR_LSB_REG           = 0x300b,
    IMX377_SHR_MSB_REG           = 0x300c,
    IMX377_SVR_LSB_REG           = 0x300d,
    IMX377_SVR_MSB_REG           = 0x300e,
    IMX377_DGAIN_REG             = 0x3011,
    IMX377_SYNCDRV_REG           = 0x3018,
    IMX377_MDVREV_REG            = 0x301a,
    IMX377_SYS_MODE_REG          = 0x303d,
    IMX377_XMSTA_REG             = 0x30f4,
    IMX377_HMAX_LSB_REG          = 0x30f5,
    IMX377_HMAX_MSB_REG          = 0x30f6,
    IMX377_VMAX_LSB_REG          = 0x30f7,
    IMX377_VMAX_MSB_REG          = 0x30f8,
    IMX377_VMAX_HSB_REG          = 0x30f9,
    IMX377_STBPL_IF_AD_REG       = 0x310b,
    IMX377_PLRD1_LSB_REG         = 0x3120,
    IMX377_PLRD1_MSB_REG         = 0x3121,
    IMX377_PLRD3_REG             = 0x3122,
    IMX377_PLRD10_REG            = 0x3123,
    IMX377_PLRD11_REG            = 0x3124,
    IMX377_PLRD12_REG            = 0x3125,
    IMX377_PLRD13_REG            = 0x3127,
    IMX377_PLRD2_REG             = 0x3129,
    IMX377_PLRD4_REG             = 0x312a,
    IMX377_PLRD14_REG            = 0x312d,
    IMX377_LMRSVRG_REG           = 0x3399,
    IMX377_MIPIEBD_TAGEN_REG     = 0x3a56,
    IMX377_MIPI_HALF_EN_REG      = 0x3ac4,
} IMX377_CTRL_REG_e;

typedef enum _IMX377_READOUT_MODE_e_ {
    IMX377_TYPE_2_3_MODE_0A = 0, /* 12bit A/D, 12bit OUT, 4-lane, native (4096x3000) */
    IMX377_TYPE_2_3_MODE_0,      /* 12bit A/D, 12bit OUT, 4-lane, native */
    IMX377_TYPE_2_3_MODE_1,      /* 10bit A/D, 10bit OUT, 4-lane, native */
    IMX377_TYPE_2_3_MODE_1A,     /* 10bit A/D, 10bit OUT, 4-lane, native */
    IMX377_TYPE_2_3_MODE_2,      /* 10bit A/D, 12bit OUT, 4-lane, H: 2binning V: 2binning */
    IMX377_TYPE_2_3_MODE_3,      /* 10bit A/D, 12bit OUT, 4-lane, H: 3binning V: 3binning */
    IMX377_TYPE_2_3_MODE_4,      /* 10bit A/D, 12bit OUT, 4-lane, H: 3binning V: 1/3subsampling */
    IMX377_TYPE_2_5_MODE_0,      /* 12bit A/D, 12bit OUT, 4-lane, native */
    IMX377_TYPE_2_5_MODE_1,      /* 10bit A/D, 10bit OUT, 4-lane, native */
    IMX377_TYPE_2_5_MODE_1A,     /* 10bit A/D, 10bit OUT, 4-lane, native */
    IMX377_TYPE_2_5_MODE_2,      /* 10bit A/D, 12bit OUT, 4-lane, H: 2binning V: 2binning */
    IMX377_TYPE_2_5_MODE_4,      /* 10bit A/D, 12bit OUT, 4-lane, H: 3binning V: 1/3subsampling */

    IMX377_NUM_READOUT_MODE,
} IMX377_READOUT_MODE_e;

typedef struct _IMX377_PSTMG_REG_s_ {
    UINT16  Addr;
    UINT8   Data;
} IMX377_PSTMG_REG_s;

typedef struct _IMX377_MODE_REG_s_ {
    UINT16  Addr;
    UINT8   Data[IMX377_NUM_READOUT_MODE];
} IMX377_MODE_REG_s;

typedef struct _IMX377_FRAME_TIMING_s_ {
    UINT32  InputClk;                       /* Sensor side input clock frequency */
    UINT32  RefClkMul;                      /* RefClk = InputClk * RefClkMul */
    UINT32  NumTickPerXhs;                  /* XHS period (in input clock cycles) */
    UINT32  NumXhsPerH;                     /* horizontal operating period (in number of XHS pulses) */
    UINT32  NumXhsPerV;                     /* vertical operating period (in number of XHS pulses) */
    UINT32  NumXvsPerV;                     /* vertical operating period (in number of XVS pulses) */
    AMBA_DSP_FRAME_RATE_s   FrameRate;      /* framerate value of this sensor mode */
} IMX377_FRAME_TIMING_s;

typedef struct _IMX377_MODE_INFO_s_ {
    IMX377_READOUT_MODE_e   ReadoutMode;
    IMX377_FRAME_TIMING_s   FrameTiming;
} IMX377_MODE_INFO_s;

typedef struct _IMX377_CTRL_s_ {
    AMBA_SENSOR_STATUS_INFO_s   Status;
    IMX377_FRAME_TIMING_s       FrameTime;
    UINT32                      CurrentAgc;
    UINT32                      CurrentDgc;
    UINT32                      CurrentShrCtrlSVR;
    UINT32                      CurrentShrCtrlSHR;
} IMX377_CTRL_s;

/*-----------------------------------------------------------------------------------------------*\
 *  Application Mode Definition
\*-----------------------------------------------------------------------------------------------*/
typedef enum _AMBA_SENSOR_IMX377_MODE_ID_e_ {
    AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0A_30P = 0,
    AMBA_SENSOR_IMX377_TYPE_2_3_MODE_0_35P,
    AMBA_SENSOR_IMX377_TYPE_2_3_MODE_1_40P,
    AMBA_SENSOR_IMX377_TYPE_2_3_MODE_1A_30P,
    AMBA_SENSOR_IMX377_TYPE_2_3_MODE_2_60P,
    AMBA_SENSOR_IMX377_TYPE_2_3_MODE_3_30P,
    AMBA_SENSOR_IMX377_TYPE_2_3_MODE_4_200P,
    AMBA_SENSOR_IMX377_TYPE_2_3_MODE_4_240P,
    AMBA_SENSOR_IMX377_TYPE_2_3_MODE_4_250P,
    AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_30P,
    AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_60P,
    AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_30P,
    AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1_24P,
    AMBA_SENSOR_IMX377_TYPE_2_5_MODE_1A_60P,
    AMBA_SENSOR_IMX377_TYPE_2_5_MODE_2_120P,
    AMBA_SENSOR_IMX377_TYPE_2_5_MODE_4_240P,
    AMBA_SENSOR_IMX377_TYPE_2_5_MODE_0_25P,

    AMBA_SENSOR_IMX377_NUM_MODE,
} AMBA_SENSOR_IMX377_MODE_ID_e;

/*-----------------------------------------------------------------------------------------------*\
 *  Defined in AmbaSensor_IMX377.c
\*-----------------------------------------------------------------------------------------------*/
extern const AMBA_SENSOR_DEVICE_INFO_s IMX377DeviceInfo;
extern const AMBA_SENSOR_INPUT_INFO_s IMX377InputInfoNormalReadout[IMX377_NUM_READOUT_MODE];
extern const AMBA_SENSOR_INPUT_INFO_s IMX377InputInfoInversionReadout[IMX377_NUM_READOUT_MODE];
extern const AMBA_SENSOR_OUTPUT_INFO_s IMX377OutputInfo[IMX377_NUM_READOUT_MODE];
extern const IMX377_MODE_INFO_s IMX377ModeInfoList[AMBA_SENSOR_IMX377_NUM_MODE];

extern IMX377_PSTMG_REG_s IMX377PlstmgRegTable[IMX377_NUM_READOUT_PSTMG_REG];
extern IMX377_MODE_REG_s IMX377ModeRegTable[IMX377_NUM_READOUT_MODE_REG];

#endif /* _AMBA_SENSOR_IMX377_H_ */
