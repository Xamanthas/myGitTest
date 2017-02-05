cmd_ssp/amboot/bld/amba_bld.lds := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -E -Wp,-MD,ssp/amboot/bld/.amba_bld.lds.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly -mlittle-endian -DAMBA_KAL_NO_SMP -P -C -Uarm -D__ASSEMBLY__ -DLINKER_SCRIPT -o ssp/amboot/bld/amba_bld.lds /home/bingo/RD_A12/release_rtos/ssp/amboot/bld/amba_bld.lds.S

source_ssp/amboot/bld/amba_bld.lds := /home/bingo/RD_A12/release_rtos/ssp/amboot/bld/amba_bld.lds.S

deps_ssp/amboot/bld/amba_bld.lds := \
    $(wildcard build/include/config/bld/loadaddr.h) \
    $(wildcard build/include/config/mmu.h) \
    $(wildcard build/include/config/arm/unwind.h) \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \

ssp/amboot/bld/amba_bld.lds: $(deps_ssp/amboot/bld/amba_bld.lds)

$(deps_ssp/amboot/bld/amba_bld.lds):
