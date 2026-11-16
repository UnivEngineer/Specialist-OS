;----------------------------------------------------------------------------
; Тест экрана и пакетного вывода текста MXOS
; Строковый вывод через BIOS ABI с явно заданной длиной
;
; 2022-02-05 SpaceEnigneer
;----------------------------------------------------------------------------

    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN

    ORG 0

TEXT_LENGTH = 64*23
KOI8 = 1	; печать символов КОИ8 в тесте
    IF KOI8 == 1
TEXT_PERIOD = 160 ; 20h..7Fh, C0h..FFh
    ELSE
TEXT_PERIOD = 96  ; 20h..7Fh
    ENDIF
TEXT_BUFFER_LENGTH = TEXT_LENGTH+TEXT_PERIOD-1

;-----------------------------------------------------

TestLoop:
    ld   a, (Mode)
    cp   0
    call z, TestCheckerboard
    ;cp   1
    ;call z, TestPrint
    call TestPrint

KeyLoop:	; Ожидание нажатия клавиши
	call bios_keyScan
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
; Закрашивание экрана пикселями в шахматном порядке
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
	call Beep	; звуковой сигнал
    call SetColor
    ld   de, 0
	call ClearScreen
    ; Подготовка корпуса один раз, вне повторяющегося вывода экрана.
    ld   c,20h
    ld   hl,TextBuffer
    ld   (TextStart),hl
    xor  a
    ld   (TextShift),a
    ld   de,TEXT_BUFFER_LENGTH
PrintLoop:
    ld   (hl),c
    inc  hl
	inc  c

    IF KOI8 == 1
	    jp   nz,  Print1    ; Прыгаем, если не перешли через 0FFh
	    ld   c, 020h        ; Код первого символа КОИ7
	    jp   Print2
Print1:
	    ld   a, c
	    cp   080h           ; Код последнего символа КОИ7 + 1
	    jp   nz, Print2
	    ld   c, 0C0h        ; Код первого символа КОИ8
    ENDIF

	ld   a, c
	cp   080h               ; Код последнего символа КОИ7 + 1
	jp   nz, Print2
	ld   c, 020h            ; Код первого символа КОИ7

Print2:
	dec  de
	ld   a, d
	or   e
	jp   nz, PrintLoop

TestPrintRep:
    ld   c,0Ch              ; Курсор в начало экрана.
    call bios_printChar
    ld   hl,(TextStart)
    ld   bc,TEXT_LENGTH
    call bios_printStringRawLim

    ; Следующий экран начинается со следующего символа периода.
    ; Запас в буфере позволяет печатать одним вызовом без перекопирования.
    ld   hl,(TextStart)
    inc  hl
    ld   a,(TextShift)
    inc  a
    cp   TEXT_PERIOD
    jp   nz,TextShiftReady
    xor  a
    ld   hl,TextBuffer
TextShiftReady:
    ld   (TextShift),a
    ld   (TextStart),hl

	call bios_keyCheck  ; Проверка нажатия любой клавиши
	cp   0FFh
    jp   z,TestPrintRep ; Не нажата - продолжаем
    jp   KeyLoop        ; Иначе выходим

;-----------------------------------------------------
; Очистка экрана
; de = слово для заполнения памяти
; bc, hl - сохраняются

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
    ld   (IO_COLOR), a
    ret

;-----------------------------------------------------

Beep:	; Звуковой сигнал
	ld  bc, 0F5Fh
	ld  a, 0Ah	; команда для ВВ55 - установить PC5 в 0
Snd1:	; цикл звукового сигнала
	ld  (IO_KEYB_MODE), a   ; запись команды в регистр управления порта клавиатуры ВВ55
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

TextStart: DW TextBuffer
TextShift: DB 0
TextBuffer: DS TEXT_BUFFER_LENGTH,0

    END
