; boot.s - 32-bit Multiboot-compliant entry

BITS 32

section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002            ; magic
    dd 0x0                   ; flags
    dd -(0x1BADB002 + 0x0)   ; checksum

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    call kernel_main

.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384               ; 16 KB stack
stack_top:
