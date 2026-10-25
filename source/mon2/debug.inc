; Чтение ленты, регистры и программная точка останова
; Порядок включения задан главным .asm; см. README.md.

;----------------------------------------------------------------------------
; Директива R - чтеине файла с ленты

Dir_R:  ; загрузка файла
        CALL  mon2_tapeLoad
        PUSH  BC
        PUSH  DE
        PUSH  HL

        ; расчет контрольной суммы
        CALL  bios_calcCS

        ; печать начального адреса
        POP   HL
        CALL  printHexWordHL

        ; печать конечного адреса
        POP   HL
        CALL  printHexWordHL

        ; сравнение контрольной суммы из файла и рассчитанной
        POP   HL
        LD    D,B
        LD    E,C
        CALL  bios_cmp_hl_de

        ; если не совпадают - переход
        JP    NZ,printChecksumMismatch

        ; иначе печать контрольной суммы и выход
        CALL  printHexWordHL
        RET

printChecksumMismatch:  ; печать знака вопроса и выход
        CALL  printSpace
        LD    C, '?'
        CALL  bios_printChar
        RET

;----------------------------------------------------------------------------

drawSearchMarker:   PUSH  AF
        LD    A,(bios_vars.cursorCfg)
        PUSH  AF
        LD    A,11H
        JP    drawSearchMarkerBody

        ; Сохранённые неиспользуемые байты оригинала.
        NOP
        NOP

;----------------------------------------------------------------------------
; Директива H - печать суммы и разности двух HEX слов

Dir_H:  PUSH  HL
        ADD   HL,DE             ; сумма
        CALL  printHexWordHL    ; печатаем ее
        CALL  printSpace        ; печатаем проблел

        ; DE = -DE
        LD    A,E
        CPL
        LD    E,A
        LD    A,D
        CPL
        LD    D,A
        INC   DE

        POP   HL
        ADD   HL,DE             ; разность
        CALL  printHexWordHL    ; печатаем ее
        RET

;----------------------------------------------------------------------------
; Директива X - печать содержимого регистров процессора

Dir_X:  LD    HL,vars1.SavedAF+1 ; 36772
        LD    DE,txtRegisters             ; 63244
        LD    C,04H         ; 4
printRegisterPairsLoop:  PUSH  BC
        CALL  printSavedRegisterPair
        POP   BC
        DEC   C
        JP    NZ,printRegisterPairsLoop
        CALL  printRegisterName
        LD    HL,(vars1.SavedHL)
        CALL  printHexWordHL
        CALL  printRegisterName
        LD    HL,(vars1.BreakpointAddress)
        CALL  printHexWordHL
        CALL  printRegisterName
        LD    HL,(vars1.SavedSP)
        CALL  printHexWordHL
        RET

;----------------------------------------------------------------------------
; Если нажата клавиша, записать 0FFh по адресу (HL)

setMIfKeyPressed:
        CALL  bios_keyScan
        INC   A
        RET   Z
        LD    (HL), 0FFh
        RET

;----------------------------------------------------------------------------

printSavedRegisterPair:   LD    B,(HL)
        DEC   HL
        LD    C,(HL)
        DEC   HL
        PUSH  BC
        CALL  printRegisterName
        LD    A,B
        CALL  mon2_printHexByte
        CALL  printRegisterName
        POP   BC
        LD    A,C
        CALL  mon2_printHexByte
        RET
printRegisterName:   EX    DE,HL         ; [5]
        PUSH  BC
        CALL  printSpace
        CALL  bios_printStringOld
        INC   HL
        POP   BC
        EX    DE,HL
        RET

;----------------------------------------------------------------------------
; Директива G - запуск программы по адресу (первый параметр)
; Второй параметр — адрес одноразовой программной точки останова RST 7.
; Вход: HL = адрес запуска, DE = адрес останова (0 — без останова).
; Запуск через CALL/PCHL; при RET программы — возврат к приглашению.
; Регистры и флаги после программы не сохраняются.

Dir_G:  LD    A,E
        OR    A
        JP    NZ,installBreakpoint  ; если в E не ноль
        LD    A,D
        OR    A
        JP    Z,executeAddress   ; если в D не ноль

installBreakpoint:  PUSH  HL
        EX    DE,HL
        LD    (vars1.BreakpointAddress),HL
        LD    A,(HL)
        ; МОДИФИКАЦИЯ КОДА ПРОГРАММЫ: один байт заменяется на RST 7.
        LD    (HL),0FFH
        LD    (vars1.BreakpointOriginalByte),A
        LD    HL,0038H
        LD    DE,vars1.SavedRst7Vector
        LD    BC,breakpointTrap
        CALL  saveVectorByte
        ; МОДИФИКАЦИЯ ВЕКТОРА: три байта 0038h-003Ah заменяются на JP.
        LD    (HL),0C3H
        CALL  saveNextVectorByte
        CALL  saveNextVectorByte
        LD    (HL),B
        DEC   HL
        LD    (HL),C
        POP   HL
executeAddress:  CALL  callAddressHL
        JP    RestartNoCls
callAddressHL:   JP    (HL)
saveNextVectorByte:  INC   HL
        INC   DE
saveVectorByte:  LD    A,(HL)
        LD    (DE),A
        RET

; Вход: стек после RST 7, регистры остановленной программы.
; Выход: сохранены AF/BC/DE/HL/SP; восстановлены байт программы и вектор 0038h.
; Переходит к приглашению; продолжения программы и восстановления регистров нет.
breakpointTrap:   LD    (vars1.SavedHL),HL
        EX    DE,HL
        LD    (vars1.SavedDE),HL
        PUSH  BC
        POP   HL
        LD    (vars1.SavedBC),HL
        PUSH  AF
        POP   HL
        LD    (vars1.SavedAF),HL
        LD    HL,0000H
        ADD   HL,SP
        INC   HL
        INC   HL
        LD    (vars1.SavedSP),HL
        LD    HL,(vars1.BreakpointAddress)
        LD    A,(vars1.BreakpointOriginalByte)
        LD    (HL),A
        LD    HL,vars1.SavedRst7Vector
        LD    DE,0038H
        CALL  saveVectorByte
        CALL  saveNextVectorByte
        CALL  saveNextVectorByte
        JP    RestartNoCls

