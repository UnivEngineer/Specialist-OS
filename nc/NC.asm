;----------------------------------------------------------------------------
; MXOS - �������� �������� NC.COM
;
; 2013-12-18 ����������������� � ���������� vinxru
; 2022-01-31 ���������� SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

    IF NEW_MEMORY_MAP
        ORG 0E800h
    ELSE
        ORG 0D000h
    ENDIF

; ������ ������ �� ��� ������ �������
FULL_PANELS = 0

; ������������� ������:
; 0 = vinxru
; 1 = �����
; 2 = Norton Commander
COLOR_PRESET = 1

COLOR_CMDLINE    =  070h        ; ���� ��������� ������
COLOR_CMDSCREEN  =  COLOR_BIOS  ; ���� ���������� ������ (����� ������ ��������)

    IF COLOR_PRESET == 0

COLOR_BORDER     =  0F1h    ; ���� �����
COLOR_PANELNAME  =  0A1h    ; ���� ��������� ������ (NAME)
COLOR_FILE       =  0B1h    ; ���� ������
COLOR_INFOLINE   =  0B1h    ; ���� ������ ���������� � ������� �����
COLOR_CURSOR     =  0B0h    ; ���� ����������� ����� (���������)
COLOR_DIALOG     =  007h    ; ���� ���� �������� F1-F9
COLOR_DIALOG_ERR =  047h    ; ���� ������ �� ���� � ���������� �� ������
COLOR_INFOHEADER =  0A1h    ; ��������� �������������� ������
COLOR_INFONUMBER =  0E1h    ; ����� �� �������������� ������
COLOR_INFOTEXT   =  0F1h    ; ����� �� �������������� ������
COLOR_HELP_F     =  040h    ; ���� �������������� ������ � ������ ���������
COLOR_HELP_TEXT  =  071h    ; ���� ������ � ������ ���������

    ELSEIF COLOR_PRESET == 1

COLOR_BORDER     =  0F1h    ; ���� �����
COLOR_PANELNAME  =  0E1h    ; ���� ��������� ������ (NAME)
COLOR_FILE       =  0B1h    ; ���� ������
COLOR_INFOLINE   =  0B1h    ; ���� ������ ���������� � ������� �����
COLOR_CURSOR     =  030h    ; ���� ����������� ����� (���������)
COLOR_DIALOG     =  007h    ; ���� ���� �������� F1-F9
COLOR_DIALOG_ERR =  047h    ; ���� ������ �� ���� � ���������� �� ������
COLOR_INFOHEADER =  0E1h    ; ��������� �������������� ������
COLOR_INFONUMBER =  0E1h    ; ����� �� �������������� ������
COLOR_INFOTEXT   =  0F1h    ; ����� �� �������������� ������
COLOR_HELP_F     =  060h    ; ���� �������������� ������ � ������ ���������
COLOR_HELP_TEXT  =  030h    ; ���� ������ � ������ ���������

    ELSE

COLOR_BORDER     =  0B1h    ; ���� �����
COLOR_PANELNAME  =  0E1h    ; ���� ��������� ������ (NAME)
COLOR_FILE       =  0B1h    ; ���� ������
COLOR_INFOLINE   =  0B1h    ; ���� ������ ���������� � ������� �����
COLOR_CURSOR     =  030h    ; ���� ����������� ����� (���������)
COLOR_DIALOG     =  007h    ; ���� ���� �������� F1-F9
COLOR_DIALOG_ERR =  047h    ; ���� ������ �� ���� � ���������� �� ������
COLOR_INFOHEADER =  0B1h    ; ��������� �������������� ������
COLOR_INFONUMBER =  0E1h    ; ����� �� �������������� ������
COLOR_INFOTEXT   =  0B1h    ; ����� �� �������������� ������
COLOR_HELP_F     =  070h    ; ���� �������������� ������ � ������ ���������
COLOR_HELP_TEXT  =  030h    ; ���� ������ � ������ ���������

    ENDIF

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
    INCLUDE "butF4.inc"
    INCLUDE "butEnter.inc"
    INCLUDE "saveLoadState.inc"
    INCLUDE "drawWindow.inc"
    INCLUDE "printStringInv.inc"
    INCLUDE "inputForCopyMove.inc"
    INCLUDE "printSelDrive.inc"
    INCLUDE "butF7.inc"
    INCLUDE "tapeErrorHandler.inc"
    INCLUDE "butF9.inc"
    INCLUDE "tapeWrite.inc"
    INCLUDE "butF6.inc"
    INCLUDE "butF5.inc"
    INCLUDE "loadSelFileAt0.inc"
    INCLUDE "copyFileInt.inc"
    INCLUDE "printInvSelFile.inc"
    INCLUDE "butF8.inc"
    INCLUDE "butF2.inc"
    INCLUDE "butTab.inc"
    INCLUDE "butF3.inc"
    INCLUDE "butArrows.inc"
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
    INCLUDE "tools.inc"
    INCLUDE "compactName.inc"
    INCLUDE "input.inc"

;---------------------------------------------------------------------------
; ��������� � ����������
;---------------------------------------------------------------------------

aF1LeftF2RighF3:    DB "F1 Left F2 Rght F3 Info F4 Edit F5 Copy F6 RMov F7 Load F8 Del",0
aCommanderVer:      DB "Commander version 2.0",0
aCopyright:         DB "(C) Omsk 1992, SPb 2022",0
aFileIsReanOnly:    DB "File is read-only!",0
aABCD:              DB "A   B   C   D",0
aEFGH:              DB "E   F   G   H",0
aChooseDrive:       DB "Choose drive:",0
aDeleteFrom:        DB "Delete from ",0
asc_DC17:           DB 8, ' ',8, 0
aCopyFromTo:        DB "Copy from    to",8,8,8,8,8, 0
aCantCreateFile:    DB "Can",39,"t create file!",0
aRemoveFromTo:      DB "Rename/move from    to",8,8,8,8,8, 0
aKBytesExtMemory:   DB 18h,"KB extended memory",0   ; ����� � ����� 18h ������ �������� �������,
aKBytesMemory:      DB 18h,"bytes Memory",0         ; ����� �� ������� ���� ����������� �������
aKBytesFree:        DB 18h,"bytes Free",0
aKBytesTotalOnDrv:  DB 18h,"KB total on drive ",0
aKBytesFreeOnDrv:   DB 18h,"KB free  on drive ",0
aFilesUse:          DB 18h,"files use ",0
aKBytesIn:          DB 18h,"KB in ",0
aVolumeLabel:       DB "Volume label: ",0
aDrive:             DB "Drive ",0
aHasNoDriver:       DB 18h,"has no driver",0
aNotFormatted:      DB 18h,"is not formatted",0
aSaveFromToTape:    DB "Save from    to tape",8,8,8,8,8,8,8,8,8,8, 0
aSavingToTape:      DB "Saving to tape",0
aLoadingFromTapeTo: DB "Loading from tape to ",0
aErrorLoadingTa:    DB "Error loading from tape",0

    IF FULL_PANELS==0
aNameName:          DB "Name",0
    ENDIF

aNcExt:             DB "A:NC.EXT",0
aEditor:            DB "A:E.COM",0Dh      ; ������������ ��� ������ ���� 0Dh

;-----------------------------------------------------------------------
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
            G_LINE   0, 4, 136, 184           ; 1, 70h, 90h, 16h, 0Fh, 0F0h
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

;-----------------------------------------------------------------------
; ������ ������ ��� �������� ���������� (���� ������������ ������)

    IF FULL_PANELS
FILE_LIST_SIZE    = 40
    ELSE
FILE_LIST_SIZE    = 36
    ENDIF

;-----------------------------------------------------------------------
; ��������� ��������� ������� �� ������ �������� ���� ������
; x - ��������� �������, y - �������

P_NAME_X1         = 17  ; ��������� ������ "NAME" (2 �������)
P_NAME_X2         = 65
P_NAME_Y          = 16

P_FILE_LIST_X1    = 6   ; �������� ������� (2 �������)
P_FILE_LIST_X2    = 54

    IF FULL_PANELS
P_FILE_LIST_Y     = 12
    ELSE
P_FILE_LIST_Y     = 32
    ENDIF

P_FILE_LIST_H     = FILE_LIST_SIZE / 2  ; ������ �������� ������� � �������
P_FILE_LIST_Y_MAX = P_FILE_LIST_H * 10 + P_FILE_LIST_Y

P_DRIVE_LETTER_X  = 9   ; ����� ����� ����� ����� ������
P_DRIVE_LETTER_Y  = 222
P_FILE_NAME_X     = 15  ; ��� ����� ����� ������
P_FILE_NAME_Y     = 222
P_FILE_DATA_X     = 57  ; ������ � ����� (����� ��������, ������)
P_FILE_DATA_Y     = 222

P_INPUT_WIDTH     = 23  ; ����� ���� ����� ����� ����� � ��������

;-----------------------------------------------------------------------
; ����������
;-----------------------------------------------------------------------

; ���� ���������� ���������
; ����������� � �������������� ������� ������� �������s RAM-�����

    STRUCT NC_STATE
initState           DB    5Ah   ; �������, ��� NC.COM ��� ����������
activePanel         DB    0     ; ����� �������� ������
panelA_info         DB    0     ; 1 = ������ A � ������ ����������
panelB_info         DB    0     ; 1 = ������ B � ������ ����������
panelA_drive        DB    0     ; ����� ����� ������ A
panelB_drive        DB    7     ; ����� ����� ������ B
panelA_filesCnt     DB    0     ; ���������� ������ �� ������ A
panelB_filesCnt     DB    0     ; ���������� ������ �� ������ B
panelA_firstFile    DB    0     ; c ������ ����� ���������� ������ A
panelB_firstFile    DB    0     ; c ������ ����� ���������� ������ B
panelA_curFile      DB    0     ; �� ����� ����� ������ �� ������ A
panelB_curFile      DB    0     ; �� ����� ����� ������ �� ������ B
    ENDS

state   NC_STATE
stateEnd:           ; ����� ����� ����� ���������� ���������

; ������ ���������� - �� �������� � �������� (�������� ��� ��
; ������������, ��� ������ ������� ���������� ����� ��������� �����)

    STRUCT NC_VARIABLES
cmdLinePos      DW    0
cmdLineEnd      DW    0
chooseDrive     DB    0
tapeSaveCRC     DW    0         ; ����������� ����� ����� � �����
savedSP         DW    0
tapeLoadAddr    DW    0         ; ����� �������� ����� � �����
diskInfoPtr     DW    0         ; ����� ��������� DISK_INFO
cmdLine         BLOCK 59, 0FFh  ; ��������� ������
cmdLineCtrl     DB    0FFh      ; �������� ������������ ��������� ������
                BLOCK 13, 0FFh
input           BLOCK 21, 0FFh
                BLOCK 11
tempFileDescr   FILE_DESCRIPTOR
    ENDS

vars    NC_VARIABLES = $        ; ����� ������ ����� �����������
varsEnd = vars + NC_VARIABLES   ; ����� �����  ����� �����������

; ����� ������ ��� �������� ���������� (FILE_LIST_SIZE �������� FILE_INFO + 1 = 577 ����)
;FILE_LIST_BUFFER = 8800h
FILE_LIST_BUFFER      = varsEnd
FILE_LIST_BUFFER_SIZE = FILE_LIST_SIZE * FILE_INFO_SIZE + 1

;-----------------------------------------------------------------------
; ���������, ��� ��������� ���������� � ������ ������ �� ������ ��������
;-----------------------------------------------------------------------

; ����� NC.COM ����� �������������� � ������
NC_END = FILE_LIST_BUFFER + FILE_LIST_BUFFER_SIZE

; ������������ ���������� ������ NC.COM
    IF NEW_MEMORY_MAP
NC_END_MAX = FAT_CACHE_ADDR  ; �� ��������� ����
    ELSE
NC_END_MAX = 0E200h          ; �� ������ 0E200h
    ENDIF

; ��������
    IF NC_END >= NC_END_MAX
        ASSERT 0
        DISPLAY /l, "Error! NC.COM did not fit (", NC_END, " > ", NC_END_MAX, ")"
    ENDIF

    END
