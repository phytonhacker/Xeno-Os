;==============================================================================
; kernel_entry.asm
;==============================================================================
[BITS 32]

extern kernel_main
extern keyboard_handler_c

global _start
global keyboard_isr

section .text

_start:
    ; Szegmensek és stack már be van állítva a bootloaderben
    ; Csak meghívjuk a C kernelt
    call kernel_main

.halt:
    cli
    hlt
    jmp .halt

;--- Billentyűzet ISR wrapper -------------------------------------------------
keyboard_isr:
    pushad
    call keyboard_handler_c
    popad
    iret