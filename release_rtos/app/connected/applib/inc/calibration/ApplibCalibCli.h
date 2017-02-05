/**
 * @file src/app/connected/applib/inc/calibration/ApplibCalibCli.h
 *
 * Header file for Calibration command line interface
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

/**
* @defgroup ApplibCalibCli
* @brief Header file for Calibration command line interface
*
*/

/**
 * @addtogroup ApplibCalibCli
 * @{
 */

/**
*  Calibration manager Command Line Interface (CLI)
*
*  @param [in] Argc argument count
*  @param [in] Argv argument vector
*  @param [out] OutputStr output message
*  @param [out] MsgAvailable check the message is available
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibCli( int Argc, char *Argv[], char *OutputStr, int *MsgAvailable);
//extern void AppLib_CalibCliSdIfTimerHandler(int eid);

/**
*  Calibration by SD card interface
*
*  @param [in] Path the path for the calibration script

*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibPathIf(char* Path);

/**
*  Clear calibration data for partial load mode
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibResetPartial(UINT32 CalId); //for vignette partial load

/**
*  Clear calibration data for partial load mode
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibSDCardReset(UINT32 CalId);

/**
*  Save calibration table head from DRAM to NAND
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibNandSave(UINT32 CalId , UINT32 sub_id);

/**
*  Save calibration from DRAM to SD card
*
*  @param [in] CalId calibration ID
*  @param [in] sub_id sub-channel ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibSDCardSave(UINT32 CalId , UINT32 sub_id);

/**
*  Load calibration data from NAND to DRAM
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibNandLoad(UINT32 CalId);

/**
*  Load calibration data from SD card to DRAM
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibSDCardLoad(UINT32 CalId);

/**
*  Clear SD card calibration partition with zero initialized
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibSDCardReset(UINT32 CalId);

/**
*  Write Status data to NAND flash
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibSaveStatus(void);

/**
*  Init Calibration Nand
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibNandInit(void);

/**
*  Simple initial function for calibration
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibInitSimple(UINT32 CalId);

/**
*  Load calibration data from ROM to DRAM
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibROMLoad(UINT32 CalId);

/**
*  Load calibration table head data from ROM to DRAM
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibROMLoadTableHead(UINT32 CalId);

/**
*  Load calibration table data from ROM to DRAM
*
*  @param [in] CalId calibration ID
*  @param [in] TableIdx entry table id for loading
*  @param [in] TableNums number of tables for loading
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibROMLoadTable(UINT32 CalId, UINT32 TableIdx, UINT32 TableNums);

/**
*  Load calibration table head data from NAND to DRAM
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibNandLoadTableHead(UINT32 CalId);

/**
*  Load calibration table data from NAND to DRAM
*
*  @param [in] CalId calibration ID
*  @param [in] TableIdx entry table id for loading
*  @param [in] TableNums number of tables for loading
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibNandLoadTable(UINT32 CalId, UINT32 TableIdx, UINT32 TableNums);

/**
*  Save calibration table data from DRAM to NAND
*
*  @param [in] CalId calibration ID
*  @param [in] TableIdx entry table id for loading
*  @param [in] TableNums number of tables for loading
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibNandSaveTable(UINT32 CalId, UINT32 TableIdx, UINT32 TableNums);

/**
*  Save calibration table head from DRAM to NAND
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibNandSaveTableHead(UINT32 CalId);

/**
*  Simple initial load function for calibration
*
*  @param [in] CalId calibration ID
*  @param [in] Format calibration data source (rom, nand or sdcard)
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibInitLoadSimple(UINT32 CalId, UINT8 Format);

/**
*  Simple initial function for calibration
*
*  @param [in] CalId calibration ID
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibInitSimple(UINT32 CalId);

/**
*  Check whether the file exists in SD card
*
*  @param [in] Filename file path in SD card
*
*  @return 0 - OK, -1 - Failed
*/
extern int AppLib_CalibSDCardFileExists(char *Filename);

/**
*  check calib structure size alignment
*
*  @return 0 - OK
*/
extern int AppLib_CalibCheckStructure(void);

extern int AppLibCalibAdjust_Init(void);

__END_C_PROTO__

#endif
