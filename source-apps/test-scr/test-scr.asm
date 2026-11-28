;----------------------------------------------------------------------------
; Тест экрана и пакетного вывода текста MXOS
; Шахматное поле, пакетный вывод текста и таблица цветов
;
; 2022-02-05 SpaceEnigneer
;----------------------------------------------------------------------------

    INCLUDE "../../source/common/system.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN

    ORG 0

TEXT_LENGTH = 64*23
KOI8        = 1  ; печать символов КОИ8 в тесте
    IF KOI8 == 1
TEXT_PERIOD = 160 ; 20h..7Fh, C0h..FFh
    ELSE
TEXT_PERIOD = 96  ; 20h..7Fh
    ENDIF
TEXT_BUFFER_LENGTH = TEXT_LENGTH+TEXT_PERIOD-1

MODE_CHECKER = 1
MODE_TEXT    = 2
MODE_TABLE   = 3
TABLE_X      = 64        ; Левый край таблицы в пикселях
TABLE_Y      = 44        ; Верхний край таблицы в пикселях
CELL_HEIGHT  = 12        ; Шаг строк, включая промежуток

;----------------------------------------------------------------------------
; MXOS
; Меню тестов экрана. Цифра выбирает тест, Esc/СТР возвращает на уровень выше.
; вход: без параметров, программа в MAIN, на стеке адрес возврата в оболочку.
; выход: экран очищен, курсор в начале, установлен COLOR_BIOS. SP сохранён,
;   af, bc, de, hl и флаги используются.
;----------------------------------------------------------------------------

Menu:
    ld   a,COLOR_BIOS
    ld   (IO_COLOR),a
    ld   hl,MenuText
    call mon2_printStringOld
MenuKey:
    call ReadKey
    cp   1Bh
    jp   z,Exit
    cp   1Fh
    jp   z,Exit
    sub  '0'
    cp   MODE_CHECKER
    jp   c,MenuKey
    cp   MODE_TABLE+1
    jp   nc,MenuKey
SelectMode:
    ld   (Mode),a
DrawMode:
    ld   a,(Mode)
    cp   MODE_TABLE
    jp   z,TestColorTable
    call Beep
    call SetColor
    ld   a,(Mode)
    cp   MODE_TEXT
    jp   z,TestPrint
    ld   de,55AAh
    call ClearScreen
KeyLoop:
    call ReadKey
TestKey:
    cp   1Bh
    jp   z,Menu
    cp   1Fh
    jp   z,Menu
    cp   ' '
    jp   z,NextMode
    ld   b,a
    ld   a,(Mode)
    cp   MODE_TABLE
    jp   z,KeyLoop
    ld   a,b
    ld   hl,FrontColor
    cp   19h                ; Вверх / вниз — цвет изображения
    jp   z,ColorUp
    cp   1Ah
    jp   z,ColorDown
    ld   hl,BackColor
    cp   18h                ; Вправо / влево — цвет фона
    jp   z,ColorUp
    cp   08h
    jp   nz,KeyLoop
ColorDown:
    ld   a,(hl)
    dec  a
    jp   StoreColor
ColorUp:
    ld   a,(hl)
    inc  a
StoreColor:
    and  0Fh
    ld   (hl),a
    jp   DrawMode
NextMode:
    ld   a,(Mode)
    inc  a
    cp   MODE_TABLE+1
    jp   c,SelectMode
    ld   a,MODE_CHECKER
    jp   SelectMode
Exit:
    ld   c,1Fh
    jp   mon2_printCharOld

;----------------------------------------------------------------------------
; MXOS
; Дождаться одного нажатия без рисования курсора на тестовом изображении.
; вход: ReadKey — без параметров, ReleaseKey — в a код уже нажатой клавиши.
; выход: a = код после отпускания клавиши, bc, de, hl и SP сохранены.
;----------------------------------------------------------------------------

ReadKey:
    MXOS_CALL_REGS MXOS_BIOS_KEY_SCAN
    cp   0FFh
    jp   z,ReadKey
ReleaseKey:
    push af
WaitRelease:
    MXOS_CALL_REGS MXOS_BIOS_KEY_SCAN
    cp   0FFh
    jp   nz,WaitRelease
    pop  af
    ret

;----------------------------------------------------------------------------
; MXOS
; Выводить сдвигающийся корпус символов кадрами по TEXT_LENGTH байт.
; вход: выбран цвет теста в IO_COLOR. Параметров в регистрах нет.
; выход: при нажатии переход в TestKey, a = код клавиши. Изменяет экран,
;   TextBuffer, TextStart, TextShift, af, bc, de, hl и флаги. SP сохранён.
;----------------------------------------------------------------------------

TestPrint:
    ld   de,0
    call ClearScreen
    ; Подготовка корпуса один раз, вне повторяющегося вывода экрана
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
    ld   c,0Ch              ; Курсор в начало экрана
    call mon2_printCharOld
    ld   hl,(TextStart)
    ld   bc,TEXT_LENGTH
    call FONT_ENGINE_LIM

    ; Следующий экран начинается со следующего символа периода
    ; Запас в буфере позволяет печатать одним вызовом без перекопирования
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

    MXOS_CALL_REGS MXOS_BIOS_KEY_CHECK
    cp   0FFh
    jp   z,TestPrintRep
    call ReleaseKey
    jp   TestKey

;----------------------------------------------------------------------------
; MXOS
; Показать 16 цветов изображения по строкам и 16 цветов фона по столбцам.
; вход: без параметров, экран MAIN доступен для записи.
; выход: переход в KeyLoop, на экране таблица, в IO_COLOR цвет меню.
;   af, bc, de, hl и флаги используются, SP сохранён.
;----------------------------------------------------------------------------

TestColorTable:
    ld   a,COLOR_BIOS
    ld   (IO_COLOR),a
    ld   hl,TableText
    call mon2_printStringOld
    ld   de,HexDigits
    ld   b,(TABLE_X+4)/2    ; X подписи столбца в единицах BIOS (два пикселя)
    ld   c,TABLE_Y+8        ; Y нижней строки глифа слева от ячейки
TableLabel:
    ld   h,b
    ld   l,TABLE_Y-5
    call mx_setCursorPos
    ld   a,(de)
    call FONT_ENGINE_RAW
    ld   h,(TABLE_X-16)/2
    ld   l,c
    call mx_setCursorPos
    call FONT_ENGINE_RAW
    inc  de
    ld   a,b
    add  a,8
    ld   b,a
    ld   a,c
    add  a,CELL_HEIGHT
    ld   c,a
    ld   a,(de)
    or   a
    jp   nz,TableLabel

    ld   l,TABLE_Y
    ld   c,0                ; Старшая тетрада — изображение, младшая — фон
TableRow:
    ld   h,90h+TABLE_X/8    ; Соседний экранный столбец отстоит на 100h
TableCell:
    ld   a,c
    ld   (IO_COLOR),a
    push hl
    ld   de,CellPattern
    ld   b,CellPatternEnd-CellPattern
TableLine:
    ld   a,(de)             ; Левая и правая половины образуют прямоугольник на фоне
    ld   (hl),a
    inc  h
    rlca
    rlca
    rlca
    rlca
    ld   (hl),a
    dec  h
    inc  l
    inc  de
    dec  b
    jp   nz,TableLine
    pop  hl
    inc  h
    inc  h
    inc  c
    ld   a,c
    and  0Fh
    jp   nz,TableCell
    ld   a,l
    add  a,CELL_HEIGHT
    ld   l,a
    ld   a,c
    or   a
    jp   nz,TableRow
    ld   a,COLOR_BIOS
    ld   (IO_COLOR),a
    jp   KeyLoop

;----------------------------------------------------------------------------
; MXOS
;
; Заполнить 12 КиБ экрана повторяющимся словом через стек 8080.
;
; вход:
;   de = 16-битный узор. Прерывания исключены на время временной установки SP в конец
;   VRAM.
;
; выход:
;   Узор записан в 9000h–0BFFFh, SP, hl и bc восстановлены. de и af используются при
;   заполнении.
;----------------------------------------------------------------------------

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

;----------------------------------------------------------------------------
; MXOS
;
; Объединить цвета текста и фона тестовой программы в аппаратный байт.
;
; вход:
;   FrontColor и BackColor содержат значения 0..15.
;
; выход:
;   a и IO_COLOR = (FrontColor << 4) OR BackColor, e = FrontColor << 4.
;   hl, bc и d сохранены. Флаги определяет OR.
;----------------------------------------------------------------------------

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
Mode:           DB     MODE_CHECKER

MenuText:       DB 1Fh,"Screen tests",13,10,13,10
                DB "1  Checkerboard",13,10
                DB "2  Text output speed",13,10
                DB "3  Color table (16 x 16)",13,10,13,10
                DB "Esc  Exit",13,10,13,10
                DB "In a test: Esc = menu, Space = next test",13,10
                DB "Tests 1-2: Up/Down = foreground",13,10
                DB "           Left/Right = background",0
TableText:      DB 1Fh,"Color table",13,10
                DB "Rows: foreground    Columns: background",0
HexDigits:      DB "0123456789ABCDEF",0
CellPattern:    DB 00h,00h,0Fh,0Fh,0Fh,0Fh,0Fh,0Fh,00h,00h
CellPatternEnd:

;-----------------------------------------------------

TextStart:      DW TextBuffer
TextShift:      DB 0
TextBuffer:     DS TEXT_BUFFER_LENGTH,0

    END
