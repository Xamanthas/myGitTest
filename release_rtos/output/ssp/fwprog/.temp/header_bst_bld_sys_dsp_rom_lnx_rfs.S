



.text

.globl begin_bst_image
.globl end_bst_image
.globl begin_bld_image
.globl end_bld_image
.globl begin_pba_image
.globl end_pba_image
.globl begin_sys_image
.globl end_sys_image
.globl begin_dsp_image
.globl end_dsp_image
.globl begin_rom_image
.globl end_rom_image
.globl begin_lnx_image
.globl end_lnx_image
.globl begin_dtb_image
.globl end_dtb_image
.globl begin_rfs_image
.globl end_rfs_image



begin_bst_image: .word __BEGIN_BST_IMAGE__





begin_bld_image: .word __BEGIN_BLD_IMAGE__







begin_pba_image: .word 0x0



begin_sys_image: .word __BEGIN_SYS_IMAGE__





begin_rom_image: .word __BEGIN_ROM_IMAGE__





begin_dsp_image: .word __BEGIN_DSP_IMAGE__





begin_lnx_image: .word __BEGIN_LNX_IMAGE__





begin_dtb_image: .word __BEGIN_DTB_IMAGE__





begin_rfs_image: .word __BEGIN_RFS_IMAGE__







end_bst_image: .word __END_BST_IMAGE__





end_bld_image: .word __END_BLD_IMAGE__







end_pba_image: .word 0x0



end_sys_image: .word __END_SYS_IMAGE__





end_rom_image: .word __END_ROM_IMAGE__





end_dsp_image: .word __END_DSP_IMAGE__





end_lnx_image: .word __END_LNX_IMAGE__





end_dtb_image: .word __END_DTB_IMAGE__





end_rfs_image: .word __END_RFS_IMAGE__
