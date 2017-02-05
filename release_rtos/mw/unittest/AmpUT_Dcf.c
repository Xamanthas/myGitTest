/**
 * @file AmpUT_Dcf.c
 *
 * History:
 *    2013/07/11 - [Irene Wang] created file
 *    2013/10/02 - [Chen-Lung Chan] refined
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "AmpUnitTest.h"
#include <dcf/AmpDcf.h>
#include <util.h>
#include <AmbaCache_Def.h>

/**
 * Log Macro
 */

#define AMP_DCF_UT_LOG_COL_P    GREEN
#define AMP_DCF_UT_LOG_COL_F    RED
#define AMP_DCF_UT_LOG_COL_M    BLACK

#define AMP_DCF_UT_LOG_PMT_P    "SUCCESS"
#define AMP_DCF_UT_LOG_PMT_F    "FAIL"
#define AMP_DCF_UT_LOG_PMT_M    "MSG"

#define AMP_DCF_UT_PRINT(color, fmt, ...) do {\
    AmbaPrintColor(color, fmt" ", ##__VA_ARGS__);\
    AmbaKAL_TaskSleep(10);\
} while(0)
#define AMP_DCF_UT_LOG_RESULT(lType, fmt, ...)    AMP_DCF_UT_PRINT(AMP_DCF_UT_LOG_COL_##lType, "[" AMP_DCF_UT_LOG_PMT_##lType "]" fmt, ##__VA_ARGS__)
#define LOG_PRT             AmbaPrint
#define LOG_MARK(fmt, ...)  AMP_DCF_UT_PRINT(MAGENTA, fmt, ##__VA_ARGS__)
#define LOG_PASS(fmt, ...)  AMP_DCF_UT_LOG_RESULT(P, " "fmt, ##__VA_ARGS__)
#define LOG_FAIL(fmt, ...)  AMP_DCF_UT_LOG_RESULT(F, " "fmt, ##__VA_ARGS__)
#define LOG_MSG(fmt, ...)   AMP_DCF_UT_LOG_RESULT(M, " "fmt, ##__VA_ARGS__)
#define LOG_RESULT(res, fmt, ...) do { if ((res) == AMP_OK) LOG_PASS(fmt, ##__VA_ARGS__); else LOG_FAIL(fmt, ##__VA_ARGS__); } while(0)

/**
 * Module fields & struct
 */

static AMP_DCF_HDLR_s *g_pDcfHdlr = NULL; /**< DCF handler */
static UINT32 g_ObjId = 0; /**< obj id */
static UINT32 g_Dnum = 0; /**< dir number */

typedef int (*AMP_DCF_UT_TEST_CMD_HDLR_f)(int argc, char **argv);

typedef struct AMP_DCF_UT_TEST_CMD_s_ {
    AMP_DCF_UT_TEST_CMD_HDLR_f  Hdlr;
    char  *Name;
    UINT32 HashCode;
} AMP_DCF_UT_TEST_CMD_s;

typedef struct AMP_DCF_UT_LIST_ITEM_s_ AMP_DCF_UT_LIST_ITEM_s;
struct AMP_DCF_UT_LIST_ITEM_s_ {
    void *Itme;
    AMP_DCF_UT_LIST_ITEM_s *Next;
};

typedef struct AMP_DCF_UT_LIST_s_ {
    AMP_DCF_UT_LIST_ITEM_s *Head;
    AMP_DCF_UT_LIST_ITEM_s *Tail;
    UINT32 Count;
} AMP_DCF_UT_LIST_s;

typedef struct AMP_DCF_UT_CONTEXT_s_ {
    BOOL8 IsInit;
    AMP_DCF_UT_LIST_s *TestCmdList;
} AMP_DCF_UT_CONTEXT_s;

static AMP_DCF_UT_CONTEXT_s g_AmpDcfUtConext = {0};

/**
 * Utility Macro
 */

#define AMP_DCF_UT_PATH_ROOT_DCIM   "c:\\DCIM"
#define AMP_DCF_UT_PATH_DIR(dir)    AMP_DCF_UT_PATH_ROOT_DCIM "\\" #dir
#define AMP_DCF_UT_PATH(dir, file)  AMP_DCF_UT_PATH_DIR(dir) "\\" #file

#define AMP_DCF_UT_TEST_CMD_FUNC(x)     DCF_TestCase_##x
#define AMP_DCF_UT_TEST_CMD_ENTRY(x)    {AMP_DCF_UT_TEST_CMD_FUNC(x), #x, AmpUT_DcfSDBMHash(#x)}
#define AMP_DCF_UT_TEST_CMD_DECL(x)     int DCF_TestCase_##x(int argc, char **argv) __attribute__((weak))

#define AMP_DCF_UT_TEST_CMD_ADD(rt, cmd) do {    \
    AMP_DCF_UT_TEST_CMD_DECL(cmd);  \
    if ((rt) == AMP_OK && AMP_DCF_UT_TEST_CMD_FUNC(cmd)) {   \
        DCF_TestCmdList_AddItem(g_AmpDcfUtConext.TestCmdList, &((AMP_DCF_UT_TEST_CMD_s)AMP_DCF_UT_TEST_CMD_ENTRY(cmd)));  \
    }   \
} while(0)

#define AMP_DCF_UT_ALLOC_MEM(rt, addr, size) do {   \
    if ((rt) == AMP_OK) {   \
        UINT32 _Size = (UINT32)(size);  \
        if (((rt) = AmpUT_DCFAllocMemFormPool((UINT8 **)(addr), _Size)) != AMP_OK) {   \
            LOG_FAIL("Failed to allocate memory from the pool. addr: %p, size: %u", (UINT8 *)(addr), _Size);    \
        }   \
    }   \
} while(0)

/**
 * The implementation of DCF UT functions
 */

static UINT32 AmpUT_DcfSDBMHash(char *str)
{
    UINT32 Hash = 0;

    while (*str) {
        Hash = *str++ + (Hash << 6) + (Hash << 16) - Hash;
    }
    return Hash;
}

static int AmpUT_DCFAllocMemFormPool(UINT8 **addr, UINT32 size)
{
    void *RawAddr = NULL;
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)addr, &RawAddr, size, AMBA_CACHE_LINE_SIZE) != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    memset(*addr, 0, size);
    return AMP_OK;
}

static AMP_DCF_UT_TEST_CMD_s *DCF_TestCmdList_FindCmd(AMP_DCF_UT_LIST_s *list, char *cmdName)
{
    AMP_DCF_UT_TEST_CMD_s *Cmd;
    UINT32 HashCode = AmpUT_DcfSDBMHash(cmdName);

    for (AMP_DCF_UT_LIST_ITEM_s *item = list->Head; item != NULL; item = item->Next) {
        Cmd = (AMP_DCF_UT_TEST_CMD_s *)item->Itme;
        if (Cmd->HashCode == HashCode) {
            return Cmd;
        }
    }
    return NULL;
}

static int DCF_TestCmdList_CreateCmd(AMP_DCF_UT_TEST_CMD_s **newCmd, AMP_DCF_UT_TEST_CMD_s *cmd) {
    int Ret = AMP_OK;
    AMP_DCF_UT_ALLOC_MEM(Ret, newCmd, sizeof(AMP_DCF_UT_TEST_CMD_s));
    if (Ret == AMP_OK) {
        UINT8 Size;
        (*newCmd)->HashCode = cmd->HashCode;
        (*newCmd)->Hdlr = cmd->Hdlr;
        Size = strlen(cmd->Name);
        AMP_DCF_UT_ALLOC_MEM(Ret, &(*newCmd)->Name, Size + 1);
        if (Ret == AMP_OK) {
            strncpy((*newCmd)->Name, cmd->Name, Size);
        }
    }
    return Ret;
}

static int DCF_TestCmdList_AddItem(AMP_DCF_UT_LIST_s *list, AMP_DCF_UT_TEST_CMD_s *cmd)
{
    int Ret = AMP_OK;
    AMP_DCF_UT_LIST_ITEM_s *Item = NULL;
    AMP_DCF_UT_TEST_CMD_s *Cmd = NULL;

    DCF_TestCmdList_CreateCmd(&Cmd, cmd);
    AMP_DCF_UT_ALLOC_MEM(Ret, &Item, sizeof(AMP_DCF_UT_LIST_ITEM_s));
    if (Ret == AMP_OK) {
        Item->Itme = Cmd;
        Item->Next = NULL;
        if (list->Tail != NULL) {
            list->Tail->Next = Item;
        } else {
            list->Head = Item;
        }
        list->Tail = Item;
        list->Count += 1;
    }
    return Ret;
}

static int AmpUT_DcfRegisterTestCmd(void)
{
    int Ret = AMP_OK;
    AMP_DCF_UT_TEST_CMD_ADD(Ret, auto);
    AMP_DCF_UT_TEST_CMD_ADD(Ret, add_files);
    AMP_DCF_UT_TEST_CMD_ADD(Ret, rmedir);
    return Ret;
}

/**
 * DCF UT - DCF initiate function
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_DcfInit(void)
{
    AMP_DCF_INIT_CFG_s InitCfg;
    UINT32 Size;
    void *buffer;

    if (g_AmpDcfUtConext.IsInit == TRUE) {
        return AMP_OK;
    }
    Size = sizeof(AMP_DCF_UT_LIST_s);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_AmpDcfUtConext.TestCmdList, &buffer, Size, AMBA_CACHE_LINE_SIZE) != OK) {
        LOG_FAIL("Failed to create TestCmdList.");
        return AMP_ERROR_GENERAL_ERROR;
    }
    memset(g_AmpDcfUtConext.TestCmdList, 0, Size);
    if (AmpUT_DcfRegisterTestCmd() != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    // Dcf Init
    if (AmpDCF_GetInitDefaultCfg(&InitCfg) != AMP_OK) {
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpUtil_GetAlignedPool(&G_MMPL, &InitCfg.Buffer, &buffer, InitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        LOG_FAIL("Failed to allocate memory.");
        return AMP_ERROR_GENERAL_ERROR;
    }
    if (AmpDCF_Init(&InitCfg) != AMP_OK) {
        AmbaKAL_BytePoolFree(buffer);
        return AMP_ERROR_GENERAL_ERROR;
    }
    g_AmpDcfUtConext.IsInit = TRUE;
    return AMP_OK;
}

/**
 * DCF UT - CF create function
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_DcfCreate(void)
{
    AMP_DCF_CFG_s DefCfg;
    if (AmpDCF_GetDefaultCfg(&DefCfg) == AMP_OK) {
        if (AmpDCF_Create(&DefCfg, &g_pDcfHdlr) == AMP_OK) {
            return AMP_OK;
        }
        LOG_FAIL("Failed to create a DCF instance.");
    }
    LOG_FAIL("Failed to get DCF default configuration.");
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * DCF UT - DCF delete function
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_DcfDelete(void)
{
    if (AmpDCF_Delete(g_pDcfHdlr) == AMP_OK) {
        return AMP_OK;
    }
    LOG_FAIL("Failed to delete the DCF instance.");
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * DCF UT - DCF add root function
 *
 * @param Name file name
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_DcfAddRoot(char *Name)
{
    if (AmpDCF_AddRoot(g_pDcfHdlr, Name) == AMP_OK) {
        return AMP_OK;
    }
    LOG_FAIL("Failed to add the root %s into DCF.", Name);
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * DCF UT - DCF remove root function
 *
 * @param Name file name
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_DcfRemoveRoot(char *Name)
{
    if (AmpDCF_RemoveRoot(g_pDcfHdlr, Name) == AMP_OK) {
        return AMP_OK;
    }
    LOG_FAIL("Failed to remove the root %s from DCF.", Name);
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * DCF UT - DCF get first id function
 *
 * @return id
 *
 */
static int AmpUT_DcfGetFirstID(void)
{
    const int id = AmpDCF_GetFirstId(g_pDcfHdlr);
    if (id > 0)
        g_ObjId = id;
    return id;
}

/**
 * DCF UT - DCF get last id function
 *
 * @return id
 *
 */
static int AmpUT_DcfGetLastID(void)
{
    const int id = AmpDCF_GetLastId(g_pDcfHdlr);
    if (id > 0)
        g_ObjId = id;
    return id;
}

/**
 * DCF UT - DCF get next id function
 *
 * @return id
 *
 */
static int AmpUT_DcfGetNextID(void)
{
    const int id = AmpDCF_GetNextId(g_pDcfHdlr);
    if (id > 0)
        g_ObjId = id;
    return id;
}

/**
 * DCF UT - DCF get previous id function
 *
 * @return id
 *
 */
static int AmpUT_DcfGetPrevID(void)
{
    const int id = AmpDCF_GetPrevId(g_pDcfHdlr);
    if (id > 0)
        g_ObjId = id;
    return id;
}

/**
 * DCF UT - DCF show file list function
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_DcfShowFileList(void)
{
    if (g_ObjId > 0) {
        UINT32 i;
        AMP_DCF_FILE_LIST_s * const pList = AmpDCF_GetFileList(g_pDcfHdlr, g_ObjId);
        if (pList != NULL) {
            LOG_PRT("%s() : ", __FUNCTION__);
            for (i=0; i<pList->Count; i++) {
                LOG_PRT("\t[%s]", pList->FileList[i].Name);
            }
            AmpDCF_RelFileList(g_pDcfHdlr, pList);
            return 0;
        }
    }
    return -1;
}

/**
 * DCF UT - DCF first directory number function
 *
 * @return directory number
 *
 */
static int AmpUT_DcfGetFirstDnum(void)
{
    const int dnum = AmpDCF_GetFirstDnum(g_pDcfHdlr);
    if (dnum > 0)
        g_Dnum = dnum;
    return dnum;
}

/**
 * DCF UT - DCF last directory number function
 *
 * @return directory number
 *
 */
static int AmpUT_DcfGetLastDnum(void)
{
    const int dnum = AmpDCF_GetLastDnum(g_pDcfHdlr);
    if (dnum > 0)
        g_Dnum = dnum;
    return dnum;
}

/**
 * DCF UT - DCF next directory number function
 *
 * @return directory number
 *
 */
static int AmpUT_DcfGetNextDnum(void)
{
    const int dnum = AmpDCF_GetNextDnum(g_pDcfHdlr);
    if (dnum > 0)
        g_Dnum = dnum;
    return dnum;
}

/**
 * DCF UT - DCF prevoius directory number function
 *
 * @return directory number
 *
 */
static int AmpUT_DcfGetPrevDnum(void)
{
    const int dnum = AmpDCF_GetPrevDnum(g_pDcfHdlr);
    if (dnum > 0)
        g_Dnum = dnum;
    return dnum;
}

/**
 * DCF UT - DCF show directory list function
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_DcfShowDirList(void)
{
    if (g_Dnum > 0) {
        UINT32 i;
        AMP_DCF_DIR_LIST_s * const pList = AmpDCF_GetDirectoryList(g_pDcfHdlr, g_Dnum);
        if (pList != NULL) {
            LOG_PRT("%s() : ", __FUNCTION__);
            for (i=0; i<pList->Count; i++) {
                LOG_PRT("\t[%s]", pList->DirList[i].Name);
            }
            AmpDCF_RelDirectoryList(g_pDcfHdlr, pList);
            return 0;
        }
    }
    return -1;
}

static int AmpUT_DcfAddFile(AMP_DCF_HDLR_s *hdlr, char *name)
{
    int Res = AmpDCF_AddFile(hdlr, name);
    return (Res == AMP_OK || Res == AMP_ERROR_OBJ_ALREADY_EXISTS) ? AMP_OK : Res;
}

static BOOL AmpUT_DcfCompareDirPath(AMP_DCF_HDLR_s *hdlr, char *dir, char* file)
{
    int DirNum = g_pDcfHdlr->Filter->NameToDnum(dir);
    UINT32 FId = g_pDcfHdlr->Filter->NameToId(file);
    int FDirNum = g_pDcfHdlr->Filter->IdToDnum(FId);
    UINT32 DirPathLen = strlen(dir);

    if (DirNum == FDirNum && DirPathLen <= strlen(file)) {
        return (strncmp(dir, file, DirPathLen) == 0) ? TRUE : FALSE;
    }
    return FALSE;
}

int DCF_TestCase_rmedir(int argc, char **argv)
{
#define AMP_DCF_UT_MAX_DIR  100
    int Rval = AMP_OK;
    int DirNum;
    int DirCount, TDirCount;
    int Id;
    char *DirName, *TFileName;
    AMP_DCF_DIR_s AllDirBuf[AMP_DCF_UT_MAX_DIR] = {0};
    AMP_DCF_DIR_LIST_s AllDir = {.Count = 0, .DirList = AllDirBuf};
    AMP_DCF_DIR_LIST_s *DirList;
    AMP_DCF_FILE_LIST_s *FileList;

    LOG_MARK("====== Create DCF =====");
    Rval += AmpUT_DcfCreate();
    Rval += AmpUT_DcfAddRoot(AMP_DCF_UT_PATH_ROOT_DCIM);
    Rval += AmpDCF_Scan(g_pDcfHdlr, FALSE);
    LOG_MARK("====== Add testing files & folders =====");
    Rval += AmpDCF_AddDirectory(g_pDcfHdlr, AMP_DCF_UT_PATH_DIR(104MEDIA));
    Rval += AmpDCF_AddDirectory(g_pDcfHdlr, AMP_DCF_UT_PATH_DIR(105VIDEO));
    Rval += AmpDCF_AddDirectory(g_pDcfHdlr, AMP_DCF_UT_PATH_DIR(105IMAGE));
    Rval += AmpUT_DcfAddFile(g_pDcfHdlr, AMP_DCF_UT_PATH(105VIDEO, AMBA0001.MP4));
    Rval += AmpUT_DcfAddFile(g_pDcfHdlr, AMP_DCF_UT_PATH(105IMAGE, AMBA0001.JPG));
    Rval += AmpUT_DcfAddFile(g_pDcfHdlr, AMP_DCF_UT_PATH(105IMAGE, SGML0001.JPG));
    Rval += AmpDCF_AddDirectory(g_pDcfHdlr, AMP_DCF_UT_PATH_DIR(106VIDEO));
    Rval += AmpDCF_AddDirectory(g_pDcfHdlr, AMP_DCF_UT_PATH_DIR(106IMAGE));
    LOG_MARK("====== Check empty folders =====");
    for (DirNum = AmpDCF_GetFirstDnum(g_pDcfHdlr); DirNum > 0; DirNum = AmpDCF_GetNextDnum(g_pDcfHdlr)) {
        if ((DirList = AmpDCF_GetDirectoryList(g_pDcfHdlr, DirNum)) != NULL) {
            DirCount = DirList->Count;
            while (DirCount--) {
                strncpy(AllDir.DirList[AllDir.Count].Name, DirList->DirList[DirCount].Name, MAX_FILENAME_LENGTH);
                AllDir.Count += 1;
            }
        }
        AmpDCF_RelDirectoryList(g_pDcfHdlr, DirList);
    }
    for (Id = AmpDCF_GetFirstId(g_pDcfHdlr); Id > 0; Id = AmpDCF_GetNextId(g_pDcfHdlr)) {
        if ((FileList = AmpDCF_GetFileList(g_pDcfHdlr, Id)) != NULL) {
            TDirCount = FileList->Count;
            while (TDirCount--) {
                TFileName = FileList->FileList[TDirCount].Name;
                for (DirCount = 0; DirCount < AllDir.Count; DirCount++) {
                    if ((DirName = AllDir.DirList[DirCount].Name)[0] == 0) {
                        continue;
                    }
                    if (AmpUT_DcfCompareDirPath(g_pDcfHdlr, DirName, TFileName) == TRUE) {
                        AllDir.DirList[DirCount].Name[0] = 0;
                    }
                }
            }
            AmpDCF_RelFileList(g_pDcfHdlr, FileList);
        }
    }
    DirCount = AllDir.Count;
    while (DirCount--) {
        if ((DirName = AllDir.DirList[DirCount].Name)[0] != 0) {
            LOG_PRT("Remove the empty folder %s.", DirName);
            if ((Rval += AmpDCF_RemoveDirectory(g_pDcfHdlr, DirName)) != AMP_OK) {
                LOG_FAIL("Failed to remove the empty folder.");
            }
        }
    }
    LOG_MARK("====== Delete DCF =====");
    Rval += AmpUT_DcfRemoveRoot(AMP_DCF_UT_PATH_ROOT_DCIM);
    Rval += AmpUT_DcfDelete();
    return Rval;
#undef AMP_DCF_UT_MAX_DIR
}

int DCF_TestCase_add_files(int argc, char **argv)
{
    int Rval = AMP_OK;
    if (argc > 2) {
        char *Root = argv[2];

        LOG_MARK("====== Create DCF =====");
        Rval += AmpUT_DcfCreate();
        Rval += AmpUT_DcfAddRoot(Root);
        Rval += AmpDCF_Scan(g_pDcfHdlr, TRUE);
        LOG_MARK("====== Add 100MEDIA =====");
        Rval += AmpDCF_AddDirectory(g_pDcfHdlr, "c:\\DCIM\\100MEDIA");
        LOG_MARK("====== Add 100MEDIA\\AMBA0001.TXT =====");
        Rval += AmpUT_DcfAddFile(g_pDcfHdlr, "c:\\DCIM\\100MEDIA\\AMBA0001.TXT");
        AmpUT_DcfGetFirstID();
        Rval += AmpUT_DcfShowFileList();
        LOG_MARK("====== Add 100MEDIA\\AMBA0001.MP4 =====");
        Rval += AmpUT_DcfAddFile(g_pDcfHdlr, "c:\\DCIM\\100MEDIA\\AMBA0001.MP4");
        AmpUT_DcfGetFirstID();
        Rval += AmpUT_DcfShowFileList();
        LOG_MARK("====== Add 100MEDIA\\AMBC0001.TXT =====");
        Rval += AmpUT_DcfAddFile(g_pDcfHdlr, "c:\\DCIM\\100MEDIA\\AMBC0001.TXT");
        AmpUT_DcfGetFirstID();
        Rval += AmpUT_DcfShowFileList();
        LOG_MARK("====== Add 100MEDIA\\AMBA0004.TXT =====");
        Rval += AmpUT_DcfAddFile(g_pDcfHdlr, "c:\\DCIM\\100MEDIA\\AMBA0004.TXT");
        AmpUT_DcfGetFirstID();
        Rval += AmpUT_DcfShowFileList();
        LOG_MARK("====== Delete DCF =====");
        Rval += AmpUT_DcfRemoveRoot(Root);
        Rval += AmpUT_DcfDelete();
    } else {
        Rval = AMP_ERROR_GENERAL_ERROR;
        LOG_PRT("Usage : t dcf add_files {RootPath}");
    }
    return Rval;
}

int DCF_TestCase_auto(int argc, char **argv)
{
    int Rval = AMP_OK;
    if (argc > 2) {
        // init test
        Rval += AmpUT_DcfCreate();
        Rval += AmpUT_DcfAddRoot(argv[2]);
        Rval += AmpDCF_Scan(g_pDcfHdlr, TRUE);
        // directory browsing
        LOG_MARK("====== First Directory =====");
        AmpUT_DcfGetFirstDnum();
        Rval += AmpUT_DcfShowDirList();
        LOG_MARK("====== Next Directory =====");
        AmpUT_DcfGetNextDnum();
        Rval += AmpUT_DcfShowDirList();
        LOG_MARK("====== Last Directory =====");
        AmpUT_DcfGetLastDnum();
        Rval += AmpUT_DcfShowDirList();
        LOG_MARK("====== Prev Directory =====");
        AmpUT_DcfGetPrevDnum();
        Rval += AmpUT_DcfShowDirList();
        // append a legal directory with new dnum
        LOG_MARK("====== Add 104MEDIA =====");
        Rval += AmpDCF_AddDirectory(g_pDcfHdlr, "c:\\DCIM\\104MEDIA");
        AmpUT_DcfGetLastDnum();
        Rval += AmpUT_DcfShowDirList();
        LOG_MARK("====== Add 105MEDIA =====");
        Rval += AmpDCF_AddDirectory(g_pDcfHdlr, "c:\\DCIM\\105MEDIA");
        AmpUT_DcfGetLastDnum();
        Rval += AmpUT_DcfShowDirList();
        LOG_MARK("====== Add 106MEDIA =====");
        Rval += AmpDCF_AddDirectory(g_pDcfHdlr, "c:\\DCIM\\106MEDIA");
        AmpUT_DcfGetLastDnum();
        Rval += AmpUT_DcfShowDirList();
        LOG_MARK("====== Remove 104MEDIA and then show the last three directories =====");
        // remove a empty directory
        Rval += AmpDCF_RemoveDirectory(g_pDcfHdlr, "c:\\DCIM\\104MEDIA");
        AmpUT_DcfGetLastDnum();
        Rval += AmpUT_DcfShowDirList();
        AmpUT_DcfGetPrevDnum();
        Rval += AmpUT_DcfShowDirList();
        AmpUT_DcfGetPrevDnum();
        Rval += AmpUT_DcfShowDirList();
        LOG_MARK("====== Remove 106MEDIA and then show the last directory =====");
        // remove a empty directory
        Rval += AmpDCF_RemoveDirectory(g_pDcfHdlr, "c:\\DCIM\\106MEDIA");
        AmpUT_DcfGetLastDnum();
        Rval += AmpUT_DcfShowDirList();
        LOG_MARK("====== Remove 105MEDIA and then show the last directory =====");
        // remove a empty directory
        Rval += AmpDCF_RemoveDirectory(g_pDcfHdlr, "c:\\DCIM\\105MEDIA");
        AmpUT_DcfGetLastDnum();
        Rval += AmpUT_DcfShowDirList();
        // insert a legal directory to an existed dnum
        LOG_MARK("====== Add 100VIDEO =====");
        Rval += AmpDCF_AddDirectory(g_pDcfHdlr, "c:\\DCIM\\100VIDEO");
        AmpUT_DcfGetFirstDnum();
        Rval += AmpUT_DcfShowDirList();
        // file browsing
        LOG_MARK("====== First File =====");
        AmpUT_DcfGetFirstID();
        Rval += AmpUT_DcfShowFileList();
        LOG_MARK("====== Next File =====");
        AmpUT_DcfGetNextID();
        Rval += AmpUT_DcfShowFileList();
        LOG_MARK("====== Last File =====");
        AmpUT_DcfGetLastID();
        Rval += AmpUT_DcfShowFileList();
        LOG_MARK("====== Prev File =====");
        AmpUT_DcfGetPrevID();
        Rval += AmpUT_DcfShowFileList();
        // add a legal file to an existed fnum
        LOG_MARK("====== Add 100MEDIA\\TEST0001.MP4 and show the first object =====");
        Rval += AmpDCF_AddFile(g_pDcfHdlr, "c:\\DCIM\\100MEDIA\\TEST0001.MP4");
        AmpUT_DcfGetFirstID();
        Rval += AmpUT_DcfShowFileList();
        // remove c:\\DCIM\\100MEDIA\\TEST0001.MP4
        LOG_MARK("====== Remove 100MEDIA\\TEST0001.MP4 and show the first object =====");
        Rval += AmpDCF_RemoveFile(g_pDcfHdlr, "c:\\DCIM\\100MEDIA\\TEST0001.MP4");
        AmpUT_DcfGetFirstID();
        Rval += AmpUT_DcfShowFileList();
        // append a legal file with new fnum
        LOG_MARK("====== Add 103MEDIA\\AMBA0010.TXT and show the last object =====");
        Rval += AmpDCF_AddFile(g_pDcfHdlr, "c:\\DCIM\\103MEDIA\\AMBA0010.TXT");
        AmpUT_DcfGetLastID();
        Rval += AmpUT_DcfShowFileList();
        // remove the last 2nd file of 103MEDIA
        LOG_MARK("====== Remove 103MEDIA\\AMBA0009.TXT and show the last object =====");
        Rval += AmpDCF_RemoveFile(g_pDcfHdlr, "c:\\DCIM\\103MEDIA\\AMBA0009.TXT");
        AmpUT_DcfGetLastID();
        Rval += AmpUT_DcfShowFileList();
        // remove the last file of 103MEDIA
        LOG_MARK("====== Remove 103MEDIA\\AMBA0010.TXT and show the last object =====");
        Rval += AmpDCF_RemoveFile(g_pDcfHdlr, "c:\\DCIM\\103MEDIA\\AMBA0010.TXT");
        AmpUT_DcfGetLastID();
        Rval += AmpUT_DcfShowFileList();
        // remove the last directory
        LOG_MARK("====== Remove 103MEDIA and then show the last directory =====");
        Rval += AmpDCF_RemoveDirectory(g_pDcfHdlr, "c:\\DCIM\\103MEDIA");
        AmpUT_DcfGetLastID();
        Rval += AmpUT_DcfShowFileList();
        // append a legal directory with new dnum
        LOG_MARK("====== Add 103MEDIA and 103MEDIA\\TEST0001.MP4 =====");
        Rval += AmpDCF_AddDirectory(g_pDcfHdlr, "c:\\DCIM\\103MEDIA");
        Rval += AmpDCF_AddFile(g_pDcfHdlr, "c:\\DCIM\\103MEDIA\\TEST0001.MP4");
        // remove the last file of 102MEDIA
        LOG_MARK("====== Remove 102MEDIA\\AMBA0007.TXT and show the last object =====");
        Rval += AmpDCF_RemoveFile(g_pDcfHdlr, "c:\\DCIM\\102MEDIA\\AMBA0007.TXT");
        AmpUT_DcfGetLastID();
        Rval += AmpUT_DcfShowFileList();
        // deinit
        Rval += AmpUT_DcfRemoveRoot(argv[2]);
        Rval += AmpUT_DcfDelete();
    } else {
        Rval = AMP_ERROR_GENERAL_ERROR;
        LOG_PRT("Usage : t dcf auto {RootPath}");
    }
    return Rval;
}

static AMP_DCF_UT_TEST_CMD_HDLR_f AmpUT_DcfGetTestCmdHdlr(char *cmdName)
{
    AMP_DCF_UT_TEST_CMD_HDLR_f Hdlr = NULL;
    AMP_DCF_UT_TEST_CMD_s *Cmd;

    if ((Cmd = DCF_TestCmdList_FindCmd(g_AmpDcfUtConext.TestCmdList, cmdName)) != NULL) {
        Hdlr = Cmd->Hdlr;
    }
    return Hdlr;
}

/**
 * DCF UT - DCF test function
 */
static int AmpUT_DcfTest(AMBA_SHELL_ENV_s *env, int argc, char **argv)
{
    int Res = AMP_ERROR_GENERAL_ERROR;
    AMP_DCF_UT_TEST_CMD_HDLR_f Hdlr;

    if (AmpUT_DcfInit() == AMP_OK) {
        if ((Hdlr = AmpUT_DcfGetTestCmdHdlr(argv[1])) != NULL) {
            Res = Hdlr(argc, argv);
        }
    }
    LOG_RESULT(Res, "DCF %s", argv[1]);
    return Res;
}

/**
 * DCF UT - add DCF test function
 */
int AmpUT_DcfTestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("dcf", AmpUT_DcfTest);
    return AMP_OK;
}

