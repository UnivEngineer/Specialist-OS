; HL=буфер FILE_INFO, DE=ёмкость записей, BC=начальный номер.
; DE=число файлов, BC/HL сохранены, FFh завершает буфер, CF=ошибка.
fatFileList:
            push hl
            push bc
            ld h,b
            ld l,c
            ld (v_dirFirstFile),hl
            ex de,hl
            ld (v_dirMaxFiles),hl
            ld hl,0
            ld (v_dirTotalFiles),hl
            ld (v_dirListedFiles),hl
            pop bc
            pop hl
            push hl
            push bc
            ex de,hl
            call fatDirStart
            jp c,fileList_end
fileList_loop:
            call fatDirFile
            jp c,fileList_end
            push hl
            ld hl,(v_dirTotalFiles)
            inc hl
            ld (v_dirTotalFiles),hl
            push de
            ex de,hl
            ld hl,(v_dirFirstFile)
            call cmp_hl_de
            pop de
            jp nc,fileList_skip
            push de
            ld hl,(v_dirMaxFiles)
            ex de,hl
            ld hl,(v_dirListedFiles)
            call cmp_hl_de
            pop de
            jp nc,fileList_skip
            inc hl
            ld (v_dirListedFiles),hl
            pop hl
            ex de,hl
            push bc
            call copyDescriptorCompact
            pop bc
            ex de,hl
            jp fileList_loop
fileList_skip:
            pop hl
            jp fileList_loop
fileList_end:
            push af
            ld a,0FFh
            ld (de),a
            push hl
            ld hl,(v_dirTotalFiles)
            ex de,hl
            pop hl
            pop af
            pop bc
            pop hl
            or a
            ret z
            scf
            ret
