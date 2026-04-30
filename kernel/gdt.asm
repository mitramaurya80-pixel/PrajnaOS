global gdt_flush       ; make this function visible to C code

gdt_flush:
    ; when C calls gdt_flush(&gp), the stack looks like:
    ;   esp+0 = return address
    ;   esp+4 = &gp (our argument)

    mov eax, [esp+4]
    ; grab the argument — address of our gdt_ptr struct

    lgdt [eax]
    ; tell CPU where the GDT is
    ; BUT segment registers are still old values after this!

    mov ax, 0x10
    ; 0x10 = entry 2 in GDT = kernel data segment
    ; (entry 2 × 8 bytes per entry = offset 16 = 0x10)

    mov ds, ax   ; data segment
    mov es, ax   ; extra segment
    mov fs, ax   ; general segment
    mov gs, ax   ; general segment
    mov ss, ax   ; stack segment

    jmp 0x08:.flush
    ; FAR JUMP — the only way to reload the CS register
    ; 0x08 = entry 1 in GDT = kernel code segment
    ; lands immediately at .flush label below

.flush:
    ret
    ; GDT fully loaded, all segment registers updated
    ; return back to gdt_init() in C
