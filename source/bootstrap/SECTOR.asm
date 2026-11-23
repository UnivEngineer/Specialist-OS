; Заголовок MXOS-256/FAT16 занимает 3Eh байт. Исторический шаблон сохранён.
    IFDEF STAGED_BOOT
    INCLUDE "../../bin/bootstrap/sector-config.inc"
    ENDIF
    ORG 0
    jp boot
    DB 053h,050h,045h,054h,053h,04Dh,058h,032h,000h,001h,001h,001h,000h,001h,000h,002h
    DB 000h,020h,0F8h,040h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
    DB 000h,080h,000h,029h,000h,000h,000h,000h,053h,059h,053h,054h,045h,04Dh,020h,052h
    DB 04Fh,04Dh,020h,046h,041h,054h,031h,036h,020h,020h,020h

;----------------------------------------------------------------------------
; MXOS
;
; Передать управление следующей ступени системной загрузки.
;
; вход:
;   Исполняется сектор по адресу 0. Для STAGED_BOOT адрес и длину BOOT.SYS задаёт
;   сборщик образа. Рабочий стек вызывающего не требуется.
;
; выход:
;   В STAGED_BOOT экран очищен и управление передано копии BOOT.SYS по BOOT_STAGE_BASE.
;   В линейном варианте первые 16 КиБ скопированы на C000h и выполнен переход туда.
;   Возврата через RET нет.
;----------------------------------------------------------------------------

boot:
    IFDEF STAGED_BOOT
    ; No calls or RAM variables before the screen has been cleared.
    di
    ld a,0F0h
    ld (BOOT_COLOR_PORT),a
    ld sp,0C000h
    ld hl,0
    ld bc,03000h/16
clearScreen:
    DUP 8
        push hl
    EDUP
    dec bc
    ld a,b
    or c
    jp nz,clearScreen
screenCleared:
    ; BOOT.SYS runs from RAM; the ROM slot remains available for its data.
    ld hl,02000h
    ld de,BOOT_STAGE_BASE
    ld bc,BOOT_STAGE_BYTES
copyBoot:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,copyBoot
    jp BOOT_STAGE_BASE
    ELSE
    ld hl,0
    ld de,0C000h
copy:
    ld a,(hl)
    ld (de),a
    inc de
    inc hl
    ld a,d
    cp 0
    jp nz,copy
    jp 0C000h
    ENDIF
    DS 256-$,0FFh
