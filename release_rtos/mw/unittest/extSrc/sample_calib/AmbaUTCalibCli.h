/**
 * @file mw/unittest/extSrc/sample_calib/AmbaUTCalibCli.h
 *
 * header file for calibration command line interface
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

#ifndef __CALIB_CLI_H__
#define __CALIB_CLI_H__


__BEGIN_C_PROTO__


/* ---------- */
// Calibration manager Command Line Interface (CLI)
extern int AmpUT_CalibCli( int Argc, char *Argv[], char *OutputStr, int *MsgAvailable);
/* ---------- */
extern void AmpUT_CalibCliSdIfTimerHandler(int eid);
/* ---------- */
extern int AmpUT_CalibPathIf(char* Path);
extern int AmpUT_CalibResetPartial(UINT32 CalId); //for vignette partial load
extern int AmpUT_CalibSDCardReset(UINT32 CalId);
extern int AmpUT_CalibNandSave(UINT32 CalId , UINT32 sub_id);
extern int AmpUT_CalibSDCardSave(UINT32 CalId , UINT32 sub_id);
extern int AmpUT_CalibPathIf(char* Path);
extern int AmpUT_CalibNandLoad(UINT32 CalId);
extern int AmpUT_CalibSDCardLoad(UINT32 CalId);
extern int AmpUT_CalibSDCardReset(UINT32 CalId);
extern int AmpUT_CalibSaveStatus(void);
extern int AmpUT_CalibNandInit(void);
extern int AmpUT_CalibInitSimple(UINT32 CalId);
extern int AmpUT_CalibROMLoad(UINT32 CalId);
extern int AmpUT_CalibROMLoadTableHead(UINT32 CalId);
extern int AmpUT_CalibROMLoadTable(UINT32 CalId, UINT32 TableIdx, UINT32 TableNums);
extern int AmpUT_CalibNandLoadTableHead(UINT32 CalId);
extern int AmpUT_CalibNandLoadTable(UINT32 CalId, UINT32 TableIdx, UINT32 TableNums);
extern int AmpUT_CalibNandSaveTable(UINT32 CalId, UINT32 TableIdx, UINT32 TableNums);
extern int AmpUT_CalibNandSaveTableHead(UINT32 CalId);
extern int AmpUT_CalibInitLoadSimple(UINT32 CalId, UINT8 Format);
extern int AmpUT_CalibInitSimple(UINT32 CalId);
extern int AmpUT_CalibSDCardFileExists(char *Filename);
extern int AmpUT_CalibCheckStructure(void);
extern int vsprintf(char *, const char *, va_list);

__END_C_PROTO__

#endif
