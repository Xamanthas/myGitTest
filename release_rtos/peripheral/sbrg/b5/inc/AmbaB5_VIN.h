/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaB5_VIN.h
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions & Constants for B5 VIN Control APIs
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_B5_VIN_H_
#define _AMBA_B5_VIN_H_

typedef enum _AMBA_B5_VIN_INTERFACE_e_ {
    AMBA_B5_VIN_LVDS,                               /* LVDS/SLVS/MLVS */
    AMBA_B5_VIN_DVP,                                /* Digital Video Port */
    AMBA_B5_VIN_MIPI,                               /* MIPI */

    AMBA_NUM_B5_VIN_INTERFACE                       /* Number of B5 VIN Interface */
} AMBA_B5_VIN_INTERFACE_e;

typedef struct _AMBA_B5_VIN_WINDOW_s_ {
    UINT16  StartX;                                /* Crop Start Col */
    UINT16  StartY;                                /* Crop Start Row */
    UINT16  EndX;                                  /* Crop End Col */
    UINT16  EndY;                                  /* Crop End Row */
} AMBA_B5_VIN_WINDOW_s;

/*-----------------------------------------------------------------------------------------------*\
 * VIN Trigger Pulse Configuration Structure
\*-----------------------------------------------------------------------------------------------*/
typedef enum _AMBA_B5_VIN_TRIGGER_PULSE_e_ {
    AMBA_B5_VIN_TRIGGER_PULSE0 = 0,                 /* VIN Trigger Pulse 0 */
    AMBA_B5_VIN_TRIGGER_PULSE1,                     /* VIN Trigger Pulse 1 */

    AMBA_B5_NUM_VIN_TRIGGER_PULSE                   /* Number of trigger pulses */
} AMBA_B5_VIN_TRIGGER_PULSE_e;

typedef enum _AMBA_B5_VIN_PULSE_POLARITY_e_ {
    AMBA_B5_VIN_PULSE_LOW = 0,                      /* Active pulse is low */
    AMBA_B5_VIN_PULSE_HIGH                          /* Active pulse is high */
} AMBA_B5_VIN_PULSE_POLARITY_e;

typedef struct _AMBA_B5_VIN_TRIGGER_PULSE_s_ {
    int    Enable;
    AMBA_B5_VIN_PULSE_POLARITY_e PulsePolarity;     /* Polarity of the trigger pulse */
    UINT16 PulseStartLine;                          /* Start line of the pulse */
    UINT16 PulseEndLine;                            /* End line of the pulse */
} AMBA_B5_VIN_TRIGGER_PULSE_s;

/*-----------------------------------------------------------------------------------------------*\
 * VIN to VOUT Synchronization Structure
\*-----------------------------------------------------------------------------------------------*/
typedef enum _AMBA_B5_VIN_VOUT_SYNC_e_ {
    AMBA_B5_VIN_VOUT_SYNC0 = 0,                     /* VIN-VOUT0 synchronization */
    AMBA_B5_VIN_VOUT_SYNC1,                         /* VIN-VOUT1 synchronization */

    AMBA_B5_NUM_VIN_VOUT_SYNC                       /* Number of VIN-VOUT synchronization */
} AMBA_B5_VIN_VOUT_SYNC_e;

typedef enum _AMBA_B5_VIN_VOUT_SYNC_TYPE_e_ {
    AMBA_B5_VIN_VOUT_SYNC_FRAME = 0,                /* Send synchronization signal on even field */
    AMBA_B5_VIN_VOUT_SYNC_FIELD                     /* Send synchronization signal on every field */
} AMBA_B5_VIN_VOUT_SYNC_TYPE_e;

typedef struct _AMBA_B5_VIN_VOUT_SYNC_s_ {
    AMBA_B5_VIN_VOUT_SYNC_TYPE_e SignalFreq;        /* Send sync signal every frame/every field */
    UINT32 SignalLine;                              /* Start active line to send synchronization signal to VOUT */
} AMBA_B5_VIN_VOUT_SYNC_s;

typedef enum _AMBA_B5_VIN_DVP_TYPE_e_ {
    AMBA_B5_VIN_DVP_SINGLE_PEL_SDR = 0,             /* Video data over Digital Video Port (DVP) interface */
    AMBA_B5_VIN_DVP_SINGLE_PEL_DDR,                 /* Video data over Digital Video Port (DVP) interface */
    AMBA_B5_VIN_DVP_DOUBLE_PEL_SDR,                 /* Video data over Digital Video Port (DVP) interface */
    AMBA_B5_VIN_DVP_DOUBLE_PEL_DDR                  /* Video data over Digital Video Port (DVP) interface */
} AMBA_B5_VIN_DVP_TYPE_e;

typedef enum _AMBA_B5_VIN_SYNC_TYPE_e_ {
    AMBA_B5_VIN_SYNC_BT601 = 0,                     /* No embedded sync code */
    AMBA_B5_VIN_SYNC_BT656_LOWER_PEL,               /* Embedded sync code carried on lower pixel only */
    AMBA_B5_VIN_SYNC_BT656_UPPER_PEL,               /* Embedded sync code carried on upper pixel only */
    AMBA_B5_VIN_SYNC_BT656_BOTH_PEL,                /* Embedded sync code carried on both pixels */
} AMBA_B5_VIN_SYNC_TYPE_e;

typedef enum _AMBA_B5_VIN_SIGNAL_EDGE_TYPE_e_ {
    AMBA_B5_VIN_SIGNAL_RISING_EDGE = 0,             /* Rising edge. A signal transition from low to high (0 to 1) */
    AMBA_B5_VIN_SIGNAL_FALLING_EDGE                 /* Falling edge. A signal transition from high to low (1 to 0) */
} AMBA_B5_VIN_SIGNAL_EDGE_TYPE_e;

typedef struct _AMBA_B5_VIN_RX_HV_SYNC_s_ {
    UINT32  NumActivePixels;                        /* Horizontal active region width */
    UINT32  NumActiveLines;                         /* Vertical active region height */
    UINT32  NumTotalPixels;                         /* Horizontal total width (line_length_pck) */
    UINT32  NumTotalLines;                          /* Vertical total height (frame_length_lines) */
} AMBA_B5_VIN_RX_HV_SYNC_s;

typedef struct _AMBA_B5_VIN_TX_HV_SYNC_s_ {
    UINT8   Enable;                                 /* 1 - Enable H/V Sync output to sensor */
    UINT32  NumActivePixels;                        /* Horizontal active region width */
    UINT32  NumActiveLines;                         /* Vertical active region height */
    UINT32  NumTotalPixels;                         /* Horizontal total width (line_length_pck) */
    UINT32  NumTotalLines;                          /* Vertical total height (frame_length_lines) */
    UINT32  VsyncStartPixel;                        /* Vsync start pixel offset from the end of Hsync */
    UINT32  VsyncEndPixel;                          /* Vsync end pixel offset from the end of Hsync */
} AMBA_B5_VIN_TX_HV_SYNC_s;

typedef struct _AMBA_B5_VIN_SLVS_SYNC_CODE_s_ {
    UINT8    SyncInterleaving;   /* 0=none; 1=2-lane interleaving; 2=4-lane interleaving */
    UINT8    ITU656Type;         /* 1= ITU-656 type, don't care CustomSyncCode structure */
    struct {
        UINT8   PatternAlign;   /* Sync code mask/patterns are: 0=LSB aligned; 1=MSB aligned */
        UINT16  SyncCodeMask;   /* Sync Code Mask */

        struct {
            UINT8    Sol:    1;  /* 1 = Enable detection of SOL sync codes */
            UINT8    Eol:    1;  /* 1 = Enable detection of EOL sync codes */
            UINT8    Sof:    1;  /* 1 = Enable detection of SOF sync codes */
            UINT8    Eof:    1;  /* 1 = Enable detection of EOF sync codes */
            UINT8    Sov:    1;  /* 1 = Enable detection of SOV sync codes */
            UINT8    Eov:    1;  /* 1 = Enable detection of EOV sync codes */
        } DetectEnable;

        UINT16  PatternSol;      /* Start of active line */
        UINT16  PatternEol;      /* End of active line */
        UINT16  PatternSof;      /* Start of frame */
        UINT16  PatternEof;      /* End of frame */
        UINT16  PatternSov;      /* Start of vertical blanking line */
        UINT16  PatternEov;      /* End of vertical blanking line */
    } CustomSyncCode;

} AMBA_B5_VIN_SLVS_SYNC_CODE_s;

typedef struct _AMBA_B5_VIN_SPILT_CTRL_s_ {
        UINT8   NumSplits;                          /* number of splits to make */
        UINT16  SplitWidth;                         /* Split each input line to lines of SPLIT_WIDTH pixels(including intermediate HBLANK) */
} AMBA_B5_VIN_SPILT_CTRL_s;

typedef struct _AMBA_B5_VIN_MIPI_DPHY_CONFIG_s_ {
    UINT8   HsSettleTime;                           /* D-PHY HS-SETTLE time */
    UINT8   HsTermTime;                             /* D-PHY HS-TERM time */
    UINT8   ClkSettleTime;                          /* D-PHY CLK-SETTLE time */
    UINT8   ClkTermTime;                            /* D-PHY CLK-TERM time */
    UINT8   ClkMissTime;                            /* D-PHY CLK-MISS time */
    UINT8   RxInitTime;                             /* D-PHY RX-INIT time */
} AMBA_B5_VIN_MIPI_DPHY_CONFIG_s;

typedef struct _AMBA_B5_VIN_DVP_CONFIG_s_ {
    UINT8   NumDataBits;                /* Bit depth of pixel data */
    AMBA_B5_VIN_DVP_TYPE_e              DvpType;        /* Type of parallel transmission (DVP) */
    AMBA_B5_VIN_SYNC_TYPE_e             SyncType;       /* BT.601/BT.656 line sync and frame sync */
    AMBA_B5_VIN_SIGNAL_EDGE_TYPE_e      DataClockEdge;  /* Data are valid on rising/falling clock edge */
    AMBA_B5_VIN_SIGNAL_EDGE_TYPE_e      HsyncPolarity;  /* Leading edge of H-sync/line-sync pulse */
    AMBA_B5_VIN_SIGNAL_EDGE_TYPE_e      VsyncPolarity;  /* Leading edge of V-sync/frame-sync pulse */
    AMBA_B5_VIN_SIGNAL_EDGE_TYPE_e      FieldPolarity;  /* Leading edge of field pulse */
    AMBA_B5_VIN_RX_HV_SYNC_s            RxHvSyncCtrl;   /* Input H/V sync signal format (from sensor) */
    AMBA_B5_VIN_TRIGGER_PULSE_s         VinTrigPulse[AMBA_B5_NUM_VIN_TRIGGER_PULSE];
    AMBA_B5_VIN_VOUT_SYNC_s             VinVoutSync[AMBA_B5_NUM_VIN_VOUT_SYNC];
} AMBA_B5_VIN_DVP_CONFIG_s;

typedef struct _AMBA_B5_VIN_SLVS_CONFIG_s_ {
    UINT8   NumDataBits;                /* Bit depth of pixel data */
    UINT8   NumDataLane;                /* Number of active data lanes */
    UINT8   DataLaneSelect[4];          /* Logical to Physical Lane Mapping */
    AMBA_B5_VIN_SLVS_SYNC_CODE_s        SyncDetectCtrl; /* Sync code detection control */
    AMBA_B5_VIN_RX_HV_SYNC_s            RxHvSyncCtrl;   /* Input H/V sync signal format (from sensor) */
    AMBA_B5_VIN_TX_HV_SYNC_s            TxHvSyncCtrl;   /* Output H/V sync signal format (to sensor) */
    AMBA_B5_VIN_SPILT_CTRL_s            SplitCtrl;      /* Vin split function control */
    AMBA_B5_VIN_TRIGGER_PULSE_s         VinTrigPulse[AMBA_B5_NUM_VIN_TRIGGER_PULSE];
    AMBA_B5_VIN_VOUT_SYNC_s             VinVoutSync[AMBA_B5_NUM_VIN_VOUT_SYNC];
} AMBA_B5_VIN_SLVS_CONFIG_s;

typedef struct _AMBA_B5_VIN_MIPI_CONFIG_s_ {
    UINT8   NumDataBits;                /* Bit depth of pixel data */
    UINT8   NumDataLane;                /* Number of active data lanes */
    AMBA_B5_VIN_RX_HV_SYNC_s            RxHvSyncCtrl;   /* Input H/V sync signal format (from sensor) */
    AMBA_B5_VIN_TX_HV_SYNC_s            TxHvSyncCtrl;   /* Output H/V sync signal format (to sensor) */
    AMBA_B5_VIN_TRIGGER_PULSE_s         VinTrigPulse[AMBA_B5_NUM_VIN_TRIGGER_PULSE];
    AMBA_B5_VIN_VOUT_SYNC_s             VinVoutSync[AMBA_B5_NUM_VIN_VOUT_SYNC];
} AMBA_B5_VIN_MIPI_CONFIG_s;

/*---------------------------------------------------------------------------*\
 * Defined in AmbaB5_VIN.c
\*---------------------------------------------------------------------------*/
int AmbaB5_VinReset(AMBA_B5_CHANNEL_s *pB5Chan);
int AmbaB5_VinPhySetDVP(AMBA_B5_CHANNEL_s *pB5Chan);
int AmbaB5_VinPhySetSLVS(AMBA_B5_CHANNEL_s *pB5Chan);
int AmbaB5_VinPhySetMIPI(AMBA_B5_CHANNEL_s *pB5Chan, AMBA_B5_VIN_MIPI_DPHY_CONFIG_s *pDphyConfig);
int AmbaB5_VinConfigDVP(AMBA_B5_CHANNEL_s *pB5Chan, AMBA_B5_VIN_DVP_CONFIG_s *pVinDvpConfig);
int AmbaB5_VinConfigSLVS(AMBA_B5_CHANNEL_s *pB5Chan, AMBA_B5_VIN_SLVS_CONFIG_s *pVinSlvsConfig);
int AmbaB5_VinConfigMIPI(AMBA_B5_CHANNEL_s *pB5Chan, AMBA_B5_VIN_MIPI_CONFIG_s *pVinMipiConfig);
int AmbaB5_VinCaptureConfig(AMBA_B5_CHANNEL_s *pB5Chan, AMBA_B5_VIN_WINDOW_s *pCaptureWindow);

#endif  /* _AMBA_B5_VIN_H_ */
