cmd_ssp/amboot/bst/amba_bst.lds := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -E -Wp,-MD,ssp/amboot/bst/.amba_bst.lds.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly -mlittle-endian -DAMBA_KAL_NO_SMP -P -C -Uarm -D__ASSEMBLY__ -DLINKER_SCRIPT -o ssp/amboot/bst/amba_bst.lds /home/bingo/RD_A12/release_rtos/ssp/amboot/bst/amba_bst.lds.S

source_ssp/amboot/bst/amba_bst.lds := /home/bingo/RD_A12/release_rtos/ssp/amboot/bst/amba_bst.lds.S

deps_ssp/amboot/bst/amba_bst.lds := \
    $(wildcard build/include/config/enable/spinor/boot.h) \
    $(wildcard build/include/config/enable/emmc/boot.h) \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \

ssp/amboot/bst/amba_bst.lds: $(deps_ssp/amboot/bst/amba_bst.lds)

$(deps_ssp/amboot/bst/amba_bst.lds):
