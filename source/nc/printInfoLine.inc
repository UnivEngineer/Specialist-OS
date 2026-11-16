;----------------------------------------------------------------------------
; MXOS NC.COM
; Вывод инфомарции о файле внизу окна
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printInfoLine:
        ; Если файлов нет, выходим
        ld      hl, state.panelA_filesCnt
        call    readBytePanel
        and     a
        jp z,printInfoEmpty

        COLOR   0F1h

        ld      hl, (P_FILE_NAME_X << 8) + P_FILE_NAME_Y
        call    setCursorPosPanel

        ; Устанавливаем hl на описатель файла
        call    getSelectedFile
        push hl
        ld de,FILE_INFO.attrib
        add hl,de
        ld a,(hl)
        and 10h
        pop hl
        jp z,ncStatusName
        ld a,(hl)
        cp 05Ch
        jp z,ncStatusRoot
        cp '.'
        jp nz,ncStatusName
        inc hl
        ld a,(hl)
        dec hl
        cp '.'
        jp nz,ncStatusName
        inc hl
        inc hl
        ld a,(hl)
        dec hl
        dec hl
        cp ' '
        jp z,ncStatusParent
ncStatusName:
        push hl
        ld de,vars.pathArg
        call compactName
        ld hl,vars.pathArg
ncStatusPrint:
        ld a,(ncStatusChars)
        ld b,a
        call    ncPrintField

        ; Смещаем курсор
        ld a,(ncStatusSizeX)
        ld h,a
        ld l,P_FILE_DATA_Y
        call    setCursorPosPanel

        ; Адрес не является метаданными FAT. Печатать только точный размер.
        ; Устанавливаем hl на поле размера файла
        pop     hl
        push    hl
        ld      bc, FILE_INFO.attrib
        add     hl, bc
        ld      a, (hl)
        and     010h
        pop     hl
        jp nz,  printInfoDirectory
        ld      bc, FILE_INFO.size ; bc = смещение
        add     hl, bc

        ; Читаем размер файла в de и печатаем в десятичной форме
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      b, 20h
        jp      ncPrintFileSize

printInfoDirectory:
        ld hl,infoDirectoryText
        jp bios_printStringReal
infoDirectoryText: DB "   <DIR>",0

; Navigation entries describe the action; no size or <DIR> suffix.
ncStatusRoot:
        ld hl,infoRootText
        jp ncStatusNavigation
ncStatusParent:
        ld hl,infoParentText
ncStatusNavigation:
        push hl
        call printInfoEmpty
        ld hl,(P_FILE_NAME_X << 8) + P_FILE_NAME_Y
        call setCursorPosPanel
        pop hl
        jp bios_printStringReal
infoRootText: DB 'Go to drive root',0
infoParentText: DB 'Go to parent dir',0

printInfoEmpty:
        COLOR 0F1h
        ld hl,(P_FILE_NAME_X << 8) + P_FILE_NAME_Y
        call setCursorPosPanel
        ld a,(ncHeaderChars)
        ld b,a
        ld a,(ncFontWidth)
        cp 6
        jp nz,ncEmptyStatusWidth
        ld b,30
ncEmptyStatusWidth:
        jp printSpaces
