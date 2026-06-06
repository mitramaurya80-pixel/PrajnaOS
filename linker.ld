ENTRY(_start)
SECTIONS {
    . = 1M;
    .text  : { *(.multiboot) *(.text) }
    .data  : { *(.data) }
    .bss   : { 
        *(.bss)
        . = ALIGN(4096);
        kernel_end = .;
    }
}
