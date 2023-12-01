;----------------------------------------------------------------------------
; MXOS - MON2.COM
;
; 2022-02-07 ����������������� SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

; ������������ ������������ DOS.SYS:
; bios_keyScanOld      = 0C003h
; bios_drawCursorOld   = 0C006h
; bios_printCharOld    = 0C037h
; bios_beep_Old        = 0C170h
; bios_getchOld        = 0C337h
; bios_tapeReadOld     = 0C377h
; bios_tapeWriteOld    = 0C3D0h
; bios_cmp_hl_de       = 0C427h
; bios_printStringOld  = 0C438h
; bios_reboot          = 0C800h
; bios_getch           = 0C803h
; bios_printChar       = 0C809h
; bios_input           = 0C80Fh
; bios_keyCheck        = 0C812h
; bios_printHexByte    = 0C815h
; bios_printString     = 0C818h
; bios_keyScan         = 0C81Bh
; bios_getCursorPos    = 0C81Eh
; bios_tapeLoad        = 0C824h
; bios_tapeSave        = 0C827h
; bios_calcCS          = 0C82Ah
; bios_getMemTop       = 0C830h
; bios_setMemTop       = 0C833h
; bios_printer         = 0C836h
; bios_fileList        = 0C83Fh
; bios_fileGetSetDrive = 0C842h
; bios_fileCreate      = 0C845h
; bios_fileLoad        = 0C848h
; bios_fileLoadInfo    = 0C851h
; bios_fileGetSetAddr  = 0C854h
; bios_fileGetSetAttr  = 0C857h
; bios_fileNamePrepare = 0C85Ah
; bios_fileLoad2       = 0C866h

; ������������ ���������� DOS.SYS:
; bios_vars.tapeError  = 8FE1h
; bios_vars.tapeAddr   = 8FE3h
; bios_vars.cursorCfg  = 8FE9h
; bios_vars.koi7       = 8FEAh
; bios_vars.cursorX    = 8FFCh

;----------------------------------------------------------------------------
; ����������� ����������
INPUT_BUF     = 8F60h
INPUT_BUF_END = 8F6Dh

    STRUCT MON2_VARS_1
Directive:      DS    1
FirstParam:     DS    2
SecondParam:    DS    2
ThirdParam:     DS    2
V_8F87:         DS    2
V_8F89:         DS    1
V_8F8A:         DS    1
V_8F8B:         DS    1
V_8F8C:         DS    2
V_8F8E:         DS    2
V_8F90:         DS    1
V_8F91:         DS    4
V_8F95:         DS    1
V_8F96:         DS    1
V_8F97:         DS    2
V_8F99:         DS    1
V_8F9A:         DS    3
V_8F9D:         DS    2
V_8F9F:         DS    2
V_8FA1:         DS    2
V_8FA3:         DS    2
V_8FA5:         DS    2
V_8FA7:         DS    4
V_8FAB:         DS    2
    ENDS

    STRUCT MON2_VARS_2
V_F0E0:         DS    5
V_F0E5:         DS    1
V_F0E6:         DS    1
V_F0E7:         DS    1
    ENDS

;----------------------------------------------------------------------------
; ������ ���������
    ORG   0F100h

; ������ ������ ����������
vars1       MON2_VARS_1     = 08F80H
vars2       MON2_VARS_2     = 0F0E0H
v_fileDescr FILE_DESCRIPTOR = 0F900H

; ����� ��� �������� ����������
; � ����� ����� ������ ����� ����������� ���
; ����� ��������� - ��� ������ �����
    IF  NEW_MEMORY_MAP
v_fileInfo  FILE_INFO = 0E800H
DIR_BUFFER_SIZE = ($ - v_fileInfo) / FILE_INFO_SIZE - 1
    ELSE
v_fileInfo  FILE_INFO = v_fileDescr + FILE_DESCRIPTOR_SIZE
DIR_BUFFER_SIZE = (0FB00h - v_fileInfo) / FILE_INFO_SIZE - 1
    ENDIF

;----------------------------------------------------------------------------
; ���

        ; �������� � ������� ������
        LD    C,1FH ; rjl ������� ������� ������
        CALL  bios_printChar

        ; ������� ��� ������� ������
RestartNoCls:
        LD    SP,0FFBFH
        LD    HL,mon2_restart           ; ���������� ���������� ������ �����������
        LD    (bios_vars.tapeError),HL  ; �� ������� ��������
        LD    HL,7EFFH    ; 32511
        LD    (vars1.V_8FAB),HL
        CALL  MonitorMain
        JP    RestartNoCls

;----------------------------------------------------------------------------
; ������� ������������ - ���� ���������, ������, ����������

MonitorMain:
        ; ������� ������
        LD    HL,txtNewLine
        CALL  bios_printStringOld

        ; ������ ������� ����
        LD    E,02H
        CALL  bios_fileGetSetDrive

        ; ������ ������� A:\>
        ADD   A,'A'
        LD    C,A
        CALL  bios_printChar
        LD    C,':'
        CALL  bios_printChar
        LD    C,'\'
        CALL  bios_printChar
        LD    C,'>'
        CALL  bios_printChar

        ; ������� ������
        CALL  ClearInputBuf

        ; ���� ������
        CALL  SUB19

        PUSH  HL
        PUSH  DE
        PUSH  BC

        ; ������ ������� ����
        LD    E,02H
        CALL  bios_fileGetSetDrive
        PUSH  AF

        ; ������������� ��� ����� � ������
        LD    HL,INPUT_BUF
        LD    DE,v_fileInfo.name
        CALL  bios_fileNamePrepare

        ; ������ ������� ����
        LD    E,02H
        CALL  bios_fileGetSetDrive

        LD    E,A
        POP   AF
        CP    E
        POP   BC
        POP   DE
        POP   HL
        RET   NZ

        ; ������� ������
        CALL  printNewLine

        ; ������ ������
        CALL  Tokenizer

        ; ������ ����� ���������
        CP    44H           ; ��������� D - ���� ����� ������ � HEX ����
        JP    Z,Dir_D
        CP    4DH           ; ��������� M - �������������� ����� ������
        JP    Z,Dir_M
        CP    4CH           ; ��������� L - ���� ����� ������ � ��������� ����
        JP    Z,Dir_L
        CP    4BH           ; ��������� K - ������� ����������� ����� ����� ������
        JP    Z,Dir_K
        CP    54H           ; ��������� T - ����������� ����� ������ �� ����� �����
        JP    Z,Dir_T
        CP    58H           ; ��������� X - ������ ����������� ��������� ����������
        JP    Z,Dir_X
        CP    57H           ; ��������� W - ������ ����� ������ �� ����� ��� �����
        JP    Z,mon2_tapeSave
        CP    52H           ; ��������� R - ������ ����� � �����
        JP    Z,Dir_R
        CP    43H           ; ��������� C - ��������� ���� ������ ������
        JP    Z,Dir_C
        CP    48H           ; ��������� H - ������ ����� � �������� ���� HEX ����
        JP    Z,Dir_H
        CP    4EH           ; ��������� N - ???
        JP    Z,Dir_N
        CP    47H           ; ��������� G - ������ ��������� �� ������
        JP    Z,Dir_G
        CP    46H           ; ��������� F - ��������� ���� ������ ������
        JP    Z,Dir_F
        CP    53H           ; ��������� S - ???
        JP    Z,Dir_S
        CP    4AH           ; ��������� J - ����� �� M�������
        JP    Z,Dir_J
        CP    3FH           ; ��������� ? - ������� ����������
        JP    Z,Dir_DirDisk
        CP    42H           ; ��������� B - ��������� ������� ������ � ����
        JP    Z,Dir_B
        CP    41H           ; ��������� A - ��������� ������ �������� �����
        JP    Z,Dir_A
        CP    56H           ; ��������� V - �������� ����� � ������ �� ���������� ������
        JP    Z,Dir_V
        CP    55H           ; ��������� U - �������� ����� � ������
        JP    Z,Dir_U 
        CP    59H           ; ��������� Y - ��������� ��������� �����
        JP    Z,Dir_Y
        CP    51H           ; ��������� Q - ������ ��������� �����
        JP    Z,Dir_Q

        ; ��������� �� ���������� - ������ ����� ������� � ����������
        JP    TypeErrorRestart

;----------------------------------------------------------------------------
; ��������� J - ����� �� M�������

Dir_J:  CALL  bios_keyCheck
        INC   A
        JP    Z,bios_reboot
        JP    Dir_J

;----------------------------------------------------------------------------
; ������ ����� 0000h � ������ ������ ��� ����� ������ INPUT_BUF

ClearInputBuf:
        LD    HL,INPUT_BUF
        LD    (HL),00H
        INC   HL
        LD    (HL),00H
        RET

;----------------------------------------------------------------------------
; ��������� ? - ����� �������� �����

Dir_DirDisk:
        LD    BC, 0                 ; �������� � 0 �����
        LD    HL, v_fileInfo.name   ; ����� ������
        LD    DE, DIR_BUFFER_SIZE   ; ������ ������ � ������
        CALL  bios_fileList
DirDiskLoop:
        LD    A,(HL)            ; ������ ������ ����� �����
        INC   A
        RET   Z                 ; ���� FF (����� ��������) - �������
        CALL  bios_keyCheck     ; ���������, �� ������ �� �������
        CP    1FH               ; ���
        CALL  Z,bios_getch      ; ���� ������ ���, ���� ������� ��� ��� ���
        CP    1FH               ; ���
        RET   Z                 ; �����, ���� ��� ������ ��� ���
        LD    B,08H             ; �������� 8 �������� ����� �����
        CALL  printStringB
        LD    C,2EH             ; �������� �����
        CALL  bios_printChar
        LD    B,03H             ; �������� 3 ������� ���������� �����
        CALL  printStringB
        CALL  printSpace
        INC   HL                ; ��������� ����� ���� �������� �����
        LD    E,(HL)            ; DE = ����� �������� �����
        INC   HL
        LD    D,(HL)
        PUSH  DE                ; ����� �������� ����� � �����
        INC   HL
        LD    E,(HL)            ; DE = ������ �����
        INC   HL
        LD    D,(HL)
        EX    (SP),HL           ; ������� ��������� ����������� � ����, � HL = ����� �������� �����
        CALL  printHexWordHL    ; �������� ��������� ����� �������� �����
        ADD   HL,DE
        CALL  printHexWordHL    ; �������� �������� ����� �������� �����
        POP   HL                ; HL = ��������� �����������
        LD    A,L
        AND   0F0H
        LD    L,A               ; �������� ������� 4 ���� HL - ������� �� ������ �������� ����������� �����
        LD    DE,0010H
        ADD   HL,DE             ; HL += 16 - ������� �� ��������� ���������� �����
        CALL  printNewLine
        JP    DirDiskLoop

;----------------------------------------------------------------------------
; ������ ������ �� ������ HL ������ B

printStringB:
        LD    C,(HL)
        CALL  bios_printChar
        INC   HL
        DEC   B
        JP    NZ,printStringB
        RET

;----------------------------------------------------------------------------
; ��������� B - ���������� ������� ������� � ����

Dir_B:  ; ����� ������ - � ����������
        LD    (v_fileDescr.loadAddress), HL

        ; DE = -HL
        EX    DE,HL
        LD    A,D
        CPL
        LD    D,A
        LD    A,E
        CPL
        LD    E,A
        INC   DE

        ; HL = �����_����� - �����_������ = ������ - 1
        ADD   HL,DE

        ; ������ - � ����������
        LD    (v_fileDescr.size),HL

        ; ������ ����� �����
        CALL  EnterFileName

        ; ���������� �����
        CALL  bios_fileCreate

        ; � ������ ������ �������� ������� "��� ����"
        LD    HL,txtSmallDisk
        CALL  C,SUB4
        RET

        ; ��� "��� DIR"
SUB4:   OR    A
        JP    Z,LBL6
        JP    bios_printString

LBL6:   LD    HL, txtSmallDir
        JP    bios_printString

;----------------------------------------------------------------------------
; ������� ����� � ��������� ���-7

txtSmallDisk:   DB  0AH,0DH,"SMALL DISK!",00H
txtSmallDir:    DB  0AH,0DH,"SMALL DIR!",00H
txtNoFile:      DB  0AH,0DH,"NO FILE ",00H

;----------------------------------------------------------------------------
; ��������� A - ��������� ������ �������� �����

Dir_A:  PUSH  HL
        CALL  EnterFileName
        POP   DE
        LD    C,01H
        CALL  bios_fileGetSetAddr
        LD    HL,txtNoFile
        CALL  C,bios_printString
        RET

;----------------------------------------------------------------------------
; ��������� V - �������� ����� � ������ �� ���������� ������

Dir_V:  ; ���������� ������ �������� ��������� � �����
        PUSH  HL

        ; ������ ����� �����
        CALL  EnterFileName

        ; ����������� ������ �������� ��������� �� �����
        POP   DE
        PUSH  DE

        ; ��������� ���� �� ������, ���������� � ���������
        CALL  bios_fileLoad2

        ; ���� ���� �� ������ - ����� ��������� "��� �����" � �����
        PUSH  AF
        LD    HL,txtNoFile
        CALL  C,bios_printString
        POP   AF
        POP   DE
        RET   C

        ; ��������� ���������� � �����
        PUSH  DE
        LD    HL,v_fileDescr.name
        CALL  bios_fileLoadInfo

        ; ������ �����
        LD    HL,(v_fileDescr.size)
        EX    DE,HL

        ; ������ ���������� ������
        POP   HL
        CALL  printHexWordHL

        ; ������ ��������� ������
        ADD   HL,DE
        CALL  printHexWordHL
        RET

;----------------------------------------------------------------------------
; ��������� U - �������� ����� � ������

Dir_U:  ; ������ ����� �����
        CALL  EnterFileName
        CALL  bios_fileLoad

        ; ���� ���� �� ������ - ����� ��������� "��� �����" � �����
        PUSH  AF
        LD    HL,txtNoFile
        CALL  C,bios_printString
        CALL  printSpace
        POP   AF
        RET   C

        ; ��������� ���������� � �����
        LD    HL,v_fileDescr.name
        CALL  bios_fileLoadInfo

        ; ������ ������ ��������
        LD    HL,(v_fileDescr.loadAddress)
        PUSH  HL
        CALL  printHexWordHL
        POP   DE

        ; ������ ������� �����
        LD    HL,(v_fileDescr.size)
        ADD   HL,DE
        CALL  printHexWordHL
        RET

;----------------------------------------------------------------------------
; ��������� Y - ��������� ��������� �����

Dir_Y:  PUSH  HL
        CALL  EnterFileName         ; ���� ����� �����
        LD    C,01H                 ; ����� ��� �������� bios_fileGetSetAttr - ��������� ����� ��������� �����
        EX    (SP),HL
        LD    A,L
        POP   HL
        CALL  bios_fileGetSetAttr   ; ��������� ����� ��������� �����
        LD    HL,txtNoFile
        CALL  C,bios_printString    ; ���� ���� �� ������ - ������� ���������
        RET

;----------------------------------------------------------------------------
; ��������� Q - ������ ��������� �����

Dir_Q:
        CALL  EnterFileName         ; ���� ����� �����
        LD    C,02H                 ; ����� ��� �������� bios_fileGetSetAttr - ������ ����� ��������� �����
        CALL  bios_fileGetSetAttr   ; ������ ����� ��������� �����
        PUSH  AF                    ; ��������� � ���� ������ (cf) - � ����
        LD    HL,txtNoFile
        CALL  C,bios_printString    ; ���� ���� �� ������ - ������� ���������
        CALL  printSpace
        POP   AF                    ; ����������� ���� ���������
        CALL  NC,bios_printHexByte  ; �������� ���� ���������, ���� �� ���� ������
        RET

;----------------------------------------------------------------------------
; ������ ����� �����
; �����:
;   HL = ����� ������ � �������������� ������ �����

EnterFileName:
        LD    HL,txtFileQuestMark   ; ������ ����������� ������ ��� �����
        CALL  bios_printString
        LD    HL,INPUT_BUF          ; ����� ��� ����� ������
        LD    DE,INPUT_BUF_END
        CALL  bios_input
        LD    DE,v_fileDescr.name   ; ����� ��� ��������������� ����� �����
        CALL  bios_fileNamePrepare  ; �������������� ��� �����
        EX    DE,HL                 ; HL = �������������� ��� �����
        RET

;----------------------------------------------------------------------------
; ��������� R - ������ ����� � �����

Dir_R:  ; �������� �����
        CALL  mon2_tapeLoad
        PUSH  BC
        PUSH  DE
        PUSH  HL

        ; ������ ����������� �����
        CALL  bios_calcCS

        ; ������ ���������� ������
        POP   HL
        CALL  printHexWordHL

        ; ������ ��������� ������
        POP   HL
        CALL  printHexWordHL

        ; ��������� ����������� ����� �� ����� � ������������
        POP   HL
        LD    D,B
        LD    E,C
        CALL  bios_cmp_hl_de

        ; ���� �� ��������� - �������
        JP    NZ,LBL13

        ; ����� ������ ����������� ����� � �����
        CALL  printHexWordHL
        RET

LBL13:  ; ������ ����� ������� � �����
        CALL  printSpace
        LD    C, '?'
        CALL  bios_printChar
        RET

;----------------------------------------------------------------------------

SUB6:   PUSH  AF
        LD    A,(bios_vars.cursorCfg)
        PUSH  AF
        LD    A,11H
        JP    LBL63

        ; ???
        NOP
        NOP

;----------------------------------------------------------------------------
; ��������� H - ������ ����� � �������� ���� HEX ����

Dir_H:  PUSH  HL
        ADD   HL,DE             ; �����
        CALL  printHexWordHL    ; �������� ��
        CALL  printSpace        ; �������� �������

        ; DE = -DE
        LD    A,E
        CPL
        LD    E,A
        LD    A,D
        CPL
        LD    D,A
        INC   DE

        POP   HL
        ADD   HL,DE             ; ��������
        CALL  printHexWordHL    ; �������� ��
        RET

;----------------------------------------------------------------------------
; ��������� X - ������ ����������� ��������� ����������

Dir_X:  LD    HL,vars1.V_8FA3+1 ; 36772
        LD    DE,txtRegisters             ; 63244
        LD    C,04H         ; 4
LBL16:  PUSH  BC
        CALL  SUB7
        POP   BC
        DEC   C
        JP    NZ,LBL16
        CALL  SUB8
        LD    HL,(vars1.V_8F9D)
        CALL  printHexWordHL
        CALL  SUB8
        LD    HL,(vars1.V_8F97)
        CALL  printHexWordHL
        CALL  SUB8
        LD    HL,(vars1.V_8FA5)
        CALL  printHexWordHL
        RET

;----------------------------------------------------------------------------
; ���� ������ �������, �������� 0FFh �� ������ (HL)

setMIfKeyPressed:
        CALL  bios_keyScan
        INC   A
        RET   Z
        LD    (HL), 0FFh
        RET

;----------------------------------------------------------------------------

SUB7:   LD    B,(HL)
        DEC   HL
        LD    C,(HL)
        DEC   HL
        PUSH  BC
        CALL  SUB8
        LD    A,B
        CALL  mon2_printHexByte
        CALL  SUB8
        POP   BC
        LD    A,C
        CALL  mon2_printHexByte
        RET
SUB8:   EX    DE,HL         ; [5]
        PUSH  BC
        CALL  printSpace
        CALL  bios_printStringOld
        INC   HL
        POP   BC
        EX    DE,HL
        RET

;----------------------------------------------------------------------------
; ��������� G - ������ ��������� �� ������ (������ ��������)
; ������ �������� - ����� ���������?

Dir_G:  LD    A,E
        OR    A
        JP    NZ,LBL19  ; ���� � E �� ����
        LD    A,D
        OR    A
        JP    Z,LBL20   ; ���� � D �� ����

LBL19:  PUSH  HL
        EX    DE,HL
        LD    (vars1.V_8F97),HL
        LD    A,(HL)
        LD    (HL),0FFH
        LD    (vars1.V_8F99),A
        LD    HL,0038H
        LD    DE,vars1.V_8F9A
        LD    BC,REF4
        CALL  SUB11
        LD    (HL),0C3H
        CALL  SUB10
        CALL  SUB10
        LD    (HL),B
        DEC   HL
        LD    (HL),C
        POP   HL
LBL20:  CALL  SUB9
        JP    RestartNoCls
SUB9:   JP    (HL)
SUB10:  INC   HL
        INC   DE
SUB11:  LD    A,(HL)
        LD    (DE),A
        RET

REF4:   LD    (vars1.V_8F9D),HL
        EX    DE,HL
        LD    (vars1.V_8F9F),HL
        PUSH  BC
        POP   HL
        LD    (vars1.V_8FA1),HL
        PUSH  AF
        POP   HL
        LD    (vars1.V_8FA3),HL
        LD    HL,0000H
        ADD   HL,SP
        INC   HL
        INC   HL
        LD    (vars1.V_8FA5),HL
        LD    HL,(vars1.V_8F97)
        LD    A,(vars1.V_8F99)
        LD    (HL),A
        LD    HL,vars1.V_8F9A
        LD    DE,0038H
        CALL  SUB11
        CALL  SUB10
        CALL  SUB10
        JP    RestartNoCls

;----------------------------------------------------------------------------
; ��������� S - ???

Dir_S:  PUSH  HL
        PUSH  BC
        LD    HL,INPUT_BUF
        LD    C,00H
LBL22:  LD    A,(HL)
        CP    2CH           ; ������ ','
        JP    NZ,LBL23
        INC   C             ; ���������� ','
LBL23:  INC   HL
        CP    0DH           ; ������� ������
        JP    NZ,LBL22
        DEC   C
        LD    A,C
        LD    (vars1.V_8F89),A
        CP    01H
        POP   BC
        POP   HL
        JP    Z,LBL24
        PUSH  HL
        PUSH  DE
        PUSH  BC
        LD    HL,(vars1.V_8F87)
        LD    C,L
        LD    B,H
        CALL  SUB15
        LD    A,L
        LD    (vars1.V_8F8B),A
        LD    HL,vars1.V_8F8C
        LD    (HL),E
        INC   HL
        LD    (HL),C
        POP   BC
        POP   DE
        POP   HL
LBL24:  LD    A,C
        LD    (vars1.V_8F8A),A
LBL25:  PUSH  HL
        LD    HL,vars1.V_8F8A
        LD    (vars1.V_8F8E),HL
        POP   HL
        CALL  bios_cmp_hl_de
        RET   Z
        CALL  WaitClsKey
        LD    A,(vars1.V_8F89)
        LD    (vars1.V_8F90),A
        LD    B,A
LBL26:  LD    C,(HL)
        PUSH  HL
        LD    HL,(vars1.V_8F8E)
        LD    A,(HL)
        CP    C
        JP    NZ,LBL30
        INC   HL
        LD    (vars1.V_8F8E),HL
        POP   HL
        INC   HL
        DEC   B
        JP    NZ,LBL26
        PUSH  HL
        PUSH  DE
        PUSH  BC
        PUSH  AF
        LD    A,(vars1.V_8F89)
LBL27:  DEC   HL
        DEC   A
        JP    NZ,LBL27
        CALL  printHexWordHL
        DEC   HL
        DEC   HL
        LD    A,(vars1.V_8F89)
        ADD   A,04H         ; 4
        LD    B,A
LBL28:  CALL  SUB12
        INC   HL
        DEC   B
        JP    NZ,LBL28
        LD    HL,txtHomeRight11Dot
        CALL  mon2_printString
        LD    A,(vars1.V_8F89)
        LD    B,A
        ADD   A,B
        ADD   A,B
        DEC   A
        LD    E,A
        LD    C,18H         ; 24
LBL29:  CALL  SUB6
        DEC   E
        JP    NZ,LBL29
        CALL  printNewLine
        POP   AF
        POP   BC
        POP   DE
        POP   HL
        JP    LBL25
LBL30:  POP   HL
        INC   HL
        JP    LBL25
SUB12:  LD    A,(HL)
        PUSH  BC
        CALL  mon2_printHexByte
        CALL  printSpace
        POP   BC
        RET

;----------------------------------------------------------------------------

Dir_N:  LD    A,(HL)
        CP    C
        JP    Z,LBL32
        CALL  printHexWordHL
        LD    A,(HL)
        PUSH  BC
        CALL  mon2_printHexByte
        CALL  WaitClsKey
        CALL  printNewLine
        POP   BC
LBL32:  CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        JP    Dir_N

;----------------------------------------------------------------------------
; ��������� F - ��������� ���� ������ ������

Dir_F:  LD    (HL),C
        CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        JP    Dir_F

;----------------------------------------------------------------------------
; ��������� T - ����������� ����� ������ �� ����� �����
; ��������������� ������� ������ ����� ���������

Dir_T:  LD    A,(HL)
        LD    (BC),A
        CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        INC   BC
        JP    Dir_T

;----------------------------------------------------------------------------
; ��������� C - ��������� ���� ������ ������

Dir_C:  LD    A,(BC)
        CP    (HL)
        JP    Z,LBL36
        PUSH  BC
        CALL  printHexWordHL
        LD    A,(HL)
        CALL  mon2_printHexByte
        CALL  printSpace
        POP   BC
        PUSH  BC
        LD    A,(BC)
        CALL  mon2_printHexByte
        CALL  WaitClsKey
        CALL  printNewLine
        POP   BC
LBL36:  CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        INC   BC
        JP    Dir_C

;----------------------------------------------------------------------------
; ������ ����� � HEX ������� �� HL, ����������� ���������

printHexWordHL:
        PUSH  BC
        CALL  printSpace
printHexWordHL_noSpace:
        LD    A,H
        CALL  mon2_printHexByte
        LD    A,L
        CALL  mon2_printHexByte
        CALL  printSpace
        POP   BC
        RET

;----------------------------------------------------------------------------
; ������ � ������ ����������� �����
; ����:
;   hl = ��������� �����
;   de = �������� �����

Dir_K:
        CALL  mon2_calcCS
        PUSH  BC
        POP   HL    ; hl = bc
        CALL  printHexWordHL
        RET

;----------------------------------------------------------------------------
; ��������� L - ���� ����� ������ � ��������� ����

Dir_L:  CALL  printHexWordHL
        LD    B,10H         ; 16
LBL40:  LD    C,(HL)
        LD    A,(vars1.ThirdParam)
        OR    A
        JP    Z,LBL41
        AND   C
        LD    C,A
LBL41:  LD    A,C
        CP    20H           ; 32 ' '
        JP    C,LBL42
        CP    7FH           ; 127
        JP    C,LBL43
LBL42:  LD    C,2EH         ; 46 '.'
LBL43:  CALL  mon2_printChar
        CALL  printSpace
        CALL  bios_cmp_hl_de
        RET   Z
        DEC   B
        INC   HL
        JP    NZ,LBL40
        CALL  WaitClsKey
        CALL  printNewLine
        JP    Dir_L

;----------------------------------------------------------------------------
; ������ ������ � ������ � �������� �� ����������:
; A = ����� ���������
; HL = ������ HEX ��������
; DE = ������ HEX ��������
; BC = ������ HEX ��������

Tokenizer:
        ; ������ ����� ���������
        LD    BC,INPUT_BUF
        LD    A,(BC)

        ; ��������� ����� ���������
        LD    (vars1.Directive),A
        INC   BC

SUB15:  ; ������ � ��������� ��� HEX ���������
        CALL  ReadHexWord
        LD    (vars1.FirstParam),HL
        CALL  ReadHexWord
        LD    (vars1.SecondParam),HL
        CALL  ReadHexWord
        LD    (vars1.ThirdParam),HL

        LD    L,C
        LD    H,B
        LD    (vars1.V_8F87),HL

        ; �������� ��� HEX ��������� � �������� HL, DE, BC
        LD    HL,(vars1.ThirdParam)
        LD    C,L
        LD    B,H
        LD    HL,(vars1.SecondParam)
        EX    DE,HL
        LD    HL,(vars1.FirstParam)

        ; � ����� ��������� - � ������� A
        LD    A,(vars1.Directive)

        ; �������
        RET

;----------------------------------------------------------------------------
; �������������� ������ �� ������ BC � HEX ����� � HL

ReadHexWord:
        LD    HL,0000H
LBL44:  LD    A,(BC)
        CP    0DH           ; 13
        RET   Z
        CP    2CH           ; 44 ','
        JP    Z,LBL45
        CALL  SUB17
        ADD   HL,HL
        ADD   HL,HL
        ADD   HL,HL
        ADD   HL,HL
        ADD   A,L
        LD    L,A
        INC   BC
        JP    LBL44
LBL45:  INC   BC
        RET

;----------------------------------------------------------------------------

SUB17:  SUB   '0'
        CP    0Ah
        RET   C
        SUB   11h
        CP    06h
        JP    NC,TypeErrorRestart
        ADD   A,10
        RET

;----------------------------------------------------------------------------

SUB18:  PUSH  BC
        LD    C,19h
        CALL  mon2_printChar
        POP   BC
        RET

;----------------------------------------------------------------------------

SUB19:  PUSH  HL
        LD    HL,INPUT_BUF
        JP    LBL53

;----------------------------------------------------------------------------
; ������ �������� ������ (0Ah, 0Dh)

printNewLine:
        LD    C,0AH
        CALL  mon2_printChar
        LD    C,0DH
        CALL  mon2_printChar
        RET

;----------------------------------------------------------------------------
; ������� ����� � ��������� ���-7

txtSpaceQuestMark:  DB  "  ?",00H
txtNewLine:         DB  0AH,0DH,00H
txtHomeRight11Dot:  DB  0DH,18H,18H,18H,18H,18H,18H,18H,18H,18H,18H,18H,2EH,00H
txtFileQuestMark:   DB  "FILE? ",00H
                    DB  0AH,"FILE: ",00H

;----------------------------------------------------------------------------
; ��������� M - �������������� ����� ������

Dir_M:  CALL  SUB18
LBL47:  CALL  printNewLine
        CALL  printHexWordHL
        LD    A,(HL)
        CALL  mon2_printHexByte
        CALL  printSpace
        CALL  mon2_getch
        CALL  RestartIfCls
        CP    1AH           ; ������ ����
        JP    Z,LBL48
        CP    08H           ; ������ �����
        JP    Z,LBL49
        CALL  SUB21
        LD    (HL),A
LBL48:  INC   HL
        JP    LBL47
LBL49:  DEC   HL
        JP    LBL47
SUB21:  PUSH  BC
        JP    LBL50
        PUSH  BC
        CALL  mon2_getch
LBL50:  CALL  SUB22
        RLCA
        RLCA
        RLCA
        RLCA
        LD    B,A
        CALL  mon2_getch
        CALL  SUB22
        OR    B
        POP   BC
        RET

SUB22:  CP    20H           ; 32 ' '; [2]
        JP    C,RestartNoCls
        CP    3AH           ; 58 ':'
        JP    C,LBL51
        AND   5FH           ; 95 '_'
        CP    41H           ; 65 'A'
        JP    C,TypeErrorRestart
        CP    47H           ; 71 'G'
        JP    NC,TypeErrorRestart
        PUSH  AF
        LD    C,A
        CALL  mon2_printChar
        POP   AF
        SUB   37H           ; 55 '7'
        RET
LBL51:  OR    10H           ; 16
        PUSH  AF
        LD    C,A
        CALL  mon2_printChar
        POP   AF
        SUB   30H           ; 48 '0'
        RET

;----------------------------------------------------------------------------
; ������ ����� ������� � ����������

TypeErrorRestart:
        LD    HL,txtSpaceQuestMark
        CALL  bios_printStringOld
        JP    RestartNoCls

;----------------------------------------------------------------------------

LBL53:  PUSH  BC
        PUSH  DE
        LD    DE,vars1.Directive
        CALL  SUB28
        POP   DE
        POP   BC
        POP   HL
        RET

;----------------------------------------------------------------------------
; ������ �������

printSpace:
        LD    C,20H
        CALL  mon2_printChar
        RET

;----------------------------------------------------------------------------
; �������� ������� ������� ���

WaitClsKey:
        CALL  mon2_keyScan
        CP    1FH
        RET   NZ
        CALL  mon2_getch
        ; ����������� � RestartIfCls

;----------------------------------------------------------------------------
; ���������� ��������, ���� A == 1Fh

RestartIfCls:
        CP    1FH
        RET   NZ
        JP    RestartNoCls

;----------------------------------------------------------------------------
; ��������� D - ���� ����� ������ � HEX ����

Dir_D:  CALL  mon2_printHexWord
        LD    A,L
        AND   0FH
        LD    C,A
        CPL
        AND   0FH
        INC   A
        LD    B,A
        LD    A,C
        ADD   A,A
        ADD   A,A
        ADD   A,A
        ADD   A,C
        ADD   A,0FH
        LD    (bios_vars.cursorX),A
LBL55:  LD    A,(HL)
        CALL  mon2_printHexByte
        LD    A,B
        CP    09H
        JP    NZ,LBL56
        LD    C,2DH         ; ������ ������� '-'
        CALL  mon2_printChar
        JP    LBL57
LBL56:  CALL  printSpace
LBL57:  CALL  bios_cmp_hl_de
        RET   Z
        DEC   B
        INC   HL
        JP    NZ,LBL55
        CALL  WaitClsKey
        CALL  printNewLine
        JP    Dir_D
        LD    E,A
        RRCA
        RRCA
        RRCA
        RRCA
        CALL  SUB26
        LD    D,A
        LD    A,E
        CALL  SUB26
        LD    E,A
        RET
SUB26:  AND   0FH           ; 15; [2]
        CP    0AH           ; 10
        JP    C,LBL58
        ADD   A,07H         ; 7
LBL58:  ADD   A,30H         ; 48 '0'
        RET

;----------------------------------------------------------------------------

txtRegisters:
        DB    "A=",0
        DB    "F=",0
        DB    "B=",0
        DB    "C=",0
        DB    "D=",0
        DB    "E=",0
        DB    "H=",0
        DB    "L=",0
        DB    0AH,0DH," M(HL)=",0
        DB    "PC=",0
        DB    "SP=",0

;----------------------------------------------------------------------------

SUB27:  PUSH  AF            ; [2]
LBL59:  LD    A,(vars2.V_F0E6)
        AND   0CH           ; 12
        JP    NZ,LBL59
        LD    A,C
        LD    (vars2.V_F0E5),A
        LD    A,(vars2.V_F0E6)
        OR    20H           ; 32 ' '
        LD    (vars2.V_F0E6),A
LBL60:  LD    A,(vars2.V_F0E6)
        AND   04H           ; 4
        JP    Z,LBL60
        XOR   A
        LD    (vars2.V_F0E6),A
        POP   AF
        RET
        PUSH  HL
LBL61:  LD    A,(HL)
        OR    A
        JP    Z,LBL62
        LD    C,A
        CALL  SUB27
        INC   HL
        JP    LBL61
LBL62:  POP   HL
        RET
        PUSH  AF
        LD    A,91H         ; 145
        LD    (vars2.V_F0E7),A
        LD    A,(vars2.V_F0E6)
        OR    10H           ; 16
        LD    (vars2.V_F0E6),A
        AND   0EFH          ; 239
        LD    (vars2.V_F0E6),A
        LD    C,0FH         ; 15
        CALL  SUB27
        POP   AF
        RET
LBL63:  LD    (bios_vars.cursorCfg),A
        CALL  bios_drawCursorOld
        POP   AF
        LD    (bios_vars.cursorCfg),A
        LD    C,18H         ; 24
        POP   AF
        JP    mon2_printChar
;----------------------------------------------------------------------------

SUB28:  LD    B,H
        LD    C,L
LBL64:  CALL  mon2_getch
        CP    08H           ; ������� �����
        JP    Z,LBL65
        CP    0DH           ; ������� Enter
        JP    Z,LBL67
        LD    (vars1.V_8F95),A
        CALL  bios_cmp_hl_de
        JP    Z,LBL64
        LD    A,(vars1.V_8F95)
        LD    (HL),A
        PUSH  BC
        LD    C,A
        CALL  mon2_printChar
        POP   BC
        INC   HL
        JP    LBL64

;----------------------------------------------------------------------------

LBL65:  LD    A,H
        CP    B
        JP    NZ,LBL66
        LD    A,L
        CP    C
        JP    Z,LBL64
LBL66:  DEC   HL
        PUSH  BC
        LD    C,08H         ; 8
        CALL  mon2_printChar
        LD    C,20H         ; 32 ' '
        CALL  mon2_printChar
        LD    C,08H         ; 8
        CALL  mon2_printChar
        POP   BC
        JP    LBL64
LBL67:
        LD    (HL),A
        RET
        PUSH  HL
        LD    HL,0FFE3H   ; 65507
        LD    (HL),0DH    ; 13
        LD    (HL),0CH    ; 12
        POP   HL
        RET

;----------------------------------------------------------------------------
; ������� ��������� F800h
;----------------------------------------------------------------------------
    ORG_PAD0 0F800h


mon2_restart:       JP    RestartNoCls          ; F800
mon2_getch:         JP    bios_getchOld         ; F803
                    JP    bios_tapeReadOld      ; F806
mon2_printChar:     JP    bios_printCharOld     ; F809
                    JP    bios_tapeWriteOld     ; F80C
                    JP    bios_printer          ; F80F
                    JP    setMIfKeyPressed      ; F812
mon2_printHexByte:  JP    bios_printHexByte     ; F815
mon2_printString:   JP    bios_printStringOld   ; F818
mon2_keyScan:       JP    bios_keyScanOld       ; F81B
                    JP    bios_getCursorPos     ; F81E
                    RET                         ; F821 - �� ������������
                    NOP
                    NOP
mon2_tapeLoad:      JP    bios_tapeLoad         ; F824
mon2_tapeSave:      JP    bios_tapeSave         ; F827
mon2_calcCS:        JP    bios_calcCS           ; F82A
                    JP    bios_beep_Old         ; F82D
                    JP    bios_getMemTop        ; F830
                    JP    bios_setMemTop        ; F833
mon2_printHexWord:  PUSH  BC                    ; F836 - ������ ����� �� HL � HEX �������, ����������� ��������
                    JP    printHexWordHL_noSpace

;----------------------------------------------------------------------------

    END
