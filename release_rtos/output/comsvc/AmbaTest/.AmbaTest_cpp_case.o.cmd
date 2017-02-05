cmd_comsvc/AmbaTest/AmbaTest_cpp_case.o := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-g++ -Wp,-MD,comsvc/AmbaTest/.AmbaTest_cpp_case.o.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly -I/home/bingo/RD_A12/release_rtos/comsvc/AmbaTest -Icomsvc/AmbaTest -mlittle-endian -DAMBA_KAL_NO_SMP -Wall -Wundef -Wno-trigraphs -fno-strict-aliasing -fno-common -Wno-format-security -fno-delete-null-pointer-checks -fshort-enums -fshort-wchar -D"SOC=KBUILD_STR(A12)" -O2 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfpv3 -marm -march=armv7-a -mcpu=cortex-a9 -mhard-float -ffunction-sections -fdata-sections -Wno-missing-braces -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -g -gdwarf-3 -fno-strict-overflow -fconserve-stack -Werror=implicit-int -Werror=strict-prototypes -DCC_HAVE_ASM_GOTO -DAMBA_KAL_NO_SMP -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaTest -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/MW -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/RTSL -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/fs -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/kal -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaShell -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaSys -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/misc    -fno-exceptions -fno-rtti -fno-use-cxa-atexit -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(AmbaTest_cpp_case)"  -D"KBUILD_MODNAME=KBUILD_STR(libcomsvc_test)" -c -o comsvc/AmbaTest/AmbaTest_cpp_case.o /home/bingo/RD_A12/release_rtos/comsvc/AmbaTest/AmbaTest_cpp_case.cpp

source_comsvc/AmbaTest/AmbaTest_cpp_case.o := /home/bingo/RD_A12/release_rtos/comsvc/AmbaTest/AmbaTest_cpp_case.cpp

deps_comsvc/AmbaTest/AmbaTest_cpp_case.o := \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/iostream \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb/fpu/bits/c++config.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb/fpu/bits/os_defines.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb/fpu/bits/cpu_defines.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/ostream \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/ios \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/iosfwd \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stringfwd.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/memoryfwd.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/postypes.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/cwchar \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/wchar.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/_ansi.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/newlib.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/config.h \
    $(wildcard build/include/config/h//.h) \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/ieeefp.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/features.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/reent.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/lib/gcc/arm-none-eabi/4.9.3/include/stddef.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/_types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/_types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/_default_types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/lock.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/lib/gcc/arm-none-eabi/4.9.3/include/stdarg.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/cdefs.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/exception \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/atomic_lockfree_defines.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/char_traits.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_algobase.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/functexcept.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/exception_defines.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/cpp_type_traits.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/ext/type_traits.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/ext/numeric_traits.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_pair.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/move.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/concept_check.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_iterator_base_types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_iterator_base_funcs.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/debug/debug.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_iterator.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/ptr_traits.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/predefined_ops.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/localefwd.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb/fpu/bits/c++locale.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/clocale \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/locale.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/_ansi.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/cctype \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/ctype.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/ios_base.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/ext/atomicity.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb/fpu/bits/gthr.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb/fpu/bits/gthr-default.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb/fpu/bits/atomic_word.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/locale_classes.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/string \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/allocator.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb/fpu/bits/c++allocator.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/ext/new_allocator.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/new \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/ostream_insert.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/cxxabi_forced.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_function.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/backward/binders.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/range_access.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/basic_string.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/basic_string.tcc \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/locale_classes.tcc \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/streambuf \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/streambuf.tcc \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/basic_ios.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/locale_facets.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/cwctype \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/wctype.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb/fpu/bits/ctype_base.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/streambuf_iterator.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb/fpu/bits/ctype_inline.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/locale_facets.tcc \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/basic_ios.tcc \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/ostream.tcc \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/istream \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/istream.tcc \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/vector \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_construct.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/ext/alloc_traits.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_uninitialized.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_vector.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_bvector.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/vector.tcc \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/algorithm \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/utility \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_relops.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_algo.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/cstdlib \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/stdlib.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/stdlib.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/alloca.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/algorithmfwd.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_heap.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/c++/4.9.3/bits/stl_tempbuf.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/AmbaDataType.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/AmbaKAL.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_api_Uniprocessor.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_port_Uniprocessor.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_user.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/string.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/string.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_execution_profile.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaSys/AmbaPrintk.h \
    $(wildcard build/include/config/s/.h) \
    $(wildcard build/include/config/s.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/intrinsics.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/lib/gcc/arm-none-eabi/4.9.3/include/stdint.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/stdint.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/_intsup.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaSysTimer.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaSys/AmbaPrint.h \

comsvc/AmbaTest/AmbaTest_cpp_case.o: $(deps_comsvc/AmbaTest/AmbaTest_cpp_case.o)

$(deps_comsvc/AmbaTest/AmbaTest_cpp_case.o):
