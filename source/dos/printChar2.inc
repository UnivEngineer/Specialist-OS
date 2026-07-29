;+---------------------------------------------------------------------------
; MXOS
; Вывод символа на экран (или принтер)
;
; На входе
;  с - символ
;
; На выходе
;  регистры не сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printChar2:
            ; Помещаем в hl координаты курсора
            ld  hl, (bios_vars.cursorY)

            ; Продолжаем, если не включен режим обработки ESC последовательности (bios_vars.escMode==0FFh)
            ld  a, (bios_vars.escMode)
            inc a
            jp  nz,  printChar_esc

            ; Тут bios_vars.escMode==0FFh

            ; Если это печатные символы (>=20h), переходим к печати
            ld  a, c
            cp  ' '
            jp  nc, printChar_real

            ; Это табуляция
            cp  9
            jp  z, printChar_c9      ; там bios_vars.cursorX+=24 и ret

            ; Любой символ кроме ESC
            cp  1Bh
            jp  nz, printChar3

            ; Далее ESC. Включаем режим обработки ESC последовательности (bios_vars.escMode=0)
            xor a
            ld  (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------
; Преобразование символа из KOI-7 в KOI-8 и вывод на экран (служебные коды 
; также будут выводится)

printChar_real:
            ; Символ для печати
            ld  c, a

            ; Переходим к печати, если включен режим KOI-8 (bios_vars.koi7 == 0)
            ld  a, (bios_vars.koi7)
            inc a
            jp  nz, printChar_real2

            ; Включен режим KOI-7 (bios_vars.koi7 == 0FFh), надо
            ; перекодировать символы для знакогнератора.

            ; Если символ больше или равен 0C0h просто выходим
            ld  a, c
            cp  0C0h
            ret nc      

            ; Если символ находится в пределах 60h ... 7Fh, увеличиваем его код на 80h
            cp  60h
            jp  c, printChar_real2
            cp  80h
            jp  nc,  printChar_real2
            add a, 80h
            ld  c, a

; ---------------------------------------------------------------------------
; Вывод символа на экран в KOI-8 (служебные коды так же будут выводится)

printChar_real2:  
            ; Тут полиморфизм
            ld    a, (bios_vars.inverse)
            and   a

printChar_poly:
            call z, normalVideo

            ; Если bios_vars.charGen не равен 0FFFFh, то используется знакогнератор по адресу bios_vars.charGen*8
            ld    hl, (bios_vars.charGen)
            ld    a, h
            inc   a
            jp nz,  printChar_alt
            ld    a, l
            inc   a
            jp nz,  printChar_alt

            ; Иначе используем символы из ПЗУ. 
            ; Адрес символа в ПЗУ (de = 0x800 + c*8)
            ld    hl, 100h          
            ld    b,  0
            add   hl, bc
            add   hl, hl
            add   hl, hl
            add   hl, hl
            ex    de, hl

            ; Промежуточное место для символа
            ld    hl, v_char
            push  hl

            ; Копируем 8 байт из ПЗУ
            ld    (IO_PAGE_ROM),a
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            ld    (IO_PAGE_RAM),a

            ; Вывод символа
            pop   de
            jp    printChar_de

; ---------------------------------------------------------------------------
; Обработка режима ESC+F

printChar_esc80:; Если символ с кодом 0, выключаем режим обработки ESC
            ld    a, c
            or    a
            jp z, printChar_esc80_exit

            ; Обработка кодов < 20h
            cp    20h
            jp c, printChar3

            ; Если код >= 80h, выйти
            cp    80h
            ret nc      

            ; Если c >= 40h, вывести на экран c+40h
            cp    40h
            jp nc,  printChar_esc80_2

            ; Иначе вывести c - 20h
            sub   20h ; ' '
            ld    c, a
            jp    printChar_real2

; ---------------------------------------------------------------------------

printChar_esc80_2:
            ; Если c >= 40h, вывести на экран c+40h
            add   a,40h
            ld    c, a
            jp    printChar_real2

; ---------------------------------------------------------------------------

printChar_esc80_exit:
            dec   a
            ld    (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------
; Печать символа в ESC-режиме

printChar_esc:    ; Перейти если bios_vars.escMode=80h
            ld    a, (bios_vars.escMode)
            cp    80h
            jp z, printChar_esc80

            ; Выключить режим обработки ESC последовательности
            push  af
            ld    a, 0FFh
            ld    (bios_vars.escMode),a
            pop   af

            ; Перейти если bios_vars.escMode==0
            or    a
            jp z, printChar_esc0

            ; Перейти если обработка ESC+Y (bios_vars.escMode==1)
            dec   a
            jp z, printChar_esc1

            ; Перейти если bios_vars.escMode==2
            dec   a
            jp z, printChar_esc2

            ; Перейти если bios_vars.escMode==3
            dec   a
            jp z, printChar_esc3

            ; Выйти
            ret

; ---------------------------------------------------------------------------
; Обработка ESC+Y

printChar_esc1:   ; Если код меньше 20h или больше 20h + 24, то пропускаем установку Y
            ld    a, c
            cp    20h + 25
            jp nc,  loc_C693
            sub   20h
            jp c, loc_C693

            ; cursorY = 8 + a*10
            add   a, a
            ld    c, a
            add   a, c
            add   a, c
            add   a, c
            add   a, c
            add   a,8
            ld    (bios_vars.cursorY),a

loc_C693:   ; Следующий символ будет обработан в printChar_esc2
            ld    a, 2
            ld    (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------
; Обработка ESC+Y

printChar_esc2:   ; Если код меньше 20h или больше 20h + 63, то пропускаем установку X
            ld    a, c
            cp    60h
            ret nc
            sub   20h
            ret c 

            ; cursorX = c*3;
            ld    c, a
            add   a, a
            add   a, c
            ld    (bios_vars.cursorX),a
            ret

; ---------------------------------------------------------------------------
; Обработка первого символа ESC последовательности

printChar_esc0:   ; Если код больше 20h
            ld    a, c
            cp    '(' ; 28h
            jp z, printChar_e28
            cp    ')' ; 29h
            jp z, printChar_e29
            cp    'P' ; 50h
            jp z, printChar_eP
            sub   'A' ; 41h
            jp z, printChar19
            dec   a ; 'b' 42h
            jp z, printChar1A
            dec   a ; 'c' 43h
            jp z, printChar18
            dec   a ; 'd' 44h
            jp z, printChar08
            dec   a ; 'e' 45h
            jp z, printChar1F
            dec   a ; 'F' 46h
            jp z, printChar_eF
            dec   a
            dec   a ; 'h' 48h
            jp z, printChar0C
            dec   a ; 'I' 49h
            jp z, printChar_eI
            dec   a
            dec   a
            sub   14 ; 'Y' 59h
            jp z, setEscMode_eY

            ; Вывод символа
            jp    printChar2

; ---------------------------------------------------------------------------
; Обработка ESC+Y.

setEscMode_eY:    ; Следующий символ будет обработан в printChar_esc1 
            ld    a, 1
            ld    (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------

printChar18:
            ld    c, 18h
            jp    printChar2

; ---------------------------------------------------------------------------

printChar08:
            ld    c, 8
            jp    printChar2

; ---------------------------------------------------------------------------

printChar1F:      ld    c, 1Fh
            jp    printChar2

; ---------------------------------------------------------------------------

printChar0C:
            ld    c, 0Ch
            jp    printChar2

; ---------------------------------------------------------------------------
; Обработка ESP+P. Вклчюение режима эхопечати на принтере

printChar_eP:
            ld    a, 3
            ld    (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------

printChar19:
            ld    c, 19h
            jp    printChar2

; ---------------------------------------------------------------------------

printChar1A:
            ld    c, 1Ah
            jp    printChar2

; ---------------------------------------------------------------------------
; Режим эхопечати на принтере. Включается ESC+P

printChar_esc3:
            ; Выходим, из режима, если код нулевой
            ld    a, c
            or    a
            ret z 

            ; Печать на принтере
            call  printer

            ; Печать на экране
            call  printChar2

            ; Находимся в этом режиме до кода 0 или ESC
            ld    a, 3
            ld    (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------
; Продолжение обработки ESC+I. Перемещение курсора вверх с прокруткой экрана

printChar_eI_2:
            ; Если cursorY > 18, перемещаем курсор вверх и выходим
            cp    19
            jp nc,  printChar2

            ; Устанаваливаем bios_vars.cursorY=8
            ld    a, 8
            ld    (bios_vars.cursorY),a
            ret

; ---------------------------------------------------------------------------
; Обработка ESC+I. Перемещение курсора вверх с прокруткой экрана

printChar_eI:
            ; Используется дальше функцией printChar2
            ld    c, 19h

            ; Прокручиваем экран вних, только если bios_vars.cursorY == 8.
            ; иначе переходим на printChar_eI_2
            ld    a, (bios_vars.cursorY)
            cp    8
            jp nz,  printChar_eI_2

