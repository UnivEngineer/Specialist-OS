; Разбор HEX-параметров и вход ввода строки
; Порядок включения задан главным .asm; см. README.md.

;----------------------------------------------------------------------------
; Анализ строки в буфере и разбивка на компоненты:
; A = буква директивы
; HL = первый HEX параметр
; DE = второй HEX параметр
; BC = третий HEX параметр

; Вход: INPUT_BUF с директивой и HEX-параметрами.
; Выход: A = директива, HL/DE/BC = три параметра, флаги разрушены.
; Пропущенные параметры равны нулю; хвост строки — в vars1.ParseTail для S.
Tokenizer:
        ; читаем букву директивы
        LD    BC,INPUT_BUF
        LD    A,(BC)

        ; сохраняем букву директивы
        LD    (vars1.Directive),A
        INC   BC

parseThreeParameters:  ; читаем и сохраняем три HEX параметра
        CALL  ReadHexWord
        LD    (vars1.FirstParam),HL
        CALL  ReadHexWord
        LD    (vars1.SecondParam),HL
        CALL  ReadHexWord
        LD    (vars1.ThirdParam),HL

        LD    L,C
        LD    H,B
        LD    (vars1.ParseTail),HL

        ; помещаем три HEX параметра в регитсры HL, DE, BC
        LD    HL,(vars1.ThirdParam)
        LD    C,L
        LD    B,H
        LD    HL,(vars1.SecondParam)
        EX    DE,HL
        LD    HL,(vars1.FirstParam)

        ; и букву директиры - в регистр A
        LD    A,(vars1.Directive)

        ; выходим
        RET

;----------------------------------------------------------------------------
; Преобразование строки по адресу BC в HEX число в HL
; Выход: HL = младшие 16 бит, BC на CR либо после запятой; AF/флаги разрушены.
; При неверном символе — TypeErrorRestart. Пробелы и строчные a-f не принимаются.

ReadHexWord:
        LD    HL,0000H
readHexWordDigit:  LD    A,(BC)
        CP    0DH           ; 13
        RET   Z
        CP    2CH           ; 44 ','
        JP    Z,skipParameterComma
        CALL  hexDigitValue
        ADD   HL,HL
        ADD   HL,HL
        ADD   HL,HL
        ADD   HL,HL
        ADD   A,L
        LD    L,A
        INC   BC
        JP    readHexWordDigit
skipParameterComma:  INC   BC
        RET

;----------------------------------------------------------------------------

hexDigitValue:  SUB   '0'
        CP    0Ah
        RET   C
        SUB   11h
        CP    06h
        JP    NC,TypeErrorRestart
        ADD   A,10
        RET

;----------------------------------------------------------------------------

moveCursorUp:  PUSH  BC
        LD    C,19h
        CALL  mon2_printChar
        POP   BC
        RET

;----------------------------------------------------------------------------

inputCommandLine:  PUSH  HL
        LD    HL,INPUT_BUF
        JP    inputCommandLineBody

;----------------------------------------------------------------------------
; Печать перевода строки (0Ah, 0Dh)

printNewLine:
        LD    C,0AH
        CALL  mon2_printChar
        LD    C,0DH
        CALL  mon2_printChar
        RET

;----------------------------------------------------------------------------
; Русский текст в кодировке КОИ-7

