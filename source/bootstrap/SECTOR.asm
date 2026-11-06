; Заголовок MXOS-256/FAT16 занимает 3Eh байт. Исторический шаблон сохранён.
    ORG 0
    jp boot
    DB 053h,050h,045h,054h,053h,04Dh,058h,032h,000h,001h,001h,001h,000h,001h,000h,002h
    DB 000h,020h,0F8h,040h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
    DB 000h,080h,000h,029h,000h,000h,000h,000h,053h,059h,053h,054h,045h,04Dh,020h,052h
    DB 04Fh,04Dh,020h,046h,041h,054h,031h,036h,020h,020h,020h
boot:
    IFDEF STAGED_BOOT
    jp 02000h
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
