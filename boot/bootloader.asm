;==============================================================================
; MyOS Bootloader
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

    mov si, msg1
    call print

    ; Kernel betöltés
    call load_kernel

    mov si, msg2
    call print

    ; === JAVÍTOTT BILLENTYŰ VÁRAKOZÁS ===
    call wait_key

    mov si, msg3
    call print

    ; Szegmensek ugrás előtt
    mov ax, 0x1000
    mov ds, ax
    mov es, ax

    ; Ugrás kernelre
    jmp 0x1000:0x0000

;------------------------------------------------------------------------------
print:
    mov ah, 0x0E
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

;------------------------------------------------------------------------------
; JAVÍTOTT billentyű várakozás
;------------------------------------------------------------------------------
wait_key:
    ; Billentyűzet buffer ürítése
.clear:
    mov ah, 0x01
    int 0x16
    jz .wait          ; Ha üres, várunk
    mov ah, 0x00
    int 0x16          ; Karakter kivétele
    jmp .clear

.wait:
    ; Várakozás új billentyűre
    mov ah, 0x01
    int 0x16
    jz .wait          ; Amíg nincs, várunk
    
    ; Billentyű kiolvasása
    mov ah, 0x00
    int 0x16
    ret

;------------------------------------------------------------------------------
load_kernel:
    mov ax, 0x1000
    mov es, ax
    xor bx, bx

    mov dl, [boot_drive]
    call try_read
    jnc .success

    mov dl, 0x00
    call try_read
    jnc .success

    mov dl, 0x80
    call try_read
    jnc .success

    mov si, msg_err
    call print
    jmp $

.success:
    ret

;------------------------------------------------------------------------------
try_read:
    mov cx, 3

.retry:
    push cx

    mov ah, 0x00
    int 0x13

    mov ax, 0x1000
    mov es, ax
    xor bx, bx

    mov ah, 0x02
    mov al, 32
    mov ch, 0
    mov cl, 2
    mov dh, 0
    int 0x13

    pop cx
    jnc .ok

    loop .retry
    stc
    ret

.ok:
    clc
    ret

;------------------------------------------------------------------------------
boot_drive: db 0
msg1:       db 'MyOS Booting...', 13, 10, 0
msg2:       db 'Press any key...', 13, 10, 0
msg3:       db 'Starting kernel...', 13, 10, 0
msg_err:    db 'DISK ERROR!', 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55