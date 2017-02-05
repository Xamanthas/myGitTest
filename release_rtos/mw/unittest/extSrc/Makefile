

ifdef CONFIG_SOC_A12
SUBPATH=a12
endif
ifdef CONFIG_SOC_A9
SUBPATH=a9
endif

#ccflags-y += -I$(src)/extSrc/tune/$(SUBPATH)
ccflags-y += -I$(src)/extSrc/sample_calib
ccflags-y += -I$(src)/extSrc/sample_iqparam_handler/$(SUBPATH)
ccflags-y += -I$(src)/extSrc/ik_core_ut

#libmw_unittest-objs += extSrc/tune/$(SUBPATH)/AmbaTUNE_HdlrManager.o
#libmw_unittest-objs += extSrc/tune/$(SUBPATH)/AmbaTUNE_TextHdlr.o
#libmw_unittest-objs += extSrc/tune/$(SUBPATH)/AmbaTUNE_USBHdlr.o


libmw_unittest-objs += extSrc/sample_calib/AmbaUTCalibVar.o
libmw_unittest-objs += extSrc/sample_calib/AmbaUTCalibMgr.o
libmw_unittest-objs += extSrc/sample_calib/wb/AmbaUTCalibWb.o
libmw_unittest-objs += extSrc/sample_calib/audio/AmbaUTCalibAudio.o
libmw_unittest-objs += extSrc/sample_calib/adjust/AmbaUTCalibAdjust.o
libmw_unittest-objs += extSrc/sample_calib/adjust/AmbaUTCalibParams.o

ifdef CONFIG_SOC_A12
libmw_unittest-objs += extSrc/sample_aaa/AmpUT_Image.o
libmw_unittest-objs += extSrc/sample_aaa/AmpUT_ImgProcTest.o
libmw_unittest-objs += extSrc/sample_aaa/AmbaSample_Adj.o
libmw_unittest-objs += extSrc/sample_aaa/Amba_AeAwbAdj_Control.o
libmw_unittest-objs += extSrc/sample_aaa/AmbaSample_Awb.o
libmw_unittest-objs += extSrc/sample_aaa/AmbaSample_Ae.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/$(SUBPATH)/AmbaIQParamHandler.o
libmw_unittest-objs += extSrc/ik_core_ut/IK_TestUtility.o
libmw_unittest-objs += extSrc/sample_calib/AmbaUTCalibCli.o
libmw_unittest-objs += extSrc/sample_calib/warp/AmbaUTCalibWarp.o
libmw_unittest-objs += extSrc/sample_calib/vignette/AmbaUTCalibVig.o
libmw_unittest-objs += extSrc/sample_calib/bpc/AmbaUTCalibBpc.o
libmw_unittest-objs += extSrc/sample_calib/ca/AmbaUTCalibCA.o
libmw_unittest-objs += extSrc/sample_calib/blc/AmbaUTCalibBlc.o

libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/imx117_a12/AmbaIQParamImx117_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/ov4689_a12/AmbaIQParamOv4689_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/ov4689_hdr_a12/AmbaIQParamOv4689Hdr_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/ov9750_a12/AmbaIQParamOv9750_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/imx206_a12/AmbaIQParamImx206_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/imx290_a12/AmbaIQParamImx290_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/imx290_hdr_a12/AmbaIQParamImx290Hdr_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/ar0330_parallel_a12/AmbaIQParamAr0330Parallel_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/ar0230_a12/AmbaIQParamAr0230_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/ar0230_hdr_a12/AmbaIQParamAr0230Hdr_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/ar0230_shdr_a12/AmbaIQParamAr0230sHdr_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/mn34120_a12/AmbaIQParamMn34120_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/mn34222_a12/AmbaIQParamMn34222_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/mn34222_hdr_a12/AmbaIQParamMn34222Hdr_A12.o
libmw_unittest-objs += extSrc/sample_iqparam_handler/a12/ar1335_a12/AmbaIQParamAr1335_A12.o
endif
ifdef CONFIG_SOC_A9
#libmw_unittest-objs += extSrc/sample_iqparam_handler/a9/imx117_a9/AmbaIQParamImx117_A9.o
#libmw_unittest-objs += extSrc/sample_iqparam_handler/a9/imx144_a9/AmbaIQParamImx144_A9.o
#libmw_unittest-objs += extSrc/sample_iqparam_handler/a9/imx183_a9/AmbaIQParamImx183_A9.o
endif


libmw_unittest-objs += extSrc/sample_usb/AmpUsb.o
libmw_unittest-objs += extSrc/sample_usb/AmpUsb_Custom.o
libmw_unittest-objs += extSrc/sample_usb/AmpUsb_Msc.o
libmw_unittest-objs += extSrc/sample_usb/AmpUsb_MTP_Amage.o


libmw_unittest-objs += extSrc/AmbaShell_AmbaLink.o
libmw_unittest-objs += extSrc/AmbaCVBS.o

