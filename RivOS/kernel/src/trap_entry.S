    .section .text
    .globl trap_entry
trap_entry:
    csrr a0, scause
    csrr a1, sepc
    csrr a2, stval
    call trap_handler
1:  j 1b
