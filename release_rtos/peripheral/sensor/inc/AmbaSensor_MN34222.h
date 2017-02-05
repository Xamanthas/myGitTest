/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_MN34222.h
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of Panasonic MN34222 CMOS sensor with LVDS interface
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_SENSOR_MN34222_H_
#define _AMBA_SENSOR_MN34222_H_

/*-----------------------------------------------------------------------------------------------*\
 * Control registers
\*-----------------------------------------------------------------------------------------------*/
#define MN34222_NUM_READOUT_MODE_REG         144

typedef struct _MN34222_FRAME_TIMING_s_ {
    UINT32  InputClk;                       /* Sensor side input clock frequency */
    UINT32  NumTickPerXhs;                  /* XHS period (in input clock cycles) */
    UINT32  NumXhsPerH;                     /* horizontal operating period (in number of XHS pulses) */
    UINT32  NumXhsPerV;                     /* vertical operating period (in number of XHS pulses) */
    AMBA_DSP_FRAME_RATE_s   FrameRate;      /* framerate value of this sensor mode */
} MN34222_FRAME_TIMING_s;

typedef struct _MN34222_CTRL_s_ {
    AMBA_SENSOR_STATUS_INFO_s   Status;
    MN34222_FRAME_TIMING_s      FrameTime; /* Fixed */
    AMBA_SENSOR_HDR_INFO_s      HdrInfo;   /* Fixed */
    UINT32                      CurrentAgcCtrl[2];      /* 0: long exposure frame, 1: short exposure frame */
    UINT32                      CurrentWbCtrl[2][4];    /* 0: long exposure frame, 1: short exposure frame (R, Gr, Gb, B) */
    UINT32                      CurrentShrCtrlSVR[2];   /* 0: long exposure frame, 1: short exposure frame */
    UINT32                      CurrentShrCtrlSHR[2];   /* 0: long exposure frame, 1: short exposure frame */
} MN34222_CTRL_s;

typedef enum _MN34222_READOUT_MODE_e_ {
    MN34222_S1_12_1944X1092_60P = 0,  /* full resolution */
    MN34222_S1_12_1944X1092_60P_TO_30P,
    MN34222_S1_12_1944X1092_30P,
    MN34222_S1_12_1944X1092_30P_HDR,
    MN34222_S1_12_1944X1092_60P_HDR,
    MN34222_NUM_READOUT_MODE,
} MN34222_READOUT_MODE_e;

typedef enum _MN34222_INPUT_MODE_ID_e_ {
    MN34222_INPUT_MODE_0 = 0,
    MN34222_NUM_INPUT_MODE,
} MN34222_INPUT_MODE_ID_e;

typedef enum _MN34222_OUTPUT_MODE_ID_e_ {
    MN34222_OUTPUT_MODE_0 = 0,
    MN34222_OUTPUT_MODE_1,
    MN34222_OUTPUT_MODE_2,
    MN34222_OUTPUT_MODE_3,
    MN34222_NUM_OUTPUT_MODE,
} MN34222_OUTPUT_MODE_ID_e;

typedef enum _MN34222_HDR_MODE_ID_e_ {
    MN34222_HDR_MODE_0 = 0,  /* Not support HDR */
    MN34222_HDR_MODE_1,
    MN34222_HDR_MODE_2,
    MN34222_NUM_HDR_MODE,
} MN34222_HDR_MODE_ID_e;

typedef enum _AMBA_SENSOR_MN34222_MODE_ID_e_ {
    AMBA_SENSOR_MN34222_V1_12_1944X1092_60P = 0,
    AMBA_SENSOR_MN34222_V1_12_1944X1092_60P_TO_30P,
    AMBA_SENSOR_MN34222_V1_12_1944X1092_30P,
    AMBA_SENSOR_MN34222_V1_12_1944X1092_30P_HDR,
    AMBA_SENSOR_MN34222_V1_12_1944X1092_60P_HDR,
    AMBA_SENSOR_MN34222_NUM_MODE,
} AMBA_SENSOR_MODE_ID_e;

typedef struct _MN34222_REG_s_ {
    UINT16  Addr;
    UINT16  Data[MN34222_NUM_READOUT_MODE];
} MN34222_REG_s;

typedef struct _MN34222_MODE_INFO_s_ {
    MN34222_READOUT_MODE_e      ReadoutMode;
    MN34222_INPUT_MODE_ID_e     InputMode;
    MN34222_OUTPUT_MODE_ID_e    OutputMode;
    MN34222_FRAME_TIMING_s      FrameTiming;
    MN34222_HDR_MODE_ID_e       HdrMode;
} MN34222_MODE_INFO_s;

/*-----------------------------------------------------------------------------------------------*\
 *  Defined in AmbaSensor_MN34222.c
\*-----------------------------------------------------------------------------------------------*/
extern const AMBA_SENSOR_DEVICE_INFO_s MN34222DeviceInfo;
extern const AMBA_SENSOR_INPUT_INFO_s MN34222InputInfo[MN34222_NUM_INPUT_MODE];
extern const AMBA_SENSOR_OUTPUT_INFO_s MN34222OutputInfo[MN34222_NUM_OUTPUT_MODE];
extern const MN34222_MODE_INFO_s MN34222ModeInfoList[AMBA_SENSOR_MN34222_NUM_MODE];
extern const AMBA_SENSOR_HDR_INFO_s MN34222HdrInfo[MN34222_NUM_HDR_MODE];
extern MN34222_REG_s MN34222RegTable[MN34222_NUM_READOUT_MODE_REG];

#endif /* _AMBA_SENSOR_MN34222_H_ */
