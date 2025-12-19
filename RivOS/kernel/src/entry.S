    .section .text.entry
    .globl _start
_start:
    la sp, boot_stack_top
    call kmain
1:  j 1b

    .section .bss
    .align 16
boot_stack:
    .space 4096 * 4
boot_stack_top:
