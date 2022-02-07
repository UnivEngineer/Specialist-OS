;----------------------------------------------------------------------------
; MXOS
; E.COM - текстовой редактор
;
; 2022-02-03 Дизассемблировано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

; Используемые подпрограммы DOS.SYS:
; bios_beep_Old         = 0C170h
; bios_delay_b          = 0C190h
; bios_cmp_hl_de        = 0C427h
; bios_memcpy_bc_hl     = 0C42Dh
; bios_reboot           = 0C800h
; bios_getch            = 0C803h
; bios_tapeRead         = 0C806h
; bios_printChar        = 0C809h
; bios_tapeWrite        = 0C80Ch
; bios_printString      = 0C818h
; bios_calcCS           = 0C82Ah
; bios_fileCreate       = 0C845h
; bios_fileLoadInfo     = 0C851h
; bios_fileNamePrepare  = 0C85Ah
; bios_fileLoad2        = 0C866h

; Используемые переменные DOS.SYS:
; bios_vars.lastKey     = 08FF0h
; bios_vars.tapeInverse = 08FF3h
; bios_vars.cursorDelay = 08FF4h
; bios_vars.inverse     = 08FFAh
; bios_vars.cursorY     = 08FFCh

; Используемые порты устройств:
; IO_KEYB_A = 0FFE0h
; IO_KEYB_B = 0FFE1h

; Собственные переменные
    STRUCT EDITOR_VARIABLES
buffer  BLOCK  10, 0
last    BLOCK  1,  0
    ENDS

; Блок переменных редактора начинается с адреса 8F80H
editor_vars EDITOR_VARIABLES = 8F80H

; Начало программы
        ORG   0D000H

SMC1:
        JP    LBL145
        JP    LBL103
        DB    0,18h,0
LBL1:   ADD   A,L
LBL2:   LD    HL,REF2             ; 53282
        CALL  bios_printString
LBL3:   LD    HL,7000H    ; 28672; [2]
        LD    DE,8F00H    ; 36608
        CALL  bios_calcCS
        LD    HL,LBL4             ; 53457
        LD    (SMC1+1),HL
        JP    LBL4
REF2:   DB    0AH,"          EDITOR VERSION 4.1, (C) OM"
REF3:   DB    "S"
REF4:   DB    "K"
REF5:   DB    " "
REF6:   DB    "KSOFT ",27H,"92",2EH,00H
VAR1:   DW    0000H
VAR2:   DW    0000H
VAR3:   DW    0000H
VAR4:   DB    00H
VAR5:   DB    00H
VAR6:   DW    0000H
VAR7:   DW    0000H
VAR8:   DW    0000H
REF7:   DB    04H,05H,08H
VAR9:   DB    00H
REF8:   DB    1FH,0AH,"  OUT OF MEMORY",2EH,00H
REF9:   DB    1FH
REF10:  DB    0AH," FILE: ",00H
REF11:  DB    08H,20H,08H,00H           ; <-, ' ', <-
REF12:  DB    0AH," MISTAKE",2EH,00H
REF13:  DB    "SEARCH: "
REF14:  DB    00H,"ALL V33",00H,"RLN]N",00H,"y"
REF15:  DB    1FH,0AH," NEW ?",00H
REF16:  DB    "INSERT   ",00H
REF17:  DB    "OVERWRITE",00H
REF18:  DB    "LINE",00H
REF19:  DB    "COL",00H
LBL4:   LD    HL,0000H
        ADD   HL,SP
        LD    (SMC4+1),HL
LBL5:   LD    A,10H         ; 16
        LD    (editor_vars.last),A
        XOR   A
        LD    (VAR4),A
        LD    H,A
        LD    L,A
        LD    (VAR6),HL
        LD    (bios_vars.inverse),HL
        LD    HL,0001H    ; 1
        LD    (VAR8),HL
        LD    HL,(0D006H)
        LD    (VAR1),HL
        LD    (VAR3),HL
        LD    E,L
        LD    D,H
LBL6:   LD    A,(HL)
        OR    A
        JP    NZ,LBL7
        DEC   A
        LD    (HL),A
LBL7:   INC   A
        JP    Z,LBL8
        INC   HL
        JP    LBL6
LBL8:   CALL  SUB38
        JP    NZ,LBL9
        LD    (HL),0DH    ; 13
        INC   HL
        LD    (HL),0FFH   ; 255
LBL9:   LD    (VAR2),HL
LBL10:  LD    C,1FH         ; 31
        CALL  bios_printChar
        CALL  SUB1
REF21:  LD    HL,REF21    ; 53535
        PUSH  HL
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  SUB56
        LD    HL,0000H
        CALL  SUB14
        EX    DE,HL
        LD    HL,8CF9H    ; 36089
        LD    (bios_vars.cursorY),HL
        EX    DE,HL
        LD    DE,0FFF6H   ; 65526
        LD    B,20H         ; 32 ' '
        INC   HL
        CALL  SUB51
        LD    A,L
        ADD   A,3AH         ; 58 ':'
        LD    C,A
        CALL  bios_printChar
        CALL  SUB57
        POP   HL
        LD    (bios_vars.cursorY),HL
        JP    LBL141
LBL11:  LD    C,A           ; [2]
        CP    7FH           ; 127
        JP    Z,LBL33
        CP    07H           ; 7
        JP    Z,LBL102
        CP    01H           ; 1
        JP    Z,LBL54
        CP    02H           ; 2
        JP    Z,LBL57
        CP    20H           ; 32 ' '
        JP    NC,LBL27
        OR    A
        JP    Z,LBL31
        LD    HL,REF22    ; 53635
LBL12:  LD    A,(HL)
        OR    A
        JP    Z,SUB40
        INC   HL
        LD    E,(HL)
        INC   HL
        LD    D,(HL)
        INC   HL
        CP    C
        JP    NZ,LBL12
        PUSH  DE
        RET
REF22:  DB    1AH,49H,0D2H,19H  ; ".I.."
        DB    73H,0D2H,08H,0EEH ; "s..."
        DB    0D2H,18H,0E3H,0D2H        ; "...."
        DB    0DH,13H,0D3H,09H  ; "...."
        DB    0F6H,0D4H,1FH,0E0H        ; "...."
        DB    0D1H,02H,0EEH,0D1H        ; "...."
        DB    06H,6BH,0D8H,0CH  ; ".k.."
        DB    0EH,0D3H,0AH,0FEH ; "...."
        DB    0D2H,1BH,0A8H,0D1H        ; "...."
        DB    00H,0CDH,17H,0D4H ; "...."
        DB    0CDH,03H,0C8H,4FH ; "...O"
        DB    21H,0B5H,0D1H,0C3H        ; "!..."
        DB    73H,0D1H,0CH,0FAH ; "s..."
        DB    0D1H,0AH,9EH,0D5H ; "...."
        DB    4CH,0B2H,0D5H,44H ; "L..D"
        DB    0E3H,0D5H,55H,57H ; "..UW"
        DB    0D6H,4FH,80H,0D6H ; ".O.."
        DB    49H,10H,0D7H,56H  ; "I..V"
        DB    00H,0D8H,47H,05H  ; "..G."
        DB    0D8H,4EH,0B4H,0D9H        ; ".N.."
        DB    4AH,0D7H,0D8H,53H ; "J..S"
        DB    06H,0D9H,43H,3FH  ; "..C?"
        DB    0D9H,4DH,72H,0D9H ; ".Mr."
        DB    00H                   ; "."
        LD    C,1FH         ; 31
        CALL  bios_printChar
        CALL  SUB15
        CALL  SUB54
        JP    bios_reboot
        CALL  SUB15
        LD    C,1FH         ; 31
        CALL  bios_printChar
        CALL  SUB54
        RET
LBL13:  LD    HL,0001H    ; 1; [4]
        LD    (VAR8),HL
        LD    HL,(0D006H)
        LD    (VAR1),HL
        LD    (VAR3),HL
SUB1:   LD    HL,(VAR1)   ; [10]
        EX    DE,HL
        LD    HL,(VAR3)
        CALL  SUB38
        JP    Z,SUB2
        CALL  SUB49
        JP    SUB1
SUB2:   CALL  SUB55         ; [2]
SUB3:   LD    BC,180CH    ; 6156; [2]
LBL14:  CALL  bios_printChar
        CALL  SUB7
        INC   A
        JP    Z,SUB4
        DEC   B
        LD    C,0AH         ; 10
        JP    NZ,LBL14
SUB4:   CALL  SUB53         ; [2]
        LD    C,0CH         ; 12
        JP    bios_printChar
LBL15:  CALL  SUB15         ; [3]
SUB5:   CALL  SUB43         ; [5]
        LD    HL,(VAR1)
        CALL  SUB2
        JP    SUB44
SUB6:   CALL  SUB15
        CALL  SUB48
        RET   Z
        CALL  SUB50
        LD    A,(bios_vars.cursorY)
        CP    0EEH          ; 238
        JP    NZ,bios_printChar
        LD    HL,(VAR1)
        CALL  SUB42
        LD    (VAR1),HL
        CALL  SUB43
        CALL  SUB45
        LD    HL,(VAR3)
        CALL  SUB7
        JP    SUB44
        CALL  SUB15
        CALL  SUB49
        RET   Z
        CALL  SUB50
        LD    A,(bios_vars.cursorY)
        CP    08H           ; 8
        JP    NZ,bios_printChar
        LD    HL,(VAR3)
        LD    (VAR1),HL
        PUSH  HL
        CALL  SUB46
        POP   HL
        CALL  SUB43
        CALL  SUB7
        JP    SUB44
SUB7:   CALL  SUB9          ; [4]
        CALL  SUB8
        JP    NC,LBL16
        LD    A,0FFH                ; 255
        LD    (bios_vars.inverse),A
LBL16:  LD    A,(HL)
        CP    0DH           ; 13
        LD    C,20H         ; 32 ' '
        CALL  Z,bios_printChar
LBL17:  LD    A,(HL)
        LD    C,A
        INC   A
        JP    Z,LBL18
        INC   HL
        CALL  bios_printChar
        CP    0EH           ; 14
        JP    NZ,LBL17
LBL18:  XOR   A
        LD    (bios_vars.inverse),A
        LD    (bios_vars.inverse+1),A
        LD    A,C
        RET
SUB8: PUSH  DE            ; [7]
        EX    DE,HL
        LD    HL,(VAR6)
        LD    A,H
        OR    L
        JP    Z,LBL19
        DEC   HL
        CALL  SUB38
        JP    NC,LBL19
        LD    HL,(VAR7)
        DEC   HL
        CALL  SUB38
        CCF
LBL19:  EX    DE,HL         ; [2]
        POP   DE
        RET
        CALL  SUB36
        JP    Z,LBL117
LBL20:  LD    B,0BAH                ; 186
        JP    LBL21
        CALL  SUB36
        JP    Z,LBL120
        LD    B,00H
LBL21:  LD    A,(bios_vars.cursorY+1)
        CP    B
        RET   Z
        JP    bios_printChar
LBL22:  CALL  SUB12
        CALL  SUB10
        LD    HL,LBL2             ; 53258
        CALL  SUB39
        LD    A,E
        JP    LBL48
SUB9:   XOR   A             ; [4]
        LD    (bios_vars.cursorY+1),A
        RET
        LD    HL,(VAR3)
        CALL  SUB8
        JP    C,SUB40
        CALL  SUB12
        CALL  SUB10
        LD    A,0DH         ; 13
        LD    (DE),A
        CALL  SUB15
        CALL  SUB9
        LD    HL,(VAR3)
        LD    C,18H         ; 24
LBL23:  LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    NZ,LBL24
        CALL  bios_printChar
        INC   HL
        JP    LBL23
LBL24:  LD    C,1AH         ; 26
        CALL  SUB6
        JP    SUB46
SUB10:  LD    DE,REF6             ; 53322; [4]
SUB11:  DEC   DE            ; [3]
        LD    A,(DE)
        CP    20H           ; 32 ' '
        JP    Z,SUB11
        INC   DE
        RET
SUB12:  LD    A,(VAR4)    ; [7]
        OR    A
        RET   NZ
        cpl
        LD    (VAR4),A
        LD    HL,LBL2             ; 53258
        LD    A,40H         ; 64 '@'
LBL25:  LD    (HL),20H    ; 32 ' '
        INC   HL
        DEC   A
        JP    NZ,LBL25
        LD    DE,LBL2             ; 53258
        LD    HL,(VAR3)
        LD    B,40H         ; 64 '@'
LBL26:  LD    A,(HL)
        CP    0DH           ; 13
        RET   Z
        CP    0FFH          ; 255
        RET   Z
        DEC   B
        RET   Z
        LD    (DE),A
        INC   HL
        INC   DE
        JP    LBL26
LBL27:  LD    HL,(VAR3)
        CALL  SUB8
        JP    C,SUB40
        CALL  SUB12
        CALL  SUB13
        LD    A,(VAR5)
        OR    A
        JP    NZ,LBL28
        LD    (HL),C
        CALL  bios_printChar
        LD    A,(bios_vars.cursorY+1)
        CP    0BDH          ; 189
        RET   NZ
        LD    C,08H         ; 8
        JP    bios_printChar
LBL28:  LD    DE,REF5             ; 53321
        CALL  SUB11
        PUSH  HL
        LD    HL,REF5             ; 53321
        CALL  SUB38
        JP    Z,LBL29
        INC   DE
LBL29:  POP   HL
        CALL  SUB43
LBL30:  LD    B,(HL)
        LD    (HL),C
        CALL  bios_printChar
        LD    C,B
        INC   HL
        CALL  SUB38
        JP    C,LBL30
        CALL  SUB44
        LD    C,18H         ; 24
        JP    LBL20
LBL31:  LD    HL,VAR5             ; 53340
        LD    A,(HL)
        cpl
        LD    (HL),A
        JP    SUB47
SUB13:  LD    HL,LBL2             ; 53258; [4]
SUB14:  LD    A,(bios_vars.cursorY+1)        ; [4]
        OR    A
LBL32:  RET   Z
        INC   HL
        SUB   03H           ; 3
        JP    LBL32
LBL33:  CALL  SUB12
        CALL  SUB13
        PUSH  HL
        LD    C,L
        LD    B,H
        INC   BC
        LD    DE,REF5             ; 53321
        CALL  SUB37
        POP   HL
        LD    DE,REF5             ; 53321
        CALL  SUB11
        PUSH  HL
        LD    HL,REF5             ; 53321
        CALL  SUB38
        JP    Z,LBL34
        INC   DE
LBL34:  POP   HL
        CALL  SUB43
LBL35:  LD    C,(HL)
        CALL  bios_printChar
        CALL  SUB38
        INC   HL
        JP    C,LBL35
        JP    SUB44
SUB15:  LD    A,(VAR4)    ; [10]
        OR    A
        RET   Z
        XOR   A
        LD    (VAR4),A
        PUSH  BC
        CALL  SUB10
        LD    HL,LBL2             ; 53258
        CALL  SUB39
        LD    HL,(VAR3)
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        CALL  SUB42
        INC   A
        JP    Z,LBL36
        DEC   HL
LBL36:  CALL  SUB16
        LD    HL,(VAR3)
        LD    BC,LBL2             ; 53258
        CALL  SUB38
        CALL  NZ,SUB37
        POP   BC
        RET
SUB16:  CALL  SUB38         ; [5]
        RET   Z
        PUSH  AF
        PUSH  DE
        CALL  SUB39
        LD    B,D
        LD    C,E
        EX    DE,HL
        LD    HL,(VAR6)
        CALL  SUB38
        JP    C,LBL37
        ADD   HL,BC
        LD    (VAR6),HL
LBL37:  LD    HL,(VAR7)
        EX    DE,HL
        CALL  SUB38
        EX    DE,HL
        JP    NC,LBL38
        ADD   HL,BC
        LD    (VAR7),HL
LBL38:  EX    DE,HL
        POP   DE
        POP   AF
        JP    C,LBL42
        PUSH  DE
        LD    C,E
        LD    B,D
        EX    DE,HL
        LD    HL,(VAR2)
        INC   HL
        EX    DE,HL
        CALL  SUB39
LBL39:  LD    A,E
        AND   0FEH          ; 254
        OR    D
        JP    Z,LBL40
        LD    A,(HL)
        LD    (BC),A
        INC   BC
        INC   HL
        DEC   DE
        LD    A,(HL)
        LD    (BC),A
        INC   BC
        INC   HL
        DEC   DE
        JP    LBL39
LBL40:  CP    E
        JP    Z,LBL41
        LD    A,(HL)
        LD    (BC),A
        INC   BC
LBL41:  DEC   BC
        LD    L,C
        LD    H,B
        LD    (VAR2),HL
        POP   DE
        RET
LBL42:  PUSH  DE
        CALL  SUB39
        LD    HL,(VAR2)
        LD    B,H
        LD    C,L
        ADD   HL,DE
        POP   DE
        PUSH  DE
        PUSH  HL
        EX    DE,HL
        LD    HL,(0D008H)
        CALL  SUB38
        JP    C,LBL71
        POP   HL
        POP   DE
        LD    (VAR2),HL
        PUSH  HL
        EX    DE,HL
        CALL  SUB39
        EX    (SP),HL
        INC   DE
LBL43:  LD    A,E
        AND   0FCH          ; 252
        OR    D
        JP    Z,LBL44
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        JP    LBL43
LBL44:  LD    A,E
        OR    A
        JP    Z,LBL46
LBL45:  LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   E
        JP    NZ,LBL45
LBL46:  POP   DE
        RET
        CALL  SUB36
        JP    Z,LBL51
        LD    HL,0001H    ; 1
        CALL  SUB14
        LD    E,L
        LD    HL,REF7             ; 53347
        LD    C,03H         ; 3
        XOR   A
LBL47:  DEC   C
        JP    Z,LBL50
        ADD   A,(HL)
        CP    E
        INC   HL
        JP    C,LBL47
LBL48:  CP    3EH           ; 62 '>'; [2]
        JP    C,LBL49
        LD    A,3EH         ; 62 '>'
LBL49:  LD    L,A           ; [3]
        ADD   A,A
        ADD   A,L
        LD    (bios_vars.cursorY+1),A
        RET
LBL50:  ADD   A,(HL)                ; [2]
        CP    E
        JP    C,LBL50
        JP    LBL48
LBL51:  LD    HL,0000H
        CALL  SUB14
        LD    E,L
        LD    HL,REF7             ; 53347
        LD    C,03H         ; 3
        XOR   A
LBL52:  DEC   C
        JP    Z,LBL53
        ADD   A,(HL)
        CP    E
        INC   HL
        JP    C,LBL52
        DEC   HL
        SUB   (HL)
        JP    LBL49
LBL53:  ADD   A,(HL)                ; [2]
        CP    E
        JP    C,LBL53
        SUB   (HL)
        JP    LBL49
LBL54:  CALL  SUB15         ; [2]
        LD    HL,(0D006H)
        EX    DE,HL
        LD    HL,(VAR1)
        LD    B,16H         ; 22
LBL55:  CALL  SUB38
        JP    Z,LBL56
        CALL  SUB35
        PUSH  HL
        CALL  SUB49
        POP   HL
        DEC   B
        JP    NZ,LBL55
LBL56:  LD    (VAR1),HL
        CALL  SUB5
        RET
LBL57:  CALL  SUB15
        LD    HL,(VAR1)
        LD    BC,1619H    ; 5657
LBL58:  LD    E,L
        LD    D,H
        CALL  SUB42
        LD    A,(HL)
        INC   A
        JP    NZ,LBL59
        LD    L,E
        LD    H,D
        JP    LBL60
LBL59:  PUSH  HL
        CALL  SUB48
        CALL  Z,bios_printChar
        POP   HL
        DEC   B
        JP    NZ,LBL58
LBL60:  LD    (VAR1),HL
        CALL  SUB5
        RET
LBL61:  CALL  SUB48
        JP    NZ,LBL61
        LD    HL,(VAR3)
        LD    (VAR1),HL
        LD    A,08H         ; 8
        LD    (bios_vars.cursorY),A
        JP    LBL54
        LD    HL,REF23    ; 54885
        PUSH  HL
        LD    HL,(VAR6)
        LD    A,H
        OR    L
        JP    NZ,LBL62
        CALL  SUB17
        JP    LBL63
LBL62:  EX    DE,HL
        LD    HL,(VAR3)
        CALL  SUB42
        EX    DE,HL
        CALL  SUB38
        JP    C,LBL63
SUB17:  LD    HL,(VAR3)
        LD    (VAR6),HL
        RET
LBL63:  LD    HL,(VAR3)   ; [2]
        CALL  SUB42
        LD    (VAR7),HL
        RET
        LD    HL,(VAR6)
        LD    A,L
        OR    H
        JP    Z,SUB40
        EX    DE,HL
        LD    HL,(VAR3)
LBL64:  CALL  SUB8
        JP    NC,LBL65
        CALL  SUB35
        JP    NZ,LBL64
LBL65:  LD    (VAR3),HL
        LD    HL,0000H
        LD    (VAR6),HL
        LD    HL,(VAR7)
LBL66:  PUSH  DE
        CALL  SUB39
        LD    B,D
        LD    C,E
        EX    DE,HL
        LD    HL,(VAR1)
        CALL  SUB38
        JP    C,LBL67
        ADD   HL,BC
        LD    (VAR1),HL
LBL67:  LD    HL,(VAR3)
        CALL  SUB38
        JP    C,LBL68
        ADD   HL,BC
        LD    (VAR3),HL
LBL68:  EX    DE,HL
        POP   DE
        CALL  SUB16
        CALL  SUB52
        LD    A,(bios_vars.cursorY)
        SUB   08H           ; 8
        LD    B,A
        JP    Z,LBL70
LBL69:  CALL  SUB35
        CALL  Z,SUB18
        LD    A,B
        SUB   0AH           ; 10
        LD    B,A
        JP    NZ,LBL69
LBL70:  LD    (VAR1),HL
        JP    SUB5
SUB18:  PUSH  HL
        CALL  SUB48
        POP   HL
        RET   NZ
        LD    C,19H         ; 25
        JP    bios_printChar
        LD    HL,(VAR6)
        LD    DE,0000H
        CALL  SUB38
        RET   Z
        EX    DE,HL
        LD    (VAR6),HL
REF23:  CALL  SUB43
        LD    HL,(VAR1)
        CALL  SUB3
        JP    SUB44
LBL71:  LD    HL,REF8             ; 53351; [2]
        CALL  bios_printString
        CALL  SUB40
        CALL  bios_getch
        JP    LBL5
        LD    HL,REF9             ; 53370
        CALL  bios_printString
        CALL  SUB33
        JP    NZ,SUB1
        LD    HL,(0D006H)
        CALL  SUB32
        PUSH  BC
        PUSH  DE
        CALL  SUB19
        EX    (SP),HL
        EX    DE,HL
        LD    HL,LBL2             ; 53258
        LD    A,0E6H                ; 230
        LD    B,05H         ; 5
LBL72:  CALL  SUB24
        DEC   B
        JP    NZ,LBL72
LBL73:  LD    A,(HL)
        CALL  SUB24
        OR    A
        JP    Z,LBL74
        INC   HL
        JP    LBL73
LBL74:  CALL  SUB20
        POP   DE
        LD    A,(0D006H)
        cpl
        INC   A
        LD    L,A
        LD    A,(0D007H)
        cpl
        INC   A
        LD    H,A
        ADD   HL,DE
        LD    A,0E6H                ; 230
        CALL  SUB24
        LD    A,L
        cpl
        CALL  SUB24
        LD    A,H
        cpl
        LD    HL,(0D006H)
        CALL  SUB23
        POP   BC
        LD    A,C
        CALL  SUB24
        LD    A,B
        CALL  SUB24
        JP    SUB1
SUB19:  LD    D,04H         ; 4
        XOR   A
LBL75:  LD    E,40H         ; 64 '@'
        XOR   55H           ; 85 'U'
        CALL  SUB22
        DEC   D
        JP    NZ,LBL75
        RET
SUB20:  CALL  SUB21
SUB21:  XOR   A
        LD    E,A
SUB22:  CALL  SUB24         ; [2]
        DEC   E
        JP    NZ,SUB22
        RET
SUB23:  CALL  SUB24         ; [2]
        CALL  SUB38
        LD    A,(HL)
        INC   HL
        JP    NZ,SUB23
        JP    SUB24
SUB24:  LD    C,A           ; [9]
        JP    bios_tapeWrite
        LD    B,00H
LBL76:  LD    A,B           ; [2]
        LD    (VAR9),A
        LD    HL,0000H
        LD    (VAR6),HL
        LD    HL,REF9             ; 53370
        CALL  bios_printString
        CALL  SUB33
        JP    NZ,SUB1
        CALL  SUB31
        PUSH  HL
LBL77:  CALL  SUB30
        LD    B,A
        LD    A,(VAR9)
        INC   A
        JP    NZ,LBL78
        LD    A,B
        CP    (HL)
        JP    NZ,LBL79
LBL78:  LD    (HL),B
        INC   HL
        INC   B
        JP    NZ,LBL77
        LD    A,08H         ; 8
        CALL  SUB28
        POP   HL
        PUSH  BC
        CALL  SUB32
        EX    (SP),HL
        LD    D,B
        LD    E,C
        CALL  SUB38
        JP    NZ,LBL79
        POP   HL
        LD    (VAR2),HL
        JP    LBL13
LBL79:  POP   HL            ; [2]
        LD    HL,REF12    ; 53384
        CALL  bios_printString
        CALL  SUB40
        CALL  bios_getch
        LD    HL,(VAR2)
        LD    (HL),0FFH   ; 255
        JP    LBL13
SUB25:  LD    HL,0D01AH   ; 53274
        CALL  SUB29
LBL80:  CALL  SUB30
        LD    (HL),A
        OR    A
        JP    Z,LBL81
        INC   HL
        JP    LBL80
LBL81:  LD    HL,REF10    ; 53371
        CALL  bios_printString
        LD    HL,0D01AH   ; 53274
        PUSH  HL
        CALL  bios_printString
        CALL  SUB26
        POP   HL
        LD    DE,LBL2             ; 53258
LBL82:  LD    A,(DE)
        OR    A
        RET   Z
        CP    (HL)
        INC   HL
        INC   DE
        JP    Z,LBL82
        RET
SUB26:  LD    HL,0000H
        LD    E,L
        LD    D,H
LBL83:  DEC   HL
        CALL  SUB38
        RET   Z
        JP    LBL83
SUB27:  LD    A,0FFH                ; 255
SUB28:  CALL  bios_tapeRead
        LD    C,A
        CALL  SUB30
        LD    B,A
        RET
LBL84:  XOR   A
        LD    (bios_vars.tapeInverse),A
SUB29:  LD    B,04H         ; 4
        LD    A,0FFH                ; 255
LBL85:  CALL  bios_tapeRead
        CP    0E6H          ; 230
        JP    NZ,LBL84
        DEC   B
        LD    A,08H         ; 8
        JP    NZ,LBL85
        RET
SUB30:  LD    A,08H         ; 8; [3]
        JP    bios_tapeRead
SUB31:  CALL  SUB25         ; [2]
        JP    NZ,SUB31
        CALL  SUB27
        LD    HL,(0D006H)
        LD    A,(VAR9)
        DEC   A
        JP    M,LBL86
        LD    HL,(VAR2)
LBL86:  LD    A,B
        cpl
        LD    B,A
        LD    A,C
        cpl
        LD    C,A
        PUSH  HL
        EX    DE,HL
        LD    HL,(0D008H)
        EX    DE,HL
        ADD   HL,BC
        CALL  SUB38
        POP   HL
        RET   C
        JP    LBL71
        LD    B,0FFH                ; 255
        JP    LBL76
        LD    B,01H         ; 1
        JP    LBL76
SUB32:  LD    BC,0000H    ; [2]
LBL87:  LD    A,(HL)
        CP    0FFH          ; 255
        RET   Z
        ADD   A,C
        LD    C,A
        LD    A,00H
        ADC   A,B
        LD    B,A
        INC   HL
        JP    LBL87
SUB33:  LD    B,30H         ; 48 '0'; [2]
        LD    HL,LBL2             ; 53258
        LD    E,L
        LD    D,H
LBL88:  CALL  bios_getch          ; [5]
SUB34:  CP    20H           ; 32 ' '
        JP    C,LBL90
        DEC   B
        JP    NZ,LBL89
        INC   B
        JP    LBL88
LBL89:  LD    C,A
        CALL  bios_printChar
        LD    (HL),A
        INC   HL
        JP    LBL88
        LD    BC,2A20H    ; 10784
        CALL  SUB9
        JP    SUB41
LBL90:  CP    1FH           ; 31
        JP    NZ,LBL91
        OR    A
        RET
LBL91:  CP    08H           ; 8
        JP    NZ,LBL92
        CALL  SUB38
        JP    Z,LBL88
        PUSH  HL
        LD    HL,REF11    ; 53380
        CALL  bios_printString
        POP   HL
        DEC   HL
        INC   B
        JP    LBL88
LBL92:  CP    0DH           ; 13
        JP    NZ,LBL88
        XOR   A
        LD    (HL),A
        RET
        CALL  SUB15
        LD    HL,06F9H    ; 1785
        LD    (bios_vars.cursorY),HL
        CALL  SUB56
        LD    HL,REF13    ; 53395
        CALL  bios_printString
        LD    HL,1EF9H    ; 7929
        LD    (bios_vars.cursorY),HL
        CALL  bios_getch
        CP    0DH           ; 13
        JP    Z,LBL93
        LD    BC,1020H    ; 4128
        CALL  SUB41
        LD    (bios_vars.cursorY),HL
        LD    HL,REF14    ; 53403
        LD    E,L
        LD    D,H
        LD    B,0FH         ; 15
        CALL  SUB34
        PUSH  AF
        CALL  SUB57
        POP   AF
        JP    NZ,SUB1
LBL93:  CALL  SUB57
        LD    HL,(VAR1)
        CALL  SUB42
LBL94:  LD    DE,REF14    ; 53403
LBL95:  LD    B,(HL)
        LD    A,(DE)
        OR    A
        JP    Z,LBL96
        CP    B
        INC   HL
        INC   DE
        JP    Z,LBL95
        INC   B
        JP    NZ,LBL94
        CALL  SUB40
        JP    LBL13
LBL96:  CALL  SUB35
        LD    (VAR1),HL
        LD    (VAR3),HL
        CALL  SUB52
        JP    SUB1
        CALL  SUB12
        CALL  SUB10
        LD    HL,(VAR3)
        CALL  SUB42
        LD    A,(HL)
        INC   A
        RET   Z
        DEC   HL
        LD    (HL),20H    ; 32 ' '
        INC   HL
        LD    C,L
        LD    B,H
        LD    HL,REF3             ; 53319
LBL97:  CALL  SUB38
        JP    C,LBL15
        LD    A,(BC)
        CP    0DH           ; 13
        JP    Z,LBL15
        CP    0FFH          ; 255
        JP    Z,LBL15
        LD    (DE),A
        INC   DE
        INC   BC
        JP    LBL97
        LD    HL,(VAR3)
        CALL  SUB14
        LD    E,L
        LD    D,H
        INC   DE
        PUSH  HL
        CALL  SUB16
        POP   HL
        LD    (HL),0DH    ; 13
        JP    SUB5
SUB35:  PUSH  DE            ; [6]
        EX    DE,HL
        LD    HL,(0D006H)
        EX    DE,HL
        CALL  SUB38
        JP    Z,LBL101
        DEC   HL
        CALL  SUB38
        JP    Z,LBL100
        DEC   HL
LBL98:  LD    A,(HL)
        CP    0DH           ; 13
        JP    Z,LBL99
        CALL  SUB38
        DEC   HL
        JP    NZ,LBL98
LBL99:  INC   HL
LBL100:  XOR   A
        INC   A
LBL101:  POP   DE
        RET
        LD    HL,(VAR7)
        EX    DE,HL
        LD    HL,(VAR6)
        LD    A,H
        OR    L
        JP    Z,SUB40
        CALL  SUB39
        LD    HL,(VAR3)
        CALL  SUB8
        PUSH  AF
        CALL  SUB42
        POP   AF
        CALL  C,SUB8
        JP    C,SUB40
        PUSH  HL
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        CALL  SUB16
        LD    HL,(VAR6)
        LD    C,L
        LD    B,H
        POP   HL
        CALL  SUB37
        JP    SUB5
        LD    HL,(VAR7)
        EX    DE,HL
        LD    HL,(VAR6)
        LD    A,H
        OR    L
        JP    Z,SUB40
        CALL  SUB39
        LD    HL,(VAR3)
        CALL  SUB8
        JP    C,SUB40
        CALL  SUB42
        PUSH  HL
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        POP   BC
        PUSH  DE
        PUSH  HL
        PUSH  BC
        CALL  SUB16
        LD    HL,(VAR6)
        LD    C,L
        LD    B,H
        POP   HL
        CALL  SUB37
        LD    HL,(VAR6)
        EX    DE,HL
        POP   HL
        LD    (VAR6),HL
        LD    HL,(VAR7)
        EX    (SP),HL
        LD    (VAR7),HL
        POP   HL
        JP    LBL66
        LD    HL,REF15    ; 53419
        CALL  bios_printString
        CALL  bios_getch
        CP    59H           ; 89 'Y'
        JP    NZ,SUB1
        LD    HL,(0D006H)
        LD    (HL),0DH    ; 13
        INC   HL
        LD    (HL),0FFH   ; 255
        LD    (VAR2),HL
        JP    LBL13
LBL102:  XOR   A
        LD    (VAR4),A
        CALL  SUB43
        CALL  SUB9
        LD    BC,3F20H    ; 16160
        CALL  SUB41
        LD    HL,(VAR3)
        CALL  SUB7
        JP    SUB44
SUB36:  LD    A,(IO_KEYB_B)  ; [3]
        AND   02H           ; 2
        RET
SUB37:  LD    A,(BC)                ; [5]
        LD    (HL),A
        INC   HL
        INC   BC
        CALL  SUB38
        JP    NZ,SUB37
        RET
SUB38:  LD    A,H           ; [34]
        CP    D
        RET   NZ
        LD    A,L
        CP    E
        RET
SUB39:  LD    A,E           ; [9]
        SUB   L
        LD    E,A
        LD    A,D
        SBC   A,H
        LD    D,A
        RET
SUB40:  LD    BC,0407H    ; 1031; [11]
SUB41:  CALL  bios_printChar            ; [5]
        DEC   B
        JP    NZ,SUB41
        RET
SUB42:  LD    A,(HL)                ; [11]
        CP    0FFH          ; 255
        RET   Z
        CP    0DH           ; 13
        INC   HL
        JP    NZ,SUB42
        RET
SUB43:  PUSH  HL            ; [7]
        LD    HL,(bios_vars.cursorY)
        LD    (SMC2+1),HL
        POP   HL
        RET
SUB44:  PUSH  HL            ; [7]
SMC2:   LD    HL,0000H
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET
LBL103: CALL  SUB54
        CALL  SUB35
        LD    (VAR3),HL
        LD    (VAR1),HL
        CALL  SUB52
        CALL  SUB53
        CALL  SUB3
        CALL  bios_getch
        PUSH  AF
        LD    HL,06F9H    ; 1785
        LD    (bios_vars.cursorY),HL
        LD    BC,3020H    ; 12320
        CALL  SUB41
        CALL  SUB4
        POP   AF
        LD    HL,REF21    ; 53535
        PUSH  HL
        JP    LBL11
SUB45:  LD    H,90H         ; 144
        LD    D,H
LBL104: LD    C,39H         ; 57 '9'
        LD    L,01H         ; 1
        LD    E,0BH         ; 11
LBL105: LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        DEC   C
        JP    NZ,LBL105
        LD    C,0AH         ; 10
        XOR   A
LBL106: LD    (HL),A
        INC   HL
        DEC   C
        JP    NZ,LBL106
        LD    A,H
        CP    0BFH          ; 191
        RET   Z
        INC   H
        INC   D
        JP    LBL104
SUB46:  LD    A,(bios_vars.cursorY)  ; [2]
        LD    L,A
        LD    A,0EEH                ; 238
        SUB   L
        OR    A
        rra
        LD    (SMC3+1),A
        LD    H,0BFH                ; 191
        LD    D,H
        LD    B,30H         ; 48 '0'
LBL107: LD    L,0EFH                ; 239
        LD    E,0E5H                ; 229
SMC3:   LD    A,00H
        LD    C,A
        OR    A
        JP    Z,LBL109
LBL108: LD    A,(DE)
        LD    (HL),A
        DEC   HL
        DEC   DE
        LD    A,(DE)
        LD    (HL),A
        DEC   HL
        DEC   DE
        DEC   C
        JP    NZ,LBL108
LBL109: LD    C,09H         ; 9
        XOR   A
LBL110: LD    (HL),A
        DEC   HL
        DEC   DE
        DEC   C
        JP    NZ,LBL110
        DEC   H
        LD    D,H
        DEC   B
        JP    NZ,LBL107
        RET
SUB47:  PUSH  HL            ; [2]
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  SUB56
        LD    HL,9DF9H    ; 40441
        LD    (bios_vars.cursorY),HL
        LD    A,(VAR5)
        OR    A
        LD    HL,REF16    ; 53428
        JP    NZ,LBL111
        LD    HL,REF17    ; 53438
LBL111: CALL  bios_printString
        CALL  SUB57
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET
SUB48:  LD    HL,(VAR3)   ; [5]
        CALL  SUB42
        LD    A,(HL)
        INC   A
        RET   Z
        LD    (VAR3),HL
        LD    HL,(VAR8)
        INC   HL
        LD    (VAR8),HL
        XOR   A
        INC   A
        RET
SUB49:  LD    HL,(VAR3)   ; [3]
        CALL  SUB35
        RET   Z
        LD    (VAR3),HL
        LD    HL,(VAR8)
        DEC   HL
        LD    (VAR8),HL
        RET
SUB50:  PUSH  DE            ; [3]
        PUSH  BC
        PUSH  HL
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  SUB56
        LD    HL,67F9H    ; 26617
        LD    (bios_vars.cursorY),HL
        LD    HL,(VAR8)
        LD    B,20H         ; 32 ' '
        LD    DE,0FC18H   ; 64536
        CALL  SUB51
        LD    DE,03E8H    ; 1000
        ADD   HL,DE
        LD    DE,0FF9CH   ; 65436
        CALL  SUB51
        LD    DE,0064H    ; 100
        ADD   HL,DE
        LD    DE,0FFF6H   ; 65526
        CALL  SUB51
        LD    A,L
        ADD   A,3AH         ; 58 ':'
        LD    C,A
        CALL  bios_printChar
        CALL  SUB57
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        POP   BC
        POP   DE
        RET
SUB51:  LD    C,2FH         ; 47 '/'; [4]
LBL112: INC   C
        ADD   HL,DE
        JP    C,LBL112
        LD    A,C
        CP    30H           ; 48 '0'
        JP    Z,LBL113
        LD    B,0FFH                ; 255
LBL113: AND   B
        LD    C,A
        JP    bios_printChar
SUB52:  LD    HL,(VAR3)   ; [3]
        EX    DE,HL
        LD    HL,(0D006H)
        LD    (VAR3),HL
        LD    HL,0001H    ; 1
        LD    (VAR8),HL
LBL114: LD    HL,(VAR3)
        CALL  SUB38
        RET   Z
        CALL  SUB48
        JP    LBL114
SUB53:  PUSH  HL            ; [2]
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  SUB56
        LD    H,90H         ; 144
        LD    C,30H         ; 48 '0'
LBL115: LD    L,0F1H                ; 241
        LD    A,09H         ; 9
LBL116: LD    (HL),0FFH   ; 255
        INC   L
        DEC   A
        JP    NZ,LBL116
        INC   H
        DEC   C
        JP    NZ,LBL115
        LD    HL,58F9H    ; 22777
        LD    (bios_vars.cursorY),HL
        LD    HL,REF18    ; 53448
        CALL  bios_printString
        LD    HL,80F9H    ; 33017
        LD    (bios_vars.cursorY),HL
        LD    HL,REF19    ; 53453
        CALL  bios_printString
        CALL  SUB50
        CALL  SUB47
        CALL  SUB57
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET
SUB54:  POP   HL            ; [3]
SMC4:   LD    SP,8F50H    ; 36688
        JP    (HL)
LBL117: CALL  SUB12
        CALL  SUB13
        LD    DE,REF4             ; 53320
        LD    C,18H         ; 24
LBL118: CALL  SUB38
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    Z,LBL119
        INC   HL
        CALL  bios_printChar
        JP    LBL118
LBL119: CALL  SUB38         ; [2]
        JP    Z,LBL22
        LD    A,(HL)
        CP    20H           ; 32 ' '
        RET   NZ
        INC   HL
        CALL  bios_printChar
        JP    LBL119
LBL120: CALL  SUB12
        CALL  SUB13
        LD    DE,LBL2             ; 53258
        LD    C,08H         ; 8
        CALL  SUB38
        RET   Z
        DEC   HL
        DEC   DE
LBL121: CALL  SUB38
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    NZ,LBL122
        CALL  bios_printChar
        DEC   HL
        JP    LBL121
LBL122: CALL  SUB38         ; [2]
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        RET   Z
        DEC   HL
        CALL  bios_printChar
        JP    LBL122
SUB55:  PUSH  HL
        PUSH  DE
        LD    HL,0000H
        ADD   HL,SP
        LD    (SMC5+1),HL
        LD    DE,0000H
        LD    HL,0BFF0H   ; 49136
        LD    C,30H         ; 48 '0'
LBL123: LD    SP,HL
        LD    A,0FH         ; 15
LBL124: PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        DEC   A
        JP    NZ,LBL124
        DEC   H
        DEC   C
        JP    NZ,LBL123
SMC5:   LD    SP,0FFFFH   ; 65535
        POP   DE
        POP   HL
        RET
SUB56:  PUSH  HL            ; [5]
        LD    HL,0FFFFH   ; 65535
LBL125: LD    (bios_vars.inverse),HL
        POP   HL
        RET
SUB57:  PUSH  HL            ; [6]
        LD    HL,0000H
        JP    LBL125
SUB58:  CALL  SUB63         ; [3]
        LD    DE,0DF0AH   ; 57098
        LD    BC,REF25    ; 56771
        CALL  bios_memcpy_bc_hl
        LD    HL,00F9H    ; 249
        LD    (bios_vars.cursorY),HL
        LD    HL,0FFFFH   ; 65535
        LD    (bios_vars.inverse),HL
        LD    HL,REF24    ; 56766
        CALL  bios_printString
        CALL  bios_getch
        LD    HL,0FF9H    ; 4089
        LD    (bios_vars.cursorY),HL
        CP    0DH           ; 13
        JP    Z,LBL126
        PUSH  AF
        LD    HL,REF28    ; 56800
        CALL  bios_printString
        LD    HL,0FF9H    ; 4089
        LD    (bios_vars.cursorY),HL
        LD    HL,REF25    ; 56771
        LD    BC,REF25    ; 56771
        LD    DE,REF26    ; 56781
        POP   AF
        JP    LBL131
LBL126: LD    HL,REF25    ; 56771; [6]
        LD    DE,REF26    ; 56781
        LD    BC,0DF00H   ; 57088
        CALL  bios_memcpy_bc_hl
        LD    HL,0000H
        LD    (bios_vars.inverse),HL
        RET
SUB59:  CALL  SUB60
        LD    HL,(0D006H)
        EX    DE,HL
        LD    HL,REF27    ; 56784
        CALL  bios_fileLoad2
        JP    C,LBL135
        JP    LBL126
SUB60:  LD    HL,REF25    ; 56771; [3]
        LD    DE,REF27    ; 56784
        CALL  bios_fileNamePrepare
        RET
SUB61:  CALL  SUB60
        LD    DE,0000H
        LD    B,00H
        LD    HL,(0D006H)
LBL127: LD    A,B
        ADD   A,(HL)
        LD    B,A
        INC   DE
        LD    A,(HL)
        INC   A
        JP    Z,LBL128
        INC   HL
        JP    LBL127
LBL128: LD    HL,(0D006H)
        LD    (VAR10),HL
        EX    DE,HL
        LD    (VAR11),HL
        LD    A,B
        LD    (VAR12),A
        LD    HL,REF27    ; 56784
        CALL  bios_fileCreate
        JP    C,LBL135
        JP    LBL126
LBL129: POP   AF
        JP    SUB1
        LD    B,H
        LD    C,L
LBL130: CALL  bios_getch          ; [4]
LBL131: CP    08H           ; 8
        JP    Z,LBL132
        CP    0DH           ; 13
        JP    Z,LBL134
        CP    1FH           ; 31
        JP    Z,LBL140
        CP    20H           ; 32 ' '
        JP    C,LBL130
        PUSH  AF
        CALL  bios_cmp_hl_de
        JP    Z,LBL129
        POP   AF
        LD    (HL),A
        PUSH  BC
        LD    C,A
        CALL  bios_printChar
        POP   BC
        INC   HL
        JP    LBL130
LBL132: LD    A,H
        CP    B
        JP    NZ,LBL133
        LD    A,L
        CP    C
        JP    Z,LBL130
LBL133: DEC   HL
        PUSH  BC
        LD    C,08H         ; 8
        CALL  bios_printChar
        LD    C,20H         ; 32 ' '
        CALL  bios_printChar
        LD    C,08H         ; 8
        CALL  bios_printChar
        POP   BC
        JP    LBL130
LBL134: LD    (HL),00H
        JP    LBL126
LBL135:  LD    C,05H         ; 5; [5]
LBL136: CALL  bios_beep_Old
        LD    B,0FFH                ; 255
        CALL  bios_delay_b
        CALL  bios_delay_b
        DEC   C
        JP    NZ,LBL136
        JP    LBL126
SUB62:  CALL  SUB60
        LD    HL,REF27    ; 56784
        CALL  bios_fileLoadInfo
        LD    HL,(VAR11)
        EX    DE,HL
        PUSH  DE
        LD    HL,(0D006H)
LBL137:  LD    A,(HL)
        INC   A
        JP    Z,LBL138
        INC   HL
        JP    LBL137
LBL138:  POP   DE
        PUSH  HL
        ADD   HL,DE
        EX    DE,HL
        LD    HL,(0D008H)
        LD    A,H
        CP    D
        JP    C,LBL135
        JP    NZ,LBL139
        LD    A,L
        CP    E
        JP    C,LBL135
LBL139: POP   DE
        LD    HL,REF27    ; 56784
        CALL  bios_fileLoad2
        JP    C,LBL135
        JP    LBL126
LBL140: POP   DE
        LD    HL,0000H
        LD    (bios_vars.inverse),HL
        JP    LBL10
REF24:  DB    46H,49H,4CH,45H           ; "FILE"
        DB    3AH                   ; ":"
REF25:  DB    00H,00H,00H,00H           ; "...."
        DB    00H,00H,00H,00H           ; "...."
        DB    00H,00H                     ; ".."
REF26:  DB    00H,00H,00H         ; "..."
REF27:  DB    20H,20H,20H,20H           ; "    "
        DB    20H,20H,20H,20H           ; "    "
        DB    20H,20H                     ; "  "
VAR10:  DW    2020H
VAR11:  DW    2020H
VAR12:  DB    20H
        DB    20H                   ; " "
REF28:  DB    20H,20H,20H,20H           ; "    "
        DB    20H,20H,20H,20H           ; "    "
        DB    20H,20H,20H,20H           ; "    "
        DB    00H                   ; "."
LBL141:
        CALL  bios_getch
        LD    C,A
        CP    03H           ; 3
        JP    Z,LBL142
        CP    04H           ; 4
        JP    Z,LBL143
        CP    05H           ; 5
        JP    Z,LBL144
        JP    LBL11
LBL142:
        CALL  SUB58
        CALL  SUB59
        JP    LBL3
LBL143:
        CALL  SUB58
        CALL  SUB61
        JP    SUB1
LBL144:
        CALL  SUB58
        CALL  SUB62
        JP    LBL3
        NOP
SUB63:
        CALL  SUB15
        LD    HL,0DF00H   ; 57088
        RET
LBL145:
        EX    DE,HL
        LD    DE,REF29    ; 56892
        CALL  bios_fileNamePrepare
        LD    HL,(0D006H)
        EX    DE,HL
        LD    HL,REF29    ; 56892
        CALL  bios_fileLoad2
        JP    LBL2
        DB    00H,00H                     ; ".."
REF29:  DB    00H,00H,00H,00H           ; "...."
