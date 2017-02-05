/*-------------------------------------------------------------------------------------------------------------------*\
*  @FileName       :: AmbaUSB_Hosst_API.h
*
*  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
*
*                     No part of this file may be reproduced, stored in a retrieval system,
*                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
*                     recording, or otherwise, without the prior consent of Ambarella Corporation.
*
*  @Description    :: AmbaUSB.lib external API.
*
*  @History        ::
*      Date        Name        Comments
*      03/13/2013  ycliao      Created
*
\*-----------------------------------------------------------------------------------------------*/

#ifndef AMBAUSB_HOST_API_H
#define AMBAUSB_HOST_API_H

#include "AmbaDataType.h"
#include "AmbaKAL.h"
#include "AmbaCardManager.h"
#include "StdUSB.h"
#include "ux_api.h"
#include "ux_host_class_pima.h"
#include "ux_hcd_ehci.h"

/*-----------------------------------------------------------------------------------------------*\
 * Define USB Host IRQ Onwer
\*-----------------------------------------------------------------------------------------------*/

typedef enum _USB_HOST_IRQ_OWNER_e_ {
	USB_HOST_IRQ_RTOS = 0,
	USB_HOST_IRQ_LINUX
} USB_HOST_IRQ_OWNER_e;

/*-----------------------------------------------------------------------------------------------*\
 * Define USB CLASS
\*-----------------------------------------------------------------------------------------------*/

typedef enum _UHC_CLASS_e_ {
	UHC_CLASS_NONE,
	UHC_CLASS_HID,      /* HID */
	UHC_CLASS_STORAGE,  /* Storage */
	UHC_CLASS_MTP,      /* Mtp*/
	UHC_CLASS_VIDEO,    /* Video */
	UHC_CLASS_ISO,      /* Isochronous Test */
	UHC_CLASS_MIX_MSC_UVC,	/* enable both msc and uvc class*/
	UHC_CLASS_SIMPLE,       /* Simple class */
} UHC_CLASS_e;

/*-----------------------------------------------------------------------------------------------*\
 * Define USB Host system Init parameters
\*-----------------------------------------------------------------------------------------------*/

typedef struct _USB_HOST_SYSTEM_INIT_s_ {
	UINT8   *MemPoolPtr;                  // memory pool pointer
	UINT32  TotalMemSize;                 // Total memory size
	UINT32  EnumThreadStackSize;          // stack size of ux_host_stack_enum_thread
	UINT32  EnumThreadPriority;           // thread priority ux_host_stack_enum_thread
	UINT32  HcdThreadStackSize;           // stack size of ux_host_stack_hcd_thread
	UINT32  HcdThreadPriority;            // thread priority ux_host_stack_hcd_thread
	UINT32  EnumThreadAffinityMask;       // Task Affinity of ux_host_stack_enum_thread
	UINT32  HcdThreadAffinityMask;        // Task Affinity of ux_host_stack_hcd_thread
} USB_HOST_SYSTEM_INIT_s;

/*-----------------------------------------------------------------------------------------------*\
 * Define USB Host Class Hook parameters
\*-----------------------------------------------------------------------------------------------*/

typedef struct _USB_HOST_CLASS_INIT_s_ {
	UHC_CLASS_e classID;
	UINT32      ClassTaskStackSize;
	UINT32      ClassTaskPriority;
	UINT32      ClassTaskAffinityMask;
} USB_HOST_CLASS_INIT_s;

/*-----------------------------------------------------------------------------------------------*\
 * Define USB PORT OWNER
\*-----------------------------------------------------------------------------------------------*/

typedef enum _USB0_PORT_OWNER_e_ {
	UDC_OWN_PORT = (0x1 << 0),  // USB0 = Device
	UHC_OWN_PORT = (0x1 << 1),  // USB0 = Host
} USB0_PORT_OWNER_e;

/*-----------------------------------------------------------------------------------------------*\
 * Define USB Host Storage Class Media Information
\*-----------------------------------------------------------------------------------------------*/

typedef struct _USB_HOST_STORAGE_INFO_s_ {
	UINT32  present;
	INT32   format;
	UINT32  lun;
	UINT32  SectorSize;
	UINT32  lba;
	UINT32  wp;
} USB_HOST_STORAGE_INFO_s;

typedef struct UHC_TASKINFO_s_ {
	UINT32 Priority;
	UINT32 AffinityMask;
	UINT32 StackSize;
} UHC_TASKINFO_s;

typedef struct _USB_HOST_STORAGE_CB_s_ {
    void        (*MediaInsert)(void);
    void        (*MediaRemove)(void);
} USB_HOST_STORAGE_CB_s;

/*-----------------------------------------------------------------------------------------------*\
 * Define USB Host Storage Class Media Information
\*-----------------------------------------------------------------------------------------------*/
typedef struct _USB_HOST_MTP_CB_s_ {
    void        (*MediaInsert)(void);
    void        (*MediaRemove)(void);
} USB_HOST_MTP_CB_s;
/*-----------------------------------------------------------------------------------------------*\
 * Define USB Host Video Class structures
\*-----------------------------------------------------------------------------------------------*/

typedef struct UVCH_FRAME_INFO_S {
	ULONG			index;
	ULONG			width;
	ULONG			height;
	ULONG			min_bitrate;
	ULONG			max_bitrate;
	ULONG			max_buffer_size;
	ULONG			default_frame_interval;
	ULONG			interval_type;
	ULONG			*interval;
} UVCH_FRAME_INFO;

// For UVC after v1.5
typedef struct UVCH_H264_FRAME_INFO_S {
	ULONG			index;
	ULONG			width;
	ULONG			height;
	ULONG			SAR_width;
	ULONG			SAR_height;
	ULONG			profile;
	ULONG			level_idc;
	ULONG			support_usages;
	ULONG			capabilities;
	ULONG			SVC_capabilities;
	ULONG			MVC_capabilities;
	ULONG			min_bitrate;
	ULONG			max_bitrate;
	ULONG			default_frame_interval;
	ULONG			num_frame_interval;
	ULONG			*interval;
} UVCH_H264_FRAME_INFO;

typedef struct UVCH_FORMAT_INFO_S {
	ULONG				subtype;
	ULONG				index;
	ULONG				frame_count;
	UVCH_FRAME_INFO *frame;
} UVCH_FORMAT_INFO;

typedef struct UVCH_CONTROL_ITEM_STRUCT {
	UINT8  selector;
	UINT8  is_supported;
	UINT8  size;
	UINT8  rsvd;
	UINT32 buffer_size;
	UINT8 *max;
	UINT8 *min;
	UINT8 *def;
	UINT8 *cur;
} UVCH_CONTROL_ITEM;

typedef struct UVCH_DEVICE_INFO_STRUCT {
	ULONG  size;
	ULONG  it_id;
	ULONG  xu_id;
	ULONG  pu_id;
	ULONG  format_count;
	UVCH_FORMAT_INFO *format;
	/* UVC 1.5 */
	ULONG	eu_id;
	ULONG	uvc_ver;
	ULONG   device_id;
	USHORT  vendor_id;
	USHORT  product_id;
	ULONG   cur_altsetting;
} UVCH_DEVICE_INFO;

typedef struct _USB_HOST_UVC_CB_s_ {
    void        (*MediaInsert)(void);
    void        (*MediaRemove)(void);
} USB_HOST_UVC_CB_s;

typedef struct _USB_HOST_UVC_MULTI_CB_s_ {
	void        (*MediaInsert)(UINT32 id);
	void        (*MediaRemove)(UINT32 id);
} USB_HOST_UVC_MULTI_CB_s;

typedef struct _UVCH_ALTERNATE_SETTING_s_ {
	UINT32      AlternateSetting;
	UINT32      MaxPacketSize;
	UINT32      BandWidth;
} UVCH_ALTERNATE_SETTING_s;

typedef struct UVCX_VIDEO_CONFIG_STRUCT {
	ULONG   dwFrameInterval;
	ULONG   dwBitRate;
	USHORT  bmHints;
	USHORT  wConfigurationIndex;
	USHORT  wWidth;
	USHORT  wHeight;
	USHORT  wSliceUnits;
	USHORT  wSliceMode;
	USHORT  wProfile;
	USHORT  wIFramePeriod;
	USHORT  wEstimatedVideoDelay;
	USHORT  wEstimatedMaxConfigDelay;
	UCHAR   bUsageType;
	UCHAR   bRateControlMode;
	UCHAR   bTemporalScaleMode;
	UCHAR   bSpatialScaleMode;
	UCHAR   bSNRScaleMode;
	UCHAR   bStreamMuxOption;
	UCHAR   bStreamFormat;
	UCHAR   bEntropyCABAC;
	UCHAR   bTimestamp;
	UCHAR   bNumOfReorderFrames;
	UCHAR   bPreviewFlipped;
	UCHAR   bView;
	UCHAR   bReserved1;
	UCHAR   bReserved2;
	UCHAR   bStreamID;
	UCHAR   bSpatialLayerRatio;
	USHORT  wLeakyBucketSize;
} UVCX_VIDEO_CONFIG_S;

typedef enum _UVC_POWER_LINE_FREQUENCY_e_ {
	UVC_PLF_DISABLED = 0,
	UVC_PLF_50_HZ,
	UVC_PLF_60_HZ,
} UVC_POWER_LINE_FREQUENCY_e;

typedef enum _UVC_ANALOG_VIDEO_STANDARD_e_ {
	UVC_AVS_NONE = 0,
	UVC_AVS_NTSC_525_60,
	UVC_AVS_PAL_625_50,
	UVC_AVS_SECAM_625_50,
	UVC_AVS_NSC_625_50,
	UVC_AVS_PAL_525_60,
} UVC_ANALOG_VIDEO_STANDARD_e;

typedef enum _UVC_ANALOG_VIDEO_LOCK_e_ {
	UVC_AVL_LOCKED = 0,
	UVC_AVL_UNLOCKED,
} UVC_ANALOG_VIDEO_LOCK_e;

typedef struct _UVCH_CTRL_INFO_s_ {
    UINT32 max;
    UINT32 min;
    UINT32 cur;
} UVCH_CTRL_INFO_s;

/*-----------------------------------------------------------------------------------------------*\
 * Define USB Host Simple Class structures
\*-----------------------------------------------------------------------------------------------*/

typedef struct _USB_HOST_SIMPLE_CB_s_ {
    void        (*MediaInsert)(void);
    void        (*MediaRemove)(void);
} USB_HOST_SIMPLE_CB_s;

/*-----------------------------------------------------------------------------------------------*\
 * Define USB HCD Test Mode.
\*-----------------------------------------------------------------------------------------------*/

#define USBH_TEST_MODE_J_STATE                           1
#define USBH_TEST_MODE_K_STATE                           2
#define USBH_TEST_MODE_SE0_NAK                           3
#define USBH_TEST_MODE_PACKET                            4
#define USBH_TEST_MODE_FORCE_ENABLE                      5
#define USBH_TEST_MODE_SUSPEND                           6
#define USBH_TEST_MODE_RESUME                            7
#define USBH_TEST_MODE_ONE_STEP_GET_DESCRIPTOR           8
#define USBH_TEST_MODE_ONE_STEP_SET_FEATURE_STEP1        9
#define USBH_TEST_MODE_ONE_STEP_SET_FEATURE_STEP2        10
#define USBH_TEST_MODE_SUSPEND_RESUME                    0xFF         // No logner use
#define USBH_TEST_MODE_ONE_STEP_SET_FEATURE              0xFF         // No longer use


/*-----------------------------------------------------------------------------------------------*\
 * Define USB HCD Extern functions.
\*-----------------------------------------------------------------------------------------------*/

extern UINT32 AmbaUSB_Host_System_Init(USB_HOST_SYSTEM_INIT_s *config);
extern void   AmbaUSB_System_Host_SetUsbOwner(USB_HOST_IRQ_OWNER_e owner, int update);
extern UINT32 AmbaUSB_Host_System_SetMemoryPool(TX_BYTE_POOL *cached_pool, TX_BYTE_POOL *noncached_pool);
extern UINT32 AmbaUSB_Host_Class_Hook(USB_HOST_CLASS_INIT_s *config);
extern UINT32 AmbaUSB_Host_Class_UnHook(UHC_CLASS_e ClassID);
extern void   AmbaUSB_Host_Init_SwitchUsbOwner(USB0_PORT_OWNER_e owner);
extern void   AmbaUSB_Host_Init_EnableISR(void);
extern void   AmbaUSB_Host_Init_DisableISR(void);

// Mass Storage Class
extern int    AmbaUSB_Host_Class_Storage_FileCopy(char *pFilePathSource, char *pFilePathDestin);
extern void   AmbaUSB_Host_Class_Storage_Thrughput(int slot, UINT32* bs_multi, UINT32 align);
extern void   AmbaUSB_Host_Class_Storage_SetSlotInfo(UINT32 slot);
extern USB_HOST_STORAGE_INFO_s* AmbaUSB_Host_Class_Storage_GetStatus(int UsbMedia);
extern int    AmbaUSB_Host_Class_Storage_Read(int UsbMedia, UINT8 *pBuf, UINT32 Sec, UINT32 Secs);
extern int    AmbaUSB_Host_Class_Storage_Write(int UsbMedia, UINT8 *pBuf, UINT32 Sec, UINT32 Secs);
extern UINT32 AmbaUSB_Host_Class_Storage_RegisterCallback(USB_HOST_STORAGE_CB_s *cb);

// Ambarella Iso Class
extern UINT32 AmbaUSB_Host_Class_Iso_Read(UINT8 *data_pointer, UINT32 len, void (*func)(UINT32 rval));
extern UINT32 AmbaUSB_Host_Class_Iso_Write(UINT8 *data_pointer, UINT32 len, void (*func)(UINT32 rval));
extern UINT32 AmbaUSB_Host_Class_Iso_GetFrame(void);
extern UINT32 AmbaUSB_Host_System_SetEhciOCPolarity(UINT32 polarity);

// Ambarella Mtp Class
extern UINT32 AmbaUSBH_Mtp_RegisterCallback(USB_HOST_MTP_CB_s *cb);
extern UINT32 AmbaUSBH_Mtp_DeviceInfoGet(USBH_MTP_DEVICE *Device);
extern UINT32 AmbaUSBH_Mtp_DeviceReset(void);
extern UINT32 AmbaUSBH_Mtp_SessionOpen(USBH_MTP_SESSION *Session);
extern UINT32 AmbaUSBH_Mtp_SessionClose(USBH_MTP_SESSION *Session);
extern UINT32 AmbaUSBH_Mtp_StorageIdsGet(USBH_MTP_SESSION *Session, UINT32 *IdsArray, UINT32 ArrayLength);
extern UINT32 AmbaUSBH_Mtp_StorageInfoGet(USBH_MTP_SESSION *Session, UINT32 StorageId, USBH_MTP_STORAGE *Storage);
extern UINT32 AmbaUSBH_Mtp_ObjectNumberGet(USBH_MTP_SESSION *Session, UINT32 StorageId, UINT32 FormatCode, UINT32 Association);
extern UINT32 AmbaUSBH_Mtp_ObjectHandlesGet(USBH_MTP_SESSION *Session,
                                            UINT32 *ObjectHandlesArray,
                                            UINT32 ObjectHandlesLength,
                                            UINT32 StorageId,
                                            UINT32 FormatCode,
                                            UINT32 ObjectHandleAssociation);
extern UINT32 AmbaUSBH_Mtp_ObjectInfoGet(USBH_MTP_SESSION *Session, UINT32 ObjectHandle, USBH_MTP_OBJECT *Object);
extern UINT32 AmbaUSBH_Mtp_ObjectInfoSend(USBH_MTP_SESSION *Session, UINT32 StorageId, UINT32 ParentObjectId, USBH_MTP_OBJECT *Object);
extern UINT32 AmbaUSBH_Mtp_ObjectGet(USBH_MTP_SESSION *Session,
                                     UINT32 ObjectHandle,
                                     USBH_MTP_OBJECT *Object,
                                     UINT8 *ObjectBuf,
                                     UINT32 ObjectBufLen,
                                     UINT32 *ObjectActualLen);
extern UINT32 AmbaUSBH_Mtp_ObjectSend(USBH_MTP_SESSION *Session,
                                      USBH_MTP_OBJECT *Object,
                                      UINT8 *ObjectBuf,
                                      UINT32 ObjectBufLen);
extern UINT32 AmbaUSBH_Mtp_ObjectCopy(USBH_MTP_SESSION *Session,
                                      UINT32 ObjectHandle,
                                      UINT32 StorageId,
                                      UINT32 ParentObjectId);
extern UINT32 AmbaUSBH_Mtp_ObjectMove(USBH_MTP_SESSION *Session,
                                      UINT32 ObjectHandle,
                                      UINT32 StorageId,
                                      UINT32 ParentObjectId);
extern UINT32 AmbaUSBH_Mtp_ObjectDelete(USBH_MTP_SESSION *Session, UINT32 ObjectHandle, UINT32 FormatCode);
extern UINT32 AmbaUSBH_Mtp_ObjectTransferAbort(USBH_MTP_SESSION *Session,
                                               UINT32 ObjectHandle,
                                               USBH_MTP_OBJECT *Object);
extern UINT32 AmbaUSBH_Mtp_ThumbGet(USBH_MTP_SESSION *Session,
                                    UINT32 ObjectHandle,
                                    UINT32 FormatCode,
                                    USBH_MTP_OBJECT *Object,
                                    UINT8 *ObjectBuf,
                                    UINT32 ObjectBufLen,
                                    UINT32 *ObjectActualLen);

extern UINT32 AmbaUSBH_Mtp_VendorCommand(USBH_MTP_SESSION *Session, UINT32 *params);
extern UINT32 AmbaUSBH_Mtp_VendorCommandSend(USBH_MTP_SESSION *Ssession,
	                                         USBH_MTP_COMMAND * Command,
                                             USBH_MTP_VENDOR_PAYLOAD *VendorPayload,
                                             UINT8 *Buffer,
                                             UINT32 Length);
extern UINT32 AmbaUSBH_Mtp_VendorCommandGet(USBH_MTP_SESSION *Session,
	                                        USBH_MTP_COMMAND * Command,
		                                    USBH_MTP_VENDOR_PAYLOAD *VendorPayload,
		                                    UINT8 *Buffer,
		                                    UINT32 Length,
		                                    UINT32 *ActualLength);
extern VOID AmbaUSBH_Mtp_GetResponseCode(USBH_MTP_COMMAND* Response);
extern VOID AmbaUSBH_Mtp_SetResponseCode(USBH_MTP_COMMAND* Response);


// Video Class
extern void   AmbaUSBH_Uvc_SetCurValues(ULONG unit,ULONG selector,ULONG value);
extern void   AmbaUSBH_Uvc_StreamingStart(void);
extern void   AmbaUSBH_Uvc_StreamingStop(void);
extern void   AmbaUSBH_Uvc_PrintDeviceValues(void);
extern void   AmbaUSBH_Uvc_ProbeAndCommit(ULONG formatIndex,ULONG frameIndex,ULONG fps);
extern void   AmbaUSBH_Uvc_GetCurrentProbeSetting(UINT32 *formatIndex,UINT32 *frameIndex, UINT32 *fps);
extern UINT32 AmbaUSBH_Uvc_GetDeviceInfo(UVCH_DEVICE_INFO *info);
extern UINT32 AmbaUSBH_Uvc_GetPuControlInfo(UVCH_CONTROL_ITEM *item, UVC_PROCESSING_UNIT_CONTROL_SELECTOR selector);
extern UINT32 AmbaUSBH_Uvc_GetItControlInfo(UVCH_CONTROL_ITEM *item, UVC_CAMERA_TERMINAL_CONTROL_SELECTOR selector);
extern UINT   AmbaUSBH_Uvc_Read(UINT8 *buffer, UINT32 len, void (*complete_func)(UINT32 len));
extern UINT   AmbaUSBH_Uvc_ReadBlock(UX_EHCI_ISO_REQUEST *request, UINT32 PacketNum, void (*complete_func)(UX_EHCI_ISO_REQUEST *request));
extern UINT   AmbaUSBH_Uvc_ReadAppend(UX_EHCI_ISO_REQUEST *request, UINT32 PacketNum, void (*complete_func)(UX_EHCI_ISO_REQUEST *request));
extern UINT   AmbaUSBH_Uvc_GetMaxPacketSize(void);
extern UINT32 AmbaUSBH_Uvc_RegisterCallback(USB_HOST_UVC_CB_s *cb);

// redefine APIs. Applications should call APIs below in the future.
#define AmbaUSB_HostSystemSetup        AmbaUSB_Host_System_Init
#define AmbaUSBH_System_SetMemoryPool  AmbaUSB_Host_System_SetMemoryPool
#define AmbaUSBH_System_ClassHook      AmbaUSB_Host_Class_Hook
#define AmbaUSBH_System_ClassUnHook    AmbaUSB_Host_Class_UnHook
#define AmbaUSBH_System_SetPhy0Owner   AmbaUSB_Host_Init_SwitchUsbOwner
#define AmbaUSBH_System_SetUsbOwner    AmbaUSB_System_Host_SetUsbOwner
#define AmbaUSBH_System_EnableISR      AmbaUSB_Host_Init_EnableISR
#define AmbaUSBH_System_DisableISR     AmbaUSB_Host_Init_DisableISR
#define AmbaUSBH_System_SetEhciOCPolarity AmbaUSB_Host_System_SetEhciOCPolarity
#define AmbaUSBH_Storage_SetSlotInfo   AmbaUSB_Host_Class_Storage_SetSlotInfo
#define AmbaUSBH_Storage_GetStatus     AmbaUSB_Host_Class_Storage_GetStatus
#define AmbaUSBH_Storage_FileCopy      AmbaUSB_Host_Class_Storage_FileCopy
#define AmbaUSBH_Storage_Thrughput     AmbaUSB_Host_Class_Storage_Thrughput
#define AmbaUSBH_Storage_Read          AmbaUSB_Host_Class_Storage_Read
#define AmbaUSBH_Storage_Write         AmbaUSB_Host_Class_Storage_Write

// new API naming here
UINT32 AmbaUSBH_System_IsOhciEnabled(void);
void   AmbaUSBH_System_SetOhciEnabled(UINT32 value);
void   AmbaUSBH_System_SetIsoSlopDelay(UINT32 slop);
// MSC
UINT32 AmbaUSBH_Msc_SetMainTaskInfo(UHC_TASKINFO_s *info);
UINT32 AmbaUSBH_Msc_GetMainTaskInfo(UHC_TASKINFO_s *info);

// HID
UINT32 AmbaUSBH_Hid_SetMainTaskInfo(UHC_TASKINFO_s *info);
UINT32 AmbaUSBH_Hid_GetMainTaskInfo(UHC_TASKINFO_s *info);
void   AmbaUSBH_Sysem_SetTestMode(UINT32 TestMode);

// Video
UINT32 AmbaUSBH_Uvc_GetBacklightCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetBacklightCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetBrightnessCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetBrightnessCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetContrastCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetContrastCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetHueCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetHueCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetSaturationCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetSaturationCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetSharpnessCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetSharpnessCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetGammaCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetGammaCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetWBTemperatureCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetWBTemperatureCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetWBComponentCtrlInfo(UVCH_CTRL_INFO_s *InfoBlue, UVCH_CTRL_INFO_s *InfoRed);
UINT32 AmbaUSBH_Uvc_SetWBComponentCtrl(UINT16 blue, UINT16 red);
UINT32 AmbaUSBH_Uvc_GetGainCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetGainCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetCurLineFrequencyCtrl(void);
UINT32 AmbaUSBH_Uvc_SetLineFrequencyCtrl(UVC_POWER_LINE_FREQUENCY_e ctrl);
UINT32 AmbaUSBH_Uvc_GetHueAutoCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetHueAutoCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetWBTempAutoCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetWBTempAutoCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetWBCompAutoCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetWBCompAutoCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetDigitalMultiplierCtrlInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetDigitalMultiplierCtrl(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetDigitalMultiplierLimitInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetDigitalMultiplierLimit(UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetCurAnalogVideoStandard(void);
UINT32 AmbaUSBH_Uvc_SetAnalogVideoStandard(UVC_ANALOG_VIDEO_STANDARD_e ctrl);
UINT32 AmbaUSBH_Uvc_GetCurAnalogLockStatus(void);
UINT32 AmbaUSBH_Uvc_SetAnalogLockStatus(UVC_ANALOG_VIDEO_LOCK_e status);
UINT32 AmbaUSBH_Uvc_H264Support(void);
UINT32 AmbaUSBH_Uvc_H264Enable(UINT32 enable);
void   AmbaUSBH_Uvc_H264GetVideoConfigInfo(UVCX_VIDEO_CONFIG_S *ConfigMin, UVCX_VIDEO_CONFIG_S *ConfigMax, UVCX_VIDEO_CONFIG_S *ConfigCur);
void   AmbaUSBH_Uvc_H264SetVideoConfig(UVCX_VIDEO_CONFIG_S *config);
UINT32 AmbaUSBH_Uvc_H264RequestIdr(void);
UINT32 AmbaUSBH_Uvc_H264GetRateCtrlModeInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_H264SetRateCtrlMode(UINT32 mode);
UINT32 AmbaUSBH_Uvc_H264GetTemporalScaleModeInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_H264SetTemporalScaleMode(UINT32 mode);
UINT32 AmbaUSBH_Uvc_H264GetSpatialScaleModeInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_H264SetSpatialScaleMode(UINT32 mode);
UINT32 AmbaUSBH_Uvc_H264GetFrameRateConfigInfo(UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_H264SetFrameRateConfig(UINT32 FrameRate);

#define AmbaUSBH_Uvc_IsH264Supported         AmbaUSBH_Uvc_H264Support

// Simple
UINT32 AmbaUSBH_Simple_ControlRequest(UINT32 request, UINT32 type, UINT32 value, UINT32 index, UINT8 *buf, UINT32 length);
UINT32 AmbaUSBH_Simple_BulkRead(UINT8 *DataPtr, UINT32 len, UINT32 *ActualLen, UINT32 EpAddr, UINT32 timeout);
UINT32 AmbaUSBH_Simple_BulkWrite(UINT8 *DataPtr, UINT32 len, UINT32 *ActualLen, UINT32 EpAddr, UINT32 timeout);
UINT32 AmbaUSBH_Simple_GetBulkInEndpoint(UINT8 *buf, UINT32 *len);
UINT32 AmbaUSBH_Simple_GetBulkOutEndpoint(UINT8 *buf, UINT32 *len);
UINT32 AmbaUSBH_Simple_RegisterCallback(USB_HOST_SIMPLE_CB_s *cb);
void   AmbaUSBH_Simple_SetPidVid(UINT32 pid, UINT32 vid);

#define AmbaUSBH_Simple_GetBulkInEndpointList         AmbaUSBH_Simple_GetBulkInEndpoint
#define AmbaUSBH_Simple_GetBulkOutEndpointList         AmbaUSBH_Simple_GetBulkOutEndpoint

// Suggest to use AmbaUSBH_Uvc_GetxxxCtrlInfo to take place of below APIs.
UINT32 AmbaUSBH_Uvc_GetCurBacklightCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinBacklightCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxBacklightCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurBrightnessCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinBrightnessCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxBrightnessCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurContrastCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinContrastCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxContrastCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurHueCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinHueCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxHueCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurSaturationCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinSaturationCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxSaturationCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurSharpnessCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinSharpnessCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxSharpnessCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurGammaCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinGammaCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxGammaCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurWBTemperatureCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinWBTemperatureCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxWBTemperatureCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurWBComponentCtrl(UINT16 *blue, UINT16 *red);
UINT32 AmbaUSBH_Uvc_GetMinWBComponentCtrl(UINT16 *blue, UINT16 *red);
UINT32 AmbaUSBH_Uvc_GetMaxWBComponentCtrl(UINT16 *blue, UINT16 *red);
UINT32 AmbaUSBH_Uvc_GetCurGainCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinGainCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxGainCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurHueAutoCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinHueAutoCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxHueAutoCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurWBTempAutoCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinWBTempAutoCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxWBTempAutoCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurWBCompAutoCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinWBCompAutoCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxWBCompAutoCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurDigitalMultiplierCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMinDigitalMultiplierCtrl(void);
UINT32 AmbaUSBH_Uvc_GetMaxDigitalMultiplierCtrl(void);
UINT32 AmbaUSBH_Uvc_GetCurDigitalMultiplierLimit(void);
UINT32 AmbaUSBH_Uvc_GetMinDigitalMultiplierLimit(void);
UINT32 AmbaUSBH_Uvc_GetMaxDigitalMultiplierLimit(void);
void   AmbaUSBH_Uvc_H264GetCurVideoConfig(UVCX_VIDEO_CONFIG_S *config);
void   AmbaUSBH_Uvc_H264GetMinVideoConfig(UVCX_VIDEO_CONFIG_S *config);
void   AmbaUSBH_Uvc_H264GetMaxVideoConfig(UVCX_VIDEO_CONFIG_S *config);
UINT32 AmbaUSBH_Uvc_H264GetCurRateCtrlMode(void);
UINT32 AmbaUSBH_Uvc_H264GetMinRateCtrlMode(void);
UINT32 AmbaUSBH_Uvc_H264GetMaxRateCtrlMode(void);
UINT32 AmbaUSBH_Uvc_H264GetCurTemporalScaleMode(void);
UINT32 AmbaUSBH_Uvc_H264GetMinTemporalScaleMode(void);
UINT32 AmbaUSBH_Uvc_H264GetMaxTemporalScaleMode(void);
UINT32 AmbaUSBH_Uvc_H264GetCurSpatialScaleMode(void);
UINT32 AmbaUSBH_Uvc_H264GetMinSpatialScaleMode(void);
UINT32 AmbaUSBH_Uvc_H264GetMaxSpatialScaleMode(void);
UINT32 AmbaUSBH_Uvc_H264GetCurFrameRateConfig(void);
UINT32 AmbaUSBH_Uvc_H264GetMinFrameRateConfig(void);
UINT32 AmbaUSBH_Uvc_H264GetMaxFrameRateConfig(void);

//  Multiple Video
UINT32 AmbaUSBH_Uvc_SetCurValuesEx(UINT32 id, UINT32 unit,UINT32 selector,UINT32 value);
UINT32 AmbaUSBH_Uvc_StreamingStartEx(UINT32 id, UINT32 AltSetting);
UINT32 AmbaUSBH_Uvc_StreamingStopEx(UINT32 id);
UINT32 AmbaUSBH_Uvc_PrintDeviceValuesEx(UINT32 id);
UINT32 AmbaUSBH_Uvc_ProbeAndCommitEx(UINT32 id, UINT32 formatIndex,UINT32 frameIndex,UINT32 fps);
UINT32 AmbaUSBH_Uvc_GetCurrentProbeSettingEx(UINT32 id, UINT32 *formatIndex,UINT32 *frameIndex, UINT32 *fps);
UINT32 AmbaUSBH_Uvc_GetDeviceInfoEx(UINT32 id, UVCH_DEVICE_INFO *info);
UINT32 AmbaUSBH_Uvc_GetActiveDeviceEx(UINT8* IdArray, UINT32* length);
UINT32 AmbaUSBH_Uvc_GetPuControlInfoEx(UINT32 id, UVCH_CONTROL_ITEM *item, UVC_PROCESSING_UNIT_CONTROL_SELECTOR selector);
UINT32 AmbaUSBH_Uvc_GetItControlInfoEx(UINT32 id, UVCH_CONTROL_ITEM *item, UVC_CAMERA_TERMINAL_CONTROL_SELECTOR selector);
UINT32 AmbaUSBH_Uvc_GetAlternateSettingEx(UINT32 id, UVCH_ALTERNATE_SETTING_s *setting, UINT32 *length);
UINT32 AmbaUSBH_Uvc_ReadBlockEx(UINT32 id, UX_EHCI_ISO_REQUEST *request, UINT32 PacketNum, void (*complete_func)(UINT32 id, UX_EHCI_ISO_REQUEST *request));
UINT32 AmbaUSBH_Uvc_ReadAppendEx(UINT32 id, UX_EHCI_ISO_REQUEST *request, UINT32 PacketNum, void (*complete_func)(UINT32 id, UX_EHCI_ISO_REQUEST *request));
UINT32 AmbaUSBH_Uvc_GetMaxPacketSizeEx(UINT32 id, UINT32 *size);
UINT32 AmbaUSBH_Uvc_RegisterCallbackEx(USB_HOST_UVC_MULTI_CB_s *cb);
UINT32 AmbaUSBH_Uvc_GetBacklightCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetBacklightCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetBrightnessCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetBrightnessCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetContrastCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetContrastCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetHueCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetHueCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetSaturationCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetSaturationCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetSharpnessCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetSharpnessCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetGammaCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetGammaCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetWBTemperatureCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetWBTemperatureCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetWBComponentCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *InfoBlue, UVCH_CTRL_INFO_s *InfoRed);
UINT32 AmbaUSBH_Uvc_SetWBComponentCtrlEx(UINT32 id, UINT16 blue, UINT16 red);
UINT32 AmbaUSBH_Uvc_GetGainCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetGainCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetCurLineFrequencyCtrlEx(UINT32 id, UINT32 *ctrl);
UINT32 AmbaUSBH_Uvc_SetLineFrequencyCtrlEx(UINT32 id, UVC_POWER_LINE_FREQUENCY_e ctrl);
UINT32 AmbaUSBH_Uvc_GetHueAutoCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetHueAutoCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetWBTempAutoCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetWBTempAutoCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetWBCompAutoCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetWBCompAutoCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetDigitalMultiplierCtrlInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetDigitalMultiplierCtrlEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetDigitalMultiplierLimitInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_SetDigitalMultiplierLimitEx(UINT32 id, UINT32 ctrl);
UINT32 AmbaUSBH_Uvc_GetCurAnalogVideoStandardEx(UINT32 id, UINT32 *ctrl);
UINT32 AmbaUSBH_Uvc_SetAnalogVideoStandardEx(UINT32 id, UVC_ANALOG_VIDEO_STANDARD_e ctrl);
UINT32 AmbaUSBH_Uvc_GetCurAnalogLockStatusEx(UINT32 id, UINT32 *ctrl);
UINT32 AmbaUSBH_Uvc_SetAnalogLockStatusEx(UINT32 id, UVC_ANALOG_VIDEO_LOCK_e status);
UINT32 AmbaUSBH_Uvc_IsH264SupportedEx(UINT32 id);
UINT32 AmbaUSBH_Uvc_H264EnableEx(UINT32 id, UINT32 enable);
UINT32 AmbaUSBH_Uvc_H264GetVideoConfigInfoEx(UINT32 id, UVCX_VIDEO_CONFIG_S *ConfigMin, UVCX_VIDEO_CONFIG_S *ConfigMax, UVCX_VIDEO_CONFIG_S *ConfigCur);
UINT32 AmbaUSBH_Uvc_H264SetVideoConfigEx(UINT32 id, UVCX_VIDEO_CONFIG_S *config);
UINT32 AmbaUSBH_Uvc_H264RequestIdrEx(UINT32 id);
UINT32 AmbaUSBH_Uvc_H264GetRateCtrlModeInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_H264SetRateCtrlModeEx(UINT32 id, UINT32 mode);
UINT32 AmbaUSBH_Uvc_H264GetTemporalScaleModeInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_H264SetTemporalScaleModeEx(UINT32 id, UINT32 mode);
UINT32 AmbaUSBH_Uvc_H264GetSpatialScaleModeInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_H264SetSpatialScaleModeEx(UINT32 id, UINT32 mode);
UINT32 AmbaUSBH_Uvc_H264GetFrameRateConfigInfoEx(UINT32 id, UVCH_CTRL_INFO_s *info);
UINT32 AmbaUSBH_Uvc_H264SetFrameRateConfigEx(UINT32 id, UINT32 FrameRate);
#endif

