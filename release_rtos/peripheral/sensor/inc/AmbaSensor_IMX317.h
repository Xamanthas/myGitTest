/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_IMX317.h
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of SONY IMX317 CMOS sensor with LVDS interface
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_SENSOR_IMX317_H_
#define _AMBA_SENSOR_IMX317_H_

/*-----------------------------------------------------------------------------------------------*\
 * Control registers
\*-----------------------------------------------------------------------------------------------*/
#define IMX317_NUM_READOUT_PSTMG_REG   25
#define IMX317_NUM_READOUT_MODE_REG    27

typedef enum _IMX317_CTRL_REG_e_ {
    IMX317_OP_REG                = 0x0000,
    IMX317_CLPSQRST_REG          = 0x0001,
    IMX317_DCKRST_REG            = 0x0009,
    IMX317_PGC_LSB_REG           = 0x000a,
    IMX317_PGC_MSB_REG           = 0x000b,
    IMX317_SHR_LSB_REG           = 0x000c,
    IMX317_SHR_MSB_REG           = 0x000d,
    IMX317_SVR_LSB_REG           = 0x000e,
    IMX317_SVR_MSB_REG           = 0x000f,
    IMX317_DGAIN_REG             = 0x0012,
    IMX317_MDVREV_REG            = 0x001a,
    IMX317_SYS_MODE_REG          = 0x003e,
    IMX317_PSMOVEN_REG           = 0x00ee,
    IMX317_PLL_CKEN_REG          = 0x00e6,
    IMX317_PACKEN_REG            = 0x00e8,
    IMX317_STBPL_IF_AD_REG       = 0x010b,
    IMX317_PLRD1_LSB_REG         = 0x0120,
    IMX317_PLRD1_MSB_REG         = 0x0121,
    IMX317_PLRD2_REG             = 0x0122,
    IMX317_PLRD3_REG             = 0x0129,
    IMX317_PLRD4_REG             = 0x012a,
    IMX317_PLRD5_REG             = 0x012d,
    IMX317_PSLVDS1_LSB_REG       = 0x032c,
    IMX317_PSLVDS1_MSB_REG       = 0x032d,
    IMX317_PSLVDS2_LSB_REG       = 0x034a,
    IMX317_PSLVDS2_MSB_REG       = 0x034b,
    IMX317_PSLVDS3_LSB_REG       = 0x05b6,
    IMX317_PSLVDS3_MSB_REG       = 0x05b7,
    IMX317_PSLVDS4_LSB_REG       = 0x05b8,
    IMX317_PSLVDS4_MSB_REG       = 0x05b9,
} IMX317_CTRL_REG_e;

typedef enum _IMX317_READOUT_MODE_e_ {
    IMX317_TYPE_2_5_MODE_C = 0, /* 12bit A/D, 12bit OUT, 4-lane, native (for calibration) */
    IMX317_TYPE_2_5_MODE_0,     /* 12bit A/D, 12bit OUT, 4-lane, native */
    IMX317_TYPE_2_5_MODE_1,     /* 10bit A/D, 10bit OUT, 4-lane, native */
    IMX317_TYPE_2_5_MODE_2,     /* 12bit A/D, 12bit OUT, 4-lane, H: 2binning V: 2binning */
    IMX317_TYPE_2_5_MODE_3,     /* 10bit A/D, 10bit OUT, 4-lane, H: 2binning V: 2binning */
    IMX317_TYPE_2_5_MODE_4,     /* 10bit A/D, 10bit OUT, 4-lane, H: 2binning V: 2binning */
    IMX317_TYPE_2_5_MODE_5,     /* 10bit A/D, 10bit OUT, 4-lane, H: 3binning V: 2/3subsampling */
    IMX317_TYPE_2_5_MODE_6,     /* 10bit A/D, 10bit OUT, 4-lane, H: 3binning V: 2/8subsampling */

    IMX317_NUM_READOUT_MODE,
} IMX317_READOUT_MODE_e;

typedef enum _IMX317_LVDS_CHANNEL_SELECT_e_ {
    IMX317_LVDS_10CH = 0,
    IMX317_LVDS_8CH,
    IMX317_LVDS_6CH,
    IMX317_LVDS_4CH,
    IMX317_LVDS_2CH,
    IMX317_LVDS_1CH
} IMX317_LVDS_CHANNEL_SELECT_e;

typedef struct _IMX317_PSTMG_REG_s_ {
    UINT16  Addr;
    UINT8   Data;
} IMX317_PSTMG_REG_s;

typedef struct _IMX317_MODE_REG_s_ {
    UINT16  Addr;
    UINT8   Data[IMX317_NUM_READOUT_MODE];
} IMX317_MODE_REG_s;

typedef struct _IMX317_FRAME_TIMING_s_ {
    UINT32  InputClk;                       /* Sensor side input clock frequency */
    UINT32  NumTickPerXhs;                  /* XHS period (in input clock cycles) */
    UINT32  NumXhsPerH;                     /* horizontal operating period (in number of XHS pulses) */
    UINT32  NumXhsPerV;                     /* vertical operating period (in number of XHS pulses) */
    UINT32  NumXvsPerV;                     /* vertical operating period (in number of XVS pulses) */
    AMBA_DSP_FRAME_RATE_s   FrameRate;      /* framerate value of this sensor mode */
} IMX317_FRAME_TIMING_s;

typedef struct _IMX317_MODE_INFO_s_ {
    IMX317_READOUT_MODE_e   ReadoutMode;
    IMX317_FRAME_TIMING_s   FrameTiming;
} IMX317_MODE_INFO_s;

typedef struct _IMX317_CTRL_s_ {
    AMBA_SENSOR_STATUS_INFO_s   Status;
    IMX317_FRAME_TIMING_s       FrameTime;
    UINT32                      CurrentAgc;
    UINT32                      CurrentDgc;
    UINT32                      CurrentShrCtrlSVR;
    UINT32                      CurrentShrCtrlSHR;
} IMX317_CTRL_s;

/*-----------------------------------------------------------------------------------------------*\
 *  Application Mode Definition
\*-----------------------------------------------------------------------------------------------*/
typedef enum _AMBA_SENSOR_IMX317_MODE_ID_e_ {
    AMBA_SENSOR_IMX317_TYPE_2_5_MODE_C_30P = 0, /* customized mode for calibration */
    AMBA_SENSOR_IMX317_TYPE_2_5_MODE_0_30P,
    AMBA_SENSOR_IMX317_TYPE_2_5_MODE_1_60P,
    AMBA_SENSOR_IMX317_TYPE_2_5_MODE_2_60P,
    AMBA_SENSOR_IMX317_TYPE_2_5_MODE_3_120P,
    AMBA_SENSOR_IMX317_TYPE_2_5_MODE_4_30P,
    AMBA_SENSOR_IMX317_TYPE_2_5_MODE_5_120P,
    AMBA_SENSOR_IMX317_TYPE_2_5_MODE_5_60P,
    AMBA_SENSOR_IMX317_TYPE_2_5_MODE_6_240P,
    AMBA_SENSOR_IMX317_TYPE_2_5_MODE_6_30P,

    AMBA_SENSOR_IMX317_NUM_MODE,
} AMBA_SENSOR_IMX317_MODE_ID_e;

/*-----------------------------------------------------------------------------------------------*\
 *  Defined in AmbaSensor_IMX317.c
\*-----------------------------------------------------------------------------------------------*/
extern const AMBA_SENSOR_DEVICE_INFO_s IMX317DeviceInfo;
extern const AMBA_SENSOR_INPUT_INFO_s IMX317InputInfoNormalReadout[IMX317_NUM_READOUT_MODE];
extern const AMBA_SENSOR_INPUT_INFO_s IMX317InputInfoInversionReadout[IMX317_NUM_READOUT_MODE];
extern const AMBA_SENSOR_OUTPUT_INFO_s IMX317OutputInfo[IMX317_NUM_READOUT_MODE];
extern const IMX317_MODE_INFO_s IMX317ModeInfoList[AMBA_SENSOR_IMX317_NUM_MODE];

extern IMX317_PSTMG_REG_s IMX317PlstmgRegTable[IMX317_NUM_READOUT_PSTMG_REG];
extern IMX317_MODE_REG_s IMX317ModeRegTable[IMX317_NUM_READOUT_MODE_REG];

#endif /* _AMBA_SENSOR_IMX317_H_ */
