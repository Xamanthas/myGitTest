
#define DDR_START			0xa0000000
#define DDR_RESV_SIZE			0x00001000
#ifdef CONFIG_AMBALINK_MEM_SIZE
#define LINUX_SIZE			CONFIG_AMBALINK_MEM_SIZE
#else
#define LINUX_SIZE			0
#endif

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(__vector)
SECTIONS
{
	. = DDR_START;
	__ddr_start = .;
	__ddr_resv_start = .;
	. = __ddr_resv_start + DDR_RESV_SIZE;
	__ddr_resv_end = .;
	.text : {
		__ddr_region0_ro_start = .;
		*AmbaBoot.o (.text*)
		*AmbaHwStack.o (.text*)
		*AmbaBoot.o (.rodata*)
		*AmbaHwStack.o (.rodata*)
		
		/******************** libapp.a START ********************/
		apps* (.text*)
		*rec_connected_cam.o (.text*)
		*rec_connected_cam_netctrl.o (.text*)
		button_op* (.text*)
		irbutton_op* (.text*)
		AmbaIQParam* (.text*)
		sensor_* (.text*)
		lcd_* (.text*)
		appmaintask* (.text*)
		appmgt* (.text*)
		app_pref* (.text*)
		app_util* (.text*)
		handler* (.text*)
		onlinePlayback* (.text*)
		/******************** libapp.a END ********************/
		
		/******************** libapplib.a START ********************/
		AmbaSample_Adj* (.text*)
		ApplibAeAwbAd_ImgProcTest* (.text*)
		ApplibAeAwbAdj_Control* (.text*)
		ApplibIQParam* (.text*)
		ApplibCalib* (.text*)
		ApplibComSvc* (.text*)
		ApplibGraphics* (.text*)
		ApplibIndex* (.text*)
		ApplibMonitor* (.text*)
		ApplibSys* (.text*)
		ApplibTune* (.text*)
		/******************** libapplib.a END ********************/
		

		AmbaAudioLib_Ver* (.text*)
		AmbaAudio_Ver* (.text*)
		
		AmbaLinkMemoryCfg* (.text*)
		AmbaMmuMemMap* (.text*)
		bsp* (.text*)
		buttons* (.text*)
		
		/******************** comsvc START ********************/
		AmbaFwUpdaterSD* (.text*)
		math* (.text*)
		msgqueue* (.text*)
		ptrlist* (.text*)
		util* (.text*)
		firmwareloader* (.text*)
		wchar* (.text*)
		syncsvc* (.text*)
		AmbaVerImpl* (.text*)
		AmbaMonitor_SD* (.text*)
		memcpy* (.text*)
		aeabi_memclr* (.text*)
		aeabi_memcpy-armv7a* (.text*)
		aeabi_memmove* (.text*)
		aeabi_memset* (.text*)
		AmbaShell* (.text*)
		AmbaCtorDtor* (.text*)
		AmbaExceptionHandler* (.text*)
		AmbaFwLoader* (.text*)
		AmbaMemcpy* (.text*)
		AmbaPrintk* (.text*)
		AmbaRootTask* (.text*)
		AmbaUtility* (.text*)
		decompress* (.text*)
		/******************** comsvc END  ********************/
		
		AmbaTest* (.text*)
		
		/******************** dspkernel START ********************/
		dump_cmd_msg_a12* (.text*)
		AmbaHL* (.text*)
		AmbaLL* (.text*)
	
		AmbaVerSetDspDspKernel* (.text*)
		AmbaDSP_ImgColorCorrection* (.text*)
		AmbaDSP_ImgAntiAliasing* (.text*)
		AmbaDSP_ImgVinSensorInfo* (.text*)
		AmbaDSP_Img1stSharpenFilter* (.text*)
		AmbaDSP_ImgChromaMedianFilter* (.text*)
		AmbaDSP_ImgCawarpCompensation* (.text*)
		AmbaDSP_ImgAdvanceSpatialFilter* (.text*)
		AmbaDSP_ImgDynamicBadPixelCorrection* (.text*)
		AmbaDSP_ImgDgainSaturationLevel* (.text*)
		AmbaDSP_ImgChromaScale* (.text*)
		AmbaDSP_ImgVideoMctf* (.text*)
		AmbaDSP_ImgFinalSharpenFilter* (.text*)
		AmbaDSP_ImgChromaFilter* (.text*)
		AmbaDSP_Img3aStatistics* (.text*)
		AmbaDSP_ImgStaticBlackLevel* (.text*)
		AmbaDSP_ImgCfaLeakageFilter* (.text*)
		AmbaDSP_ImgWbGain* (.text*)
		AmbaDSP_ImgGbGrMismatch* (.text*)
		AmbaDSP_ImgRgbToYuvMatrix* (.text*)
		AmbaDSP_ImgResamplerCoefAdj* (.text*)
		AmbaDSP_ImgVignetteCompensation* (.text*)
		AmbaDSP_ImgDeferredBlackLevel* (.text*)
		AmbaDSP_ImgLumaProcessMode* (.text*)
		AmbaDSP_ImgCfaNoiseFilter* (.text*)
		AmbaDSP_ImgWarpCompensation* (.text*)
		AmbaDSP_ImgStaticBadPixelCorrection* (.text*)
		AmbaDSP_ImgLocalExposure* (.text*)
		AmbaDSP_ImgDemosaic* (.text*)
		AmbaDSP_ImgColorDependentNoiseReduction* (.text*)
		AmbaDSP_ImgToneCurve* (.text*)
		AmbaDSP_ImgUtility* (.text*)
		AmbaDSP_ImgLowIso* (.text*)
		AmbaDSP_ImgColorCorrectionReg* (.text*)
		AmbaDSP_ImgInternalUtility* (.text*)
		AmbaDSP_ImgVideoHdr* (.text*)
		AmbaDSP_ImgCoreWarpCompensation* (.text*)
		AmbaDSP_ImgCoreCawarpCompensation* (.text*)
		AmbaDSP_ImgCoreLowIso* (.text*)
		AmbaDSP_ImgCoreArchUtility* (.text*)
		AmbaDSP_ImgCoreAdvanceSpatialFilter* (.text*)
		AmbaDSP_ImgCoreLumaProcessMode* (.text*)
		AmbaDSP_ImgCore1stSharpenFilter* (.text*)
		AmbaDSP_ImgCoreFinalSharpenFilter* (.text*)
		AmbaDSP_ImgCoreAntiAliasing* (.text*)
		AmbaDSP_ImgCoreCfaLeakageFilter* (.text*)
		AmbaDSP_ImgCoreGbGrMismatch* (.text*)
		AmbaDSP_ImgCoreStaticBadPixelCorrection* (.text*)
		AmbaDSP_ImgCoreChromaFilter* (.text*)
		AmbaDSP_ImgCoreVideoMctf* (.text*)
		AmbaDSP_ImgCoreVignetteCompensation* (.text*)
		AmbaDSP_ImgCoreDemosaic* (.text*)
		AmbaDSP_ImgCoreShpApi* (.text*)
		AmbaDSP_ImgCoreTestProbe* (.text*)
		AmbaDSP_ImgCoreUtility* (.text*)
		AmbaDSP_ImgCoreChromaMedianFilter* (.text*)
		AmbaDSP_ImgCoreStaticBlackLevel* (.text*)
		AmbaDSP_ImgCoreWbGain* (.text*)
		AmbaDSP_ImgCoreDeferredBlackLevel* (.text*)
		AmbaDSP_ImgCoreDynamicBadPixelCorrection* (.text*)
		AmbaDSP_ImgCoreLocalExposure* (.text*)
		AmbaDSP_ImgCoreDgainSaturationLevel* (.text*)
		AmbaDSP_ImgCoreResamplerCoefAdj* (.text*)
		AmbaDSP_ImgCoreCfaNoiseFilter* (.text*)
		AmbaDSP_ImgCoreChromaScale* (.text*)
		AmbaDSP_ImgCoreColorDependentNoiseReduction* (.text*)
		AmbaDSP_ImgCoreToneCurve* (.text*)
		AmbaDSP_ImgCoreColorCorrection* (.text*)
		AmbaDSP_ImgCoreRgbToYuvMatrix* (.text*)
		AmbaDSP_ImgCoreColorCorrectionReg* (.text*)
		AmbaDSP_ImgCoreVinSensorInfo* (.text*)
		AmbaDSP_ImgCoreGlobal* (.text*)
		AmbaDSP_ImgCoreMctfApi* (.text*)
		AmbaDSP_ImgCoreVideoHdr* (.text*)
		AmbaVerSetDspImgKernel* (.text*)
		/******************** dspkernel END ********************/
		
		/******************** libfs.a START ********************/
		AmbaFS* (.text*)
		/******************** libfs.a END ********************/
		
		/******************** libimg_adj.a START ********************/
		Amba_Quality_Func* (.text*)
		Amba_Adj_Func* (.text*)
		Amba_Hdr_Func* (.text*)
		/******************** libimg_adj.a END ********************/
		
		/******************** libimg_ae.a START ********************/
		AmbaImg_AntiFlicker* (.text*)
		Amba_Ae* (.text*)
		ae_ev_table* (.text*)
		/******************** libimg_ae.a END ********************/
		
		/******************** libimg_awb.a START ********************/
		Amba_Awb* (.text*)
		/******************** libimg_awb.a END ********************/
		
		/******************** libimg_calib.a START ********************/
		AmbaCalib* (.text*)
		/******************** libimg_calib.a END ********************/
		
		/******************** libimg_eis.a START ********************/
		AmbaUTCalibGyro* (.text*)
		AmbaImageMainEis* (.text*)
		Amba_Eis_Control* (.text*)
		/******************** libimg_eis.a END ********************/
		
		/******************** libimg_encmonitor.a START ********************/
		encMonitor* (.text*)
		/******************** libimg_encmonitor.a END ********************/
		
		/******************** libimgproc.a START ********************/
		AmbaImg_AdjTsk* (.text*)
		AmbaImg_AeTsk* (.text*)
		AmbaImg_AwbTsk* (.text*)
		AmbaImg_ObTsk* (.text*)
		AmbaImg_CtrlFunc* (.text*)
		AmbaImg_Exif* (.text*)
		AmbaImg_Impl* (.text*)
		AmbaImg_ProcFunc* (.text*)
		AmbaImg_VDsp_Handler* (.text*)
		AmbaImg_VIn_Handler* (.text*)
		AmbaVer_SetImgProc* (.text*)
		/******************** libimgproc.a END ********************/
		
		/******************** libimg_schdlr.a START ********************/
		ImgScheduler* (.text*)
		ImgSchdlr_Debug* (.text*)
		ImgSchdlr_Util* (.text*)
		/******************** libimg_schdlr.a END ********************/
		
		/******************** libimg_utils.a START ********************/
		AmbaRawDecompress* (.text*)
		/******************** libimg_utils.a END ********************/
		
		/******************** libkal.a START ********************/
		AmbaMain* (.text*)
		AmbaKAL_ThreadX* (.text*)
		/******************** libkal.a END ********************/
		
		/******************** liblink.a START ********************/
		AmbaIPC_* (.text*)
		AmbaLink* (.text*)
		fdt* (.text*)
		AmbaAoss* (.text*)
		AmbaBoss* (.text*)
		/******************** liblink.a END ********************/
		
		/******************** libmw_dataflow.a START ********************/
		DefTable* (.text*)
		/******************** libmw_dataflow.a END ********************/
		
		/******************** libmw_dspflow.a START ********************/
		Mw_Display* (.text*)
		Mw_Osd* (.text*)
		Fifo* (.text*)
		VideoEnc* (.text*)
		Resource* (.text*)
		vin* (.text*)
		/******************** libmw_dspflow.a END ********************/
		

		
		/******************** libperipheral.a START ********************/
		AmbaSensor* (.text*)
		AmbaAudio_TLV320AIC3256* (.text*)
		AmbaLCD* (.text*)
		AmbaIMU* (.text*)
		AmbaIR_Panasonic* (.text*)
		/******************** libperipheral.a END ********************/
		
		/******************** libprfile.a START ********************/
		pfk_api* (.text*)
		pf_w_api_util* (.text*)
		pf_w_cdelete* (.text*)
		pf_w_chdir* (.text*)
		pf_w_chmod* (.text*)
		pf_w_chmoddir* (.text*)
		pf_w_cinsert* (.text*)
		pf_w_cleandir* (.text*)
		pf_w_combine* (.text*)
		pf_w_divide* (.text*)
		pf_w_fopen* (.text*)
		pf_w_fstat* (.text*)
		pf_w_mkdir* (.text*)
		pf_w_move* (.text*)
		pf_w_remove* (.text*)
		pf_w_rmdir* (.text*)
		pf_api_util* (.text*)
		pf_attach* (.text*)
		pf_cdelete* (.text*)
		pf_chdir* (.text*)
		pf_chmod* (.text*)
		pf_chmoddir* (.text*)
		pf_cinsert* (.text*)
		pf_cleandir* (.text*)
		pf_combine* (.text*)
		pf_divide* (.text*)
		pf_fappend* (.text*)
		pf_fopen* (.text*)
		pf_fstat* (.text*)
		pf_init_prfile2* (.text*)
		pf_mount* (.text*)
		pf_rmdir* (.text*)
		pf_setupfsi* (.text*)
		pf_clib* (.text*)
		pf_code* (.text*)
		pf_str* (.text*)
		pf_driver* (.text*)
		pdm_bpb* (.text*)
		pdm_disk* (.text*)
		pdm_dskmng* (.text*)
		pdm_exfat_bss* (.text*)
		pdm_mbr* (.text*)
		pdm_partition* (.text*)
		pf_cache* (.text*)
		pf_cluster* (.text*)
		pf_dir* (.text*)
		pf_entry* (.text*)
		pf_entry_iterator* (.text*)
		pf_fat* (.text*)
		pf_fat12* (.text*)
		pf_fatfs* (.text*)
		pf_file* (.text*)
		pf_mkdir* (.text*)
		pf_move* (.text*)
		pf_remove* (.text*)
		pf_path* (.text*)
		pf_sector* (.text*)
		pf_volume* (.text*)
		pf_ascii* (.text*)
		pf_filelock* (.text*)
		pf_system* (.text*)
		/******************** libprfile.a END ********************/
		
		/******************** libsoc.a START ********************/
		AmbaCSL* (.text*)
		AmbaADC* (.text*)
		AmbaCache* (.text*)
		AmbaCVBS* (.text*)
		AmbaDMA* (.text*)
		AmbaFIO* (.text*)
		AmbaGDMA* (.text*)
		AmbaGPIO* (.text*)
		AmbaHDMI* (.text*)
		AmbaI2C* (.text*)
		AmbaI2S* (.text*)
		AmbaINT* (.text*)
		AmbaIR* (.text*)
		AmbaNandBadBlockTable* (.text*)
		AmbaNAND* (.text*)
		AmbaNAND_PartitionTable* (.text*)
		AmbaNFTL* (.text*)
		AmbaEMMC* (.text*)
		AmbaEMMC_PartitionTable* (.text*)
		AmbaNOR* (.text*)
		AmbaNOR_PartitionTable* (.text*)
		AmbaPLL* (.text*)
		AmbaPLL_SetClkFreq* (.text*)
		AmbaPWC* (.text*)
		AmbaROM* (.text*)
		AmbaRTC* (.text*)
		AmbaSD* (.text*)
		AmbaSPI* (.text*)
		AmbaSysTimer* (.text*)
		AmbaTimer* (.text*)
		AmbaUART* (.text*)
		AmbaVideoTiming* (.text*)
		AmbaVIN* (.text*)
		AmbaCardManager* (.text*)
		AmbaVinInfoParser* (.text*)
		AmbaWDT* (.text*)
		AmbaSysInit* (.text*)
		AmbaSysInfo* (.text*)
		AmbaSysProfile* (.text*)
		AmbaRTSL* (.text*)
		/******************** libsoc.a END ********************/
		
		/******************** libthreadx.a START ********************/
		tx* (.text*)
		/******************** libthreadx.a END ********************/
		
		/******************** libusb.a START ********************/
		udc_resource* (.text*)
		ux_device_stack_* (.text*)
		ux_system_initialize* (.text*)
		ux_utility_* (.text*)
		/******************** libusb.a END ********************/
		
		/******************** libusb_driver.a START ********************/
		AmbaUSB_Drv_Endpoint* (.text*)
		AmbaUSB_Drv_FrameNumGet* (.text*)
		AmbaUSB_Drv_Function* (.text*)
		AmbaUSB_Drv_Init* (.text*)
		AmbaUSB_Host_Class_Storage* (.text*)
		AmbaUSB_Host_Class_StoragePrFile2* (.text*)
		AmbaUSB_Host_Drv_Init* (.text*)
		/******************** libusb_driver.a END ********************/
		
		/******************** libc.a libnosys.a libm.a START ********************/
		lib_a* (.text*)
		sbrk* (.text*)
		/******************** libc.a libnosys.a libm.a END ********************/
		
		/******************** libgcc.a START ********************/
		_udivsi3* (.text*)
		_divsi3* (.text*)
		_dvmd_tls* (.text*)
		_lshrdi3* (.text*)
		_ashrdi3* (.text*)
		_ashldi3* (.text*)
		_arm_addsubdf3* (.text*)
		_aeabi_ldivmod* (.text*)
		_aeabi_uldivmod* (.text*)
		_fixdfdi* (.text*)
		_fixunsdfdi* (.text*)
		bpabi* (.text*)
		_divdi3* (.text*)
		_udivdi3* (.text*)
		/******************** libgcc.a END ********************/

		AmbaVer* (.text*)

		
		
		
		
		/******************** libapp.a START ********************/
		apps* (.rodata*)
		*rec_connected_cam.o (.rodata*)
		*rec_connected_cam_netctrl.o (.rodata*)
		button_op* (.rodata*)
		irbutton_op* (.rodata*)
		AmbaIQParam* (.rodata*)
		sensor_* (.rodata*)
		lcd_* (.rodata*)
		appmaintask* (.rodata*)
		appmgt* (.rodata*)
		app_pref* (.rodata*)
		app_util* (.rodata*)
		handler* (.rodata*)
		onlinePlayback* (.rodata*)
		/******************** libapp.a END ********************/
		
		/******************** libapplib.a START ********************/
		ApplibAeAwbAd_ImgProcTest* (.rodata*)
		ApplibIQParam* (.rodata*)
		ApplibCalib* (.rodata*)
		ApplibComSvc* (.rodata*)
		ApplibGraphics* (.rodata*)
		ApplibIndex* (.rodata*)
		ApplibMonitor* (.rodata*)
		ApplibSys* (.rodata*)
		ApplibTune* (.rodata*)

		/******************** libapplib.a END ********************/
		
		AmbaAudio_Ver* (.rodata*)
		
		AmbaLinkMemoryCfg* (.rodata*)
		AmbaMmuMemMap* (.rodata*)
		bsp* (.rodata*)
		buttons* (.rodata*)
		
		/******************** comsvc START ********************/
		AmbaFwUpdaterSD* (.rodata*)
		math* (.rodata*)
		msgqueue* (.rodata*)
		ptrlist* (.rodata*)
		util* (.rodata*)
		firmwareloader* (.rodata*)
		wchar* (.rodata*)
		syncsvc* (.rodata*)
		AmbaVerImpl* (.rodata*)
		AmbaMonitor_SD* (.rodata*)
		AmbaShell* (.rodata*)
		AmbaExceptionHandler* (.rodata*)
		AmbaFwLoader* (.rodata*)
		AmbaMemcpy* (.rodata*)
		AmbaPrintk* (.rodata*)
		AmbaUtility* (.rodata*)
		decompress* (.rodata*)
		/******************** comsvc END  ********************/
		
		AmbaTest* (.rodata*)
		
		/******************** dspkernel START ********************/
		dump_cmd_msg_a12* (.rodata*)
		AmbaHL* (.rodata*)
		AmbaLL* (.rodata*)
		
		AmbaVerSetDspDspKernel* (.rodata*)
		AmbaDSP_ImgColorCorrection* (.rodata*)
		AmbaDSP_ImgAntiAliasing* (.rodata*)
		AmbaDSP_ImgVinSensorInfo* (.rodata*)
		AmbaDSP_Img1stSharpenFilter* (.rodata*)
		AmbaDSP_ImgChromaMedianFilter* (.rodata*)
		AmbaDSP_ImgCawarpCompensation* (.rodata*)
		AmbaDSP_ImgAdvanceSpatialFilter* (.rodata*)
		AmbaDSP_ImgDynamicBadPixelCorrection* (.rodata*)
		AmbaDSP_ImgDgainSaturationLevel* (.rodata*)
		AmbaDSP_ImgChromaScale* (.rodata*)
		AmbaDSP_ImgVideoMctf* (.rodata*)
		AmbaDSP_ImgFinalSharpenFilter* (.rodata*)
		AmbaDSP_ImgChromaFilter* (.rodata*)
		AmbaDSP_Img3aStatistics* (.rodata*)
		AmbaDSP_ImgStaticBlackLevel* (.rodata*)
		AmbaDSP_ImgCfaLeakageFilter* (.rodata*)
		AmbaDSP_ImgWbGain* (.rodata*)
		AmbaDSP_ImgGbGrMismatch* (.rodata*)
		AmbaDSP_ImgRgbToYuvMatrix* (.rodata*)
		AmbaDSP_ImgResamplerCoefAdj* (.rodata*)
		AmbaDSP_ImgVignetteCompensation* (.rodata*)
		AmbaDSP_ImgDeferredBlackLevel* (.rodata*)
		AmbaDSP_ImgLumaProcessMode* (.rodata*)
		AmbaDSP_ImgCfaNoiseFilter* (.rodata*)
		AmbaDSP_ImgWarpCompensation* (.rodata*)
		AmbaDSP_ImgStaticBadPixelCorrection* (.rodata*)
		AmbaDSP_ImgLocalExposure* (.rodata*)
		AmbaDSP_ImgDemosaic* (.rodata*)
		AmbaDSP_ImgColorDependentNoiseReduction* (.rodata*)
		AmbaDSP_ImgToneCurve* (.rodata*)
		AmbaDSP_ImgUtility* (.rodata*)
		AmbaDSP_ImgLowIso* (.rodata*)
		AmbaDSP_ImgColorCorrectionReg* (.rodata*)
		AmbaDSP_ImgInternalUtility* (.rodata*)
		AmbaDSP_ImgVideoHdr* (.rodata*)
		AmbaDSP_ImgCoreWarpCompensation* (.rodata*)
		AmbaDSP_ImgCoreCawarpCompensation* (.rodata*)
		AmbaDSP_ImgCoreLowIso* (.rodata*)
		AmbaDSP_ImgCoreArchUtility* (.rodata*)
		AmbaDSP_ImgCoreAdvanceSpatialFilter* (.rodata*)
		AmbaDSP_ImgCoreLumaProcessMode* (.rodata*)
		AmbaDSP_ImgCore1stSharpenFilter* (.rodata*)
		AmbaDSP_ImgCoreFinalSharpenFilter* (.rodata*)
		AmbaDSP_ImgCoreAntiAliasing* (.rodata*)
		AmbaDSP_ImgCoreCfaLeakageFilter* (.rodata*)
		AmbaDSP_ImgCoreGbGrMismatch* (.rodata*)
		AmbaDSP_ImgCoreStaticBadPixelCorrection* (.rodata*)
		AmbaDSP_ImgCoreChromaFilter* (.rodata*)
		AmbaDSP_ImgCoreVideoMctf* (.rodata*)
		AmbaDSP_ImgCoreVignetteCompensation* (.rodata*)
		AmbaDSP_ImgCoreDemosaic* (.rodata*)
		AmbaDSP_ImgCoreShpApi* (.rodata*)
		AmbaDSP_ImgCoreTestProbe* (.rodata*)
		AmbaDSP_ImgCoreUtility* (.rodata*)
		AmbaDSP_ImgCoreChromaMedianFilter* (.rodata*)
		AmbaDSP_ImgCoreStaticBlackLevel* (.rodata*)
		AmbaDSP_ImgCoreWbGain* (.rodata*)
		AmbaDSP_ImgCoreDeferredBlackLevel* (.rodata*)
		AmbaDSP_ImgCoreDynamicBadPixelCorrection* (.rodata*)
		AmbaDSP_ImgCoreLocalExposure* (.rodata*)
		AmbaDSP_ImgCoreDgainSaturationLevel* (.rodata*)
		AmbaDSP_ImgCoreResamplerCoefAdj* (.rodata*)
		AmbaDSP_ImgCoreCfaNoiseFilter* (.rodata*)
		AmbaDSP_ImgCoreChromaScale* (.rodata*)
		AmbaDSP_ImgCoreColorDependentNoiseReduction* (.rodata*)
		AmbaDSP_ImgCoreToneCurve* (.rodata*)
		AmbaDSP_ImgCoreColorCorrection* (.rodata*)
		AmbaDSP_ImgCoreRgbToYuvMatrix* (.rodata*)
		AmbaDSP_ImgCoreColorCorrectionReg* (.rodata*)
		AmbaDSP_ImgCoreVinSensorInfo* (.rodata*)
		AmbaDSP_ImgCoreConst* (.rodata*)
		AmbaDSP_ImgCoreVideoHdr* (.rodata*)
		AmbaVerSetDspImgKernel* (.rodata*)
		/******************** dspkernel END ********************/
		
		/******************** libfs.a START ********************/
		AmbaFS* (.rodata*)
		/******************** libfs.a END ********************/
		
		/******************** libimg_adj.a START ********************/
		Amba_Quality_Func* (.rodata*)
		Amba_Adj_Func* (.rodata*)
		Amba_Hdr_Func* (.rodata*)
		/******************** libimg_adj.a END ********************/
		
		/******************** libimg_ae.a START ********************/
		AmbaImg_AntiFlicker* (.rodata*)
		Amba_Ae* (.rodata*)
		ae_ev_table* (.rodata*)
		/******************** libimg_ae.a END ********************/
		
		/******************** libimg_awb.a START ********************/
		Amba_Awb* (.rodata*)
		/******************** libimg_awb.a END ********************/
		
		/******************** libimg_calib.a START ********************/
		AmbaCalib* (.rodata*)
		/******************** libimg_calib.a END ********************/
		
		/******************** libimg_eis.a START ********************/
		AmbaUTCalibGyro* (.rodata*)
		AmbaImageMainEis* (.rodata*)
		Amba_Eis_Control* (.rodata*)
		/******************** libimg_eis.a END ********************/
		

		
		/******************** libimgproc.a START ********************/
		AmbaImg_AdjTsk* (.rodata*)
		AmbaImg_AeTsk* (.rodata*)
		AmbaImg_AwbTsk* (.rodata*)
		AmbaImg_ObTsk* (.rodata*)
		AmbaImg_CtrlFunc* (.rodata*)
		AmbaImg_Exif* (.rodata*)
		AmbaImg_Impl* (.rodata*)
		AmbaImg_ProcFunc* (.rodata*)
		AmbaImg_VDsp_Handler* (.rodata*)
		AmbaImg_VIn_Handler* (.rodata*)
		AmbaVer_SetImgProc* (.rodata*)
		/******************** libimgproc.a END ********************/
		
		/******************** libimg_schdlr.a START ********************/
		ImgScheduler* (.rodata*)
		ImgSchdlr_Debug* (.rodata*)
		ImgSchdlr_Util* (.rodata*)
		/******************** libimg_schdlr.a END ********************/
		
		/******************** libimg_utils.a START ********************/
		AmbaRawDecompress* (.rodata*)
		/******************** libimg_utils.a END ********************/
		
		/******************** libkal.a START ********************/
		AmbaMain* (.rodata*)
		tx_initialize_low_level* (.rodata*)
		AmbaKAL_ThreadX* (.rodata*)
		/******************** libkal.a END ********************/
		
		/******************** liblink.a START ********************/
		AmbaIPC_* (.rodata*)
		AmbaLink* (.rodata*)
		fdt* (.rodata*)
		AmbaAoss* (.rodata*)
		AmbaBoss* (.rodata*)
		/******************** liblink.a END ********************/
		
		/******************** libmw_dataflow.a START ********************/
		DefTable* (.rodata*)
		/******************** libmw_dataflow.a END ********************/
		
		/******************** libmw_dspflow.a START ********************/
		Mw_Display* (.rodata*)
		Mw_Osd* (.rodata*)
		Fifo* (.rodata*)
		VideoEnc* (.rodata*)
		Resource* (.rodata*)
		vin* (.rodata*)
		/******************** libmw_dspflow.a END ********************/
		
		/******************** libperipheral.a START ********************/
		AmbaSensor* (.rodata*)
		AmbaCalibInfo* (.rodata*)
		AmbaAudio_TLV320AIC3256* (.rodata*)
		AmbaLCD* (.rodata*)
		/******************** libperipheral.a END ********************/
		
		/******************** libprfile.a START ********************/
		pf_code* (.rodata*)
		pdm_exfat_bss* (.rodata*)
		pf_dir* (.rodata*)
		pf_entry* (.rodata*)
		pf_entry_iterator* (.rodata*)
		pf_fat* (.rodata*)
		pf_file* (.rodata*)
		pf_path* (.rodata*)
		pf_volume* (.rodata*)
		/******************** libprfile.a END ********************/
		
		/******************** libsoc.a START ********************/
		AmbaCSL* (.rodata*)
		AmbaNandBadBlockTable* (.rodata*)
		AmbaNAND* (.rodata*)
		AmbaNAND_PartitionTable* (.rodata*)
		AmbaNFTL* (.rodata*)
		AmbaEMMC* (.rodata*)
		AmbaEMMC_PartitionTable* (.rodata*)
		AmbaNOR* (.rodata*)
		AmbaNOR_PartitionTable* (.rodata*)
		AmbaROM* (.rodata*)
		AmbaRTC* (.rodata*)
		AmbaSD* (.rodata*)
		AmbaVideoTiming* (.rodata*)
		AmbaCardManager* (.rodata*)
		AmbaVinInfoParser* (.rodata*)
		AmbaSysInit* (.rodata*)
		AmbaSysInfo* (.rodata*)
		AmbaSysProfile* (.rodata*)
		AmbaRTSL* (.rodata*)
		/******************** libsoc.a END ********************/
		
		/******************** libthreadx.a START ********************/
		tx* (.rodata*)
		/******************** libthreadx.a END ********************/
		
		/******************** libusb.a START ********************/
		udc_resource* (.rodata*)
		ux_device_stack_* (.rodata*)
		ux_system_initialize* (.rodata*)
		ux_utility_* (.rodata*)
		/******************** libusb.a END ********************/
		
		/******************** libusb_driver.a START ********************/
		AmbaUSB_Drv_Endpoint* (.rodata*)
		AmbaUSB_Drv_Init* (.rodata*)
		AmbaUSB_Host_Class_Storage* (.rodata*)
		AmbaUSB_Host_Drv_Init* (.rodata*)
		/******************** libusb_driver.a END ********************/
		
		/******************** libc.a libnosys.a libm.a START ********************/
		lib_a* (.rodata*)
		/******************** libc.a libnosys.a libm.a END ********************/
		
		*AmbaVer_LinkInfo.o (.rodata*)
		AmbaVer* (.rodata*)
		
		AmbaAudioLib_Ver* (.constdata*)

		/*** For libaudio.a ***/
		* (i.__ARM_common*)
		* (FFTCODE*)
		__ddr_region0_ro_end = .;

		. = ALIGN(32);
		__ddr_region1_ro_start = .;
		
		ApplibRecorder_MemMgr* (.text*)
		ApplibRecorder_VideoEnc* (.text*)	
		ApplibRecorder_StillEnc* (.text*)

		ApplibRecorder_MemMgr* (.rodata*)
		ApplibRecorder_VideoEnc* (.rodata*)
		ApplibRecorder_StillEnc* (.rodata*)

		ApplibRecorder_AudioEnc* (.text*)
		ApplibRecorder_ExtendEnc* (.text*)
		ApplibRecorder_LoopEnc* (.text*)
	
		ApplibRecorder_AudioEnc* (.rodata*)
		ApplibRecorder_ExtendEnc* (.rodata*)
		ApplibRecorder_LoopEnc* (.rodata*)		
	
		ApplibNet_Base* (.text*)
		ApplibNet_Control* (.text*)
		ApplibNet_Fifo* (.text*)
		ApplibNet_IPCTest* (.text*)
		ApplibNet_JsonUtility* (.text*)
		ApplibNet_HiberTest* (.text*)
		
		ApplibNet_Base* (.rodata*)
		ApplibNet_Control* (.rodata*)
		ApplibNet_Fifo* (.rodata*)
		ApplibNet_IPCTest* (.rodata*)
		ApplibNet_JsonUtility* (.rodata*)
		ApplibNet_HiberTest* (.rodata*)	
		
		ApplibTranscoder_* (.text*)
		ApplibTranscoder_* (.rodata*)
		
		/******************** libmw_net.a START ********************/	
		Json_* (.text*)
		NetCtrl* (.text*)
		Rpc_DataSvc* (.text*)
		Rpc_LnxCmdHndlr* (.text*)
		Rpc_NetCtrl* (.text*)
		Msg_Queue_Handler* (.text*)
		Ble_Network_Handler* (.text*)
		NetEventNotifier* (.text*)
		NetFifo* (.text*)
		NetUtility* (.text*)

		Json_* (.rodata*)
		NetCtrl* (.rodata*)
		Rpc_DataSvc* (.rodata*)
		Rpc_LnxCmdHndlr* (.rodata*)
		Rpc_NetCtrl* (.rodata*)
		Msg_Queue_Handler* (.rodata*)
		Ble_Network_Handler* (.rodata*)
		NetEventNotifier* (.rodata*)
		NetFifo* (.rodata*)
		NetUtility* (.rodata*)
		/******************** libmw_net.a END ********************/
		
		ambava_adas* (.text*)
		ambava_adas* (.rodata*)
		
		
		AmbaAudio_Effect* (.text*)
		AmbaAudio_Enc* (.text*)
		AmbaAudio_I2sDMA* (.text*)
		AmbaAudio_Input* (.text*)
		AmbaAudio_InternalBuffer* (.text*)
		AmbaAudio_Output* (.text*)
		AmbaAudio_System* (.text*)
		AmbaAudio_Effect* (.rodata*)
		AmbaAudio_Enc* (.rodata*)
		AmbaAudio_I2sDMA* (.rodata*)
		AmbaAudio_Input* (.rodata*)
		AmbaAudio_InternalBuffer* (.rodata*)
		AmbaAudio_Output* (.rodata*)
		AmbaAudio_System* (.rodata*)

		AmbaAudioServices* (.text*)
		AmpDecMgr* (.text*)
		AudioDec* (.text*)
		AvcDec* (.text*)
		DecodeCommon* (.text*)
		StlDec* (.text*)
		AmpEncMgr* (.text*)
		AudioEnc* (.text*)
		BitstreamManager* (.text*)
		ExtEnc* (.text*)
		StillEnc* (.text*)
		StillProf* (.text*)
		StillRawCap* (.text*)
		StillRawEnc* (.text*)
		StillUtil* (.text*)
		VideoEnc* (.text*)
		DecTranscoder* (.text*)
		AmbaAudioServices* (.rodata*)
		AmpDecMgr* (.rodata*)
		AudioDec* (.rodata*)
		AvcDec* (.rodata*)
		DecodeCommon* (.rodata*)
		StlDec* (.rodata*)
		AmpEncMgr* (.rodata*)
		AudioEnc* (.rodata*)
		BitstreamManager* (.rodata*)
		ExtEnc* (.rodata*)
		StillEnc* (.rodata*)
		StillProf* (.rodata*)
		StillRawCap* (.rodata*)
		StillRawEnc* (.rodata*)
		StillUtil* (.rodata*)
		VideoEnc* (.rodata*)
		DecTranscoder* (.rodata*)
		
		BitOp* (.text*)
		ByteOp* (.text*)
		Format* (.text*)
		FormatAPI* (.text*)
		H264* (.text*)
		FifoPool* (.text*)
		MemByteOp* (.text*)
		Tiff* (.text*)
		Index* (.text*)
		IndexCache* (.text*)
		Mem* (.text*)
		Raw* (.text*)
		Temp* (.text*)
		IsoIdx* (.text*)
		FrameFeeder* (.text*)
		FeedingRuleDefault* (.text*)
		FeedingRuleH264* (.text*)
		File* (.text*)
		DefFilter* (.text*)
		BitOp* (.rodata*)
		ByteOp* (.rodata*)
		Format* (.rodata*)
		FormatAPI* (.rodata*)
		H264* (.rodata*)
		FifoPool* (.rodata*)
		MemByteOp* (.rodata*)
		Tiff* (.rodata*)
		Index* (.rodata*)
		IndexCache* (.rodata*)
		Mem* (.rodata*)
		Raw* (.rodata*)
		Temp* (.rodata*)
		IsoIdx* (.rodata*)
		FrameFeeder* (.rodata*)
		FeedingRuleDefault* (.rodata*)
		FeedingRuleH264* (.rodata*)
		File* (.rodata*)
		DefFilter* (.rodata*)
		


		
		ApplibVideoAnal* (.text*)
		ApplibVideoAnal* (.rodata*)
		
		AmbaDSP_ImgCalibItunerImpl* (.text*)
		AmbaDSP_ImgCalibItunerImpl* (.rodata*)
		
		ApplibUtility_SW_Scalar* (.text*)
		ApplibUtility_SW_Scalar* (.rodata*)
		ApplibUtility_SW_Scalar* (.constdata*)
		ApplibImage* (.text*)
		ApplibImage* (.rodata*)
		ApplibImage* (.constdata*)
		ApplibDcf* (.text*)
		ApplibDcf* (.rodata*)
		ApplibDcf* (.constdata*)
		
		ApplibEditor* (.text*)
		ApplibEditor* (.rodata*)
		ApplibEditor* (.constdata*)
		ApplibFormat* (.text*)
		ApplibFormat* (.rodata*)
		ApplibFormat* (.constdata*)
		
		ApplibDisplay* (.text*)
		ApplibDisplay* (.rodata*)
		ApplibDisplay* (.constdata*)
		ApplibStorage* (.text*)
		ApplibStorage* (.rodata*)
		ApplibStorage* (.constdata*)
		
		ApplibUsb* (.text*)
		ApplibUsb* (.rodata*)
		ApplibUsb* (.constdata*)
		
		AmpCfs* (.text*)
		AmpCfs* (.rodata*)
		AmpCfs* (.constdata*)
		AmpDcf* (.text*)
		AmpDcf* (.rodata*)
		AmpDcf* (.constdata*)

		AmbaUSB_Class* (.text*)
		AmbaUSB_Class* (.rodata*)
		AmbaUSB_Class* (.constdata*)
		AmbaUSB_System* (.text*)
		AmbaUSB_System* (.rodata*)
		AmbaUSB_System* (.constdata*)
		AmbaUSB_Descriptor* (.text*)
		AmbaUSB_Descriptor* (.rodata*)
		AmbaUSB_Descriptor* (.constdata*)
		AmbaUSB_Drv_IntHandler* (.text*)
		AmbaUSB_Drv_IntHandler* (.rodata*)
		AmbaUSB_Drv_IntHandler* (.constdata*)
		AmbaUSB_Drv_Transfer* (.text*)
		AmbaUSB_Drv_Transfer* (.rodata*)
		AmbaUSB_Drv_Transfer* (.constdata*)

		AmbaTUNE* (.text*)
		AmbaTUNE* (.rodata*)
		AmbaTUNE* (.constdata*)

		pf_w_fsfirst* (.text*)
		pf_w_fsfirst* (.rodata*)
		pf_w_fsfirst* (.constdata*)
		pf_w_fsnext* (.text*)
		pf_w_fsnext* (.rodata*)
		pf_w_fsnext* (.constdata*)
		pf_errnum* (.text*)
		pf_errnum* (.rodata*)
		pf_errnum* (.constdata*)
		pf_exfat* (.text*)
		pf_exfat* (.rodata*)
		pf_exfat* (.constdata*)
		pf_fclose* (.text*)
		pf_fclose* (.rodata*)
		pf_fclose* (.constdata*)
		pf_feof* (.text*)
		pf_feof* (.rodata*)
		pf_feof* (.constdata*)
		pf_fgetpos* (.text*)
		pf_fgetpos* (.rodata*)
		pf_fgetpos* (.constdata*)

		pf_fread* (.text*)
		pf_fread* (.rodata*)
		pf_fread* (.constdata*)
		pf_fseek* (.text*)
		pf_fseek* (.rodata*)
		pf_fseek* (.constdata*)
		pf_fsfirst* (.text*)
		pf_fsfirst* (.rodata*)
		pf_fsfirst* (.constdata*)
		pf_fsnext* (.text*)
		pf_fsnext* (.rodata*)
		pf_fsnext* (.constdata*)
		pf_fsync* (.text*)
		pf_fsync* (.rodata*)
		pf_fsync* (.constdata*)
		pf_fwrite* (.text*)
		pf_fwrite* (.rodata*)
		pf_fwrite* (.constdata*)
		pf_getdev* (.text*)
		pf_getdev* (.rodata*)
		pf_getdev* (.constdata*)
		pf_getvol* (.text*)
		pf_getvol* (.rodata*)
		pf_getvol* (.constdata*)
		pf_module* (.text*)
		pf_module* (.rodata*)
		pf_module* (.constdata*)
		pf_setvol* (.text*)
		pf_setvol* (.rodata*)
		pf_setvol* (.constdata*)
		pf_sync* (.text*)
		pf_sync* (.rodata*)
		pf_sync* (.constdata*)
		pf_unmount* (.text*)
		pf_unmount* (.rodata*)
		pf_unmount* (.constdata*)

		rec_connected_cam_f* (.text*)
		rec_connected_cam_f* (.rodata*)
		rec_connected_cam_f* (.constdata*)
		rec_connected_cam_o* (.text*)
		rec_connected_cam_o* (.rodata*)
		rec_connected_cam_o* (.constdata*)
		menu* (.text*)
		menu* (.rodata*)
		menu* (.constdata*)
		misc_* (.text*)
		misc_* (.rodata*)
		misc_* (.constdata*)
		widgetmgt* (.text*)
		widgetmgt* (.rodata*)
		widgetmgt* (.constdata*)
		dialog* (.text*)
		dialog* (.rodata*)
		dialog* (.constdata*)

		gui_rec_connected_cam* (.text*)
		gui_rec_connected_cam* (.rodata*)
		gui_rec_connected_cam* (.constdata*)
		gui_misc_* (.text*)
		gui_misc_* (.rodata*)
		gui_misc_* (.constdata*)
		clut* (.text*)
		clut* (.rodata*)
		clut* (.constdata*)
		gui_settle* (.text*)
		gui_settle* (.rodata*)
		gui_settle* (.constdata*)
		gui_utility* (.text*)
		gui_utility* (.rodata*)
		gui_utility* (.constdata*)
		gui_dialog* (.text*)
		gui_dialog* (.rodata*)
		gui_dialog* (.constdata*)
		__ddr_region1_ro_end = .;

		. = ALIGN(32);
		__ddr_region2_ro_start = .;
		ApplibFormat_Mux* (.text*)
		ApplibFormat_Mux* (.rodata*)
		ApplibFormat_Mux* (.constdata*)

		/*** MW ***/
		*Mux* (.text*)
		*Mux* (.rodata*)
		*Mux* (.constdata*)
		Mux* (.text*)
		Mux* (.rodata*)
		Mux* (.constdata*)
		__ddr_region2_ro_end = .;

		. = ALIGN(32);
		__ddr_region3_ro_start = .;
		__ddr_region3_ro_end = .;

		. = ALIGN(32);
		__ddr_region4_ro_start = .;	
		*(EXCLUDE_FILE (
			t_app*
		) .text*)
		
		*(EXCLUDE_FILE (
			t_app*
		) .rodata*)
		
		*(EXCLUDE_FILE (
			t_app*
		) .constdata*)
		__ddr_region4_ro_end = .;

		. = ALIGN(32);
		__ddr_region5_ro_start = .;
		t_app* (.text*)
		t_app* (.rodata*)
		t_app* (.constdata*)
		__ddr_region5_ro_end = .;
	}

	.dsp_buf : {
		. = ALIGN(0x1000);
		__dsp_buf_start = .;
		*AmbaLL_Buffers.o (*)
		__dsp_buf_end = .;
	}

	.data : {
		. = ALIGN(0x1000);
		__ddr_region0_rw_start = .;
		*AmbaBoot.o (.data*)
		*AmbaHwStack.o (.data*)
		* (.data*)
		__ddr_region0_rw_end = .;

		. = ALIGN(32);
		__ddr_region1_rw_start = .;
		__ddr_region1_rw_end = .;

		. = ALIGN(32);
		__ddr_region2_rw_start = .;
		__ddr_region2_rw_end = .;

		. = ALIGN(32);
		__ddr_region3_rw_start = .;
		__ddr_region3_rw_end = .;

		. = ALIGN(32);
		__ddr_region4_rw_start = .;
		__ddr_region4_rw_end = .;

		. = ALIGN(32);
		__ddr_region5_rw_start = .;
		__ddr_region5_rw_end = .;
	}

#ifdef CONFIG_CC_CXX_SUPPORT
	. = ALIGN(8);
	/*
	 * Stack unwinding tables and exception tables for C++
	 */
	.ARM.unwind_idx : {
		__exidx_start = .;
		*(.ARM.exidx.text.*)
		*unwind-arm.o (.ARM.exidx*)
		*pr-support.o (.ARM.exidx*)
		__exidx_end = .;
	}
	.ARM.unwind_tab : {
		__start_unwind_tab = .;
		*(.ARM.extab*)
		__stop_unwind_tab = .;
	}
	/* define a global symbol _etext just after the last code byte */
/*
	_etext = .;
*/
#endif

	/* Provided malloc heap for libc. */
	PROVIDE (end = .);
	__sbrk_start = .;
	. = __sbrk_start + 0x2000;
	__sbrk_end = .;

	.bss.noinit : {
		. = ALIGN(32);
		__noinit_start = .;
		* (.bss.noinit*)
		__noinit_end = .;
	}

	.bss : {
		. = ALIGN(32);
		__bss_start = .;
		* (.bss*)
		* (COMMON)
		__bss_end = .;
	}

	/* FIXME: There are some unused memory here ! */

	/***********************/
	/* ucode orcode region */
	/***********************/
	. = ALIGN(0x100000);
	__ucode_start = .;
	__ucode_orcode_start = .;
	. = __ucode_orcode_start + 0x480000;
	__ucode_orcode_end = .;

	. = ALIGN(32);
	__ucode_orcme_start = .;
	. = __ucode_orcme_start + 0x80000;
	__ucode_orcme_end = .;

	. = ALIGN(32);
	__ucode_orcmd_start = .;
	. = __ucode_orcmd_start + 0x80000;
	__ucode_orcmd_end = .;

	. = ALIGN(32);
	__ucode_default_data_start = .;
	. = __ucode_default_data_start + 0x80000;
	__ucode_default_data_end = .;
	__ucode_end = .;

	/***************************/
	/*  non-cached heap region */
	/***************************/
	. = ALIGN(0x1000);
	__non_cache_heap_start = .;
	. = __non_cache_heap_start + 0x1000000;
	__non_cache_heap_end = .;


	/***************************/
	/*  cached heap region     */
	/***************************/
	. = ALIGN(0x1000);
	__cache_heap_start = .;
	. = __ddr_start + CONFIG_DDR_SIZE - LINUX_SIZE;
	__cache_heap_end = .;

	/*************************/
	/* linux region          */
	/*************************/
	. = ALIGN(0x100000);
	__linux_start = .;
	. = __linux_start + LINUX_SIZE;
	__linux_end = .;

	/*************************/
	/* DDR end               */
	/*************************/

	/*
	 * Provide a end symbol for newlib malloc.
	 * We don't use malloc for newlib, so it should be ok.
	 */
	PROVIDE (end = .);

	. = __ddr_start + CONFIG_DDR_SIZE;
	__ddr_end = .;
}
