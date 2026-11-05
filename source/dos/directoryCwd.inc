; GETCWD: HL=буфер MAX_PATH в памяти вызывающего. AF/BC/DE/HL разрушены.
; Имена восстанавливаются по .. и записи дочернего каталога в родителе.
; Буфер строится задом наперёд, затем разворачивается; CWD не меняется.
directoryGetCwd:
            ld bc,MAX_PATH
            call streamBufferRange
            ret c
directoryCwdUnchecked:
            ld (cwdBuffer),hl
            ld (cwdWrite),hl
            xor a
            ld (cwdLength),a
cwdParent:
            ld hl,(v_directory)
            ld a,h
            or l
            jp z,cwdRoot
            ld (cwdChild),hl
            ld hl,cwdDots
            call fileFindAny
            jp c,cwdExit
            ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstCluster)
            ld (v_directory),hl
            call fatDirStart
            jp c,cwdExit
cwdFind:
            call fatDirItem
            jp c,cwdCorrupt
            ld a,(hl)
            cp '.'
            jp z,cwdFind
            push hl
            ld de,FILE_DESCRIPTOR.attrib
            add hl,de
            ld a,(hl)
            and 010h
            pop hl
            jp z,cwdFind
            push hl
            ld de,FILE_DESCRIPTOR.firstCluster
            add hl,de
            ld e,(hl)
            inc hl
            ld d,(hl)
            ld hl,(cwdChild)
            call cmp_hl_de
            pop hl
            jp nz,cwdFind
            ; Расширение и имя в обратном порядке, пробелы пропускаются.
            push hl
            ld de,10
            add hl,de
            ld b,3
            call cwdPart
            pop hl
            jp c,cwdExit
            ld a,(cwdLength)
            ld c,a
            ; Точка нужна только при непустом расширении этой записи.
            push hl
            ld de,8
            add hl,de
            ld a,(hl)
            cp ' '
            pop hl
            ld a,'.'
            call nz,cwdPut
            jp c,cwdExit
            ld de,7
            add hl,de
            ld b,8
            call cwdPart
            jp c,cwdExit
            ld a,05Ch
            call cwdPut
            jp nc,cwdParent
            jp cwdExit
cwdRoot:
            ld a,(cwdLength)
            or a
            ld a,05Ch
            call z,cwdPut
            jp c,cwdExit
            ld a,':'
            call cwdPut
            jp c,cwdExit
            ld a,(v_drive)
            add a,'A'
            call cwdPut
            jp c,cwdExit
            ld hl,(cwdWrite)
            ld (hl),0
            dec hl
            ex de,hl
            ld hl,(cwdBuffer)
cwdReverse:
            call cmp_hl_de
            jp nc,cwdSuccess
            ld b,(hl)
            ld a,(de)
            ld (hl),a
            ld a,b
            ld (de),a
            inc hl
            dec de
            jp cwdReverse
cwdSuccess:
            xor a
            jp cwdExit
cwdCorrupt:
            call dosCorrupt
cwdExit:
            push af
            call selectDirectory
            pop af
            ret
cwdPart:
            ld a,(hl)
            cp ' '
            call nz,cwdPut
            ret c
            dec hl
            dec b
            jp nz,cwdPart
            or a
            ret
cwdPut:
            push hl
            push af
            ld a,(cwdLength)
            cp MAX_PATH-1
            jp nc,cwdOverflow
            inc a
            ld (cwdLength),a
            pop af
            ld hl,(cwdWrite)
            ld (hl),a
            inc hl
            ld (cwdWrite),hl
            pop hl
            or a
            ret
cwdOverflow:
            pop af
            pop hl
            jp blockArgument
cwdDots: DB '..         '
cwdBuffer: DW 0
cwdWrite: DW 0
cwdChild: DW 0
cwdLength: DB 0
