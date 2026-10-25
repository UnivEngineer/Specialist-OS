;+---------------------------------------------------------------------------
; RAMFOS
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

    IF BIG_ROM != 1
		
    IF $ > 0FFB6h
    ASSERT 0
    ENDIF
    ORG_PAD0 0FFB6h

pageWriteROM:	ld (IO_PAGE_ROM), a
		jp pageWriteCom

pageReadROM:	ld (IO_PAGE_ROM), a
		jp pageReadCom

    ENDIF

; ---------------------------------------------------------------------------

    IF $ > 0FFC2h
    ASSERT 0
    ENDIF
    ORG_PAD0 0FFC2h

pageRead:	ld a, (v_page)

;----------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа F836
; Чтение байта из ДОЗУ
;
; На входе
;   a  - номер страницы
;   hl - адрес
;
; На выходе
;   с  - байт
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

pageReadAny:	or a
    IF BIG_ROM
		jp m, bigRom
    ELSE
		jp m, pageReadROM
    ENDIF
disableARAM1:	ld (IO_PAGE_ARAM), a
pageReadCom:	ld c, (hl)
		ld (IO_PAGE_RAM), a
		ret

; ---------------------------------------------------------------------------

pageWrite:	ld a, (v_page)

;----------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа F839
; Запись байта в ДОЗУ
;
; На входе
;   a  - номер страницы
;   hl - адрес
;   c  - байт
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

pageWriteAny:	or a
    IF BIG_ROM
		jp m, pageWriteRet
    ELSE
		jp m, pageWriteROM
    ENDIF
disableARAM2:	ld (IO_PAGE_ARAM), a
pageWriteCom:	ld (hl), c
		ld (IO_PAGE_RAM), a
pageWriteRet:	ret
