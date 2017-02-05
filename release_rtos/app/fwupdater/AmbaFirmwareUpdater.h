/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaFirmwareUpdater.h
 *
 *  @Copyright      :: Copyright (C) 2014 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions & Constants for User Firmware Updater through SD
 *
 *  @History        ::
 *      Date        Name        Comments
 *      03/26/2014  W.Shi       Created
 *
 *  $LastChangedDate: 2014-04-20 20:45:59 +0800 (週日, 20 四月 2014) $
 *  $LastChangedRevision: 9634 $
 *  $LastChangedBy: ishidate $
 *  $HeadURL: http://ambtwsvn2/svn/DSC_Platform/trunk/SoC/A9/Workshop/A9EVK/FirmwareUpdater/inc/AmbaFirmwareUpdater.h $
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_USER_FW_UPDATER_SD_H_
#define _AMBA_USER_FW_UPDATER_SD_H_

#include "AmbaNAND_Def.h"
#include "AmbaNAND_PartitionTable.h"

typedef enum _AMBA_FIRMWARE_TYPE_e_ {
    AMBA_FIRMWARE_SYS_SOFTWARE = 0,
    AMBA_FIRMWARE_DSP_uCODE,
    AMBA_FIRMWARE_SYS_ROM_DATA,
    AMBA_FIRMWARE_LINUX_KERNEL,     /* optional for Linux Kernel */
    AMBA_FIRMWARE_LINUX_ROOT_FS,

    AMBA_NUM_FIRMWARE_TYPE
} AMBA_FIRMWARE_TYPE_e;

typedef struct _AMBA_FIRMWARE_IMAGE_HEADER_s_ {
    UINT32  Crc32;                  /* CRC32 Checksum */
    UINT32  Version;                /* Version number */
    UINT32  Date;                   /* Date */
    UINT32  Length;                 /* Image length */
    UINT32  MemAddr;                /* Location to be loaded into memory */
    UINT32  Flag;                   /* Flag of partition    */
    UINT32  Magic;                  /* The magic number     */
    UINT32  Reserved[57];
} AMBA_FIRMWARE_IMAGE_HEADER_s;

typedef struct _AMBA_FIRMWARE_HEADER_s_ {
    char    ModelName[32];          /* model name */

    struct {
        UINT8   Major;              /* Major number */
        UINT8   Minor;              /* Minor number */
        UINT16  Svn;                /* SVN serial number */
    } VER_INFO_s;

    AMBA_PLOAD_PARTITION_s PloadInfo;

    UINT32  Crc32;                  /* CRC32 of entire Binary File: AmbaCamera.bin */

    struct {
        UINT32   Size;
        UINT32   Crc32;
    } __POST_ATTRIB_PACKED__ FwInfo[AMBA_NUM_FIRMWARE_TYPE];

    UINT32  PartitionSize[AMBA_NUM_NAND_PARTITION];
} __POST_ATTRIB_PACKED__ AMBA_FIRMWARE_HEADER_s;

/*-----------------------------------------------------------------------------------------------*\
 * Error codes
\*-----------------------------------------------------------------------------------------------*/
#define AMBA_FW_UPDATER_ERR_MAGIC             (-1)
#define AMBA_FW_UPDATER_ERR_LENGTH            (-2)
#define AMBA_FW_UPDATER_ERR_CRC               (-3)
#define AMBA_FW_UPDATER_ERR_VER_NUM           (-4)
#define AMBA_FW_UPDATER_ERR_VER_DATE          (-5)
#define AMBA_FW_UPDATER_ERR_PROG_IMAGE        (-6)
#define AMBA_FW_UPDATER_ERR_FIRM_FILE         (-7)
#define AMBA_FW_UPDATER_ERR_NO_MEM            (-8)
#define AMBA_FW_UPDATER_ERR_ILLEGAL_HEADER    (-9)
#define AMBA_FW_UPDATER_ERR_META_SET          (-10)
#define AMBA_FW_UPDATER_ERR_META_GET          (-11)
#define AMBA_FW_UPDATER_ERR_LOG_SET           (-12)
#define AMBA_FW_UPDATER_ERR_LOG_GET           (-13)
#define AMBA_FW_UPDATER_ERR_MP_RAW            (-14)
#define AMBA_FW_UPDATER_ERR_MP_PREFERENCE     (-15)
#define AMBA_FW_UPDATER_ERR_MP_CALIB          (-16)

/*-----------------------------------------------------------------------------------------------*\
 * Defined in AmbaUserFwUpdaterSD.c
\*-----------------------------------------------------------------------------------------------*/
int AmbaFwUpdaterMain(void);

#endif /* _AMBA_USER_FW_UPDATER_SD_H_ */
