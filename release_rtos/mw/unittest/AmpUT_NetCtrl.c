/**
 * @file AmpUT_NetCtrl.c
 *
 * Copyright (C) 2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <net/NetCtrl.h>
#include <net/Json_Utility.h>
#include "AmbaIPC_RpcProg_RT_NetCtrl.h"
#include "AmbaIPC_RpcProg_LU_NetCtrl.h"
#include "AmpUnitTest.h"
#include "AmbaUtility.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaCache_Def.h>
#include <util.h>
#include "AmpUT_NetCtrl.h"

#if defined (CONFIG_BT_AMBA)
#define BT_AMBA_APP 1
#endif

#if defined (BT_AMBA_APP)
#include <net/NetBleCfg.h>
#include <../bt/amba_bluetooth.h>
#endif

//#define DEBUG AmbaPrint
#define DEBUG(x,...)

#define DBG_P             AmbaPrint("%s, %d >>>", __FUNCTION__, __LINE__)

//#define DEBUG_UT_NETCTRL 1

#define AMPUT_NETCTRL_CMDTASK_SIZE                                 0x2000
#define AMPUT_NETCTRL_CMDHNDLR_PRIORITY                     40
#define AMPUT_NETCTRL_CMDHNDLR_MSGQUEUE_SIZE         64
#define AMPUT_NETCTRL_DATASVC_THUMB_SIZE                   (1024*1024*256)
#define AMPUT_NETCTRL_TICKS                                                 30000
#define AMPUT_NETCTRL_RETBUF_SIZE                                     1024
#define AMPUT_NETCTRL_JSONSTROBJ_SIZE                            512

#if defined (BT_AMBA_APP)
#define SEND_BLE_CMD    0
#define SEND_BLE_DATA   0
//Define global variables:
static AMBA_KAL_TASK_t G_BtTsk;
#define BT_TSK_PRIORITY   (180)
#define BT_TSK_STACK_SIZE (0x1000)
static unsigned char G_BtStack[BT_TSK_STACK_SIZE];
static int G_BtTskID = 0;

static amba_bt_if_t *G_pBtIns = NULL;
static gatt_service_t G_GATT1;

//The chararictic id is same as "G_BleWriteId" : it is used to write the JSON string to Camera
//The chararictic id is same as "G_BleReadId" : it is used to read the JSON string from Camera
#define NUM_ATTR (4)
static int G_BleWriteId = 0;
static int G_BleReadId = 2;
static gatt_char_t G_GATTChar1[NUM_ATTR];
static int G_BleInited = 0;
#endif

#define SEND_WIFI_CMD   1
#define SEND_WIFI_DATA  1
#define SEND_DATA_NULL  2

#define MAX_JSON_STR_LEN 1024
static char G_JsonStr[MAX_JSON_STR_LEN] = {0};
static int G_CurJsonStrLen = 0;


static int G_SendType = SEND_WIFI_CMD;
static int G_SendDataType = SEND_WIFI_CMD;

static AMBA_KAL_MSG_QUEUE_t AmpUT_NetCtrl_Cmd_MsgQ;
static AMBA_KAL_TASK_t AmpUT_NetCtrl_Cmd_Task;
static char AmpUT_NetCtrl_CmdHndlr_Stack[AMPUT_NETCTRL_CMDTASK_SIZE];
static char AmpUT_NetCtrl_CmdMsgQ_Stack[sizeof(AMPUT_NETCTRL_CMD_MESSAGE_s)*AMPUT_NETCTRL_CMDHNDLR_MSGQUEUE_SIZE];

static UINT32 AmpUT_NetCtrl_MsgQ_Digit[8];
static char AmpUT_NetCtrl_MsgQ_Buf[4][AMPUT_NETCTRL_JSONSTROBJ_SIZE];
typedef char (*MsgQBuf)[AMPUT_NETCTRL_JSONSTROBJ_SIZE];

static AMP_NETCTRL_HDLR_INFO_s G_HdlrInfo = {0};
#if defined (BT_AMBA_APP)
static AMP_NETCTRL_HDLR_INFO_s G_HdlrInfoBle = {0};
#endif
static AMP_NETCTRL_DATASVC_HDLR_INFO_s G_DataHdlrInfo = {0};

#define SUPPORT_LNX_SHELL_CMD 1
#define NOT_SUPPORT_LNX_SHELL_CMD 0
static int G_SupportLnxCmdFlag = NOT_SUPPORT_LNX_SHELL_CMD;

/* The mount point for each storage drive is:*/
/* FL0 -> A
     FL1 -> B
     SD0-> C
     SD1-> D
     RD  -> E
     IDX -> F
     PRF -> G
     CAL -> H
     USB -> I
     SDIO -> L
     RF1 -> Y
     RF2 -> Z
*/

static AMPUT_FILEPATH_s G_Filepath[] = {
    { {'/', 't', 'm', 'p', '/', 'F', 'L', '0', '/'}, {'A', ':', '\\'} },
    { {'/', 't', 'm', 'p', '/', 'S', 'D', '0', '/'}, {'C', ':', '\\'} },
    { {'/', 't', 'm', 'p', '/', 'F', 'L', '1', '/'}, {'B', ':', '\\'} },
    { {'/', 't', 'm', 'p', '/', 'S', 'D', '1', '/'}, {'D', ':', '\\'} },
    { {'/', 't', 'm', 'p', '/', 'R', 'D', '/'}, {'E', ':', '\\'} },
    { {'/', 't', 'm', 'p', '/', 'I', 'D', 'X', '/'}, {'F', ':', '\\'} },
    { {'/', 't', 'm', 'p', '/', 'P', 'R', 'F', '/'}, {'G', ':', '\\'} },
    { {'/', 't', 'm', 'p', '/', 'C', 'A', 'L', '/'}, {'H', ':', '\\'} },
    { {'/', 't', 'm', 'p', '/', 'U', 'S', 'B', '/'}, {'I', ':', '\\'} },
    { {'/', 't', 'm', 'p', '/', 'S', 'D', 'I', 'O', '/'}, {'L', ':', '\\'} },
    { {'/', 't', 'm', 'p', '/', 'R', 'F', '1', '/'}, {'Y', ':', '\\'} },
    { {'/', 't', 'm', 'p', '/', 'R', 'F', '2', '/'}, {'Z', ':', '\\'} },
};

static int G_FilePathSize = sizeof(G_Filepath) / sizeof(AMPUT_FILEPATH_s);

extern AMBA_KAL_BYTE_POOL_t G_MMPL;
static int G_Task_Init = 0;
static int G_CmdTask = 0;
static unsigned int G_ActiveToken = 0;
static unsigned int G_ClientToken = 0;
static unsigned int G_ActiveClientId = 0;
static unsigned char G_ActiveClientInfo[128];
static char G_ActiveTransportType[8];
static char *G_RetBuf = NULL;
static void *G_RetBufRawBaseAddr = NULL;

int AmpUT_NetCtrl_Example_Parse(AMP_JSON_OBJECT_s *jsonObj, AMP_JSON_CMD_s *cmdMsg, UINT32 *retToken)
{
    AMP_JSON_TYPE_e ObjType;
    AMP_JSON_OBJECT_s *TmpJsonObj;
    int RetVal =0;

    TmpJsonObj = AmpJson_GetObjectByKey(jsonObj, "msg_id");
    if (TmpJsonObj == NULL) {
        AmbaPrint("tmp_obj is NULL");
        return -1;
    }

    ObjType = AmpJson_GetObjectType(TmpJsonObj);
    if (ObjType == AMP_JSON_TYPE_INTEGER) {              //we only accept msg_id with integer type
        cmdMsg->MsgId= AmpJson_GetInteger(TmpJsonObj);
    } else {
        AmbaPrint("msg_id must be integer type");
        return -1;
    }

    TmpJsonObj = AmpJson_GetObjectByKey(jsonObj, "token");
    if(TmpJsonObj == NULL){
        AmbaPrint("tmp_obj is NULL");
        return -1;
    }

    ObjType = AmpJson_GetObjectType(TmpJsonObj);
    if (ObjType == AMP_JSON_TYPE_INTEGER) {              //we only accept token with integer type
        *retToken = AmpJson_GetInteger(TmpJsonObj);
    } else {
        AmbaPrint("token must be integer type");
        return -1;
    }

    return RetVal;
}

int AmpUT_NetCtrl_Example_Parse_Param(AMP_JSON_OBJECT_s *jsonObj, AMP_JSON_CMD_s *cmdMsg)
{
    AMP_JSON_TYPE_e ObjType;
    AMP_JSON_OBJECT_s *TmpJsonObj;
    char *TmpStr = NULL;
    int RetVal = 0, JsonStrLen = 0, TmpInt = 0;
    double TmpDouble = 0.0;


    TmpJsonObj = AmpJson_GetObjectByKey(jsonObj, "param");
    if (TmpJsonObj == NULL) {
        AmbaPrint("tmp_obj is NULL");
        return 0;
    }

    ObjType = AmpJson_GetObjectType(TmpJsonObj);
    if (ObjType == AMP_JSON_TYPE_STRING) {
        if ((TmpStr = AmpJson_GetString(TmpJsonObj)) == NULL) {
            AmbaPrint("TmpStr is NULL");
            return -1;
        }
        JsonStrLen = strlen(TmpStr) + 1;
        cmdMsg->ParamSize = JsonStrLen;
        if (sizeof(cmdMsg->Param) >= JsonStrLen) {
            strcpy(cmdMsg->Param, TmpStr);
	}
    } else if (ObjType == AMP_JSON_TYPE_INTEGER){
        TmpInt = AmpJson_GetInteger(TmpJsonObj);
        cmdMsg->ParamSize= sizeof(TmpInt);
        memcpy((void*)(&cmdMsg->Param[0]), (void*) &TmpInt, cmdMsg->ParamSize);
    } else if (ObjType == AMP_JSON_TYPE_BOOLEAN){
        TmpInt = AmpJson_GetBoolean(TmpJsonObj) ? 1:0;
        cmdMsg->ParamSize= sizeof(TmpInt);
        memcpy((void*)(&cmdMsg->Param[0]), (void*) &TmpInt, cmdMsg->ParamSize);
    } else if (ObjType == AMP_JSON_TYPE_DOUBLE){
        TmpDouble = AmpJson_GetDouble(TmpJsonObj);
        cmdMsg->ParamSize = sizeof(TmpDouble);
        memcpy((void*)(&cmdMsg->Param[0]),(void*) &TmpDouble, cmdMsg->ParamSize);
    } else {
        DEBUG("type of 'param' is unregonized.");
        return -1;
    }

    return RetVal;
}

int AmpUT_NetCtrl_Transform_FilePath(char *filepath)
{
    char *character = NULL;
    int matched_index = -1, length = 0, i = 0;

    if (filepath == NULL) {
        return -1;
    }

    length = strlen(filepath);
    for (i = 0; i < G_FilePathSize; i++) {
        if (strncmp(filepath, G_Filepath[i].fp_lnx, strlen(G_Filepath[i].fp_lnx)) == 0) {
            matched_index = i;
            break;
        }
    }

    if (matched_index == -1) {
        return -1;
    }

    strncpy(filepath, G_Filepath[matched_index].fp, strlen(G_Filepath[matched_index].fp));
    strncpy(filepath + strlen(G_Filepath[matched_index].fp), filepath + strlen(G_Filepath[matched_index].fp_lnx),
                strlen(filepath) - strlen(G_Filepath[matched_index].fp_lnx));
    *((char *)filepath + length - strlen(G_Filepath[matched_index].fp_lnx) + strlen(G_Filepath[matched_index].fp)) = 0;

    while ((character = strchr(filepath, '/')) != NULL) {
        *character = '\\';
    }

    return (int)(G_Filepath[matched_index].fp[0] - 'A');
}


/**
 * This function is used to check the format of JSON string.
 */
static int* AmpUT_CheckJsonFormat(char *inString)
{
    int LenE = 0;
    int Flag = 0;
    int DataLen = strlen(inString);
    static int LenArray[4]; //[0]: 1: there is a correct json in inString
                            //the negative value or zero: the length of pure charachers without any '{' or '}' in it
                            //[1]: the begin location of the correct json str based on the begin address of inString.
                            //[2]: the end location of the correct json str based on the begin address of inString.
                            //[3]: the length of sub-JSON string such as the length "{XXXX" or "XXXX}"
    int SerchJsonStrFlag = 1;
    int PureCharLen = 0;

    if ((DataLen == 0) || (inString == NULL)) {
        return LenArray;
    }

    //reset this array because it is static in this function
    LenArray[0] = 0;
    LenArray[1] = -1;
    LenArray[2] = -1;
    LenArray[3] = -1;

    while (SerchJsonStrFlag == 1) {
        if (*inString == '{') {
             SerchJsonStrFlag = 0;
             LenArray[1] = PureCharLen;
             LenE = LenE + 1;
             Flag = 1;
             while (Flag == 1) {
                 if ((LenE+PureCharLen) <= DataLen) {
                     if ((*(inString+LenE)) == '}') {
                         Flag = 0;
                         LenArray[0] = 1;
                         LenArray[2] = PureCharLen + LenE;
                     } else {
                         /*if ((*(inString+LenE)) == '{') {
                             LenArray[1] = LenE;
                             LenE = LenE + 1;
                         }else {*/
                             LenE = LenE + 1;
                         //}
                     }
                  } else {
                     Flag = 0;
                  }
             }
             //Record the length if '{' is exised but '}' is not
             if (LenArray[2] == -1) {
                 LenArray[3] = LenE;
             }
        }else{//if (*inString == '{')
            if (*inString == '}') {
             //Record the length and index  if '}' is exised but '{' is not
              if (LenArray[1] == -1) {
                 LenArray[2] = PureCharLen;
                 LenArray[3] = PureCharLen+1;
                 SerchJsonStrFlag = 0;
              }
            }else{
                inString = inString + 1;
                PureCharLen = PureCharLen + 1;
                if (PureCharLen == DataLen) {
                    SerchJsonStrFlag = 0;
                }
            }
        }//if (*inString == '{')
    }
    if ((PureCharLen != 0) && (LenArray[1] < 0)) {
        LenArray[0] = -PureCharLen;
    }
    return LenArray;
}

int AmpUT_NetCtrl_RecvCmd(AMP_NETCTRL_HDLR_INFO_s *hdlrInfo, AMP_NETCTRL_CMD_s *dataFromLnx)
{
    int RetVal = 0, RetStatus = 0;
    UINT32 Token = 0;
    int CorrectJsonLen = 0;
    int *CheckJsonF;
    AMPUT_NETCTRL_CMD_MESSAGE_s CmdMsg;
    AMP_NETCTRL_DATASVC_CANCEL_TRANS_s CancelTrans;
    AMP_NETCTRL_DATASVC_CANCEL_RESULT_s CancelResult;
    AMP_JSON_STRING_s *JsonStr = NULL;
    AMP_JSON_CMD_s JsonCmdMsg;
    AMP_JSON_OBJECT_s *JsonObj = NULL, *JsonArrObj = NULL, *JsonResObj = NULL;
    AMP_NETCTRL_DATASVC_HDLR_INFO_s DataHdlrInfo = {0};
    AMP_NETCTRL_SEND_INFO_s SendInfo = {0};
    AMP_NETCTRL_DATASVC_DEST_INFO_s DestDataHdlrInfo = {0};
    SendInfo.ClientId = dataFromLnx->ClientId;

    //Please fill the following items based on your case
    //Use the wifi to be the example
    if ( G_SendDataType == SEND_WIFI_DATA) {
        DataHdlrInfo.InstTypeId = G_DataHdlrInfo.InstTypeId;
        DataHdlrInfo.Hdlr = G_DataHdlrInfo.Hdlr;
    }

    if(dataFromLnx != NULL) {
        AmbaPrint("%u: param = %s", dataFromLnx->ClientId, dataFromLnx->Param);
    } else {
        AmbaPrint("%s: pDataFromLnx is NULL", __FUNCTION__);
        return -1;
    }

    memset(G_RetBuf, 0x00, AMPUT_NETCTRL_RETBUF_SIZE);
    memset(&CmdMsg, 0x00, sizeof(AMPUT_NETCTRL_CMD_MESSAGE_s));
    memset(&CancelTrans, 0x00, sizeof(AMP_NETCTRL_DATASVC_CANCEL_TRANS_s));
    memset(&CancelResult, 0x00, sizeof(AMP_NETCTRL_DATASVC_CANCEL_RESULT_s));
    memset(&JsonCmdMsg, 0x00, sizeof(AMP_JSON_CMD_s));

    memset(AmpUT_NetCtrl_MsgQ_Digit, 0x0, sizeof(AmpUT_NetCtrl_MsgQ_Digit));
    memset(AmpUT_NetCtrl_MsgQ_Buf, 0x0, sizeof(AmpUT_NetCtrl_MsgQ_Buf));

    //----- check the content of input string ------------//
#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d, gotten string : %s", __FUNCTION__,__LINE__,dataFromLnx->Param);
#endif
    CheckJsonF = AmpUT_CheckJsonFormat(dataFromLnx->Param);
    CorrectJsonLen = CheckJsonF[2] - CheckJsonF[1] + 1;
#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d, CheckJsonF[0]:%d,  CheckJsonF[1]:%d,  CheckJsonF[2]:%d,  CheckJsonF[3]:%d", __FUNCTION__,__LINE__,CheckJsonF[0],CheckJsonF[1], CheckJsonF[2], CheckJsonF[3]);
#endif
    if ((CheckJsonF[0] == 1) && (CorrectJsonLen > 2)){
        //Case1: There is a full JSON string in the input
        memset(G_JsonStr, '\0', MAX_JSON_STR_LEN);
        memcpy(G_JsonStr, (dataFromLnx->Param+CheckJsonF[1]), CorrectJsonLen);
#if defined DEBUG_UT_NETCTRL
        AmbaPrint("%s,%d, Case1: All gotten JSON string is :%s, CheckJsonF[1]:%d, CorrectJsonLen:%d", __FUNCTION__,__LINE__,G_JsonStr, CheckJsonF[1], CorrectJsonLen);
#endif
    } else {
        if (CheckJsonF[1] != -1) {
            if ((CheckJsonF[2] == -1) && (CheckJsonF[3] != -1)) {
                //Case2: There is a '{' in the input but no '}'
                memset(G_JsonStr, '\0', MAX_JSON_STR_LEN);
                memcpy(G_JsonStr, dataFromLnx->Param+CheckJsonF[1], CheckJsonF[3]);
                G_CurJsonStrLen = CheckJsonF[3];
#if defined DEBUG_UT_NETCTRL
                AmbaPrint("%s,%d, Case2: All gotten JSON string is :%s", __FUNCTION__,__LINE__,G_JsonStr);
#endif
                return 0;
            }
         } else {
            if (CheckJsonF[2] != -1) {
                if (CheckJsonF[3] != -1) {
                    //Case3: There is a '}' in the input but no '{'
                    memmove(G_JsonStr+G_CurJsonStrLen-1, dataFromLnx->Param, CheckJsonF[3]);
#if defined DEBUG_UT_NETCTRL
                    AmbaPrint("%s,%d, Case3: All gotten JSON string is :%s", __FUNCTION__,__LINE__,G_JsonStr);
#endif
                    G_CurJsonStrLen = 0;
                }
             } else {
                //Case4: There is bot no '}' and '{' in the input
                memmove(G_JsonStr+G_CurJsonStrLen-1, dataFromLnx->Param, -(CheckJsonF[0]));
                G_CurJsonStrLen = G_CurJsonStrLen + (-(CheckJsonF[0]));
#if defined DEBUG_UT_NETCTRL
                AmbaPrint("%s,%d, Case4: All gotten JSON string is :%s", __FUNCTION__,__LINE__,G_JsonStr);
#endif
                return 0;
             }
         }
    }
    AmbaPrint("The final JSON string is :%s", G_JsonStr);
    //-----------------------------------------------------//

    JsonObj = AmpJson_JsonStringToJsonObject(G_JsonStr);
    AmpUT_NetCtrl_Example_Parse(JsonObj, &JsonCmdMsg, &Token);

    AmpUT_NetCtrl_MsgQ_Digit[0] = Token;
    AmpUT_NetCtrl_MsgQ_Digit[1] = JsonCmdMsg.MsgId;
    AmpUT_NetCtrl_MsgQ_Digit[2] = dataFromLnx->ClientId;

    DEBUG("spDataFromLnx: Token=%d, msg_id=%d, sockid=%d", Token, JsonCmdMsg.MsgId, dataFromLnx->ClientId);
    DEBUG("spDataFromLnx: ClientIp=%x", dataFromLnx->ClientId);

    CmdMsg.MsgId = 1;
    CmdMsg.MsgData[0] = (UINT32)&AmpUT_NetCtrl_MsgQ_Digit;
    CmdMsg.MsgData[1] = (UINT32)&AmpUT_NetCtrl_MsgQ_Buf;

    JsonResObj = AmpJson_CreateObject();
    JsonArrObj = AmpJson_CreateArrayObject();

    switch (JsonCmdMsg.MsgId) {
    case AMBA_GET_ALL_CURRENT_SETTINGS:
        if (G_ActiveToken != Token) {
            RetVal = -1;
        }

        AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
        AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_GET_ALL_CURRENT_SETTINGS));

        AmpJson_AddObject(JsonArrObj, "Video_Resolution", AmpJson_CreateStringObject("1920x1080 30P 16:9"));
        AmpJson_AddObject(JsonArrObj, "Video_Quality", AmpJson_CreateStringObject("fine"));
        AmpJson_AddObject(JsonResObj, "param", JsonArrObj);

        JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
        if (JsonStr != NULL) {
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);
        }
        break;
    case AMBA_GET_SINGLE_SETTING_OPTIONS:
        if (G_ActiveToken != Token) {
            RetVal = -1;
        }

        AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
        AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_GET_SINGLE_SETTING_OPTIONS));
        AmpJson_AddObject(JsonResObj, "permission", AmpJson_CreateStringObject("settable"));
        AmpJson_AddObject(JsonResObj, "param", AmpJson_CreateStringObject("video_resolution"));

        AmpJson_AddObjectToArray(JsonArrObj, AmpJson_CreateStringObject("1920x1080 60P 16:9"));
        AmpJson_AddObjectToArray(JsonArrObj, AmpJson_CreateStringObject("1920x1080 30P 16:9"));
        AmpJson_AddObject(JsonResObj, "options", JsonArrObj);

        JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
        if (JsonStr != NULL) {
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);

        }
        break;
    case AMBA_START_SESSION:
        if (!G_ActiveToken) {
            G_ClientToken++;
            G_ActiveToken = G_ClientToken;
            G_ActiveClientId = dataFromLnx->ClientId;
            RetVal = 0;
        } else {
            RetVal = -1;
            AmbaPrint("can't get token, start session fail, active token = %d", G_ActiveToken);
        }

        AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
        AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_START_SESSION));
        AmpJson_AddObject(JsonResObj, "param", AmpJson_CreateIntegerObject(G_ActiveToken));

        JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
        if (JsonStr != NULL) {
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);

        }
        break;
    case AMBA_STOP_SESSION:
        if (G_ActiveToken != Token) {
            RetVal = -1;
        } else {
            G_ActiveToken = 0;
            RetVal = 0;
        }

        AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
        AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_STOP_SESSION));

        JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
        if (JsonStr != NULL) {
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);
        }
        memcpy(DestDataHdlrInfo.ClientInfo, G_ActiveClientInfo, sizeof(DestDataHdlrInfo.ClientInfo));
        strcpy(DestDataHdlrInfo.TransportType, G_ActiveTransportType);
        AmpNetCtrl_DataSvc_CloseConnection(&DataHdlrInfo, &DestDataHdlrInfo, &RetStatus);
        AmbaPrint("%s: RetStatus = %d", __FUNCTION__, RetStatus);
        break;
    case AMBA_SET_CLNT_INFO:
        if (G_ActiveToken != Token) {
            RetVal = -1;
        }

        AmpJson_GetStringByKey(JsonObj, "type", AmpUT_NetCtrl_MsgQ_Buf[0], AMPUT_NETCTRL_JSONSTROBJ_SIZE);
        AmpJson_GetStringByKey(JsonObj, "param", AmpUT_NetCtrl_MsgQ_Buf[1], AMPUT_NETCTRL_JSONSTROBJ_SIZE);
        strcpy(G_ActiveTransportType, AmpUT_NetCtrl_MsgQ_Buf[0]);

        if (!strcmp(AmpUT_NetCtrl_MsgQ_Buf[0], "TCP")) {
            sscanf(AmpUT_NetCtrl_MsgQ_Buf[1], "%hhu.%hhu.%hhu.%hhu", &G_ActiveClientInfo[0], &G_ActiveClientInfo[1],
                                                                                                                     &G_ActiveClientInfo[2], &G_ActiveClientInfo[3]);
            DEBUG("%hhu.%hhu.%hhu.%hhu", G_ActiveClientInfo[0], G_ActiveClientInfo[1], G_ActiveClientInfo[2], G_ActiveClientInfo[3]);
        } else if (!strcmp(AmpUT_NetCtrl_MsgQ_Buf[0], "RFCOMM")) {
            sscanf(AmpUT_NetCtrl_MsgQ_Buf[1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &G_ActiveClientInfo[0], &G_ActiveClientInfo[1], &G_ActiveClientInfo[2],
                                                                                                                                &G_ActiveClientInfo[3], &G_ActiveClientInfo[4], &G_ActiveClientInfo[5]);
            DEBUG("%hhx:%hhx:%hhx:%hhx:%hhx:hhx", G_ActiveClientInfo[0], G_ActiveClientInfo[1], G_ActiveClientInfo[2],
                                                                                           G_ActiveClientInfo[3], G_ActiveClientInfo[4], G_ActiveClientInfo[5]);
        } else {
            AmbaPrint("No support this transport protocol type");
        }

        AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
        AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_SET_CLNT_INFO));

        JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
        if (JsonStr != NULL) {
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);

        }
        break;
    case AMBA_LS:
    case AMBA_CD:
    case AMBA_SET_WIFI_SETTING:
        if (G_ActiveToken != Token) {
            RetVal = -1;
            AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
            AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(JsonCmdMsg.MsgId));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);
        } else {
            if (strstr(G_JsonStr, "param")) {
                AmpJson_GetStringByKey(JsonObj, "param", (char*)&AmpUT_NetCtrl_MsgQ_Buf[0], AMPUT_NETCTRL_JSONSTROBJ_SIZE);
            }
            if (G_Task_Init) {
               RetVal =  AmbaKAL_MsgQueueSend(&AmpUT_NetCtrl_Cmd_MsgQ, &CmdMsg, AMBA_KAL_NO_WAIT);
            } else {
                AmbaPrint("cmd task and msgQ are not initiated. G_Task_Init = %d", G_Task_Init);
            }
        }
        break;
    case AMBA_PWD:
    case AMBA_WIFI_RESTART:
    case AMBA_WIFI_START:
    case AMBA_WIFI_STOP:
    case AMBA_GET_WIFI_SETTING:
    case AMBA_GET_WIFI_STATUS:
        if (G_ActiveToken != Token) {
            RetVal = -1;
            AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
            AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(JsonCmdMsg.MsgId));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);

         } else {
            DEBUG("send 0x%x cmd to Linux Cmd handler", AmpUT_NetCtrl_MsgQ_Digit[1]);

            if (G_Task_Init) {
                AmbaKAL_MsgQueueSend(&AmpUT_NetCtrl_Cmd_MsgQ, &CmdMsg, AMBA_KAL_NO_WAIT);
            } else {
                AmbaPrint("cmd task and msgQ are not initiated. G_Task_Init = %d", G_Task_Init);
            }
        }
        break;
    /* The system must set client info with AMBA_SET_CLNT_INFO first before data transmission.*/
    case AMBA_GET_FILE:
        if (G_ActiveToken != Token) {
            RetVal = -1;
            AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
            AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_GET_FILE));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);

        } else {
            AmpJson_GetStringByKey(JsonObj, "param", AmpUT_NetCtrl_MsgQ_Buf[0], AMPUT_NETCTRL_JSONSTROBJ_SIZE);
            AmpJson_GetIntegerByKey(JsonObj, "offset", (int*)&AmpUT_NetCtrl_MsgQ_Digit[4]);
            AmpJson_GetIntegerByKey(JsonObj, "fetch_size", (int*)&AmpUT_NetCtrl_MsgQ_Digit[5]);

            if (G_Task_Init) {
                AmbaKAL_MsgQueueSend(&AmpUT_NetCtrl_Cmd_MsgQ, &CmdMsg, AMBA_KAL_NO_WAIT);
            } else {
                AmbaPrint("cmd task and msgQ are not initiated. G_Task_Init = %d", G_Task_Init);
            }
        }
        break;
    /* The system must set client info with AMBA_SET_CLNT_INFO first before data transmission.*/
    case AMBA_PUT_FILE:
        if (G_ActiveToken != Token) {
            RetVal = -1;
            AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
            AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_PUT_FILE));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);

        } else {
            AmpJson_GetStringByKey(JsonObj, "param", AmpUT_NetCtrl_MsgQ_Buf[0], AMPUT_NETCTRL_JSONSTROBJ_SIZE);
            AmpJson_GetStringByKey(JsonObj, "md5sum", AmpUT_NetCtrl_MsgQ_Buf[1], AMPUT_NETCTRL_JSONSTROBJ_SIZE);
            AmpJson_GetIntegerByKey(JsonObj, "offset", (int*)&AmpUT_NetCtrl_MsgQ_Digit[4]);
            AmpJson_GetIntegerByKey(JsonObj, "size", (int*)&AmpUT_NetCtrl_MsgQ_Digit[5]);

            if (G_Task_Init) {
                AmbaKAL_MsgQueueSend(&AmpUT_NetCtrl_Cmd_MsgQ, &CmdMsg, AMBA_KAL_NO_WAIT);
            } else {
                AmbaPrint("cmd task and msgQ are not initiated. G_Task_Init = %d", G_Task_Init);
            }
        }
        break;
    /* The system must set client info with AMBA_SET_CLNT_INFO first before data transmission.*/
    case AMBA_GET_THUMB:
        if (G_ActiveToken != Token) {
            RetVal = -1;
            AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
            AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_GET_THUMB));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);

        } else {
            AmpJson_GetStringByKey(JsonObj, "param", AmpUT_NetCtrl_MsgQ_Buf[0], AMPUT_NETCTRL_JSONSTROBJ_SIZE);
            AmpJson_GetStringByKey(JsonObj, "type", AmpUT_NetCtrl_MsgQ_Buf[1], AMPUT_NETCTRL_JSONSTROBJ_SIZE);

            if (G_Task_Init) {
                AmbaKAL_MsgQueueSend(&AmpUT_NetCtrl_Cmd_MsgQ, &CmdMsg, AMBA_KAL_NO_WAIT);
            } else {
                AmbaPrint("cmd task and msgQ are not initiated. G_Task_Init = %d", G_Task_Init);
            }
        }
        break;
    case AMBA_CANCEL_FILE_XFER:
        if (G_ActiveToken != Token) {
            RetVal = -1;
            AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
            AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_CANCEL_FILE_XFER));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);

       } else {
            AmpJson_GetStringByKey(JsonObj, "param", AmpUT_NetCtrl_MsgQ_Buf[0], 512);
            memcpy(CancelTrans.ClientInfo, G_ActiveClientInfo, sizeof(CancelTrans.ClientInfo));
            strcpy(CancelTrans.TransportType, G_ActiveTransportType);

            if (strstr(G_JsonStr, "sent_size")) {
                AmpJson_GetIntegerByKey(JsonObj, "sent_size", (int*)&AmpUT_NetCtrl_MsgQ_Digit[4]);
                CancelTrans.MsgId = DATASVC_PUT_FILE;
                CancelTrans.SentSize = AmpUT_NetCtrl_MsgQ_Digit[4];
                AmpNetCtrl_DataSvc_CancelDataTrans(&DataHdlrInfo,&CancelTrans, &CancelResult);
                AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(CancelResult.Rval));
                AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_CANCEL_FILE_XFER));
            } else {
                CancelTrans.MsgId = DATASVC_GET_FILE;
                CancelTrans.SentSize = 0;
                AmpNetCtrl_DataSvc_CancelDataTrans(&DataHdlrInfo, &CancelTrans, &CancelResult);
                AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(CancelResult.Rval));
                AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_CANCEL_FILE_XFER));
                AmpJson_AddObject(JsonResObj, "transferred_size", AmpJson_CreateIntegerObject(CancelResult.TransSize));
            }

            JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
            if (JsonStr != NULL) {
                SendInfo.Size =  strlen((char*)JsonStr->JsonString);
                SendInfo.MemAddr = JsonStr->JsonString;
                AmpNetCtrl_Send(hdlrInfo, &SendInfo);
            }
        }
        break;
    default:
        if (G_ActiveToken != Token) {
            RetVal = -1;
            AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
            AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(JsonCmdMsg.MsgId));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(hdlrInfo, &SendInfo);
         } else {
            AmbaPrint("%s %d: 0x%x is not support cmd", __FUNCTION__, __LINE__, AmpUT_NetCtrl_MsgQ_Digit[1]);
            AmpJson_AddObject(JsonResObj, "rval", AmpJson_CreateIntegerObject(RetVal));
            AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AmpUT_NetCtrl_MsgQ_Digit[1]));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
            if (JsonStr != NULL) {
                SendInfo.Size =  strlen((char*)JsonStr->JsonString);
                SendInfo.MemAddr = JsonStr->JsonString;
                AmpNetCtrl_Send(hdlrInfo, &SendInfo);

             }
        }
        break;
    }

    AmpJson_FreeJsonObject(JsonObj);
    if (JsonStr != NULL) {
        AmpJson_FreeJsonString(JsonStr);
    }
    AmpJson_FreeJsonObject(JsonArrObj->ObjValue.Array);
    AmpJson_FreeJsonObject(JsonResObj);

    return RetVal;
}

int AmpUT_NetCtrl_RecvDataReqStatus(AMP_NETCTRL_DATASVC_HDLR_INFO_s *dataHdlrInfo, AMP_NETCTRL_DATASVC_STATUS_s *statusFromDataSvc)
{
    int RetVal = 0;
    AMP_JSON_OBJECT_s *JsonResObj = NULL;
    AMP_JSON_STRING_s *JsonStr = NULL;
    char Md5sum[36] = {0};
    AMP_NETCTRL_HDLR_INFO_s HdlrInfo = {0};
    AMP_NETCTRL_SEND_INFO_s SendInfo = {0};

    //TODO: Please fill the following items based on your case
    if (G_SendType == SEND_WIFI_CMD) {
         HdlrInfo.InstTypeId = G_HdlrInfo.InstTypeId;
         HdlrInfo.Hdlr = G_HdlrInfo.Hdlr;
    }

#if defined (BT_AMBA_APP)
    else { //G_SendType = SEND_BLE_CMD
         HdlrInfo.InstTypeId = G_HdlrInfoBle.InstTypeId;
         HdlrInfo.Hdlr = G_HdlrInfoBle.Hdlr;
    }
#endif
    if(statusFromDataSvc == NULL) {
        AmbaPrint("%s : pStatus is NULL", __FUNCTION__);
        return -1;
    }
    JsonResObj = AmpJson_CreateObject();
    memcpy(Md5sum, statusFromDataSvc->Md5sum, sizeof(statusFromDataSvc->Md5sum));
    Md5sum[sizeof(statusFromDataSvc->Md5sum)] = 0x00;
    AmpJson_AddObject(JsonResObj, "token", AmpJson_CreateIntegerObject(G_ClientToken));;
    AmpJson_AddObject(JsonResObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_NOTIFICATION));
    if ((statusFromDataSvc->Type >= DATASVC_GET_FILE) && (statusFromDataSvc->Type <= DATASVC_GET_ENCAP_THUMB)) {
        if (statusFromDataSvc->Status == 0) {
            AmpJson_AddObject(JsonResObj, "type", AmpJson_CreateStringObject("get_file_complete"));
        } else {
            AmpJson_AddObject(JsonResObj, "type", AmpJson_CreateStringObject("get_file_fail"));
        }
    } else if (statusFromDataSvc->Type == DATASVC_PUT_FILE) {
        if (statusFromDataSvc->Status == 0) {
            AmpJson_AddObject(JsonResObj, "type", AmpJson_CreateStringObject("put_file_complete"));
        } else {
            AmpJson_AddObject(JsonResObj, "type", AmpJson_CreateStringObject("put_file_fail"));
        }
    } else {
        AmbaPrint("not support type = %u", statusFromDataSvc->Type);
    }

    AmpJson_AddObject(JsonResObj, "param", AmpJson_CreateIntegerObject(statusFromDataSvc->Bytes));
    AmpJson_AddObject(JsonResObj, "md5sum", AmpJson_CreateStringObject(Md5sum));

    JsonStr = AmpJson_JsonObjectToJsonString(JsonResObj);
    AmbaPrint("%d : %s", G_ClientToken, JsonStr->JsonString);
    if (JsonStr != NULL) {
            SendInfo.ClientId = G_ActiveClientId;
            SendInfo.Size =  strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
    }
    AmpJson_FreeJsonString(JsonStr);
    AmpJson_FreeJsonObject(JsonResObj);

    DEBUG("send status to RTOS App handler");
    return RetVal;
}

static void AmpUT_NetCtrl_CmdTask(UINT32 taskInfo)
{
    AMPUT_NETCTRL_CMD_MESSAGE_s CmdMsg;
    int RetVal =0, ShiftAddr = 0, RetStatus = 0, FileSize = 0;
    UINT32 *MsgDigit = NULL;
    MsgQBuf MsgBuf = NULL;
    AMP_JSON_STRING_s *JsonStr = NULL;
    AMP_JSON_OBJECT_s *JsonObj = NULL;
    AMP_NETCTRL_LNXCMD_RESULT_s LnxResult;
    AMP_NETCTRL_DATASVC_DATA_s DataReq;
    AMP_NETCTRL_DATASVC_RESULT_s DataResult;
    void *RawBaseAddr = NULL, *AlignedBaseAddr = NULL;
    AMBA_FS_FILE *FileThumb = NULL;
    AMBA_FS_STAT FileStat;
    WCHAR UniFilename[512];
    WCHAR FileMode[4] = {'r','\0'};
    char asciiFileMode[4] = "r";
    AMP_NETCTRL_HDLR_INFO_s HdlrInfo = {0};
    AMP_NETCTRL_DATASVC_HDLR_INFO_s DataHdlrInfo = {0};
    AMP_NETCTRL_DATASVC_DEST_INFO_s DestDataHdlrInfo = {0};
    AMP_NETCTRL_SEND_INFO_s SendInfo = {0};

        RetVal = AmpUtil_GetAlignedPool(&G_MMPL, &AlignedBaseAddr, &RawBaseAddr, AMPUT_NETCTRL_DATASVC_THUMB_SIZE, 32);
    if (RetVal != 0) {
        AmbaPrint("allocate memory fail, %s:%u", __FUNCTION__, __LINE__);
    }

    while (G_CmdTask) {
        AmbaKAL_MsgQueueReceive(&AmpUT_NetCtrl_Cmd_MsgQ, &CmdMsg, AMBA_KAL_WAIT_FOREVER);
        DEBUG("MsgId=0x%x MsgData[0]=0x%x MsgData[1]=0x%x", CmdMsg.MsgId, CmdMsg.MsgData[0], CmdMsg.MsgData[1]);
        MsgDigit =  (UINT32*)CmdMsg.MsgData[0];
        MsgBuf = (MsgQBuf)CmdMsg.MsgData[1];
        JsonObj = AmpJson_CreateObject();
        if (JsonObj == NULL) {
            AmbaPrint("%s : JsonObj is NULL", __FUNCTION__);
            continue;
        }

        RetVal = 0;
        memset(G_RetBuf, 0x00, AMPUT_NETCTRL_RETBUF_SIZE);
        memset(&DataReq, 0x00, sizeof(AMP_NETCTRL_DATASVC_DATA_s));
        memset(&DataResult, 0x00, sizeof(AMP_NETCTRL_DATASVC_RESULT_s));

        //TODO: Please fill the following items based on your case
        if (G_SendType == SEND_WIFI_CMD) {
            HdlrInfo.InstTypeId = G_HdlrInfo.InstTypeId;
            HdlrInfo.Hdlr = G_HdlrInfo.Hdlr;
        }
#if defined (BT_AMBA_APP)
        else { //G_SendType = SEND_BLE_CMD
            HdlrInfo.InstTypeId = G_HdlrInfoBle.InstTypeId;
            HdlrInfo.Hdlr = G_HdlrInfoBle.Hdlr;

#if defined DEBUG_UT_NETCTRL
AmbaPrint("DEBUG: %s,%d, HdlrInfo.InstTypeId:%d, HdlrInfo.Hdlr:0x%x,",__FUNCTION__,__LINE__,HdlrInfo.InstTypeId, HdlrInfo.Hdlr);
AmbaPrint("DEBUG: %s,%d, G_HdlrInfoBle.InstTypeId:%d, G_HdlrInfoBle.Hdlr:0x%x,",__FUNCTION__,__LINE__,G_HdlrInfoBle.InstTypeId, G_HdlrInfoBle.Hdlr);
#endif
       }
#endif

        if(G_SupportLnxCmdFlag == NOT_SUPPORT_LNX_SHELL_CMD) {
            AmbaPrint("%s,%d, This command is not supported!",__FUNCTION__,__LINE__);
            //TODO: return result
        }
        //Please fill the following items based on your case
        //Use the wifi to be the example
        if (G_SendDataType == SEND_WIFI_DATA) {
            DataHdlrInfo.InstTypeId = G_DataHdlrInfo.InstTypeId;
            DataHdlrInfo.Hdlr = G_DataHdlrInfo.Hdlr;
        }
        switch (MsgDigit[1]) {
        case AMBA_LS:
            sprintf(G_RetBuf, "{\"rval\":0,\"msg_id\":%d,\"listing\":", AMBA_LS);
            ShiftAddr = strlen(G_RetBuf);

            RetVal = AmpNetCtrl_LnxLs(MsgBuf[0], G_RetBuf+ShiftAddr, AMPUT_NETCTRL_RETBUF_SIZE-ShiftAddr, &LnxResult);
            if (RetVal == 0) {
                if (LnxResult.Rval == 0) {
                    strcat(G_RetBuf, "}");
                    SendInfo.ClientId = MsgDigit[2];
                    SendInfo.Size = strlen(G_RetBuf)+1;
                    SendInfo.MemAddr = G_RetBuf;
                    AmpNetCtrl_Send(&HdlrInfo, &SendInfo);


                } else {
                    AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(LnxResult.Rval));
                    AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_LS));

                    JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);
                    SendInfo.ClientId = MsgDigit[2];
                    SendInfo.Size = strlen((char*)JsonStr->JsonString);
                    SendInfo.MemAddr = JsonStr->JsonString;
                    AmpNetCtrl_Send(&HdlrInfo, &SendInfo);

               }
            } else {
                    AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(RetVal));
                    AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_LS));

                    JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);

                    SendInfo.ClientId = MsgDigit[2];
                    SendInfo.Size = strlen((char*)JsonStr->JsonString);
                    SendInfo.MemAddr = JsonStr->JsonString;
                    AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
            }
            break;
         case AMBA_CD:
            RetVal = AmpNetCtrl_LnxCd(MsgBuf[0], G_RetBuf, AMPUT_NETCTRL_RETBUF_SIZE, &LnxResult);
            if (RetVal == 0) {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(LnxResult.Rval));
                AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_CD));
                if (LnxResult.Rval == 0) {
                    if (sizeof(JsonObj->ObjValue.String) >= LnxResult.ResultSize) {
                        AmpJson_AddObject(JsonObj, "pwd", AmpJson_CreateStringObject(G_RetBuf));
                    } else {
                        AmbaPrint("size of G_RetBuf is larger than size of json object value");
                    }
                }
            } else {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(RetVal));
                AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_CD));
            }

            JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);
            SendInfo.ClientId = MsgDigit[2];
            SendInfo.Size = strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
            break;
        case AMBA_PWD:
            RetVal = AmpNetCtrl_LnxPwd(G_RetBuf, AMPUT_NETCTRL_RETBUF_SIZE, &LnxResult);
            if (RetVal == 0) {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(LnxResult.Rval));
                AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_PWD));
                if (LnxResult.Rval == 0) {
                    if (sizeof(JsonObj->ObjValue.String) >= LnxResult.ResultSize) {
                        AmpJson_AddObject(JsonObj, "pwd", AmpJson_CreateStringObject(G_RetBuf));
                    } else {
                        AmbaPrint("size of G_RetBuf is larger than size of json object value");
                    }
                }
            } else {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(RetVal));
                AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_PWD));
            }

            JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);
            SendInfo.ClientId = MsgDigit[2];
            SendInfo.Size = strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
            break;
        case AMBA_SET_WIFI_SETTING:
            RetVal = AmpNetCtrl_SetWifiConfig(MsgBuf[0], NULL, &LnxResult);
            if (RetVal == 0) {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(LnxResult.Rval));
            } else {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(RetVal));
            }
            AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_SET_WIFI_SETTING));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);
            SendInfo.ClientId = MsgDigit[2];
            SendInfo.Size = strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
            break;
        case AMBA_GET_WIFI_SETTING:
            sprintf(G_RetBuf, "{\"rval\":0,\"msg_id\":%d,\"param\":", AMBA_GET_WIFI_SETTING);
            ShiftAddr = strlen(G_RetBuf);

            RetVal = AmpNetCtrl_GetWifiConfig(NULL, G_RetBuf+ShiftAddr, AMPUT_NETCTRL_RETBUF_SIZE-ShiftAddr, &LnxResult);
            if (RetVal == 0) {
                if (LnxResult.Rval == 0) {
                    strcat(G_RetBuf, "}");
                    SendInfo.ClientId = MsgDigit[2];
                    SendInfo.Size = strlen(G_RetBuf)+1;
                    SendInfo.MemAddr = G_RetBuf;
                    AmpNetCtrl_Send(&HdlrInfo, &SendInfo);

                } else {
                    AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(LnxResult.Rval));
                    AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_GET_WIFI_SETTING));

                    JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);
                    SendInfo.ClientId = MsgDigit[2];
                    SendInfo.Size = strlen((char*)JsonStr->JsonString);
                    SendInfo.MemAddr = JsonStr->JsonString;
                    AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
               }
            } else {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(RetVal));
                AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_GET_WIFI_SETTING));

                JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);
                SendInfo.ClientId = MsgDigit[2];
                SendInfo.Size = strlen((char*)JsonStr->JsonString);
                SendInfo.MemAddr = JsonStr->JsonString;
                AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
            }
            break;
        case AMBA_WIFI_RESTART:
            RetVal = AmpNetCtrl_WifiRestart(NULL);
            if (RetVal == 0) {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(0));
            } else {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(-1));
            }
            AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_WIFI_RESTART));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);
            SendInfo.ClientId = MsgDigit[2];
            SendInfo.Size = strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
           break;
        case AMBA_WIFI_STOP:
            RetVal = AmpNetCtrl_WifiStop(NULL);
            if (RetVal == 0) {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(0));
            } else {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(-1));
            }
            AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_WIFI_STOP));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);
            SendInfo.ClientId = MsgDigit[2];
            SendInfo.Size = strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
           break;
        case AMBA_WIFI_START:
            RetVal = AmpNetCtrl_WifiStart(NULL);
            if (RetVal == 0) {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(0));
            } else {
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(-1));
            }
            AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_WIFI_START));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);

            SendInfo.ClientId = MsgDigit[2];
            SendInfo.Size = strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
            break;
        case AMBA_GET_WIFI_STATUS:
            sprintf(G_RetBuf, "{\"rval\":0,\"msg_id\":%d,\"param\":", AMBA_GET_WIFI_STATUS);
            ShiftAddr = strlen(G_RetBuf);

            RetVal = AmpNetCtrl_GetWifiStatus(NULL, G_RetBuf+ShiftAddr, AMPUT_NETCTRL_RETBUF_SIZE-ShiftAddr, &LnxResult);
            if (RetVal == 0) {
                if (LnxResult.Rval == 0) {
                    strcat(G_RetBuf, "}");
                    SendInfo.ClientId = MsgDigit[2];
                    SendInfo.Size = strlen(G_RetBuf)+1;
                    SendInfo.MemAddr = G_RetBuf;
                    AmpNetCtrl_Send(&HdlrInfo, &SendInfo);

                } else {
                    AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(LnxResult.Rval));
                    AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_GET_WIFI_STATUS));

                    JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);

                    SendInfo.ClientId = MsgDigit[2];
                    SendInfo.Size = strlen((char*)JsonStr->JsonString);
                    SendInfo.MemAddr = JsonStr->JsonString;
                    AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
               }
            } else {
                    AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(RetVal));
                    AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_GET_WIFI_STATUS));

                    JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);

                    SendInfo.ClientId = MsgDigit[2];
                    SendInfo.Size = strlen((char*)JsonStr->JsonString);
                    SendInfo.MemAddr = JsonStr->JsonString;
                    AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
            }
            break;
        case AMBA_GET_FILE:
            DataReq.MsgId = DATASVC_GET_FILE;
            strcpy(DataReq.Filepath, MsgBuf[0]);
            DataReq.Offset = MsgDigit[4];
            DataReq.FetchFilesize = MsgDigit[5];
            DataReq.BufferAddr = NULL;
            memcpy(DataReq.ClientInfo, G_ActiveClientInfo, sizeof(DataReq.ClientInfo));
            strcpy(DataReq.TransportType, G_ActiveTransportType);

            memcpy(DestDataHdlrInfo.ClientInfo, G_ActiveClientInfo, sizeof(DestDataHdlrInfo.ClientInfo));
            strcpy(DestDataHdlrInfo.TransportType, G_ActiveTransportType);
            AmpNetCtrl_DataSvc_GetStatus(&DataHdlrInfo, &DestDataHdlrInfo, &RetStatus);
            if (RetStatus != 0) {
                AmbaPrint("Data server is not ready to get file, ret = %d", RetStatus);
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(RetStatus));
            } else {
                AmpNetCtrl_DataSvc_Send(&DataHdlrInfo,&DataReq, &DataResult);
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(DataResult.Rval));
            }

            AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_GET_FILE));
            AmpJson_AddObject(JsonObj, "rem_size", AmpJson_CreateIntegerObject(DataResult.RemSize));
            AmpJson_AddObject(JsonObj, "size", AmpJson_CreateIntegerObject(DataResult.TotalFilesize));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);

            SendInfo.ClientId = MsgDigit[2];
            SendInfo.Size = strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
            break;

       case AMBA_PUT_FILE:
            DataReq.MsgId = DATASVC_PUT_FILE;
            strcpy(DataReq.Filepath, MsgBuf[0]);
            memcpy(DataReq.Md5sum, MsgBuf[1], sizeof(DataReq.Md5sum));
            DataReq.Offset = MsgDigit[4];
            DataReq.FetchFilesize = MsgDigit[5];
            DataReq.BufferAddr = NULL;
            memcpy(DataReq.ClientInfo, G_ActiveClientInfo, sizeof(DataReq.ClientInfo));
            strcpy(DataReq.TransportType, G_ActiveTransportType);

            memcpy(DestDataHdlrInfo.ClientInfo, G_ActiveClientInfo, sizeof(DestDataHdlrInfo.ClientInfo));
            strcpy(DestDataHdlrInfo.TransportType, G_ActiveTransportType);
            AmpNetCtrl_DataSvc_GetStatus(&DataHdlrInfo, &DestDataHdlrInfo, &RetStatus);
            if (RetStatus != 0) {
                AmbaPrint("Data server is not ready to put file, ret = %d", RetStatus);
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(RetStatus));
            } else {
                AmpNetCtrl_DataSvc_Send(&DataHdlrInfo,&DataReq, &DataResult);
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(DataResult.Rval));
            }

            AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_PUT_FILE));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);
            SendInfo.ClientId = MsgDigit[2];
            SendInfo.Size = strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
            break;

       case AMBA_GET_THUMB:
            AmpUT_NetCtrl_Transform_FilePath(MsgBuf[0]);

            if (AmbaFS_GetCodeMode() == AMBA_FS_UNICODE) {
                AmbaUtility_Ascii2Unicode(MsgBuf[0], UniFilename);
                AmbaFS_Stat((char const *)UniFilename, &FileStat);
                FileThumb = AmbaFS_fopen((char const *)UniFilename, (char const *)FileMode);
            } else {
                AmbaFS_Stat(MsgBuf[0], &FileStat);
                FileThumb = AmbaFS_fopen(MsgBuf[0], asciiFileMode);
            }

            if (FileThumb != NULL) {
                FileSize = FileStat.Size;
                if (FileSize <= AMPUT_NETCTRL_DATASVC_THUMB_SIZE) {
                    AmbaFS_fread(AlignedBaseAddr, FileSize, 1, FileThumb);
                    AmbaFS_fclose(FileThumb);

                    DataReq.MsgId = DATASVC_GET_THUMB;
                    DataReq.Offset = MsgDigit[4];
                    DataReq.FetchFilesize = FileSize;
                    DataReq.BufferAddr = AlignedBaseAddr;
                    memcpy(DataReq.ClientInfo, G_ActiveClientInfo, sizeof(DataReq.ClientInfo));
                    strcpy(DataReq.TransportType, G_ActiveTransportType);

                    memcpy(DestDataHdlrInfo.ClientInfo, G_ActiveClientInfo, sizeof(DestDataHdlrInfo.ClientInfo));
                    strcpy(DestDataHdlrInfo.TransportType, G_ActiveTransportType);
                    AmpNetCtrl_DataSvc_GetStatus(&DataHdlrInfo, &DestDataHdlrInfo, &RetStatus);
                    if (RetStatus != 0) {
                        AmbaPrint("Data server is not ready to get thumb, ret = %d", RetStatus);
                        AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(RetStatus));
                    } else {
                        AmpNetCtrl_DataSvc_Send(&DataHdlrInfo,&DataReq, &DataResult);
                        AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(DataResult.Rval));
                    }
                } else {
                    AmbaPrint("file size is too large = %d\n", FileSize);
                    AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(-1));
                }
            } else {
                AmbaPrint("fopen fail");
                AmpJson_AddObject(JsonObj, "rval", AmpJson_CreateIntegerObject(-1));
            }

            AmpJson_AddObject(JsonObj, "msg_id", AmpJson_CreateIntegerObject(AMBA_GET_THUMB));
            AmpJson_AddObject(JsonObj, "thumb_file", AmpJson_CreateStringObject(MsgBuf[0]));
            AmpJson_AddObject(JsonObj, "size", AmpJson_CreateIntegerObject(FileSize));
            AmpJson_AddObject(JsonObj, "type", AmpJson_CreateStringObject("thumb"));
            AmpJson_AddObject(JsonObj, "md5sum", AmpJson_CreateStringObject("19d26795e5ca6cd5d2e7f1b1f5f15f62"));

            JsonStr = AmpJson_JsonObjectToJsonString(JsonObj);

            SendInfo.ClientId = MsgDigit[2];
            SendInfo.Size = strlen((char*)JsonStr->JsonString);
            SendInfo.MemAddr = JsonStr->JsonString;
            AmpNetCtrl_Send(&HdlrInfo, &SendInfo);
            break;
        default:
            break;
        }

        if (JsonStr != NULL) {
            AmpJson_FreeJsonString(JsonStr);
            JsonStr = NULL;
        }
        AmpJson_FreeJsonObject(JsonObj);
    }

    RetVal = AmbaKAL_BytePoolFree(RawBaseAddr);
    if (RetVal != 0) {
        AmbaPrint("%s,%d, Fail to free the heap: RetVal:%d",__FUNCTION__,__LINE__,RetVal);
    } else {
        AlignedBaseAddr = NULL;
        RawBaseAddr = NULL;
    }

}

int AmpUT_NetCtrl_InitCmdTask(void)
{
    int RetVal = 0;

    /* Create RTOS cmd handler message queue */
    RetVal = AmbaKAL_MsgQueueCreate(&AmpUT_NetCtrl_Cmd_MsgQ,
                                                                  AmpUT_NetCtrl_CmdMsgQ_Stack,
                                                                  sizeof(AMPUT_NETCTRL_CMD_MESSAGE_s),
                                                                  AMPUT_NETCTRL_CMDHNDLR_MSGQUEUE_SIZE);
    if(RetVal != OK ) {
        AmbaPrint("Amp_NetCtrl_MsgQueue create MsgQ Fail, ReturnValue = %d", RetVal);
        return -1;
    }

    /* Create RTOS cmd handler task */
    G_CmdTask = 1;
    RetVal = AmbaKAL_TaskCreate(&AmpUT_NetCtrl_Cmd_Task,
                                                        "DummyNetCtrl",
                                                        AMPUT_NETCTRL_CMDHNDLR_PRIORITY,
                                                        AmpUT_NetCtrl_CmdTask,
                                                        0x0,
                                                        (void *)AmpUT_NetCtrl_CmdHndlr_Stack,
                                                        AMPUT_NETCTRL_CMDTASK_SIZE,
                                                        AMBA_KAL_AUTO_START);
    if (RetVal != OK) {
        AmbaPrint("AmbaKAL_TaskCreate create MsgQ Fail, ReturnValue = %d", RetVal);
        return -1;
    }
    G_Task_Init = 1;
    AmbaPrint("Cmd task and msgQ are initiated. G_Task_Init = %d", G_Task_Init);

    return RetVal;
}

int AmpUT_NetCtrl_TerminateCmdTask(void)
{
    int RetVal = 0;

    // terminate RTOS cmd handler task //
    RetVal = AmbaKAL_TaskTerminate(&AmpUT_NetCtrl_Cmd_Task);
    if (RetVal != OK) {
        AmbaPrint("AmbaKAL_TaskCreate Delete MsgQ Fail, ReturnValue = %d", RetVal);
        return -1;
    }

    //AmbaLink_Free(G_RetBuf);
    //G_RetBuf = NULL;
    if ((G_RetBuf != NULL) && (G_RetBufRawBaseAddr != NULL)) {
       RetVal = AmbaKAL_BytePoolFree(G_RetBufRawBaseAddr);
       if (RetVal != 0) {
          AmbaPrint("%s,%d, Fail to free the heap: RetVal:%d",__FUNCTION__,__LINE__,RetVal);
       } else {
          G_RetBuf = NULL;
          G_RetBufRawBaseAddr = NULL;
       }
    }else {
      AmbaPrint("%s,%d, Fail to free the heap: The pointer is NULL",__FUNCTION__,__LINE__);
      return -1;
    }
    G_CmdTask = 0;

    // Delete RTOS cmd handler message queue //
   RetVal = AmbaKAL_MsgQueueDelete(&AmpUT_NetCtrl_Cmd_MsgQ);
    if(RetVal != OK ) {
        AmbaPrint("Amp_NetCtrl_MsgQueue Delete MsgQ Fail, ReturnValue = %d", RetVal);
        return -1;
    }

    // Delete RTOS cmd handler task //
    RetVal = AmbaKAL_TaskDelete(&AmpUT_NetCtrl_Cmd_Task);
    if (RetVal != OK) {
        AmbaPrint("AmbaKAL_TaskCreate Delete MsgQ Fail, ReturnValue = %d", RetVal);
        return -1;
    }

    G_Task_Init = 0;
    DEBUG("Cmd task and msgQ are deleted. G_Task_Init = %d", G_Task_Init);

    memset(&AmpUT_NetCtrl_Cmd_MsgQ, '0', sizeof(AMBA_KAL_MSG_QUEUE_t));
    memset(&AmpUT_NetCtrl_Cmd_Task, '0', sizeof(AMBA_KAL_TASK_t));
    memset(&AmpUT_NetCtrl_CmdHndlr_Stack, '0', sizeof(char)*AMPUT_NETCTRL_CMDTASK_SIZE);
    memset(&AmpUT_NetCtrl_CmdMsgQ_Stack,'0', sizeof(char)*sizeof(AMPUT_NETCTRL_CMD_MESSAGE_s)*AMPUT_NETCTRL_CMDHNDLR_MSGQUEUE_SIZE);

//static char AmpUT_NetCtrl_MsgQ_Buf[4][AMPUT_NETCTRL_JSONSTROBJ_SIZE];
//typedef char (*MsgQBuf)[AMPUT_NETCTRL_JSONSTROBJ_SIZE];


    return RetVal;
}

/**
 * AmpUT rt_send function.
 */
static int AmpUT_NetCtrl_rt_send(char *string)
{
    int RetVal = -1;
    UINT32 StrSize = 0;
    RT_NETCTRL_CMD_s Cmd;
    AMBA_IPC_SVC_RESULT_s Result;

    if (string == NULL)
        return RetVal;

    StrSize = strlen(string) + 1;
    strcpy(Cmd.Param, string);
    Cmd.ParamSize = StrSize;
    Cmd.ClientId = 1;

    G_SendType = SEND_WIFI_CMD;

#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d", __FUNCTION__, __LINE__);
#endif
    RetVal = RT_NetCtrl_Send_Svc(&Cmd, &Result);

#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d,RetVal:%d", __FUNCTION__, __LINE__,RetVal);
#endif
    if (RetVal < 0) {
        AmbaPrint("%s,%d,Error Code:%d", __FUNCTION__,__LINE__,RetVal);
    }
    return RetVal;
}
/**
 * BLE functions
 */
//------------------------------------------------------------------------//
#if defined (BT_AMBA_APP)
/**
 * AmpUT Init GATT1 function.
 */

static void AmpUT_NetCtrl_InitGATT1(void)
{
    int i;

    memset(&G_GATT1, 0, sizeof(G_GATT1));

    G_GATT1.primary.len              = LEN_UUID128;
    G_GATT1.primary.uuid.uuid128[15] = 0x00;
    G_GATT1.primary.uuid.uuid128[14] = 0x00;
    G_GATT1.primary.uuid.uuid128[13] = 0x00;
    G_GATT1.primary.uuid.uuid128[12] = 0x00;
    G_GATT1.primary.uuid.uuid128[11] = 0x61;     // 'a'
    G_GATT1.primary.uuid.uuid128[10] = 0x6D;     // 'm'
    G_GATT1.primary.uuid.uuid128[9]  = 0x62;     // 'b'
    G_GATT1.primary.uuid.uuid128[8]  = 0x61;     // 'a'
    G_GATT1.primary.uuid.uuid128[7]  = 0x5F;     // '_'
    G_GATT1.primary.uuid.uuid128[6]  = 0x69;     // 'i'
    G_GATT1.primary.uuid.uuid128[5]  = 0x64;     // 'd'
    G_GATT1.primary.uuid.uuid128[4]  = 0x5F;     // '_'
    G_GATT1.primary.uuid.uuid128[3]  = 0x62;     // 'b'
    G_GATT1.primary.uuid.uuid128[2]  = 0x61;     // 'a'
    G_GATT1.primary.uuid.uuid128[1]  = 0x73;     // 's'
    G_GATT1.primary.uuid.uuid128[0]  = 0x65;     // 'e'

#define NUM_ATTR (4)
    G_GATT1.num_char = NUM_ATTR;
    G_GATT1.chars    = G_GATTChar1;
    for (i = 0; i < NUM_ATTR; i++) {
        memcpy(&(G_GATTChar1[i].characteristic), &(G_GATT1.primary), sizeof(bt_uuid_t));

        G_GATTChar1[i].characteristic.uuid.uuid128[15] = 0x11 * (i + 1);
        G_GATTChar1[i].characteristic.uuid.uuid128[14] = 0x11 * (i + 1);
        G_GATTChar1[i].characteristic.uuid.uuid128[13] = 0x11 * (i + 1);
        G_GATTChar1[i].characteristic.uuid.uuid128[12] = 0x11 * (i + 1);

        if (i == G_BleWriteId) {
            G_GATTChar1[i].char_permission = (BT_GATT_PERM_WRITE);
            G_GATTChar1[i].char_property = (BT_GATT_CHAR_PROP_WRITE);
        } else { //ex. i == G_BleReadId
            G_GATTChar1[i].char_permission = (BT_GATT_PERM_READ);
            G_GATTChar1[i].char_property = (BT_GATT_CHAR_PROP_READ |
                                            BT_GATT_CHAR_PROP_NOTIFY |
                                            BT_GATT_CHAR_PROP_INDICATE);
        }

        G_GATTChar1[i].descriptor.len = LEN_UUID16;
        /* cf. https://developer.bluetooth.org/gatt/descriptors/Pages/DescriptorsHomePage.aspx */
        G_GATTChar1[i].descriptor.uuid.uuid16 = 0x2902;
        G_GATTChar1[i].desc_permission        = (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE);
    }

    /* Fill up Call back function. */
    G_GATT1.cbf_gatt_reqRead  = NULL;
    G_GATT1.cbf_gatt_reqWrite = AmpNetBleCfg_GattWriteHandler;
}

/* Callback function by HAL when connection status changed. */
static void AmpUT_NetCtrl_GattConnection(amba_bt_status_t conn)
{
    if (conn == AMBA_BT_STATUS_CONNECTED) {
        AmbaPrint("%s: BT Connected", __func__);
    } else {
        AmbaPrint("%s: BT Disconnected", __func__);
    }
}

static int AmpUT_NetCtrl_InitBleHdlr(void)
{
    AMP_NETBLECFG_BLEINFO_s BleInfo;
    amba_bt_if_t BleInterfaceIns;
    gatt_service_t BleGattServiceIns;

    int RetVal = 0;
    AMP_NETCTRL_CFG_s ClntCfg;

    //Get the default BLE information in the module "BleNetworkHandler" if need
    memset(&BleInfo,'0',sizeof(AMP_NETBLECFG_BLEINFO_s));
    memset(&BleInterfaceIns, '0', sizeof(amba_bt_if_t));
    memset(&BleGattServiceIns, '0', sizeof(gatt_service_t));
    BleInfo.BleInterfaceIns = &BleInterfaceIns;
    BleInfo.BleGattServiceIns = &BleGattServiceIns;
    RetVal = AmpNetBleCfg_GetBleInfo(&BleInfo);

    if (RetVal < 0) {
        return RetVal;
    }
    //TODO: do something with the current BLE information which is gotten from the module "BleNetworkHandler" if need
    //Set the BLE information to the BLE module in the framework (BleNetworkHandler)
    //and use the default value of  MsgBufSize
    BleInfo.BleWriteCharId = G_BleWriteId;
    BleInfo.BleReadCharId = G_BleReadId;
    BleInfo.BleInterfaceIns = G_pBtIns;
    BleInfo.BleGattServiceIns = &G_GATT1;
    RetVal = AmpNetBleCfg_SetBleInfo(&BleInfo);
    if (RetVal < 0) {
        return RetVal;
    }
    //Init the NetCtrl  in the pure_RTOS for BLE ------------------------------------------------:
    //for command server: Recv/Send commands by using the BLE
    //step1. Get the default config
    RetVal = AmpNetCtrl_GetInstanceDefaultCfg(&ClntCfg);
    if (RetVal < 0) {
        //error handler
        AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, RetVal);
        return RetVal;
    }
    //step2. Create the instance based on the speified case
    //       such as send commands by usign the BLE

    ClntCfg.InstTypeId = INST_TYPE_BLE;
    RetVal = AmpNetCtrl_CreateInstance(&ClntCfg, &G_HdlrInfoBle);
    if (RetVal < 0) {
        //error handler
        AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, RetVal);
        return RetVal;
    }
    //step3. Register the receiving function
    RetVal = AmpNetCtrl_RegCmdRcvCB(&G_HdlrInfoBle, AmpUT_NetCtrl_RecvCmd);
    if (RetVal < 0) {
        //error handler
        AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, RetVal);
        return RetVal;
    }
   //step4. Init the server of linux commands if the linux is existed
   //Please design the section based on your case
    if (G_SendDataType == SEND_WIFI_DATA) {
        RetVal = AmpNetCtrl_LnxCmdInit();
        if (RetVal < 0) {
            //error handler
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, RetVal);
            return RetVal;
        }
        G_SupportLnxCmdFlag = SUPPORT_LNX_SHELL_CMD;
    }
    //Case1: for the data server: Recv/Send the data by using the wifi/bt in the linux
    if (G_SendDataType == SEND_WIFI_DATA) {
        //step1. Get the default config
        RetVal = AmpNetCtrl_DataSvc_GetInstanceDefaultCfg(&ClntCfg);
        if (RetVal < 0) {
            //error handler
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, RetVal);
            return RetVal;
        }
#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d, Rval: %d, ClntCfg.InstTypeId:%d", __FUNCTION__, __LINE__, RetVal, ClntCfg.InstTypeId);
#endif
    //step2. Create the instance based on the speified case
    //       such as send the data by usign the wifi in the linux
        RetVal = AmpNetCtrl_DataSvc_CreateInstance(&ClntCfg, &G_DataHdlrInfo);
        if (RetVal < 0) {
            //error handler
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, RetVal);
            return RetVal;
        }

#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d, Rval: %d, G_DataHdlrInfo.Hdlr:%x, G_DataHdlrInfo.InstTypeId:%d", __FUNCTION__, __LINE__, RetVal, G_DataHdlrInfo.Hdlr, G_DataHdlrInfo.InstTypeId);
#endif
    //step3. Register the receiving function
        RetVal = AmpNetCtrl_DataSvc_RegRecvCb(&G_DataHdlrInfo, AmpUT_NetCtrl_RecvDataReqStatus);
        if (RetVal < 0) {
            //error handler
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, RetVal);
            return RetVal;
        }
    } //if (G_SendDataType == SEND_WIFI_DATA) {

   //-------------------------------------------------------------------------------------------//
   return RetVal;
}

/**
 * The task is used to init the BLE.
 */
void AmpUT_NetCtrl_InitBleTask(UINT32 exinf)
{
    amba_bt_conf_t bt_conf = { "uITRON_BT_05", { 0x05, 0x33, 0x03, 0x13, 0x20, 0x05 } };
    amba_bt_errno_t err;
    int RetVal = 0;
    G_pBtIns = amba_bt_getIf();
    K_ASSERT(G_pBtIns != NULL);
    memset(&(G_pBtIns->bt_conf), 0, sizeof(amba_bt_conf_t));
    memcpy(G_pBtIns->bt_conf.name, bt_conf.name, sizeof(bt_conf.name));
    memcpy(G_pBtIns->bt_conf.bdaddr, bt_conf.bdaddr, sizeof(bt_conf.bdaddr));
    G_pBtIns->bt_conf.max_dl_pri = 50;
    G_pBtIns->bt_conf.max_wk_pri = 100;

    /* Init tasks of BT stack (tasks) and enabled. */
    G_pBtIns->bt_init(exinf);     // exinf for BT stack log enable or disable

   /* Wait... Initialization of BT stack will spend few seconds.
     Do something.*/

   /* Initialization of GATT service data. */
    AmpUT_NetCtrl_InitGATT1();
    G_pBtIns->gatt_if->cbf_connection = AmpUT_NetCtrl_GattConnection;

    while (G_pBtIns->getStatus() != AMBA_BT_STATUS_ENABLED) {
        AmbaKAL_TaskSleep(10);
    }

    err = G_pBtIns->gatt_if->init();
    if (err != AMBA_BT_EOK) {
        return;
    }
    err = G_pBtIns->gatt_if->addConf(&G_GATT1);
    if (err != AMBA_BT_EOK) {
        return;
    }
    RetVal = AmpUT_NetCtrl_InitBleHdlr();
    if (RetVal < 0) {
        return;
    }
}

static int AmpUT_NetCtrl_InitBle(void)
{
    int log_en = 0;
    int err;
    int RetVal = 0;

    if (1 == G_BleInited) {
        RetVal = AmpUT_NetCtrl_InitBleHdlr();
        if (RetVal < 0) {
            AmbaPrint("%s,%d, Fail to init the Ble Hdlr! err code: %d ", __FUNCTION__, __LINE__, RetVal);
        }
        return RetVal;
    }else{
        G_BleInited = 1;
    }
    err = AmbaKAL_TaskCreate(&G_BtTsk, "AmpUT_BLEInit", BT_TSK_PRIORITY,AmpUT_NetCtrl_InitBleTask,
                             log_en, G_BtStack, BT_TSK_STACK_SIZE, AMBA_KAL_AUTO_START);
    K_ASSERT(err == OK);
    G_BtTskID = G_BtTsk.tx_thread_id;
    return RetVal;
}

/**
 * @brief This function is used to test to receive the command from ble and
 *  then dispatch to the rtos-App with the client information.
 *
 * @param [in] the command information
 * @return 0 - OK, negative value - AMP_NETCTRL_ERROR_e
 */
extern int AmpBleNetworkHandler_BleMsgTest(const void *data);

/**
 * AmpUT rt_ble_send function.
 */
static int AmpUT_NetCtrl_rt_ble_send(char *string)
{
    int RetVal = -1;
    UINT32 StrSize = 0;
    AMP_NETCTRL_CMD_s Cmd;

    if (string == NULL)
        return RetVal;

    StrSize = strlen(string) + 1;
    strcpy(Cmd.Param, string);
    Cmd.ParamSize = StrSize;
    Cmd.ClientId = 1;

    G_SendType = SEND_BLE_CMD;

#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d, Rval: %d", __FUNCTION__, __LINE__, RetVal);
#endif
    RetVal = AmpBleNetworkHandler_BleMsgTest(string);
#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d, Rval: %d", __FUNCTION__, __LINE__, RetVal);
#endif
    if (RetVal < 0)
    {
        AmbaPrint("%s,%d,Error Code:%d", __FUNCTION__,__LINE__,RetVal);
    }

   return RetVal;
}
//------------------------------------------------------------------------//
#endif
/**
 * AmpUT lu_dara_request function.
 */
static int AmpUT_NetCtrl_DataReq_Send(char *filePath, char *clntIp, int msgId)
{
    int RetVal = -1, RetStatus = 0;
    AMP_NETCTRL_DATASVC_DATA_s DataReq = {0};
    AMP_NETCTRL_DATASVC_RESULT_s DataResult = {0};
    AMP_NETCTRL_DATASVC_DEST_INFO_s DetailDataClntInfo = {0};

    DataReq.MsgId = msgId;
    strcpy(DataReq.Filepath, filePath);
    DataReq.Offset = 0;
    DataReq.FetchFilesize = 0;
    memcpy(DataReq.ClientInfo, G_ActiveClientInfo, sizeof(DataReq.ClientInfo));
    strcpy(DataReq.TransportType, G_ActiveTransportType);

    memcpy(DetailDataClntInfo.ClientInfo, G_ActiveClientInfo, sizeof(DetailDataClntInfo.ClientInfo));
    memcpy(DetailDataClntInfo.TransportType, G_ActiveTransportType, sizeof(DetailDataClntInfo.TransportType));
#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d, DetailDataClntInfo.ClientInfo:%s, DetailDataClntInfo.TransportType:%s", __FUNCTION__, __LINE__, DetailDataClntInfo.ClientInfo, DetailDataClntInfo.TransportType);
#endif

    RetVal = AmpNetCtrl_DataSvc_GetStatus(&G_DataHdlrInfo, &DetailDataClntInfo, &RetStatus);
    if (RetVal < 0) {
        AmbaPrint("%s,%d,Error code:%d", __FUNCTION__, __LINE__, RetVal);
        return RetVal;
    }
    if (RetStatus != 0) {
        AmbaPrint("Data server is busy, ret = %d", RetStatus);
    }
    RetVal = AmpNetCtrl_DataSvc_Send(&G_DataHdlrInfo, &DataReq, &DataResult);
    if (RetVal < 0) {
        AmbaPrint("%s,%d,Error code:%d", __FUNCTION__, __LINE__, RetVal);
        return RetVal;
    }
    DEBUG("rval = %d, remsize = %llu, filesize = %llu", DataResult.Rval, DataResult.RemSize, DataResult.TotalFilesize);
    return RetVal;
}

/**
 * Unitest initiate function.
 */

static int AmpUT_NetCtrl_init(void)
{
    AMP_NETCTRL_INIT_CFG_s InitNetCtrlCfg;
    AMP_NETCTRL_CFG_s ClntCfg;
    int Rval = 0;
    //init AmpJson lib
    do {
        AMP_JSON_INIT_CFG_s InitJsonCfg;
        AmpJson_GetInitDefaultCfg(&InitJsonCfg);
        InitJsonCfg.MemPoolAddr = (void*)&G_MMPL;
        AmpJson_Init(&InitJsonCfg);
    } while (0);
    AmpNetCtrl_GetInitDefaultCfg(&InitNetCtrlCfg);
    InitNetCtrlCfg.MemPoolAddr = (void*)&G_MMPL;
    InitNetCtrlCfg.MemPoolSize = (4096+2048)*2;
    Rval = AmpNetCtrl_Init(&InitNetCtrlCfg);
    if (Rval < 0) {
        AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, Rval);
        return Rval;
    }

    if (G_RetBuf == NULL) {
        //G_RetBuf = AmbaLink_Malloc(1024);
        Rval = AmpUtil_GetAlignedPool((void*)&G_MMPL,(void*)&G_RetBuf, (void*)&G_RetBufRawBaseAddr, 1024, 32);
        if (Rval != 0) {
           AmbaPrint("%s,%d,Fail to allocate the heap for the returned buffer:Rval:%d", __FUNCTION__,__LINE__,Rval);
        }
    }
    G_RetBuf = (char *)G_RetBuf;
    //Case1: for command server: Recv/Send commands by using the wifi/bt in the linux
    if (G_SendType == SEND_WIFI_CMD) {
        //step1. Get the default config
        Rval = AmpNetCtrl_GetInstanceDefaultCfg(&ClntCfg);
        if (Rval < 0) {
            //error handler
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, Rval);
            return Rval;
        }
#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d, Rval: %d, ClntCfg.InstTypeId:%d", __FUNCTION__, __LINE__, Rval, ClntCfg.InstTypeId);
#endif
        //step2. Create the instance based on the speified case
        //      such as send commands by usign the wifi in the linux
        //use the default case: (wifi/bt in the linux)

        Rval = AmpNetCtrl_CreateInstance(&ClntCfg, &G_HdlrInfo);
        if (Rval < 0) {
            //error handler
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, Rval);
            return Rval;
        }

#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d, Rval: %d, G_HdlrInfo.Hdlr:%x, G_HdlrInfo.InstTypeId:%d", __FUNCTION__, __LINE__, Rval, G_HdlrInfo.Hdlr, G_HdlrInfo.InstTypeId);
#endif
        //step3. Register the receiving function
        Rval = AmpNetCtrl_RegCmdRcvCB(&G_HdlrInfo, AmpUT_NetCtrl_RecvCmd);
        if (Rval < 0) {
            //error handler
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, Rval);
            return Rval;
        }
        //step4. Init the server of linux commands
        Rval = AmpNetCtrl_LnxCmdInit();
        if (Rval < 0) {
            //error handler
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, Rval);
            return Rval;
        }
        G_SupportLnxCmdFlag = SUPPORT_LNX_SHELL_CMD;

    } //if (G_SendType == SEND_WIFI_CMD) {

#if defined (BT_AMBA_APP)
    //Case2: for command server: Recv/Send commands by using the BLE
    if (G_SendType == SEND_BLE_CMD) {
        //step1. Init the Ble
        Rval = AmpUT_NetCtrl_InitBle();
        if (Rval < 0) {
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, Rval);
            return Rval;
        }
    }//if (G_SendType == SEND_BLE_CMD) {
#endif //defined (BT_AMBA_APP)

    //Case1: for the data server: Recv/Send the data by using the wifi/bt in the linux
    if ((G_SendDataType == SEND_WIFI_DATA) && (G_SendType == SEND_WIFI_CMD)) {
        //step1. Get the default config
        Rval = AmpNetCtrl_DataSvc_GetInstanceDefaultCfg(&ClntCfg);
        if (Rval < 0) {
            //error handler
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, Rval);
            return Rval;
        }
#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d, Rval: %d, ClntCfg.InstTypeId:%d", __FUNCTION__, __LINE__, Rval, ClntCfg.InstTypeId);
#endif
    //step2. Create the instance based on the speified case
    //       such as send the data by usign the wifi in the linux
        Rval = AmpNetCtrl_DataSvc_CreateInstance(&ClntCfg, &G_DataHdlrInfo);
        if (Rval < 0) {
            //error handler
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, Rval);
            return Rval;
        }

#if defined DEBUG_UT_NETCTRL
    AmbaPrint("%s,%d, Rval: %d, G_DataHdlrInfo.Hdlr:%x, G_DataHdlrInfo.InstTypeId:%d", __FUNCTION__, __LINE__, Rval, G_DataHdlrInfo.Hdlr, G_DataHdlrInfo.InstTypeId);
#endif
    //step3. Register the receiving function
        Rval = AmpNetCtrl_DataSvc_RegRecvCb(&G_DataHdlrInfo, AmpUT_NetCtrl_RecvDataReqStatus);
        if (Rval < 0) {
            //error handler
            AmbaPrint("%s,%d, Error code: %d", __FUNCTION__, __LINE__, Rval);
            return Rval;
        }
    } //if (G_SendDataType == SEND_WIFI_DATA) {
    AmpUT_NetCtrl_InitCmdTask();
    return 0;
}

static int AmpUT_NetCtrl_end(void)
{
    int RetVal = 0;
    RetVal = AmpUT_NetCtrl_TerminateCmdTask();
    if (RetVal < 0) {
        return RetVal;
    }
    //release  the resource
    /*if (G_SendType == SEND_WIFI_CMD) {
AmbaPrint("%s,%d",__FUNCTION__,__LINE__);
        RetVal = AmpNetCtrl_DeleteInstance(&G_HdlrInfo);
        if (RetVal < 0) {
            return RetVal;
        }
    }
#if defined (BT_AMBA_APP)
    if (G_SendType == SEND_BLE_CMD) {
        RetVal = AmpNetCtrl_DeleteInstance(&G_HdlrInfoBle);
        if (RetVal < 0) {
            return RetVal;
        }
   }
#endif
    if (G_SendDataType == SEND_WIFI_DATA) {
        RetVal = AmpNetCtrl_DataSvc_DeleteInstance(&G_DataHdlrInfo);
        if (RetVal < 0) {
            return RetVal;
        }
    }*/
    //release the framework
    RetVal =  AmpNetCtrl_Release();
    if (RetVal < 0) {
        return RetVal;
    }

    //Clean all of global variables

    memset(&G_JsonStr,'0',(sizeof(char)*MAX_JSON_STR_LEN));
    G_CurJsonStrLen = 0;

    G_SendType = SEND_WIFI_CMD;
    G_SendDataType = SEND_WIFI_CMD;

    memset(&G_HdlrInfo,'0', sizeof(AMP_NETCTRL_HDLR_INFO_s));
#if defined (BT_AMBA_APP)
    memset(&G_HdlrInfoBle, '0', sizeof(AMP_NETCTRL_HDLR_INFO_s));
#endif
    memset(&G_DataHdlrInfo, '0', sizeof(AMP_NETCTRL_DATASVC_HDLR_INFO_s));

    G_ActiveToken = 0;
    G_ClientToken = 0;
    G_ActiveClientId = 0;
    memset(&G_ActiveClientInfo,'0',sizeof(unsigned char)*128);
    memset(&G_ActiveTransportType,'0',sizeof(char)*8);

    G_SupportLnxCmdFlag = NOT_SUPPORT_LNX_SHELL_CMD;
    //TODO: Clean the JSON library
    return RetVal;
}


static int AmpUT_NetCtrl_Change_Set_Mode(char *type )
{
    if ((strcmp(type,"wifi_cmd_data")) == 0) {
        G_SendType = SEND_WIFI_CMD;
        G_SendDataType = SEND_WIFI_DATA;
    }
#if defined (BT_AMBA_APP)
    else if ((strcmp(type,"ble_cmd")) == 0) {
        G_SendType = SEND_BLE_CMD;
        G_SendDataType = SEND_DATA_NULL;
    }
    else if ((strcmp(type,"ble_cmd_wifi_data")) == 0) {
        G_SendType = SEND_BLE_CMD;
        G_SendDataType = SEND_WIFI_DATA;
    }
#endif
    AmbaPrint("Current set mode: 0:ble, 1:wifi, 2:null -> cmd_mode:%d, data_mode:%d",G_SendType, G_SendDataType);
    return 0;
}
static void AmpUT_NetCtrl_usage(char *paramName)
{
    AmbaPrint("usage: %s <CMD>", paramName);
//#if defined (BT_AMBA_APP)
    //AmbaPrint("\t ble_init : Please initialize the BLE before do 't netctrl init' if you want to use the BLE");
//#endif
    AmbaPrint("\t init");
    AmbaPrint("\t rt_send '{\"token\":123,\"msg_id\":513}' ");
#if defined (BT_AMBA_APP)
    //AmbaPrint("\t rt_ble_send '{\"token\":123,\"msg_id\":513}' ");
    AmbaPrint("\t set_mode '\"wifi_cmd_data\"' \\ '\"ble_cmd\"' \\ '\"ble_cmd_wifi_data\"'");
#else
    AmbaPrint("\t set_mode '\"wifi_cmd_data\"'");
#endif
    AmbaPrint("\t lnx_ls");
    AmbaPrint("\t lnx_cd tmp");
    AmbaPrint("\t lnx_pwd");
    AmbaPrint("\t wifi_setcfg AP_SSID=AMBA\\nLOCAL_IP=192.128.42.2\\nESSID=amba_TEST\\nAP_CHANNEL=4");
    AmbaPrint("\t wifi_getcfg");
    AmbaPrint("\t wifi_st");
    AmbaPrint("\t wifi_start");
    AmbaPrint("\t wifi_stop");
    AmbaPrint("\t wifi_restart");
    AmbaPrint("\t datareq_send /tmp/fuse_d/dcim/amba_0001.mp4 192.168.42.5 1");
    AmbaPrint("\t end\n\n");
}

/**
 * NetCtrl test function.
 */
static int AmpUT_NetCtrl_Test(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int RetVal = -1;
    AMP_NETCTRL_LNXCMD_RESULT_s LnxResult;

    if (strcmp(argv[1], "init") == 0) { //init netctrl module
        RetVal = AmpUT_NetCtrl_init();
    } else if (strcmp(argv[1], "rt_send") == 0) { //simulate send cmd from lnx to rtos
        RetVal = AmpUT_NetCtrl_rt_send(argv[2]);
#if defined (BT_AMBA_APP)
    } else if (strcmp(argv[1], "ble_init") == 0) { //init the BLE
        RetVal = AmpUT_NetCtrl_InitBle();
    //} else if (strcmp(argv[1], "rt_ble_send") == 0) { //simulate to send cmd by using BLE
    //    RetVal = AmpUT_NetCtrl_rt_ble_send(argv[2]);
#endif
    } else if (strcmp(argv[1], "set_mode") == 0) { //change the send type
        RetVal = AmpUT_NetCtrl_Change_Set_Mode(argv[2]);
    } else if (strcmp(argv[1], "lnx_ls") == 0) {
        RetVal = AmpNetCtrl_LnxLs(argv[2], G_RetBuf, (UINT32)1000, &LnxResult);
    } else if (strcmp(argv[1], "lnx_cd") == 0) {
        RetVal = AmpNetCtrl_LnxCd(argv[2], G_RetBuf, (UINT32)1000, &LnxResult);
    } else if (strcmp(argv[1], "lnx_pwd") == 0) {
        RetVal = AmpNetCtrl_LnxPwd(G_RetBuf, (UINT32)1000, &LnxResult);
    } else if (strcmp(argv[1], "wifi_setcfg") == 0) {
        RetVal = AmpNetCtrl_SetWifiConfig(argv[2], NULL, &LnxResult);
    } else if (strcmp(argv[1], "wifi_getcfg") == 0) {
        RetVal = AmpNetCtrl_GetWifiConfig(NULL, G_RetBuf, (UINT32)1000, &LnxResult);
    } else if (strcmp(argv[1], "wifi_st") == 0) {
        RetVal = AmpNetCtrl_GetWifiStatus(NULL, G_RetBuf, (UINT32)1000, &LnxResult);
    } else if (strcmp(argv[1], "wifi_start") == 0) {
        RetVal = AmpNetCtrl_WifiStart(NULL);
    } else if (strcmp(argv[1], "wifi_stop") == 0) {
        RetVal = AmpNetCtrl_WifiStop(NULL);
    } else if (strcmp(argv[1], "wifi_restart") == 0) {
        RetVal = AmpNetCtrl_WifiRestart(NULL);
    } else if (strcmp(argv[1], "datareq_send") == 0) { //simulate send datareq from rtos to lnx
        RetVal = AmpUT_NetCtrl_DataReq_Send(argv[2], argv[3], atoi(argv[4]));
    } else if (strcmp(argv[1], "end") == 0) { //release all resource and exit
        RetVal = AmpUT_NetCtrl_end();
    } else { //show usage
        AmbaPrint("%s: Not support yet!", argv[1]);
        AmpUT_NetCtrl_usage(argv[0]);
    }

    return RetVal;
}

/**
 * NetFifo test command hooking function.
 */
int AmpUT_NetCtrl_TestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("netctrl", AmpUT_NetCtrl_Test);

    return AMP_OK;
}

