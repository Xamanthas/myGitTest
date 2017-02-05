#ifndef UX_HOST_CLASS_VIDEO_H
#define UX_HOST_CLASS_VIDEO_H

#include <StdUSB.h>
#include <AmbaUSB_Host_API.h>

#ifndef UX_HOST_CLASS_VIDEO_THREAD_STACK_SIZE
#define UX_HOST_CLASS_VIDEO_THREAD_STACK_SIZE             UX_THREAD_STACK_SIZE
#endif

#ifndef UX_THREAD_PRIORITY_VIDEO
#define UX_THREAD_PRIORITY_VIDEO                         70
#endif

typedef struct _USB_HOST_VIDEO_INFO_s_ {
	ULONG   unit;
	ULONG   selector;
	ULONG   value;
	ULONG   rval;
} USB_HOST_VIDEO_INFO_s;

typedef struct _USB_HOST_VIDEO_PROBE_INFO_s_ {
	ULONG   formatIndex;
	ULONG   frameIndex;
	ULONG   fps;
} USB_HOST_VIDEO_PROBE_INFO_s;

enum {
	HOST_VIDEO_SET_CUR              = 0x00000001,
	HOST_VIDEO_STREAMING_START      = 0x00000002,
	HOST_VIDEO_STREAMING_STOP       = 0x00000004,
	HOST_VIDEO_PRINT_DEVICE_VALUES  = 0x00000010,
	HOST_VIDEO_PROBING_AND_COMMIT   = 0x00000020,
	HOST_VIDEO_GET_DEVICE_INFO      = 0x00000040,
	HOST_VIDEO_REQUEST_DONE	        = 0x80000000,
	HOST_VIDEO_ALL                  = 0x00000077,
};

typedef struct UVCH_INSTANCE_INFO_STRUCT {
	UINT32                           used;
	UINT32                           id;
	UINT32                           selected_alt_setting;
	struct UVCH_INSTANCE_STRUCT      *uvch_instance;
	USB_HOST_VIDEO_INFO_s            video_info;
	USB_HOST_VIDEO_PROBE_INFO_s      probe_info;
	TX_EVENT_FLAGS_GROUP             control_thread_flag;
	struct UVCH_INSTANCE_INFO_STRUCT *next_instance_info;
} UVC_INSTANCE_INFO;

/* Define Video Class main constants.  */

#define UVCH_THREAD_PRIORITY_CLASS                          70
#define UX_HOST_CLASS_VIDEO_CLASS                           0xE
#define UX_HOST_CLASS_VIDEO_SUBCLASS_CONTROL                1
#define UX_HOST_CLASS_VIDEO_SUBCLASS_STREAMING              2

/* Define Video Class interface descriptor subclasses.  */
#define UX_HOST_CLASS_VIDEO_SC_UNDEFINED                    0x00
#define UX_HOST_CLASS_VIDEO_SC_CONTROL                      0x01
#define UX_HOST_CLASS_VIDEO_SC_STREAMING                    0x02
#define UX_HOST_CLASS_VIDEO_SC_INTERFACE_COLLECTION         0x03

#if 0
/* Define Video Class specific request codes.  */
#define UX_HOST_CLASS_VIDEO_REQUEST_CODE_UNDEFINED          0x00
#define UX_HOST_CLASS_VIDEO_SET_CUR                         0x01
#define UX_HOST_CLASS_VIDEO_GET_CUR                         0x81
#define UX_HOST_CLASS_VIDEO_GET_MIN                         0x82
#define UX_HOST_CLASS_VIDEO_GET_MAX                         0x83
#define UX_HOST_CLASS_VIDEO_GET_RES                         0x84
#define UX_HOST_CLASS_VIDEO_GET_LEN                         0x85
#define UX_HOST_CLASS_VIDEO_GET_INFO                        0x86
#define UX_HOST_CLASS_VIDEO_GET_DEF                         0x87
#endif

/* Define Video Class external terminal types.  */
#define UX_HOST_CLASS_VIDEO_EXTERNAL_VENDOR_SPECIFIC        0x0400
#define UX_HOST_CLASS_VIDEO_EXTERNAL_CONNECTOR              0X0401
#define UX_HOST_CLASS_VIDEO_EXTERNAL_SVIDEO_CONNECTOR       0X0402
#define UX_HOST_CLASS_VIDEO_EXTERNAL_COMPONENT_CONNECTOR    0X0403

#define UVCH_INTERFACE_DESCRIPTOR_ENTRIES                   8
#define UVCH_INTERFACE_DESCRIPTOR_LENGTH                    8

#define UVCH_OT_DESC_ENTRIES                                8
#define UVCH_OT_DESC_LENGTH                                 9

#define UVCH_FEATURE_UNIT_DESCRIPTOR_ENTRIES                7
#define UVCH_FEATURE_UNIT_DESCRIPTOR_LENGTH                 7

#define UVCH_STREAMING_INTERFACE_DESCRIPTOR_ENTRIES         6
#define UVCH_STREAMING_INTERFACE_DESCRIPTOR_LENGTH          6

#define UVCH_STREAMING_ENDPOINT_DESCRIPTOR_ENTRIES          6
#define UVCH_STREAMING_ENDPOINT_DESCRIPTOR_LENGTH           6

#define UVCH_PU_DESC_ENTRIES                                7
#define UVCH_PU_DESC_LENGTH                                 8

#define UVCH_EU_DESC_ENTRIES                                7
#define UVCH_EU_DESC_LENGTH                                 7

#define UVCH_IT_DESC_ENTRIES                                11
#define UVCH_IT_DESC_LENGTH                                 15

#define UVCH_XU_DESC_ENTRIES                                10
#define UVCH_XU_DESC_LENGTH                                 22

#define UVCH_VS_INPUT_HEADER_DESCRIPTOR_ENTRIES             12
#define UVCH_VS_INPUT_HEADER_DESCRIPTOR_LENGTH              13

#define UVCH_MJPEG_FORMAT_DESCRIPTOR_ENTRIES                11
#define UVCH_MJPEG_FORMAT_DESCRIPTOR_LENGTH                 11

#define UVCH_UNCOMPRESSED_FORMAT_DESCRIPTOR_ENTRIES         15
#define UVCH_UNCOMPRESSED_FORMAT_DESCRIPTOR_LENGTH          27

#define UVCH_H264_FORMAT_DESCRIPTOR_ENTRIES                 32
#define UVCH_H264_FORMAT_DESCRIPTOR_LENGTH                  52

#define UVCH_FRAME_DESCRIPTOR_ENTRIES                       12
#define UVCH_FRAME_DESCRIPTOR_LENGTH                        26

#define UVCH_H264_FRAME_DESCRIPTOR_ENTRIES                  19
#define UVCH_H264_FRAME_DESCRIPTOR_LENGTH                   44

/* Define Video Class specific interface descriptor.  */

#define UVCH_MAX_CHANNEL                                    8
#define UVCH_NAME_LENGTH                                    64

typedef struct UVCH_INTF_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bFormatType;
	ULONG           bNrChannels;
	ULONG           bSubframeSize;
	ULONG           bBitResolution;
	ULONG           bSamFreqType;
} UVCH_INTF_DESC;

/* Define Video Class specific output terminal interface descriptor.  */
typedef struct UVCH_OT_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bTerminalID;
	ULONG           wTerminalType;
	ULONG           bAssocTerminal;
	ULONG           bSourceID;
	ULONG           iTerminal;
} UVCH_OT_DESC;

/* Define Video Class streaming interface descriptor.  */
typedef struct UVCH_STREAMING_INTF_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubtype;
	ULONG           bTerminalLink;
	ULONG           bDelay;
	ULONG           wFormatTag;
} UVCH_STREAMING_INTF_DESC;


/* Define Video Class specific streaming endpoint descriptor.  */
typedef struct UVCH_STREAMING_ENDPOINT_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubtype;
	ULONG           bmAttributes;
	ULONG           bLockDelayUnits;
	ULONG           wLockDelay;
} UVCH_STREAMING_ENDPOINT_DESC;

/* Define Video Class specific processing unit interface descriptor.  */
typedef struct UVCH_PU_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bUnitID;
	ULONG           bSourceID;;
	ULONG           wMaxMultiplier;
	ULONG           bControlSize;
} UVCH_PU_DESC;

/* Define Video Class specific encode unit interface descriptor.  */
typedef struct UVCH_EU_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bUnitID;
	ULONG           bSourceID;;
	ULONG           iEncode;
	ULONG           bControlSize;
} UVCH_EU_DESC;

/* Define Video Class specific input terminal(Camera) interface descriptor.  */
typedef struct UVCH_IT_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bTerminalID;
	ULONG           wTerminalType;
	ULONG           bAssocTerminal;
	ULONG           iTerminal;
	ULONG           wObjectiveFocalLengthMin;
	ULONG           wObjectiveFocalLengthMax;
	ULONG           wOcularFocalLength;
	ULONG           bControlSize;
} UVCH_IT_DESC;


/* Define Video Class specific extension unit descriptor.  */

typedef struct UVCH_XU_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bUnitID;
	ULONG           guidExtensionCode0;
	ULONG           guidExtensionCode1;
	ULONG           guidExtensionCode2;
	ULONG           guidExtensionCode3;
	ULONG           bNumControls;
	ULONG           bNrInPins;
} UVCH_XU_DESC;



/* Define Video Class specific VS Interface Input Header descriptor.  */

typedef struct UVCH_VS_INPUT_HEADER_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bNumFormats;
	ULONG           wTotalLength;
	ULONG           bEndpointAddress;
	ULONG           bmInfo;
	ULONG           bTerminalLink;
	ULONG           bStillCaptureMethod;
	ULONG           bTriggerSupport;
	ULONG           bTriggerUsage;
	ULONG           bControlSize;
	// Currently not support bmaControls
} UVCH_VS_INPUT_HEADER_DESC;

/* Define Video Class specific MJPEG Video Format descriptor.  */

typedef struct UVCH_MJPEG_FORMAT_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bFormatIndex;
	ULONG           bNumFrameDescriptors;
	ULONG           bmFlags;
	ULONG           bDefaultFrameIndex;
	ULONG           bAspectRatioX;
	ULONG           bAspectRatioY;
	ULONG           bmInterlaceFlags;
	ULONG           bCopyProtect;
} UVCH_MJPEG_FORMAT_DESC;

/* Define Video Class specific Umncompressed Video Format descriptor.  */

typedef struct UVCH_UNCOMPRESSED_FORMAT_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bFormatIndex;
	ULONG           bNumFrameDescriptors;
	ULONG           guidFormat0;
	ULONG           guidFormat1;
	ULONG           guidFormat2;
	ULONG           guidFormat3;
	ULONG           bBitsPerPixel;
	ULONG           bDefaultFrameIndex;
	ULONG           bAspectRatioX;
	ULONG           bAspectRatioY;
	ULONG           bmInterlaceFlags;
	ULONG           bCopyProtect;
} UVCH_UNCOMPRESSED_FORMAT_DESC;

/* Define Video Class specific H264 Video Format descriptor.  */

typedef struct UVCH_H264_FORMAT_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bFormatIndex;
	ULONG           bNumFrameDescriptors;
	ULONG           bDefaultFrameIndex;
	ULONG           bMaxCodecConfigDelay;
	ULONG           bmSupportedSliceModes;
	ULONG           bmSupportedSyncFrameTypes;
	ULONG           bResolutionScaling;
	ULONG           Reserved1;
	ULONG           bmSupportedRateControlModes;
	ULONG           wMaxMBperSecOneResolutionNoScalability;
	ULONG           wMaxMBperSecTwoResolutionsNoScalability;
	ULONG           wMaxMBperSecThreeResolutionsNoScalability;
	ULONG           wMaxMBperSecFourResolutionsNoScalability;
	ULONG           wMaxMBperSecOneResolutionTemporalScalability;
	ULONG           wMaxMBperSecTwoResolutionsTemporalScalablility;
	ULONG           wMaxMBperSecThreeResolutionsTemporalScalability;
	ULONG           wMaxMBperSecFourResolutionsTemporalScalability;
	ULONG           wMaxMBperSecOneResolutionTemporalQualityScalability;
	ULONG           wMaxMBperSecTwoResolutionsTemporalQualityScalability;
	ULONG           wMaxMBperSecThreeResolutionsTemporalQualityScalablity;
	ULONG           wMaxMBperSecFourResolutionsTemporalQualityScalability;
	ULONG           wMaxMBperSecOneResolutionsTemporalSpatialScalability;
	ULONG           wMaxMBperSecTwoResolutionsTemporalSpatialScalability;
	ULONG           wMaxMBperSecThreeResolutionsTemporalSpatialScalability;
	ULONG           wMaxMBperSecFourResolutionsTemporalSpatialScalability;
	ULONG           wMaxMBperSecOneResolutionFullScalability;
	ULONG           wMaxMBperSecTwoResolutionsFullScalability;
	ULONG           wMaxMBperSecThreeResolutionsFullScalability;
	ULONG           wMaxMBperSecFourResolutionsFullScalability;
} UVCH_H264_FORMAT_DESC;

/* Define Video Class specific Video Frame descriptor.  */
typedef struct UVCH_FRAME_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bFrameIndex;
	ULONG           bmCapabilities;
	ULONG           wWidth;
	ULONG           wHeight;
	ULONG           dwMinBitRate;
	ULONG           dwMaxBitRate;
	ULONG           dwMaxVideoFrameBufferSize;
	ULONG           dwDefaultFrameInterval;
	ULONG           bFrameIntervalType;
	// Currently not the following items
} UVCH_FRAME_DESC;

/* Define Video Class specific H264 Video Frame descriptor.  */
typedef struct UVCH_H264_FRAME_DESC_STRUCT {
	ULONG           bLength;
	ULONG           bDescriptorType;
	ULONG           bDescriptorSubType;
	ULONG           bFrameIndex;
	ULONG           wWidth;
	ULONG           wHeight;
	ULONG           wSARwidth;
	ULONG           wSARheight;
	ULONG           wProfile;
	ULONG           bLevelIDC;
	ULONG           wConstrainedToolset;
	ULONG           bmSupportedUsages;
	ULONG           bmCapabilities;
	ULONG           bmSVCCapabilities;
	ULONG           bmMVCCapabilities;
	ULONG           dwMinBitRate;
	ULONG           dwMaxBitRate;
	ULONG           dwDefaultFrameInterval;
	ULONG           bNumFrameIntervals;
	// Currently not the following items
} UVCH_H264_FRAME_DESC;

/* Define Video Class instance structure.  */
typedef struct UVCH_INSTANCE_STRUCT {
	struct UVCH_INSTANCE_STRUCT *next_instance;
	UX_HOST_CLASS   *ux_host_class;
	UX_DEVICE       *ux_device;
	UX_INTERFACE    *streaming_interface;
	UX_INTERFACE    *streaming_interface_active;
	ULONG           vc_interface_number;
	ULONG           vs_interface_number;
	UX_ENDPOINT     *streaming_endpoint;
	struct UVCH_TRANSFER_REQUEST_STRUCT
		*head_transfer_request;
	struct UVCH_TRANSFER_REQUEST_STRUCT
		*tail_transfer_request;
	UINT            state;
	ULONG           terminal_link;
	ULONG           type;
	UCHAR           *config_desc;
	ULONG           config_desc_length;
	TX_THREAD       control_thread;
	TX_SEMAPHORE    control_semaphore;
	VOID            *control_thread_stack;
	ULONG           it_id;
	ULONG           xu_id;
	ULONG           xu_id_h264;
	ULONG           pu_id;
	ULONG           format_count;
	UVCH_FORMAT_INFO *format_lut;
	ULONG           uvc_ver;
	ULONG           h264_probe_default;
	ULONG           enable_mux_h264;
	struct UVCH_INSTANCE_INFO_STRUCT
		*uvch_instance_info;
	struct UVCH_DEVICE_CTRL_DATA_STRUCT
		*uvch_ctrl_data;
	struct UVCH_UNIT_SUPPORT_BITMAP_STRUCT
		*uvch_support_bitmap;
	/* UVC 1.5 */
	ULONG           eu_id;
} UVCH_INSTANCE;

/* Define Video Class isochronous USB transfer request structure.  */

typedef struct UVCH_TRANSFER_REQUEST_STRUCT {
	ULONG           status;
	UCHAR           *data_pointer;
	ULONG           requested_length;
	ULONG           actual_length;
	VOID            (*completion_function) (struct UVCH_TRANSFER_REQUEST_STRUCT *);
	TX_SEMAPHORE    semaphore;
	VOID            *class_instance;
	UINT            completion_code;
	struct UVCH_TRANSFER_REQUEST_STRUCT
		*next_request;
	UX_TRANSFER     ux_request;
} UVCH_TRANSFER_REQUEST;

/* Define Video Class control request structure.  */

typedef struct UVCH_CONTROL_REQUEST_STRUCT {
	ULONG   selector;
	ULONG   size;
	INT     set_cur;
	INT     get_cur;
	INT     get_min;
	INT     get_max;
	INT     get_res;
	INT     get_len;
	INT     get_info;
	INT     get_def;
} UVCH_CONTROL_REQUEST;

/* Define Video Class probe control structure.  */

typedef struct UVCH_PROBE_CONTROL_STRUCT {
	USHORT  bmHint;
	UCHAR   bFormatIndex;
	UCHAR   bFrameIndex;
	ULONG   dwFrameInterval;
	USHORT  wKeyFrameRate;
	USHORT  wPFrameRate;
	USHORT  wCompQuality;
	USHORT  wCompWindowSize;
	USHORT  wDelay;
	ULONG   dwMaxVideoFrameSize;
	ULONG   dwMaxPayloadTransferSize;
} UVCH_PROBE_CONTROL;

/* Define Video Class probe control structure.  */

typedef struct UVCH_INPUT_TERMINAL_STRUCT {
	UCHAR   scanning_mode;
	UCHAR   ae_mode;
	UCHAR   ae_priority;
	ULONG   exposure_time_absolute;
	CHAR    exposure_time_relative;
	USHORT  focus_absolute;
	UCHAR   focus_relative[2];
	UCHAR   focus_auto;
	USHORT  iris_absolute;
	UCHAR   iris_relative;
	USHORT  zoom_absolute;
	UCHAR   zoom_relative[3];
	ULONG   pantilt_absolute[2];
	UCHAR   pantilt_relative[4];
	SHORT   roll_absolute;
	UCHAR   roll_relative[2];
	UCHAR   privacy;
}  UVCH_INPUT_TERMINAL;

typedef struct UVCH_PROCESSING_UNIT_STRUCT {
	USHORT  backlight;
	SHORT   brightness;
	USHORT  contrast;
	USHORT  gain;
	UCHAR   power_line_frequency;
	SHORT   hue;
	USHORT  saturation;
	USHORT  sharpness;
	USHORT  gamma;
	USHORT  wb_temperature;
	UCHAR   wb_temperature_auto;
	USHORT  wb_component[2];
	UCHAR   wb_component_auto;
	USHORT  digital_multiplier;
	USHORT  digital_multiplier_limit;
	UCHAR   hue_auto;
	UCHAR   analog_video_standard;
	UCHAR   analog_lock_status;
}  UVCH_PROCESSING_UNIT;

typedef struct UVCX_RATE_CONTROL_MODE_STRUCT {
	USHORT  wLayerID;
	UCHAR   bRateControlMode;
} UVCX_RATE_CONTROL_MODE_S;

typedef struct UVCX_TEMPORAL_SCALE_MODE_STRUCT {
	USHORT  wLayerID;
	UCHAR   bTemporalScaleMode;
} UVCX_TEMPORAL_SCALE_MODE_S;

typedef struct UVCX_SPATIAL_SCALE_MODE_STRUCT {
	USHORT  wLayerID;
	UCHAR   bSpatialScaleMode;
} UVCX_SPATIAL_SCALE_MODE_S;

typedef struct UVCX_SNR_SCALE_MODE_STRUCT {
	USHORT  wLayerID;
	UCHAR   bSNRScaleMode;
	UCHAR   bMGSSublayerMode;
} UVCX_SNR_SCALE_MODE_S;

typedef struct UVCX_LTR_BUFFER_SIZE_CONTROL_STRUCT {
	USHORT  wLayerID;
	UCHAR   bLTRBufferSize;
	UCHAR   bLTREncoderControl;
} UVCX_LTR_BUFFER_SIZE_CONTROL_S;

typedef struct UVCX_LTR_PICTURE_CONTROL_STRUCT {
	USHORT  wLayerID;
	UCHAR   bPutAtPositionInLTRBuffer;
	UCHAR   bEncodeUsingLTR;
} UVCX_LTR_PICTURE_CONTROL_S;

typedef struct UVCX_PICTURE_TYPE_CONTROL_STRUCT {
	USHORT  wLayerID;
	USHORT  wPicType;
} UVCX_PICTURE_TYPE_CONTROL_S;

typedef struct UVCX_VERSION_STRUCT {
	USHORT  wVersion;
} UVCX_VERSION_S;

typedef struct UVCX_ENCODER_RESET_STRUCT {
	USHORT  wLayerID;
} UVCX_ENCODER_RESET_S;

typedef struct UVCX_FRAMERATE_CONFIG_STRUCT {
	USHORT  wLayerID;
	ULONG   dwFrameInterval;
} UVCX_FRAMERATE_CONFIG_S;

typedef struct UVCX_VIDEO_ADVANCE_CONFIG_STRUCT {
	USHORT  wLayerID;
	ULONG   dwMb_max;
	UCHAR   blevel_idc;
	UCHAR   bReserved;
} UVCX_VIDEO_ADVANCE_CONFIG_S;

typedef struct UVCX_BITRATE_LAYERS_STRUCT {
	USHORT  wLayerID;
	ULONG   dwPeakBitrate;
	ULONG   dwAverageBitrate;
} UVCX_BITRATE_LAYERS_S;

typedef struct UVCX_QP_STEPS_LAYERS_STRUCT {
	USHORT  wLayerID;
	UCHAR   bFrameType;
	UCHAR   bMinQp;
	UCHAR   bMaxQp;
} UVCX_QP_STEPS_LAYERS_S;

typedef struct UVCH_EXTENSION_UNIT_H264_STRUCT {
	UVCX_VIDEO_CONFIG_S            video_config;
	UVCX_RATE_CONTROL_MODE_S       rate_ctrl_mode;
	UVCX_TEMPORAL_SCALE_MODE_S     temporal_scale_mode;
	UVCX_SPATIAL_SCALE_MODE_S      spatial_scale_mode;
	UVCX_SNR_SCALE_MODE_S          snr_scale_mode;
	UVCX_LTR_BUFFER_SIZE_CONTROL_S ltr_buffer_size_ctrl;
	UVCX_LTR_PICTURE_CONTROL_S     ltr_picture_ctrl;
	UVCX_PICTURE_TYPE_CONTROL_S    picture_type_ctrl;
	UVCX_VERSION_S                 version;
	UVCX_ENCODER_RESET_S           encoder_reset;
	UVCX_FRAMERATE_CONFIG_S        framerate_config;
	UVCX_VIDEO_ADVANCE_CONFIG_S    video_advance_config;
	UVCX_BITRATE_LAYERS_S          bitrate_layers;
	UVCX_QP_STEPS_LAYERS_S         qp_steps_layers;
} UVCH_EXTENSION_UNIT_H264_S;

typedef struct UVC_ENCODE_UNIT_PROFILE_TOOLSET_STRUCT {
	USHORT	profile;
	USHORT	toolset;
	UCHAR	settings;
} UVC_ENCODE_UNIT_PROFILE_TOOLSET;

typedef struct UVC_ENCODE_UNIT_SYNC_REF_FRAME_STRUCT {
	UCHAR	sync_frame_type;
	USHORT	sync_frame_interval;
	UCHAR	gradual_decoder_refresh;
} UVC_ENCODE_UNIT_SYNC_REF_FRAME;

typedef struct UVCH_ENCODING_UNIT_STRUCT {
	USHORT                                                  select_layer;
	USHORT                                                  resolution[2];		// wWidth and wHeight
	UVC_ENCODE_UNIT_PROFILE_TOOLSET                         profile;
	ULONG                                                   frame_interval;
	USHORT                                                  slice_mode[2];
	UCHAR                                                   rate_control_mode;
	ULONG                                                   average_bit_rate;
	ULONG                                                   cpb_size;
	ULONG                                                   peak_bit_rate;
	USHORT                                                  quantization_params[3];
	UCHAR                                                   qp_range[2];
	UVC_ENCODE_UNIT_SYNC_REF_FRAME                          sync_ref_frame;
	UCHAR                                                   ltr_buffer[2];
	UCHAR                                                   ltr_picture[2];
	USHORT                                                  ltr_validation;
	ULONG                                                   sei_payload_type[2];
	UCHAR                                                   priority;
	UCHAR                                                   start_or_stop_layer;
	UCHAR                                                   leve_idc_limit;
	USHORT                                                  error_resiliency;
}  UVCH_ENCODING_UNIT;

/* Define Video Class channel/value control structures.  */
typedef struct UX_HOST_CLASS_VIDEO_CONTROL_STRUCT {
	ULONG  bmcontrol_bit_in_descriptor; // Used in all request except set_cur
	ULONG  selector;                    // Used in set_cur
	ULONG  request;
} UX_HOST_CLASS_VIDEO_CONTROL;

typedef struct UVCH_DEVICE_CTRL_DATA_STRUCT {
	struct UVCH_INSTANCE_STRUCT                 *uvch_instance;

	struct UVCH_INPUT_TERMINAL_STRUCT           InputTerminal;
	struct UVCH_INPUT_TERMINAL_STRUCT           InputTerminalMin;
	struct UVCH_INPUT_TERMINAL_STRUCT           InputTerminalMax;
	struct UVCH_INPUT_TERMINAL_STRUCT           InputTerminalCur;
	struct UVCH_INPUT_TERMINAL_STRUCT           InputTerminalDef;
	struct UVCH_INPUT_TERMINAL_STRUCT           InputTerminalRes;
	struct UVCH_INPUT_TERMINAL_STRUCT           InputTerminalInfo;

	struct UVCH_PROCESSING_UNIT_STRUCT          ProcessingUnit;
	struct UVCH_PROCESSING_UNIT_STRUCT          ProcessingUnitMin;
	struct UVCH_PROCESSING_UNIT_STRUCT          ProcessingUnitMax;
	struct UVCH_PROCESSING_UNIT_STRUCT          ProcessingUnitCur;
	struct UVCH_PROCESSING_UNIT_STRUCT          ProcessingUnitDef;
	struct UVCH_PROCESSING_UNIT_STRUCT          ProcessingUnitRes;
	struct UVCH_PROCESSING_UNIT_STRUCT          ProcessingUnitInfo;

	struct UVCH_EXTENSION_UNIT_H264_STRUCT      ExUnitH264;
	struct UVCH_EXTENSION_UNIT_H264_STRUCT      ExUnitH264Min;
	struct UVCH_EXTENSION_UNIT_H264_STRUCT      ExUnitH264Max;
	struct UVCH_EXTENSION_UNIT_H264_STRUCT      ExUnitH264Cur;
	struct UVCH_EXTENSION_UNIT_H264_STRUCT      ExUnitH264Def;

	struct UVCH_ENCODING_UNIT_STRUCT            EncodingUnit;
	struct UVCH_ENCODING_UNIT_STRUCT            EncodingUnitMin;
	struct UVCH_ENCODING_UNIT_STRUCT            EncodingUnitMax;
	struct UVCH_ENCODING_UNIT_STRUCT            EncodingUnitCur;
	struct UVCH_ENCODING_UNIT_STRUCT            EncodingUnitDef;
	struct UVCH_ENCODING_UNIT_STRUCT            EncodingUnitRes;
	struct UVCH_ENCODING_UNIT_STRUCT            EncodingUnitInfo;

	struct UVCH_PROBE_CONTROL_STRUCT            Probe;
	struct UVCH_PROBE_CONTROL_STRUCT            ProbeMin;
	struct UVCH_PROBE_CONTROL_STRUCT            ProbeMax;
	struct UVCH_PROBE_CONTROL_STRUCT            ProbeCur;
} UVCH_DEVICE_CTRL_DATA;

typedef enum UVC_IT_BITMAP_E {
	IT_BIT_SCANNING = 0,
	IT_BIT_AE_MODE,
	IT_BIT_AE_PRIORITY,
	IT_BIT_EXPOSE_TIME_ABSOLUTE,
	IT_BIT_EXPOSE_TIME_RELATIVE,
	IT_BIT_FOCUS_ABSOLUTE,
	IT_BIT_FOCUS_RELATIVE,
	IT_BIT_IRIS_ABSOLUTE,
	IT_BIT_IRIS_RELATIVE,
	IT_BIT_ZOOM_ABSOLUTE,
	IT_BIT_ZOOM_RELATIVE,
	IT_BIT_PANTILT_ABSOLUTE,
	IT_BIT_PANTILT_RELATIVE,
	IT_BIT_ROLL_ABSOLUTE,
	IT_BIT_ROLL_RELATIVE,
	IT_BIT_CONTROL_UNDEFINED_1,
	IT_BIT_CONTROL_UNDEFINED_2,
	IT_BIT_FOCUS_AUTO,
	IT_BIT_PRIVACY,
	IT_BIT_UNDEFINED,
	MAX_IT_CONTROL_NUM
} UVC_IT_BITMAP;

typedef enum UVC_PU_BITMAP_E {
	PU_BIT_BRIGHTNESS = 0,
	PU_BIT_CONTRAST,
	PU_BIT_HUE,
	PU_BIT_SATURATION,
	PU_BIT_SHARPNESS,
	PU_BIT_GAMMA,
	PU_BIT_WB_TEMPERATURE,
	PU_BIT_WB_COMPONENT,
	PU_BIT_BACKLIGHT,
	PU_BIT_GAIN,
	PU_BIT_POWER_LINE_FREQUENCY,
	PU_BIT_HUE_AUTO,
	PU_BIT_WB_TEMPERATURE_AUTO,
	PU_BIT_WB_COMPONENT_AUTO,
	PU_BIT_DIGITAL_MULTIPLIER,
	PU_BIT_DIGITAL_MULTIPLIER_LIMIT,
	PU_BIT_ANALOG_VIDEO_STANDARD,
	PU_BIT_ANALOG_LOCK_STATUS,
	PU_BIT_UNDEFINED,
	MAX_PU_CONTROL_NUM
} UVC_PU_BITMAP;

typedef enum UVC_EU_BITMAP_E {
	EU_BIT_SELECT_LAYER = 0,
	EU_BIT_PROFILE_TOOLSET,
	EU_BIT_RESOLUTION,
	EU_BIT_MIN_FRAME_INTERVAL,
	EU_BIT_SLICE_MODE,
	EU_BIT_RATE_CTRL_MODE,
	EU_BIT_AVERAGE_BIT_RATE,
	EU_BIT_CPB_SIZE,
	EU_BIT_PEAK_BIT_RATE,
	EU_BIT_QUANTIZATION_PARAM,
	EU_BIT_SYNC_REF_FRAME,
	EU_BIT_LTR_BUFFER,
	EU_BIT_LTR_PICTURE,
	EU_BIT_LTR_VALIDATION,
	EU_BIT_LEVEL_IDC,
	EU_BIT_SEI_MESSAGE,
	EU_BIT_QP_RANGE,
	EU_BIT_PRIORITY_ID,
	EU_BIT_START_OR_STOP_LAYER,
	EU_BIT_ERROR_RESILIENCY,
	EU_BIT_UNDEFINED,
	MAX_EU_CONTROL_NUM
} UVC_EU_BITMAP;

typedef enum UVC_XU_H264_BITMAP_E {
	XU_H264_BIT_VIDEO_CONFIG_PROBE = 0,
	XU_H264_BIT_VIDEO_CONFIG_COMMIT,
	XU_H264_BIT_RATE_CONTROL_MODE,
	XU_H264_BIT_TEMPORAL_SCALE_MODE,
	XU_H264_BIT_SPATIAL_SCALE_MODE,
	XU_H264_BIT_SNR_SCALE_MODE,
	XU_H264_BIT_LTR_BUFFER_SIZE_CONTROL,
	XU_H264_BIT_LTR_PICTURE_CONTROL,
	XU_H264_BIT_PICTURE_TYPE_CONTROL,
	XU_H264_BIT_VERSION,
	XU_H264_BIT_ENCODER_RESET,
	XU_H264_BIT_FRAMERATE_CONFIG,
	XU_H264_BIT_VIDEO_ADVANCE_CONFIG,
	XU_H264_BIT_BITRATE_LAYERS,
	XU_H264_BIT_QP_STEPS_LAYERS,
	XU_H264_BIT_UNDEFINED,
	MAX_XU_H264_CONTROL_NUM
} UVC_XU_H264_BITMAP;

typedef struct UVCH_UNIT_SUPPORT_BITMAP_STRUCT {
	struct UVCH_INSTANCE_STRUCT                 *uvch_instance;
	UCHAR                                       xu_h264[MAX_XU_H264_CONTROL_NUM];
	UCHAR                                       pu[MAX_PU_CONTROL_NUM];
	UCHAR                                       it[MAX_IT_CONTROL_NUM];
	UCHAR                                       eu[MAX_EU_CONTROL_NUM];
	UCHAR                                       eu_runtime[MAX_EU_CONTROL_NUM];
} UVCH_UNIT_SUPPORT_BITMAP;

/* Define Video Class function prototypes.  */

UINT    _uvch_activate(UX_HOST_CLASS_COMMAND *command);
UINT    _uvch_set_configuration(UVCH_INSTANCE *video);
UINT    _uvch_deactivate(UX_HOST_CLASS_COMMAND *command);
UINT    _uvch_descriptor_get(UVCH_INSTANCE *video);
UINT    _uvch_controls_list_get(UVCH_INSTANCE *video);
UINT    _uvch_device_type_get(UVCH_INSTANCE *video);
UINT    _uvch_endpoints_get(UVCH_INSTANCE *video);
UINT    _uvch_max_stream_packet_size_get(UVCH_INSTANCE *video, ULONG *size);
UINT    _uvch_entry(UX_HOST_CLASS_COMMAND *command);
UINT    _uvch_read(UVCH_INSTANCE *instance,
				UX_EHCI_ISO_REQUEST *request,
				UINT32 PacketNumbers,
				void (*complete_func)(UX_EHCI_ISO_REQUEST *request),
				UINT32 append);
UINT   _uvch_multi_read(UVCH_INSTANCE *instance,
				UX_EHCI_ISO_REQUEST *request,
				UINT32 PacketNumbers,
				void (*complete_func)(ULONG id, UX_EHCI_ISO_REQUEST *request),
				UINT32 append);
UINT    _uvch_streaming_terminal_get(UVCH_INSTANCE *video);
UINT    _uvch_transfer_request(UVCH_INSTANCE *video, UVCH_TRANSFER_REQUEST *video_transfer_request);
VOID    _uvch_transfer_request_completed(UX_TRANSFER *transfer_request);
UINT    _uvch_write(UVCH_INSTANCE *video, UVCH_TRANSFER_REQUEST *video_transfer_request);
UINT    _uvch_streaming_interface_get(UVCH_INSTANCE *video);
void    _uvch_streaming_memory_free(UVCH_INSTANCE *video);
UINT    _uvch_control_probe(UVCH_INSTANCE *video, UX_HOST_CLASS_VIDEO_CONTROL *video_control, UVCH_PROBE_CONTROL* probe);
UINT    _uvch_control_commit(UVCH_INSTANCE *video, UX_HOST_CLASS_VIDEO_CONTROL *video_control,UVCH_PROBE_CONTROL* probe_control, ULONG selector);
VOID    _uvch_print_probe_value(UVCH_PROBE_CONTROL* probe, ULONG request);

UINT    _uvch_alternate_setting_locate(UVCH_INSTANCE *video, UVCH_PROBE_CONTROL* probe_ctrl,UINT *alternate_setting);
UINT    _uvch_alternate_setting_get(UVCH_INSTANCE *video, UVCH_ALTERNATE_SETTING_s *AltPtr, ULONG *length);
/*-----------------------------------------------------------------------------------------------*\
 * Defined in ux_host_class_video_device_data.c
\*-----------------------------------------------------------------------------------------------*/
UINT32 uhc_uvc_allocate_control_data(UVCH_INSTANCE *video);
void uhc_uvc_free_control_data(UVCH_INSTANCE *video);
UVCH_INPUT_TERMINAL* uhc_uvc_get_it_data(UVCH_INSTANCE *video, ULONG request);
UVCH_PROCESSING_UNIT* uhc_uvc_get_pu_data(UVCH_INSTANCE *video, ULONG request);
UVCH_EXTENSION_UNIT_H264_S* uhc_uvc_get_xu_h264_data(UVCH_INSTANCE *video, ULONG request);
UVCH_ENCODING_UNIT*   uhc_uvc_get_eu_data(UVCH_INSTANCE *video, ULONG request);
UVCH_PROBE_CONTROL* uhc_uvc_get_probe_data(UVCH_INSTANCE *video, ULONG request);
/*-----------------------------------------------------------------------------------------------*\
 * Defined in ux_host_class_video_control_input_terminal.c
\*-----------------------------------------------------------------------------------------------*/
UINT    _uvch_it_get_value(UVCH_INSTANCE *video, UX_HOST_CLASS_VIDEO_CONTROL *video_control, UVCH_INPUT_TERMINAL* InputTerminal);
UINT    _uvch_it_desc_data_get(UVCH_INSTANCE *video, INT* pControl_size, UCHAR** bmControls);
VOID    _uvch_it_print_values(UVCH_INSTANCE *video, UVCH_INPUT_TERMINAL* inputTerminal, ULONG request);
UINT    _uvch_it_bitmap_reset(UVCH_INSTANCE *video);
UINT    _uvch_it_bitmap_set(UVCH_INSTANCE *video, ULONG bit_field);
INT     _uvch_it_is_request_supported(UVCH_INSTANCE *video, ULONG bmControl_bit, ULONG request);
UINT32  _uvch_it_fill_control_item(UVCH_INSTANCE *video, UVCH_CONTROL_ITEM *item, UVC_CAMERA_TERMINAL_CONTROL_SELECTOR selector);

/*-----------------------------------------------------------------------------------------------*\
 * Defined in ux_host_class_video_control_processing_unit.c
\*-----------------------------------------------------------------------------------------------*/
UINT32  _uvch_pu_set_value(UVCH_INSTANCE *video,ULONG selector,ULONG value);
VOID    _uvch_pu_print_values(UVCH_INSTANCE *video, UVCH_PROCESSING_UNIT* ProcessingUnit, ULONG request);
UINT    _uvch_pu_desc_data_get(UVCH_INSTANCE *video, INT* pControl_size, UCHAR** bmControls);
UINT    _uvch_pu_bitmap_set(UVCH_INSTANCE *video, ULONG bit_field);
UINT    _uvch_pu_bitmap_reset(UVCH_INSTANCE *video);
UINT    _uvch_pu_get_device_control_value(UVCH_INSTANCE *video, UX_HOST_CLASS_VIDEO_CONTROL *video_control, UVCH_PROCESSING_UNIT* ProcessingUnit);
INT     _uvch_pu_is_request_supported(UVCH_INSTANCE *video, ULONG bmControl_bit,ULONG request);
UINT32  _uvch_pu_fill_control_item(UVCH_INSTANCE *video, UVCH_CONTROL_ITEM *item, UVC_PROCESSING_UNIT_CONTROL_SELECTOR selector);

/*-----------------------------------------------------------------------------------------------*\
 * Defined in ux_host_class_video_control_encoding_unit.c
\*-----------------------------------------------------------------------------------------------*/
UINT32  _uvch_eu_set_value(UVCH_INSTANCE *video, ULONG selector, ULONG *value);
VOID    _uvch_eu_print_values(UVCH_INSTANCE *video, UVCH_ENCODING_UNIT* EncodingUnit, ULONG request);
UINT    _uvch_eu_desc_data_get(UVCH_INSTANCE *video, INT* pControl_size, UCHAR** bmControls, UCHAR** bmControlsRuntime);
UINT    _uvch_eu_bitmap_set(UVCH_INSTANCE *video, ULONG bit_field, ULONG runtime);
UINT    _uvch_eu_bitmap_reset(UVCH_INSTANCE *video);
UINT    _uvch_eu_get_device_control_value(UVCH_INSTANCE *video, UX_HOST_CLASS_VIDEO_CONTROL *video_control, UVCH_ENCODING_UNIT* EncodingUnit);
INT     _uvch_eu_is_request_supported(UVCH_INSTANCE *video, ULONG bmControl_bit,ULONG request);
UINT32  _uvch_eu_fill_control_item(UVCH_INSTANCE *video, UVCH_CONTROL_ITEM *item, UVC_ENCODING_UNIT_CONTROL_SELECTOR selector);

/*-----------------------------------------------------------------------------------------------*\
 * Defined in ux_host_class_video_control_extension_unit.c
\*-----------------------------------------------------------------------------------------------*/
UINT    _uvch_xu_get_value(UVCH_INSTANCE *video, UX_HOST_CLASS_VIDEO_CONTROL *video_control);
UINT    _uvch_xu_desc_data_get(UVCH_INSTANCE *video, INT* pControl_size, UCHAR** bmControls);
/*-----------------------------------------------------------------------------------------------*\
 * Defined in ux_host_class_video_control_extension_unit_h264.c
\*-----------------------------------------------------------------------------------------------*/
INT32   _uvch_xu_h264_fill_control_item(UVCH_INSTANCE *video, UVCH_CONTROL_ITEM *item, UVC_EXTENSION_UNIT_H264_CONTROL_SELECTOR selector);
VOID    _uvch_xu_h264_print_values(UVCH_INSTANCE *video, UVCH_EXTENSION_UNIT_H264_S* ExtensionUnit, ULONG request);
UINT    _uvch_xu_h264_bitmap_set(UVCH_INSTANCE *video, ULONG bit_field);
UINT    _uvch_xu_h264_bitmap_reset(UVCH_INSTANCE *video);
UINT32  _uvch_xu_h264_set_value(UVCH_INSTANCE *video, ULONG selector, UCHAR *buffer);
UINT    _uvch_xu_h264_desc_data_get(UVCH_INSTANCE *video, INT* pControl_size, UCHAR** bmControls);
UINT    _uvch_xu_h264_get_device_control_value(UVCH_INSTANCE *video, UX_HOST_CLASS_VIDEO_CONTROL *video_control, UVCH_EXTENSION_UNIT_H264_S* ExtensionUnit);
INT     _uvch_xu_h264_is_request_supported(UVCH_INSTANCE *video, ULONG bmControl_bit,ULONG request);
INT32   _uvch_xu_h264_query_control(UVCH_INSTANCE *video, ULONG selector, ULONG query, UVCH_EXTENSION_UNIT_H264_S* xu);
int     _uvch_xu_h264_set_picture_type(UVCH_INSTANCE *video, ULONG type);
void    _uvch_xu_h264_encoder_reset(UVCH_INSTANCE *video);
int     _uvch_xu_h264_video_config_probe(UVCH_INSTANCE *video, UCHAR query, UVCX_VIDEO_CONFIG_S *config);
int     _uvch_xu_h264_video_config_commit(UVCH_INSTANCE *video, UVCX_VIDEO_CONFIG_S *config);
ULONG   _uvch_xu_h264_get_rate_control_mode(UVCH_INSTANCE *video, ULONG query, ULONG update, ULONG *ctrl);
int     _uvch_xu_h264_set_rate_control_mode(UVCH_INSTANCE *video, ULONG mode);
ULONG   _uvch_xu_h264_get_temporal_scale_mode(UVCH_INSTANCE *video, ULONG query, ULONG update, ULONG *ctrl);
int     _uvch_xu_h264_set_temporal_scale_mode(UVCH_INSTANCE *video, ULONG mode);
ULONG   _uvch_xu_h264_get_spatial_scale_mode(UVCH_INSTANCE *video, ULONG query, ULONG update, ULONG *ctrl);
int     _uvch_xu_h264_set_spatial_scale_mode(UVCH_INSTANCE *video, ULONG mode);
ULONG   _uvch_xu_h264_get_frame_rate_config(UVCH_INSTANCE *video, ULONG query, ULONG update, ULONG *ctrl);
int     _uvch_xu_h264_set_frame_rate_config(UVCH_INSTANCE *video, ULONG FrameRate);
VOID    _uvch_xu_h264_print_format_values(UVCX_VIDEO_CONFIG_S *config);
int     _uvch_xu_h264_is_mux_h264_support(UVCH_INSTANCE *video);
int     _uvch_xu_h264_enable_mux_h264(UVCH_INSTANCE *video, int enable);
int     _uvch_xu_h264_set_update_video_config(UVCH_INSTANCE *video);
/*-----------------------------------------------------------------------------------------------*\
 * Defined in ux_host_class_video_device_initialize.c
\*-----------------------------------------------------------------------------------------------*/
VOID    _uvch_device_initialize(UVCH_INSTANCE *video);
char    *_uvch_get_control_request_string(ULONG request);
UINT32  uhc_uvc_allocate_support_bitmap(UVCH_INSTANCE *video);
UINT32  uhc_uvc_free_support_bitmap(UVCH_INSTANCE *video);
/*-----------------------------------------------------------------------------------------------*\
 * Defined in ux_host_class_video_thread.c
\*-----------------------------------------------------------------------------------------------*/
TX_EVENT_FLAGS_GROUP *_uvch_get_control_thread_event_flag(UVCH_INSTANCE *video);
USB_HOST_VIDEO_INFO_s *_uvch_get_video_info(UVCH_INSTANCE *video);
USB_HOST_VIDEO_PROBE_INFO_s *_uvch_get_probe_info(UVCH_INSTANCE *video);
UINT32  _uvch_set_alternate_setting(UVCH_INSTANCE *video, UINT32 AltSetting);
UINT32  _uvch_get_alternate_setting(UVCH_INSTANCE *video);
UINT32  _uvch_set_cur(UVCH_INSTANCE *video, ULONG unit,ULONG selector,ULONG value);
UINT32  _uvch_streaming_start(UVCH_INSTANCE *video);
UINT32  _uvch_streaming_stop(UVCH_INSTANCE *video);
VOID    _uvch_print_device_values(UVCH_INSTANCE *video);
UINT32  _uvch_probe_and_commit(UVCH_INSTANCE *video, ULONG formatIndex,ULONG frameIndex,ULONG fps);
VOID    _uvch_get_current_probe_setting(UVCH_INSTANCE *video, ULONG *formatIndex,ULONG *frameIndex,ULONG *fps);
VOID    _uvch_control_thread_entry(ULONG class_address);
UINT32  _uvch_get_device_info(UVCH_INSTANCE *video, UVCH_DEVICE_INFO *info);
UVCH_INSTANCE *_uvch_allocate_instance(void);
UINT32  _uvch_free_instance(UVCH_INSTANCE *video);
UVCH_INSTANCE *_uvch_get_instance_by_id(UINT32 id);
UINT32  _uvch_get_id_by_instance(UVCH_INSTANCE *video);
UINT32  _uvch_get_active_device(UINT8 *IdArray, UINT32 *length);
#endif


