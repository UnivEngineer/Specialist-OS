;+---------------------------------------------------------------------------
; MXOS
; FLPAGE.COM - переключатель страниц флеш-диска ВВ55
;
; 2022-01-25 Разработано SpaceEngineer
;
;----------------------------------------------------------------------------

; Функции системы
printChar       = 0C809h ; Вывод символа на экран
input           = 0C80Fh ; Ввод строки с клавиатуры
printHexByte	= 0C815h ; Вывести 16-ричное число (байт)
printString     = 0C818h ; Вывести строку на экран
strToHex        = 0C839h ; Преобразвоние строки в HEX формате в число

; Переменные системы
v_flashPage     = 8FF9h  ; Текущая страница флеш-диска (макс. 1Fh = 31)
v_input         = 0DE8Eh ; Буфер для ввода строки

.org 08000H

        ; вывод текущей страницы
	    lxi  h, txtCurrentPage
	    call printString
        lda  v_flashPage
        call printHexByte

Repeat:
        ; вывод запроса
	    lxi  h, txtEnterPage
	    call printString

        ; ввод строки
	    lxi  h, v_input
	    lxi  d, v_input+23
        call input

        ; анализ строки
        xchg
        call strToHex
        jz   Repeat

        ; проверяем значение
        lxi  d, 20h ; макс. 1Fh + 1
        call cmp_hl_de
        jnc  Repeat

        ; записываем результат
        mov a, l
        sta v_flashPage
        ret

cmp_hl_de:
        mov	a, h
        cmp	d
        rnz
        mov	a, l
        cmp	e
        ret

txtCurrentPage:
        .db 0Ah,"CURRENT PAGE NUMBER: ",0

txtEnterPage:
        .db 0Ah,"ENTER NEW PAGE NUMBER (0-1F): ",0

.END
