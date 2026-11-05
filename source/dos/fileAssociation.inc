; Поиск ассоциации в системном A:CLI.EXT, независимо от NC/CLI.
; HL = три байта расширения, DE = временный буфер, BC = его ёмкость.
; Буфер выровнен на 256 байт, ёмкость кратна 256, диапазон внутри ОЗУ;
; он не пересекается с кодом вызывающего, расширением, стеком и DOS.
; CF=0: HL = команда до CR/FF внутри буфера; CF=1: нет записи/ошибка.
; Исходный диск восстанавливается; AF/BC/DE/HL и флаги разрушаются.
; Результат временный: скопировать команду в CMD_LINE до загрузки COM.
fileAssociation:
            ld      a, (v_drive)
            push    af
            push    hl
            push    de
            push    bc
            ld      hl, pathCliExt
            ld      de, v_tmpFileDescr
            call    fileNamePrepare
            ld      hl, v_tmpFileDescr
            call    fileLoadInfo
            jp c,   fa_ParamError
            ld      hl, (v_tmpFileDescr + FILE_DESCRIPTOR.size + 2)
            ld      a, h
            or      l
            jp nz,  fa_ParamError
            ld      hl, (v_tmpFileDescr + FILE_DESCRIPTOR.size)
            pop     de              ; ёмкость
            call    cmp_hl_de
            jp nc,  fa_BufferError  ; требуется место для FF после точного N
            pop     bc              ; буфер
            push    hl              ; точный размер
            add     hl, bc
            ex      de, hl
            pop     hl
            push    de              ; конец файла
            push    bc              ; начало буфера
            ld b,h
            ld c,l
            ld hl,0
            ld (v_streamOffset),hl
            ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstCluster)
            ex de,hl
            pop hl
            push hl
            ld a,2
            call fatStream
            pop de
            pop hl
            jp c,fa_LoadError
fa_Terminate:
            ld      (hl), 0FFh
            pop     hl              ; расширение
fa_Search:
            ; CLI.EXT допускает как прежний CR, так и текстовые строки CR/LF.
            ld a,(de)
            cp 00Ah
            jp nz,fa_SearchLine
            inc de
fa_SearchLine:
            push    hl
            ld      c, 3
            call    stricmp
            jp nz,  fa_NextLine
            pop     hl
            ld      a, (de)
            cp      ' '
            jp nz,  fa_Skip
            inc     de
            ld      a, (de)
            inc     a
            jp z,   fa_NotFound
            dec     a
            cp      20h
            jp c,   fa_Skip
            ex      de, hl
            or      a
            jp      fa_Return
fa_NextLine:
            pop     hl
fa_Skip:
            ld      a, (de)
            inc     de
            cp      0Dh
            jp z,   fa_Search
            inc     a
            jp nz,  fa_Skip
fa_NotFound:
            scf
            jp      fa_Return
fa_ParamError:
            pop     bc
fa_BufferError:
            pop     de
fa_LoadError:
            pop     hl
            scf
fa_Return:
            ; Флаги результата переживают восстановление исходного тома.
            push    af
            pop     de
            pop     af
            push    de
            call    fileSelectDrive
            pop     af
            ret
pathCliExt:  DB "A:",05Ch,"CLI.EXT",0
