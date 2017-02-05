/**
 * @file AmpUT_Cfs.c
 *
 * History:
 *    2013/05/22 - [Irene Wang] created file
 *    2013/09/25 - modified by [Yi-Ching Liao]
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <cfs/AmpCfs.h>
#include <AmbaFS.h>
#include <AmbaVer.h>
#include <util.h>
#include <AmbaCache_Def.h>
#include <errno.h>
#include <ssp/AmbaKAL.h>
#include "AmpUnitTest.h"
#include "AmbaUtility.h"

#include "AmbaCache.h"
#include "AmbaGDMA.h"
#define AMP_CFS_GDMA_s  AMBA_GDMA_LINEAR_CTRL_s
#define AMP_CFS_UT_ST_GDMA_BUF_LEN   (AMBA_GDMA_MAX_WIDTH * 2)
#define AMP_CFS_UT_GET_ALIGNMENT_MASK(alignment)               (~((alignment) - 1))
#define AMP_CFS_UT_GET_NEXT_ALIGNED_ADDR(addr, alignment)      ((UINT8 *)(((UINT32)(addr) + (alignment) - 1) & AMP_CFS_UT_GET_ALIGNMENT_MASK(alignment)))
#define AMP_CFS_UT_GET_PREVIOUS_ALIGNED_ADDR(addr, alignment)  ((UINT8 *)((UINT32)(addr) & AMP_CFS_UT_GET_ALIGNMENT_MASK(alignment)))

/*
 * Alias & Helper
 */
#define AMP_CFS_SYNC_NINVALIDATE    AMP_CFS_SYNC_FLUSH
#define AMP_CFS_MODE_ATTR_ADD   AMP_CFS_ATTR_ADD
#define AMP_CFS_DRIVE_e         char
#define AMP_CFS_DRIVE_C         'C'

#define AMP_CFS_DTA_s           AMP_CFS_DTA
#define AMP_CFS_DRIVE_INFO_s    AMP_CFS_DEVINF
#define AMP_CFS_FILE_INFO_s     AMP_CFS_STAT

#define AMBA_FS_DTA_t           AMBA_FS_DTA
#define AMBA_FS_DRIVE_INFO_t    AMBA_FS_DEVINF
#define AMBA_FS_FILE_INFO_s     AMBA_FS_STAT

#define AmbaFS_GetFileInfo      AmbaFS_Stat
#define AmbaFS_GetDriveInfo     AmbaFS_GetDev
#define AmbaFS_FileSearchFirst  AmbaFS_FirstDirEnt
#define AmbaFS_FileSearchNext   AmbaFS_NextDirEnt
#define AmbaFS_Fsync            AmbaFS_FSync

#define AmpCFS_GetFileInfo      AmpCFS_Stat
#define AmpCFS_GetDriveInfo     AmpCFS_GetDev
#define AmpCFS_FileSearchFirst  AmpCFS_FirstDirEnt
#define AmpCFS_FileSearchNext   AmpCFS_NextDirEnt
#define AmpCFS_UnMount          AmpCFS_Unmount
#define AmpCFS_Fsync            AmpCFS_FSync
#define AmpCFS_ClusterAppend    AmpCFS_fappend
#define AmpCFS_RemoveDir        AmpCFS_Rmdir
#define AmpCFS_MakeDir          AmpCFS_Mkdir
#define AmpCFS_ClusterDelete    AmpCFS_Cdelete
#define AmpCFS_ClusterInsert    AmpCFS_Cinsert
#define AmpCFS_FileDivide       AmpCFS_Divide
#define AmpCFS_FileCombine      AmpCFS_Combine
#define AmpCFS_ChangeFileMode   AmpCFS_Chmod

#define AmpUT_CfsGetDtaRawInfo(Dta)    (Dta->u)
#define AmpUT_CfsGetDriveBytesPerCluster(info)  ((info).Bps * (info).Spc)
#define AmpUT_CfsGetDriveEmptyClusters(info)    ((info).Ucl)

/*
 * Flags
 */
#define AMP_CFS_UT_COMPILE_WARNING      0   // Disable flag for warning message
#define AMP_CFS_UT_LOG_VERB_ENABLE      0
#define AMP_CFS_UT_ST_ERR_MSG_ENABLE    0    // Flag to enable CFS error message

#define AMP_CFS_UT_TEST_CMD_SET_DEFAULT AMP_CFS_UT_TEST_CMD_SET_ALL // AMP_CFS_UT_TEST_CMD_SET_UG or AMP_CFS_UT_TEST_CMD_SET_ALL

#if AMP_CFS_UT_ST_ERR_MSG_ENABLE
    #define AMP_CFS_UT_ST_MULTIOPEN_FILE_AMOUNT 4
#else
    #define AMP_CFS_UT_ST_MULTIOPEN_FILE_AMOUNT 1
#endif

/*
 * Macro - print debug message function
 * note: should not use static buffer, but unit test only single test, so ok (otherwise, AmpUnitTest's stack would overflow)
 */
#define AMP_CFS_UT_LOG_COL_P    GREEN
#define AMP_CFS_UT_LOG_COL_F    RED
#define AMP_CFS_UT_LOG_COL_M    BLACK
#define AMP_CFS_UT_LOG_COL_V    BLACK
#define AMP_CFS_UT_LOG_COL_A    MAGENTA

#define AMP_CFS_UT_STRINGIFY(x) #x
#define AMP_CFS_UT_TOSTR(x) AMP_CFS_UT_STRINGIFY(x)
#define AMP_CFS_UT_WIDE_(x)  L ## x
#define AMP_CFS_UT_WIDE(x)  AMP_CFS_UT_WIDE_(x)

#define AMP_CFS_UT_PRINT(color, fmt, ...) do {  \
    AmbaPrintColor(color, fmt" ", ##__VA_ARGS__); \
    AmbaKAL_TaskSleep(10);  \
} while(0)

#define AMP_CFS_UT_LOG(lType, fmt, ...) AMP_CFS_UT_PRINT(AMP_CFS_UT_LOG_COL_##lType, "[%05u][UT]["#lType"][%s] "fmt, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOG_PRT(fmt, ...)   AmbaPrint(fmt" ", ##__VA_ARGS__)
#define LOG_PASS(fmt, ...)  AMP_CFS_UT_LOG(P, fmt, ##__VA_ARGS__)
#define LOG_FAIL(fmt, ...)  AMP_CFS_UT_LOG(F, fmt, ##__VA_ARGS__)
#define LOG_MSG(fmt, ...)   AMP_CFS_UT_LOG(M, fmt, ##__VA_ARGS__)
#define LOG_MARK(fmt, ...)  AMP_CFS_UT_LOG(A, fmt, ##__VA_ARGS__)

#define LOG_RESULT(res) do { if ((res) == AMP_OK) LOG_PASS("Pass!"); else LOG_FAIL("Fail!"); } while(0)
#define LOG_RESULT_FMT(res, fmt, ...) do { if ((res) == AMP_OK) LOG_PASS(fmt, ##__VA_ARGS__); else LOG_FAIL(fmt, ##__VA_ARGS__); } while(0)
#define LOG_FIN_RESULT_FMT(res, fmt, ...) do { if ((res) == AMP_OK) AMP_CFS_UT_PRINT(GREEN, "[SUCCESS] " fmt, ##__VA_ARGS__); else AMP_CFS_UT_PRINT(RED, "[FAIL] " fmt, ##__VA_ARGS__); } while(0)

#if AMP_CFS_UT_LOG_VERB_ENABLE
    #define LOG_VERB(fmt, ...) AMP_CFS_UT_LOG(V, fmt, ##__VA_ARGS__)
    #define LOG_V_PASS      LOG_PASS
    #define LOG_V_FAIL      LOG_FAIL
    #define LOG_V_MSG       LOG_MSG
    #define LOG_V_RESULT    LOG_RESULT_FMT
#else
    #define LOG_VERB(fmt, ...)
    #define LOG_V_PASS(fmt, ...)
    #define LOG_V_FAIL(fmt, ...)
    #define LOG_V_MSG(fmt, ...)
    #define LOG_V_RESULT(...)
#endif

/*
 * The API of CFS log module
 */
#ifdef CONFIG_MW_CFS_LOGM_ENABLE
extern int AmpCFS_LogProbeTimeEntryStart(UINT32 *id);
extern int AmpCFS_LogProbeTimeEntryEnd(UINT32 id);
extern int AmpCFS_LogProbeOPEntries(void);
extern int AmpCFS_LogDumpEntryList(void);
#define LOGM_DECLARE(type, name, val) type name = val
#else
#define AmpCFS_LogProbeTimeEntryStart(...)
#define AmpCFS_LogProbeTimeEntryEnd(...)
#define AmpCFS_LogProbeOPEntries(...)
#define AmpCFS_LogDumpEntryList(...)
#define LOGM_DECLARE(...)
#endif

/**
 * File modes
 */
#define AMP_CFS_FILE_ASYNC_MODE             1 /**< async mode */
#define AMP_CFS_FILE_SYNC_MODE              0 /**< sync mode */

/**
 * CFS stress test
 */
#define AMP_CFS_UT_ST_BUF_UINT      (1048576)   // 1 MiB
#define AMP_CFS_UT_ST_WBUF_COUNT    (2)
#define AMP_CFS_UT_ST_WBUF_SIZE     AMP_CFS_UT_ST_BUF_UINT
#define AMP_CFS_UT_ST_RBUF_COUNT    (4)
#define AMP_CFS_UT_ST_RBUF_SIZE     AMP_CFS_UT_ST_BUF_UINT
#define AMP_CFS_UT_ST_BUF_SIZE      (AMP_CFS_UT_ST_BUF_UINT * 3)
#define AMP_CFS_UT_ST_FN_AMBAFS     "C:\\UT_CFS_STRESS_AMBAFS.txt"
#define AMP_CFS_UT_ST_FN_SYNC       "C:\\UT_CFS_STRESS_SYNC.txt"
#define AMP_CFS_UT_ST_FN_ASYNC      "C:\\UT_CFS_STRESS_ASYNC.txt"
#define AMP_CFS_UT_ST_FN(tkn)       "C:\\UT_CFS_STRESS" AMP_CFS_UT_TOSTR(tkn) ".txt"

#define AMP_CFS_UT_ST_CMD_PREFIX        "stress_"
#define AMP_CFS_UT_ST_CMD_PREFIX_LEN    7   //strlen(AMP_CFS_UT_ST_CMD_PREFIX)
#define AMP_CFS_UT_ST_CMD_PARAM_PREFIX      "-"
#define AMP_CFS_UT_ST_CMD_PARAM_PREFIX_LEN  1

#define AMP_CFS_UT_ST_OPENCONFIG_AMOUNT     8
#define AMP_CFS_UT_ST_TESTCASE_CMD_AMOUNT   16

#define AMP_CFS_UT_ST_OPEN_CONFIG_ID_DEFAULT        0
#define AMP_CFS_UT_ST_OPEN_CONFIG_ID_ALIGNMENT      1
#define AMP_CFS_UT_ST_OPEN_CONFIG_ID_LOWPRI         2

#define AMP_CFS_UT_ST_BYTE_TEST_BASE        251 /**< byte test base */

typedef enum AMP_CFS_UT_ST_TEST_CASE_MODE_e_ {
    AMP_CFS_UT_ST_TEST_CASE_MODE_SYNC   = 0x01,
    AMP_CFS_UT_ST_TEST_CASE_MODE_ASYNC  = 0x02,
    AMP_CFS_UT_ST_TEST_CASE_MODE_ALL    = 0x03,
} AMP_CFS_UT_ST_TEST_CASE_MODE_e;

typedef struct AMP_CFS_UT_ST_CMD_PARAMS_s_ AMP_CFS_UT_ST_CMD_PARAMS_s;

typedef struct AMP_CFS_UT_ST_TEST_RUNNER_s_ {
    void *Params;
    int (*Init)(AMP_CFS_UT_ST_CMD_PARAMS_s*);
    int (*Run)(AMP_CFS_UT_ST_CMD_PARAMS_s*);
} AMP_CFS_UT_ST_TEST_RUNNER_s ;

typedef struct AMP_CFS_UT_ST_OPEN_CONFIG_s_ {
    char fileName[MAX_FILENAME_LENGTH];
    UINT8 fileMode;
    UINT32 alignment;
    UINT32 byteToSync;
    BOOL8 isAsync;
    UINT8 maxNumBank;
    BOOL8 isLowPriority;
    UINT8 id;
} AMP_CFS_UT_ST_OPEN_CONFIG_s;

typedef struct AMP_CFS_UT_ST_TEST_CASE_PARAMS_s_ {
    UINT32 FileSize;
    UINT8 TestMode;
} AMP_CFS_UT_ST_TEST_CASE_PARAMS_s;

typedef int (*AMP_CFS_UT_ST_TESTCASE_COMMAND_FP) (AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params);
typedef int (*AMP_CFS_UT_ST_OPEN_CONFIG_SETTER_FP)(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, void *params);

struct AMP_CFS_UT_ST_CMD_PARAMS_s_ {
    AMP_CFS_UT_ST_OPEN_CONFIG_s OpenConfigs[AMP_CFS_UT_ST_OPENCONFIG_AMOUNT];
    AMP_CFS_UT_ST_TESTCASE_COMMAND_FP TestCmds[AMP_CFS_UT_ST_TESTCASE_CMD_AMOUNT];
    AMP_CFS_UT_ST_TEST_CASE_PARAMS_s TestCaseParams;
    UINT8 OpenConfigAmount;
    BOOL8 IsStressTest;
};

#define AMP_CFS_UT_ST_SYS_FILE_OPEN_MAX         (10)
#define AMP_CFS_UT_ST_MULTIOPEN_STREAM_MAX      (32)

#define AMP_CFS_UT_ST_BITMASK {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}

#define AMP_CFS_UT_ST_TASK_AMOUNT       (2)
#define AMP_CFS_UT_ST_TASK_STACK_SIZE   (1024 * 32)

typedef struct AMP_CFS_UT_ST_TASK_INFO_s_ {
    AMBA_KAL_TASK_t Task[AMP_CFS_UT_ST_TASK_AMOUNT];
    char TaskStacts[AMP_CFS_UT_ST_TASK_AMOUNT][AMP_CFS_UT_ST_TASK_STACK_SIZE];
} AMP_CFS_UT_ST_TASK_INFO_s;

/**
 * The commands of CFS unit test
 */
typedef struct AMP_CFS_SHELL_ENV_s_ {
    int Argc;
    char **Argv;
} AMP_CFS_UT_SHELL_ENV_s;

#define AMP_CFS_UT_TEST_CMD_FUNC(x)     Cfs_TestCase_##x
#define AMP_CFS_UT_TEST_CMD_ENTRY(x)    {#x, AMP_CFS_UT_TEST_CMD_FUNC(x)}
#define AMP_CFS_UT_TEST_CMD_DECL(x)     int Cfs_TestCase_##x(AMP_CFS_UT_SHELL_ENV_s *env) __attribute__((weak))
typedef int (*AMP_CFS_UT_TEST_CMD_HDLR_f)(AMP_CFS_UT_SHELL_ENV_s *env);
typedef struct AMP_CFS_TEST_CMD_s_ {
    char  *Name;
    AMP_CFS_UT_TEST_CMD_HDLR_f  Hdlr;
} AMP_CFS_UT_TEST_CMD_s;

#define AMP_CFS_UT_TEST_CMD_LIST_ITEM_MAX   4
typedef struct AMP_CFS_UT_TEST_CMD_LIST_s_ AMP_CFS_UT_TEST_CMD_LIST_s;
struct AMP_CFS_UT_TEST_CMD_LIST_s_ {
    UINT8 Count;
    AMP_CFS_UT_TEST_CMD_s Items[AMP_CFS_UT_TEST_CMD_LIST_ITEM_MAX];
    AMP_CFS_UT_TEST_CMD_LIST_s *NextList;
};

typedef enum AMP_CFS_UT_TEST_CMD_SET_e_ {
    AMP_CFS_UT_TEST_CMD_SET_UG = 1,
    AMP_CFS_UT_TEST_CMD_SET_ALL
} AMP_CFS_UT_TEST_CMD_SET_e;

/**
 * CFS unit test
 */
#define AMP_CFS_UT_SMALL_WRITE_COUNT        16384 /**< small write count */
#define AMP_CFS_UT_ALIGNMENT_DEFAULT        0 /**, alignment parameter */
#define AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT    (4*1024*1024) /**< number of bytes to sync file */
#define AMP_CFS_UT_PRIORITY_DEFAULT         FALSE /**< file priority */
#define AMP_CFS_UT_BIG_TEST_BUF_SIZE        (16*128*1024) /**< test buffer size, 2 MiB */
#define AMP_CFS_UT_MAX_FILE                 100 /**< max number of file */
#define AMP_CFS_UT_MAX_BANK_AMOUNT          16 /**< max number of bank */
#define AMP_CFS_UT_BYTE_TEST_BASE           256 /**< byte test base */

#ifdef CONFIG_MW_UT_CFS_DTA_AMOUNT
    #define AMP_CFS_UT_DTA_NUM      (CONFIG_MW_UT_CFS_DTA_AMOUNT)
#else
    #define AMP_CFS_UT_DTA_NUM      4
#endif
#define AMP_CFS_UT_AMBAFS_DTA_NUM   1

typedef struct AMP_CFS_UNIT_TEST_s_{
    AMP_CFS_UT_ST_CMD_PARAMS_s *STCmdParamsAligned;
    AMP_CFS_UT_ST_TASK_INFO_s *TaskInfo;
    UINT8 *STWriteBuffer;
    UINT8 *STReadBuffer;
    UINT8 *pBufferTest;
    AMP_CFS_DTA_s *CfsDta;
    AMBA_FS_DTA_t *AmbaFsDta;
    AMP_CFS_UT_TEST_CMD_LIST_s *CfsTestCmdList;
    BOOL8 bInit;
    int ClusterSize;
    AMP_CFS_UT_SHELL_ENV_s *CurrentShellEnv;
} AMP_CFS_UNIT_TEST_s;

static AMP_CFS_UNIT_TEST_s g_AmpCfsUnitTest = {0}; /**< unit test */

static int AmpUT_CfsInit(void);

/**
 *  Prototypes of external APIs
 */
extern int AmpCFS_CachePrintTree(AMP_CFS_DRIVE_e drive);

/**
 * Macro function
 */
#define IIF(exp, tp, fp) (((exp)) ? (tp) : (fp))
#define IS_FULL_PATH(x) ((x)[1] == ':')

#define ALLOC_MEM(rt, addr, size) do {   \
    if ((rt) == AMP_OK) {   \
        UINT32 _Size = (UINT32)(size);  \
        if (((rt) = AmpUT_CfsAllocMemFormPool((UINT8 **)(addr), _Size)) != AMP_OK) {   \
            LOG_FAIL("Failed to allocate memory from the pool. addr: %p, size: %u", (UINT8 *)(addr), _Size);    \
        }   \
    }   \
} while(0)

#define TEST_CMD_IS_EMPTY(cmd) ((cmd)->Hdlr == NULL && (cmd)->Name == 0)
#define TEST_CMD_ADD(rt, cmd) do {    \
    AMP_CFS_UT_TEST_CMD_DECL(cmd);  \
    if ((rt) == AMP_OK && AMP_CFS_UT_TEST_CMD_FUNC(cmd)) {   \
        Cfs_TestCmdList_AddItem(g_AmpCfsUnitTest.CfsTestCmdList, &((AMP_CFS_UT_TEST_CMD_s)AMP_CFS_UT_TEST_CMD_ENTRY(cmd)));  \
    }   \
} while(0)

/**
 * Function implementation
 */
void AmpUT_CfsPrintDriveInfoAmbaFs(const AMBA_FS_DRIVE_INFO_t *info)
{
    K_ASSERT(info != NULL);
    LOG_PRT("Device Information");
    LOG_PRT("Total clusters        = %d",info->Cls);
    LOG_PRT("Unused clusters       = %d",info->Ucl);
    LOG_PRT("Bytes per sector      = %d",info->Bps);
    LOG_PRT("Sectors per cluster   = %d",info->Spc);
    LOG_PRT("Clusters per group    = %d",info->Cpg);
    LOG_PRT("Unused cluster groups = %d",info->Ucg);
    LOG_PRT("Format type           = %d",info->Fmt);
}

void AmpUT_CfsPrintDriveInfoCfs(const AMP_CFS_DRIVE_INFO_s *info)
{
    K_ASSERT(info != NULL);
    LOG_PRT("Device Information");
    LOG_PRT("Total clusters        = %d",info->Cls);
    LOG_PRT("Unused clusters       = %d",info->Ucl);
    LOG_PRT("Bytes per sector      = %d",info->Bps);
    LOG_PRT("Sectors per cluster   = %d",info->Spc);
    LOG_PRT("Clusters per group    = %d",info->Cpg);
    LOG_PRT("Unused cluster groups = %d",info->Ucg);
    LOG_PRT("Format type           = %d",info->Fmt);
}

void AmpUT_CfsPrintDtaLineAmbaFs(const AMBA_FS_DTA_t *dirEntry)
{
    const char *File;
    K_ASSERT(dirEntry != NULL);
    File = (const char *)((dirEntry->LongName[0] != '\0') ? dirEntry->LongName : dirEntry->FileName);
    LOG_PRT("%c%c%c%c%c %.4d/%.2d/%.2d %.2d:%.2d:%.2d %13lld %c%s%c",
        (dirEntry->Attribute & ATTR_DIR)    ? 'd' : 'f',
        (dirEntry->Attribute & ATTR_RDONLY) ? 'r' : '-',
        (dirEntry->Attribute & ATTR_ARCH)   ? 'a' : '-',
        (dirEntry->Attribute & ATTR_SYSTEM) ? 's' : '-',
        (dirEntry->Attribute & ATTR_HIDDEN) ? 'h' : '-',
        ((dirEntry->Date>>9) & 0x7F) + 1980,
        ((dirEntry->Date>>5) & 0x0f),
        (dirEntry->Date & 0x1f),
        ((dirEntry->Time>>11) & 0x1f),
        ((dirEntry->Time>>5) & 0x3f),
        (dirEntry->Time & 0x1f)<<1,
        dirEntry->FileSize,
        (dirEntry->Attribute & AMP_CFS_ATTR_DIR) ? '[' : ' ',
        File,
        (dirEntry->Attribute & AMP_CFS_ATTR_DIR) ? ']' : ' ');
}

/**
 *  Print the contain of a DirEnt - for cls command.
 *  @param [in] pDat address of a Dta record
 */
void AmpUT_CfsPrintDtaLineCfs(const AMP_CFS_DTA_s *dirEntry)
{
    AmpUT_CfsPrintDtaLineAmbaFs(dirEntry);
}

void AmpUT_CfsPrintDtaAmbaFs(const AMBA_FS_DTA_t *dta)
{
    K_ASSERT(dta != NULL);
    LOG_PRT("============ DTA DATA ============");
    LOG_PRT("- u ");
    LOG_PRT("  - p_file             = 0X%X", dta->u.dta.p_file);
    LOG_PRT("  - p_dir              = 0X%X", dta->u.dta.p_dir);
    LOG_PRT("  - p_vol              = 0X%X", dta->u.dta.p_vol);
    LOG_PRT("  - parentStartCluster = %d",   dta->u.dta.parent_start_cluster);
    LOG_PRT("  - parentPos          = %d",   dta->u.dta.parent_pos);
    LOG_PRT("  - hint");
    LOG_PRT("    - ci_idx           = %d",   dta->u.dta.hint.ci_idx);
    LOG_PRT("    - ci_clst          = %d",   dta->u.dta.hint.ci_clst);
    LOG_PRT("  - status             = %d",   dta->u.dta.status);
#if PF_CHECK_PATH_LENGTH
    LOG_PRT("  - path_len           = %d",   dta->u.dta.path_len);
#endif
    LOG_PRT("  - num_entry_LFNs     = %d",   dta->u.dta.num_entry_LFNs);
    LOG_PRT("  - ordinal            = %d",   dta->u.dta.ordinal);
    LOG_PRT("  - check_sum          = %d",   dta->u.dta.check_sum);
#if PF_EXFAT_SUPPORT
    LOG_PRT("  - parent_general_flg = %d",   dta->u.dta.parent_general_flg);
    LOG_PRT("  - parent_dir_size    = %lld", dta->u.dta.parent_dir_size);
    LOG_PRT("  - parent_path        = %s",   dta->u.dta.parent_path);
    LOG_PRT("  - num_secondary      = %d",   dta->u.dta.num_secondary);
    LOG_PRT("  - general_flg        = %d",   dta->u.dta.general_flg);
#endif
    LOG_PRT("  - attr               = %d",   dta->u.dta.attr);
    LOG_PRT("  - reg_exp            = %s",   dta->u.dta.reg_exp);
#if PF_EXFAT_SUPPORT
    LOG_PRT("  - TimeMs             = %d",   dta->u.dta.TimeMs);
#endif
    LOG_PRT("  - Time               = 0X%X", dta->u.dta.Time);
    LOG_PRT("  - Date               = 0X%X", dta->u.dta.Date);
#if PF_EXFAT_SUPPORT
    LOG_PRT("  - UtcOffset          = %d",   dta->u.dta.UtcOffset);
    LOG_PRT("  - ATime              = 0X%X", dta->u.dta.ATime);
#endif
    LOG_PRT("  - ADate              = 0X%X", dta->u.dta.ADate);
#if PF_EXFAT_SUPPORT
    LOG_PRT("  - AUtcOffset         = %d",   dta->u.dta.AUtcOffset);
#endif
    LOG_PRT("  - CTimeMs            = %d",   dta->u.dta.CTimeMs);
    LOG_PRT("  - CTime              = %d",   dta->u.dta.CTime);
    LOG_PRT("  - CDate              = %d",   dta->u.dta.CDate);
#if PF_EXFAT_SUPPORT
    LOG_PRT("  - CUtcOffset         = %d",   dta->u.dta.CUtcOffset);
#endif
    LOG_PRT("  - FileSize           = %d",   dta->u.dta.FileSize);
    LOG_PRT("  - Attribute          = %d",   dta->u.dta.Attribute);
    LOG_PRT("  - FileName           = %s",   dta->u.dta.FileName);
    LOG_PRT("  - LongName           = %s",   dta->u.dta.LongName);
#if defined(__PRKERNEL_AMB__) || defined(__TX_AMB__)
    LOG_PRT("  - StartClst          = %d",   dta->u.dta.StartClst);
#endif
    LOG_PRT("- FsType    = %d",   dta->FsType);
    LOG_PRT("- Time      = 0X%X", dta->Time);
    LOG_PRT("- Date      = 0X%X", dta->Date);
    LOG_PRT("- FileSize  = %lld", dta->FileSize);
    LOG_PRT("- StartClst = %d",   dta->StartClst);
    LOG_PRT("- FileName  = %s",   dta->FileName);
    LOG_PRT("- LongName  = %s",   dta->LongName);
    LOG_PRT("- Attribute = 0X%X", dta->Attribute);
    LOG_PRT("- SearchMode= %d",   dta->SearchMode);
    LOG_PRT("- pCurInode = 0X%X", dta->pRomfsCurrentInode);
}

/**
 *  Print the contain of a Dta record.
 *  @param [in] Dta address of a dta record
 */
void AmpUT_CfsPrintDtaCfs(const AMP_CFS_DTA_s *cfsDta)
{
    AmpUT_CfsPrintDtaAmbaFs(cfsDta);
}

/**
 *  Print the contain of a file stat.
 *  @param [in] Stat address of a stat record
 */
void AmpUT_CfsPrintFileInfo(const AMP_CFS_FILE_INFO_s *info)
{
    K_ASSERT(info != NULL);
    LOG_PRT("============ STAT DATA ============");
    LOG_PRT("status ");
    LOG_PRT("  - fstfz  = %d",   info->Stat.fstfz);
    LOG_PRT("  - fstact = 0X%X", info->Stat.fstact);
    LOG_PRT("  - fstad  = 0X%X", info->Stat.fstad);
    LOG_PRT("  - fstautc= 0X%X", info->Stat.fstautc);
    LOG_PRT("  - fstut  = 0X%X", info->Stat.fstut);
    LOG_PRT("  - fstuc  = 0X%X", info->Stat.fstut);
    LOG_PRT("  - fstud  = 0X%X", info->Stat.fstud);
    LOG_PRT("  - fstuutc= 0X%X", info->Stat.fstuutc);
    LOG_PRT("  - fstct  = 0X%X", info->Stat.fstct);
    LOG_PRT("  - fstcd  = 0X%X", info->Stat.fstcd);
    LOG_PRT("  - fstcc  = 0X%X", info->Stat.fstcc);
    LOG_PRT("  - fstcutc= 0X%X", info->Stat.fstcutc);
    LOG_PRT("  - fstat  = 0X%X", info->Stat.fstat);
    LOG_PRT("Size           = %lld", info->Size);
    LOG_PRT("LastAccDate    = 0X%X", info->LastAccDate);
    LOG_PRT("LastAccTime    = 0X%X", info->LastAccTime);
    LOG_PRT("LastMdyDate    = 0X%X", info->LastMdyDate);
    LOG_PRT("LastMdyTime    = 0X%X", info->LastMdyTime);
    LOG_PRT("CreateTime     = 0X%X", info->CreateTime);
    LOG_PRT("CreateDate     = 0X%X", info->CreateDate);
    LOG_PRT("CreateCompTime = 0X%X", info->CreateCompTime);
    LOG_PRT("Attr           = 0X%X", info->Attr);
}

/**
 * CFS UT - file operation function.
 *
 * @param [in] opCode CFS operation code.
 * @param [in] param CFS parameter.
 * @return 0 - OK, others - fail
 *
 */
#if AMP_CFS_UT_COMPILE_WARNING
static int AmpUT_CfsFileOperation(int opCode, UINT32 param)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    char file[MAX_FILENAME_LENGTH];
    /**< Can't waste too much time in this function. */
    switch (opCode) {
        case AMP_CFS_EVENT_FOPEN:
            {
                AMP_CFS_OP_TYPE1_s *pm1;
                pm1 = (AMP_CFS_OP_TYPE1_s *)param;
                AmbaPrint("AMP_CFS_EVENT_FOPEN: filename=%s", pm1->File);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_FCLOSE:
            {
                AMP_CFS_OP_TYPE1_s *pm1;
                pm1 = (AMP_CFS_OP_TYPE1_s *)param;
                AmbaPrint("AMP_CFS_EVENT_FCLOSE: filename=%s", pm1->File);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_REMOVE:
            {
                AMP_CFS_OP_TYPE1_s *pm1;
                pm1 = (AMP_CFS_OP_TYPE1_s *)param;
                AmbaPrint("AMP_CFS_EVENT_REMOVE: filename=%s", pm1->File);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_MOVE:
            {
                AMP_CFS_OP_TYPE2_s *pm2;
                pm2 = (AMP_CFS_OP_TYPE2_s *)param;
                AmbaPrint("AMP_CFS_EVENT_MOVE: source=%s destination=%s", pm2->File1, pm2->File2);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_CHMOD:
            {
                AMP_CFS_OP_TYPE3_s *pm3;
                pm3 = (AMP_CFS_OP_TYPE3_s *)param;
                AmbaPrint("AMP_CFS_EVENT_CHMOD: filename=%s attribute=0x%x", pm3->File, pm3->Attr);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_FIRSTDIRENT:
            {
                AMP_CFS_OP_TYPE4_s *pm4;
                char *FName;
                pm4 = (AMP_CFS_OP_TYPE4_s *)param;
                FName = (pm4->pDirEnt->LongName[0] == 0) ? pm4->pDirEnt->FileName : pm4->pDirEnt->LongName;
                AmbaPrint("AMP_CFS_EVENT_FIRSTDIRENT: pattern=%s attribute=0x%x filename=%s", pm4->pDirName, pm4->Attr, FName);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_NEXTDIRENT:
            {
                AMP_CFS_OP_TYPE5_s *pm5 = (AMP_CFS_OP_TYPE5_s *)param;
                char *FName = (pm5->pDirEnt->LongName[0] == 0) ? pm5->pDirEnt->FileName : pm5->pDirEnt->LongName;
                AmbaPrint("AMP_CFS_EVENT_NEXTDIRENT: filename=%s", FName);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_COMBINE:
            {
                AMP_CFS_OP_TYPE2_s *pm2;
                pm2 = (AMP_CFS_OP_TYPE2_s *)param;
                AmbaPrint("AMP_CFS_EVENT_COMBINE: base=%s add=%s", pm2->File1, pm2->File2);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_DIVIDE:
            {
                AMP_CFS_OP_TYPE2_s *pm2;
                pm2 = (AMP_CFS_OP_TYPE2_s *)param;
                AmbaPrint("AMP_CFS_EVENT_DIVIDE: file1=%s file2=%s", pm2->File1, pm2->File2);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_CINSERT:
            {
                AMP_CFS_OP_TYPE1_s *pm1;
                pm1 = (AMP_CFS_OP_TYPE1_s *)param;
                AmbaPrint("AMP_CFS_EVENT_CINSERT: filename=%s", pm1->File);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_CDELETE:
            {
                AMP_CFS_OP_TYPE1_s *pm1;;
                pm1 = (AMP_CFS_OP_TYPE1_s *)param;
                AmbaPrint("AMP_CFS_EVENT_CDELETE: filename=%s", pm1->File);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_RMDIR:
            {
                AMP_CFS_OP_TYPE1_s *pm1;
                pm1 = (AMP_CFS_OP_TYPE1_s *)param;
                AmbaPrint("AMP_CFS_EVENT_RMDIR: dir=%s", pm1->File);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_MKDIR:
            {
                AMP_CFS_OP_TYPE1_s *pm1;
                pm1 = (AMP_CFS_OP_TYPE1_s *)param;
                AmbaPrint("AMP_CFS_EVENT_MKDIR: dir=%s", pm1->File);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_FORMAT:
            {
                AMP_CFS_OP_TYPE1_s *pm1;
                pm1 = (AMP_CFS_OP_TYPE1_s *)param;
                AmbaPrint("AMP_CFS_EVENT_FORMAT: drive=%c", pm1->File[0]);
                Rval = AMP_OK;
                break;
            }
        case AMP_CFS_EVENT_LOWSPEED:
            {
                AMP_CFS_OP_TYPE2_s *pm6 = (AMP_CFS_OP_TYPE2_s *)param;
                AmbaPrint("Low Speed: Drive %c", pm6->Drive);
                Rval = AMP_OK;
                break;
            }
        default:
            AmbaPrint("%s: The operation(%d) is not supported.", __FUNCTION__, opCode);
            break;
    }
    return Rval;

    if (opCode == AMP_CFS_EVENT_LOWSPEED) {
        AMP_CFS_OP_TYPE2_s *pm6 = (AMP_CFS_OP_TYPE2_s *)param;
        AmbaPrint("Low Speed: Drive %c", (char) pm6->Drive);
    }
    return AMP_OK;
}
#endif

static int AmpUT_CfsEventHandlers(int opCode, UINT32 param)
{
    int Rval = AMP_OK;

    switch(opCode) {
    case AMP_CFS_EVENT_IO_ERROR:
    {
        AMP_CFS_OP_TYPE2_s *pm = (AMP_CFS_OP_TYPE2_s *)param;
        LOG_FAIL("I/O Error at Drive %c.", pm->Drive);
        break;
    }
#if AMP_CFS_UT_LOG_VERB_ENABLE
    case AMP_CFS_EVENT_FOPEN:
    {
        AMP_CFS_OP_TYPE1_s *pm = (AMP_CFS_OP_TYPE1_s *)param;
        LOG_V_MSG("AMP_CFS_EVENT_FOPEN. File: %s", pm->File);
        break;
    }
    case AMP_CFS_EVENT_FCLOSE:
    {
        AMP_CFS_OP_TYPE1_s *pm = (AMP_CFS_OP_TYPE1_s *)param;
        LOG_V_MSG("AMP_CFS_EVENT_FCLOSE. File: %s", pm->File);
        break;
    }
    case AMP_CFS_EVENT_LOWSPEED:
    {
        AMP_CFS_OP_TYPE2_s *pm6 = (AMP_CFS_OP_TYPE2_s *)param;
        LOG_V_MSG("AMP_CFS_EVENT_LOWSPEED. Drive: %c", pm6->Drive);
        break;
    }
    case AMP_CFS_EVENT_REMOVE:
    {
        AMP_CFS_OP_TYPE1_s *pm1;
        pm1 = (AMP_CFS_OP_TYPE1_s *)param;
        LOG_V_MSG("AMP_CFS_EVENT_REMOVE: filename=%s", pm1->File);
        Rval = AMP_OK;
        break;
    }
#endif
    default:
        LOG_V_MSG("Unknown operation: %d", opCode);
        break;
    }

    return Rval;
}

static int RegisterEvtHandler(AMP_CFS_EVENT_HDLR_FP evtHdlr)
{
    CfsEventHandler = evtHdlr;
    return AMP_OK;
}

static int UnregisterEventHandler(AMP_CFS_EVENT_HDLR_FP evtHdlr)
{
    CfsEventHandler = NULL;
    return AMP_OK;
}

static int AmpUT_CfsGetAmbaFsError(void) {
    int ENum;
    AmbaFS_GetError(&ENum);
    return ENum;
}

static AMP_CFS_DTA_s* AmpUT_CfsGetDta(UINT8 idx, BOOL8 isNew)
{
    if (idx < AMP_CFS_UT_DTA_NUM) {
        AMP_CFS_DTA_s *Dta = g_AmpCfsUnitTest.CfsDta + idx;
        if (isNew) {
            memset(Dta, 0, sizeof(AMP_CFS_DTA_s));
        }
        return Dta;
    }
    LOG_FAIL("Invalid index to access the preserved CFS DTAs.");
    return NULL;
}

static AMBA_FS_DTA_t* AmpUT_CfsGetAmbaFsDta(BOOL8 isNew)
{
    AMBA_FS_DTA_t *Dta = g_AmpCfsUnitTest.AmbaFsDta;
    if (isNew) {
        memset(Dta, 0, sizeof(AMP_CFS_DTA_s));
    }
    return Dta;
}

AMP_CFS_DRIVE_e AmpUT_CfsGetDrive(const char *path)
{
    if (IS_FULL_PATH(path)) {
        return path[0];
    }
    return 0;
}

int AmpUT_CfsGetDriveClusterSize(AMP_CFS_DRIVE_e driveName)
{
    AMP_CFS_DRIVE_INFO_s Info;
    if (AmpCFS_GetDriveInfo(driveName, &Info) != AMP_OK) {
        return -1;
    }
    return AmpUT_CfsGetDriveBytesPerCluster(Info);
}

static int AmpUT_CfsAllocMemFormPool(UINT8 **addr, UINT32 size)
{
    void *RawAddr = NULL;
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)addr, &RawAddr, size, AMBA_CACHE_LINE_SIZE) != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    memset(*addr, 0, size);
    return AMP_OK;
}

#if AMP_CFS_UT_COMPILE_WARNING
/**
 * CFS UT - initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsInit(void)
{
    void *cfshdlrAddr = NULL;
    UINT32 Size;

    if (g_AmpCfsUnitTest.bInit) {
       return AMP_OK;
    }

#if 0
    AMP_CFS_CFG_s cfsCfg;

    AmbaPrint("==%s==", __FUNCTION__);
    /** Cfs Init */
    if (AmpCFS_GetDefaultCfg(&cfsCfg) != AMP_OK) {
        AmbaPrint("%s: Can't get default config.", __FUNCTION__);
        return AMP_ERROR_GENERAL_ERROR;
    }
    cfsCfg.SchBankAmount = AMP_CFS_UT_MAX_BANK_AMOUNT;
    cfsCfg.CacheMaxFileNum = AMP_CFS_UT_MAX_FILE;
    cfsCfg.BufferSize = AmpCFS_GetRequiredBufferSize(cfsCfg.SchBankSize, \
                                               cfsCfg.SchBankAmount, cfsCfg.SchStackSize, cfsCfg.SchTaskAmount, \
                                               cfsCfg.CacheEnable, cfsCfg.CacheMaxFileNum);
    /** Allocate memory */
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **) &cfsCfg.Buffer, &cfshdlrAddr, cfsCfg.BufferSize + AMP_CFS_UT_BIG_TEST_BUF_SIZE, AMBA_CACHE_LINE_SIZE) != AMP_OK) {
        AmbaPrint("%s: Can't allocate memory.", __FUNCTION__);
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Set call back function*/
    cfsCfg.FileOperation = AmpUT_CfsFileOperation;
    //cfsCfg.FileOperation = AmpUT_CfsEventHandlers;
    /** Initialize CFS */
    if (AmpCFS_Init(&cfsCfg) != AMP_OK) {
        AmbaPrint("%s: Can't initialize.", __FUNCTION__);
        AmbaKAL_BytePoolFree(cfshdlrAddr);
        return AMP_ERROR_GENERAL_ERROR;
    }
#endif

    /** Get test buffer for unit test */
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **) &g_AmpCfsUnitTest.pBufferTest, &cfshdlrAddr, AMP_CFS_UT_BIG_TEST_BUF_SIZE, AMBA_CACHE_LINE_SIZE) != AMP_OK) {
        AmbaPrint("%s: Can't allocate memory.", __FUNCTION__);
        return AMP_ERROR_GENERAL_ERROR;
    }
    //g_AmpCfsUnitTest.pBufferTest = (UINT8 *)cfsCfg.Buffer+cfsCfg.BufferSize;
    memset(g_AmpCfsUnitTest.pBufferTest, 0, AMP_CFS_UT_BIG_TEST_BUF_SIZE);

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **) &g_AmpCfsUnitTest.STWriteBuffer, &cfshdlrAddr, AMP_CFS_UT_ST_BUF_SIZE, AMBA_CACHE_LINE_SIZE) != AMP_OK) {
        LOG_FAIL("Failed to get the buffer space for stress test.");
        return AMP_ERROR_GENERAL_ERROR;
    }
    memset(g_AmpCfsUnitTest.STWriteBuffer, 0, AMP_CFS_UT_ST_BUF_SIZE);

    Size = sizeof(AMP_CFS_UT_ST_CMD_PARAMS_s);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **) &g_AmpCfsUnitTest.STCmdParamsAligned, &g_AmpCfsUnitTest.STCmdParamsRaw, Size, AMBA_CACHE_LINE_SIZE) != AMP_OK) {
        LOG_FAIL("Failed to get the space of test command parameters");
        return AMP_ERROR_GENERAL_ERROR;
    }
    memset(g_AmpCfsUnitTest.STCmdParamsAligned, 0, Size);

    Size = sizeof(AMP_CFS_UT_ST_TASK_INFO_s);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **) &g_AmpCfsUnitTest.TaskInfo, &g_AmpCfsUnitTest.TaskInfoRaw, Size, AMBA_CACHE_LINE_SIZE) != AMP_OK) {
        LOG_FAIL("Failed to get the space of task info objects");
        return AMP_ERROR_GENERAL_ERROR;
    }
    memset(g_AmpCfsUnitTest.TaskInfo, 0, Size);

    g_AmpCfsUnitTest.bInit = TRUE;
    return AMP_OK;
}
#endif

/**
 * CFS UT - open function.
 *
 * @param [in] pwcFile file name.
 * @param [in] uMode open file mode.
 * @param [in] uAlignment alignment parameter.
 * @param [in] uByteToSync number of bytes to sync.
 * @param [in] bAsyncMode async mode.
 * @param [in] MaxNumBank max number of bank.
 * @param [in] bPriority file pripority.
 * @return AMP_CFS_FILE_S
 *
 */
static AMP_CFS_FILE_s* AmpUT_CfsOpen(char *fileName, UINT8 uMode, UINT32 uAlignment, UINT32 uByteToSync, BOOL8 bAsyncMode, UINT8 MaxNumBank, BOOL8 bPriority)
{
    AMP_CFS_FILE_s *Rval = NULL;
    AMP_CFS_FILE_PARAM_s FileDesc;
    int Status;

    /** Check file availibility */
    if (bAsyncMode == TRUE) {
        do {
            Status = AmpCFS_FStatus((const char *)fileName);
            AmbaKAL_TaskSleep(10);
        } while (Status != AMP_CFS_STATUS_UNUSED && Status != AMP_CFS_STATUS_ERROR);
    }
    /** Get file parameters */
    if (AmpCFS_GetFileParam(&FileDesc) != AMP_OK) {
        AmbaPrint("%s: Can't get file descriptor.", __FUNCTION__);
        return NULL;
    }
    /** Set file parameters */
    strncpy(FileDesc.Filename, fileName, MAX_FILENAME_LENGTH);
    FileDesc.Mode = uMode;
    FileDesc.Alignment = uAlignment;
    FileDesc.BytesToSync = uByteToSync;
    FileDesc.AsyncMode = bAsyncMode;
    if (FileDesc.AsyncMode)
        FileDesc.AsyncData.MaxNumBank = MaxNumBank;
    FileDesc.LowPriority = bPriority;
    /** If the file is not opened, open it. */
    Status = AmpCFS_FStatus(fileName);
    if (Status == AMP_CFS_STATUS_UNUSED || Status == AMP_CFS_STATUS_ERROR) {
        Rval = AmpCFS_fopen(&FileDesc);
    } else {
        AmbaPrint("File is already opened!");
    }
    return Rval;
}

/**
 * CFS UT - write function.
 *
 * @param [in] pBuffer data buffer
 * @param [in] uSize data size
 * @param [in] uCount data count
 * @param [in] pFile file handler
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsWrite(const void *pBuffer, UINT64 uSize, UINT64 uCount, AMP_CFS_FILE_s *pFile)
{
    /** Write data to file */
    if (AmpCFS_fwrite(pBuffer, uSize, uCount, pFile) == uCount) {
        return AMP_OK;
    }
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * CFS UT - read function.
 *
 * @param [in] pBuffer data buffer
 * @param [in] uSize data size
 * @param [in] uCount data count
 * @param [in] pFile file handler
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsRead(void *pBuffer, UINT64 uSize, UINT64 uCount, AMP_CFS_FILE_s *pFile)
{
    /** Read data from file */
    if (AmpCFS_fread(pBuffer, uSize, uCount, pFile) == uCount) {
        return AMP_OK;
    }
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * CFS UT - seek function.
 *
 * @param [in] pFile file handler
 * @param [in] lOffest file offset
 * @param [in] iOrigin seek point
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsSeek(AMP_CFS_FILE_s *pFile, INT64 lOffset, int iOrigin)
{
    /** Seek file */
    return AmpCFS_fseek(pFile, lOffset, iOrigin);
}

/**
 * CFS UT - ftell function.
 *
 * @param [in] pFile file pointer
 * @return tell.
 *
 */
static INT64 AmpUT_Cfsftell(AMP_CFS_FILE_s *pFile)
{
    /** Get file position */
    return AmpCFS_ftell(pFile);
}

/**
 * CFS UT - FGetLen function.
 *
 * @param [in] pFile file pointer
 * @return file length.
 *
 */
static INT64 AmpUT_CfsFGetLen(AMP_CFS_FILE_s *pFile)
{
    /** Get file length */
    return AmpCFS_FGetLen(pFile);
}

/**
 * CFS UT - FStatus function.
 *
 * @param [in] pwcFile file name
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsFStatus(const char *fileName)
{
    /** Get file status */
    return AmpCFS_FStatus(fileName);
}

/**
 * CFS UT - FSync function.
 *
 * @param [in] pFile file name
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsFSync(AMP_CFS_FILE_s *pFile)
{
    /** Sync file */
    return AmpCFS_Fsync(pFile);
}

/**
 * CFS UT - close function.
 *
 * @param [in] pFile file name
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsClose(AMP_CFS_FILE_s *pFile)
{
    /** Close file */
    return AmpCFS_fclose(pFile);
}

/**
 * CFS UT - remove function.
 *
 * @param [in] pwcFile file name
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Cfsremove(const char *fileName)
{
    /** Remove file */
    return AmpCFS_remove(fileName);
}

/**
 * CFS UT - move function.
 *
 * @param [in] pwcSrcFile source file name
 * @param [in] pwcDstFile source file name
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsMove(const char *srcFileName, const char *dstFileName)
{
    /** Move file */
    return AmpCFS_Move(srcFileName, dstFileName);
}

/**
 * CFS UT - stat function.
 *
 * @param [in] pwcFile file name
 * @param [in] pStat file state.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsStat(const char *fileName, AMP_CFS_FILE_INFO_s *pStat)
{
    /** Get file stat */
    return AmpCFS_GetFileInfo(fileName, pStat);
}

/**
 * CFS UT - Chmod function.
 *
 * @param [in] pwcFile file name
 * @param [in] iAttr attribute
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsChmod(const char *fileName, int iAttr)
{
    /** Change mode */
    return AmpCFS_ChangeFileMode(fileName, iAttr);
}

/**
 * CFS UT - search first dir entry function.
 *
 * @param [in] pwcDir dir name
 * @param [in] uAttr attribute
 * @param [in] pDta data type
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsFirstDirEnt(const char *dirName, UINT8 uAttr, AMP_CFS_DTA_s *pDta)
{
    int Ret;
    /** Get first directory entry */
    if ((Ret = AmpCFS_FileSearchFirst(dirName, uAttr, pDta)) != AMP_OK) {
        if (Ret == AMP_ERROR_OBJ_UNAVAILABLE) {
            LOG_PRT("No such file or directory.");
        } else {
            LOG_FAIL("File search failed");
        }
    }
    return Ret;
}

static int AmpUT_CfsAmbsFsFirstDirEnt(const char *name, UINT8 attr, AMBA_FS_DTA_t *dta)
{
    if (AmbaFS_FileSearchFirst(name, attr, dta) != OK) {
        int ENum = AmpUT_CfsGetAmbaFsError();
        if (ENum == PF_ERR_ENOENT) {
            LOG_PRT("No such file or directory.");
            return AMP_ERROR_IO_ERROR;
        } else {
            LOG_FAIL("File search failed. ErrNUM: %d", ENum);
            return AMP_ERROR_GENERAL_ERROR;
        }
    }
    return AMP_OK;
}

/**
 * CFS UT - search next dir entry function.
 *
 * @param [in] pDta data type
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsNextDirEnt(AMP_CFS_DTA_s *pDta)
{
    int Ret;
    /** Get next directory entry */
    if ((Ret = AmpCFS_FileSearchNext(pDta)) != AMP_OK) {
        if (Ret == AMP_ERROR_OBJ_UNAVAILABLE) {
            LOG_PRT("No such file or directory.");
        } else {
            LOG_FAIL("File search failed");
        }
    }
    return Ret;
}

static int AmpUT_CfsAmbsFsNextDirEnt(AMBA_FS_DTA_t *dta)
{
    if (AmbaFS_FileSearchNext(dta) != OK) {
        int ENum = AmpUT_CfsGetAmbaFsError();
        if (ENum == PF_ERR_ENOENT) {
            LOG_PRT("No such file or directory.");
            return AMP_ERROR_IO_ERROR;
        } else {
            LOG_FAIL("File search failed. ErrNUM: %d", ENum);
            return AMP_ERROR_GENERAL_ERROR;
        }
    }
    return AMP_OK;
}

/**
 * CFS UT - combine function.
 *
 * @param [in] pwcBaseFile base file name
 * @param [in] pwcAddFile add file name
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsCombine(char *baseFileName, char *addFileName)
{
    /** Combine two files */
    return AmpCFS_FileCombine(baseFileName, addFileName);
}

/**
 * CFS UT - divide function.
 *
 * @param [in] pwcOrgFile ori file name
 * @param [in] pwcNewFile new file name
 * @param [in] uOffset file offset
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsDivide(char *orgFileName, char *nNewFileName, UINT32 uOffset)
{
    /** Divide a file into two files */
    return AmpCFS_FileDivide(orgFileName, nNewFileName, uOffset);
}

/**
 * CFS UT - insert function.
 *
 * @param [in] pwcFile file name
 * @param [in] uOffset file offset
 * @param [in] uNumber cluster number
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsCinsert(const char *fileName, UINT32 uOffset, UINT32 uNumber)
{
    /**< Insert clusters into a file */
    if (AmpCFS_ClusterInsert(fileName, uOffset, uNumber) == uNumber) {
        return AMP_OK;
    }
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * CFS UT - Cdelete function.
 *
 * @param [in] pwcFile file name
 * @param [in] uOffset file offset
 * @param [in] uNumber cluster number
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsCdelete(const char *fileName, UINT32 uOffset, UINT32 uNumber)
{
    /** Delete clusters from a file */
    if (AmpCFS_ClusterDelete(fileName, uOffset, uNumber) == uNumber) {
        return AMP_OK;
    }
    return AmpCFS_ClusterDelete(fileName, uOffset, uNumber);
}

/**
 * CFS UT - make dir function.
 *
 * @param [in] pwcDir dir name
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsMkdir(const char *dirName)
{
    /** Make a new directory */
    return AmpCFS_MakeDir(dirName);
}

/**
 * CFS UT - remove dir function.
 *
 * @param [in] pwcDir dir name
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsRmdir(const char *dirName)
{
    /** Remove a directory */
    return AmpCFS_RemoveDir(dirName);
}

/**
 * CFS UT - get device function.
 *
 * @param [in] wcDir dir name
 * @param [out] pDevInf device information
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsGetDev(char driveName, AMP_CFS_DRIVE_INFO_s *pDevInf)
{
    /** Get device information */
    return AmpCFS_GetDriveInfo(driveName, pDevInf);
}

/**
 * CFS UT - get device format function.
 *
 * @param [in] wcDir dir name
 * @param [in] Param address of a parameter record
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsFormat(char driveName, const char *param)
{
    /** Format a drive */
    return AmpCFS_Format(driveName, param);
}

/**
 * CFS UT - sync function.
 *
 * @param [in] wcDrive dir name
 * @param [in] iMode sync mode
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsSync(char driveName, int iMode)
{
    /** Synchronous a drive */
    return AmpCFS_Sync(driveName, iMode);
}

/**
 * CFS UT - show usage function.
 */
static int AmpUT_CfsUsage(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMP_CFS_UT_PRINT(RED, "Unavailable command.");
    AMP_CFS_UT_PRINT(BLACK, "Please enter CFS test command:");
    AMP_CFS_UT_PRINT(BLACK, "t cfs sync_auto");
    AMP_CFS_UT_PRINT(BLACK, "t cfs async_auto");
    AMP_CFS_UT_PRINT(BLACK, "t cfs stress_test [file size = 1:4095 (MiB)]");
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * CFS UT - amba write mode test function.
 *
 * @param [in] pwcFile file name
 * @param [in] pBuffer buffer
 * @param [in] iBufSize buffer size
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_AmbaWriteModeTest(char *fileName, UINT8 *pBuffer, int iBufSize)
{
    AMBA_FS_FILE *pFile = NULL;
    char fileMode[] = "w+";
    int Rval = AMP_OK;
    /** Open test */
    pFile = AmbaFS_fopen((const char *)fileName, fileMode);
    if (pFile != NULL) {
        AMP_CFS_UT_PRINT(GREEN, "fopen: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "fopen: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Write test */
    if (AmbaFS_fwrite(pBuffer, sizeof(char), iBufSize, pFile) == iBufSize) {
        AMP_CFS_UT_PRINT(GREEN, "fwrite: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "fwrite: Fail");
        Rval = AMP_ERROR_GENERAL_ERROR;
    }
    /** FSync */
    if (Rval == AMP_OK) {
        if (AmbaFS_Fsync(pFile) == OK) {
            AMP_CFS_UT_PRINT(GREEN, "FSync: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "FSync: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** ftell test */
    if (Rval == AMP_OK) {
        if (AmbaFS_ftell(pFile) == iBufSize) {
            AMP_CFS_UT_PRINT(GREEN, "ftell: Pass.");
        } else {
            AMP_CFS_UT_PRINT(RED, "ftell: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Seek test */
    if (Rval == AMP_OK) {
        if (AmbaFS_fseek(pFile, 0, AMBA_FS_SEEK_START) == OK) {
            AMP_CFS_UT_PRINT(GREEN, "fseek: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "fseek: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Read test */
    if (Rval == AMP_OK) {
        if (AmbaFS_fread(pBuffer, sizeof(char), iBufSize, pFile) == iBufSize) {
            AMP_CFS_UT_PRINT(GREEN, "fread: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "fread: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Close test */
    if (AmbaFS_fclose(pFile) == OK) {
        AMP_CFS_UT_PRINT(GREEN, "fclose: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "fclose: Fail");
        Rval = AMP_ERROR_GENERAL_ERROR;
    }
    return Rval;
}

/**
 * CFS UT - write mode test function.
 *
 * @param [in] pwcFile file name
 * @param [in] bAsyncMode async mode
 * @param [in] MaxNumbBank max number of bank
 * @param [in] pBuffer buffer
 * @param [in] iBufSize buffer size
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsWriteModeTest(char *fileName, BOOL8 bAsyncMode, UINT8 MaxNumBank, UINT8 *pBuffer, int iBufSize)
{
    int Rval = AMP_OK;
    AMP_CFS_FILE_s *pFile = NULL;
    /** Open test */
    pFile = AmpUT_CfsOpen(fileName, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, bAsyncMode, MaxNumBank, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile != NULL) {
        AMP_CFS_UT_PRINT(GREEN, "fopen: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "fopen: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Fappend test */
    if (AmpCFS_ClusterAppend(pFile, iBufSize) == iBufSize) {
        AMP_CFS_UT_PRINT(GREEN, "fappend: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "fappend: Fail");
        Rval = AMP_ERROR_GENERAL_ERROR;
    }
    /** Write test */
    if (Rval == AMP_OK) {
        if (AmpUT_CfsWrite(pBuffer, sizeof(UINT8), iBufSize, pFile) == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "fwrite: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "fwrite: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** FSync */
    if (Rval == AMP_OK) {
        if (AmpCFS_Fsync(pFile) == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "FSync: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "FSync: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** ftell test */
    if (Rval == AMP_OK) {
        if (AmpCFS_ftell(pFile) == iBufSize) {
            AMP_CFS_UT_PRINT(GREEN, "ftell: Pass.");
        } else {
            AMP_CFS_UT_PRINT(RED, "ftell: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** fGetLen test */
    if (Rval == AMP_OK) {
        UINT64 ulLen = iBufSize;
        if (AMP_CFS_UT_ALIGNMENT_DEFAULT != 0) {
            UINT64 ulAlignment = AMP_CFS_UT_ALIGNMENT_DEFAULT;
            ulLen = ((iBufSize + (ulAlignment - 1)) / ulAlignment) * ulAlignment;
        }
        if (AmpCFS_FGetLen(pFile) == ulLen) {
            AMP_CFS_UT_PRINT(GREEN, "flen: Pass.");
        } else {
            AMP_CFS_UT_PRINT(RED, "flen: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Seek test */
    if (Rval == AMP_OK) {
        if (AmpUT_CfsSeek(pFile, 0, AMP_CFS_SEEK_START) == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "fseek: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "fseek: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Read test */
    if (Rval == AMP_OK) {
        if (AmpUT_CfsRead(pBuffer, sizeof(UINT8), iBufSize, pFile) == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "fread: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "fread: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Close test */
    if (AmpUT_CfsClose(pFile) == AMP_OK) {
        AMP_CFS_UT_PRINT(GREEN, "fclose: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "fclose: Fail");
        Rval = AMP_ERROR_GENERAL_ERROR;
    }
    return Rval;
}

/**
 * CFS UT - amba read mode test function.
 * @param [in] pwcFile file name
 * @param [in] pBuffer buffer
 * @param [in] nBufferSize buffer size
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_AmbaReadModeTest(char *fileName, UINT8 *pBuffer, int nBufferSize)
{
    int Rval = AMP_OK;
    AMBA_FS_FILE *pFile = NULL;
    char pwcMode[] = "r";
    /** Open test */
    pFile = AmbaFS_fopen((const char *)fileName, (char *)pwcMode);
    if (pFile != NULL) {
        AMP_CFS_UT_PRINT(GREEN, "fopen: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "fopen: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Ftell test */
    if (AmbaFS_ftell(pFile) == 0) {
        AMP_CFS_UT_PRINT(GREEN, "ftell: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "ftell: Fail");
        Rval = AMP_ERROR_GENERAL_ERROR;
    }
    /** Seek test */
    if (Rval == AMP_OK) {
        if (AmbaFS_fseek(pFile, nBufferSize-1, AMBA_FS_SEEK_START) == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "fseek: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "fseek: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Ftell test */
    if (Rval == AMP_OK) {
        if (AmbaFS_ftell(pFile) == (nBufferSize-1)) {
            AMP_CFS_UT_PRINT(GREEN, "ftell: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "ftell: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Seek test */
    if (Rval == AMP_OK) {
        if (AmbaFS_fseek(pFile, 0, AMBA_FS_SEEK_START) == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "fseek: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "fseek: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Read test */
    if (Rval == AMP_OK) {
        if (AmbaFS_fread(pBuffer, sizeof(char), nBufferSize, pFile) == nBufferSize) {
            AMP_CFS_UT_PRINT(GREEN, "fread: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "fread: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Close test */
    if (AmbaFS_fclose(pFile) == AMP_OK) {
        AMP_CFS_UT_PRINT(GREEN, "fclose: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "fclose: Fail");
        Rval = AMP_ERROR_GENERAL_ERROR;
    }
    return Rval;
}

/**
 * CFS UT - read mode test function.
 *
 * @param [in] pwcFile file name
 * @param [in] bAsyncMode async mode
 * @param [in] MaxNumbBank max number of bank
 * @param [in] pBuffer buffer
 * @param [in] nBufferSize buffer size
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsReadModeTest(char *fileName, BOOL8 bAsyncMode, UINT8 MaxNumBank, UINT8 *pBuffer, int nBufferSize)
{
    int Rval = AMP_OK;
    AMP_CFS_FILE_s *pFile = NULL;
     /** Open test */
    pFile = AmpUT_CfsOpen(fileName, AMP_CFS_FILE_MODE_READ_ONLY, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, bAsyncMode, MaxNumBank, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile != NULL) {
        AMP_CFS_UT_PRINT(GREEN, "fopen: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "fopen: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Ftell test */
    if (AmpCFS_ftell(pFile) == 0) {
        AMP_CFS_UT_PRINT(GREEN, "ftell: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "ftell: Fail");
        Rval = AMP_ERROR_GENERAL_ERROR;
    }
    /** Seek test */
    if (Rval == AMP_OK) {
        if (AmpUT_CfsSeek(pFile, nBufferSize-1, AMP_CFS_SEEK_START) == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "fseek: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "fseek: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Ftell test */
    if (Rval == AMP_OK) {
        if (AmpCFS_ftell(pFile) == (nBufferSize-1)) {
            AMP_CFS_UT_PRINT(GREEN, "ftell: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "ftell: Fail" );
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Seek test */
    if (Rval == AMP_OK) {
        if (AmpUT_CfsSeek(pFile, 0, AMP_CFS_SEEK_START) == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "fseek: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "fseek: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Read test */
    if (Rval == AMP_OK) {
        if (AmpUT_CfsRead(pBuffer, sizeof(UINT8), nBufferSize, pFile) == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "fread: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "fread: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** fGetLen test */
    if (Rval == AMP_OK) {
        if (AmpCFS_FGetLen(pFile) == nBufferSize) {
            AMP_CFS_UT_PRINT(GREEN, "flen: Pass");
        } else {
            AMP_CFS_UT_PRINT(RED, "flen: Fail");
            Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Close test */
    if (AmpUT_CfsClose(pFile) == AMP_OK) {
        AMP_CFS_UT_PRINT(GREEN, "fclose: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "fclose: Fail");
        Rval = AMP_ERROR_GENERAL_ERROR;
    }
    return Rval;
}

#define PRIORITY_TEST_NUM_FILE 4 /**< number of file for test */

/**
 * CFS UT - priority test function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_CfsPriorityTest(void)
{
    int Rval = AMP_OK;
    AMP_CFS_FILE_s *pFiles[PRIORITY_TEST_NUM_FILE];
    char  fileNames[PRIORITY_TEST_NUM_FILE][MAX_FILENAME_LENGTH]= {"C:\\SYNC_RT.txt", "C:\\ASYNC_RT.txt", "C:\\SYNC_NRT.txt", "C:\\ASYNC_NRT.txt"};
    BOOL8  bSyncModes[PRIORITY_TEST_NUM_FILE] =  {AMP_CFS_FILE_SYNC_MODE, AMP_CFS_FILE_ASYNC_MODE, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_FILE_ASYNC_MODE};
    BOOL8  bPriorities[PRIORITY_TEST_NUM_FILE] = {FALSE, FALSE, TRUE, TRUE};
    int i, j, k;

    /** Open multiple files */
    AmbaPrint("open:");
    for (i=0;(i<PRIORITY_TEST_NUM_FILE)&&(Rval==AMP_OK);i++) {
        pFiles[i] = AmpUT_CfsOpen(fileNames[i], AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, bSyncModes[i], AMP_CFS_UT_MAX_BANK_AMOUNT / PRIORITY_TEST_NUM_FILE, bPriorities[i]);
        if (pFiles[i] == NULL) {
            AMP_CFS_UT_PRINT(RED, "open %dth file : Fail", i);
            for (j=0;j<i;j++) {
                if (AmpUT_CfsClose(pFiles[j]) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close %dth file: Fail", j);
               }
            }
            return AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** write test */
    AmbaPrint("write test");
    for (i=0;i<AMP_CFS_UT_BIG_TEST_BUF_SIZE;i++) {
        g_AmpCfsUnitTest.pBufferTest[i] = i % AMP_CFS_UT_BYTE_TEST_BASE;
    }
    for (i=0;(i<10)&&(Rval==AMP_OK);i++) {
        g_AmpCfsUnitTest.pBufferTest[0] = i;
        for (j=0;(j<PRIORITY_TEST_NUM_FILE)&&(Rval==AMP_OK);j++) {
            if (AmpUT_CfsWrite(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), AMP_CFS_UT_BIG_TEST_BUF_SIZE, pFiles[j]) != AMP_OK) {
               AMP_CFS_UT_PRINT(RED, "write %dth file : Fail", j);
               Rval = AMP_ERROR_GENERAL_ERROR;
               break;
            }
        }
    }
    /** Seek test */
    AmbaPrint("seek test");
    for (i=0;(i<PRIORITY_TEST_NUM_FILE)&&(Rval==AMP_OK);i++) {
        if (AmpUT_CfsSeek(pFiles[i], 0, 0) != AMP_OK) {
           AMP_CFS_UT_PRINT(RED, "write %d: Fail", i);
           Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    // write again
    g_AmpCfsUnitTest.pBufferTest[0] = 0;
    for (j=0;(j<PRIORITY_TEST_NUM_FILE)&&(Rval==AMP_OK);j++) {
        if (AmpUT_CfsWrite(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), AMP_CFS_UT_BIG_TEST_BUF_SIZE, pFiles[j]) != AMP_OK) {
           AMP_CFS_UT_PRINT(RED, "write %dth file : Fail", j);
           Rval = AMP_ERROR_GENERAL_ERROR;
           break;
        }
    }
    // seek to head again
    for (i=0;(i<PRIORITY_TEST_NUM_FILE)&&(Rval==AMP_OK);i++) {
        if (AmpUT_CfsSeek(pFiles[i], 0, 0) != AMP_OK) {
           AMP_CFS_UT_PRINT(RED, "write %d: Fail", i);
           Rval = AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Read test */
    AmbaPrint("read test");
    for (i=0;(i<10)&&(Rval==AMP_OK);i++) {
        for (j=0;(j<PRIORITY_TEST_NUM_FILE)&&(Rval==AMP_OK);j++) {
            if (AmpUT_CfsRead(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), AMP_CFS_UT_BIG_TEST_BUF_SIZE, pFiles[j]) != AMP_OK) {
                AMP_CFS_UT_PRINT(RED, "read %dth file : Fail", j);
                Rval = AMP_ERROR_GENERAL_ERROR;
            }
            if (g_AmpCfsUnitTest.pBufferTest[0] != i) {
                AMP_CFS_UT_PRINT(RED, "read byte %d: Fail", 0);
                Rval = AMP_ERROR_GENERAL_ERROR;
            }
            for (k=1;k<AMP_CFS_UT_BIG_TEST_BUF_SIZE;k++) {
                if (g_AmpCfsUnitTest.pBufferTest[k] != k % AMP_CFS_UT_BYTE_TEST_BASE) {
                   AMP_CFS_UT_PRINT(RED, "read byte %d: Fail", k);
                   Rval = AMP_ERROR_GENERAL_ERROR;
                   break;
                }
            }
        }
    }
    /** Close test */
    AmbaPrint("close test:");
    for (i=0;i<PRIORITY_TEST_NUM_FILE;i++) {
        if (AmpUT_CfsClose(pFiles[i]) != AMP_OK) {
           AMP_CFS_UT_PRINT(RED, "close %dth file: Fail", i);
           Rval = AMP_ERROR_GENERAL_ERROR;
       }
    }
    if (Rval == AMP_OK) {
        AMP_CFS_UT_PRINT(GREEN, "Pass");
    }
    return Rval;
}

/**
 * CFS UT - cache devide verification function.
 *
 * @param [in] cDrive Drive name
 * @return TRUE: ok, FALSE: error
 *
 */
static BOOL AmpUT_CacheDevInfVerification(char driveName)
{
    AMBA_FS_DRIVE_INFO_t DevInf;
    AMP_CFS_DRIVE_INFO_s cDevInf;
    int Rval1, Rval2;
    /** Clear data structure */
    memset(&DevInf,0,sizeof(AMBA_FS_DRIVE_INFO_t));
    memset(&cDevInf,0,sizeof(AMP_CFS_DRIVE_INFO_s));
    /** Get Dev Compare */
    Rval1 = AmbaFS_GetDriveInfo(driveName, &DevInf);
    Rval2 = AmpUT_CfsGetDev(driveName, &cDevInf);
    if (Rval1 == Rval2) {
        if (Rval1 != AMP_OK) {
            return TRUE;
        } else {
            if (memcmp(&DevInf, &cDevInf, sizeof(AMP_CFS_DRIVE_INFO_s)) == 0) {
                return TRUE;
            } else {
                AMP_CFS_UT_PRINT(RED,"Memory compare fail!");
                AmbaPrint("Data from Amba_FS");
                AmpUT_CfsPrintDriveInfoAmbaFs(&DevInf);
                AmbaPrint("Data from Amp_CFS");
                AmpUT_CfsPrintDriveInfoCfs(&cDevInf);
            }
        }
    } else AMP_CFS_UT_PRINT(RED, "%s %u, AmpCFS_GetDriveInfo and AmbaFS_GetDriveInfo are different!", __FUNCTION__, __LINE__);
    return FALSE;
}

/**
 * CFS UT - cache test device function.
 *
 * @param [in] cDrive Drive name
 *
 */
static int AmpUT_CacheTestDevInf(char cDrive)
{
    char pwcFile1[MAX_FILENAME_LENGTH] = "C:\\CT_0000.TXT";
    char pwcFile2[MAX_FILENAME_LENGTH] = "C:\\CT_0001.TXT";
    char pwcDir[MAX_FILENAME_LENGTH] = "C:\\CT_DIR";
    char  cBuf[16] = "This is a test.";
    AMP_CFS_FILE_s *pFile;
    /** Clear Cache */
    AmpCFS_ClearCache('C');
    /** Get Dev Test */
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("First compare: Pass!");
    } else {
        AMP_CFS_UT_PRINT(RED, "Cached GetDev Test: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get Dev Test after fopen with write mode */
    pFile = AmpUT_CfsOpen(pwcFile1, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) {
        AMP_CFS_UT_PRINT(RED, "%s %u, Open file: Fail!", __FUNCTION__, __LINE__);
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("Device Information compare after fopen: Pass!");
    } else {
        AMP_CFS_UT_PRINT(RED, "Device Information compare after fopen: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** fwrite */
    if (AmpUT_CfsWrite(cBuf, sizeof(char), sizeof(cBuf) , pFile) != AMP_OK) {
        AMP_CFS_UT_PRINT(RED, "write file: Fail");
        if (AmpUT_CfsClose(pFile) != AMP_OK) {
           AMP_CFS_UT_PRINT(RED, "close file: Fail");
        }
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get Dev Test after fclose */
    if (AmpUT_CfsClose(pFile)!=AMP_OK) {
        AMP_CFS_UT_PRINT(RED, "Close file: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("Device Information compare after fclose: Pass!");
    } else {
        AmbaPrint("Device Information compare after fclose: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get Dev Test after chmod */
    if (AmpUT_CfsChmod(pwcFile1, AMP_CFS_MODE_ATTR_ADD|AMP_CFS_ATTR_HIDDEN) != AMP_OK) {
        AMP_CFS_UT_PRINT(RED, "chmod file: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("Device Information compare after chmod: Pass!");
    } else {
        AmbaPrint("Device Information compare after chmod: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get Dev Test after move */
    if (AmpUT_CfsMove(pwcFile1, pwcFile2) != AMP_OK) {
        AMP_CFS_UT_PRINT(RED, "move file: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("Device Information compare after move: Pass!");
    } else {
        AmbaPrint("Device Information compare after move: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get Dev Test after cinsert */
    if (AmpUT_CfsCinsert(pwcFile2, 1, 2) != AMP_OK) {
        AMP_CFS_UT_PRINT(RED, "cinsert file: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("Device Information compare after cinsert: Pass!");
    } else {
        AmbaPrint("Device Information compare after cinsert: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get Dev Test after cdelete */
    if (AmpUT_CfsCdelete(pwcFile2, 2, 1) != AMP_OK) {
        AMP_CFS_UT_PRINT(RED, "cdelete file: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("Device Information compare after cdelete: Pass!");
    } else {
        AmbaPrint("Device Information compare after cdelete: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get Dev Test after divide */
    if (AmpUT_CfsDivide(pwcFile2, pwcFile1, 1024) != AMP_OK) {
        AMP_CFS_UT_PRINT(RED, "divide file: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("Device Information compare after divide: Pass!");
    } else {
        AmbaPrint("Device Information compare after divide: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get Dev Test after combine */
    if (AmpUT_CfsCombine(pwcFile1, pwcFile2) != AMP_OK) {
        AMP_CFS_UT_PRINT(RED, "combine file: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("Device Information compare after combine: Pass!");
    } else {
        AmbaPrint("Device Information compare after combine: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get Dev Test after remove */
    if (AmpUT_Cfsremove(pwcFile1) != AMP_OK) {
        AMP_CFS_UT_PRINT(RED, "remove file: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("Device Information compare after remove: Pass!");
    } else {
        AmbaPrint("Device Information compare after remove: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get Dev Test after mkdir */
    if (AmpUT_CfsMkdir(pwcDir) != AMP_OK) {
        AMP_CFS_UT_PRINT(RED, "mkdir: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("Device Information compare after mkdir: Pass!");
    } else {
        AmbaPrint("Device Information compare after mkdir: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get Dev Test after rmdir */
    if (AmpUT_CfsRmdir(pwcDir) != AMP_OK) {
        AMP_CFS_UT_PRINT(RED, "rmdir: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CacheDevInfVerification(cDrive)==TRUE) {
        AmbaPrint("Device Information compare after rmdir: Pass!");
    } else {
        AmbaPrint("Device Information compare after rmdir: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    return AMP_OK;
}

/**
 * CFS UT - cache stat verification function.
 *
 * @param [in] pwcFile file name
 * @return TRUE: ok, FALSE: fail
 *
 */
static BOOL AmpUT_CacheStatVerification(char *fileName)
{
    AMBA_FS_FILE_INFO_s Stat;
    AMP_CFS_FILE_INFO_s cStat;
    int Rval1, Rval2;

    /** Clear data structure */
    memset(&Stat,0,sizeof(AMBA_FS_FILE_INFO_s));
    memset(&cStat,0,sizeof(AMP_CFS_FILE_INFO_s));
    /** Stat Compare */
    Rval1 = AmpCFS_GetFileInfo(fileName, &cStat);
    Rval2 = AmbaFS_GetFileInfo(fileName, &Stat);
    if (Rval1 == Rval2) {
        if (Rval1 != AMP_OK) {
            return TRUE;
        } else {
            if (memcmp(&Stat, &cStat, sizeof(AMP_CFS_FILE_INFO_s)) == 0) {
                return TRUE;
            } else {
                AMP_CFS_UT_PRINT(RED,"Memory compare fail!");
                AmbaPrint("Data from Amba_FS");
                AmpUT_CfsPrintFileInfo((AMP_CFS_FILE_INFO_s *)&Stat);
                AmbaPrint("Data from Amp_CFS");
                AmpUT_CfsPrintFileInfo(&cStat);
            }
        }
    } else AMP_CFS_UT_PRINT(RED, "%s %u, AmpCFS_stat and AmbaFS_GetFileInfo not match!", __FUNCTION__, __LINE__);
    return FALSE;
}

/**
 * CFS UT - cache test stat function.
 *
 * @param [in] cDrive Drive name
 *
 */
static int AmpUT_CacheTestStat(char cDrive)
{
    char pwcL1D1[] = "C:\\L1D1";
    char pwcL2D1[] = "C:\\L1D1\\L2D1";
    char pwcL2D2[] = "C:\\L1D1\\L2D2";
    char pwcL2D3[] = "C:\\L1D1\\L2D3";
    char pwcL1F1[] = "C:\\L1D1\\L1D1F1";
    char pwcL1F2[] = "C:\\L1D1\\L1D1F2";
    char pwcL1F3[] = "C:\\L1D1\\L1D1F3";
    char pwcL2D1F1[] = "C:\\L1D1\\L2D1\\L2D1F1";
    char pwcL2D1F2[] = "C:\\L1D1\\L2D1\\L2D1F2";
    char pwcL2D1F3[] = "C:\\L1D1\\L2D1\\L2D1F3";
    char pwcL2D1F4[] = "C:\\L1D1\\L2D1\\L2D1F4";
    char pwcL1D1F4[] = "C:\\L1D1\\L1D1F4";
    char  cBuf[16] = "This is a test.";
    AMP_CFS_FILE_s *pFile;
    BOOL  bRlt;
    /** Clear Cache and files */
    AmpCFS_ClearCache('C');
    AmpUT_Cfsremove(pwcL1F1);
    AmpUT_Cfsremove(pwcL1F2);
    AmpUT_Cfsremove(pwcL1F3);
    AmpUT_Cfsremove(pwcL2D1F1);
    AmpUT_Cfsremove(pwcL2D1F2);
    AmpUT_Cfsremove(pwcL2D1F3);
    AmpUT_Cfsremove(pwcL2D1F4);
    AmpUT_Cfsremove(pwcL1D1F4);
    AmpUT_CfsRmdir(pwcL2D1);
    AmpUT_CfsRmdir(pwcL2D2);
    AmpUT_CfsRmdir(pwcL2D3);
    AmpUT_CfsRmdir(pwcL1D1);
    /** Create directories */
    bRlt = TRUE;
    if (AmpUT_CfsMkdir(pwcL1D1) != AMP_OK) bRlt = FALSE;
    if (AmpUT_CfsMkdir(pwcL2D1) != AMP_OK) bRlt = FALSE;
    if (AmpUT_CfsMkdir(pwcL2D2) != AMP_OK) bRlt = FALSE;
    if (bRlt == FALSE) return AMP_ERROR_GENERAL_ERROR;
    /** Create files */
    pFile = AmpUT_CfsOpen(pwcL1F1, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    pFile = AmpUT_CfsOpen(pwcL1F2, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    pFile = AmpUT_CfsOpen(pwcL1F3, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    pFile = AmpUT_CfsOpen(pwcL2D1F1, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    pFile = AmpUT_CfsOpen(pwcL2D1F2, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    pFile = AmpUT_CfsOpen(pwcL2D1F3, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    if (bRlt == TRUE) {
        /** Build file stat test */
        AmpCFS_ClearCache('C');
        AmbaPrint("Build nodes test!");
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        AmbaPrint("Search nodes test!");
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** File stat test after file operations */
        /** fopen, fwrite, fclose */
        AmbaPrint("File sate test after fopen, fwrite, fclose!");
        pFile = AmpUT_CfsOpen(pwcL2D1F4, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
        if (pFile == NULL) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CfsWrite(cBuf, sizeof(char), sizeof(cBuf) , pFile) != AMP_OK) {
            AmpUT_CfsClose(pFile);
            return AMP_ERROR_GENERAL_ERROR;
        }
        if (AmpUT_CfsClose(pFile)!=AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F4)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** chmod */
        AmbaPrint("File sate test after chmod!");
        if (AmpUT_CfsChmod(pwcL2D1F4, AMP_CFS_MODE_ATTR_ADD|AMP_CFS_ATTR_HIDDEN) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F4)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** move */
        AmbaPrint("File sate test after move!");
        if (AmpUT_CfsMove(pwcL2D1F4, pwcL1D1F4) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F4)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1D1F4)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** cinsert */
        AmbaPrint("File sate test after cinsert!");
        if (AmpUT_CfsCinsert(pwcL1D1F4, 1, 2) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1D1F4)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** cdelete */
        AmbaPrint("File sate test after cdelete!");
        if (AmpUT_CfsCdelete(pwcL1D1F4, 2, 1) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1D1F4)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** divide */
        AmbaPrint("File sate test after divide!");
        if (AmpUT_CfsDivide(pwcL1D1F4, pwcL2D1F4, 1024) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F4)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1D1F4)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** combine */
        AmbaPrint("File sate test after combine!");
        if (AmpUT_CfsCombine(pwcL2D1F4, pwcL1D1F4) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F4)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1D1F4)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** remove */
        AmbaPrint("File sate test after remove!");
        if (AmpUT_Cfsremove(pwcL2D1F4) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F4)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** mkdir */
        AmbaPrint("File sate test after mkdir!");
        if (AmpUT_CfsMkdir(pwcL2D3) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** rmdir */
        AmbaPrint("File sate test after rmdir!");
        if (AmpUT_CfsRmdir(pwcL2D3) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** Remove files */
        AmpUT_Cfsremove(pwcL1F1);
        AmpUT_Cfsremove(pwcL1F2);
        AmpUT_Cfsremove(pwcL1F3);
        AmpUT_Cfsremove(pwcL2D1F1);
        AmpUT_Cfsremove(pwcL2D1F2);
        AmpUT_Cfsremove(pwcL2D1F3);
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL1F3)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F1)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F2)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1F3)!=TRUE) return AMP_ERROR_GENERAL_ERROR;
        /** Remove directories */
        if (AmpUT_CfsRmdir(pwcL2D1) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CfsRmdir(pwcL2D2) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CfsRmdir(pwcL1D1) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Search file stat test */
        if (AmpUT_CacheStatVerification(pwcL1D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D1)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CacheStatVerification(pwcL2D2)!=TRUE)   return AMP_ERROR_GENERAL_ERROR;
    }
    return AMP_OK;
}

/**
 * CFS UT - cache dir entry compare function.
 *
 * @param [in] pDta dir entry
 * @param [in] pcDta dir entry
 * @return TRUE: ok, FALSE: fail
 *
 */
static BOOL AmpUT_CacheDirEntCompare(AMBA_FS_DTA_t *pDta, AMP_CFS_DTA_s *pcDta)
{
    BOOL bRlt = TRUE;
    bRlt &= (pDta->u.dta.p_vol == pcDta->u.dta.p_vol);
    bRlt &= (pDta->u.dta.parent_start_cluster == pcDta->u.dta.parent_start_cluster);
    bRlt &= (pDta->u.dta.attr == pcDta->u.dta.attr);
    bRlt &= (strcmp((char *)pDta->u.dta.reg_exp, (char *)pcDta->u.dta.reg_exp)==0);

    bRlt &= (pDta->FsType == pcDta->FsType);
    bRlt &= (pDta->Time == pcDta->Time);
    bRlt &= (pDta->Date == pcDta->Date);
    bRlt &= (pDta->FileSize == pcDta->FileSize);
    bRlt &= (pDta->StartClst == pcDta->StartClst);
    bRlt &= (strcmp(pDta->FileName, pcDta->FileName)==0);
    bRlt &= (strcmp(pDta->LongName, pcDta->LongName)==0);
    bRlt &= (pDta->Attribute == pcDta->Attribute);
    bRlt &= (pDta->SearchMode == pcDta->SearchMode);
    bRlt &= (pDta->pRomfsCurrentInode == pcDta->pRomfsCurrentInode);
    return bRlt;
}

/**
 * CFS UT - cache search first dir entry verification function.
 *
 * @param [in] pwcPattern pattern
 * @param [in] ucAttr attribute
 * @param [in] pDta dir entry
 * @param [in] pcDta dir entry
 * @return 0 - ok, others - fail
 *
 */
int AmpUT_CacheFirstDirEntVerification(char *pwcPattern, unsigned char ucAttr, AMBA_FS_DTA_t *pDta, AMP_CFS_DTA_s *pcDta)
{
    int Rval1, Rval2;
    /** Clear data structure */
    memset(pDta,0,sizeof(AMBA_FS_DTA_t));
    memset(pcDta,0,sizeof(AMP_CFS_DTA_s));
    /** Get Dev Compare */
    Rval1 = AmpCFS_FileSearchFirst(pwcPattern, ucAttr, pcDta);
    Rval2 = AmbaFS_FileSearchFirst(pwcPattern, ucAttr, pDta);
    if (Rval1 == Rval2) {
        if (Rval1 != AMP_OK) {
            return AMP_ERROR_RESOURCE_INVALID;
        } else {
            if (AmpUT_CacheDirEntCompare(pDta, pcDta) == TRUE) {
                return TRUE;
            } else {
                AMP_CFS_UT_PRINT(RED,"Memory compare fail!");
                AmbaPrint("Data from Amba_FS");
                AmpUT_CfsPrintDtaAmbaFs(pDta);
                AmbaPrint("Data from Amp_CFS");
                AmpUT_CfsPrintDtaCfs(pcDta);
            }
        }
    } else AMP_CFS_UT_PRINT(RED,"AmpCFS_FileSearchFirst and AmbaFS_FileSearchFirst is different!");
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * CFS UT - cache search next dir entry verification function.
 *
 * @param [in] pDta dir entry
 * @param [in] pcDta dir entry
 * @return 0 - ok, others - fail
 *
 */
static int AmpUT_CacheNextDirEntVerification(AMBA_FS_DTA_t *pDta, AMP_CFS_DTA_s *pcDta)
{
    int Rval1, Rval2;
    /** Get Dev Compare */
    Rval1 = AmpCFS_FileSearchNext(pcDta);
    Rval2 = AmbaFS_FileSearchNext(pDta);
    if (Rval1 == Rval2) {
        if (Rval1 != AMP_OK) {
            return AMP_ERROR_RESOURCE_INVALID;
        } else {
            if (AmpUT_CacheDirEntCompare(pDta, pcDta) == TRUE) {
                return AMP_OK;
            } else {
                AMP_CFS_UT_PRINT(RED,"Memory compare fail!");
                AmbaPrint("Data from Amba_FS");
                AmpUT_CfsPrintDtaAmbaFs(pDta);
                AmbaPrint("Data from Amp_CFS");
                AmpUT_CfsPrintDtaCfs(pcDta);
            }
        }
    } else AMP_CFS_UT_PRINT(RED, "AmpCFS_FileSearchNext and AmbaFS_FileSearchNext is different!");
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * CFS UT - cache test dir entry function.
 *
 * @param [in] cDrive Drive name
 *
 */
static int AmpUT_CacheTestDirEnt(char cDrive)
{
    char pwcL1D1[] = "C:\\L1D1";
    char pwcL2D1[] = "C:\\L1D1\\L2D1";
    char pwcL2D2[] = "C:\\L1D1\\L2D2";
    char pwcL2D3[] = "C:\\L1D1\\L2D3";
    char pwcL1F1[] = "C:\\L1D1\\L1D1F1";
    char pwcL1F2[] = "C:\\L1D1\\L1D1F2";
    char pwcL1F3[] = "C:\\L1D1\\L1D1F3";
    char pwcL2D1F1[] = "C:\\L1D1\\L2D1\\L2D1F1";
    char pwcL2D1F2[] = "C:\\L1D1\\L2D1\\L2D1F2";
    char pwcL2D1F3[] = "C:\\L1D1\\L2D1\\L2D1F3";
    char pwcL2D1F4[] = "C:\\L1D1\\L2D1\\L2D1F4";
    char pwcL1D1F4[] = "C:\\L1D1\\L1D1F4";
    char pwcPattern1[] = "C:\\*";
    char pwcPattern2[] = "C:\\L1D1\\*";
    char pwcPattern3[] = "C:\\L1D1\\L2D1\\*";
    char ucAttr = ATTR_ALL;
    char cBuf[16] = "This is a test.";
    AMP_CFS_FILE_s *pFile;
    AMBA_FS_DTA_t Dta, Dta2;
    AMP_CFS_DTA_s cDta, cDta2;
    BOOL bRlt;
    int Rval;

    /** Clear Cache and files */
    AmpCFS_ClearCache('C');
    AmpUT_Cfsremove(pwcL1F1);
    AmpUT_Cfsremove(pwcL1F2);
    AmpUT_Cfsremove(pwcL1F3);
    AmpUT_Cfsremove(pwcL2D1F1);
    AmpUT_Cfsremove(pwcL2D1F2);
    AmpUT_Cfsremove(pwcL2D1F3);
    AmpUT_Cfsremove(pwcL2D1F4);
    AmpUT_Cfsremove(pwcL1D1F4);
    AmpUT_CfsRmdir(pwcL2D1);
    AmpUT_CfsRmdir(pwcL2D2);
    AmpUT_CfsRmdir(pwcL2D3);
    AmpUT_CfsRmdir(pwcL1D1);

    /** Drive test */
    AmbaPrint("Drive character test!");
    pwcPattern1[0]='A';
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, ucAttr, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    pwcPattern1[0]='B';
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, ucAttr, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    pwcPattern1[0]='C';
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, ucAttr, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    pwcPattern1[0]='D';
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, ucAttr, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    pwcPattern1[0]='C';
    /** Attribute range test */
    AmbaPrint("Attribute range test!");
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, 0x80, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }

    /** Incomplete cache test*/
    AmbaPrint("Incomplet cache test!");
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, ucAttr, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    AmpCFS_ClearCache('C');
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, ucAttr, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    /** Build and read cache test */
    AmbaPrint("Build and read cache test: root directory!");
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, ucAttr, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    AmbaPrint("Read cache for root directory!");
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, ucAttr, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    /** Attribute test */
    AmbaPrint("Attribute test: list dirent with attribute 1, 2, 4, 8, 16, 32!");
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, 1, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, 2, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, 4, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, 8, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, 16, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    AmbaPrint("Read cache for root directory!");
    Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, 32, &Dta, &cDta);
    if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    while (Rval == AMP_OK) {
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
    }
    /** Create directories and files for testing purpose */
    bRlt = TRUE;
    if (AmpUT_CfsMkdir(pwcL1D1) != AMP_OK) bRlt = FALSE;
    if (AmpUT_CfsMkdir(pwcL2D1) != AMP_OK) bRlt = FALSE;
    if (AmpUT_CfsMkdir(pwcL2D2) != AMP_OK) bRlt = FALSE;
    if (bRlt == FALSE) return AMP_ERROR_GENERAL_ERROR;
    pFile = AmpUT_CfsOpen(pwcL1F1, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    pFile = AmpUT_CfsOpen(pwcL1F2, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    pFile = AmpUT_CfsOpen(pwcL1F3, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    pFile = AmpUT_CfsOpen(pwcL2D1F1, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    pFile = AmpUT_CfsOpen(pwcL2D1F2, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    pFile = AmpUT_CfsOpen(pwcL2D1F3, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
    if (pFile == NULL) bRlt = FALSE;
    else if (AmpUT_CfsClose(pFile)!=AMP_OK) bRlt = FALSE;
    if (bRlt == TRUE) {
        /** Clear cache */
        AmpCFS_ClearCache('C');
        /** Build and read cache test */
        AmbaPrint("Build and read cache test!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Rebuild and read cache test after file operations */
        /** fopen, fwrite, fclose */
        pFile = AmpUT_CfsOpen(pwcL2D1F4, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
        if (pFile == NULL) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CfsWrite(cBuf, sizeof(char), sizeof(cBuf) , pFile) != AMP_OK) {
            AmpUT_CfsClose(pFile);
            return AMP_ERROR_GENERAL_ERROR;
        }
        if (AmpUT_CfsClose(pFile)!=AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Rebuild cache */
        AmbaPrint("Read directory entries after creating a new file!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Read cache */
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** chmod */
        if (AmpUT_CfsChmod(pwcL2D1F4, AMP_CFS_MODE_ATTR_ADD|AMP_CFS_ATTR_HIDDEN) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Rebuild cache */
        AmbaPrint("Read directory entries after chmod!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Read cache */
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** move */
        if (AmpUT_CfsMove(pwcL2D1F4, pwcL1D1F4) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Rebuild cache */
        AmbaPrint("Read directory entries after move!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Read cache */
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** cinsert */
        if (AmpUT_CfsCinsert(pwcL1D1F4, 1, 2) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Rebuild cache */
        AmbaPrint("Read directory entries after cinsert!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Read cache */
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** cdelete */
        if (AmpUT_CfsCdelete(pwcL1D1F4, 2, 1) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Rebuild cache */
        AmbaPrint("Read directory entries after cdelete!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Read cache */
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** divide */
        if (AmpUT_CfsDivide(pwcL1D1F4, pwcL2D1F4, 1024) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Rebuild cache */
        AmbaPrint("Read directory entries after divide!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Read cache */
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** combine */
        if (AmpUT_CfsCombine(pwcL2D1F4, pwcL1D1F4) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Rebuild cache */
        AmbaPrint("Read directory entries after combine!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /**< Read cache */
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** remove */
        if (AmpUT_Cfsremove(pwcL2D1F4) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Rebuild cache */
        AmbaPrint("Read directory entries after remove!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Read cache */
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** mkdir */
        if (AmpUT_CfsMkdir(pwcL2D3) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Rebuild cache */
        AmbaPrint("Read directory entries after mkdir!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Read cache */
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** rmdir */
        if (AmpUT_CfsRmdir(pwcL2D3) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Rebuild cache */
        AmbaPrint("Read directory entries after rmdir!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Read cache */
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Clear cache */
        AmpCFS_ClearCache('C');
        /** Concurrent search test */
        AmbaPrint("Concurrent search test!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta2, &cDta2);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        Rval=AmpUT_CacheNextDirEntVerification(&Dta2, &cDta2);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        Rval=AmpUT_CacheNextDirEntVerification(&Dta2, &cDta2);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        Rval = AMP_OK;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta2, &cDta2);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Remove files */
        AmpUT_Cfsremove(pwcL1F1);
        AmpUT_Cfsremove(pwcL1F2);
        AmpUT_Cfsremove(pwcL1F3);
        AmpUT_Cfsremove(pwcL2D1F1);
        AmpUT_Cfsremove(pwcL2D1F2);
        AmpUT_Cfsremove(pwcL2D1F3);
        /** Rebuild cache */
        AmbaPrint("Read directory entries after remove test files!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Read cache */
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        /** Remove directories */
        if (AmpUT_CfsRmdir(pwcL2D1) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CfsRmdir(pwcL2D2) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        if (AmpUT_CfsRmdir(pwcL1D1) != AMP_OK) return AMP_ERROR_GENERAL_ERROR;
        /** Read cache */
        AmbaPrint("Read directory entries after remove test directories!");
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern1, ucAttr, &Dta, &cDta);
        if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        while (Rval == AMP_OK) {
            Rval=AmpUT_CacheNextDirEntVerification(&Dta, &cDta);
            if (Rval == AMP_ERROR_GENERAL_ERROR) return AMP_ERROR_GENERAL_ERROR;
        }
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern2, ucAttr, &Dta, &cDta);
        if (Rval != AMP_ERROR_RESOURCE_INVALID) return AMP_ERROR_GENERAL_ERROR;
        Rval=AmpUT_CacheFirstDirEntVerification(pwcPattern3, ucAttr, &Dta, &cDta);
        if (Rval != AMP_ERROR_RESOURCE_INVALID) return AMP_ERROR_GENERAL_ERROR;
    }
    /** Clear cache */
    AmpCFS_ClearCache('C');
    return AMP_OK;
}

/**
 * CFS UT - cache create test file function.
 *
 * @return TRUE - OK, FALSE - fail
 *
 */
static BOOL AmpUT_CacheCreateTestFiles(void)
{
    char pwcStr[MAX_FILENAME_LENGTH];
    char pwcFile[10][MAX_FILENAME_LENGTH]={"C:\\D4", "C:\\D8", "C:\\D16", "C:\\D32", "C:\\D64", "C:\\D128", "C:\\D256", "C:\\D512", "C:\\D1024"};
    AMP_CFS_FILE_s *pFile;
    int i, j, NumFiles = 4;
    for (i=0;i<9;i++,NumFiles*=2) {
        if (AmpUT_CfsMkdir(pwcFile[i]) != AMP_OK)
            return FALSE;
        for (j=1;j<=NumFiles-2;j++) {
            snprintf (pwcStr, MAX_FILENAME_LENGTH, "%s\\F_%d", pwcFile[i], j);
            pFile = AmpUT_CfsOpen(pwcStr, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
            if (pFile == NULL) return FALSE;
            else if (AmpUT_CfsClose(pFile)!=AMP_OK) return FALSE;
        }
    }
    return TRUE;
}

/**
 * CFS UT - cache remove test file function.
 *
 * @return TRUE - OK, FALSE - fail
 *
 */
static BOOL AmpUT_CacheRemoveTestFiles(void)
{
    char pwcStr[MAX_FILENAME_LENGTH];
    char pwcFile[10][MAX_FILENAME_LENGTH]={"C:\\D4", "C:\\D8", "C:\\D16", "C:\\D32", "C:\\D64", "C:\\D128", "C:\\D256", "C:\\D512", "C:\\D1024"};
    int i, j, NumFiles = 4;
    for (i=0;i<9;i++,NumFiles*=2) {
        for (j=1;j<=NumFiles-2;j++) {
            snprintf (pwcStr, MAX_FILENAME_LENGTH, "%s\\F_%d", pwcFile[i], j);
            AmpUT_Cfsremove(pwcStr);
        }
        AmpUT_CfsRmdir(pwcFile[i]);
    }
    return TRUE;
}

/**
 * CFS UT - generate file function.
 */
static void AmpUT_CfsGenFile(void)
{
    UINT i;
    AMP_CFS_FILE_PARAM_s param;
    AmpUT_CfsMkdir("C:\\CFS");
    if (AmpCFS_GetFileParam(&param) == AMP_OK) {
        AMP_CFS_FILE_s *pFile;
        AMP_CFS_DTA_s dta = {0};
        param.Mode = AMP_CFS_FILE_MODE_WRITE_ONLY;
        for (i=0; i<AMP_CFS_UT_MAX_FILE - 3; i++) {
            snprintf(param.Filename, MAX_FILENAME_LENGTH, "C:\\CFS\\%04u.TXT", i);
            pFile = AmpCFS_fopen(&param);
            if (pFile != NULL)
                AmpCFS_fclose(pFile);
        }
        if (AmpUT_CfsFirstDirEnt("C:\\CFS\\*.*", AMP_CFS_ATTR_ALL, &dta) == AMP_OK)
            while (AmpUT_CfsNextDirEnt(&dta) == AMP_OK);
    }
}

/**
 * The implementation of CFS stress tests
 */

static inline BOOL Stress_IsSetTestCaseAsyncMode(UINT8 mode)
{
    return ((mode & AMP_CFS_UT_ST_TEST_CASE_MODE_ASYNC) != 0);
}

static inline BOOL Stress_IsSetTestCaseSyncMode(UINT8 mode)
{
    return ((mode & AMP_CFS_UT_ST_TEST_CASE_MODE_SYNC) != 0);
}

static inline UINT32 Stress_GetRandWriteSize(void)
{
    return (UINT32) ((AMP_CFS_UT_ST_WBUF_SIZE - 1) * ((double) rand() / RAND_MAX) + 1);
}

static int Stress_PrepareWriteBufferData(UINT8 *wBuf, UINT32 size)
{
    int i;
    K_ASSERT(size <= AMP_CFS_UT_ST_WBUF_SIZE * AMP_CFS_UT_ST_WBUF_COUNT);
    for (i = 0; i < size; i++) {
        wBuf[i] = i % AMP_CFS_UT_ST_BYTE_TEST_BASE;
    }
    return AMP_OK;
}

static int Stress_CleanCmdParams(void)
{
    memset(g_AmpCfsUnitTest.STCmdParamsAligned, 0, sizeof(AMP_CFS_UT_ST_CMD_PARAMS_s));
    return AMP_OK;
}

static int Stress_ConfigBufferPool(void)
{
    UINT8 *WBuf = g_AmpCfsUnitTest.STWriteBuffer;
    return Stress_PrepareWriteBufferData(WBuf, AMP_CFS_UT_ST_WBUF_SIZE * AMP_CFS_UT_ST_WBUF_COUNT);
}

static int Stress_GetOpenConfigDefault(AMP_CFS_UT_ST_OPEN_CONFIG_s *config)
{
    config->fileMode = AMP_CFS_FILE_MODE_WRITE_READ;
    config->alignment = AMP_CFS_UT_ALIGNMENT_DEFAULT;
    config->byteToSync = AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT;
    config->isLowPriority = AMP_CFS_UT_PRIORITY_DEFAULT;
    return AMP_OK;
}

static int Stress_SetOpenConfigSyncWithName(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, const char *name)
{
    strncpy(config->fileName, name, MAX_FILENAME_LENGTH);
    config->isAsync = AMP_CFS_FILE_SYNC_MODE;
    config->maxNumBank = 0;
    return AMP_OK;
}

static int Stress_SetOpenConfigSync(AMP_CFS_UT_ST_OPEN_CONFIG_s *config)
{
    Stress_SetOpenConfigSyncWithName(config, AMP_CFS_UT_ST_FN_SYNC);
    return AMP_OK;
}

static int Stress_SetOpenConfigAsyncWithName(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, const char *name)
{
    strncpy(config->fileName, name, MAX_FILENAME_LENGTH);
    config->isAsync = AMP_CFS_FILE_ASYNC_MODE;
    config->maxNumBank = AMP_CFS_UT_MAX_BANK_AMOUNT / 8;
    return AMP_OK;
}

#if AMP_CFS_UT_COMPILE_WARNING
static int Stress_SetOpenConfigAsyncWithBankNum(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT8 bankNum)
{
    strncpy(config->fileName, AMP_CFS_UT_ST_FN_ASYNC, MAX_FILENAME_LENGTH);
    config->isAsync = AMP_CFS_FILE_ASYNC_MODE;
    config->maxNumBank = bankNum;
    return AMP_OK;
}
#endif

static int Stress_SetOpenConfigAsync(AMP_CFS_UT_ST_OPEN_CONFIG_s *config)
{
    Stress_SetOpenConfigAsyncWithName(config, AMP_CFS_UT_ST_FN_ASYNC);
    return AMP_OK;
}

static AMP_CFS_FILE_s* Stress_OpenFile(AMP_CFS_UT_ST_OPEN_CONFIG_s *config)
{
    AMP_CFS_FILE_s *File;
    AMP_CFS_FILE_PARAM_s FileParam;

    if (AmpCFS_GetFileParam(&FileParam) != AMP_OK) {
        LOG_FAIL("Failed to get default CFS file parameters.");
        return NULL;
    }

    strncpy(FileParam.Filename, config->fileName, MAX_FILENAME_LENGTH);
    FileParam.Mode = config->fileMode;
    FileParam.Alignment = config->alignment;
    FileParam.BytesToSync = config->byteToSync;
    FileParam.AsyncMode = config->isAsync;
    FileParam.LowPriority = config->isLowPriority;
    if (FileParam.AsyncMode) {
        FileParam.AsyncData.MaxNumBank = config->maxNumBank;
    }

    File = AmpCFS_fopen(&FileParam);
    LOG_V_RESULT((File != NULL) ? AMP_OK : AMP_ERROR_GENERAL_ERROR, "AmpCFS_fopen: %s", FileParam.Filename);
    return File;
}

static int Stress_CloseFile(AMP_CFS_FILE_s *file)
{
    int Ret;

    if (file != NULL) {
        char FileName[MAX_FILENAME_LENGTH] = {0};
        strncpy(FileName, file->Filename, MAX_FILENAME_LENGTH);
        Ret = AmpCFS_fclose(file);
        LOG_V_RESULT(Ret, "AmpCFS_fclose: %s", FileName);
    } else {
        LOG_VERB("The file is null.");
        Ret = AMP_ERROR_GENERAL_ERROR;
    }
    return Ret;
}

static int Stress_RemoveFile(char *fileName)
{
    int Ret;
    if (fileName != NULL) {
        Ret = AmpCFS_remove((const char *)fileName);
        LOG_V_RESULT(Ret, "AmpCFS_remove: %s", fileName);
    } else {
        Ret = AMP_ERROR_GENERAL_ERROR;
        LOG_VERB("The file name is null.");
    }
    return Ret;
}

static int Stress_SeekFile(AMP_CFS_FILE_s *file, INT64 offset, int origin)
{
    int Ret;
    if (file != NULL) {
        Ret = AmpCFS_fseek(file, offset, origin);
        LOG_V_RESULT(Ret, "AmpCFS_fseek: %s", file->Filename);
    } else {
        LOG_VERB("The file is null.");
        Ret = AMP_ERROR_GENERAL_ERROR;
    }
    return Ret;
}

static int Stress_SyncFile(AMP_CFS_FILE_s *file)
{
    int Ret;
    if (file != NULL) {
        Ret = AmpCFS_Fsync(file);
        LOG_V_RESULT(Ret, "AmpCFS_FSync: %s", file->Filename);
    } else {
        LOG_VERB("The file is null.");
        Ret = AMP_ERROR_GENERAL_ERROR;
    }
    return Ret;
}

static UINT8 *Stress_GetWriteBufferByID(UINT8 id)
{
    if (id > AMP_CFS_UT_ST_WBUF_COUNT) {
        K_ASSERT(0);
    }
    return g_AmpCfsUnitTest.STWriteBuffer + (id * AMP_CFS_UT_ST_WBUF_SIZE);
}

static UINT8 *Stress_GetWriteBuffer(void)
{
    return Stress_GetWriteBufferByID(0);
}

static void *Stress_GetReadBufferByID(UINT8 id)
{
    if (id > AMP_CFS_UT_ST_RBUF_COUNT) {
        K_ASSERT(0);
    }
    return g_AmpCfsUnitTest.STReadBuffer + (id * AMP_CFS_UT_ST_RBUF_SIZE);
}

static UINT8 *Stress_GetReadBuffer(void)
{
    return Stress_GetReadBufferByID(0);
}

static AMP_CFS_UT_ST_CMD_PARAMS_s* Stress_GetCmdParams(void)
{
    return g_AmpCfsUnitTest.STCmdParamsAligned;
}

static AMP_CFS_UT_ST_TASK_INFO_s* Stress_GetTaskInfo(void)
{
    return g_AmpCfsUnitTest.TaskInfo;
}

static inline int Stress_ValidateBufferData(UINT8 *rBuf, UINT32 bufSize)
{
    int i;

    K_ASSERT(bufSize <= AMP_CFS_UT_ST_WBUF_SIZE);
    for (i = 0; i < bufSize; i++) {
        if (rBuf[i] != i % AMP_CFS_UT_ST_BYTE_TEST_BASE) {
            LOG_FAIL("Invalid Data. bufSize:%u, rBuf[%d]:%d", bufSize, i, rBuf[i]);
            return AMP_ERROR_GENERAL_ERROR;
        }
    }

    return AMP_OK;
}

#if AMP_CFS_UT_COMPILE_WARNING
static int Stress_WriteFile_AmbaFS(AMBA_FS_FILE *file, UINT32 fSize)
{
    UINT8 *WBuf = Stress_GetWriteBuffer();
    UINT8 *WBufPos;
    UINT32 WSize, WBufRest;
    UINT32 TStart, TEnd;
    int i, Ret = AMP_OK;

    if (file == NULL) {
        LOG_VERB("File is null.");
        Ret = AMP_ERROR_GENERAL_ERROR;
        goto END_SYNC_WRITE;
    }
    AmbaUtility_GetTimeStart(&TStart);
    for (i = 0; i < fSize; i++) {
        WBufPos = WBuf;
        WBufRest = AMP_CFS_UT_ST_WBUF_SIZE;
        while (WBufRest > 0) {
            WSize = Stress_GetRandWriteSize();
            if (WSize > WBufRest) {
                WSize = WBufRest;
            }
            if (AmbaFS_fwrite(WBufPos, WSize, 1, file) != 1) {
                LOG_V_FAIL("AmbaFS_fwrite: failed to write %u Bytes", WSize);
                Ret = AMP_ERROR_IO_ERROR;
                goto END_SYNC_WRITE;
            }
            WBufPos += WSize;
            WBufRest -= WSize;
            LOG_VERB("AmbaFS_fwrite: %u Bytes", WSize);
        }
    }
    LOG_V_PASS("AmbaFS_fwrite: %u MiB", fSize);
    AmbaUtility_GetTimeEnd(&TStart, &TEnd);
    LOG_VERB("Elapsed Time: %d", TEnd - TStart);

END_SYNC_WRITE:
    return Ret;
}
#endif

static int Stress_WriteFileFromBuffer(AMP_CFS_FILE_s *file, UINT32 fSize, UINT8 *buffer)
{
    UINT8 *WBuf = buffer;
    UINT8 *WBufPos;
    UINT32 WSize, WBufRest;
    UINT32 TStart, TEnd;
    int i, Ret = AMP_OK;

    if (file == NULL) {
        LOG_VERB("File is null.");
        Ret = AMP_ERROR_GENERAL_ERROR;
        goto END_SYNC_WRITE;
    }
    Stress_PrepareWriteBufferData(WBuf, AMP_CFS_UT_ST_WBUF_SIZE);
    AmbaUtility_GetTimeStart(&TStart);
    for (i = 0; i < fSize; i++) {
        WBufPos = WBuf;
        WBufRest = AMP_CFS_UT_ST_WBUF_SIZE;
        while (WBufRest > 0) {
            WSize = Stress_GetRandWriteSize();
            if (WSize > WBufRest) {
                WSize = WBufRest;
            }
            if (AmpCFS_fwrite(WBufPos, WSize, 1, file) != 1) {
                LOG_V_FAIL("AmpCFS_fwrite: failed to write %u Bytes", WSize);
                Ret = AMP_ERROR_IO_ERROR;
                goto END_SYNC_WRITE;
            }
            WBufPos += WSize;
            WBufRest -= WSize;
            LOG_VERB("AmpCFS_fwrite: %u Bytes", WSize);
        }
    }
    LOG_V_PASS("AmpCFS_fwrite: %u MiB", fSize);
    AmbaUtility_GetTimeEnd(&TStart, &TEnd);
    LOG_VERB("Elapsed Time: %d", TEnd - TStart);

END_SYNC_WRITE:
    return Ret;
}

static int Stress_WriteFile(AMP_CFS_FILE_s *file, UINT32 fSize)
{
    return Stress_WriteFileFromBuffer(file, fSize, Stress_GetWriteBuffer());
}

static int Stress_ReadFileToBuffer(AMP_CFS_FILE_s *file, UINT32 fSize, UINT8 *buffer)
{
    UINT8 *RBuf = buffer;
    UINT8 *RBufPos;
    UINT32 RSize, RBufRest, RCount;
    UINT32 TStart, TEnd;
    int i, Ret = AMP_OK;

    AmbaUtility_GetTimeStart(&TStart);

    if (file == NULL) {
        LOG_VERB("File is null.");
        Ret = AMP_ERROR_GENERAL_ERROR;
        goto END_SYNC_READ;
    }

    for (i = 0; i < fSize; i++) {
        memset(RBuf, -1, AMP_CFS_UT_ST_WBUF_SIZE);
        RBufPos = RBuf;
        RBufRest = AMP_CFS_UT_ST_WBUF_SIZE;
        while (RBufRest > 0) {
            RSize = Stress_GetRandWriteSize();
            if (RSize > RBufRest) {
                RSize = RBufRest;
            }
            if ((RCount = AmpCFS_fread(RBufPos, 1, RSize, file)) == RSize) {
                RBufPos += RSize;
                RBufRest -= RSize;
                LOG_VERB("AmpCFS_fread: %u Bytes", RCount);
            } else {
                if (AmpCFS_feof(file)) {
                    RBufPos += RCount;
                    RBufRest = 0;
                    LOG_VERB("Read EOF! Read %u Bytes.", RCount);
                } else {
                    LOG_V_FAIL("AmpCFS_fread failed! Read %u Bytes.", RCount);
                    Ret = AMP_ERROR_IO_ERROR;
                }
                goto END_SYNC_READ;
            }
        }
        if (Stress_ValidateBufferData(RBuf, RBufPos - RBuf) != AMP_OK) {
            LOG_MSG("Read size: %d MiB", i);
            Ret = AMP_ERROR_GENERAL_ERROR;
            goto END_SYNC_READ;
        }
    }

END_SYNC_READ:
    if (Ret == AMP_OK) {
        LOG_V_PASS("AmpCFS_fread: %u MiB", i);
    }
    AmbaUtility_GetTimeEnd(&TStart, &TEnd);
    LOG_VERB("Elapsed Time: %d", TEnd - TStart);

    return Ret;
}

static int Stress_ReadFile(AMP_CFS_FILE_s *file, UINT32 fSize)
{
    return Stress_ReadFileToBuffer(file, fSize, Stress_GetReadBuffer());
}

static int Stress_Config(void)
{
    UINT32 TimeSeed;

    AmbaUtility_GetTimeStart(&TimeSeed);
    srand(TimeSeed);
    return Stress_ConfigBufferPool();
}

static int Stress_Init(AMP_CFS_UT_ST_CMD_PARAMS_s *params)
{
    return Stress_Config();
}

#define AMP_CFS_UT_ST_STRESS_WAIT_FILE_CLOSE(config) do { \
    AMP_CFS_UT_ST_OPEN_CONFIG_s *_c = (config); \
    if (_c->isAsync) {   \
        Stress_WaitFileClose(_c->fileName, TRUE);  \
    }   \
} while(0) \

static int Stress_WaitFileClose(const char *fileName, BOOL checkError)
{
    int Status;

    K_ASSERT(fileName != NULL)
    LOG_V_MSG("Wait the file to be closed: %s", fileName);
    while ((Status = AmpCFS_FStatus(fileName)) != AMP_CFS_STATUS_UNUSED) {
        if (checkError == TRUE && Status == AMP_CFS_STATUS_ERROR) {
            LOG_FAIL("Error status. File = %s", fileName);
            return AMP_CFS_STATUS_ERROR;
        }
        AmbaKAL_TaskSleep(30);
    }
    return AMP_CFS_STATUS_UNUSED;
}

static int Stress_OpenWriteClose(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    AMP_CFS_FILE_s* File;
    int Ret = AMP_OK;

    AMP_CFS_UT_ST_STRESS_WAIT_FILE_CLOSE(config);
    config->fileMode = AMP_CFS_FILE_MODE_WRITE_READ;
    if ((File = Stress_OpenFile(config)) != NULL) {
        Ret += Stress_WriteFile(File, fSize);
        Ret += Stress_CloseFile(File);
    } else {
        LOG_FAIL("Failed to open the file %s", config->fileName);
        Ret = AMP_ERROR_IO_ERROR;
    }
    return Ret;
}

static int Stress_OpenReadClose(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    AMP_CFS_FILE_s* File;
    int Ret = AMP_OK;

    AMP_CFS_UT_ST_STRESS_WAIT_FILE_CLOSE(config);
    config->fileMode = AMP_CFS_FILE_MODE_READ_ONLY;
    if ((File = Stress_OpenFile(config)) != NULL) {
        Ret += Stress_ReadFile(File, fSize);
        Ret += Stress_CloseFile(File);
    } else {
        LOG_FAIL("Failed to open the file %s", config->fileName);
        Ret = AMP_ERROR_IO_ERROR;
    }
    return Ret;
}

static UINT8 Stress_GetBankAmount(void)
{
#define AMP_CFS_UT_ST_BANK_AMOUNT  ((AMP_UT_CFS_MAX_STREAM_NUM) * (AMP_UT_CFS_FILE_PER_STREAM) * (AMP_UT_CFS_BANK_PER_FILE))
    return AMP_CFS_UT_ST_BANK_AMOUNT;
#undef AMP_CFS_UT_ST_BANK_AMOUNT
}

static UINT8 Stress_GetBankPerFile(UINT8 fileCount)
{
    UINT8 BankPerFile;
    AMP_CFS_CFG_s CFSConfig = {0};

    AmpCFS_GetDefaultCfg(&CFSConfig);
    BankPerFile = CFSConfig.SchBankAmount / fileCount;
    if (BankPerFile == 0) {
        BankPerFile = 1;
    }
    return BankPerFile;
}

static int Stress_GetDeviceFreeSpace(char driveName, UINT64 *freeSpace)
{
    AMP_CFS_DRIVE_INFO_s DevInfo;
    int Ret = AMP_OK;

    if ((Ret = AmpCFS_GetDriveInfo(driveName, &DevInfo)) != AMP_OK) {
#if AMP_CFS_UT_LOG_VERB_ENABLE
        char DevStr[2] = {driveName, '\0'};
        LOG_V_FAIL("Failed to get the information of the drive %s", DevStr);
#endif
        Ret = AMP_ERROR_IO_ERROR;
        goto EXIT;
    }

    if (freeSpace != NULL) {
        *freeSpace = (UINT64)AmpUT_CfsGetDriveEmptyClusters(DevInfo) * AmpUT_CfsGetDriveBytesPerCluster(DevInfo);
    }
EXIT:
    return Ret;
}

static int Stress_ResizeFile(UINT32 *fSize, UINT32 *fAmount)
{
    int Ret = AMP_OK;
    UINT64 FreeSpace;
    UINT32 FileAmount = *fAmount;
    UINT32 FileSize = *fSize;

    if ((Ret = Stress_GetDeviceFreeSpace('C', &FreeSpace)) != AMP_OK) {
        goto EXIT;
    }
    if (FreeSpace < AMP_CFS_UT_ST_WBUF_SIZE) {
        LOG_FAIL("No free space to create files.");
        Ret = AMP_ERROR_GENERAL_ERROR;
        goto EXIT;
    }
    if (FreeSpace < (AMP_CFS_UT_ST_WBUF_SIZE * FileSize * FileAmount)) {
        if ((FileSize = FreeSpace / (AMP_CFS_UT_ST_WBUF_SIZE * FileAmount)) == 0) {
            FileSize = *fSize;
            if ((FileAmount = FreeSpace / (AMP_CFS_UT_ST_WBUF_SIZE * FileSize)) == 0) {
                FileAmount = 1;
                FileSize = FreeSpace / AMP_CFS_UT_ST_WBUF_SIZE;
            }
        }
        *fSize = FileSize;
        *fAmount = FileAmount;
    }
EXIT:
    return Ret;
}

static int Stress_TestCase_SyncWrite(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    int Ret = AMP_OK;

    if (Stress_IsSetTestCaseSyncMode(params->TestMode)) {
        LOG_MSG("Sync write %u MiB", params->FileSize);
        Stress_SetOpenConfigSync(config);
        Ret = Stress_OpenWriteClose(config, params->FileSize);
    }
    return Ret;
}

static int Stress_TestCase_AsyncWrite(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    int Ret = AMP_OK;

    if (Stress_IsSetTestCaseAsyncMode(params->TestMode)) {
        LOG_MSG("Async write %u MiB", params->FileSize);
        Stress_SetOpenConfigAsync(config);
        Ret = Stress_OpenWriteClose(config, params->FileSize);
    }
    return Ret;
}

static int Stress_TestCase_SyncRead(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    int Ret = AMP_OK;

    if (Stress_IsSetTestCaseSyncMode(params->TestMode)) {
        LOG_MSG("Try to sync read %u MiB", params->FileSize);
        Stress_SetOpenConfigSync(config);
        Ret = Stress_OpenReadClose(config, params->FileSize);
    }
    return Ret;
}

static int Stress_TestCase_AsyncRead(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    int Ret = AMP_OK;

    if (Stress_IsSetTestCaseAsyncMode(params->TestMode)) {
        LOG_MSG("Try to async read %u MiB", params->FileSize);
        Stress_SetOpenConfigAsync(config);
        Ret = Stress_OpenReadClose(config, params->FileSize);
    }
    return Ret;
}

static int Stress_TestItem_WriteRead_Closed(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    int Ret = AMP_OK;
    Ret += Stress_OpenWriteClose(config, fSize);
    Ret += Stress_OpenReadClose(config, fSize);
    LOG_RESULT(Ret);
    return Ret;
}

static int Stress_TestItem_WriteRead_NonClosed(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    AMP_CFS_FILE_s* File;
    int Ret = AMP_OK;

    {
        UINT32 FileAmount = 3;
        if ((Ret = Stress_ResizeFile(&fSize, &FileAmount)) != AMP_OK) {
            LOG_FAIL("Failed to modify the file size or amount.");
            goto EXIT;
        }
        if (FileAmount < 3) {
            LOG_FAIL("No free space to create files.");
            Ret = AMP_ERROR_GENERAL_ERROR;
            goto EXIT;
        }
    }
    AMP_CFS_UT_ST_STRESS_WAIT_FILE_CLOSE(config);
    config->fileMode = AMP_CFS_FILE_MODE_WRITE_READ;
    File = Stress_OpenFile(config);
    if (File != NULL) {
        Ret += Stress_WriteFile(File, fSize);
        Ret += Stress_WriteFile(File, fSize);
        Ret += Stress_ReadFile(File, fSize);
        Ret += Stress_SyncFile(File);
        Ret += Stress_SeekFile(File, 0, AMP_CFS_SEEK_START);
        Ret += Stress_ReadFile(File, fSize);
        Ret += Stress_SeekFile(File, 0, AMP_CFS_SEEK_END);
        Ret += Stress_WriteFile(File, fSize);
        Ret += Stress_CloseFile(File);
    } else {
        Ret = AMP_ERROR_GENERAL_ERROR;
    }
EXIT:
    LOG_RESULT(Ret);
    return Ret;
}

static int Stress_TestCase_WriteReadImpl(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    int Ret = AMP_OK;
    Ret += Stress_TestItem_WriteRead_Closed(config, fSize);
    Ret += Stress_TestItem_WriteRead_NonClosed(config, fSize);
    return Ret;
}

static int Stress_TestCase_WriteRead(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    int Ret = AMP_OK;
    if (Stress_IsSetTestCaseSyncMode(params->TestMode)) {
        LOG_MSG("Sync mode");
        Stress_SetOpenConfigSync(config);
        Ret += Stress_TestCase_WriteReadImpl(config, params->FileSize);
    }
    if (Stress_IsSetTestCaseAsyncMode(params->TestMode)) {
        LOG_MSG("Async mode");
        Stress_SetOpenConfigAsync(config);
        Ret += Stress_TestCase_WriteReadImpl(config, params->FileSize);
    }
    return Ret;
}

//TODO append mode
static int Stress_TestCase_MultiOpenImpl(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    AMP_CFS_FILE_s* Files[AMP_CFS_UT_ST_MULTIOPEN_FILE_AMOUNT + 1] = {0};
    AMP_CFS_FILE_s** SourceFile = &Files[AMP_CFS_UT_ST_MULTIOPEN_FILE_AMOUNT];
    UINT8 OpenRes;
    UINT8 OpenResults[4][4] = {
        {0, 1, 1, 1},
        {1, 1, 1, 1},
        {1, 1, 1, 1},
        {1, 1, 1, 1}
    };
    int Ret = AMP_OK;

    if (config->isAsync) {
        UINT8 BankAmount = Stress_GetBankAmount();
        config->maxNumBank = BankAmount / 2; // two file opened concurrently
        LOG_V_MSG("MaxNumBank: %d", config->maxNumBank);
    }
    /* Create the source file */
    config->fileMode = AMP_CFS_FILE_MODE_WRITE_READ;
    AMP_CFS_UT_ST_STRESS_WAIT_FILE_CLOSE(config);
    if ((*SourceFile = Stress_OpenFile(config)) != NULL) {
        if ((Ret = Stress_WriteFile(*SourceFile, fSize)) != AMP_OK) {
            LOG_FAIL("Failed to write the file %s", (*SourceFile)->Filename);
        }
        if ((Ret += Stress_CloseFile(*SourceFile)) != AMP_OK) {
            LOG_FAIL("Failed to close the file %s", (*SourceFile)->Filename);
        }
    } else {
        LOG_FAIL("Failed to open the file %s", (*SourceFile)->Filename);
        Ret = AMP_ERROR_IO_ERROR;
    }
    if (Ret != AMP_OK) {
        goto EXIT;
    }
    /* Test file open modes */
    for (UINT8 sm = 0; sm <= AMP_CFS_FILE_MODE_WRITE_READ && sm < AMP_CFS_UT_ST_MULTIOPEN_FILE_AMOUNT; sm++) {
        LOG_V_MSG("Open source file, mode: %hhu", sm);
        config->fileMode = sm;
        AMP_CFS_UT_ST_STRESS_WAIT_FILE_CLOSE(config);
        if ((*SourceFile = Stress_OpenFile(config)) == NULL) {
            LOG_FAIL("Failed to open the file %s", (*SourceFile)->Filename);
            Ret = AMP_ERROR_IO_ERROR;
            break;
        }
        for (UINT8 m = 0; m <= AMP_CFS_FILE_MODE_WRITE_READ && m < AMP_CFS_UT_ST_MULTIOPEN_FILE_AMOUNT; m++) {
            config->fileMode = m;
            LOG_V_MSG("Open Test file, mode: %hhu", m);
            OpenRes = ((Files[m] = Stress_OpenFile(config)) != NULL) ? 0 : 1;
            if (OpenResults[sm][m] != OpenRes) {
                LOG_FAIL("Invalid result. Source file mode: %hhu, Test file mode: %hhu", sm, m);
                Ret = AMP_ERROR_IO_ERROR;
            }
            if (OpenRes == 0 && Stress_CloseFile(Files[m]) != AMP_OK) {
                LOG_FAIL("Failed to close the file %s", Files[m]->Filename);
                Ret = AMP_ERROR_IO_ERROR;
            }
        }
        if (Stress_CloseFile(*SourceFile) != AMP_OK) {
            LOG_FAIL("Failed to close the source file %s", (*SourceFile)->Filename);
            Ret = AMP_ERROR_IO_ERROR;
        }
    }
EXIT:
    LOG_RESULT(Ret);
    return Ret;
}

static int Stress_TestCase_MultiOpen(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    int Ret = AMP_OK;
    if (Stress_IsSetTestCaseSyncMode(params->TestMode)) {
        LOG_MSG("Sync mode");
        Stress_SetOpenConfigSync(config);
        Ret += Stress_TestCase_MultiOpenImpl(config, params->FileSize);
    }
    if (Stress_IsSetTestCaseAsyncMode(params->TestMode)) {
        LOG_MSG("Async mode");
        Stress_SetOpenConfigAsync(config);
        Ret += Stress_TestCase_MultiOpenImpl(config, params->FileSize);
    }
    return Ret;
}

static int Stress_TestItem_MultiFile_OpenFileNonclosed(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
#define AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT ((AMP_CFS_UT_ST_SYS_FILE_OPEN_MAX <= AMP_CFS_UT_ST_MULTIOPEN_STREAM_MAX) ? AMP_CFS_UT_ST_SYS_FILE_OPEN_MAX : AMP_CFS_UT_ST_MULTIOPEN_STREAM_MAX)
    AMP_CFS_FILE_s* Files[AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT + 1] = {0};
    char FileNames[AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT + 1][MAX_FILENAME_LENGTH] = {0};
    char FileName[MAX_FILENAME_LENGTH] = {0};
    int i, FileAmount, Ret = AMP_OK;

    /* Create & open the maximum number of files */
    if (config->isAsync) {
        UINT8 BankAmount = Stress_GetBankAmount();
        config->maxNumBank = 2;
        BankAmount /= 2; // for read mode.
        FileAmount = (BankAmount <= AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT) ? BankAmount : AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT;
    } else {
        FileAmount = AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT;
    }

    config->fileMode = AMP_CFS_FILE_MODE_WRITE_READ;
    for (i = 0; i < FileAmount; i++) {
        snprintf(FileName, MAX_FILENAME_LENGTH, AMP_CFS_UT_ST_FN(_MULTIFILE_%u), i + 1);
        strncpy(config->fileName, FileName, MAX_FILENAME_LENGTH);
        AMP_CFS_UT_ST_STRESS_WAIT_FILE_CLOSE(config);
        if ((Files[i] = Stress_OpenFile(config)) != NULL) {
            strncpy(FileNames[i], FileName, MAX_FILENAME_LENGTH);
            if (Stress_WriteFile(Files[i], fSize) != AMP_OK) {
                LOG_FAIL("Failed to write the file %s", FileName);
                Ret = AMP_ERROR_GENERAL_ERROR;
            }
        } else {
            LOG_FAIL("Failed to create and open the file %s", FileName);
            Ret = AMP_ERROR_GENERAL_ERROR;
            break;
        }
    }
#if AMP_CFS_UT_ST_ERR_MSG_ENABLE
    if (i >= FileAmount) {
        snprintf(FileName, MAX_FILENAME_LENGTH, AMP_CFS_UT_ST_FN(_MULTIFILE_%u), i + 1);
        strncpy(config->fileName, FileName, MAX_FILENAME_LENGTH);
        if ((Files[i] = Stress_OpenFile(config)) == NULL) {
            FileNames[i][0] = '\0';
            LOG_V_PASS("Got the maximum number of files: %d", i);
        } else {
            strncpy(FileNames[i], FileName, MAX_FILENAME_LENGTH);
            LOG_FAIL("Failed to get the maximum number of files");
            Ret = AMP_ERROR_GENERAL_ERROR;
        }
    }
#endif
    for (i = 0; i <= FileAmount; i++) {
        if (Files[i] != NULL) {
            if (Stress_CloseFile(Files[i]) != AMP_OK) {
                LOG_FAIL("Failed to close the file: %s", FileNames[i]);
                Ret = AMP_ERROR_GENERAL_ERROR;
            }
        } else {
            break;
        }
    }
    for (i = 0; i <= FileAmount; i++) {
        if (FileNames[i][0] != '\0') {
            Stress_WaitFileClose(FileNames[i], TRUE);
            if (Stress_RemoveFile(FileNames[i]) != AMP_OK) {
                LOG_FAIL("Failed to remove the file: %s", FileNames[i]);
                Ret = AMP_ERROR_GENERAL_ERROR;
            }
        } else {
            break;
        }
    }
    LOG_RESULT(Ret);
    return Ret;
#undef AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT
}

static int Stress_TestItem_MultiFile_OpenFileClosed(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
#define AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT (8)
    AMP_CFS_FILE_s* File = NULL;
    char FileName[MAX_FILENAME_LENGTH] = {0};
    UINT32 FileAmount = 0;
    int i, Ret = AMP_OK;

    {
        UINT64 FreeSpace;
        if (Stress_GetDeviceFreeSpace('C', &FreeSpace) != AMP_OK) {
            Ret = AMP_ERROR_GENERAL_ERROR;
            goto EXIT;
        }
        if (FreeSpace < AMP_CFS_UT_ST_WBUF_SIZE) {
            LOG_FAIL("No free space to create files.");
            Ret = AMP_ERROR_GENERAL_ERROR;
            goto EXIT;
        }
        if (FreeSpace < (AMP_CFS_UT_ST_WBUF_SIZE * fSize * AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT)) {
            fSize = FreeSpace / (AMP_CFS_UT_ST_WBUF_SIZE * AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT);
            if (fSize > 0) {
                FileAmount = FreeSpace / (fSize * AMP_CFS_UT_ST_WBUF_SIZE);
            } else {
                FileAmount = 1;
                fSize = FreeSpace / AMP_CFS_UT_ST_WBUF_SIZE;
            }
        } else {
            FileAmount = AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT;
        }
    }
    LOG_V_MSG("Create %u files.", FileAmount);
    config->fileMode = AMP_CFS_FILE_MODE_WRITE_READ;
    for (i = 0; i < FileAmount; i++) {
        memset(FileName, 0, MAX_FILENAME_LENGTH);
        snprintf(FileName, MAX_FILENAME_LENGTH, AMP_CFS_UT_ST_FN(_MULTIFILE_%u), i + 1);
        strncpy(config->fileName, FileName, MAX_FILENAME_LENGTH);
        AMP_CFS_UT_ST_STRESS_WAIT_FILE_CLOSE(config);
        File = Stress_OpenFile(config);
        if (File != NULL) {
            if (Stress_WriteFile(File, fSize) != AMP_OK) {
                Ret = AMP_ERROR_GENERAL_ERROR;
                LOG_FAIL("Failed to write the file %s", FileName);
            }
            if (Stress_SyncFile(File) != AMP_OK) {
                Ret = AMP_ERROR_GENERAL_ERROR;
                LOG_FAIL("Failed to sync the file %s", FileName);
            }
            if (Stress_SeekFile(File, 0, AMP_CFS_SEEK_START) != AMP_OK) {
                Ret = AMP_ERROR_GENERAL_ERROR;
                LOG_FAIL("Failed to seek the file %s", FileName);
            }
            if (Stress_ReadFile(File, fSize) != AMP_OK) {
                Ret = AMP_ERROR_GENERAL_ERROR;
                LOG_FAIL("Failed to read the file %s", FileName);
            }
            if (Stress_CloseFile(File) != AMP_OK) {
                Ret = AMP_ERROR_GENERAL_ERROR;
                LOG_FAIL("Failed to close the file %s", FileName);
            }
            if (Ret != AMP_OK) {
                break;
            }
        } else {
            Ret = AMP_ERROR_GENERAL_ERROR;
            LOG_FAIL("Failed to create and open the file %s", FileName);
            break;
        }
    }
    if (Ret == AMP_OK) {
        for (i = 0; i < FileAmount; i++) {
            memset(FileName, 0, MAX_FILENAME_LENGTH);
            snprintf(FileName, MAX_FILENAME_LENGTH, AMP_CFS_UT_ST_FN(_MULTIFILE_%u), i + 1);
            Stress_WaitFileClose(FileName, TRUE);
            if ((Ret += Stress_RemoveFile(FileName)) != AMP_OK) {
                LOG_FAIL("Failed to remove the file: %s", FileName);
                break;
            }
        }
    }

EXIT:
    LOG_RESULT(Ret);
    return Ret;
#undef AMP_CFS_UT_ST_MULTIFILE_FILE_AMOUNT
}
static int Stress_TestCase_MultiFileImpl(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    int Ret = AMP_OK;
    Ret += Stress_TestItem_MultiFile_OpenFileNonclosed(config, fSize);
    Ret += Stress_TestItem_MultiFile_OpenFileClosed(config, fSize);
    return Ret;
}

static int Stress_TestCase_MultiFile(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    int Ret = AMP_OK;
    if (Stress_IsSetTestCaseSyncMode(params->TestMode)) {
        LOG_MSG("Sync mode");
        Stress_SetOpenConfigSync(config);
        Ret += Stress_TestCase_MultiFileImpl(config, params->FileSize);
    }
    if (Stress_IsSetTestCaseAsyncMode(params->TestMode)) {
        LOG_MSG("Async mode");
        Stress_SetOpenConfigAsync(config);
        Ret += Stress_TestCase_MultiFileImpl(config, params->FileSize);
    }
    return Ret;
}

#if AMP_CFS_UT_COMPILE_WARNING
static int Stress_TestItem_MultiRead_DiffMode(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
#define AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT 2
    AMP_CFS_FILE_s* Files[AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT + 1] = {0};
    AMP_CFS_FILE_s** SourceFile = &Files[AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT];
    UINT8 ReadModes[AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT] = {AMP_CFS_FILE_MODE_READ_ONLY, AMP_CFS_FILE_MODE_READ_WRITE};
    UINT32 ReadSize, ReadCount = 0;
    int Ret = AMP_OK, i;

    if (config->isAsync) {
        config->maxNumBank = Stress_GetBankPerFile(AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT + 1, NULL);
    }
    /* Create the source file */
    if ((*SourceFile = Stress_OpenFile(config)) != NULL) {
        if (Stress_WriteFile(*SourceFile, fSize) != AMP_OK) {
            LOG_FAIL("Failed to write the source file %s", (*SourceFile)->Filename);
            Ret = AMP_ERROR_IO_ERROR;
        }
        if (Stress_CloseFile(*SourceFile) != AMP_OK) {
            LOG_FAIL("Failed to close the source file %s", (*SourceFile)->Filename);
            Ret = AMP_ERROR_IO_ERROR;
        }
    } else {
        LOG_FAIL("Failed to open the source file %s", (*SourceFile)->Filename);
        Ret = AMP_ERROR_IO_ERROR;
    }
    if (Ret != AMP_OK) {
        goto EXIT;
    }
    /* Test file read modes */
    for (i = 0; i < AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT; i++) {
        config->fileMode = ReadModes[i];
        if ((Files[i] = Stress_OpenFile(config)) == NULL) {
            LOG_FAIL("Failed to open the file %s", Files[i]->Filename);
            Ret = AMP_ERROR_IO_ERROR;
        }
    }
    ReadSize = (fSize < 5) ? 1 : fSize / 5;
    while(Ret == AMP_OK && ReadCount < fSize) {
        for (i = 0; i <AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT; i++) {
            if (Stress_ReadFile(Files[i], ReadSize) != AMP_OK) {
                LOG_FAIL("Failed to read the source file with mode %hhu", i);
                Ret = AMP_ERROR_IO_ERROR;
                break;
            }
        }
        ReadCount += ReadSize;
    }
    for (i = 0; i < AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT; i++) {
        if (Files[i] != NULL && Stress_CloseFile(Files[i]) != AMP_OK) {
            LOG_FAIL("Failed to close the file %s", Files[i]->Filename);
            Ret = AMP_ERROR_IO_ERROR;
        }
    }
EXIT:
    LOG_RESULT(Ret);
    return Ret;
#undef AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT
}
#endif

static int Stress_TestItem_MultiRead_SameMode(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
#define AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT        1
#define AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT_PER_MODE    (3)
#define AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT             ((AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT_PER_MODE > AMP_CFS_UT_ST_SYS_FILE_OPEN_MAX) ? AMP_CFS_UT_ST_SYS_FILE_OPEN_MAX : AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT_PER_MODE)
    AMP_CFS_FILE_s* Files[AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT] = {0};
    UINT8 ReadModes[AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT] = {AMP_CFS_FILE_MODE_READ_ONLY};
    UINT32 ReadSize, ReadCount = 0;
    int Ret = AMP_OK, i;

    if (config->isAsync) {
        UINT8 BankAmount = Stress_GetBankAmount();
        config->maxNumBank = Stress_GetBankPerFile(AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT);
        if ((BankAmount / 2) < AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT) {
            LOG_FAIL("There is not enough banks to open files with async mode.");
            Ret = AMP_ERROR_GENERAL_ERROR;
            goto EXIT;
        }
    }
    /* Create the source file */
    if ((Ret = Stress_OpenWriteClose(config, fSize)) != AMP_OK) {
        LOG_FAIL("Failed to create the source file %s", config->fileName);
        goto EXIT;
    }
    /* Test file read modes */
    AMP_CFS_UT_ST_STRESS_WAIT_FILE_CLOSE(config);
    for (UINT8 m = 0; m < AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT; m++) {
        config->fileMode = ReadModes[m];
        for (i = 0; i < AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT; i++) {
            if ((Files[i] = Stress_OpenFile(config)) == NULL) {
                LOG_FAIL("Failed to open the file %s", config->fileName);
                Ret = AMP_ERROR_IO_ERROR;
            }
        }
        ReadSize = (fSize < 5) ? 1 : fSize / 5;
        while(Ret == AMP_OK && ReadCount < fSize) {
            for (i = 0; i <AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT; i++) {
                if (Stress_ReadFile(Files[i], ReadSize) != AMP_OK) {
                    LOG_FAIL("Failed to read the source file with mode %d", i);
                    Ret = AMP_ERROR_IO_ERROR;
                    break;
                }
            }
            ReadCount += ReadSize;
        }
        for (i = 0; i < AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT; i++) {
            if (Files[i] != NULL && Stress_CloseFile(Files[i]) != AMP_OK) {
                LOG_FAIL("Failed to close the file %s", Files[i]->Filename);
                Ret = AMP_ERROR_IO_ERROR;
            }
        }
    }
EXIT:
    LOG_RESULT(Ret);
    return Ret;
#undef AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT
#undef AMP_CFS_UT_ST_MULTIREAD_FILE_AMOUNT_PER_MODE
#undef AMP_CFS_UT_ST_MULTIREAD_READ_MODE_AMOUNT
}

static int Stress_TestCase_MultiReadImpl(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    return Stress_TestItem_MultiRead_SameMode(config, fSize);
}

static int Stress_TestCase_MultiRead(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    int Ret = AMP_OK;
    if (Stress_IsSetTestCaseSyncMode(params->TestMode)) {
        LOG_MSG("Sync mode");
        Stress_SetOpenConfigSync(config);
        Ret += Stress_TestCase_MultiReadImpl(config, params->FileSize);
    }
    if (Stress_IsSetTestCaseAsyncMode(params->TestMode)) {
        LOG_MSG("Async mode");
        Stress_SetOpenConfigAsync(config);
        Ret += Stress_TestCase_MultiReadImpl(config, params->FileSize);
    }
    return Ret;
}

static UINT32 Stress_GetTaskId(AMBA_KAL_TASK_t *task)
{
    char * TN = task->tx_thread_name;
    size_t TNLen = strlen(TN);
    return atoi(TN + TNLen - 1);
}

static int Stress_QueryTaskState(AMBA_KAL_TASK_t *task, UINT32 *state)
{
    AMBA_KAL_TASK_INFO_s Info = {0};
    int Ret = AmbaKAL_TaskQuery(task, &Info);
    if (Ret == OK && state != NULL) {
        *state = Info.CurState;
    }
    return Ret;
}

static int Stress_CreateTask(void (*entry)(UINT32), UINT32 entryArg, char *name, AMBA_KAL_TASK_t **task)
{
    int Ret = AMP_ERROR_GENERAL_ERROR;
    AMP_CFS_UT_ST_TASK_INFO_s *TaskInfo = Stress_GetTaskInfo();
    AMBA_KAL_TASK_t *Task = {0};
    UINT32 TaskState;

    for (int i = 0; i < AMP_CFS_UT_ST_TASK_AMOUNT; i++) {
        Task = &(TaskInfo->Task[i]);
        if (Task != NULL) {
            if (Task->tx_thread_name != NULL) {
                if ((Ret = Stress_QueryTaskState(Task, &TaskState)) != OK) {
                    break;
                }
                if (TaskState == TX_COMPLETED || TaskState == TX_TERMINATED) {
                    if ((Ret = AmbaKAL_TaskDelete(Task)) != OK) {
                        LOG_V_FAIL("Failed to delete the task %s", Task->tx_thread_name);
                        break;
                    }
                    Ret = AmbaKAL_TaskCreate(Task, name, 50, entry, entryArg, TaskInfo->TaskStacts[i], AMP_CFS_UT_ST_TASK_STACK_SIZE, AMBA_KAL_DO_NOT_START);
                    break;
                }
                continue;
            }
            Ret = AmbaKAL_TaskCreate(Task, name, 50, entry, entryArg, TaskInfo->TaskStacts[i], AMP_CFS_UT_ST_TASK_STACK_SIZE, AMBA_KAL_DO_NOT_START);
            break;
        }
    }
    if (task != NULL && Ret == AMP_OK) {
        *task = Task;
    }
    return Ret;
}

static int Stress_WaitTasksComplete(void)
{
    AMP_CFS_UT_ST_TASK_INFO_s *TaskInfo = Stress_GetTaskInfo();
    AMBA_KAL_TASK_t *Task;
    UINT32 TaskState;
    UINT8 TCount;
    int Ret = AMP_OK;

    while (TRUE) {
        TCount = 0;
        for (int i = 0; i < AMP_CFS_UT_ST_TASK_AMOUNT; i++) {
            Task = &(TaskInfo->Task[i]);
            if ((Ret = Stress_QueryTaskState(Task, &TaskState)) != OK) {
                LOG_V_FAIL("Failed to query the task %s", Task->tx_thread_name);
                continue;
            }
            if (TaskState == TX_COMPLETED || TaskState == TX_TERMINATED) {
                continue;
            }
            if (TaskState == TX_SUSPENDED) {
                if ((Ret = AmbaKAL_TaskTerminate(Task)) != OK) {
                    LOG_V_FAIL("Failed to terminate the task %s", Task->tx_thread_name);
                }
                continue;
            }
            TCount++;
        }
        if (TCount > 0) {
            AmbaKAL_TaskSleep(50);
        } else {
            break;
        }
    }
    return Ret;
}

#if AMP_CFS_UT_COMPILE_WARNING
static int Stress_DeleteTask(AMBA_KAL_TASK_t *task)
{
    int Ret = AMP_OK;
    AMP_CFS_UT_ST_TASK_INFO_s *TaskInfo = Stress_GetTaskInfo();

    if (AmbaKAL_TaskDelete(task) != OK) {
        LOG_V_FAIL("Failed to delete the task %s", task->tx_thread_name);
        Ret = AMP_ERROR_GENERAL_ERROR;
    }

    return Ret;
}
#endif

typedef struct AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s_ {
    AMP_CFS_UT_ST_OPEN_CONFIG_s *Config;
    void *CmdParams;
    int Return;
} AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s;

static void Stress_Task_MultiRead(UINT32 param)
{
    AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s *TaskParams = (AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s *)param;
    AMP_CFS_UT_ST_OPEN_CONFIG_s *Config = TaskParams->Config;
    UINT32 FSize = (UINT32) TaskParams->CmdParams;
    int Ret = AMP_OK;
    AMP_CFS_FILE_s* File;
    AMBA_KAL_TASK_t *Task = AmbaKAL_TaskIdentify();
    UINT8 TaskID;

    {
        char * TN = Task->tx_thread_name;
        size_t TNLen = strlen(TN);
        TaskID = atoi(TN + TNLen - 1);
    }
    K_ASSERT(TaskID > 0 && TaskID <= AMP_CFS_UT_ST_TASK_AMOUNT);
    LOG_V_MSG("Stress_Task_ReadFile (Task %hhu)", TaskID);
    Config->fileMode = AMP_CFS_FILE_MODE_READ_ONLY;
    if ((File = Stress_OpenFile(Config)) != NULL) {
        if ((Ret += Stress_ReadFileToBuffer(File, FSize, Stress_GetReadBufferByID(TaskID))) != AMP_OK) {
            LOG_FAIL("Failed to read the file %s (Task %hhu)", Config->fileName, TaskID);
        }
        if ((Ret += Stress_CloseFile(File)) != AMP_OK) {
            LOG_FAIL("Failed to close the file %s (Task %hhu)", Config->fileName, TaskID);
        }
    } else {
        LOG_FAIL("Failed to open the file %s (Task %hhu)", Config->fileName, TaskID);
        Ret = AMP_ERROR_GENERAL_ERROR;
    }
    AmpCFS_LogProbeOPEntries();
    TaskParams->Return = Ret;
}

static int Stress_TestCase_MultiTask_MultiReadImpl(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    int Ret = AMP_OK;
    AMBA_KAL_TASK_t *Task1, *Task2;
    static AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s CmdParams1;
    static AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s CmdParams2;

    CmdParams1.Config = config;
    CmdParams1.CmdParams = (void *)fSize;
    CmdParams1.Return = AMP_OK;
    CmdParams2.Config = config;
    CmdParams2.CmdParams = (void *)fSize;
    CmdParams2.Return = AMP_OK;

    /* Create the source file */
    if ((Ret = Stress_OpenWriteClose(config, fSize)) != AMP_OK) {
        LOG_FAIL("Failed to create the source file %s", config->fileName);
        goto EXIT;
    }
    AMP_CFS_UT_ST_STRESS_WAIT_FILE_CLOSE(config);
    Ret += Stress_CreateTask(Stress_Task_MultiRead, (UINT32)&CmdParams1, "MT_MultiRead_T1", &Task1);
    Ret += Stress_CreateTask(Stress_Task_MultiRead, (UINT32)&CmdParams2, "MT_MultiRead_T2", &Task2);
    if (Ret != AMP_OK) {
        goto EXIT;
    }
    if (Task1 == NULL || Task2 == NULL) {
        Ret = AMP_ERROR_GENERAL_ERROR;
        LOG_FAIL("Failed to create tasks.");
        goto EXIT;
    }
    AmbaKAL_TaskResume(Task1);
    AmbaKAL_TaskResume(Task2);
    Stress_WaitTasksComplete();
EXIT:
    Ret += (CmdParams1.Return + CmdParams2.Return);
    LOG_RESULT(Ret);
    return Ret;
}

static int Stress_TestCase_MultiTask_MultiRead(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    int Ret = AMP_OK;
    if (Stress_IsSetTestCaseSyncMode(params->TestMode)) {
        LOG_MSG("Sync mode");
        Stress_SetOpenConfigSync(config);
        Ret += Stress_TestCase_MultiTask_MultiReadImpl(config, params->FileSize);
    }
    if (Stress_IsSetTestCaseAsyncMode(params->TestMode)) {
        LOG_MSG("Async mode");
        Stress_SetOpenConfigAsync(config);
        Ret += Stress_TestCase_MultiTask_MultiReadImpl(config, params->FileSize);
    }
    return Ret;
}

static void Stress_Task_MultiFile(UINT32 param)
{
#define AMP_CFS_UT_ST_MT_MULTIFILE_FILE_AMOUNT (2)
    AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s *TaskParams = (AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s *)param;
    AMP_CFS_UT_ST_OPEN_CONFIG_s *Config = TaskParams->Config;
    UINT32 FSize = (UINT32) TaskParams->CmdParams;
    int Ret = AMP_OK;
    AMP_CFS_FILE_s* File;
    char FileName[MAX_FILENAME_LENGTH] = {0};
    UINT32 TaskID, i;

    {
        AMBA_KAL_TASK_t *Task = AmbaKAL_TaskIdentify();
        char * TN = Task->tx_thread_name;
        size_t TNLen = strlen(TN);
        TaskID = atoi(TN + TNLen - 1);
    }
    K_ASSERT(TaskID > 0 && TaskID <= AMP_CFS_UT_ST_TASK_AMOUNT);
    LOG_V_MSG("Stress_Task_MultiFile (Task %u)", TaskID);
    Config->fileMode = AMP_CFS_FILE_MODE_WRITE_READ;
    for (i = 0; i < AMP_CFS_UT_ST_MT_MULTIFILE_FILE_AMOUNT; i++) {
        snprintf(FileName, MAX_FILENAME_LENGTH, AMP_CFS_UT_ST_FN(_T%u_MF%u), TaskID, i + 1);
        strncpy(Config->fileName, FileName, MAX_FILENAME_LENGTH);
        if ((File = Stress_OpenFile(Config)) != NULL) {
            if ((Ret += Stress_WriteFileFromBuffer(File, FSize, Stress_GetWriteBufferByID(TaskID - 1))) != AMP_OK) {
                LOG_FAIL("Failed to read the file %s (Task %u)", FileName, TaskID);
            }
            if ((Ret += Stress_SyncFile(File)) != AMP_OK) {
                LOG_FAIL("Failed to sync the file %s (Task %u)", FileName, TaskID);
            }
            if ((Ret += Stress_SeekFile(File, 0, AMP_CFS_SEEK_START)) != AMP_OK) {
                LOG_FAIL("Failed to seek the file %s (Task %u)", FileName, TaskID);
            }
            if ((Ret += Stress_ReadFileToBuffer(File, FSize, Stress_GetReadBufferByID(TaskID))) != AMP_OK) {
                LOG_FAIL("Failed to read the file %s (Task %u)", FileName, TaskID);
            }
            if ((Ret += Stress_CloseFile(File)) != AMP_OK) {
                LOG_FAIL("Failed to close the file %s (Task %u)", FileName, TaskID);
            }
        } else {
            LOG_FAIL("Failed to open the file %s (Task %u)", FileName, TaskID);
            Ret += AMP_ERROR_GENERAL_ERROR;
        }
        AmpCFS_LogProbeOPEntries();
    }
    for (i = 0; i < AMP_CFS_UT_ST_MT_MULTIFILE_FILE_AMOUNT; i++) {
        snprintf(FileName, MAX_FILENAME_LENGTH, AMP_CFS_UT_ST_FN(_T%u_MF%u), TaskID, i + 1);
        Stress_WaitFileClose(FileName, TRUE);
        if ((Ret += Stress_RemoveFile(FileName)) != AMP_OK) {
            LOG_FAIL("Failed to remove the file: %s", FileName);
            break;
        }
    }
    TaskParams->Return = Ret;
#undef AMP_CFS_UT_ST_MT_MULTIFILE_FILE_AMOUNT
}

static int Stress_TestCase_MultiTask_MultiFileImpl(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    int Ret = AMP_OK;
    AMBA_KAL_TASK_t *Task1, *Task2;
    static AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s CmdParams1;
    static AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s CmdParams2;

    CmdParams1.Config = config;
    CmdParams1.CmdParams = (void *)fSize;
    CmdParams1.Return = AMP_OK;
    CmdParams2.Config = config;
    CmdParams2.CmdParams = (void *)fSize;
    CmdParams2.Return = AMP_OK;

    Ret += Stress_CreateTask(Stress_Task_MultiFile, (UINT32)&CmdParams1, "MT_MF_T1", &Task1);
    Ret += Stress_CreateTask(Stress_Task_MultiFile, (UINT32)&CmdParams2, "MT_MF_T2", &Task2);
    if (Ret != AMP_OK) {
        goto EXIT;
    }
    if (Task1 == NULL || Task2 == NULL) {
        Ret = AMP_ERROR_GENERAL_ERROR;
        LOG_FAIL("Failed to create tasks.");
        goto EXIT;
    }
    AmbaKAL_TaskResume(Task1);
    AmbaKAL_TaskResume(Task2);
    Stress_WaitTasksComplete();
EXIT:
    Ret += (CmdParams1.Return + CmdParams2.Return);
    LOG_RESULT(Ret);
    return Ret;
}

static int Stress_TestCase_MultiTask_MultiFile(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    int Ret = AMP_OK;
    if (Stress_IsSetTestCaseSyncMode(params->TestMode)) {
        LOG_MSG("Sync mode");
        Stress_SetOpenConfigSync(config);
        Ret += Stress_TestCase_MultiTask_MultiFileImpl(config, params->FileSize);
    }
    if (Stress_IsSetTestCaseAsyncMode(params->TestMode)) {
        LOG_MSG("Async mode");
        Stress_SetOpenConfigAsync(config);
        Ret += Stress_TestCase_MultiTask_MultiFileImpl(config, params->FileSize);
    }
    return Ret;
}

static int Stress_TestCase_Other(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    //AmbaVer_ShowVerInfo("libmw_cfs.a");
    return AMP_OK;
}

#if AMP_CFS_UT_COMPILE_WARNING
static int Stress_TestItem_MultiWrite_SameMode_AMmbaFS(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
#define AMP_CFS_UT_ST_MULTIWRITE_MODE_AMOUNT            (1)
#define AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT_PER_MODE   (2)
#define AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT             ((AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT_PER_MODE > AMP_CFS_UT_ST_SYS_FILE_OPEN_MAX) ? AMP_CFS_UT_ST_SYS_FILE_OPEN_MAX : AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT_PER_MODE)
    AMBA_FS_FILE* Files[AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT] = {0};
    const char WriteModes[AMP_CFS_UT_ST_MULTIWRITE_MODE_AMOUNT][3] = {{'r', '+', '\0'}};
    char CreateMode[]= "w+";
    UINT32 WriteSize, WriteCount = 0;
    int Ret = AMP_OK, i;

    if (config->isAsync) {
        goto EXIT;
    }
    /* Create the source file */
    if ((Files[0] = AmbaFS_fopen(AMP_CFS_UT_ST_FN_AMBAFS, CreateMode)) != NULL) {
        if (Stress_WriteFile_AmbaFS(Files[0], 1) != AMP_OK) {
            LOG_FAIL("Failed to write the source file %s", AMP_CFS_UT_ST_FN_AMBAFS);
           Ret = AMP_ERROR_IO_ERROR;
        }
        if (AmbaFS_fclose(Files[0]) != OK) {
            LOG_FAIL("Failed to close the source file %s", AMP_CFS_UT_ST_FN_AMBAFS);
            Ret = AMP_ERROR_IO_ERROR;
        }
    } else {
        LOG_FAIL("Failed to open the source file %s", AMP_CFS_UT_ST_FN_AMBAFS);
        Ret = AMP_ERROR_IO_ERROR;
    }
    if (Ret != AMP_OK) {
        goto EXIT;
    }
    /* Test file write modes */
    for (UINT8 m = 0; m < AMP_CFS_UT_ST_MULTIWRITE_MODE_AMOUNT; m++) {
        for (i = 0; i < AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT; i++) {
            if ((Files[i] = AmbaFS_fopen(AMP_CFS_UT_ST_FN_AMBAFS, WriteModes[m])) == NULL) {
                LOG_FAIL("Failed to open the file %s", AMP_CFS_UT_ST_FN_AMBAFS);
                Ret = AMP_ERROR_IO_ERROR;
            }
        }
        WriteSize = (fSize < 5) ? 1 : fSize / 5;
        //WriteSize = fSize / 4;
        while(Ret == AMP_OK && WriteCount < fSize) {
            for (i = 0; i <AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT; i++) {
                if (AmbaFS_Fsync(Files[i]) != AMP_OK) {
                    LOG_FAIL("Failed to sync the file %s", AMP_CFS_UT_ST_FN_AMBAFS);
                    Ret = AMP_ERROR_IO_ERROR;
                    break;
                }
                if (AmbaFS_fseek(Files[i], 0, AMBA_FS_SEEK_END) != AMP_OK) {
                    LOG_FAIL("Failed to seek the file %s", AMP_CFS_UT_ST_FN_AMBAFS);
                    Ret = AMP_ERROR_IO_ERROR;
                    break;
                }
                if (Stress_WriteFile_AmbaFS(Files[i], WriteSize) != AMP_OK) {
                    LOG_FAIL("Failed to write the source file with mode %hhu", i);
                    Ret = AMP_ERROR_IO_ERROR;
                    break;
                }
            }
            WriteCount += WriteSize;
        }
        for (i = 0; i < AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT; i++) {
            if (Files[i] != NULL && AmbaFS_fclose(Files[i]) != AMP_OK) {
                LOG_FAIL("Failed to close the file %s", AMP_CFS_UT_ST_FN_AMBAFS);
                Ret = AMP_ERROR_IO_ERROR;
            }
        }
    }
EXIT:
    LOG_RESULT(Ret);
    return Ret;
#undef AMP_CFS_UT_ST_MULTIWRITE_MODE_AMOUNT
#undef AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT_PER_MODE
#undef AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT
}

static int Stress_TestItem_MultiWrite_SameMode(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
#define AMP_CFS_UT_ST_MULTIWRITE_MODE_AMOUNT            (1)
#define AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT_PER_MODE   (2)
#define AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT             ((AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT_PER_MODE > AMP_CFS_UT_ST_SYS_FILE_OPEN_MAX) ? AMP_CFS_UT_ST_SYS_FILE_OPEN_MAX : AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT_PER_MODE)
    AMP_CFS_FILE_s* Files[AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT] = {0};
    UINT8 WriteModes[AMP_CFS_UT_ST_MULTIWRITE_MODE_AMOUNT] = {AMP_CFS_FILE_MODE_READ_WRITE};
    UINT32 WriteSize, WriteCount = 0;
    int Ret = AMP_OK, i;

    if (config->isAsync) {
        UINT8 BankAmount;
        config->maxNumBank = Stress_GetBankPerFile(AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT, &BankAmount);
        if (BankAmount < AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT) {
            LOG_FAIL("There is not enough banks to open files with async mode.");
            Ret = AMP_ERROR_GENERAL_ERROR;
            goto EXIT;
        }
    }
    /* Create the source file */
    if ((Ret = Stress_OpenWriteClose(config, 1)) != AMP_OK) {
        LOG_FAIL("Failed to create the source file %s", config->fileName);
        goto EXIT;
    }
    /* Test file write modes */
    for (UINT8 m = 0; m < AMP_CFS_UT_ST_MULTIWRITE_MODE_AMOUNT; m++) {
        config->fileMode = WriteModes[m];
        for (i = 0; i < AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT; i++) {
            if ((Files[i] = Stress_OpenFile(config)) == NULL) {
                LOG_FAIL("Failed to open the file %s", Files[i]->Filename);
                Ret = AMP_ERROR_IO_ERROR;
            }
        }
        WriteSize = (fSize < 5) ? 1 : fSize / 5;
        //WriteSize = fSize / 4;
        while(Ret == AMP_OK && WriteCount < fSize) {
            for (i = 0; i <AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT; i++) {
                if (Stress_SyncFile(Files[i]) != AMP_OK) {
                    LOG_FAIL("Failed to sync the file %s", Files[i]->Filename);
                    Ret = AMP_ERROR_IO_ERROR;
                    break;
                }
                if (Stress_SeekFileEnd(Files[i], 0) != AMP_OK) {
                    LOG_FAIL("Failed to seek the file %s", Files[i]->Filename);
                    Ret = AMP_ERROR_IO_ERROR;
                    break;
                }
                if (Stress_WriteFile(Files[i], WriteSize) != AMP_OK) {
                    LOG_FAIL("Failed to write the source file with mode %hhu", i);
                    Ret = AMP_ERROR_IO_ERROR;
                    break;
                }
            }
            WriteCount += WriteSize;
        }
        for (i = 0; i < AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT; i++) {
            if (Files[i] != NULL && Stress_CloseFile(Files[i]) != AMP_OK) {
                LOG_FAIL("Failed to close the file %s", Files[i]->Filename);
                Ret = AMP_ERROR_IO_ERROR;
            }
        }
    }
EXIT:
    LOG_RESULT(Ret);
    return Ret;
#undef AMP_CFS_UT_ST_MULTIWRITE_MODE_AMOUNT
#undef AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT_PER_MODE
#undef AMP_CFS_UT_ST_MULTIWRITE_FILE_AMOUNT
}

static int Stress_TestCase_MultiWriteImpl(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    int Ret = AMP_OK;
    Ret += Stress_TestItem_MultiWrite_SameMode_AMmbaFS(config, fSize);
    Ret += Stress_TestItem_MultiWrite_SameMode(config, fSize);
    return Ret;
}

static int Stress_TestCase_MultiWrite(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, void *params)
{
    UINT32 FSize = (UINT32) params;

    // Sync flow
    LOG_MSG("Sync mode");
    Stress_SetOpenConfigSync(config);
    Stress_TestCase_MultiWriteImpl(config, FSize);

    // Async flow
    LOG_MSG("Async mode");
    Stress_SetOpenConfigAsync(config);
    Stress_TestCase_MultiWriteImpl(config, FSize);
    return AMP_OK;
}
#endif

static void Stress_Task_Unmount_FileWrite(UINT32 param)
{
    int Ret = AMP_OK;
    AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s *TaskParams = (AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s *)param;
    AMP_CFS_UT_ST_OPEN_CONFIG_s *Config = TaskParams->Config;
    UINT32 FSize = (UINT32) TaskParams->CmdParams;
    UINT32 TaskID = Stress_GetTaskId(AmbaKAL_TaskIdentify());
    AMP_CFS_FILE_s* File;

    K_ASSERT(TaskID > 0 && TaskID <= AMP_CFS_UT_ST_TASK_AMOUNT);
    LOG_V_MSG("TaskID: %u", TaskID);
    Config->fileMode = AMP_CFS_FILE_MODE_WRITE_READ;
    LOG_MSG("Open the file %s", Config->fileName);
    if ((File = Stress_OpenFile(Config)) != NULL) {
        LOG_MSG("Write data to the file %s", Config->fileName);
        Stress_WriteFile(File, FSize);
        LOG_MSG("Close the file %s", Config->fileName);
        if (Stress_CloseFile(File) != AMP_OK) {
            LOG_FAIL("Failed to close the file %s", Config->fileName);
        }
        LOG_MSG("Wait the file to be closed");
        Stress_WaitFileClose(Config->fileName, FALSE);
    } else {
        LOG_FAIL("Failed to open the file %s", Config->fileName);
        Ret = AMP_ERROR_IO_ERROR;
    }
    LOG_MSG("Finish Task");
    TaskParams->Return = Ret;
}

static void Stress_Task_Unmount_FileRead(UINT32 param)
{
    int Ret = AMP_OK;
    AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s *TaskParams = (AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s *)param;
    AMP_CFS_UT_ST_OPEN_CONFIG_s *Config = TaskParams->Config;
    UINT32 FSize = (UINT32) TaskParams->CmdParams;
    UINT32 TaskID = Stress_GetTaskId(AmbaKAL_TaskIdentify());
    AMP_CFS_FILE_s* File;

    K_ASSERT(TaskID > 0 && TaskID <= AMP_CFS_UT_ST_TASK_AMOUNT);
    LOG_V_MSG("TaskID: %u", TaskID);
    Config->fileMode = AMP_CFS_FILE_MODE_READ_ONLY;
    LOG_MSG("Open the file %s", Config->fileName);
    if ((File = Stress_OpenFile(Config)) != NULL) {
        LOG_MSG("Read data from the file %s", Config->fileName);
        Stress_ReadFile(File, FSize);
        LOG_MSG("Close the file %s", Config->fileName);
        if (Stress_CloseFile(File) != AMP_OK) {
            LOG_FAIL("Failed to close the file %s", Config->fileName);
        }
        LOG_MSG("Wait the file to be closed");
        Stress_WaitFileClose(Config->fileName, FALSE);
    } else {
        LOG_FAIL("Failed to open the file %s", Config->fileName);
        Ret = AMP_ERROR_IO_ERROR;
    }
    LOG_MSG("Finish Task");
    TaskParams->Return = Ret;
}

static void Stress_Task_Unmount_IOInterrupt(UINT32 param)
{
    int Ret = AMP_OK;
    AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s *TaskParams = (AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s *)param;
    AMP_CFS_UT_ST_OPEN_CONFIG_s *Config = TaskParams->Config;
    UINT32 TaskID = Stress_GetTaskId(AmbaKAL_TaskIdentify());
    char Drive = AmpUT_CfsGetDrive(Config->fileName);

    K_ASSERT(TaskID > 0 && TaskID <= AMP_CFS_UT_ST_TASK_AMOUNT);
    LOG_V_MSG("TaskID: %u", TaskID);
    LOG_MSG("Unmount drive %c", Drive);
    AmpCFS_UnMount(Drive);
    LOG_MSG("Mount drive %c", Drive);
    Ret += AmpCFS_Mount(Drive);
    LOG_MSG("Finish Task");
    TaskParams->Return = Ret;
}

static int Stress_TestItem_MultiTask_Unmount_WriteInterrupt(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    int Ret = AMP_OK;
    int i;
    AMBA_KAL_TASK_t *Task1, *Task2;
    AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s CmdParams[2] = {0};
    char FileName[MAX_FILENAME_LENGTH] = {0};

    snprintf(FileName, MAX_FILENAME_LENGTH, AMP_CFS_UT_ST_FN(_UMNT_WI));
    strncpy(config->fileName, FileName, MAX_FILENAME_LENGTH);

    for (i = 0; i < 2; i++) {
        CmdParams[i].Config = config;
        CmdParams[i].CmdParams = (void *)fSize;
        CmdParams[i].Return = AMP_OK;
    }
    Ret += Stress_CreateTask(Stress_Task_Unmount_FileWrite, (UINT32)&CmdParams[0], "MT_UNMOUNT_T1", &Task1);
    Ret += Stress_CreateTask(Stress_Task_Unmount_IOInterrupt, (UINT32)&CmdParams[1], "MT_UNMOUNT_T2", &Task2);
    if (Ret != AMP_OK) {
        goto EXIT;
    }
    if (Task1 == NULL || Task2 == NULL) {
        Ret = AMP_ERROR_GENERAL_ERROR;
        LOG_FAIL("Failed to create tasks.");
        goto EXIT;
    }
    AmbaKAL_TaskResume(Task1);
    AmbaKAL_TaskSleep(1000);
    AmbaKAL_TaskResume(Task2);
    Stress_WaitTasksComplete();
EXIT:
    for (i = 0; i < 2; i++) {
        Ret += CmdParams[i].Return;
    }
    LOG_RESULT(Ret);
    return Ret;
}

static int Stress_TestItem_MultiTask_Unmount_ReadInterrupt(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    int Ret = AMP_OK;
    int i;
    AMBA_KAL_TASK_t *Task1, *Task2;
    AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s CmdParams[2] = {0};
    char FileName[MAX_FILENAME_LENGTH] = {0};

    snprintf(FileName, MAX_FILENAME_LENGTH, AMP_CFS_UT_ST_FN(_UMNT_RI));
    strncpy(config->fileName, FileName, MAX_FILENAME_LENGTH);
    Ret += Stress_OpenWriteClose(config, fSize);
    if (Ret != AMP_OK) {
        LOG_FAIL("Failed to write data to the file %s", config->fileName);
        return Ret;
    }
    Stress_WaitFileClose(config->fileName, FALSE);
    for (i = 0; i < 2; i++) {
        CmdParams[i].Config = config;
        CmdParams[i].CmdParams = (void *)fSize;
        CmdParams[i].Return = AMP_OK;
    }
    Ret += Stress_CreateTask(Stress_Task_Unmount_FileRead, (UINT32)&CmdParams[0], "MT_UNMOUNT_T1", &Task1);
    Ret += Stress_CreateTask(Stress_Task_Unmount_IOInterrupt, (UINT32)&CmdParams[1], "MT_UNMOUNT_T2", &Task2);
    if (Ret != AMP_OK) {
        goto EXIT;
    }
    if (Task1 == NULL || Task2 == NULL) {
        Ret = AMP_ERROR_GENERAL_ERROR;
        LOG_FAIL("Failed to create tasks.");
        goto EXIT;
    }
    AmbaKAL_TaskResume(Task1);
    AmbaKAL_TaskSleep(1000);
    AmbaKAL_TaskResume(Task2);
    Stress_WaitTasksComplete();
EXIT:
    for (i = 0; i < 2; i++) {
        Ret += CmdParams[i].Return;
    }
    LOG_RESULT(Ret);
    return Ret;
}

static int Stress_TestItem_MultiTask_Unmount_ReadWriteInterrupt(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    int Ret = AMP_OK;
    int i;
    AMBA_KAL_TASK_t *Task1, *Task2;
    AMP_CFS_UT_ST_TASK_READFILE_PARAMS_s CmdParams[2] = {0};
    AMP_CFS_UT_ST_OPEN_CONFIG_s WriteConfig;
    char Drive;

    strncpy(config->fileName, AMP_CFS_UT_ST_FN(_UMNT_RWI_1), MAX_FILENAME_LENGTH);
    Ret += Stress_OpenWriteClose(config, fSize);
    if (Ret != AMP_OK) {
        LOG_FAIL("Failed to write data to the file %s", config->fileName);
        return Ret;
    }
    Stress_WaitFileClose(config->fileName, FALSE);

    memcpy(&WriteConfig, config, sizeof(AMP_CFS_UT_ST_OPEN_CONFIG_s));
    strncpy(WriteConfig.fileName, AMP_CFS_UT_ST_FN(_UMNT_RWI_2), MAX_FILENAME_LENGTH);
    for (i = 0; i < 2; i++) {
        CmdParams[i].CmdParams = (void *)fSize;
        CmdParams[i].Return = AMP_OK;
    }
    CmdParams[0].Config = config;
    CmdParams[1].Config = &WriteConfig;
    Ret += Stress_CreateTask(Stress_Task_Unmount_FileRead, (UINT32)&CmdParams[0], "MT_UNMOUNT_T1", &Task1);
    Ret += Stress_CreateTask(Stress_Task_Unmount_FileWrite, (UINT32)&CmdParams[1], "MT_UNMOUNT_T2", &Task2);
    if (Ret != AMP_OK) {
        goto EXIT;
    }
    if (Task1 == NULL || Task2 == NULL) {
        Ret = AMP_ERROR_GENERAL_ERROR;
        LOG_FAIL("Failed to create tasks.");
        goto EXIT;
    }
    AmbaKAL_TaskResume(Task1);
    AmbaKAL_TaskResume(Task2);
    AmbaKAL_TaskSleep(1000);

    Drive = AmpUT_CfsGetDrive(config->fileName);
    LOG_MSG("Unmount drive %c", Drive);
    AmpCFS_UnMount(Drive);
    LOG_MSG("Mount drive %c", Drive);
    Ret += AmpCFS_Mount(Drive);

    Stress_WaitTasksComplete();
EXIT:
    for (i = 0; i < 2; i++) {
        Ret += CmdParams[i].Return;
    }
    LOG_RESULT(Ret);
    return Ret;
}

static int Stress_TestCase_MultiTask_UnmountImpl(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, UINT32 fSize)
{
    int Ret = AMP_OK;
    Ret += Stress_TestItem_MultiTask_Unmount_WriteInterrupt(config, fSize);
    Ret += Stress_TestItem_MultiTask_Unmount_ReadInterrupt(config, fSize);
    Ret += Stress_TestItem_MultiTask_Unmount_ReadWriteInterrupt(config, fSize);
    return Ret;
}

static int Stress_TestCase_MultiTask_Unmount(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *params)
{
    int Ret = AMP_OK;
    if (Stress_IsSetTestCaseSyncMode(params->TestMode)) {
        LOG_MSG("Sync mode");
        Stress_SetOpenConfigSync(config);
        Ret += Stress_TestCase_MultiTask_UnmountImpl(config, params->FileSize);
    }
    if (Stress_IsSetTestCaseAsyncMode(params->TestMode)) {
        LOG_MSG("Async mode");
        Stress_SetOpenConfigAsync(config);
        Ret += Stress_TestCase_MultiTask_UnmountImpl(config, params->FileSize);
    }
    return Ret;
}

static int Stress_TCO_SetDefault(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, void *params)
{
    Stress_GetOpenConfigDefault(config);
    config->id = AMP_CFS_UT_ST_OPEN_CONFIG_ID_DEFAULT;
    return AMP_OK;
}

static int Stress_TCO_SetAlignment(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, void *params)
{
    UINT32 Alignment = (params != NULL) ? (UINT32) params : g_AmpCfsUnitTest.ClusterSize;

    K_ASSERT(Alignment % g_AmpCfsUnitTest.ClusterSize == 0);
    Stress_GetOpenConfigDefault(config);
    config->alignment = Alignment;
    config->id = AMP_CFS_UT_ST_OPEN_CONFIG_ID_ALIGNMENT;
    return AMP_OK;
}

static int Stress_TCO_SetLowPriority(AMP_CFS_UT_ST_OPEN_CONFIG_s *config, void *params)
{
    Stress_GetOpenConfigDefault(config);
    config->isLowPriority = TRUE;
    config->id = AMP_CFS_UT_ST_OPEN_CONFIG_ID_LOWPRI;
    return AMP_OK;
}


static int Stress_TestSuite_AddAllCmds(AMP_CFS_UT_ST_TESTCASE_COMMAND_FP *cmds)
{
    UINT8 Cnt = 0;
    cmds[Cnt++] = Stress_TestCase_WriteRead;
    cmds[Cnt++] = Stress_TestCase_MultiOpen;
    cmds[Cnt++] = Stress_TestCase_MultiRead;
    cmds[Cnt++] = Stress_TestCase_MultiFile;
    cmds[Cnt++] = Stress_TestCase_MultiTask_MultiRead;
    cmds[Cnt++] = Stress_TestCase_MultiTask_MultiFile;
    K_ASSERT(Cnt <= AMP_CFS_UT_ST_TESTCASE_CMD_AMOUNT);
    return AMP_OK;
}

static int Stress_TCRunner_Run(AMP_CFS_UT_ST_CMD_PARAMS_s *params)
{
    int Res = AMP_OK;
    UINT32 i, j;
    AMP_CFS_UT_ST_TESTCASE_COMMAND_FP Cmd;
    AMP_CFS_UT_ST_OPEN_CONFIG_s *Config;
    LOGM_DECLARE(UINT32, TimeEntryId, 0);

    AmpCFS_LogProbeTimeEntryStart(&TimeEntryId);
    for (i = 0; i < AMP_CFS_UT_ST_TESTCASE_CMD_AMOUNT; i++) {
        if ((Cmd = params->TestCmds[i]) == NULL) {
            if (i == 0) {
                AMP_CFS_UT_PRINT(RED, "Unavailable command.");
                AMP_CFS_UT_PRINT(BLACK, "Please enter CFS stress test command:");
                AMP_CFS_UT_PRINT(BLACK, "t cfs stress_test [file size = 1:4095 (MiB)]");
                Res = AMP_ERROR_GENERAL_ERROR;
            }
            break;
        }
        for (j = 0; j < params->OpenConfigAmount; j++) {
            Config = params->OpenConfigs + j;
            LOG_MARK("Start, Open Config ID: %hhu", Config->id);
            Res += Cmd(Config, &params->TestCaseParams);
            LOG_MARK("End");
        }
    }
    AmpCFS_LogProbeTimeEntryEnd(TimeEntryId);
    return Res;
}

static int Stress_TCRunner_GetDefault(AMP_CFS_UT_ST_TEST_RUNNER_s *runner)
{
    runner->Init  = Stress_Init;
    runner->Run = Stress_TCRunner_Run;
    return AMP_OK;
}

#define AMP_CFS_UT_ST_CMD_ID_TEST           448
#define AMP_CFS_UT_ST_CMD_ID_OTHER          546
#define AMP_CFS_UT_ST_CMD_ID_SYNC_READ      952
#define AMP_CFS_UT_ST_CMD_ID_SYNC_WRITE     1095
#define AMP_CFS_UT_ST_CMD_ID_ASYNC_READ     1049
#define AMP_CFS_UT_ST_CMD_ID_ASYNC_WRITE    1192
#define AMP_CFS_UT_ST_CMD_ID_SIMPLE         650
#define AMP_CFS_UT_ST_CMD_ID_MULTIREAD      967
#define AMP_CFS_UT_ST_CMD_ID_MULTIWRITE     1110
#define AMP_CFS_UT_ST_CMD_ID_MULTIFILE      971
#define AMP_CFS_UT_ST_CMD_ID_MULTIOPEN      989
#define AMP_CFS_UT_ST_CMD_ID_MT_MULTIREAD   1287
#define AMP_CFS_UT_ST_CMD_ID_MT_MULTIFILE   1291
#define AMP_CFS_UT_ST_CMD_ID_MT_UNMOUNT_TEST    1653

static int Stress_AddTestCases(int id, AMP_CFS_UT_ST_CMD_PARAMS_s* params)
{

    switch(id) {
        case AMP_CFS_UT_ST_CMD_ID_TEST:
            Stress_TestSuite_AddAllCmds(params->TestCmds);
            break;
        case AMP_CFS_UT_ST_CMD_ID_SIMPLE:
            params->TestCmds[0] = Stress_TestCase_WriteRead;
            break;
        case AMP_CFS_UT_ST_CMD_ID_MULTIREAD:
            params->TestCmds[0] = Stress_TestCase_MultiRead;
            break;
        case AMP_CFS_UT_ST_CMD_ID_MULTIFILE:
            params->TestCmds[0] = Stress_TestCase_MultiFile;
            break;
        case AMP_CFS_UT_ST_CMD_ID_MULTIOPEN:
            params->TestCmds[0] = Stress_TestCase_MultiOpen;
            break;
        case AMP_CFS_UT_ST_CMD_ID_MT_MULTIREAD:
            params->TestCmds[0] = Stress_TestCase_MultiTask_MultiRead;
            break;
        case AMP_CFS_UT_ST_CMD_ID_MT_MULTIFILE:
            params->TestCmds[0] = Stress_TestCase_MultiTask_MultiFile;
            break;
        case AMP_CFS_UT_ST_CMD_ID_SYNC_READ:
            params->TestCmds[0] = Stress_TestCase_SyncRead;
            break;
        case AMP_CFS_UT_ST_CMD_ID_SYNC_WRITE:
            params->TestCmds[0] = Stress_TestCase_SyncWrite;
            break;
        case AMP_CFS_UT_ST_CMD_ID_ASYNC_READ:
            params->TestCmds[0] = Stress_TestCase_AsyncRead;
            break;
        case AMP_CFS_UT_ST_CMD_ID_ASYNC_WRITE:
            params->TestCmds[0] = Stress_TestCase_AsyncWrite;
            break;
        case AMP_CFS_UT_ST_CMD_ID_MT_UNMOUNT_TEST:
            params->TestCmds[0] = Stress_TestCase_MultiTask_Unmount;
            break;
        case AMP_CFS_UT_ST_CMD_ID_OTHER:
            params->TestCmds[0] = Stress_TestCase_Other;
            break;
        default:
            params->TestCmds[0] = NULL;
    }
    return AMP_OK;
}

static int Stress_AddOpenConfigs(AMP_CFS_UT_ST_CMD_PARAMS_s *params, UINT8 ocParam)
{
    AMP_CFS_UT_ST_OPEN_CONFIG_SETTER_FP OCSetters[8] = {NULL, NULL, NULL, NULL, NULL, Stress_TCO_SetLowPriority, Stress_TCO_SetAlignment, Stress_TCO_SetDefault};
    UINT8 BigMask[8] = AMP_CFS_UT_ST_BITMASK;
    AMP_CFS_UT_ST_OPEN_CONFIG_s *configs = params->OpenConfigs;
    int i = 8;

    while(i--) {
        if (((BigMask[i] & ocParam) != 0) && OCSetters[i] != NULL) {
            OCSetters[i](configs++, NULL);
        }
    }

    params->OpenConfigAmount = ((UINT32)configs - (UINT32)params->OpenConfigs) / sizeof(AMP_CFS_UT_ST_OPEN_CONFIG_s);
    K_ASSERT(params->OpenConfigAmount <= AMP_CFS_UT_ST_OPENCONFIG_AMOUNT);
    return AMP_OK;
}

static int Stress_GetCmdID(char *cmdName)
{
    // ync = 330, sync = 445, async = 542, _ = 95, read = 412, write = 555
    // sync_read: 952, async_read: 1049, sync_write: 1095, async_write: 1192
    // multi = 555, open = 434, file = 416, mt = 225, simple = 650
    // mount = 563, unmount = 790, test = 448
    int ID = 0;

    ID = atoi(cmdName);
    if (ID <= 0) {
        while(*cmdName) {
            ID += *cmdName;
            cmdName++;
        }
    }
    return ID;
}

static BOOL Stress_IsTestCaseParameters(char *str)
{
    return (strncmp(str, AMP_CFS_UT_ST_CMD_PARAM_PREFIX, AMP_CFS_UT_ST_CMD_PARAM_PREFIX_LEN) == 0 && strlen(str) > 1);
}

static int Stress_ParseCmdParams(int argc, char **argv, AMP_CFS_UT_ST_CMD_PARAMS_s **params)
{
    AMP_CFS_UT_ST_CMD_PARAMS_s *CmdParams;
    AMP_CFS_UT_ST_TEST_CASE_PARAMS_s *TestCaseParams;
    UINT8 OpenConfigParam = 0xFF;

    if (strncmp(argv[1], AMP_CFS_UT_ST_CMD_PREFIX, AMP_CFS_UT_ST_CMD_PREFIX_LEN) != 0 || strlen(argv[1]) <= AMP_CFS_UT_ST_CMD_PREFIX_LEN) {
        return AMP_ERROR_GENERAL_ERROR;
    }

    CmdParams = Stress_GetCmdParams();
    Stress_CleanCmdParams();
    CmdParams->IsStressTest = FALSE;
    TestCaseParams = &CmdParams->TestCaseParams;
    TestCaseParams->FileSize = 20;
    TestCaseParams->TestMode = AMP_CFS_UT_ST_TEST_CASE_MODE_ALL;

    if (argc > 2 && Stress_IsTestCaseParameters(argv[2]) == FALSE) {
        int fsmb = atoi(argv[2]);
        if (fsmb < 1 || fsmb > (4 << 10)) {
            LOG_MSG("Usage: The file size is from 1 to 1024 MiB.");
            return AMP_OK;
        }
        TestCaseParams->FileSize = fsmb;
    }

    for (int i = 2; i < argc; i++) {
        if (Stress_IsTestCaseParameters(argv[i]) == TRUE && i + 2 <= argc) {
            switch(argv[i][1]) {
            case 'o':
            {
                char *end;
                unsigned long oc = strtoul(argv[i + 1], &end, 10);
                if (errno != ERANGE && oc <= 0xFF) {
                    OpenConfigParam = (UINT8) oc;
                }
            }
            break;
            case 'm':
            {
                UINT8 Mode = atoi(argv[i + 1]);
                if (Mode <= AMP_CFS_UT_ST_TEST_CASE_MODE_ALL) {
                    TestCaseParams->TestMode = Mode;
                }
            }
            break;
            default:
                LOG_MSG("Invalid command parameters!");
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    }

    Stress_AddOpenConfigs(CmdParams, OpenConfigParam);
    Stress_AddTestCases(Stress_GetCmdID(argv[1] + AMP_CFS_UT_ST_CMD_PREFIX_LEN), CmdParams);
    CmdParams->IsStressTest = TRUE;
    *params = CmdParams;
    return AMP_OK;
}

/**
 * CFS TestCase - Basic and utility functions.
 */
static int Cfs_TestCmdList_Create(AMP_CFS_UT_TEST_CMD_LIST_s **list)
{
    int Ret = AMP_OK;
    ALLOC_MEM(Ret, list, sizeof(AMP_CFS_UT_TEST_CMD_LIST_s));
    (*list)->Count = AMP_CFS_UT_TEST_CMD_LIST_ITEM_MAX;
    return Ret;
}

static int Cfs_TestCmdList_FindFreeItem(AMP_CFS_UT_TEST_CMD_LIST_s *list, AMP_CFS_UT_TEST_CMD_s **item)
{
    int Ret = AMP_OK;
    int i;
    AMP_CFS_UT_TEST_CMD_s *Item;
    AMP_CFS_UT_TEST_CMD_LIST_s *CurList;
    AMP_CFS_UT_TEST_CMD_LIST_s *NewList = NULL;

    K_ASSERT(list != NULL);
    while(list != NULL) {
        CurList = list;
        for (i = 0; i < list->Count; i++) {
            Item = list->Items + i;
            if (Item->Hdlr == NULL && Item->Name == 0) {
                *item = Item;
                return Ret;
            }
        }
        list = list->NextList;
    }
    if ((Ret = Cfs_TestCmdList_Create(&NewList)) == AMP_OK) {
        CurList->NextList = NewList;
        *item = NewList->Items;
    }
    return Ret;
}

static int Cfs_TestCmdList_AddItem(AMP_CFS_UT_TEST_CMD_LIST_s *list, AMP_CFS_UT_TEST_CMD_s *item)
{
    AMP_CFS_UT_TEST_CMD_s *Item = NULL;

    if (Cfs_TestCmdList_FindFreeItem(list, &Item) != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    } else {
        size_t NameSize = strlen(item->Name);
        int Ret = (NameSize > 0) ? AMP_OK : AMP_ERROR_GENERAL_ERROR;

        ALLOC_MEM(Ret, &Item->Name, NameSize + 1);
        if (Ret == AMP_OK) {
            Item->Hdlr = item->Hdlr;
            strncpy(Item->Name, item->Name, NameSize);
        }
        return Ret;
    }
}

static int Cfs_Gdma_MemcpyImpl(UINT8 *dst, UINT8 *src, UINT32 size, AMP_CFS_GDMA_s *gdma)
{
    int Rest = size;
    int Offset = 0;

    memset(gdma, 0, sizeof(AMP_CFS_GDMA_s));
    gdma->PixelFormat = AMBA_GDMA_8_BIT;
    while (Rest > 0) {
        gdma->pSrc = src + Offset;
        gdma->pDest = dst + Offset;
        gdma->NumPixels = (Rest > AMBA_GDMA_MAX_WIDTH) ? AMBA_GDMA_MAX_WIDTH : Rest;
        if (AmbaGDMA_LinearCopy(gdma, AMBA_KAL_WAIT_FOREVER) != OK) {
            LOG_FAIL("Errors occurred in AmbaGDMA_LinearCopy.");
            return AMP_ERROR_GENERAL_ERROR;
        }
        Rest -= AMBA_GDMA_MAX_WIDTH;
        Offset += AMBA_GDMA_MAX_WIDTH;
    }
    if (AmbaGDMA_WaitCompletion(AMBA_KAL_WAIT_FOREVER) != OK) {
        LOG_FAIL("Errors occurred in AmbaGDMA_WaitCompletion.");
        return AMP_ERROR_GENERAL_ERROR;
    }
    return AMP_OK;
}

static int Cfs_Gdma_Memcpy(UINT8 *dst, UINT8 *src, UINT32 size, AMP_CFS_GDMA_s *gdma)
{
    int Rval;
    UINT8 *AlignedAddr;
    UINT8 *EndAddr;
    UINT8 *Source = src;
    UINT8 *Destination = dst;
    UINT32 DataSize = size;
    UINT32 Offset;
    LOG_V_MSG("dst: %p, src: %p, size: %d, gdma: %p", dst, src, size, gdma);

    if (size < AMBA_CACHE_LINE_SIZE * 2) {
        memcpy(dst, src, size);
        return AMP_OK;
    }
    AlignedAddr = AMP_CFS_UT_GET_NEXT_ALIGNED_ADDR(dst, AMBA_CACHE_LINE_SIZE);
    if (dst != AlignedAddr) {
        Offset = AlignedAddr - dst;
        memcpy(dst, src, Offset);
        DataSize -= Offset;
        Destination = AlignedAddr;
        Source = src + Offset;
    }
    EndAddr = dst + size;
    AlignedAddr = AMP_CFS_UT_GET_PREVIOUS_ALIGNED_ADDR(EndAddr, AMBA_CACHE_LINE_SIZE);
    if(AlignedAddr != EndAddr) {
        Offset = EndAddr - AlignedAddr;
        memcpy(AlignedAddr, src + size - Offset, Offset);
        DataSize -= Offset;
    }
    AmbaCache_Clean((void *)Source, DataSize);
    AmbaCache_Invalidate((void *)Destination, DataSize);
    Rval = Cfs_Gdma_MemcpyImpl(Destination, Source, DataSize, gdma);
    if (Rval == AMP_OK && (Rval = memcmp((void *)src, (void *)dst, size)) != OK) {
        LOG_FAIL("Copied Data mismatch.");
    }
    return Rval;
}

static UINT32 Cfs_Hash_Sdbm(char *str)
{
    UINT32 Hash = 0;

    while (*str) {
        Hash = *str++ + (Hash << 6) + (Hash << 16) - Hash;
    }
    return Hash;
}

static int Cfs_TestCmdSet_RegisterALL(void)
{
    int Ret = AMP_OK;
    TEST_CMD_ADD(Ret, init);
    TEST_CMD_ADD(Ret, sync_auto);
    TEST_CMD_ADD(Ret, async_auto);
    TEST_CMD_ADD(Ret, amba_auto);
    TEST_CMD_ADD(Ret, speed_test);
    TEST_CMD_ADD(Ret, alignment_test);
    TEST_CMD_ADD(Ret, read_test);
    TEST_CMD_ADD(Ret, stat_test);
    TEST_CMD_ADD(Ret, cstat_test);
    TEST_CMD_ADD(Ret, priority);
    TEST_CMD_ADD(Ret, small_write);
    TEST_CMD_ADD(Ret, mount_test);
    TEST_CMD_ADD(Ret, unmount_test);
    TEST_CMD_ADD(Ret, ls_test);
    TEST_CMD_ADD(Ret, ls);
    TEST_CMD_ADD(Ret, first);
    TEST_CMD_ADD(Ret, next);
    TEST_CMD_ADD(Ret, cls_test);
    TEST_CMD_ADD(Ret, cls);
    TEST_CMD_ADD(Ret, cfirst);
    TEST_CMD_ADD(Ret, cnext);
    TEST_CMD_ADD(Ret, clear_cache);
    TEST_CMD_ADD(Ret, cstat);
    TEST_CMD_ADD(Ret, fstatus);
    TEST_CMD_ADD(Ret, getdev);
    TEST_CMD_ADD(Ret, cgetdev);
    TEST_CMD_ADD(Ret, remove);
    TEST_CMD_ADD(Ret, move);
    TEST_CMD_ADD(Ret, chmod);
    TEST_CMD_ADD(Ret, combine);
    TEST_CMD_ADD(Ret, divide);
    TEST_CMD_ADD(Ret, cinsert);
    TEST_CMD_ADD(Ret, cdelete);
    TEST_CMD_ADD(Ret, mkdir);
    TEST_CMD_ADD(Ret, rmdir);
    TEST_CMD_ADD(Ret, format);
    TEST_CMD_ADD(Ret, mount);
    TEST_CMD_ADD(Ret, unmount);
    TEST_CMD_ADD(Ret, print_tree);
    TEST_CMD_ADD(Ret, gdma);
    TEST_CMD_ADD(Ret, memcpy);
    TEST_CMD_ADD(Ret, bank_size);
    TEST_CMD_ADD(Ret, logm_dump);
    TEST_CMD_ADD(Ret, hash_sdbm);
    return Ret;
}

/**
 * CFS TestCase - The implementation of all cfs test function.
 */

int Cfs_TestCase_sync_auto(AMP_CFS_UT_SHELL_ENV_s *env)
{
#define AMP_CFS_UT_FN_AMBA_SYNC1 "C:\\AMBA_SYNC1.txt"
#define AMP_CFS_UT_FN_AMBA_SYNC2 "C:\\AMBA_SYNC2.txt"
    char  cBuf[] = "This is a test.";
    UINT32 tStart, tEnd;
    int   i;

    /** Initialize CFS */
    if (AmpUT_CfsInit() == AMP_OK) {
       AMP_CFS_UT_PRINT(GREEN, "init: Pass");
    } else {
       AMP_CFS_UT_PRINT(RED, "init: Fail");
       return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get current time */
    AmbaUtility_GetTimeStart(&tStart);
    // Write mode test : small data test and big data test
    //functions : fopen, fwrite, FSync, ftell, FgetLen, fseek, fread, fclose
    AmbaPrint("Synchronous write mode test");
    AmbaPrint("writing small data...");
    if (AmpUT_CfsWriteModeTest(AMP_CFS_UT_FN_AMBA_SYNC1, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT / 2, (UINT8 *)cBuf, sizeof(cBuf)) != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    AmbaPrint("writing big data...");
    for (i=0;i<AMP_CFS_UT_BIG_TEST_BUF_SIZE;i++) {
        g_AmpCfsUnitTest.pBufferTest[i] = i % AMP_CFS_UT_BYTE_TEST_BASE;
    }
    if (AmpUT_CfsWriteModeTest(AMP_CFS_UT_FN_AMBA_SYNC2, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT / 2, g_AmpCfsUnitTest.pBufferTest, AMP_CFS_UT_BIG_TEST_BUF_SIZE) != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    //Read mode test : small data test and big data test
    //functions : fopen, ftell, FgetLen, fseek, fread, fclose
    AmbaPrint("Synchronous read mode test");
    AmbaPrint("Reading big data...");
    if (AmpUT_CfsReadModeTest(AMP_CFS_UT_FN_AMBA_SYNC2, AMP_CFS_FILE_SYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT / 2, g_AmpCfsUnitTest.pBufferTest, AMP_CFS_UT_BIG_TEST_BUF_SIZE) == AMP_OK) {
        for (i=0;i<AMP_CFS_UT_BIG_TEST_BUF_SIZE;i++) {
            if (g_AmpCfsUnitTest.pBufferTest[i] != i % AMP_CFS_UT_BYTE_TEST_BASE) {
                AMP_CFS_UT_PRINT(RED, "read: Fail");
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    } else return AMP_ERROR_GENERAL_ERROR;
    AmbaPrint("Test complete!");
    AmbaUtility_GetTimeStart(&tEnd);
    AmbaPrint("Elapsed Time = %d", tEnd - tStart);
    AmbaKAL_TaskSleep(50);
    return AMP_OK;
}

int Cfs_TestCase_async_auto(AMP_CFS_UT_SHELL_ENV_s *env)
{
#define AMP_CFS_UT_FN_AMBA_ASYNC1 "C:\\AMBA_ASYNC1.txt"
#define AMP_CFS_UT_FN_AMBA_ASYNC2 "C:\\AMBA_ASYNC2.txt"
    char  cBuf[] = "This is a test.";
    UINT32 tStart, tEnd;
    int   i;

    /** Initialize CFS */
    if (AmpUT_CfsInit() == AMP_OK) {
       AMP_CFS_UT_PRINT(GREEN, "init: Pass");
    } else {
       AMP_CFS_UT_PRINT(RED, "init: Fail");
       return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get current time */
    AmbaUtility_GetTimeStart(&tStart);

    // Write mode test : small data test and big data test
    //       functions : fopen, fwrite, FSync, ftell, FgetLen, fseek, fread, fclose
    AmbaPrint("Asynchronous write mode test");
    AmbaPrint("writing small data...");
    if (AmpUT_CfsWriteModeTest(AMP_CFS_UT_FN_AMBA_ASYNC1, AMP_CFS_FILE_ASYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT / 2, (UINT8 *)cBuf, sizeof(cBuf)) != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    AmbaPrint("writing big data...");
    for (i=0;i<AMP_CFS_UT_BIG_TEST_BUF_SIZE;i++) {
        g_AmpCfsUnitTest.pBufferTest[i] = i % AMP_CFS_UT_BYTE_TEST_BASE;
    }
    if (AmpUT_CfsWriteModeTest(AMP_CFS_UT_FN_AMBA_ASYNC2, AMP_CFS_FILE_ASYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT / 2, g_AmpCfsUnitTest.pBufferTest, AMP_CFS_UT_BIG_TEST_BUF_SIZE) != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    //Read mode test : small data test and big data test
    //      functions : fopen, ftell, FgetLen, fseek, fread, fclose
    AmbaPrint("Asynchronous read mode test");
    AmbaPrint("Reading big data...");
    if (AmpUT_CfsReadModeTest(AMP_CFS_UT_FN_AMBA_ASYNC2, AMP_CFS_FILE_ASYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT / 2, g_AmpCfsUnitTest.pBufferTest, AMP_CFS_UT_BIG_TEST_BUF_SIZE) == AMP_OK) {
        for (i=0;i<AMP_CFS_UT_BIG_TEST_BUF_SIZE;i++) {
            if (g_AmpCfsUnitTest.pBufferTest[i] != i % AMP_CFS_UT_BYTE_TEST_BASE) {
                AMP_CFS_UT_PRINT(RED, "read: Fail on %u %x", (UINT32)i, (UINT32)g_AmpCfsUnitTest.pBufferTest[i]);
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    } else return AMP_ERROR_GENERAL_ERROR;
    AmbaPrint("Test complete!");
    AmbaUtility_GetTimeStart(&tEnd);
    AmbaPrint("Elapsed Time = %d", tEnd - tStart);
    AmbaKAL_TaskSleep(50);
    return AMP_OK;
}

int Cfs_TestCase_gdma(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int i;
    int Ret;
    UINT8 *WBuf = Stress_GetWriteBuffer();
    UINT32 DataSize = AMP_CFS_UT_ST_GDMA_BUF_LEN;
    UINT8 *Source = (UINT8*)(((UINT32)WBuf + AMBA_CACHE_LINE_SIZE - 1) & (-AMBA_CACHE_LINE_SIZE));
    UINT8 *Destination = Source + DataSize;
    AMP_CFS_GDMA_s Gdma = {0};

    LOG_MSG("CFS Test Case: GDMA copy");
    Source += 1;
    Destination += 1;
    for (i = 0; i < AMP_CFS_UT_ST_GDMA_BUF_LEN; i++) {
        Source[i] = i % AMP_CFS_UT_BYTE_TEST_BASE;
    }
    memset(Destination, 0, AMP_CFS_UT_ST_GDMA_BUF_LEN);
    Ret = Cfs_Gdma_Memcpy(Destination, Source, DataSize, &Gdma);
    LOG_RESULT(Ret);
    return Ret;
}

int Cfs_TestCase_memcpy(AMP_CFS_UT_SHELL_ENV_s *env)
{
#define AMP_CFS_UT_MEMCPY_SIZE_LEN  10
    UINT8 *Source = g_AmpCfsUnitTest.STWriteBuffer + 8;
    UINT8 *Destination = Source + AMP_CFS_UT_ST_BUF_UINT;
    UINT32 DataSizes[AMP_CFS_UT_MEMCPY_SIZE_LEN + 1] = {1024};
    UINT32 StartTime, EndTime;
    UINT32 Count = 10000;
    int i, j;

    if (env->Argc > 2) {
        Count = atoi(env->Argv[2]);
        if (Count < 1) {
            Count = 10000;
        }
    }
    LOG_MSG("CFS Test Case: GDMA copy");
    for (i = 0; i < AMP_CFS_UT_ST_BUF_UINT; i++) {
        Source[i] = i % AMP_CFS_UT_BYTE_TEST_BASE;
    }
    for (i = 1; i <= AMP_CFS_UT_MEMCPY_SIZE_LEN; i++) {
        DataSizes[i] = DataSizes[0] << i;
    }
    for (i = 0; i <= AMP_CFS_UT_MEMCPY_SIZE_LEN; i++) {
        AmbaUtility_GetTimeStart(&StartTime);
        for (j = 0; j < Count; j++) {
            memcpy(Destination, Source, DataSizes[i]);
        }
        AmbaUtility_GetTimeStart(&EndTime);
        LOG_MSG("Size: %u, Elapsed Time: %u", DataSizes[i], EndTime - StartTime);
    }

    return AMP_OK;
}

int Cfs_TestCase_cls(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Ret = AMP_ERROR_GENERAL_ERROR;
    UINT32 tStart, tEnd;
    AMP_CFS_DTA_s Dta = {0};
    unsigned char ucAttr = AMP_CFS_ATTR_ALL;
    char pwcPattern[MAX_FILENAME_LENGTH]= "*";

    if (env->Argc >= 3) {
        memset(pwcPattern, 0, MAX_FILENAME_LENGTH);
        strncpy(pwcPattern, env->Argv[2], MAX_FILENAME_LENGTH);
    }
    if (env->Argc >= 4) {
        ucAttr = (unsigned char) atoi(env->Argv[3]);
    }
    if (AmpUT_CfsInit() == AMP_OK) {
        AmbaUtility_GetTimeStart(&tStart);
        LOG_MSG("Pattern:%s, Attr:0x%hhX, DTA:%p", pwcPattern, ucAttr, &Dta);
        LOG_PRT();
        for (Ret = AmpUT_CfsFirstDirEnt(pwcPattern, ucAttr, &Dta); Ret == AMP_OK; Ret = AmpUT_CfsNextDirEnt(&Dta)) {
            AmbaKAL_TaskSleep(10);
            AmpUT_CfsPrintDtaLineCfs(&Dta);
        }
        AmbaUtility_GetTimeStart(&tEnd);
        AmbaPrint("Elapsed Time: %d", tEnd - tStart);
    }
    return (Ret == AMP_OK || Ret == AMP_ERROR_OBJ_UNAVAILABLE) ? AMP_OK : AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_ls(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Ret = AMP_ERROR_GENERAL_ERROR;
    UINT32 tStart, tEnd;
    AMBA_FS_DTA_t Dta = {0};
    char pwcPattern[MAX_FILENAME_LENGTH] = "*";
    unsigned char ucAttr = ATTR_ALL;

    if (env->Argc == 3) {
        memset(pwcPattern, 0, MAX_FILENAME_LENGTH);
        strncpy(pwcPattern, env->Argv[2], MAX_FILENAME_LENGTH);
    }
    if (AmpUT_CfsInit() == AMP_OK) {
        AmbaUtility_GetTimeStart(&tStart);
        LOG_MSG("Pattern:%s, Attr:0x%hhX, DTA:%p", pwcPattern, ucAttr, &Dta);
        LOG_PRT();
        for (Ret = AmpUT_CfsAmbsFsFirstDirEnt(pwcPattern, ucAttr, &Dta); Ret == AMP_OK; Ret = AmpUT_CfsAmbsFsNextDirEnt(&Dta)) {
            AmbaKAL_TaskSleep(10);
            AmpUT_CfsPrintDtaLineAmbaFs(&Dta);
        }
        AmbaUtility_GetTimeStart(&tEnd);
        AmbaPrint("Elapsed Time: %d", tEnd - tStart);
    }
    return (Ret != AMP_ERROR_GENERAL_ERROR) ? AMP_OK : AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cfirst(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Ret = AMP_ERROR_GENERAL_ERROR;
    int Argc = env->Argc;
    AMP_CFS_DTA_s *Dta;
    UINT8 DtaIdx = 0;
    char Pattern[MAX_FILENAME_LENGTH] = {0};
    unsigned char Attr = AMP_CFS_ATTR_ALL;
    BOOL8 IsCleanDta = TRUE;

    if (Argc > 2) {
        strncpy(Pattern, env->Argv[2], MAX_FILENAME_LENGTH);
        if (!IS_FULL_PATH(Pattern)) {
            LOG_FAIL("The pattern is not a full path. Pattern: %s", Pattern);
            return AMP_ERROR_GENERAL_ERROR;
        }
    }
    if (Argc > 3) {
        DtaIdx = (UINT8) atoi(env->Argv[3]);
        if (DtaIdx >= AMP_CFS_UT_DTA_NUM) {
            LOG_FAIL("Invalid DTA index.");
            return AMP_ERROR_GENERAL_ERROR;
        }
    } else {
        LOG_PRT("Usage: t cfs cfirst {Pattern} {DTA Index} [File Attribute]");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (Argc > 4) {
        for (int i = 4; i < Argc; i++) {
            if (env->Argv[i][0] == '-') {
                switch (env->Argv[i][1]) {
                case 'a':
                    if (Argc < i + 1) {
                        LOG_FAIL("Invalid parameter.");
                        return AMP_ERROR_GENERAL_ERROR;
                    }
                    Attr = (UINT8) atoi(env->Argv[i + 1]);
                    if (Attr == 0) {
                        LOG_FAIL("Invalid File Attribute.");
                        return AMP_ERROR_GENERAL_ERROR;
                    }
                    break;
                case 'n':
                    IsCleanDta = (BOOL8)atoi(env->Argv[i + 1]);
                }
            }
        }

    }

    Dta = AmpUT_CfsGetDta(DtaIdx, IsCleanDta);
    LOG_MSG("Pattern:%s, Attr:0x%hhX", Pattern, Attr);
    if (Dta != NULL && (Ret = AmpUT_CfsFirstDirEnt(Pattern, Attr, Dta)) == AMP_OK) {
        LOG_MSG("DTA:%p, Search:%p", Dta, AmpUT_CfsGetDtaRawInfo(Dta));
        AmbaKAL_TaskSleep(10);
        AmpUT_CfsPrintDtaLineCfs(Dta);
#ifdef AMP_CFS_CACHE_VERBOSE
        AmpUT_CfsPrintDtaCfs(&Dta);
#endif
    }
    return (Ret == AMP_OK || Ret == AMP_ERROR_OBJ_UNAVAILABLE) ? AMP_OK : AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_first(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Ret = AMP_ERROR_GENERAL_ERROR;
    if (env->Argc == 3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            AMBA_FS_DTA_t* Dta = AmpUT_CfsGetAmbaFsDta(TRUE);
            unsigned char ucAttr = ATTR_ALL;

            if (Dta != NULL && (Ret = AmpUT_CfsAmbsFsFirstDirEnt(env->Argv[2], ucAttr, Dta)) == AMP_OK) {
                AmpUT_CfsPrintDtaLineAmbaFs(Dta);
#ifdef AMP_CFS_CACHE_VERBOSE
                AmpUT_CfsPrintDtaAmbaFs(&Dta);
#endif
            }
        }
    } else {
        AmbaPrint("Usage : t cfs first pattern");
    }
    return (Ret != AMP_ERROR_GENERAL_ERROR) ? AMP_OK : AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cnext(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Ret = AMP_ERROR_GENERAL_ERROR;
    AMP_CFS_DTA_s* Dta;
    UINT8 DtaIdx = 0;

    if (env->Argc > 2) {
        DtaIdx = (UINT8) atoi(env->Argv[2]);
        if (DtaIdx >= AMP_CFS_UT_DTA_NUM) {
            LOG_FAIL("Invalid DTA index.");
            return AMP_ERROR_GENERAL_ERROR;
        }
    } else {
        LOG_PRT("Usage: t cfs cnext {DTA Index}");
        return AMP_ERROR_GENERAL_ERROR;
    }

    Dta = AmpUT_CfsGetDta(DtaIdx, FALSE);
    LOG_MSG("DTA:%p, Search:%p", Dta, AmpUT_CfsGetDtaRawInfo(Dta));
    if (Dta != NULL && (Ret = AmpUT_CfsNextDirEnt(Dta)) == AMP_OK) {
        AmbaKAL_TaskSleep(10);
        AmpUT_CfsPrintDtaLineCfs(Dta);
#ifdef AMP_CFS_CACHE_VERBOSE
        AmpUT_CfsPrintDtaCfs(&Dta);
#endif
    }
    return (Ret == AMP_OK || Ret == AMP_ERROR_OBJ_UNAVAILABLE) ? AMP_OK : AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_next(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Ret = AMP_ERROR_GENERAL_ERROR;
    if (AmpUT_CfsInit() == AMP_OK) {
        AMBA_FS_DTA_t *Dta = AmpUT_CfsGetAmbaFsDta(FALSE);
        if (Dta != NULL && (Ret = AmpUT_CfsAmbsFsNextDirEnt(Dta)) == AMP_OK) {
            AmpUT_CfsPrintDtaLineAmbaFs(Dta);
#ifdef AMP_CFS_CACHE_VERBOSE
            AmpUT_CfsPrintDtaAmbaFs(&Dta);
#endif
        }
    }
    return (Ret != AMP_ERROR_GENERAL_ERROR) ? AMP_OK : AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_speed_test(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMP_CFS_FILE_s *pFile = NULL;
    AMBA_FS_FILE *pAmbaFile;
    char pwcFile1[MAX_FILENAME_LENGTH] = "C:\\SPEED_SYNC.txt";
    char pwcFile2[MAX_FILENAME_LENGTH] = "C:\\SPEED_ASYN.txt";
    char pwcFile3[MAX_FILENAME_LENGTH] = "C:\\SPEED_AMBA.txt";
    char pwcMode[] = "w+";
    UINT32 tStart, tEnd;
    int    iAsyncMode, i, j;

    /** Get Parameters : Asyncmode */
    if (env->Argc < 3) iAsyncMode = 0;
    else          iAsyncMode = atoi(env->Argv[2]);
    /** Initialize CFS */
    if (AmpUT_CfsInit() == AMP_OK) {
        AMP_CFS_UT_PRINT(GREEN, "init: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "init: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Prepare data */
    for (i=0;i<AMP_CFS_UT_BIG_TEST_BUF_SIZE;i++) {
        g_AmpCfsUnitTest.pBufferTest[i] = i % AMP_CFS_UT_BYTE_TEST_BASE;
    }
    /** Open file */
    switch (iAsyncMode) {
    case 0 :
        AmbaPrint("Synchronous mode speed test!");
        pFile = AmpUT_CfsOpen(pwcFile1, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, 0, AMP_CFS_UT_PRIORITY_DEFAULT);
        if (pFile != NULL) {
           AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
        } else {
           AMP_CFS_UT_PRINT(RED, "open file : Fail");
           return AMP_ERROR_GENERAL_ERROR;
        }
        break;
    case 1:
        AmbaPrint("Asynchronous mode speed test!");
        pFile = AmpUT_CfsOpen(pwcFile2, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_ASYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
        if (pFile != NULL) {
           AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
        } else {
           AMP_CFS_UT_PRINT(RED, "open file : Fail");
           return AMP_ERROR_GENERAL_ERROR;
        }
        break;
    default:
        AmbaPrint("AmbaFS mode speed test!");
        pAmbaFile = AmbaFS_fopen((const char *)pwcFile3, pwcMode);
        if (pAmbaFile != NULL) {
           AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
        } else {
           AMP_CFS_UT_PRINT(RED, "open file : Fail");
           return AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Get current time */
    AmbaUtility_GetTimeStart(&tStart);
    /** Seek and Write test */
    AmbaPrint("write test");
    if ((iAsyncMode==0)||(iAsyncMode==1)) {
        /** Write 400M */
        for (i=0;i<1024;i++) {
            /** write 10K */
            for (j=0;j<10;j++) {
                if (AmpUT_CfsWrite(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 10240, pFile) != AMP_OK) {
                    AMP_CFS_UT_PRINT(RED, "write %dth byte : Fail", i);
                    if (AmpUT_CfsClose(pFile) != AMP_OK) {
                       AMP_CFS_UT_PRINT(RED, "close file: Fail");
                    }
                    return AMP_ERROR_GENERAL_ERROR;
                }
            }
            /** write 300K */
            if (AmpUT_CfsWrite(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 307200, pFile) != AMP_OK) {
                AMP_CFS_UT_PRINT(RED, "write %dth byte : Fail", i);
                if (AmpUT_CfsClose(pFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    } else {
        /** Write 400M */
        for (i=0;i<1024;i++) {
            /** write 10K */
            for (j=0;j<10;j++) {
                if (AmbaFS_fwrite(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 10240, pAmbaFile) != 10240) {
                    AMP_CFS_UT_PRINT(RED, "write %dth byte : Fail", i);
                    if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
                       AMP_CFS_UT_PRINT(RED, "close file: Fail");
                    }
                    return AMP_ERROR_GENERAL_ERROR;
                }
            }
            /** write 300K */
            if (AmbaFS_fwrite(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 307200, pAmbaFile) != 307200) {
                AMP_CFS_UT_PRINT(RED, "write %dth byte : Fail", i);
                if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    }
    AMP_CFS_UT_PRINT(GREEN, "Pass!");
    /** Close File */
    if ((iAsyncMode==0)||(iAsyncMode==1)) {
        if (AmpUT_CfsClose(pFile) != AMP_OK) {
            AMP_CFS_UT_PRINT(RED, "close file: Fail");
        }
    } else {
        if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
            AMP_CFS_UT_PRINT(RED, "close file: Fail");
        }
    }
    AmbaUtility_GetTimeStart(&tEnd);
    AmbaPrint("Elapsed Time = %d", tEnd - tStart);
    return AMP_OK;
}

int Cfs_TestCase_alignment_test(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMP_CFS_FILE_s *pFile = NULL;
    AMBA_FS_FILE *pAmbaFile;
    char  pwcFile1[MAX_FILENAME_LENGTH] = "C:\\ALIGN_SYNC.txt";
    char  pwcFile2[MAX_FILENAME_LENGTH] = "C:\\ALIGN_ASYN.txt";
    char  pwcFile3[MAX_FILENAME_LENGTH] = "C:\\ALIGN_AMBA.txt";
    char  pwcMode[]="w+";
    UINT32 tStart, tEnd;
    UINT32 uAlignment = 0;
    int    iAsyncMode = 0, i, j;

    /** Get Parameters : Asyncmode */
    if (env->Argc == 4) {
        iAsyncMode = atoi(env->Argv[2]);
        uAlignment = atoi(env->Argv[3]);
    } else {
        AmbaPrint("Usage: t cfs alignment_test asyncmode alignment");
    }
    /** Initialize CFS */
    if (AmpUT_CfsInit() == AMP_OK) {
        AMP_CFS_UT_PRINT(GREEN, "init: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "init: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Prepare data */
    for (i=0;i<AMP_CFS_UT_BIG_TEST_BUF_SIZE;i++) {
        g_AmpCfsUnitTest.pBufferTest[i] = i % AMP_CFS_UT_BYTE_TEST_BASE;
    }
    /** Open file */
    switch (iAsyncMode) {
    case 0 :
        AmbaPrint("Synchronous mode speed test!");
        AmbaPrint("Alignment = %d", uAlignment);
        pFile = AmpUT_CfsOpen(pwcFile1, AMP_CFS_FILE_MODE_WRITE_READ, uAlignment, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, 0, AMP_CFS_UT_PRIORITY_DEFAULT);
        if (pFile != NULL) {
           AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
        } else {
           AMP_CFS_UT_PRINT(RED, "open file : Fail");
           return AMP_ERROR_GENERAL_ERROR;
        }
        break;
    case 1:
        AmbaPrint("Asynchronous mode speed test!");
        AmbaPrint("Alignment = %d", uAlignment);
        pFile = AmpUT_CfsOpen(pwcFile2, AMP_CFS_FILE_MODE_WRITE_READ, uAlignment, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_ASYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
        if (pFile != NULL) {
           AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
        } else {
           AMP_CFS_UT_PRINT(RED, "open file : Fail");
           return AMP_ERROR_GENERAL_ERROR;
        }
        break;
    default:
        AmbaPrint("AmbaFS mode speed test!");
        AmbaPrint("No Alignment");
        pAmbaFile = AmbaFS_fopen((const char *)pwcFile3, pwcMode);
        if (pAmbaFile != NULL) {
           AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
        } else {
           AMP_CFS_UT_PRINT(RED, "open file : Fail");
           return AMP_ERROR_GENERAL_ERROR;
        }
    }
    /** Get current time */
    AmbaUtility_GetTimeStart(&tStart);
    /** Seek and Write test */
    AmbaPrint("write test");
    if ((iAsyncMode==0)||(iAsyncMode==1)) {
        /** Write 100M */
        for (i=0;i<256;i++) {
            /** write 10K */
            for (j=0;j<10;j++) {
                if (AmpUT_CfsWrite(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 10240, pFile) != AMP_OK) {
                    AMP_CFS_UT_PRINT(RED, "write %dth byte : Fail", i);
                    if (AmpUT_CfsClose(pFile) != AMP_OK) {
                       AMP_CFS_UT_PRINT(RED, "close file: Fail");
                    }
                    return AMP_ERROR_GENERAL_ERROR;
                }
            }
            /** write 300K */
            if (AmpUT_CfsWrite(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 307200, pFile) != AMP_OK) {
                AMP_CFS_UT_PRINT(RED, "write %dth byte : Fail", i);
                if (AmpUT_CfsClose(pFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    } else {
        /** Write 100M */
        for (i=0;i<256;i++) {
            /** write 10K */
            for (j=0;j<10;j++) {
                if (AmbaFS_fwrite(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 10240, pAmbaFile) != 10240) {
                    AMP_CFS_UT_PRINT(RED, "write %dth byte : Fail", i);
                    if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
                       AMP_CFS_UT_PRINT(RED, "close file: Fail");
                    }
                    return AMP_ERROR_GENERAL_ERROR;
                }
            }
            /** write 300K */
            if (AmbaFS_fwrite(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 307200, pAmbaFile) != 307200) {
                AMP_CFS_UT_PRINT(RED, "write %dth byte : Fail", i);
                if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    }
    AMP_CFS_UT_PRINT(GREEN, "Pass!");
    /** Close File */
    if ((iAsyncMode==0)||(iAsyncMode==1)) {
        if (AmpUT_CfsClose(pFile) != AMP_OK) {
            AMP_CFS_UT_PRINT(RED, "close file: Fail");
        }
    } else {
        if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
            AMP_CFS_UT_PRINT(RED, "close file: Fail");
        }
    }
    AmbaUtility_GetTimeStart(&tEnd);
    AmbaPrint("Elapsed Time = %d", tEnd - tStart);
    return AMP_OK;
}

int Cfs_TestCase_read_test(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMP_CFS_FILE_s *pFile = NULL;
    AMBA_FS_FILE *pAmbaFile;
    char pwcFile1[MAX_FILENAME_LENGTH] = "C:\\SPEED_SYNC.txt";
    char pwcFile2[MAX_FILENAME_LENGTH] = "C:\\SPEED_ASYN.txt";
    char pwcFile3[MAX_FILENAME_LENGTH] = "C:\\SPEED_AMBA.txt";
    char pwcMode[] = "r";
    UINT64 Pos;
    UINT32 tStart, tEnd;
    int    iAsyncMode = 0, i, j;

    /** Get Parameters : Asyncmode */
    if (env->Argc == 3) {
        iAsyncMode = atoi(env->Argv[2]);
    } else {
        AmbaPrint("Usage: t cfs read_speed_test mode(0:sync, 1:async, 2:ambaFS)");
    }
    /** Initialize CFS */
    if (AmpUT_CfsInit() == AMP_OK) {
        AMP_CFS_UT_PRINT(GREEN, "init: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "init: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Open file */
    switch (iAsyncMode) {
        case 0 :
            AmbaPrint("Synchronous mode speed test!");
            pFile = AmpUT_CfsOpen(pwcFile1, AMP_CFS_FILE_MODE_READ_ONLY, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, 0, AMP_CFS_UT_PRIORITY_DEFAULT);
            if (pFile != NULL) {
               AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
            } else {
               AMP_CFS_UT_PRINT(RED, "open file : Fail");
               return AMP_ERROR_GENERAL_ERROR;
            }
            break;
        case 1:
            AmbaPrint("Asynchronous mode speed test!");
            pFile = AmpUT_CfsOpen(pwcFile2, AMP_CFS_FILE_MODE_READ_ONLY, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_ASYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
            if (pFile != NULL) {
               AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
            } else {
               AMP_CFS_UT_PRINT(RED, "open file : Fail");
               return AMP_ERROR_GENERAL_ERROR;
            }
            break;
        default:
            AmbaPrint("AmbaFS mode speed test!");
            pAmbaFile = AmbaFS_fopen((const char *)pwcFile3, pwcMode);
            if (pAmbaFile != NULL) {
               AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
            } else {
               AMP_CFS_UT_PRINT(RED, "open file : Fail");
               return AMP_ERROR_GENERAL_ERROR;
            }
    }
    /** Get current time */
    AmbaUtility_GetTimeStart(&tStart);
    /** Seek and Write test */
    AmbaPrint("read test");
    Pos = 0;
    if ((iAsyncMode==0)||(iAsyncMode==1)) {
        for (i=0;i<1024;i++) {
            for (j=0;j<10;j++) {
                AmpUT_CfsSeek(pFile, Pos, 0);
                if (AmpUT_CfsRead(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 10240 , pFile) == AMP_OK) {
                    Pos += 10240;
                    for (int k=0;k<10240;k++) {
                        if (g_AmpCfsUnitTest.pBufferTest[k] != k % AMP_CFS_UT_BYTE_TEST_BASE) {
                            AMP_CFS_UT_PRINT(RED, "Data error %2X!=%2X",g_AmpCfsUnitTest.pBufferTest[k], k % AMP_CFS_UT_BYTE_TEST_BASE);
                            if (AmpUT_CfsClose(pFile) != AMP_OK) {
                               AMP_CFS_UT_PRINT(RED, "close file: Fail");
                            }
                            return AMP_ERROR_GENERAL_ERROR;
                        }
                    }
                } else {
                    AMP_CFS_UT_PRINT(RED, "read %dth byte : Fail", i);
                    if (AmpUT_CfsClose(pFile) != AMP_OK) {
                       AMP_CFS_UT_PRINT(RED, "close file: Fail");
                    }
                    return AMP_ERROR_GENERAL_ERROR;
                }
            }
            AmpUT_CfsSeek(pFile, Pos, 0);
            if (AmpUT_CfsRead(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 307200, pFile) == AMP_OK) {
                Pos += 307200;
                for (int k=0;k<307200;k++) {
                    if (g_AmpCfsUnitTest.pBufferTest[k] != k % AMP_CFS_UT_BYTE_TEST_BASE) {
                        AMP_CFS_UT_PRINT(RED, "Data error %2X!=%2X",g_AmpCfsUnitTest.pBufferTest[k], k % AMP_CFS_UT_BYTE_TEST_BASE);
                        if (AmpUT_CfsClose(pFile) != AMP_OK) {
                           AMP_CFS_UT_PRINT(RED, "close file: Fail");
                        }
                        return AMP_ERROR_GENERAL_ERROR;
                    }
                }
            } else {
                AMP_CFS_UT_PRINT(RED, "read %dth byte : Fail", i);
                if (AmpUT_CfsClose(pFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    } else {
        for (i=0;i<1024;i++) {
            for (j=0;j<10;j++) {
                AmbaFS_fseek(pAmbaFile, Pos, 0);
                if (AmbaFS_fread(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 10240, pAmbaFile) == 10240) {
                    Pos += 10240;
                    for (int k=0;k<10240;k++) {
                        if (g_AmpCfsUnitTest.pBufferTest[k] != k  % AMP_CFS_UT_BYTE_TEST_BASE) {
                            AMP_CFS_UT_PRINT(RED, "Data error %2X!=%2X", g_AmpCfsUnitTest.pBufferTest[k], k % AMP_CFS_UT_BYTE_TEST_BASE);
                            if (AmpUT_CfsClose(pFile) != AMP_OK) {
                               AMP_CFS_UT_PRINT(RED, "close file: Fail");
                            }
                            return AMP_ERROR_GENERAL_ERROR;
                        }
                    }
                } else {
                    AMP_CFS_UT_PRINT(RED, "read %dth byte : Fail", i);
                    if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
                       AMP_CFS_UT_PRINT(RED, "close file: Fail");
                    }
                    return AMP_ERROR_GENERAL_ERROR;
                }
            }
            AmbaFS_fseek(pAmbaFile, Pos, 0);
            if (AmbaFS_fread(g_AmpCfsUnitTest.pBufferTest, sizeof(UINT8), 307200, pAmbaFile) == 307200) {
                Pos += 307200;
                for (int k=0;k<307200;k++) {
                    if (g_AmpCfsUnitTest.pBufferTest[k] != k % AMP_CFS_UT_BYTE_TEST_BASE) {
                        AMP_CFS_UT_PRINT(RED, "Data error %2X!=%2X", g_AmpCfsUnitTest.pBufferTest[k], k % AMP_CFS_UT_BYTE_TEST_BASE);
                        if (AmpUT_CfsClose(pFile) != AMP_OK) {
                           AMP_CFS_UT_PRINT(RED, "close file: Fail");
                        }
                        return AMP_ERROR_GENERAL_ERROR;
                    }
                }
            } else {
                AMP_CFS_UT_PRINT(RED, "read %dth byte : Fail", i);
                if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    }
    AMP_CFS_UT_PRINT(GREEN, "Pass!");
    /** Close File */
    if ((iAsyncMode==0)||(iAsyncMode==1)) {
        if (AmpUT_CfsClose(pFile) != AMP_OK) {
            AMP_CFS_UT_PRINT(RED, "close file: Fail");
        }
    } else {
        if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
            AMP_CFS_UT_PRINT(RED, "close file: Fail");
        }
    }
    AmbaUtility_GetTimeStart(&tEnd);
    AmbaPrint("Elapsed Time = %d", tEnd - tStart);
    return AMP_OK;
}

int Cfs_TestCase_small_write(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMP_CFS_FILE_s *pFile = NULL;
    AMBA_FS_FILE *pAmbaFile;
    char pwcFile1[MAX_FILENAME_LENGTH] = "C:\\SMALL_SYNC.txt";
    char pwcFile2[MAX_FILENAME_LENGTH] = "C:\\SMALL_ASYNC.txt";
    char pwcFile3[MAX_FILENAME_LENGTH] = "C:\\SMALL_AMBA.txt";
    char pwcMode[] = "w+";
    UINT32 tStart, tEnd;
    int    iAsyncMode, i;
    UINT8  uBuf[2];

    /** Get Parameters : Asyncmode */
    iAsyncMode = (env->Argc < 3) ? 0 : atoi(env->Argv[2]);
    /** Initialize CFS */
    if (AmpUT_CfsInit() == AMP_OK) {
        AMP_CFS_UT_PRINT(GREEN, "init: Pass");
    } else {
        AMP_CFS_UT_PRINT(RED, "init: Fail");
        return AMP_ERROR_GENERAL_ERROR;
    }
    /** Open file */
    switch (iAsyncMode) {
        case 0 :
            AmbaPrint("Synchronous mode multiple small write test!");
            pFile = AmpUT_CfsOpen(pwcFile1, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, 0, AMP_CFS_UT_PRIORITY_DEFAULT);
            if (pFile != NULL) {
               AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
            } else {
               AMP_CFS_UT_PRINT(RED, "open file : Fail");
               return AMP_ERROR_GENERAL_ERROR;
            }
            break;
        case 1:
            AmbaPrint("Asynchronous mode multiple small write test!");
            pFile = AmpUT_CfsOpen(pwcFile2, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_ASYNC_MODE, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
            if (pFile != NULL) {
               AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
            } else {
               AMP_CFS_UT_PRINT(RED, "open file : Fail");
               return AMP_ERROR_GENERAL_ERROR;
            }
            break;
        default:
            AmbaPrint("AmbaFS mode multiple small write test!");
            pAmbaFile = AmbaFS_fopen((const char *)pwcFile3, (char *)pwcMode);
            if (pAmbaFile != NULL) {
               AMP_CFS_UT_PRINT(GREEN, "open file : Pass");
            } else {
               AMP_CFS_UT_PRINT(RED, "open file : Fail");
               return AMP_ERROR_GENERAL_ERROR;
            }
    }
    /** Get current time */
    AmbaUtility_GetTimeStart(&tStart);
    /** Seek and Write test */
    AmbaPrint("write test");
    if ((iAsyncMode==0)||(iAsyncMode==1)) {
        for (i=0;i<AMP_CFS_UT_SMALL_WRITE_COUNT;i+=2) {
            uBuf[0]=(UINT8)(i % AMP_CFS_UT_BYTE_TEST_BASE);
            uBuf[1]=(UINT8)((i+1) % AMP_CFS_UT_BYTE_TEST_BASE);
            if (AmpUT_CfsSeek(pFile , i, 0) != AMP_OK) {
                AMP_CFS_UT_PRINT(RED, "seek %dth byte : Fail", i);
                if (AmpUT_CfsClose(pFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
            if (AmpUT_CfsWrite(uBuf, sizeof(UINT8), 2, pFile) != AMP_OK) {
                AMP_CFS_UT_PRINT(RED, "write %dth byte : Fail", i);
                if (AmpUT_CfsClose(pFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    } else {
        for (i=0;i<AMP_CFS_UT_SMALL_WRITE_COUNT;i+=2) {
            uBuf[0]=(UINT8)(i % AMP_CFS_UT_BYTE_TEST_BASE);
            uBuf[1]=(UINT8)((i+1) % AMP_CFS_UT_BYTE_TEST_BASE);
            if (AmbaFS_fseek(pAmbaFile , i, 0) != AMP_OK) {
                AMP_CFS_UT_PRINT(RED, "seek %dth byte : Fail", i);
                if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
            if (AmbaFS_fwrite(uBuf, sizeof(UINT8), 2, pAmbaFile) != 2) {
                AMP_CFS_UT_PRINT(RED, "write %dth byte : Fail", i);
                if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    }
    AMP_CFS_UT_PRINT(GREEN, "Pass!");
    /** Read test */
    AmbaPrint("read test");
    if ((iAsyncMode==0)||(iAsyncMode==1)) {
        AmpUT_CfsSeek(pFile, 0, 0);
        for (i=0;i<AMP_CFS_UT_SMALL_WRITE_COUNT;i++) {
            AmpUT_CfsSeek(pFile, i, 0);
            if ((AmpUT_CfsRead(&uBuf, sizeof(UINT8), 1, pFile) != AMP_OK)||(uBuf[0] != (UINT8)(i % AMP_CFS_UT_BYTE_TEST_BASE))) {
                AMP_CFS_UT_PRINT(RED, "read %dth byte : Fail", i);
                if (AmpUT_CfsClose(pFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    } else {
        AmbaFS_fseek(pAmbaFile, 0, 0);
        for (i=0;i<AMP_CFS_UT_SMALL_WRITE_COUNT;i++) {
            AmbaFS_fseek(pAmbaFile, i, 0);
            if ((AmbaFS_fread(&uBuf, sizeof(UINT8), 1, pAmbaFile) != 1)||(uBuf[0] != (UINT8)(i % AMP_CFS_UT_BYTE_TEST_BASE))) {
                AMP_CFS_UT_PRINT(RED, "read %dth byte : Fail", i);
                if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
                   AMP_CFS_UT_PRINT(RED, "close file: Fail");
                }
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    }
    AMP_CFS_UT_PRINT(GREEN, "Pass!");
    /** Close File */
    if ((iAsyncMode==0)||(iAsyncMode==1)) {
        if (AmpUT_CfsClose(pFile) != AMP_OK) {
            AMP_CFS_UT_PRINT(RED, "close file: Fail");
        }
    } else {
        if (AmbaFS_fclose(pAmbaFile) != AMP_OK) {
            AMP_CFS_UT_PRINT(RED, "close file: Fail");
        }
    }
    AmbaUtility_GetTimeStart(&tEnd);
    AmbaPrint("Elapsed Time = %d", tEnd - tStart);
    return AMP_OK;
}

int Cfs_TestCase_amba_auto(AMP_CFS_UT_SHELL_ENV_s *env)
{
    char pwcFile1[MAX_FILENAME_LENGTH] = "C:\\AMBA_AMBA1.txt";
    char pwcFile2[MAX_FILENAME_LENGTH] = "C:\\AMBA_AMBA2.txt";
    char  cBuf[] = "This is a test.";
    UINT32 tStart, tEnd;
    int   i;

    /** Initialize CFS */
    if (AmpUT_CfsInit() == AMP_OK) {
       AMP_CFS_UT_PRINT(GREEN, "init: Pass");
    } else {
       AMP_CFS_UT_PRINT(RED, "init: Fail");
       return AMP_ERROR_GENERAL_ERROR;
    }
    /** Get current time */
    AmbaUtility_GetTimeStart(&tStart);
    // Write mode test : small data test and big data test
    // functions : fopen, fwrite, FSync, ftell, FgetLen, fseek, fread, fclose
    AmbaPrint("Synchronous write mode test");
    AmbaPrint("writing small data...");
    if (AmpUT_AmbaWriteModeTest(pwcFile1, (UINT8 *)cBuf, sizeof(cBuf)) != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    AmbaPrint("writing big data...");
    for (i=0;i<AMP_CFS_UT_BIG_TEST_BUF_SIZE;i++) {
        g_AmpCfsUnitTest.pBufferTest[i] = i % AMP_CFS_UT_BYTE_TEST_BASE;
    }
    if (AmpUT_AmbaWriteModeTest(pwcFile2, g_AmpCfsUnitTest.pBufferTest, AMP_CFS_UT_BIG_TEST_BUF_SIZE) != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    // Read mode test : small data test and big data test
    // functions : fopen, ftell, FgetLen, fseek, fread, fclose
    AmbaPrint("Synchronous read mode test");
    AmbaPrint("Reading big data...");
    if (AmpUT_AmbaReadModeTest(pwcFile2, g_AmpCfsUnitTest.pBufferTest, AMP_CFS_UT_BIG_TEST_BUF_SIZE) == AMP_OK) {
        for (i=0;i<AMP_CFS_UT_BIG_TEST_BUF_SIZE;i++) {
            if (g_AmpCfsUnitTest.pBufferTest[i] != i % AMP_CFS_UT_BYTE_TEST_BASE) {
                AMP_CFS_UT_PRINT(RED, "read: Fail");
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    } else return AMP_ERROR_GENERAL_ERROR;
    AmbaPrint("Test complete!");
    AmbaUtility_GetTimeStart(&tEnd);
    AmbaPrint("Elapsed Time = %d", tEnd - tStart);
    AmbaKAL_TaskSleep(50);
    return AMP_OK;
}

int Cfs_TestCase_priority(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMP_CFS_UT_PRINT(GREEN, ">>Priority test");
    if (AmpUT_CfsInit() == AMP_OK) {
        AMP_CFS_UT_PRINT(GREEN, "init: Pass!");
    } else {
        AMP_CFS_UT_PRINT(RED, "init: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUT_CfsPriorityTest() == AMP_OK) {
        AMP_CFS_UT_PRINT(GREEN, "Priority test: Pass!");
    } else {
        AMP_CFS_UT_PRINT(RED, "Priority test: Fail!");
        return AMP_ERROR_GENERAL_ERROR;
    }
    return AMP_OK;
}

int Cfs_TestCase_stat_test(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMBA_FS_FILE_INFO_s Stat;
    UINT32 tStart, tEnd;
    int i;
    if (env->Argc == 3) {
        AmbaUtility_GetTimeStart(&tStart);
        for (i=0;i<10000;i++) {
            AmbaFS_GetFileInfo((const char *)env->Argv[2], &Stat);
        }
        AmbaUtility_GetTimeStart(&tEnd);
        AmbaPrint("Elapsed Time: %d", tEnd - tStart);
        return AMP_OK;
    } else AmbaPrint("Usage : t cfs cstat filename");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cstat_test(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMP_CFS_FILE_INFO_s Stat;
    UINT32 tStart, tEnd;
    int i;
    if (env->Argc == 3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            AmbaUtility_GetTimeStart(&tStart);
            for (i=0;i<10000;i++) {
                AmpCFS_GetFileInfo(env->Argv[2], &Stat);
            }
            AmbaUtility_GetTimeStart(&tEnd);
            AmbaPrint("Elapsed Time: %d", tEnd - tStart);
            return AMP_OK;
        }
    } else AmbaPrint("Usage : t cfs cstat filename");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_ls_test(AMP_CFS_UT_SHELL_ENV_s *env)
{
    UINT32 tStart, tEnd;
    AMBA_FS_DTA_t Dta = {0};
    char pwcPattern[MAX_FILENAME_LENGTH]= "C:\\*";

    if (env->Argc == 3) {
        memset(pwcPattern, 0, MAX_FILENAME_LENGTH);
        strncpy(pwcPattern, env->Argv[2], MAX_FILENAME_LENGTH);
    } else AmbaPrint("Usage: t cfs ls_test pattern");
    AmbaUtility_GetTimeStart(&tStart);
    for (int i=0;i<1000;i++) {
        if (AmbaFS_FileSearchFirst(pwcPattern, ATTR_ALL, &Dta) == AMP_OK) {
            while ( AmbaFS_FileSearchNext(&Dta) == AMP_OK);
        }
    }
    AmbaUtility_GetTimeStart(&tEnd);
    AmbaPrint("Elapsed Time: %d", tEnd - tStart);
    return AMP_OK;
}

int Cfs_TestCase_cls_test(AMP_CFS_UT_SHELL_ENV_s *env)
{
    UINT32 tStart, tEnd;
    AMP_CFS_DTA_s Dta = {0};
    char pwcPattern[MAX_FILENAME_LENGTH]= "C:\\*";

    if (env->Argc == 3) {
        memset(pwcPattern, 0, MAX_FILENAME_LENGTH);
        strncpy(pwcPattern, env->Argv[2], MAX_FILENAME_LENGTH);
    } else {
        AmbaPrint("Usage: t cfs cls_test pattern");
    }
    if (AmpUT_CfsInit() == AMP_OK) {
        AmbaUtility_GetTimeStart(&tStart);
        for (int i=0;i<1000;i++) {
            if (AmpCFS_FileSearchFirst(pwcPattern, AMP_CFS_ATTR_ALL, &Dta) == AMP_OK) {
                while ( AmpCFS_FileSearchNext(&Dta) == AMP_OK) ;
            }
        }
        AmbaUtility_GetTimeStart(&tEnd);
        AmbaPrint("Elapsed Time: %d", tEnd - tStart);
        return AMP_OK;
    }
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_print_tree(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc == 3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            const char drive = env->Argv[2][0];
            AmpCFS_CachePrintTree(drive);
            return AMP_OK;
        }
    } else AmbaPrint("Usage : t cfs print_tree drive");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_fstatus(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc == 3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            AmbaPrint("FStatus = %d", AmpUT_CfsFStatus(env->Argv[2]));
            return AMP_OK;
        }
    } else AmbaPrint("Usage : t cfs fstatus filename");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_remove(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc == 3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_Cfsremove(env->Argv[2]) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "remove: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "remove: Fail");
        }
    } else AmbaPrint("Usage : t cfs remove filename");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_move(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc == 4) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsMove(env->Argv[2], env->Argv[3]) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "move: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "move: Fail");
        }
    } else AmbaPrint("Usage : t cfs move srcfile dstfile");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cstat(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMP_CFS_FILE_INFO_s Stat;
    if (env->Argc == 3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsStat(env->Argv[2], &Stat) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "stat: Pass");
#ifdef AMP_CFS_CACHE_VERBOSE
                AmpUT_CfsPrintFileInfo(&Stat);
#endif
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "stat: Fail");
        }
    } else AmbaPrint("Usage : t cfs cstat filename");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_stat(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMBA_FS_FILE_INFO_s Stat;
    UINT32 tStart, tEnd;
    if (env->Argc == 3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            int Rval;
            AmbaUtility_GetTimeStart(&tStart);
            Rval = AmbaFS_GetFileInfo((const char *)env->Argv[2], &Stat);
            AmbaUtility_GetTimeStart(&tEnd);
            AmbaPrint("Elapsed Time: %d", tEnd - tStart);
            if (Rval == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "stat: Pass");
#ifdef AMP_CFS_CACHE_VERBOSE
                AmpUT_CfsPrintFileInfo(&Stat);
#endif
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "stat: Fail");
        }
    } else AmbaPrint("Usage : t cfs stat filename");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_chmod(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc == 4) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsChmod(env->Argv[2], atoi(env->Argv[3])) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "chmod: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "chmod: Fail");
        }
    } else AmbaPrint("Usage : t cfs chmod file attribute(bit 5~0 :ADVSHR");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_combine(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc==4) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsCombine(env->Argv[2], env->Argv[3]) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "combine: Pass");
                return AMP_OK;
            } else {
                AMP_CFS_UT_PRINT(RED, "combine: Fail");
            }
        }
    } else AmbaPrint("Usage : t cfs combine base_file add_file");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_divide(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc == 5) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsDivide(env->Argv[2], env->Argv[3], atoi(env->Argv[4])) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "divide: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "divide: Fail");
        }
    } else AmbaPrint("Usage : t cfs divide original_file new_file offset");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cinsert(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc == 5) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsCinsert(env->Argv[2], atoi(env->Argv[3]), atoi(env->Argv[4])) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "cinsert: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "cinsert: Fail");
        }
    } else AmbaPrint("Usage : t cfs cinsert filename offset number");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cdelete(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc == 5) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsCdelete(env->Argv[2], atoi(env->Argv[3]), atoi(env->Argv[4])) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "cdelete: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "cdelete: Fail");
        }
    } else AmbaPrint("Usage : t cfs cdelete filename offset number");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_mkdir(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc == 3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsMkdir(env->Argv[2]) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "mkdir: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "mkdir: Fail");
        }
    } else AmbaPrint("Usage : t cfs mkdir dirname");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_rmdir(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc == 3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsRmdir(env->Argv[2]) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "rmdir: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "rmdir: Fail");
        }
    } else AmbaPrint("Usage : t cfs rmdir dirname");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cgetdev(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMP_CFS_DRIVE_INFO_s DevInf;
    if (env->Argc==3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsGetDev(env->Argv[2][0], &DevInf) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "getdev: Pass");
                AmpUT_CfsPrintDriveInfoCfs(&DevInf);
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "getdev: Fail");
        }
    } else AmbaPrint("Usage : t cfs cgetdev drive");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_getdev(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMBA_FS_DRIVE_INFO_t DevInf;
    if (env->Argc==3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmbaFS_GetDriveInfo(env->Argv[2][0], &DevInf) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "getdev: Pass");
                AmpUT_CfsPrintDriveInfoAmbaFs(&DevInf);
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "getdev: Fail");
        }
    } else AmbaPrint("Usage : t cfs getdev drive");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_format(AMP_CFS_UT_SHELL_ENV_s *env)
{
    char pcParam[2] = {0};
    if (env->Argc==3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsFormat(env->Argv[2][0], pcParam) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "format: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "format: Fail");
        }
    } else AmbaPrint("Usage : t cfs format drive");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_sync(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc==3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpUT_CfsSync(env->Argv[2][0], AMP_CFS_SYNC_NINVALIDATE) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "sync: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "sync: Fail");
        }
    } else AmbaPrint("Usage : t cfs sync drive");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_init(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (AmpUT_CfsInit() == AMP_OK) {
        AMP_CFS_UT_PRINT(GREEN, "init: Pass");
        return AMP_OK;
    } else {
        AMP_CFS_UT_PRINT(RED, "init: Fail");
    }
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_clear_cache(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (env->Argc==3) {
        if (AmpUT_CfsInit() == AMP_OK) {
            if (AmpCFS_ClearCache(env->Argv[2][0]) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "clear cache: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "clear cache: Fail");
        }
    } else AmbaPrint("Usage : t cfs clear_cache drive");
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cached_data(AMP_CFS_UT_SHELL_ENV_s *env)
{
    UINT64 ulSize;
    if (AmpUT_CfsInit() == AMP_OK) {
        AmpCFS_GetCachedDataSize('C', &ulSize);
        AmbaPrint("Size = %u", (UINT32) ulSize);
        return AMP_OK;
    }
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_mount_test(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Ret = AMP_OK;
    AMP_CFS_FILE_s *File;
    char Drive = 'C';
    char FileName1[MAX_FILENAME_LENGTH] = "UT_CFS_MountTest.txt";
    char Path[MAX_FILENAME_LENGTH] = {0};

    if (env->Argc > 2) {
        Drive = env->Argv[2][0];
    }

    sprintf(Path, "%c:\\%s", Drive, FileName1);
    LOG_MSG("Open file: %s", Path);
    if ((File = AmpUT_CfsOpen(Path, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, 0, AMP_CFS_UT_PRIORITY_DEFAULT)) == NULL) {
        LOG_FAIL("Failed to open the file %s", Path);
        return AMP_ERROR_GENERAL_ERROR;
    }
    LOG_MSG("Unmount drive %c", Drive);
    if (AmpCFS_UnMount(Drive) != AMP_OK) {
        LOG_MSG("Forced to unmount the drive due to the unclosed file.");
        LOG_MSG("Close file: %s", Path);
        AmpUT_CfsClose(File);
    } else {
        LOG_FAIL("Unmount the drive done.");
        return AMP_ERROR_GENERAL_ERROR;
    }
    LOG_MSG("Mount drive %c", Drive);
    Ret += AmpCFS_Mount(Drive);
    return Ret;
}

#define AMP_CFS_UT_MOUNT_DRIVE_SYM  C
#define AMP_CFS_UT_MOUNT_TO_DRIVE_U(sym) AMP_CFS_DRIVE_##sym
#define AMP_CFS_UT_MOUNT_TO_DRIVE(sym) AMP_CFS_UT_MOUNT_TO_DRIVE_U(sym)
#define AMP_CFS_UT_MOUNT_FILENAME(suffix, id)  AMP_CFS_UT_TOSTR(AMP_CFS_UT_MOUNT_DRIVE_SYM) ":\\UT_CFS_UnmountTest_" AMP_CFS_UT_TOSTR(suffix) AMP_CFS_UT_STRINGIFY(id)
#define AMP_CFS_UT_MOUNT_DRIVE_DEF  AMP_CFS_UT_MOUNT_TO_DRIVE(AMP_CFS_UT_MOUNT_DRIVE_SYM)

static int Cfs_TestCase_UnmountTest_open_close(int isContinue)
{
    int Ret = AMP_OK;
    AMP_CFS_FILE_s *File1, *File2, *File3;
    char Drive = AMP_CFS_UT_MOUNT_DRIVE_DEF;
    char FName1[MAX_FILENAME_LENGTH] = AMP_CFS_UT_MOUNT_FILENAME(oc_, 1);
    char FName2[MAX_FILENAME_LENGTH] = AMP_CFS_UT_MOUNT_FILENAME(oc_, 2);

    if (isContinue != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    // Open File1 & File2
    if ((File1 = AmpUT_CfsOpen(FName1, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_SYNC_MODE, 0, AMP_CFS_UT_PRIORITY_DEFAULT)) == NULL) {
        LOG_FAIL("Failed to open the file %s", FName1);
        Ret += AMP_ERROR_GENERAL_ERROR;
    } else {
        LOG_MSG("Open file in sync mode: %s, Handler: %p", File1->Filename, File1);
    }
    if ((File2 = AmpUT_CfsOpen(FName2, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_ASYNC_MODE, 2, AMP_CFS_UT_PRIORITY_DEFAULT)) == NULL) {
        LOG_FAIL("Failed to open the file %s", FName2);
        Ret += AMP_ERROR_GENERAL_ERROR;
    } else {
        LOG_MSG("Open file in async mode: %s, Handler: %p", File2->Filename, File2);
    }
    // AmbaFS will force File1 & File2 to be closed after invoking AmbaFs_UnMount.
    // File1 & File2 are invalid in CFS.
    LOG_MSG("Unmount drive %c", Drive);
    AmpCFS_UnMount(Drive);
    // Try to reopen File1 after unmount the drive
    if ((File3 = AmpUT_CfsOpen(FName1, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_ASYNC_MODE, 2, AMP_CFS_UT_PRIORITY_DEFAULT)) != NULL) {
        LOG_FAIL("Reopen file in async mode: %s, Handler: %p", File3->Filename, File3);
        Ret += AMP_ERROR_GENERAL_ERROR;
        AmpUT_CfsClose(File3);
    }
    LOG_MSG("Mount drive %c", Drive);
    Ret += AmpCFS_Mount(Drive);
    // Try to reopen File1 after mount the drive
    if ((File3 = AmpUT_CfsOpen(FName1, AMP_CFS_FILE_MODE_READ_WRITE, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_ASYNC_MODE, 2, AMP_CFS_UT_PRIORITY_DEFAULT)) == NULL) {
        LOG_FAIL("Failed to open the file %s", FName1);
        Ret += AMP_ERROR_GENERAL_ERROR;
    } else {
        LOG_MSG("Reopen file in async mode: %s, Handler: %p", File3->Filename, File3);
        LOG_MSG("Close file: %s, Handler: %p", File3->Filename, File3);
        Ret += AmpUT_CfsClose(File3);
    }
    if (File1 != NULL) {
        LOG_MSG("Close file: %s, Handler: %p", File1->Filename, File1);
        AmpUT_CfsClose(File1);
    }
    if (File2 != NULL) {
        LOG_MSG("Close file: %s, Handler: %p", File2->Filename, File2);
        AmpUT_CfsClose(File2);
    }
    return Ret;
}

static int Cfs_TestCase_UnmountTest_reopen_write(int isContinue, UINT32 fileSize, int isAsync)
{
    int Ret = AMP_OK;
    AMP_CFS_FILE_s *File1, *File2;
    char Drive = AMP_CFS_UT_MOUNT_DRIVE_DEF;
    char FName[MAX_FILENAME_LENGTH] = AMP_CFS_UT_MOUNT_FILENAME(re_, 1);

    if (isContinue != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    // Open file
    if ((File1 = AmpUT_CfsOpen(FName, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, isAsync, IIF(isAsync, 2, 0), AMP_CFS_UT_PRIORITY_DEFAULT)) == NULL) {
        LOG_FAIL("Failed to open the file %s", FName);
        Ret += AMP_ERROR_GENERAL_ERROR;
    } else {
        LOG_MSG("Open file in %s mode: %s, Handler: %p", IIF(isAsync, "async", "sync"), File1->Filename, File1);
        LOG_MSG("Write %u Mib data, Handler %p", fileSize, File1);
        Ret += Stress_WriteFile(File1, fileSize);
        // Invalidate File1
        LOG_MSG("Unmount drive %c", Drive);
        AmpCFS_UnMount(Drive);
        LOG_MSG("Mount drive %c", Drive);
        Ret += AmpCFS_Mount(Drive);
        // Reopen the file
        if ((File2 = AmpUT_CfsOpen(FName, AMP_CFS_FILE_MODE_READ_WRITE, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, isAsync, IIF(isAsync, 2, 0), AMP_CFS_UT_PRIORITY_DEFAULT)) == NULL) {
            LOG_FAIL("Failed to open the file %s", FName);
            Ret += AMP_ERROR_GENERAL_ERROR;
        } else {
            LOG_MSG("Reopen file in %s mode: %s, Handler: %p", IIF(isAsync, "async", "sync"), File2->Filename, File2);
            LOG_MSG("Write %u Mib data, Handler %p", fileSize, File2);
            Ret += Stress_WriteFile(File2, fileSize);
            LOG_MSG("Read %u Mib data, Handler %p", fileSize, File2);
            Ret += Stress_ReadFile(File2, fileSize);
            // Close File2
            LOG_MSG("Close file: %s, Handler: %p", File2->Filename, File2);
            Ret += AmpUT_CfsClose(File2);
        }
        // Close File1
        LOG_MSG("Close file: %s, Handler: %p", File1->Filename, File1);
        AmpUT_CfsClose(File1);
    }
    if (isAsync) {
        Stress_WaitFileClose(FName, FALSE);
    }
    return Ret;
}

static int Cfs_TestCase_UnmountTest_io_interrupt(int isContinue, int isAsync)
{
    int Ret = AMP_OK;
    AMP_CFS_FILE_s *File1;
    UINT8 *WBuf;
    char Drive = AMP_CFS_UT_MOUNT_DRIVE_DEF;
    char FName[MAX_FILENAME_LENGTH] = AMP_CFS_UT_MOUNT_FILENAME(io_, 1);

    if (isContinue != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    WBuf = Stress_GetWriteBuffer();
    Stress_PrepareWriteBufferData(WBuf, AMP_CFS_UT_ST_WBUF_SIZE * AMP_CFS_UT_ST_WBUF_COUNT);
    // Open file
    if ((File1 = AmpUT_CfsOpen(FName, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, isAsync, IIF(isAsync, 2, 0), AMP_CFS_UT_PRIORITY_DEFAULT)) == NULL) {
        LOG_FAIL("Failed to open the file %s", FName);
        Ret += AMP_ERROR_GENERAL_ERROR;
    } else {
        LOG_MSG("Open file in %s mode: %s, Handler: %p", IIF(isAsync, "async", "sync"), File1->Filename, File1);
        LOG_MSG("Write %u Mib data, Handler %p", AMP_CFS_UT_ST_WBUF_COUNT, File1);
        AmpCFS_fwrite(WBuf, AMP_CFS_UT_ST_WBUF_SIZE, AMP_CFS_UT_ST_WBUF_COUNT, File1);
        // Invalidate the file
        LOG_MSG("Unmount drive %c", Drive);
        AmpCFS_UnMount(Drive);
        // Close File1
        LOG_MSG("Close file: %s, Handler: %p", File1->Filename, File1);
        AmpUT_CfsClose(File1);
    }
    if (isAsync) {
        Stress_WaitFileClose(FName, FALSE);
    }
    LOG_MSG("Mount drive %c", Drive);
    Ret += AmpCFS_Mount(Drive);
    return Ret;
}

int Cfs_TestCase_unmount_test(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Ret = AMP_OK;
    Ret += Cfs_TestCase_UnmountTest_open_close(Ret);
    Ret += Cfs_TestCase_UnmountTest_reopen_write(Ret, 2, AMP_CFS_FILE_SYNC_MODE);
    Ret += Cfs_TestCase_UnmountTest_reopen_write(Ret, 2, AMP_CFS_FILE_ASYNC_MODE);
    Ret += Cfs_TestCase_UnmountTest_io_interrupt(Ret, AMP_CFS_FILE_SYNC_MODE);
    Ret += Cfs_TestCase_UnmountTest_io_interrupt(Ret, AMP_CFS_FILE_ASYNC_MODE);
    return Ret;
}

int Cfs_TestCase_mount(AMP_CFS_UT_SHELL_ENV_s *env)
{
    char Drive = AMP_CFS_DRIVE_C;
    if (env->Argc > 2) {
        Drive = toupper((int)env->Argv[2][0]);
    }
    LOG_MSG("Mount drive %c", Drive);
    return AmpCFS_Mount(Drive);
}

int Cfs_TestCase_unmount(AMP_CFS_UT_SHELL_ENV_s *env)
{
    char Drive = AMP_CFS_DRIVE_C;
    if (env->Argc > 2) {
        Drive = toupper((int)env->Argv[2][0]);
    }
    LOG_MSG("Unmount drive %c", Drive);
    return AmpCFS_UnMount(Drive);
}

int Cfs_TestCase_bank_size(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMP_CFS_FILE_s *File;
    UINT64 DataSize = 0;
    UINT32 BankSize;
    UINT8 BankNum = 3;
    char FileName[MAX_FILENAME_LENGTH] = "C:\\UT_CFS_BankSize.txt";

    LOG_MSG("Open file %s with %u banks.", FileName, BankNum);
    if ((File = AmpUT_CfsOpen(FileName, AMP_CFS_FILE_MODE_WRITE_READ, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, AMP_CFS_FILE_ASYNC_MODE, BankNum, AMP_CFS_UT_PRIORITY_DEFAULT)) == NULL) {
        LOG_FAIL("Failed to open the file %s", FileName);
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpCFS_GetFileMaxCachedDataSize(File, &DataSize) != AMP_OK) {
        LOG_FAIL("Failed to get max banked data size");
        return AMP_ERROR_GENERAL_ERROR;
    }
    BankSize = DataSize / BankNum;
    LOG_MSG("DataSize:%llu, BankNum:%hhu, BankSize:%u", DataSize, BankNum, BankSize);
    LOG_MSG("Close file: %s", File->Filename);
    if (AmpUT_CfsClose(File) != AMP_OK) {
        LOG_FAIL("Failed to close the file %s", File->Filename);
        return AMP_ERROR_GENERAL_ERROR;
    }
    return AMP_OK;
}

int Cfs_TestCase_create_test_files(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (AmpUT_CfsInit() == AMP_OK) {
        if (AmpUT_CacheCreateTestFiles() == TRUE) {
            return AMP_OK;
        }
    }
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_remove_test_files(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (AmpUT_CfsInit() == AMP_OK) {
        if (AmpUT_CacheRemoveTestFiles() == TRUE) {
            return AMP_OK;
        }
    }
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cls_miss_test(AMP_CFS_UT_SHELL_ENV_s *env)
{
    char pwcPattern[MAX_FILENAME_LENGTH]= "C:\\*";
    if (env->Argc == 3) {
        memset(pwcPattern, 0, MAX_FILENAME_LENGTH);
        strncpy(pwcPattern, env->Argv[2], MAX_FILENAME_LENGTH);
    } else AmbaPrint("Usage: t cfs cls_miss_test pattern");
    if (AmpUT_CfsInit() == AMP_OK) {
        UINT32 tStart, tEnd, tClear;
        int i;
        AMP_CFS_DTA_s Dta = {0};

        AmbaUtility_GetTimeStart(&tStart);
        for (i=0;i<1000;i++) {
            AmpCFS_ClearCache('C');
        }
        tClear = AmbaUtility_GetTimeEnd(&tStart, &tEnd);
        AmbaPrint("tClear: %d", tClear);
        AmbaUtility_GetTimeStart(&tStart);
        for (i=0;i<1000;i++) {
            AmpCFS_ClearCache('C');
            if (AmpCFS_FileSearchFirst(pwcPattern, AMP_CFS_ATTR_ALL, &Dta) == AMP_OK) {
                while ( AmpCFS_FileSearchNext(&Dta) == AMP_OK);
            }
        }
        AmbaUtility_GetTimeStart(&tEnd);
        AmbaPrint("Elapsed Time: %d", tEnd - tStart - tClear);
        return AMP_OK;
    }
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cache_test_devinf(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (AmpUT_CfsInit() == AMP_OK) {
        AmbaPrint("==== Cached GetDev Test ====");
        if (AmpUT_CacheTestDevInf('C') == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "cache_test_devinf: Pass");
            return AMP_OK;
        } else AMP_CFS_UT_PRINT(RED, "cache_test_devinf: Fail");
    }
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cache_test_stat(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (AmpUT_CfsInit() == AMP_OK) {
        AmbaPrint("==== Cached Stat Test ====");
        if (AmpUT_CacheTestStat('C') == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "cache_test_stat: Pass");
            return AMP_OK;
        } else AMP_CFS_UT_PRINT(RED, "cache_test_stat: Fail");
    }
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_cache_test_dirent(AMP_CFS_UT_SHELL_ENV_s *env)
{
    if (AmpUT_CfsInit() == AMP_OK) {
        AmbaPrint("==== Cached DirEnt Test ====");
        if (AmpUT_CacheTestDirEnt('C') == AMP_OK) {
            AMP_CFS_UT_PRINT(GREEN, "cache_test_dirent: Pass");
            return AMP_OK;
        } else AMP_CFS_UT_PRINT(RED, "cache_test_dirent: Fail");
    }
    return AMP_ERROR_GENERAL_ERROR;
}

int Cfs_TestCase_gen_file(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AmpUT_CfsGenFile();
    return AMP_OK;
}

int Cfs_TestCase_logm_dump(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Ret = AMP_OK;
#ifdef CONFIG_MW_CFS_LOGM_ENABLE
    Ret = AmpCFS_LogDumpEntryList();
#else
    LOG_MSG("CFS log module is not enabled.");
#endif
    return Ret;
}

#define AMP_CFS_UT_HASH_SDBM_MAX_TKN_LEN    32

int Cfs_TestCase_hash_sdbm(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Ret = AMP_ERROR_GENERAL_ERROR;
    char Token[AMP_CFS_UT_HASH_SDBM_MAX_TKN_LEN + 1] = {0};

    if (env->Argc > 2) {
        strncpy(Token, env->Argv[2], AMP_CFS_UT_HASH_SDBM_MAX_TKN_LEN);
        LOG_MSG("Token: %s, Hash: %u", Token, Cfs_Hash_Sdbm(Token));
        Ret = AMP_OK;
    }
    return Ret;
}

static int Cfs_TestCase_other(AMP_CFS_UT_SHELL_ENV_s *env)
{
    static AMP_CFS_FILE_s *pTestFile = NULL;
    int argc = env->Argc;
    char **argv = env->Argv;

    if (strcmp(argv[1], "open") == 0) {
        UINT  uMode;
        int   iAsyncMode;
        if (argc == 5) {
            if (pTestFile == NULL) {
                if (AmpUT_CfsInit() == AMP_OK) {
                    uMode = atoi(argv[3]);
                    iAsyncMode = atoi(argv[4]);
                    pTestFile = AmpUT_CfsOpen(argv[2], uMode, AMP_CFS_UT_ALIGNMENT_DEFAULT, AMP_CFS_UT_BYTES_TO_SYNC_DEFAULT, iAsyncMode, AMP_CFS_UT_MAX_BANK_AMOUNT, AMP_CFS_UT_PRIORITY_DEFAULT);
                    if (pTestFile != NULL) {
                        AMP_CFS_UT_PRINT(GREEN, "open: Pass");
                        return AMP_OK;
                    } else AMP_CFS_UT_PRINT(RED, "open: Fail");
                }
            } else AmbaPrint("File is opened!");
        } else AmbaPrint("Usage : t cfs open filename mode async_mode");

    } else if (strcmp(argv[1], "write") == 0) {
        char cBuf[] = "That is a test.";
        if (pTestFile != NULL) {
            if (AmpUT_CfsWrite(cBuf, sizeof(char), sizeof(cBuf), pTestFile) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "write: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "write: Fail");
        } else AmbaPrint("File not open!");

    } else if (strcmp(argv[1], "read") == 0) {
        int  iCount;
        char cBuf[100] = {0};
        if (pTestFile != NULL) {
            if (argc == 3) {
                iCount = atoi(argv[2]);
                if (iCount>100) iCount = 100;
                if (AmpUT_CfsRead(cBuf, sizeof(char), iCount, pTestFile) == AMP_OK) {
                    AMP_CFS_UT_PRINT(GREEN, "read: Pass");
                    AmbaPrint("str = %s", cBuf);
                    return AMP_OK;
                } else AMP_CFS_UT_PRINT(RED, "read: Fail");
            } else AmbaPrint("Usage : t cfs read count");
        } else AmbaPrint("File not open!");

    } else if (strcmp(argv[1], "seek") == 0) {
        int iOffset, iOrigin;
        if (pTestFile != NULL) {
            if (argc == 4) {
                iOffset = atoi(argv[2]);
                iOrigin = atoi(argv[3]);
                if (AmpUT_CfsSeek(pTestFile, iOffset, iOrigin) == AMP_OK) {
                    AMP_CFS_UT_PRINT(GREEN, "seek: Pass");
                    return AMP_OK;
                } else AMP_CFS_UT_PRINT(RED, "seek: Fail");
            } else AmbaPrint("Usage : t cfs seek offset origin(0:Start, 1:Cur, 2:End)");
        } else AmbaPrint("File not open!");

    } else if (strcmp(argv[1], "ftell") == 0) {
        INT64 lPos;
        if (pTestFile != NULL) {
            lPos = AmpUT_Cfsftell(pTestFile);
            if (lPos >= 0) {
                AmbaPrint("Pos = %lld", lPos);
                AMP_CFS_UT_PRINT(GREEN, "ftell: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "ftell: Fail");
        } else AmbaPrint("File not open!");

    } else if (strcmp(argv[1], "flen") == 0) {
        INT64 lLen;
        if (pTestFile != NULL) {
            lLen = AmpUT_CfsFGetLen(pTestFile);
            if (lLen >= 0) {
                AmbaPrint("len=%lld", lLen);
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "flen: Fail");
        } else AmbaPrint("File not open!");

    } else if (strcmp(argv[1], "fsync") == 0) {
        if (pTestFile != NULL) {
            if (AmpUT_CfsFSync(pTestFile) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "fsync: Pass");
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "fsync: Fail");
        } else AmbaPrint("File not open!");

    } else if (strcmp(argv[1], "close") == 0) {
        if (pTestFile != NULL) {
            if (AmpUT_CfsClose(pTestFile) == AMP_OK) {
                AMP_CFS_UT_PRINT(GREEN, "close: Pass");
                pTestFile = NULL;
                return AMP_OK;
            } else AMP_CFS_UT_PRINT(RED, "close: Fail");
            pTestFile = NULL;
        } else AmbaPrint("File not open!");

    } else {
        AmpUT_CfsUsage(env);
    }
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * CFS flow controlling functions
 */

static int AmpUT_CfsLaunchStressTest(AMP_CFS_UT_SHELL_ENV_s *env)
{
    AMP_CFS_UT_ST_CMD_PARAMS_s *CmdParams = NULL;
    int Res = AMP_ERROR_GENERAL_ERROR;

    if (Stress_ParseCmdParams(env->Argc, &(env->Argv[0]), &CmdParams) == AMP_OK) {
        AMP_CFS_UT_ST_TEST_RUNNER_s Runner;

        if (CmdParams != NULL && CmdParams->IsStressTest == TRUE) {
            Stress_TCRunner_GetDefault(&Runner);
            Runner.Init(CmdParams);
            Res = Runner.Run(CmdParams);
            LOG_FIN_RESULT_FMT(Res, "CFS %s", env->Argv[1]);
            Res = AMP_OK;
        }
    }
    return Res;
}

static AMP_CFS_UT_TEST_CMD_HDLR_f AmpUT_CfsGetTestCmdHdlr(char *cmdName)
{
    int i;
    AMP_CFS_UT_TEST_CMD_LIST_s *CmdList = g_AmpCfsUnitTest.CfsTestCmdList;
    AMP_CFS_UT_TEST_CMD_s *Cmd;

    while(CmdList != NULL) {
        for (i = 0; i < CmdList->Count; i++) {
            Cmd = CmdList->Items + i;
            if (!TEST_CMD_IS_EMPTY(Cmd) && strcmp(Cmd->Name, cmdName) == 0) {
                return Cmd->Hdlr;
            }
        }
        CmdList = CmdList->NextList;
    }
    return Cfs_TestCase_other;
}

static int AmpUT_CfsLaunchUnitTest(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Ret = AMP_ERROR_GENERAL_ERROR;
    AMP_CFS_UT_TEST_CMD_HDLR_f CmdHdlr = AmpUT_CfsGetTestCmdHdlr(env->Argv[1]);
    if (CmdHdlr != NULL) {
        Ret = CmdHdlr(env);
    }
    LOG_FIN_RESULT_FMT(Ret, "CFS %s", env->Argv[1]);
    return Ret;
}

static int AmpUT_CfsTestEntryImpl(AMP_CFS_UT_SHELL_ENV_s *env)
{
    int Res = AmpUT_CfsLaunchStressTest(env);
    if (Res != AMP_OK) {
        Res = AmpUT_CfsLaunchUnitTest(env);
    }
    return Res;
}

static int AmpCFS_RegisterTestCmd(void)
{
    int Ret = AMP_OK;
    switch (AMP_CFS_UT_TEST_CMD_SET_DEFAULT) {
    case AMP_CFS_UT_TEST_CMD_SET_UG:
        TEST_CMD_ADD(Ret, sync_auto);
        TEST_CMD_ADD(Ret, async_auto);
        break;
    case AMP_CFS_UT_TEST_CMD_SET_ALL:
        Ret += Cfs_TestCmdSet_RegisterALL();
        break;
    default:
        Ret = AMP_ERROR_GENERAL_ERROR;
    }
    return Ret;
}

static int AmpUT_CfsInit(void)
{
    int Ret = AMP_OK;
    if (g_AmpCfsUnitTest.bInit) {
       return Ret;
    }
    ALLOC_MEM(Ret, &g_AmpCfsUnitTest.pBufferTest, AMP_CFS_UT_BIG_TEST_BUF_SIZE);
    ALLOC_MEM(Ret, &g_AmpCfsUnitTest.STWriteBuffer, AMP_CFS_UT_ST_WBUF_SIZE * AMP_CFS_UT_ST_WBUF_COUNT);
    ALLOC_MEM(Ret, &g_AmpCfsUnitTest.STReadBuffer, AMP_CFS_UT_ST_RBUF_SIZE * AMP_CFS_UT_ST_RBUF_COUNT);
    ALLOC_MEM(Ret, &g_AmpCfsUnitTest.STCmdParamsAligned, sizeof(AMP_CFS_UT_ST_CMD_PARAMS_s));
    ALLOC_MEM(Ret, &g_AmpCfsUnitTest.TaskInfo, sizeof(AMP_CFS_UT_ST_TASK_INFO_s));
    ALLOC_MEM(Ret, &g_AmpCfsUnitTest.CfsDta, AMP_CFS_UT_DTA_NUM * sizeof(AMP_CFS_DTA_s));
    ALLOC_MEM(Ret, &g_AmpCfsUnitTest.AmbaFsDta, AMP_CFS_UT_AMBAFS_DTA_NUM * sizeof(AMBA_FS_DTA_t));
    ALLOC_MEM(Ret, &g_AmpCfsUnitTest.CurrentShellEnv, sizeof(AMP_CFS_UT_SHELL_ENV_s));
    Ret = Cfs_TestCmdList_Create(&g_AmpCfsUnitTest.CfsTestCmdList);

    K_ASSERT(Ret == AMP_OK); // Do not release the allocated memory from the pool.
    Ret = AmpCFS_RegisterTestCmd();
    g_AmpCfsUnitTest.ClusterSize = AmpUT_CfsGetDriveClusterSize('C');
    K_ASSERT(g_AmpCfsUnitTest.ClusterSize > 0);
    g_AmpCfsUnitTest.bInit = (Ret == AMP_OK) ? TRUE : FALSE;
    return Ret;
}

static int AmpUT_CfsTestEntry(AMBA_SHELL_ENV_s *env, int argc, char **argv)
{
    int Res;
    RegisterEvtHandler(AmpUT_CfsEventHandlers);
    if ((Res = AmpUT_CfsInit()) == AMP_OK) {
        AMP_CFS_UT_SHELL_ENV_s *Env = g_AmpCfsUnitTest.CurrentShellEnv;
        Env->Argc = argc;
        Env->Argv = argv;
        Res = AmpUT_CfsTestEntryImpl(Env);
    }
    UnregisterEventHandler(AmpUT_CfsEventHandlers);
    return Res;
}

/**
 * CFS UT - add CFS test function.
 */
int AmpUT_CfsTestAdd(void)
{
    AmbaTest_RegisterCommand("cfs", AmpUT_CfsTestEntry);
    return AMP_OK;
}

#undef AMP_CFS_UT_ST_MULTIOPEN_FILE_AMOUNT

#undef LOG_PASS
#undef LOG_FAIL
#undef LOG_MSG
#undef LOG_MARK
#undef LOG_RESULT
#undef LOG_RESULT_FMT
#undef LOG_FIN_RESULT_FMT

#undef LOG_VERB
#undef LOG_V_PASS
#undef LOG_V_FAIL
#undef LOG_V_MSG
#undef LOG_V_RESULT
