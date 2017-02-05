/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_IMX179.h
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of SONY IMX179 CMOS sensor with MIPI interface
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_SENSOR_IMX179_H_
#define _AMBA_SENSOR_IMX179_H_

/*-----------------------------------------------------------------------------------------------*\
 * Control registers
\*-----------------------------------------------------------------------------------------------*/
#define IMX179_NUM_READOUT_MODE_REG    58

typedef enum _IMX179_CTRL_REG_e_ {
    IMX179_MODE_SELECT_REG            = 0x0100,
    IMX179_IMG_ORIENTATION_REG        = 0x0101,
    IMX179_COARSE_INTEG_TIME_MSB_REG  = 0x0202,
    IMX179_COARSE_INTEG_TIME_LSB_REG  = 0x0203,
    IMX179_ANA_GAIN_GLOBAL_REG        = 0x0205,
    IMX179_DIG_GAIN_GR_MSB_REG        = 0x020e,
    IMX179_DIG_GAIN_GR_LSB_REG        = 0x020f,
    IMX179_DIG_GAIN_R_MSB_REG         = 0x0210,
    IMX179_DIG_GAIN_R_LSB_REG         = 0x0211,
    IMX179_DIG_GAIN_B_MSB_REG         = 0x0212,
    IMX179_DIG_GAIN_B_LSB_REG         = 0x0213,
    IMX179_DIG_GAIN_GB_MSB_REG        = 0x0214,
    IMX179_DIG_GAIN_GB_LSB_REG        = 0x0215,
    IMX179_FRM_LENGTH_MSB_REG         = 0x0340,
    IMX179_FRM_LENGTH_LSB_REG         = 0x0341,
    IMX179_LINE_LENGTH_MSB_REG        = 0x0342,
    IMX179_LINE_LENGTH_LSB_REG        = 0x0343,
} IMX179_CTRL_REG_e;

typedef enum _IMX179_READOUT_MODE_e_ {
    IMX179_READOUT_MODE_0 = 0, /* PLL0, 10bit (4-lane), native (3200x1800 @ 30fps), up to 40fps */
    IMX179_READOUT_MODE_1,     /* PLL1, 10bit (4-lane), native (3200x1800 @ 30fps), lower power */
    IMX179_READOUT_MODE_2,     /* PLL0, 10bit (4-lane), native (1920x1080 @ 60fps) */
    IMX179_READOUT_MODE_3,     /* PLL0, 10bit (4-lane), native (3200x2400 @ 30fps) */
    IMX179_READOUT_MODE_4,     /* PLL0, 10bit (4-lane), native (2112x1188 @ 60fps) */
    IMX179_READOUT_MODE_5,     /* PLL0, 10bit (4-lane), 2x-bin (1600x900 @ 60fps) */
    IMX179_READOUT_MODE_6,     /* PLL2, 10bit (4-lane), 2x-bin (1600x1200 @ 30fps) */
    IMX179_READOUT_MODE_7,     /* PLL3, 10bit (2-lane), 2x-bin (1600x1200 @ 30fps) */
    IMX179_READOUT_MODE_8,     /* PLL0, 10bit (4-lane), 2x-bin (1600x1200 @ 60fps) */
    IMX179_READOUT_MODE_9,     /* PLL4, 10bit (4-lane), 4x-bin (800x450 @ 120fps) */

    IMX179_NUM_READOUT_MODE,
} IMX179_READOUT_MODE_e;

typedef struct _IMX179_PSTMG_REG_s_ {
    UINT16  Addr;
    UINT8   Data;
} IMX179_PSTMG_REG_s;

typedef struct _IMX179_MODE_REG_s_ {
    UINT16  Addr;
    UINT8   Data[IMX179_NUM_READOUT_MODE];
} IMX179_MODE_REG_s;

typedef struct _IMX179_FRAME_TIMING_s_ {
    UINT32  InputClk;                       /* Sensor side input clock frequency */
    UINT32  LineLengthPck;                  /* number of pixel clock cycles per line of frame */
    UINT32  FrameLengthLines;               /* number of lines per frame */
    AMBA_DSP_FRAME_RATE_s   FrameRate;      /* framerate value of this sensor mode */
} IMX179_FRAME_TIMING_s;

typedef struct _IMX179_MODE_INFO_s_ {
    IMX179_READOUT_MODE_e   ReadoutMode;
    IMX179_FRAME_TIMING_s   FrameTiming;
} IMX179_MODE_INFO_s;

typedef struct _IMX179_CTRL_s_ {
    AMBA_SENSOR_STATUS_INFO_s   Status;
    IMX179_FRAME_TIMING_s       FrameTime;
    UINT32                      ShutterCtrl;
    UINT32                      CurrentAgc;
    UINT32                      CurrentDgc;
} IMX179_CTRL_s;

/*-----------------------------------------------------------------------------------------------*\
 *  Application Mode Definition
\*-----------------------------------------------------------------------------------------------*/
typedef enum _AMBA_SENSOR_IMX179_MODE_ID_e_ {
    AMBA_SENSOR_IMX179_S1_3200_1800_30P = 0,
    AMBA_SENSOR_IMX179_S1_3200_1800_30P_L,
    AMBA_SENSOR_IMX179_S1_1920_1080_60P,
    AMBA_SENSOR_IMX179_S1_3200_2400_30P,
    AMBA_SENSOR_IMX179_S1_2112_1188_60P,
    AMBA_SENSOR_IMX179_S2_1600_900_60P,
    AMBA_SENSOR_IMX179_S2_1600_1200_30P,
    AMBA_SENSOR_IMX179_S2_1600_1200_30P_L,
    AMBA_SENSOR_IMX179_S2_1600_1200_60P,
    AMBA_SENSOR_IMX179_S4_800_450_120P,

    AMBA_SENSOR_IMX179_NUM_MODE,
} AMBA_SENSOR_IMX179_MODE_ID_e;

/*-----------------------------------------------------------------------------------------------*\
 *  Defined in AmbaSensor_IMX179.c
\*-----------------------------------------------------------------------------------------------*/
extern const AMBA_SENSOR_DEVICE_INFO_s IMX179DeviceInfo;
extern const AMBA_SENSOR_INPUT_INFO_s IMX179InputInfoNormalReadout[IMX179_NUM_READOUT_MODE];
extern const AMBA_SENSOR_INPUT_INFO_s IMX179InputInfoInversionReadout[IMX179_NUM_READOUT_MODE];
extern const AMBA_SENSOR_OUTPUT_INFO_s IMX179OutputInfo[IMX179_NUM_READOUT_MODE];
extern const IMX179_MODE_INFO_s IMX179ModeInfoList[AMBA_SENSOR_IMX179_NUM_MODE];

extern IMX179_MODE_REG_s IMX179ModeRegTable[IMX179_NUM_READOUT_MODE_REG];

#endif /* _AMBA_SENSOR_IMX179_H_ */
