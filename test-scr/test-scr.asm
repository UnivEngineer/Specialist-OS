;----------------------------------------------------------------------------
; Кросс-платформенный тест экрана для "Специалиста-MX"
; Должен работать в любой OC
;
; 2022-02-05 SpaceEnigneer
;----------------------------------------------------------------------------

    ORG 0

KOI8 = 0	; печать символов КОИ8 в тесте

REG_KEYB		= 0FFE0h
REG_TIMER		= 0FFECh
REG_COLOR		= 0FFF8h
REG_PAGE_RAM	= 0FFFCh
REG_PAGE_RAMD	= 0FFFDh
REG_PAGE_ROM	= 0FFFEh
REG_PAGE_STD	= 0FFFFh

;-----------------------------------------------------

TestLoop:
    ld   a, (Mode)
    cp   0
    call z, TestCheckerboard
    ;cp   1
    ;call z, TestPrint
    call TestPrint

KeyLoop:	; Ожидание нажатия клавиши
	call 0C81Bh
	cp   19h	; кнопка вверх
	jp   z, ButUp
	cp   1Ah	; кнопка вниз
	jp   z, ButDown
	cp   18h	; кнопка вправо
	jp   z, ButRight
	cp   08h	; кнопка влево
	jp   z, ButLeft
	cp   20h	; кнопка пробел
	jp   z, ChangeMode
	cp   1Bh	; кнопка Esc
	ret  z
	cp   1Fh	; кнопка СТР
	ret  z
	jp   KeyLoop

;-----------------------------------------------------

ButUp:		; Увеличить цвет символов
	ld  a, (FrontColor)
	inc a
	and 0Fh
	ld  (FrontColor), a
	jp  TestLoop

ButDown:	; Уменьшить цвет символов
	ld  a, (FrontColor)
	dec a
	and 0Fh
	ld  (FrontColor), a
	jp  TestLoop

ButRight:	; Увеличить цвет фона
	ld  a, (BackColor)
	inc a
	and 0Fh
	ld  (BackColor), a
	jp  TestLoop

ButLeft:	; Уменьшить цвет фона
	ld  a, (BackColor)
	dec a
	and 0Fh
	ld  (BackColor), a
	jp  TestLoop

ChangeMode: ; Переключить режим
    ld  a, (Mode)
    inc a
    cp  2
    jp  nz, SetMode
    xor a
SetMode:
    ld  (Mode), a
    jp  TestLoop

;-----------------------------------------------------
; Закрашиваение экрана пикселями в шахматном порядке
;-----------------------------------------------------

TestCheckerboard:
	call Beep	; звуковой сигнал
    call SetColor
    ld   de, 55AAh
	call ClearScreen
    jp   KeyLoop

;-----------------------------------------------------
; Тест скорости вывода текста
;-----------------------------------------------------

TestPrint:
    ld   de, 0
	call ClearScreen
	ld   c, 20h		; Код первого символа

TestPrintRep:
	push bc

    IF KOI8 == 1
	    ld   c, 1Bh		; Esc
	    call 0C809h
	    ld   c, '('		; Включение КОИ-8 (MXOS)
	    call 0C809h
    ENDIF

	ld   c, 0Ch		; Установка курсора в начало экрана путем печати символа 0Ch (кроссплатформенно)
	call 0C809h
	pop  bc
	ld   de, 64*23	; Сколько всего символов выводить (23 строки разрешено в RAMFOS)
PrintLoop:
	call 0C809h
	inc  c

    IF KOI8 == 1
	    jnz  Print1		; Прыгаем, если не перешли через 0FFh
	    ld   c, 020h	; Код первого символа КОИ7
	    jp   Print2
Print1:
	    ld   a, c
	    cp   080h		; Код последнего символа КОИ7 + 1
	    jp   nz, Print2
	    ld   c, 0C0h	; Код первого символа КОИ8
    ENDIF

	ld   a, c
	cp   080h		; Код последнего символа КОИ7 + 1
	jp   nz, Print2
	ld   c, 020h	; Код первого символа КОИ7

Print2:
	dec  de
	ld   a, d
	or   e
	jp   nz, PrintLoop
	call 0C81Bh		; Проверка нажатия любой клавиши
	cp   0FFh		; Не нажата
	jp   z, TestPrintRep
    jp   KeyLoop

;-----------------------------------------------------
; Очистка экрана
; de = слово для заполнения памяти
; bc, hl - сохраняюстя

ClearScreen:
    push hl
    push bc
    ld   hl, 0  ; Сохранение SP
    add  hl, sp
    ld   (MemSP), hl
    ld   sp, 0C000h         ; Устанавливаем SP в конец видеопамяти
    ld   bc, 300h           ; Помещаем в стек 3000h байт
    ex   hl, de
ClearLoop:
	push hl
	push hl
	push hl
	push hl
	push hl
	push hl
	push hl
	push hl
	dec  bc
	ld   a, b
	or   c
	jp   nz, ClearLoop
	ld   hl, (MemSP) ; Восстанавливаем SP
	ld   sp, hl
    ex   hl, de
	pop  bc
	pop  hl
	ret

;-----------------------------------------------------

SetColor:
    ld   a, (FrontColor)
    rlca
    rlca
    rlca
    rlca
    ld   e, a
    ld   a, (BackColor)
    or   e
    ld   (REG_COLOR), a
    ret

;-----------------------------------------------------

Beep:	; Звуковой сигнал
	ld  bc, 0F5Fh
	ld  a, 0Ah	; команда для ВВ55 - установить PC5 в 0
Snd1:	; цикл звукового сигнала
	ld  (REG_KEYB+3), a     ; запись команды в регистр управления порта клавиатуры ВВ55
Snd2:
	dec b                   ; задержка на FFh циклов (в b)
	jp  nz, Snd2
	xor 01h                 ; команда для ВВ55 - установить PC5 в инверсное значение
	dec c
	jp  nz, Snd1            ; повторить FFh раз (в C)
	ret

;-----------------------------------------------------

MemSP:          DW     0000h
FrontColor:     DB     0Fh
BackColor:      DB     00h
Mode:           DB     00h

;-----------------------------------------------------

    END
