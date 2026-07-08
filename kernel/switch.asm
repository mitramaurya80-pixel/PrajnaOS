global context_switch

context_switch:
    ; save current task registers
    push ebp 
    push ebx
    push esi
    push edi

    ; get argument 1 — save current ESP
    mov eax, [esp + 20]   ; old_esp pointer
    mov [eax], esp         ; save current ESP

    ; get argument 2 — new ESP
    mov eax, [esp + 24]   ; new_esp value — save in eax BEFORE changing esp

    ; switch stack
    mov esp, eax           ; load new task's stack

    ; restore next task's registers
    pop edi
    pop esi
    pop ebx
    pop ebp

    sti   ; enable interrupts before returning to next task

    ret   ; jump to next task's entry point