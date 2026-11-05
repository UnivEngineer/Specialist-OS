;----------------------------------------------------------------------------
; MXOS NC.COM
; Сформировать строку пути выбраного файла
;
; вход:
;   hl - адрес дескриптора файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

getSelFilePath:
        call buildSelectedFilePath
        ret c
        jp appendPathToCommand
buildSelectedFilePath:
        push hl
        ld hl,vars.path
        ld a,DIR_OP_GETCWD
        call bios_directory
        pop hl
        ret c
buildCachedSelectedPath:
        push hl
        ld hl,vars.path
        ld de,vars.pathArg
getPathCopy:
        ld a,(hl)
        ld (de),a
        inc hl
        inc de
        or a
        jp nz,getPathCopy
        dec de
        dec hl
        dec hl
        ld a,(hl)
        cp 05Ch
        jp z,getPathName
        ld a,05Ch
        ld (de),a
        inc de
getPathName:
        pop hl
        ld a,(hl)
        cp 05Ch
        jp nz,getPathNormalName
        ld de,vars.pathArg+3
        jp getPathLeafEnd
getPathNormalName:
        push de
        ld de,vars.tempFileDescr
        call compactName
        pop de
        ld hl,vars.tempFileDescr
getPathLeaf:
        ld a,(hl)
        cp 021h
        jp c,getPathLeafEnd
        push hl
        ld hl,vars.pathArg+MAX_PATH-1
        call bios_cmp_hl_de
        pop hl
        jp z,getPathLeafOverflow
        ld a,(hl)
        ld (de),a
        inc hl
        inc de
        jp getPathLeaf
getPathLeafEnd:
        xor a
        ld (de),a
        ld hl,vars.pathArg
        ret
appendPathToCommand:
        ld hl,(vars.cmdLineEnd)
        ex de,hl
        ld hl,vars.pathArg
        ; Проверяем всю добавку до изменения общей строки.
        push de
        ld b,0
getPathCount:
        ld a,(hl)
        or a
        jp z,getPathCapacity
        inc hl
        inc b
        jp getPathCount
getPathCapacity:
        ld a,e
        add a,b
        jp c,getPathReject
        cp (vars.cmdLineCtrl+1) & 0FFh
        jp nc,getPathReject
        pop de
        ld hl,vars.pathArg
getPathAppend:
        ld a,e
        cp vars.cmdLineCtrl & 0FFh
        jp z,getPathAppendEnd
        ld a,(hl)
        or a
        jp z,getPathAppendEnd
        ld (de),a
        inc de
        inc hl
        jp getPathAppend
getPathAppendEnd:
        xor a
        ld (de),a
        ret
getPathReject:
        pop de
        scf
        ret
getPathLeafOverflow:
        scf
        ret

;----------------------------------------------------------------------------
; MXOS NC.COM
; Поместить выбранный файл в ком строку
;
; вход:
;   hl - адрес дескриптора файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

selFileToCmdLine:
        call    getSelFilePath

        ret c
        ex de,hl
        ld (vars.cmdLineEnd),hl
        call redrawCommandLine
        or a
        ret
