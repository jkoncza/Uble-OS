ENTRY(_start)

SECTIONS {
    . = 1M;

    /* Multiboot header MUST be first */
    .multiboot : {
        *(.multiboot)
    }

    .text : {
        *(.text*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }

    . = ALIGN(4096);
    .stack : {
        *(.stack)
    }
}
