obj-y += libmw_unittest.a

ccflags-y := -I$(srctree)/vendors/ambarella/inc/ssp
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/link
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/link/rpcprog
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/soc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/threadx
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/dsp
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/fs
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/prfile2
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/usb
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/usb/usbx
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/usb/usbx/usbx_host_controllers
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/usb/usbx/usbx_host_classes
ccflags-y += -I$(srctree)/vendors/ambarella/inc/mw
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaSys
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaShell
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaTest
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/misc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/tune
ccflags-y += -I$(srctree)/vendors/ambarella/inc/mw
ccflags-y += -I$(srctree)/vendors/ambarella/inc/mw/net/rpcprog
#ccflags-y += -I$(srctree)/vendors/ambarella/inc/mw/adas
ccflags-y += -I$(srctree)/vendors/ambarella/inc/image/ituner
ccflags-y += -I$(srctree)/vendors/ambarella/inc/image/imgschdlr
ccflags-y += -I$(srctree)/vendors/ambarella/inc/image/encmonitor
ccflags-y += -I$(srctree)/vendors/ambarella/inc/image/imgproc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/image/imgcalib
ccflags-y += -I$(srctree)/vendors/ambarella/inc/va
ccflags-y += -I$(srctree)/vendors/ambarella/inc
ccflags-y += -I$(srctree)/vendors/foss/opencv/inc
ccflags-y += -I$(srctree)/vendors/foss/opencv/inc/opencv
ccflags-y += -I$(srctree)/vendors/foss/opencv/inc/opencv2
ccflags-y += -I$(srctree)/peripheral/sensor/inc
ccflags-y += -I$(srctree)/peripheral/yuvinput/inc
ccflags-y += -I$(srctree)/peripheral/lcd/inc
ccflags-y += -I$(srctree)/peripheral/inc/misc
ccflags-y += -I$(srctree)/peripheral/audio/inc
ccflags-y += -I$(srctree)/peripheral/imu/inc
ccflags-y += -I$(srctree)/peripheral/sbrg/b5/inc
ccflags-y += -I$(srctree)/vendors/gpl/gps
ccflags-y += -I$(srctree)/peripheral/gps/inc


ccflags-y += -fshort-wchar

libmw_unittest-objs += AmpUT_Display.o
libmw_unittest-objs += UT_Pref.o
libmw_unittest-objs += AmpUT_DummyEnc.o
libmw_unittest-objs += AmpUT_Hiber.o
libmw_unittest-objs += AmpUT_MemAccPerf.o
libmw_unittest-objs += AmpUT_Osd.o
libmw_unittest-objs += DecCFSWrapper.o
libmw_unittest-objs += AmpUnitTest.o
libmw_unittest-objs += DummyAudioDec.o
libmw_unittest-objs += DummyAudioEnc.o
libmw_unittest-objs += DummyVideoDec.o
libmw_unittest-objs += DummyVideoEnc.o
libmw_unittest-objs += ExtStream.o
libmw_unittest-objs += FormatLib.o
libmw_unittest-objs += PrecMux.o
libmw_unittest-objs += RingBuffer.o

ifdef CONFIG_BUILD_MW_UT_CFS
libmw_unittest-objs += AmpUT_Cfs.o
endif

ifdef CONFIG_BUILD_MW_UT_DCF
libmw_unittest-objs += AmpUT_Dcf.o
endif

ifdef CONFIG_BUILD_MW_UT_EDITOR
libmw_unittest-objs += AmpUT_Editor.o
libmw_unittest-objs += AmpUT_Mp4Edt.o
libmw_unittest-objs += AmpUT_MovEdt.o
libmw_unittest-objs += AmpUT_MkvEdt.o
endif

ifdef CONFIG_BUILD_MW_UT_EMERGENCY_RECORDING
libmw_unittest-objs += AmpUT_Emergency.o
endif

ifdef CONFIG_BUILD_MW_UT_EVENT_RECORDING
libmw_unittest-objs += AmpUT_EventRecord.o
libmw_unittest-objs += AmpUT_EventRecord_N.o
libmw_unittest-objs += AmpUT_EventRecord_O.o
endif

ifdef CONFIG_BUILD_MW_UT_EXIF_DMX
libmw_unittest-objs += AmpUT_ExifDmx.o
endif

ifdef CONFIG_BUILD_MW_UT_EXIF_MUX
libmw_unittest-objs += AmpUT_ExifMux.o
endif

ifdef CONFIG_BUILD_MW_UT_EXT_DMX
libmw_unittest-objs += AmpUT_ExtDmx.o
endif

ifdef CONFIG_BUILD_MW_UT_EXT_MUX
libmw_unittest-objs += AmpUT_ExtMux.o
libmw_unittest-objs += AmpUT_ExtMuxAV.o
endif

ifdef CONFIG_BUILD_MW_UT_FIO
libmw_unittest-objs += AmpUT_Fio.o
endif

ifdef CONFIG_BUILD_MW_UT_MKV_DMX
libmw_unittest-objs += AmpUT_MkvDmx.o
libmw_unittest-objs += AmpUT_MkvDmxAV.o
endif

ifdef CONFIG_BUILD_MW_UT_MKV_MUX
libmw_unittest-objs += AmpUT_MkvMux.o
libmw_unittest-objs += AmpUT_MkvMuxAV.o
libmw_unittest-objs += AmpUT_MkvMuxDual.o
endif

ifdef CONFIG_BUILD_MW_UT_MOV_DMX
libmw_unittest-objs += AmpUT_MovDmx.o
endif

ifdef CONFIG_BUILD_MW_UT_MOV_MUX
libmw_unittest-objs += AmpUT_MovMux.o
endif

ifdef CONFIG_BUILD_MW_UT_MP4_DMX
libmw_unittest-objs += AmpUT_Mp4Dmx.o
libmw_unittest-objs += AmpUT_Mp4DmxAV.o
endif

ifdef CONFIG_BUILD_MW_UT_MP4_MUX
libmw_unittest-objs += AmpUT_Mp4Mux.o
libmw_unittest-objs += AmpUT_Mp4Mux2s.o
libmw_unittest-objs += AmpUT_Mp4Mux4s.o
libmw_unittest-objs += AmpUT_Mp4MuxAV.o
libmw_unittest-objs += AmpUT_Mp4MuxDual.o
endif

libmw_unittest-objs += AmpUT_StillIsoConfig.o
libmw_unittest-objs += AmpUT_VideoIsoConfig.o
libmw_unittest-objs += AmpUT_Input.o
ifdef CONFIG_BUILD_MW_UT_VIDEO_ENCODE
libmw_unittest-objs += AmpUT_VideoEnc.o
libmw_unittest-objs += AmpUT_VideoTuning.o
endif
ifdef CONFIG_BUILD_MW_UT_STILL_ENCODE
libmw_unittest-objs += AmpUT_StillEnc.o
libmw_unittest-objs += AmpUT_StillTuning.o
endif
ifdef CONFIG_BUILD_MW_UT_AUDIO_ENCODE
libmw_unittest-objs += AmpUT_AudioEnc.o
endif
ifdef CONFIG_BUILD_MW_UT_EXT_ENCODE
libmw_unittest-objs += AmpUT_ExtEnc.o
endif
ifdef CONFIG_BUILD_MW_UT_YUV_ENCODE
libmw_unittest-objs += AmpUT_YUVEnc.o
endif
ifdef CONFIG_BUILD_MW_UT_AV_ENCODE
libmw_unittest-objs += AmpUT_AVEnc.o
endif
ifdef CONFIG_BUILD_MW_UT_DUAL_VIN_ENCODE
libmw_unittest-objs += AmpUT_DualVinEnc.o
endif
ifdef CONFIG_BUILD_MW_UT_ENC_TRANSCODE
libmw_unittest-objs += AmpUT_TranscoderEnc.o
endif

ifdef CONFIG_ENABLE_DECODE_MODULE
ifdef CONFIG_BUILD_MW_UT_VIDEO_DECODE
libmw_unittest-objs += VideoDummyDemux.o
libmw_unittest-objs += AmpUT_DummyDec.o
libmw_unittest-objs += AmpUT_VideoDec.o
libmw_unittest-objs += AmpUT_VideoMultiDec.o
endif
ifdef CONFIG_BUILD_MW_UT_AUDIO_DECODE
libmw_unittest-objs += AmpUT_AudioDec.o
libmw_unittest-objs += AmpUT_Beep.o
endif
ifdef CONFIG_BUILD_MW_UT_STILL_DECODE
libmw_unittest-objs += AmpUT_StillDec.o
endif
endif

ifdef CONFIG_SOC_A9
#libmw_unittest-objs += AmpUT_MotorVu2CH.o
#libmw_unittest-objs += AmpUT_MotorVu360.o
#libmw_unittest-objs += AmpUT_Tune.o
ifdef CONFIG_ENABLE_DECODE_MODULE
libmw_unittest-objs += AmpUT_Xcode.o
endif
endif

ifdef CONFIG_SOC_A12
libmw_unittest-objs += AmpUT_Tune.o
libmw_unittest-objs += AmpUT_Calib.o
libmw_unittest-objs += AmpUT_NetCtrl.o
libmw_unittest-objs += AmpUT_NetFifo.o
libmw_unittest-objs += AmpUT_NetPbFifo.o
libmw_unittest-objs += AmpUT_NetEventNotifier.o
libmw_unittest-objs += AmpUT_NetDec.o
ifdef CONFIG_ENABLE_DECODE_MODULE
ifdef CONFIG_BUILD_MW_UT_DEC_TRANSCODE
libmw_unittest-objs += AmpUT_TranscoderDec.o
endif
endif
libmw_unittest-objs += DummyVideoEnc_Net.o
#libmw_unittest-objs += Dummy_LinuxNetfifo_APP.o
libmw_unittest-objs += AmpUT_IPC.o
libmw_unittest-objs += AmpUT_SystemIO.o
libmw_unittest-objs += AmpUT_Csc.o
ifdef CONFIG_BUILD_MW_UT_VA
ifdef CONFIG_ENABLE_VA_ADAS
libmw_unittest-objs += AmpUT_VideoAnalysis.o
#libmw_unittest-objs += AmpUT_AdasTask.o
endif
libmw_unittest-objs += AmpUT_YUVReadSD.o
libmw_unittest-objs += AmpUT_FrameHandler.o
libmw_unittest-objs += ambava_frmhdlr.o
libmw_unittest-objs += ambava_3ahdlr.o
endif

endif

ifdef CONFIG_SENSOR_AR0230
libmw_unittest-objs += AmpUT_Input_AR0230.o
endif
ifdef CONFIG_SENSOR_AR0330_PARALLEL
libmw_unittest-objs += AmpUT_Input_AR0330Parallel.o
endif
ifdef CONFIG_SENSOR_SONY_IMX117
libmw_unittest-objs += AmpUT_Input_IMX117.o
endif
ifdef CONFIG_SENSOR_SONY_IMX206
libmw_unittest-objs += AmpUT_Input_IMX206.o
endif
ifdef CONFIG_SENSOR_OV4689
libmw_unittest-objs += AmpUT_Input_OV4689.o
endif
ifdef CONFIG_SENSOR_B5_OV4689
libmw_unittest-objs += AmpUT_Input_B5_OV4689.o
endif
ifdef CONFIG_SENSOR_OV10823
libmw_unittest-objs += AmpUT_Input_OV10823.o
endif
ifdef CONFIG_SENSOR_OV9750
libmw_unittest-objs += AmpUT_Input_OV9750.o
endif
ifdef CONFIG_SENSOR_B5_OV9750
libmw_unittest-objs += AmpUT_Input_B5_OV9750.o
endif
ifdef CONFIG_SENSOR_TI913914_OV2710
libmw_unittest-objs += AmpUT_Input_TI913914_OV2710.o
endif
ifdef CONFIG_SENSOR_MN34120
libmw_unittest-objs += AmpUT_Input_MN34120.o
endif
ifdef CONFIG_SENSOR_SONY_IMX290
libmw_unittest-objs += AmpUT_Input_IMX290.o
endif
ifdef CONFIG_SENSOR_MN34222
libmw_unittest-objs += AmpUT_Input_MN34222.o
endif
ifdef CONFIG_SENSOR_SONY_IMX377
libmw_unittest-objs += AmpUT_Input_IMX377.o
endif

libmw_unittest-objs += AmpUT_Usb.o


include $(src)/extSrc/Makefile

libmw_unittest-objs += AmbaVerSetMwUnitTest.o
$(obj)/AmbaVerSetMwUnitTest.o	:	$(call amba_ver_dep,AmbaVerSetMwUnitTest.o,$(libmw_unittest-objs))
CFLAGS_AmbaVerSetMwUnitTest.o =      -DAMBA_BUILD_DATE=$(AMBA_BUILD_DATE)            \
                               -DAMBA_BUILD_DATE_NUM=$(AMBA_BUILD_DATE_NUM)    \
                               -DAMBA_BUILD_SEC_NUM=$(AMBA_BUILD_SEC_NUM)      \
                               -DAMBA_BUILD_MACHINE=$(AMBA_BUILD_MACHINE)      \
                               -DAMBA_CI_COUNT=$(AMBA_CI_COUNT)                \
                               -DAMBA_CI_ID=$(AMBA_CI_ID)

install-files := libmw_unittest.a
