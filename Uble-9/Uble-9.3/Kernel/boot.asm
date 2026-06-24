; boot.s - 32-bit Multiboot-compliant entry with Linear Framebuffer Graphics

BITS 32

; --- MULTIBOOT FLAGS CONFIURATION ---
; Bit 0: Align modules on page boundaries
; Bit 1: Provide memory info to kernel_main
; Bit 2: Request a video mode layout (CRITICAL FOR GRAPHICS)
FLAGS       equ 0x00000007 
MAGIC       equ 0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
multiboot_header:
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    
    ; These placeholders are required when Bit 2 of FLAGS is set
    dd 0, 0, 0, 0, 0
    
    ; Video Mode Specifications (Matches kernel.c expectations)
    dd 0        ; Mode type: 0 = Linear Framebuffer Graphics
    dd 800      ; Requested Screen Width (Pixels)
    dd 600      ; Requested Screen Height (Pixels)
    dd 32       ; Requested Color Depth (32-Bit ARGB)

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    
    ; Multiboot places a pointer to the MultibootInfo struct in EBX.
    ; We must push it onto the stack so kernel_main(MultibootInfo* mbi) can receive it!
    push ebx    
    
    call kernel_main

.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384               ; 16 KB stack
stack_top: