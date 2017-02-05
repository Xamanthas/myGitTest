#ifndef __H_APPLIB_NET_IPC_TEST__
#define __H_APPLIB_NET_IPC_TEST__

int AppLibNetIPCTest_Add(void *pMemoryPool);
int AppLibNetIPC_RegisterService(char *ServiceName);
int AppLibNetIPC_ClientExec2(char *Command);
int AppLibNetIPC_UsbPtClnt(UINT32 Id, void *Buf, UINT32 BufSize, UINT32 Len);


#endif /* __H_APPLIB_NET_IPC_TEST__ */

