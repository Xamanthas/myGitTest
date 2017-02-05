#include "AmbaDSP_ImgDef.h"

#ifndef NULL
#define NULL (void*)0
#endif
#ifndef MAX_FILE_PATH
#define MAX_FILE_PATH (64)
#endif
typedef enum _IKUT_ER_CODE_e_ {
    IKUT_OK = 0,                   //!< Execution OK
    IKUT_ERROR_GENERAL_ERROR = -1, //!< General error
} IKUT_ER_CODE_e;

typedef enum _IKUT_CMD_ {
	IKUT_CMD_INIT = 0x00 ,
	IKUT_CMD_UNINIT,
	IKUT_CMD_PRINT_LOG ,
	IKUT_CMD_STILL_SAVE_ISOCONFIG ,
	IKUT_CMD_STILL_CMP_ISOCONFIG ,
	IKUT_CMD_MAX,
} IKUT_CMD_e;

typedef enum _IKUT_MODE_ {
	IKUT_MODE_STILL = 0x00 ,
	IKUT_MODE_VIDEO,
	IKUT_MODE_UNKNOW,
} IKUT_MODE_e;

typedef enum _IKUT_CMP_RESULT_ {
	IKUT_RESULT_NG = 0x00,
	IKUT_RESULT_PASS,
	IKUT_RESULT_UNKNOW,
} IKUT_CMP_RESULT_e;

typedef struct {
	UINT32 CompareNum;
	IKUT_MODE_e Mode;
} INIT_INFO_s;

typedef struct {
	char* pInItunerFileName;
	char* pOutBinFileName;
} STILL_SAVE_ISOCONFIG_INFO_S_s;

typedef struct {
	char* pInItunerFileName;
	char* pInBinFileName;
} STILL_CMP_ISOCONFIG_INFO_S_s;

typedef union {
	INIT_INFO_s InitData;
	INIT_INFO_s UninitData;
	INIT_INFO_s PrintLogData;
	//STILL
	STILL_SAVE_ISOCONFIG_INFO_S_s SaveIsoCfgData;
	STILL_CMP_ISOCONFIG_INFO_S_s CmpIsoCfgData;
	//VIDEO

	//TBD
} IKUT_INPUTS_u;

typedef struct {
	IKUT_CMP_RESULT_e Result;
} IKUT_STILL_RESULT_INFO_s;

typedef struct {
	UINT32 IsoCfgSize;
	char ItunerFileName[MAX_FILE_PATH];
	char BinFileName[MAX_FILE_PATH];
	//TBD
} IKUT_STILL_PROCESS_INFO_s;

typedef struct {
	IKUT_MODE_e Mode;
	union {
		IKUT_STILL_RESULT_INFO_s Still;
		//TBD
	} CmpData;
} IKUT_RESULT_INFO_s;
typedef struct {
	IKUT_MODE_e Mode;
	AMBA_KAL_BYTE_POOL_t *MemPool;
	union {
		IKUT_STILL_PROCESS_INFO_s Still;
		//TBD
	} ProcessInfo;
	union {
		IKUT_STILL_RESULT_INFO_s Still;
		//TBD
	} ResultInfo;
} IKUT_DATA_CONTAINER_s;
/**
 * @brief Iamge Kernel Unit Test
 * @param[in] IKUT_INPUT *input: IK unit test inputs
 *
 * return 0 (Success) / -1 (Failure)
 */
int IK_UnitTest( IKUT_CMD_e cmd, IKUT_INPUTS_u *input, AMBA_KAL_BYTE_POOL_t *memoryPool );
