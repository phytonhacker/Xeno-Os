;==============================================================================
; MyOS Kernel - BILLENTYŰZET INTERRUPT
;==============================================================================
[BITS 16]
[ORG 0x0000]

kernel_start:
    cli
    
    ; DS = CS (hogy az adatok elérhetők legyenek)
    push cs
    pop ds
    
    ; IVT beállítás (ES = 0)
    xor ax, ax
    mov es, ax
    mov word [es:0x24], keyboard_handler  ; offset (IRQ1 = INT 0x09)
    mov word [es:0x26], cs                ; szegmens
    
    sti
    
    ; Képernyő törlés
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    
    ; Üzenet kiírása
    mov si, kernel_msg
    call print
    
.loop:
    hlt
    jmp .loop

;------------------------------------------------------------------------------
; BILLENTYŰZET INTERRUPT HANDLER
;------------------------------------------------------------------------------
keyboard_handler:
    push ax
    push si
    push ds
    
    ; DS = CS (hogy scancodes elérhető legyen)
    push cs
    pop ds
    
    ; Scan code olvasása
    in al, 0x60
    
    ; Felengedés ellenőrzése (>= 0x80)
    cmp al, 0x80
    jae .release
    
    ; ASCII kód keresése a táblázatban
    mov si, scancodes
    xor ah, ah
    add si, ax
    lodsb
    
    ; Ha 0, akkor nincs ASCII megfelelő
    cmp al, 0
    je .end
    
    ; Karakter kiírása
    mov ah, 0x0E
    mov bh, 0           ; videó lap 0
    int 0x10
    
.release:
.end:
    ; EOI küldése a PIC-nek (8259)
    mov al, 0x20
    out 0x20, al
    
    pop ds
    pop si
    pop ax
    iret

;------------------------------------------------------------------------------
; PRINT FÜGGVÉNY
;------------------------------------------------------------------------------
print:
    mov ah, 0x0E
.print_loop:
    lodsb
    cmp al, 0
    je .print_done
    int 0x10
    jmp .print_loop
.print_done:
    ret

;------------------------------------------------------------------------------
; ADATOK
;------------------------------------------------------------------------------
kernel_msg db 'X-DOS loaded - Type something!', 13, 10, 0

scancodes:
    ; Scan code -> ASCII táblázat
    db 0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0
    db 'q','w','e','r','t','y','u','i','o','p','[',']',13,0
    db 'a','s','d','f','g','h','j','k','l',';',"'",0,0,'\'
    db 'z','x','c','v','b','n','m',',','.','/',0,'*',0,' '

;------------------------------------------------------------------------------
; PADDING
;------------------------------------------------------------------------------
times 8192-($-$$) db 0