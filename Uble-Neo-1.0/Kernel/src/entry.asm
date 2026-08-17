bits 64

section .text

global _start
extern kmain

_start:
    lea rsp, [rel stack_top]
    and rsp, -16

    call kmain

.hang:
    cli
    hlt
    jmp .hang

section .bss

align 16

stack_bottom:
    resb 16384

stack_top:

section .note.GNU-stack noalloc noexec nowrite progbits