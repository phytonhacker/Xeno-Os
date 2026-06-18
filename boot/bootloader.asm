;==============================================================================
; bootloader.asm
;==============================================================================
[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    ; Képernyő törlés
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    mov si, msg_boot
    call print16

    ; Kernel betöltése 0x1000:0x0000 = fizikai 0x10000
    call load_kernel

    mov si, msg_prot
    call print16

    cli

    ; A20 vonal engedélyezése
    in  al, 0x92
    or  al, 0x02
    out 0x92, al

    ; GDT betöltése
    lgdt [gdt_descriptor]

    ; Protected mode
    mov eax, cr0
    or  eax, 0x1
    mov cr0, eax

    ; Far jump -> flush, 32 bites kód szegmens
    jmp 0x08:pmode_entry

;------------------------------------------------------------------------------
[BITS 16]
print16:
    push ax
    mov ah, 0x0E
.lp:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .lp
.done:
    pop ax
    ret

;------------------------------------------------------------------------------
load_kernel:
    ; Reset lemez
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13

    ; Betöltés 0x1000:0x0000-re
    mov ax, 0x1000
    mov es, ax
    xor bx, bx

    mov ah, 0x02        ; olvasás
    mov al, 32          ; 32 szektor = 16KB
    mov ch, 0           ; track 0
    mov cl, 2           ; 2. szektortól
    mov dh, 0           ; fej 0
    mov dl, [boot_drive]
    int 0x13

    jnc .ok
    mov si, msg_err
    call print16
    jmp $
.ok:
    ret

;==============================================================================
; GDT
;==============================================================================
gdt_start:
    dq 0                        ; null descriptor

    ; kód szegmens: base=0, limit=4GB, 32bit, ring0
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00

    ; adat szegmens: base=0, limit=4GB, 32bit, ring0
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

;==============================================================================
; PROTECTED MODE BELÉPÉS (32 bit)
;==============================================================================
[BITS 32]
pmode_entry:
    mov ax, 0x10        ; adat szegmens selektor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9F000    ; stack

    jmp 0x10000         ; kernel _start

;------------------------------------------------------------------------------
[BITS 16]
boot_drive: db 0
msg_boot:   db 'MyOS Booting...', 13, 10, 0
msg_prot:   db 'Entering protected mode...', 13, 10, 0
msg_err:    db 'DISK ERROR!', 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55