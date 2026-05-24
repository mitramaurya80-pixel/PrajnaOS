section .multiboot ; Multiboot header
align 4 ; align to 4 bytes
    dd 0x1BADB002        ; magic number
    dd 0x00              ; flags (we can set this to 0 for now)
    dd -(0x1BADB002)     ; checksum (should be the negative of the magic number)

section .bss
    resb 4096 ; reserve 4096 bytes for the kernel stack
stack_top: ; this label marks the top of the stack
section .text
global _start
extern kernel_main ; declare the kernel_main function defined in C

_start:
    cli     ; disable interrupts
    mov esp, stack_top ; set the stack pointer to the top of the stack
    mov eax, 0xDEADBEEF     ; put test value in eax
    push eax                 ; push to stack
    pop ebx                  ; pop from stack
    ; if stack works ebx = 0xDEADBEEF
    call kernel_main ; call the kernel main function
    hlt ;     ; halt the CPU if kernel_main returns