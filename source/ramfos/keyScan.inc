;----------------------------------------------------------------------------
; RAMFOS
; Сканирование клавиатуры
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

keyScan:		 ; Сохраняем регистры BC, DE, HL
		push hl
		push de
		push bc
		 call	keyScan_0
		pop bc
		pop de
		pop hl

		; Флаг Z
		or a
		
keyMode82:	 ; Восстаналиваем индикатор РУС/LAT
		; Вызывается из biosinit.inc

		; Сохраняем результат
		push af
		 ; Далее устаналиваем режим клавиатуры 82h
		 ld a, 82h
		 ld (IO_KEYB_MODE), a
		 ; Устаналиваем светодиод РУС/ЛАТ
		 ld a, (v_rusLat)
		 and 1
		 or 0Ch
		 ld (IO_KEYB_MODE), a
		pop af

		ret

; ---------------------------------------------------------------------------

keyScan_0:	 ; Получаем номер ряда нажатой клавиши в B
		call	keyMode82
		ld a, (IO_KEYB_B)
		ld b, a

		; Если ни одна клавиша не нажата, выходим
		or 3
		cp 0FFh
		ret z

		; Получаем номер колонки нажатой клавиши в HL
		ld a, 91h
		ld (IO_KEYB_MODE), a
		ld a, (IO_KEYB_A)
		ld l, a
		ld a, (IO_KEYB_C)
		or 0F0h
		ld h, a

		; Если колонка не нажата или нажата особая комбинация, выходим.
		; При этом может быть возвращена ерунда!
		cp l
		ret z
		
		; Помещаем в L номер первого нулевого бита из HL
		ld a, 0FBh
keyScan_1:	inc a
		 add hl, hl
		 jp c, keyScan_1
		ld l, a

		; Помещаем в С номер первого нулевого бита из B
		ld a, b
		or 3
		scf
		ld c, 0FDh
keyScan_2:	inc c
		 rra
		jp c, keyScan_2

		; Если нажато более одной кнопки
		inc a
		jp nz, specialKeysRet

		; b = !b & 2
		ld a, b
		cpl
		and 2
		ld b, a

		ld a, c
		rlca
		rlca
		rlca
		rlca
		or l

		; a = keybMap[a]
		ld e, a
		ld d, 0
		ld hl, (v_keybMap)
		add hl, de
		ld a, (hl)

		; Сохраняем на будущее
		ld c, a ; c = a

		; if(a < 0x21) return;
		cp 21h
		ret c

		; Специальные клавиши
		cp 8Bh
		jp z, j_tapeMode
		cp 8Ch
		jp z, j_printerEchoMode

		ld hl, (v_koi8)

		cp 81h
		jp z, capsLock
		cp 8Ah
		jp z, inverseVideo
		cp 40h
		jp c, pressed40

		ld a, (v_capsLock)
		xor b
		ld b, a
		ld a, c
		inc h
		jp z, loc_CFA3
		dec b
		ret m
		add a, 20h ; ' '
		inc l
		ret nz
		add a, 0A0h ; 'а'
		cp 8
		ret nc
		add a, 7Fh ; ''
		ret

; ---------------------------------------------------------------------------

loc_CFA3:	add a, 20h ; ' '
		inc l
		ret z
		add a, 60h ; '`'
		dec b
		ret p
		add a, 20h ; ' '
		ret

; ---------------------------------------------------------------------------

pressed40:	dec b
		ret m
		sub 10h
		cp 20h ; ' '
		ret nc
		add a, 20h ; ' '
		ret

; ---------------------------------------------------------------------------

c_koi8:					 ; CODE XREF: ROM:CCD3j
		ld a, l
		cpl
		jp loc_CFBE
; ---------------------------------------------------------------------------

c_koi7:					 ; CODE XREF: ROM:CCCFj
		ld a, l

loc_CFBE:	or a
		ret nz

inverseVideo:	ld a, l ; l = ~l;
		cpl
		ld l, a

capsLockRet:	ld (v_koi8), hl

specialKeysRet:	 ; Если не не выводим в стркоу статуса, обновить её
		ld a, (v_cursorPos)
		cp 8
		call nz, printIndicators

		; Задежка
		ld hl, 4000h
keyScan_delay:	dec hl
		 ld a, h
		 or l
		jp nz, keyScan_delay

		; Переход на keySound
