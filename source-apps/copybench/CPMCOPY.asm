; Копировщик Syabr CP/M 0.1, побайтное восстановление 164 байт.
; FF означает основную память, 0..7 физические страницы.
; FFD0 источник, FFD2 назначение, FFD4 число порций по 16 байт.
; FFD6 страница источника, FFD7 назначения, FFD8 возврата.
; Окно FFC0..FFCF — буфер. SP сохраняется локально по FFA2.
; Прерывания выключаются, регистры разрушены; вложенность не поддерживается.
    ORG 0FF00h
    di
    ld      hl,00000h
    add     hl,sp
    ld      (0FFA2h),hl
    ld      a,(0FFD6h)
    cp      0FFh
    jp      z,0FF16h
    ld      (0FFFDh),a
    jp      0FF19h
    ld      (0FFFCh),a
    ld      hl,(0FFD0h)
    ld      sp,hl
    ld      de,00010h
    add     hl,de
    ld      (0FFD0h),hl
    pop     hl
    ld      (0FFC0h),hl
    pop     hl
    ld      (0FFC2h),hl
    pop     hl
    ld      (0FFC4h),hl
    pop     hl
    ld      (0FFC6h),hl
    pop     hl
    ld      (0FFC8h),hl
    pop     hl
    ld      (0FFCAh),hl
    pop     hl
    ld      (0FFCCh),hl
    pop     hl
    ld      (0FFCEh),hl
    ld      a,(0FFD7h)
    cp      0FFh
    jp      z,0FF52h
    ld      (0FFFDh),a
    jp      0FF55h
    ld      (0FFFCh),a
    ld      hl,(0FFD2h)
    ld      de,00010h
    add     hl,de
    ld      (0FFD2h),hl
    ld      sp,hl
    ld      hl,(0FFCEh)
    push    hl
    ld      hl,(0FFCCh)
    push    hl
    ld      hl,(0FFCAh)
    push    hl
    ld      hl,(0FFC8h)
    push    hl
    ld      hl,(0FFC6h)
    push    hl
    ld      hl,(0FFC4h)
    push    hl
    ld      hl,(0FFC2h)
    push    hl
    ld      hl,(0FFC0h)
    push    hl
    ld      hl,(0FFD4h)
    dec     hl
    ld      (0FFD4h),hl
    ld      a,h
    or      l
    jp      nz,0FF08h
    ld      a,(0FFD8h)
    cp      0FFh
    jp      z,0FF9Ah
    ld      (0FFFDh),a
    jp      0FF9Dh
    ld      (0FFFCh),a
    ld      hl,(0FFA2h)
    ld      sp,hl
    ret
    nop
    nop
