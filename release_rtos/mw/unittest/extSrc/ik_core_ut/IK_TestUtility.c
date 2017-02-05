#include "AmbaSSP.h" // Note: AmbaShell.h need
#include "AmbaTUNE_HdlrManager.h"
#include "AmbaFS.h"
#include "AmbaCache.h"
#include "IK_TestUtility.h"

#define IK_UT_DEBF(fmt, arg...) AmbaPrint("[IK][UT][DEBUG]%s() %d, "fmt, __func__, __LINE__, ##arg)
#define IK_UT_WARF(fmt, arg...) AmbaPrint("[IK][UT][WARNING]%s() %d, "fmt, __func__, __LINE__, ##arg)
#define IK_UT_ERRF(fmt, arg...) do {AmbaPrint("[IK][UT][ERROR]%s() %d, "fmt, __func__, __LINE__, ##arg); K_ASSERT(0);} while(0)
#define IKUT_SAVE_NG_ISOCFG_2_SD_CAR 1

static IKUT_ER_CODE_e _IK_UnitTest_Init(IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool);
static IKUT_ER_CODE_e _IK_UnitTest_Uninit(IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool);
static IKUT_ER_CODE_e _IK_UnitTest_Print_Log(IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool);
static IKUT_ER_CODE_e _IK_UnitTest_Still_Save_IsoCfg(IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool);
static IKUT_ER_CODE_e _IK_UnitTest_Still_Cmp_IsoCfg(IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool);
//TBD
typedef struct
{
    IKUT_CMD_e cmd;
    IKUT_ER_CODE_e (*funPtr)(IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *MMPL);
} IKUTCmd_s;

typedef struct
{
    UINT32 dataNum;
    IKUT_DATA_CONTAINER_s *data;
    AMBA_MEM_CTRL_s IKUTbuffer;
    AMBA_MEM_CTRL_s TuneBuffer;
} IKUT_Data;

typedef UINT32 UT_DATA_HDLR;

static const IKUTCmd_s IKUTCmd[IKUT_CMD_MAX] = {
  { IKUT_CMD_INIT, _IK_UnitTest_Init },
  { IKUT_CMD_UNINIT, _IK_UnitTest_Uninit },
  { IKUT_CMD_PRINT_LOG, _IK_UnitTest_Print_Log },
  { IKUT_CMD_STILL_SAVE_ISOCONFIG, _IK_UnitTest_Still_Save_IsoCfg },
  { IKUT_CMD_STILL_CMP_ISOCONFIG, _IK_UnitTest_Still_Cmp_IsoCfg },
};

// GOLOBAL
static IKUT_Data G_IKUTData = { 0 };
static UT_DATA_HDLR Active_UTDataHdlr = 0;
static int dataCounter = 0;

static int IKUTData_Create(IKUT_MODE_e mode, AMBA_KAL_BYTE_POOL_t *memoryPool)
{
    memset(&G_IKUTData.data[dataCounter], 0x0, sizeof(IKUT_DATA_CONTAINER_s));
    G_IKUTData.data[dataCounter].Mode = mode;
    G_IKUTData.data[dataCounter].MemPool = memoryPool;
    switch(mode) {
    case IKUT_MODE_STILL:
        G_IKUTData.data[dataCounter].ResultInfo.Still.Result = IKUT_RESULT_UNKNOW;
        break;
    case IKUT_MODE_VIDEO:
    default:
        IK_UT_ERRF("Not Support mode: %d", mode);
        break;
    }

    dataCounter = dataCounter + 1;
    return (dataCounter - 1);
}
static int IKUTData_Get(UT_DATA_HDLR utDatahdlr, IKUT_DATA_CONTAINER_s *data)
{

    if (((UINT32) utDatahdlr) > G_IKUTData.dataNum) {
        IK_UT_ERRF("Unexpected [UT_DATA_HDLR = %d] > [maxIdx = %d]", utDatahdlr, G_IKUTData.dataNum);
    }
    if (data != NULL) {
        memcpy(data, &G_IKUTData.data[utDatahdlr], sizeof(IKUT_DATA_CONTAINER_s));
    } else {
        return IKUT_ERROR_GENERAL_ERROR;
    }
    return IKUT_OK;
}
static int IKUTData_Set(UT_DATA_HDLR utDatahdlr, IKUT_DATA_CONTAINER_s *data)
{
    if (((UINT32) utDatahdlr) > G_IKUTData.dataNum) {
        IK_UT_ERRF("Unexpected [UT_DATA_HDLR = %d] > [maxIdx = %d]", utDatahdlr, G_IKUTData.dataNum);
    }
    if (data != NULL) {
        memcpy( (&(G_IKUTData.data[utDatahdlr])), data, sizeof(IKUT_DATA_CONTAINER_s));
    } else {
        return IKUT_ERROR_GENERAL_ERROR;
    }
    return IKUT_OK;
}

static int _posix_mkdir(const char *folder_name)
{
    int Rval = 0;
    if (AmbaFS_GetCodeMode() == AMBA_FS_UNICODE) {
        K_ASSERT(0);
        /* Note: A12 not support unicode
         static WCHAR w_folder_name[64]; // Note: Not allow multi instance, use static to avoid stack over flow
         w_asc2uni(w_folder_name, folder_name, sizeof(w_folder_name) / sizeof(w_folder_name[0]));
         Rval = AmbaFS_Mkdir((const char*)w_folder_name);
         */
    } else {
        Rval = AmbaFS_Mkdir(folder_name);
    }
    return Rval;
}

extern void img_dsp_core_probe_hook(UINT32 Contact_ID, UINT32 ProbeFunc, int *Result);
static IKUT_ER_CODE_e _Save2Bin(UINT32 IsoCfgAddr, UINT32 IsoCfgSize, char* filename)
{
    AMBA_FS_FILE *Fd;
    char* F_Mode = "wb";
    _posix_mkdir("c:\\bins");

    Fd = AmbaFS_fopen(filename, F_Mode);
    if (Fd == NULL) {
        IK_UT_ERRF("Open File [%s] Error!", filename);
        return IKUT_ERROR_GENERAL_ERROR;
    }
    AmbaFS_fwrite((void *) IsoCfgAddr, 0x1, (UINT64) IsoCfgSize, Fd);
    AmbaFS_fclose(Fd);
    return IKUT_OK;
}
static IKUT_ER_CODE_e _ReadBin(UINT32 IsoCfgAddr, UINT32 IsoCfgSize, char* filename)
{
    AMBA_FS_FILE *Fd;
    char* F_Mode = "rb";

    Fd = AmbaFS_fopen(filename, F_Mode);
    if (Fd == NULL) {
        IK_UT_ERRF("Open File [%s] Error!", filename);
        return IKUT_ERROR_GENERAL_ERROR;
    }
    AmbaFS_fread((void *) IsoCfgAddr, 0x1, (UINT64) IsoCfgSize, Fd);

    AmbaFS_fclose(Fd);
    return IKUT_OK;
}

extern int img_dsp_core_probe_set_liso_cfg_skip_info(UINT32 Addr, UINT32 Size, UINT32 shpSelectAddr);
extern int img_dsp_core_probe_set_hiso_cfg_skip_info(UINT32 Addr, UINT32 Size);
static int _IsoCfg_Dump(UINT32 IsoCfgAddr, UINT32 IsoCfgSize, UINT32 ModeAddr, UINT32 shpSelectAddr, UINT32 Reserved5)
{
    int Rval=0;
    UINT8 *pIsoCfgData;
    AMBA_DSP_IMG_MODE_CFG_s *pMode;
    IKUT_DATA_CONTAINER_s data;
    char *dumpFileName;
    AMBA_MEM_CTRL_s IsoCfgBuffer;
    IKUTData_Get(Active_UTDataHdlr, &data);
    Rval = AmbaKAL_MemAllocate(data.MemPool, &IsoCfgBuffer, IsoCfgSize, 32);
    if (Rval != 0) {
        IK_UT_ERRF("Call AmbaKAL_MemAllocate() Fail");
        return IKUT_ERROR_GENERAL_ERROR;
    }

    pIsoCfgData = (UINT8*) IsoCfgBuffer.pMemAlignedBase;
    memcpy(pIsoCfgData, (void*) IsoCfgAddr, IsoCfgSize);
    dumpFileName = &data.ProcessInfo.Still.BinFileName[0];
    pMode = (AMBA_DSP_IMG_MODE_CFG_s*) ModeAddr;

    if (pMode->AlgoMode == AMBA_DSP_IMG_ALGO_MODE_LISO) {
        img_dsp_core_probe_set_liso_cfg_skip_info((UINT32) pIsoCfgData, IsoCfgSize, shpSelectAddr);
    } else if (pMode->AlgoMode == AMBA_DSP_IMG_ALGO_MODE_HISO) {
        img_dsp_core_probe_set_hiso_cfg_skip_info((UINT32) pIsoCfgData, IsoCfgSize);
    }
    Rval = _Save2Bin((UINT32) pIsoCfgData, IsoCfgSize, dumpFileName);
    IK_UT_DEBF("Save IsoCfg to path: %s", dumpFileName);

    Rval = AmbaKAL_MemFree(&IsoCfgBuffer);
    if (Rval != 0) {
        IK_UT_ERRF("Call AmbaKAL_MemFree() Fail");
        return IKUT_ERROR_GENERAL_ERROR;
    }

    return Rval;
}
void AmbaIKUnitTest_hook_IsoCfg_dump(void)
{
    img_dsp_core_probe_hook(0x0002, (UINT32)_IsoCfg_Dump, NULL);
}

static int _IsoCfg_Cmp(UINT32 IsoCfgAddr, UINT32 IsoCfgSize, UINT32 ModeAddr, UINT32 shpSelectAddr, UINT32 Reserved5)
{
    int Rval;
    UINT8 *pItunerIsoCfgData;
    UINT8 *pCmpTargetIsoCfgData;
    char NG_FileName[128];
    AMBA_DSP_IMG_MODE_CFG_s *pMode;
    IKUT_DATA_CONTAINER_s data;
    AMBA_MEM_CTRL_s IsoCfgBuffer;
    IKUT_CMP_RESULT_e result;
    IKUTData_Get(Active_UTDataHdlr, &data);

    Rval = AmbaKAL_MemAllocate(data.MemPool, &IsoCfgBuffer, IsoCfgSize * 2, 32);
    if (Rval != 0) {
        IK_UT_ERRF("Call AmbaKAL_MemAllocate() Fail");
        return IKUT_ERROR_GENERAL_ERROR;
    }
    pItunerIsoCfgData = (UINT8*) IsoCfgBuffer.pMemAlignedBase;
    pCmpTargetIsoCfgData = pItunerIsoCfgData + IsoCfgSize;
    //1. get IsoCfg
    memcpy(pItunerIsoCfgData, (void*) IsoCfgAddr, IsoCfgSize);
    pMode = (AMBA_DSP_IMG_MODE_CFG_s*) ModeAddr;

    if (pMode->AlgoMode == AMBA_DSP_IMG_ALGO_MODE_LISO) {
        img_dsp_core_probe_set_liso_cfg_skip_info((UINT32) pItunerIsoCfgData, IsoCfgSize, shpSelectAddr);
    } else if (pMode->AlgoMode == AMBA_DSP_IMG_ALGO_MODE_HISO) {
        img_dsp_core_probe_set_hiso_cfg_skip_info((UINT32) pItunerIsoCfgData, IsoCfgSize);
    }
    //2. read golden bin file
    Rval = _ReadBin((UINT32) pCmpTargetIsoCfgData, (UINT32) IsoCfgSize, data.ProcessInfo.Still.BinFileName);

    //3. compare
    if (memcmp((void*) pCmpTargetIsoCfgData, (void*) pItunerIsoCfgData, (size_t) IsoCfgSize) == 0) {
        AmbaPrint("File: %s compare to %s => PASS ", data.ProcessInfo.Still.ItunerFileName, data.ProcessInfo.Still.BinFileName);
        result = IKUT_RESULT_PASS;
    } else {
        AmbaPrint("File: %s compare to %s => NG ", data.ProcessInfo.Still.ItunerFileName, data.ProcessInfo.Still.BinFileName);
        result = IKUT_RESULT_NG;
    }

    //4. Save NG Ituner IsoCfg
    if ( IKUT_SAVE_NG_ISOCFG_2_SD_CAR && (result == IKUT_RESULT_NG)) {
        sprintf(&NG_FileName[0], "%s_NG.bin", &data.ProcessInfo.Still.BinFileName[0]);
        _Save2Bin((UINT32) pItunerIsoCfgData, IsoCfgSize, NG_FileName);
        AmbaPrint("Save NG IsoCfg to Path:%s", NG_FileName);
    }

    Rval = AmbaKAL_MemFree(&IsoCfgBuffer);
    if (Rval != 0) {
        IK_UT_ERRF("Call AmbaKAL_MemFree() Fail");
        return IKUT_ERROR_GENERAL_ERROR;
    }

    return (int)result;
}

void AmbaIKUnitTest_hook_IsoCfg_cmp(IKUT_CMP_RESULT_e *result)
{
    img_dsp_core_probe_hook(0x0002, (UINT32) _IsoCfg_Cmp, (int*)result);
}
int IK_UnitTest(IKUT_CMD_e cmd, IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool)
{
    int i = 0, Rval;
    for (i = 0; i < sizeof(IKUTCmd) / sizeof(IKUTCmd[0]); i++) {
        if (cmd == IKUTCmd[i].cmd) {
            Rval = IKUTCmd[i].funPtr(input, memoryPool);
            return Rval;
        }
    }
    return IKUT_ERROR_GENERAL_ERROR;
}

static IKUT_ER_CODE_e _IK_UnitTest_Init(IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool)
{
    int Rval, i;
    dataCounter = 0;
    G_IKUTData.dataNum = input->InitData.CompareNum;
    if (G_IKUTData.IKUTbuffer.pMemAlignedBase != NULL) {
        IK_UT_WARF("Already Init, please uninit!");
        return IKUT_ERROR_GENERAL_ERROR;
    }
    Rval = AmbaKAL_MemAllocate(memoryPool, &G_IKUTData.IKUTbuffer, sizeof(IKUT_DATA_CONTAINER_s) * (G_IKUTData.dataNum), 32);
    if (Rval != 0) {
        IK_UT_ERRF("Call AmbaKAL_MemAllocate() Fail");
        return IKUT_ERROR_GENERAL_ERROR;
    }
    G_IKUTData.data = (IKUT_DATA_CONTAINER_s*) G_IKUTData.IKUTbuffer.pMemAlignedBase;
    for (i = 0; i < input->InitData.CompareNum; i++) {
//        G_IKUTData.data[i].ResultInfo.Still.Result = IKUT_RESULT_UNKNOW;
        G_IKUTData.data[i].Mode = IKUT_MODE_UNKNOW;
    }
    IK_UT_DEBF("IK Unit Test init done!");
    return IKUT_OK;
}
static IKUT_ER_CODE_e _IK_UnitTest_Uninit(IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool)
{
    if (G_IKUTData.IKUTbuffer.pMemAlignedBase != NULL) {
        AmbaKAL_MemFree(&(G_IKUTData.IKUTbuffer));
        G_IKUTData.IKUTbuffer.pMemAlignedBase = NULL;
    }
    IK_UT_DEBF("IK Unit Test uninit done!");
    return IKUT_OK;
}

static IKUT_ER_CODE_e _IK_UnitTest_Print_Log(IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool)
{
    int i;
    UT_DATA_HDLR utDataHdlr;
    IKUT_DATA_CONTAINER_s data;
    for (i = 0; i < input->InitData.CompareNum; i++) {
        utDataHdlr = i;
        IKUTData_Get(utDataHdlr, &data);
        switch (data.Mode) {
        case IKUT_MODE_STILL:
            if (data.ResultInfo.Still.Result == IKUT_RESULT_PASS) {
                AmbaPrint("[PASS]%50s:%50s", data.ProcessInfo.Still.ItunerFileName, data.ProcessInfo.Still.BinFileName);
            } else if (data.ResultInfo.Still.Result == IKUT_RESULT_NG) {
                AmbaPrint("[ NG ]%50s:%50s", data.ProcessInfo.Still.ItunerFileName, data.ProcessInfo.Still.BinFileName);
            }
            break;
        case IKUT_MODE_VIDEO:
            //TBD
            AmbaPrint("Video Print Log haven't implement yet!");
            break;
        default:
            break;
        }
    }
    return IKUT_OK;
}

static IKUT_ER_CODE_e _IK_UnitTest_Still_Save_IsoCfg(IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool)
{
    int Rval = IKUT_OK;
    TUNE_Initial_Config_s InitialConfig;
    TUNE_Load_Param_s LoadParam;
    ITUNER_INFO_s ItunerInfo;
    AMBA_ITUNER_PROC_INFO_s ProcInfo;
    AMBA_DSP_IMG_CTX_INFO_s DestCtx;
    AMBA_DSP_IMG_CTX_INFO_s SrcCtx;
    AMBA_DSP_IMG_CFG_INFO_s CfgInfo;
    IKUT_DATA_CONTAINER_s data;
    memset(&SrcCtx, 0x0, sizeof(AMBA_DSP_IMG_CTX_INFO_s));
    memset(&DestCtx, 0x0, sizeof(AMBA_DSP_IMG_CTX_INFO_s));
    memset(&CfgInfo, 0x0, sizeof(CfgInfo));

    Active_UTDataHdlr = IKUTData_Create(IKUT_MODE_STILL, memoryPool);
    IKUTData_Get(Active_UTDataHdlr, &data);
    strncpy(&data.ProcessInfo.Still.ItunerFileName[0], input->SaveIsoCfgData.pInItunerFileName, sizeof(data.ProcessInfo.Still.ItunerFileName) -1);
    strncpy(&data.ProcessInfo.Still.BinFileName[0], input->SaveIsoCfgData.pOutBinFileName, sizeof(data.ProcessInfo.Still.BinFileName) - 1);
    IKUTData_Set(Active_UTDataHdlr, &data);

    AmbaIKUnitTest_hook_IsoCfg_dump();

    do {
        AmbaTUNE_Change_Parser_Mode(TEXT_TUNE);
        memset(&InitialConfig, 0x0, sizeof(TUNE_Initial_Config_s));
        InitialConfig.Text.pBytePool = memoryPool;
        if (0 != AmbaTUNE_Init(&InitialConfig)) {
            IK_UT_WARF("Call AmbaTUNE_Init() Fail");
            break;
        }
        memset(&LoadParam, 0x0, sizeof(TUNE_Load_Param_s));
        LoadParam.Text.FilePath = input->SaveIsoCfgData.pInItunerFileName;
        if (0 != AmbaTUNE_Load_IDSP(&LoadParam)) {
            IK_UT_WARF("Call TextHdlr_Load_IDSP(%s) Fail", input->SaveIsoCfgData.pInItunerFileName);
            break;
        }
        if (0 != AmbaTUNE_Get_ItunerInfo(&ItunerInfo)) {
            IK_UT_WARF("Call TextHdlr_Get_ItunerInfo() Fail");
            break;
        }
        {
            DestCtx.Pipe = ItunerInfo.TuningAlgoMode.Pipe;
            DestCtx.CtxId = 0;
            Rval = AmbaDSP_ImgInitCtx(0, 0, &DestCtx, &SrcCtx);
            if (Rval != 0) {
                IK_UT_ERRF("AmbaDSP_ImgInitCtx Fail Line = %d", __LINE__);
                break;
            }
        }
        if (0 != AmbaTUNE_Execute_IDSP(&ItunerInfo.TuningAlgoMode, &ProcInfo)) {
            IK_UT_ERRF("Call TextHdlr_Execute_IDSP() Fail");
            break;
        }
        {
            AMBA_DSP_IMG_SIZE_INFO_s sizeInfo;
            ITUNER_SYSTEM_s System_info;
            memset(&sizeInfo, 0, sizeof(sizeInfo));
            CfgInfo.Pipe = ItunerInfo.TuningAlgoMode.Pipe;
            CfgInfo.CfgId = 0;
            Rval = AmbaDSP_ImgInitCfg(&CfgInfo, ItunerInfo.TuningAlgoMode.AlgoMode);
            if (Rval != 0) {
                break;
            }
            AmbaItuner_Get_SystemInfo(&System_info);
            sizeInfo.WidthIn = System_info.RawWidth;
            sizeInfo.HeightIn = System_info.RawHeight;
            sizeInfo.WidthMain = System_info.MainWidth;
            sizeInfo.HeightMain = System_info.MainHeight;
            AmbaDSP_ImgSetSizeInfo(&ItunerInfo.TuningAlgoMode, &sizeInfo);
            AmbaDSP_ImgPostExeCfg(&ItunerInfo.TuningAlgoMode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY);
        }
    } while (0);

    return Rval;
}

static IKUT_ER_CODE_e _IK_UnitTest_Still_Cmp_IsoCfg(IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool)
{
    int Rval = 0;
    TUNE_Initial_Config_s InitialConfig;
    TUNE_Load_Param_s LoadParam;
    ITUNER_INFO_s ItunerInfo;
    AMBA_ITUNER_PROC_INFO_s ProcInfo;
    AMBA_DSP_IMG_CTX_INFO_s DestCtx;
    AMBA_DSP_IMG_CTX_INFO_s SrcCtx;
    AMBA_DSP_IMG_CFG_INFO_s CfgInfo;
    IKUT_DATA_CONTAINER_s data;
    IKUT_CMP_RESULT_e cmpResult;

    memset(&SrcCtx, 0x0, sizeof(AMBA_DSP_IMG_CTX_INFO_s));
    memset(&DestCtx, 0x0, sizeof(AMBA_DSP_IMG_CTX_INFO_s));
    memset(&CfgInfo, 0x0, sizeof(CfgInfo));

    //1. Create UT data
    Active_UTDataHdlr = IKUTData_Create(IKUT_MODE_STILL, memoryPool);
    IKUTData_Get(Active_UTDataHdlr, &data);
    strncpy(&data.ProcessInfo.Still.ItunerFileName[0], input->SaveIsoCfgData.pInItunerFileName, sizeof(data.ProcessInfo.Still.ItunerFileName) -1);
    strncpy(&data.ProcessInfo.Still.BinFileName[0], input->SaveIsoCfgData.pOutBinFileName, sizeof(data.ProcessInfo.Still.BinFileName) - 1);
    IKUTData_Set(Active_UTDataHdlr, &data);

    //2. hook dump function
    AmbaIKUnitTest_hook_IsoCfg_cmp(&cmpResult);

    //3. load and process ituner
    do {
        AmbaTUNE_Change_Parser_Mode(TEXT_TUNE);
        memset(&InitialConfig, 0x0, sizeof(TUNE_Initial_Config_s));
        InitialConfig.Text.pBytePool = memoryPool;
        if (0 != AmbaTUNE_Init(&InitialConfig)) {
           IK_UT_WARF("Call AmbaTUNE_Init() Fail");
           break;
        }
        memset(&LoadParam, 0x0, sizeof(TUNE_Load_Param_s));
        LoadParam.Text.FilePath = input->SaveIsoCfgData.pInItunerFileName;
        if (0 != AmbaTUNE_Load_IDSP(&LoadParam)) {
           IK_UT_WARF("Call TextHdlr_Load_IDSP(%s) Fail", input->SaveIsoCfgData.pInItunerFileName);
           break;
        }
        if (0 != AmbaTUNE_Get_ItunerInfo(&ItunerInfo)) {
           IK_UT_WARF("Call TextHdlr_Get_ItunerInfo() Fail");
           break;
        }
        {
           DestCtx.Pipe = ItunerInfo.TuningAlgoMode.Pipe;
           DestCtx.CtxId = 0;
           Rval = AmbaDSP_ImgInitCtx(0, 0, &DestCtx, &SrcCtx);
           if (Rval != 0) {
               IK_UT_ERRF("AmbaDSP_ImgInitCtx Fail Line = %d", __LINE__);
               break;
           }
        }
        if (0 != AmbaTUNE_Execute_IDSP(&ItunerInfo.TuningAlgoMode, &ProcInfo)) {
           IK_UT_ERRF("Call TextHdlr_Execute_IDSP() Fail");
           break;
        }
        {
            AMBA_DSP_IMG_SIZE_INFO_s sizeInfo;
            ITUNER_SYSTEM_s System_info;
            memset(&sizeInfo, 0, sizeof(sizeInfo));
            CfgInfo.Pipe = ItunerInfo.TuningAlgoMode.Pipe;
            CfgInfo.CfgId = 0;
            Rval = AmbaDSP_ImgInitCfg(&CfgInfo, ItunerInfo.TuningAlgoMode.AlgoMode);
            if (Rval != 0) {
                break;
            }
            AmbaItuner_Get_SystemInfo(&System_info);
            sizeInfo.WidthIn = System_info.RawWidth;
            sizeInfo.HeightIn = System_info.RawHeight;
            sizeInfo.WidthMain = System_info.MainWidth;
            sizeInfo.HeightMain = System_info.MainHeight;
            AmbaDSP_ImgSetSizeInfo(&ItunerInfo.TuningAlgoMode, &sizeInfo);
            AmbaDSP_ImgPostExeCfg(&ItunerInfo.TuningAlgoMode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY);
        }
    } while (0);

    //4. Save reults
    IKUTData_Get(Active_UTDataHdlr, &data);
    data.ResultInfo.Still.Result = cmpResult;
    IKUTData_Set(Active_UTDataHdlr, &data);

    return Rval;
}

// VIDEO CMP ARCHITECTURE
//  Create_UTData();
//  GetUTData(Active_UTDataHdlr, Data);
//  Data.Video.TotalFrame = 10;
//  Data.Video.CurrentFrame = 0;
//  Data.Video.BinFile[];
//  Data.Video.ItunerFile[];
//  Data.Video.Script = ""
//  Script2Bin(Script, BinFile, ItunerFile);
//
//
//  Active_UTDataHdlr = Create_UTData();
//  GetUTData(Active_UTDataHdlr, Data);
//  Data.Still.BinFile = xxx;
//  SetUTData(Active_UTDataHdlr, Data);

// Step1 Hook
//  sprintf(G_FileName,"%s",filename);
//  G_IKUTData.resultMsg[i].cmpData.Still.BinFileName = "";
//  G_IKUTData.resultMsg[i].cmpData.Still.ItunerFileName = "";
//  img_dsp_core_probe_hook(0x0002,(UINT32)_IsoCfg_Cmp,&G_IKUTData.resultMsg[i].cmpData.Still.Result);

// Do Test Process

// Update Result
