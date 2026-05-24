global idt_flush
global isr_keyboard
global isr_timer        ; new — dummy timer handler
global isr_default      ; default handler for unhandled interrupts
extern irq0_handler   ; new — dummy timer handler
extern keyboard_handler   ; new — dummy keyboard handler

idt_flush:
    mov eax, [esp+4]    ; grab idt_ptr address from stack
    lidt [eax]          ; load IDT into CPU
    sti                 ; enable interrupts
    ret

isr_default:
    pusha               ; save all registers
    mov al, 0x20         ; EOI (End of Interrupt) command code
    out 0x20, al        ; send EOI to PIC — "handled, send more"
    out 0xA0, al        ; send EOI to slave PIC as well (in case it's a slave interrupt)
    popa                ; restore all registers
    iret                ; return from interrupt


isr_timer:
    pusha               ; save all registers
    call irq0_handler   ; call the timer handler (dummy for now)
    mov al, 0x20
    out 0x20, al        ; send EOI to PIC — "timer handled, send more"
    popa                ; restore all registers
        iret                ; return from interrupt

    isr_keyboard:
        pusha
        call keyboard_handler  ; call the keyboard handler
        mov al, 0x20
        out 0x20, al        ; send EOI to PIC
        popa
        iret
