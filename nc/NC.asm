;----------------------------------------------------------------------------
; MXOS - �������� �������� NC.COM
;
; 2013-12-18 ����������������� � ���������� vinxru
; 2022-01-31 ���������� SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

      ORG 0D000h

FIX_FREE_SPACE_BUG  =  1 ; ��������� ������ ����������� ���������� ������
SHOW_F9             =  0 ; �������� ��� ������ F1...F9 �� ������ ������� (����� F1...F8)

; �����
COLOR_CMDLINE    =  070h        ; ���� ��������� ������
COLOR_CMDSCREEN  =  COLOR_BIOS  ; ���� ���������� ������ (����� ������ ��������)
COLOR_BORDER     =  0F1h        ; ���� �����
COLOR_PANELNAME  =  0A1h        ; ���� ��������� ������ (NAME)
COLOR_FILE       =  0B1h        ; ���� ������
COLOR_INFOLINE   =  0B1h        ; ���� ������ ���������� � ������� �����
COLOR_CURSOR     =  0B0h        ; ���� ������� (���������)
COLOR_DIALOG     =  007h        ; ���� ���� �������� F1-F9
COLOR_DIALOG_ERR =  047h        ; ���� ���� � ���������� �� ������
COLOR_INFOHEADER =  0A1h        ; ��������� �������������� ������
COLOR_INFONUMBER =  0E1h        ; ����� �� �������������� ������
COLOR_INFOTEXT   =  0F1h        ; ����� �� �������������� ������
COLOR_HELP_F     =  040h        ; ���� �������������� ������ � ������ ���������
COLOR_HELP_TEXT  =  071h        ; ���� ������ � ������ ���������

; ��������� ��������� �������

P_NAME_X          = 17  ; ��������� ������ "NAME"
P_NAME_Y          = 16

    IF FAT16
P_FILE_LIST_X1    = 6   ; �������� ������� (2 �������)
P_FILE_LIST_X2    = 54
P_FILE_LIST_Y     = 32
    ELSE
P_FILE_LIST_X1    = 10  ; �������� ������� (2 �������)
P_FILE_LIST_X2    = 57
P_FILE_LIST_Y     = 32
    ENDIF

P_FILE_LIST_H     = 18  ; ������ �������� ������� � �������
P_FILE_LIST_Y_MAX = P_FILE_LIST_H * 10 + P_FILE_LIST_Y

P_DRIVE_LETTER_X  = 9   ; ����� ����� ����� ����� ������
P_DRIVE_LETTER_Y  = 222
P_FILE_NAME_X     = 15  ; ��� ����� ����� ������
P_FILE_NAME_Y     = 222
P_FILE_DATA_X     = 57  ; ������ � ����� (����� ��������, ������)
P_FILE_DATA_Y     = 222

P_INPUT_WIDTH     = 23  ; ����� ���� ����� ����� ����� � ��������

;---------------------------------------------------------------------------
; �������
;---------------------------------------------------------------------------

; ����� ���� �������, ������ ��������� ����� ���
    MACRO COLOR x
        IF ENABLE_COLOR
            ld a, x
            ld (IO_COLOR), a
        ENDIF
    ENDM

; ������ ��� ���������� ��������� ������ ������ W, ���������������� �������������
    MACRO CENTER_LINE w, y
        ld  hl, ((60h - w * 3 / 2) << 8) + y
        ld  (bios_vars.cursorY), hl
    ENDM

;---------------------------------------------------------------------------
; ���
;---------------------------------------------------------------------------

    INCLUDE "start.inc"     ; ������������ � main
    INCLUDE "main.inc"
    INCLUDE "selFileToCmdLine.inc"
    INCLUDE "f4.inc"
    INCLUDE "enter.inc"
    INCLUDE "saveState.inc"
    INCLUDE "enter2.inc"
    INCLUDE "main2.inc"
    INCLUDE "drawWindow.inc"
    INCLUDE "printStringInv.inc"
    INCLUDE "inputForCopyMove.inc"
    INCLUDE "printSelDrive.inc"
    INCLUDE "f7.inc"
    INCLUDE "tapeErrorHandler.inc"
    INCLUDE "f9.inc"
    INCLUDE "tapeWrite.inc"
    INCLUDE "f6.inc"
    INCLUDE "f5.inc"
    INCLUDE "loadSelFileAt0.inc"
    INCLUDE "copyFileInt.inc"
    INCLUDE "printInvSelFile.inc"
    INCLUDE "f8.inc"
    INCLUDE "f2.inc"
    INCLUDE "tab.inc"
    INCLUDE "f3.inc"
    INCLUDE "upDownLeftRight.inc"
    INCLUDE "clearCmdLine.inc"    ; ������������ � printSpaces
    INCLUDE "printSpaces.inc"
    INCLUDE "drawCursor.inc"
    INCLUDE "printInfoLine.inc"
    INCLUDE "inverseRect.inc"
    INCLUDE "getSelectedFile.inc"
    INCLUDE "loadAndPrint.inc"
    INCLUDE "loadFiles.inc"
    INCLUDE "printInfoPanel.inc"
    INCLUDE "printFilePanel.inc"
    INCLUDE "printCurDrive.inc"
    INCLUDE "rwBytePanel.inc"
    INCLUDE "printFileName.inc"
    INCLUDE "printString2.inc"
    INCLUDE "setCursorPosPanel.inc"
    INCLUDE "draw.inc"
    INCLUDE "memset_hl_a_c.inc"
    INCLUDE "memcpy_hl_de_3.inc"
    INCLUDE "compactName.inc"
    INCLUDE "input.inc"

;---------------------------------------------------------------------------
; ��������� � ����������
;---------------------------------------------------------------------------

aNameName:          DB "NAME",18h,18h,18h,18h,18h,18h,18h,18h,18h,18h,18h,18h,"NAME",0
    IF SHOW_F9
aF1LeftF2RighF3:    DB "F1 L F2 R F3 INF F4 EDIT F5 COPY F6 RMOV F7 LOAD F8 DEL F9 SAVE",0
    ELSE
aF1LeftF2RighF3:    DB "F1 LEFT F2 RIGH F3 INFO F4 EDIT F5 COPY F6 RMOV F7 LOAD F8 DEL",0
    ENDIF
aCommanderVer:      DB "COMMANDER VERSION 1.6",0
aCOmsk1992:         DB "(C) OMSK 1992",0
aFileIsReanOnly:    DB "FILE IS READ ONLY!",0
aABCD:              DB "A   B   C   D",0
aEFGH:              DB "E   F   G   H",0
aChooseDrive:       DB "CHOOSE DRIVE:",0
aDeleteFrom:        DB "DELETE FROM ",0
asc_DC17:           DB 8, ' ',8, 0
aCopyFromTo:        DB "COPY FROM    TO",8,8,8,8,8, 0
aCantCreateFile:    DB "CAN",39,"T CREATE FILE!",0
aRemoveFromTo:      DB "RENAME/MOVE FROM    TO",8,8,8,8,8, 0
aBytesFreeOnDrv:    DB " BYTES FREE ON DRIVE ",0
aFilesUse:          DB " FILES USE ",0
aBytesIn:           DB "BYTES IN ",0
aTotalBytes:        DB " TOTAL BYTES",0
aOnDrive:           DB "ON DRIVE ",0
aSaveFromToTape:    DB "SAVE FROM    TO TAPE",8,8,8,8,8,8,8,8,8,8, 0
aSavingToTape:      DB "SAVING TO TAPE",0
aLoadingFromTapeTo: DB "LOADING FROM TAPE TO ",0
aErrorLoadingTa:    DB "ERROR LOADING FROM TAPE",0

; �������� ����

v_window:   DB 01111111b            ; ������� ����� ����
            DB 01000000b
            DB 01011111b
            DB 01010000b            ; ����� ����
            DB 01011111b            ; ������ ����� ����
            DB 01000000b
            DB 01111111b

            DB 11111111b            ; ���� ����
            DB 00000000b
            DB 11111111b

            DB 00000000b            ; ���������� ����

            DB 11111111b            ; ��� ����
            DB 00000000b
            DB 11111111b

            DB 11111110b            ; ������ ������� ����
            DB 00000010b
            DB 11111010b
            DB 00001010b            ; ������ �������
            DB 11111010b            ; ������ ������ ����
            DB 00000010b
            DB 11111110b

            MACRO G_WINDOW A,X,Y,W,H
                DB A|2, Y, 90h+(X>>3), H-6, (W>>3)-2
            ENDM

            MACRO G_LINE A,X,Y,W
                DB A|1, Y, 90h+(X>>3), (((X&7)+W+7)>>3)-2, 0FFh>>(X&7), (0FF00h>>((W+X)&7)) & 0FFh
            ENDM

            MACRO G_VLINE A,X,Y,H
                DB A|3, Y, 90h+(X>>3), H, 80h>>(X&7)
            ENDM

g_filePanel:
            G_WINDOW 0, 0, 0, 192, 230        ; 2, 0, 90h, 0E0h, 16h
            G_LINE   0, 4, 208, 184           ; 1, 0D0h, 90h, 16h, 0Fh, 0F0h
            G_VLINE  0, 96, 3, 205            ; 3, ?, 9Ch, 0CDh, 80h
            DB 0

g_infoPanel:
            G_WINDOW 0, 0, 0, 192, 230        ; 2, 0, 90h, 0E0h, 16h
            G_LINE   0, 4, 31, 184            ; 1, 1Fh, 90h, 16h, 0Fh, 0F0h
            G_LINE   0, 4, 112, 184           ; 1, 70h, 90h, 16h, 0Fh, 0F0h
            DB 0

g_chooseDrive:
            G_WINDOW 0, 40, 85, 112, 50       ; 2, 55h, 95h, 02Ch, 0Ch
            G_LINE   0, 44, 103, 104          ; 1, 67h, 95h, 0Ch, 0Fh, 0F0h
            DB 0

g_window1:
            G_WINDOW 80h, 112, 80, 160, 37    ; 82h, 50h, 9Eh, 1Fh, 12h
            G_LINE   80h, 116, 98, 152        ; 81h, 62h, 9Eh, 12h, 0Fh, 0F0h
            DB 0

g_window2:
            G_WINDOW 80h, 104, 114, 176, 37   ; 82h, 72h, 9Dh, 1Fh, 14h
            G_LINE   80h, 108, 132, 168       ; 81h, 84h, 9Dh, 14h, 0Fh, 0F0h
            DB 0

initState:        DB 5Ah
activePanel:      DB 0
panelA_info:      DB 0
panelB_info:      DB 1
panelA_drive:     DB 0
panelB_drive:     DB 0
panelA_filesCnt:  DB 0
panelB_filesCnt:  DB 0
panelA_curFile:   DB 0
panelB_curFile:   DB 0
aNcExt:           DB "A:NC.EXT",0
aEditor:          DB "A:E.COM",0Dh      ; ������������ ��� ������ ���� 0Dh

;-----------------------------------------------------------------------
; ����������
;-----------------------------------------------------------------------

    STRUCT NC_VARIABLES
cmdLinePos      DW    0
cmdLineEnd      DW    0
chooseDrive     DB    0
tapeSaveCRC     DW    0         ; ����������� ����� ����� � �����
savedSP         DW    0
tapeLoadAddr    DW    0         ; ����� �������� ����� � �����
cmdLine         BLOCK 59, 0FFh  ; ��������� ������
cmdLineCtrl     DB    0FFh      ; �������� ������������ ��������� ������
                BLOCK 13, 0FFh
input           BLOCK 21, 0FFh
                BLOCK 11
tempFileDescr   FILE_DESCRIPTOR
fileListBuffer  BLOCK 768       ; ����� ��� �������� ���������� (768 ���� = 48 �� FILE_DESCRIPTOR) - �� ������� � ��������
    ENDS

; ��������� ������ �������� ����� vars (�������� ��� �� ������������, ���������� ����� ��������� �����)
nc_vars         NC_VARIABLES = $

; ������������ ���������� ������ NC.COM - �� ������ 0E1FFh

    END
