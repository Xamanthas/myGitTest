cmd_tools/hostutils/host_sdfw_gen := gcc -Wp,-MD,tools/hostutils/.host_sdfw_gen.d -Itools/hostutils -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -fomit-frame-pointer -m32   -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp   -I/home/bingo/RD_A12/release_rtos/output/build/include/generated -o tools/hostutils/host_sdfw_gen /home/bingo/RD_A12/release_rtos/tools/hostutils/host_sdfw_gen.c  

source_tools/hostutils/host_sdfw_gen := /home/bingo/RD_A12/release_rtos/tools/hostutils/host_sdfw_gen.c

deps_tools/hostutils/host_sdfw_gen := \
  /usr/include/stdc-predef.h \
  /usr/include/stdio.h \
  /usr/include/features.h \
  /usr/include/sys/cdefs.h \
  /usr/include/bits/wordsize.h \
  /usr/include/gnu/stubs.h \
  /usr/include/gnu/stubs-32.h \
  /usr/lib/gcc/x86_64-linux-gnu/4.8/include/stddef.h \
  /usr/include/bits/types.h \
  /usr/include/bits/typesizes.h \
  /usr/include/libio.h \
  /usr/include/_G_config.h \
  /usr/include/wchar.h \
  /usr/lib/gcc/x86_64-linux-gnu/4.8/include/stdarg.h \
  /usr/include/bits/stdio_lim.h \
  /usr/include/bits/sys_errlist.h \
  /usr/include/bits/stdio.h \
  /usr/include/bits/stdio2.h \
  /usr/include/string.h \
  /usr/include/xlocale.h \
  /usr/include/bits/string.h \
  /usr/include/bits/string2.h \
  /usr/include/endian.h \
  /usr/include/bits/endian.h \
  /usr/include/bits/byteswap.h \
  /usr/include/bits/byteswap-16.h \
  /usr/include/stdlib.h \
  /usr/include/bits/string3.h \
  /usr/include/fts.h \
  /usr/include/sys/types.h \
  /usr/include/time.h \
  /usr/include/sys/select.h \
  /usr/include/bits/select.h \
  /usr/include/bits/sigset.h \
  /usr/include/bits/time.h \
  /usr/include/bits/select2.h \
  /usr/include/sys/sysmacros.h \
  /usr/include/bits/pthreadtypes.h \
  /usr/include/sys/stat.h \
  /usr/include/bits/stat.h \
  /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h \
  /usr/include/bits/environments.h \
  /usr/include/bits/confname.h \
  /usr/include/getopt.h \
  /usr/include/bits/unistd.h \
  /home/bingo/RD_A12/release_rtos/bsp/dragonfly/bsp.h \
    $(wildcard build/include/config/enable/spinor/boot.h) \
    $(wildcard build/include/config/build/amba/mw/unittest.h) \

tools/hostutils/host_sdfw_gen: $(deps_tools/hostutils/host_sdfw_gen)

$(deps_tools/hostutils/host_sdfw_gen):
