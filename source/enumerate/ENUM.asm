; Lazy shared wildcard/recursive enumerator. No public fixed entry address.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../../bin/enumerate-imports.inc"
    ORG DOS_SERVICE_ADDR
    jp enumEntry
    INCLUDE "host.inc"
enumEntry:
    or a
    jp z,enBegin
    dec a
    jp z,enNext
    xor a
    ld (enOpen),a
    ret
; HL=local ASCIIZ path, C=flags. The resident facade preserves every CWD.
enBegin:
    ld a,c
    and 0FCh
    jp nz,blockArgument
    ld a,c
    ld (enFlags),a
    ld de,enInput
    ld b,MAX_PATH
enCopyInput:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    or a
    jp z,enCopied
    dec b
    jp nz,enCopyInput
    jp blockArgument
enCopied:
    ld a,(v_drive)
    ld (enInitialDrive),a
    ld hl,v_directories
    ld de,enInitialDirectories
    ld bc,16
    call moveBytesForward
    ld hl,(v_directory)
    ld (enInitialDirectory),hl
    ld a,(enInput)
    or a
    ld hl,enAll
    jp z,enMaskReady
    ld hl,enInput
    call directoryPathUnchecked
    ld hl,enAll
    jp nc,enMaskReady
    ld hl,enInitialDirectories
    ld de,v_directories
    ld bc,16
    call moveBytesForward
    ld a,(enInitialDrive)
    call fileChangeDrive
    ret c
    ld hl,(enInitialDirectory)
    ld (v_directory),hl
    ; Split only the final component. Wildcards in directory names are rejected.
    ld hl,enInput
    ld de,enInput
enSplit:
    ld a,(hl)
    or a
    jp z,enSplitDone
    inc hl
    cp ':'
    jp z,enSeparator
    cp '/'
    jp z,enSeparator
    cp 05Ch
    jp nz,enSplit
enSeparator:
    ld d,h
    ld e,l
    jp enSplit
enSplitDone:
    ex de,hl
    push hl
    ld de,enMask
    ld bc,MAX_PATH
enMaskCopy:
    ld a,(hl)
    ld (de),a
    inc de
    inc hl
    or a
    jp nz,enMaskCopy
    pop hl
    ld (hl),0
    ld hl,enInput
    ld a,(hl)
    or a
    call nz,directoryPathUnchecked
    ret c
    ld hl,enMask
enMaskReady:
    ld (enPattern),hl
    ld a,(v_drive)
    ld (enDrive),a
    ld hl,enPath
    call directoryCwdUnchecked
    ret c
    ld hl,enPath
    ld b,0
enPrefixLength:
    ld a,(hl)
    or a
    jp z,enPrefixEnd
    inc b
    inc hl
    jp enPrefixLength
enPrefixEnd:
    dec hl
    ld a,(hl)
    inc hl
    cp 05Ch
    jp z,enPrefixReady
    ld a,b
    cp MAX_PATH-2
    jp nc,blockArgument
    ld (hl),05Ch
    inc b
enPrefixReady:
    xor a
    ld (enDepth),a
    ld hl,enFrames
    ld (enFrame),hl
    call enStartFrame
    ret c
    ld a,1
    ld (enOpen),a
    xor a
    ret
; Frame: directory word, prefix length byte, FAT cursor 11 bytes, sector word.
enStartFrame:
    push bc
    ld hl,(enFrame)
    push hl
    ld hl,(v_directory)
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (hl),b
    call fatDirStart
    pop bc
    ret c
    jp enSaveCursor
enSaveCursor:
    ld hl,(enFrame)
    ld de,3
    add hl,de
    ex de,hl
    ld hl,v_dirRemaining
    ld bc,11
    call moveBytesForward
    ld d,b
    ld e,c
    ld hl,(v_cachedSector)
    ex de,hl
    ld (hl),e
    inc hl
    ld (hl),d
    or a
    ret
; CF=1/A=0 end, CF=1/A!=0 error. CF=0/HL=160-byte entry.
enNext:
    ld a,64
    ld (enBudget),a
    ld a,(enOpen)
    or a
    jp z,enEnd
enNextFrame:
    ld a,(enBudget)
    or a
    jp z,enYield
    dec a
    ld (enBudget),a
    ld a,(enFlags)
    and ENUM_CHECK_CANCEL
    call nz,cliCheckCancel
    ret c
    ld a,(enDrive)
    call fileChangeDrive
    ret c
    ld hl,(enFrame)
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    inc hl
    push hl
    ld l,a
    ld h,0
    ld de,enPath
    add hl,de
    ld (hl),0
    pop hl
    push hl
    dec hl
    dec hl
    dec hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    push hl
    ex de,hl
    ld (v_directory),hl
    ex de,hl
    pop hl
    pop hl
    ld de,v_dirRemaining
    ld bc,11
    call moveBytesForward
    ld e,(hl)
    inc hl
    ld d,(hl)
    push hl
    ex de,hl
    ld (v_cachedSector),hl
    ex de,hl
    pop hl
    ld a,(v_dirRemaining)
    or a
    jp z,enRead
    xor a
    call getSectorAddressInCache
    ret c
    push hl
    ld hl,(v_cachedDescrOffset)
    ex de,hl
    pop hl
    add hl,de
    ld (v_cachedDescrPtr),hl
enRead:
    call fatDirItem
    jp c,enDirectoryEnd
    push hl
    call enSaveCursor
    pop hl
    ld a,(hl)
    cp '.'
    jp z,enNextFrame
    ld de,enResult
    ld bc,32
    call moveBytesForward
    ld hl,(enResult+FILE_DESCRIPTOR.firstClusterHi)
    ld a,h
    or l
    jp nz,dosCorrupt
    ld hl,enResult
    ld a,(hl)
    cp 5
    jp nz,enName
    ld (hl),0E5h
enName:
    ld de,enNameText
    ld b,8
    call enNamePart
    ld hl,enResult+8
    ld a,(hl)
    cp ' '
    jp z,enNameDone
    ld a,'.'
    ld (de),a
    inc de
    ld b,3
    call enNamePart
enNameDone:
    xor a
    ld (de),a
    ; Bound the canonical path, including a possible recursive separator/NUL.
    ld hl,enNameText
    ex de,hl
    ld a,l
    sub e
    ld b,a
    ld hl,(enFrame)
    inc hl
    inc hl
    add a,(hl)
    cp MAX_PATH-1
    jp nc,blockArgument
    ld b,a
    xor a
    ld hl,enPath
    ld de,enResult+32
    call enString
    ld hl,enNameText
    call enString
    ld (de),a
    ; Descend before returning; the next call resumes the prepared child.
    ld a,(enFlags)
    and ENUM_RECURSIVE
    jp z,enMatch
    ld a,(enResult+11)
    and 10h
    jp z,enMatch
    ld a,(enDepth)
    cp ENUM_MAX_DEPTH-1
    jp nc,blockArgument
    ld hl,enPath
    ld de,enPath
enPathEnd:
    ld a,(hl)
    or a
    jp z,enAppend
    inc hl
    inc de
    jp enPathEnd
enAppend:
    ld hl,enNameText
    call enString
    ret c
    ld a,05Ch
    ld (de),a
    inc de
    xor a
    ld (de),a
    ld hl,enPath
    ex de,hl
    ld a,l
    sub e
    ld b,a
    ld hl,(enResult+FILE_DESCRIPTOR.firstCluster)
    ex de,hl
    call fatCheckCluster
    ret c
    ex de,hl
    ld (v_directory),hl
    ld hl,(enFrame)
    ld de,16
    add hl,de
    ld (enFrame),hl
    ld a,(enDepth)
    inc a
    ld (enDepth),a
    call enStartFrame
    ret c
enMatch:
    call enWildcard
    jp nz,enNextFrame
    ld hl,enResult
    or a
    ret
enDirectoryEnd:
    or a
    scf
    ret nz
    ld a,(enDepth)
    or a
    jp z,enEnd
    dec a
    ld (enDepth),a
    ld hl,(enFrame)
    ld de,0FFF0h
    add hl,de
    ld (enFrame),hl
    jp enNextFrame
enYield:
    ld hl,0
    xor a
    ret
enEnd:
    xor a
    ld (enOpen),a
    scf
    ret
enNamePart:
    ld a,(hl)
    inc hl
    cp ' '
    ret z
    ld (de),a
    inc de
    dec b
    jp nz,enNamePart
    ret
; NUL excluded. The caller checks room for directory/name before copying.
enString:
    ld a,(hl)
    or a
    ret z
    ld (de),a
    inc hl
    inc de
    jp enString
; Case-insensitive glob with backtracking only to the most recent '*'.
enWildcard:
    ld hl,(enPattern)
    ld de,enNameText
    ld bc,0
    push hl
    ld h,b
    ld l,c
    ld (enStar),hl
    pop hl
    ; DOS *.* also matches names without an extension.
    ld a,(hl)
    cp '*'
    jp nz,enGlob
    inc hl
    ld a,(hl)
    cp '.'
    jp nz,enGlobReset
    inc hl
    ld a,(hl)
    cp '*'
    jp nz,enGlobReset
    inc hl
    ld a,(hl)
    or a
    ret z
enGlobReset:
    ld hl,(enPattern)
enGlob:
    ld a,(hl)
    cp '*'
    jp z,enGlobStar
    ld a,(de)
    or a
    jp z,enGlobEnd
    ld a,(hl)
    cp '?'
    jp z,enGlobAdvance
    call upperCase
    ld b,a
    ld a,(de)
    call upperCase
    cp b
    jp z,enGlobAdvance
    push hl
    ld hl,(enStar)
    ld b,h
    ld c,l
    pop hl
    ld a,b
    or c
    jp z,enGlobFail
    ld h,b
    ld l,c
    push hl
    ld hl,(enRetry)
    ex de,hl
    pop hl
    inc de
    push hl
    ex de,hl
    ld (enRetry),hl
    ex de,hl
    pop hl
    jp enGlob
enGlobAdvance:
    inc hl
    inc de
    jp enGlob
enGlobStar:
    inc hl
    ld (enStar),hl
    push hl
    ex de,hl
    ld (enRetry),hl
    ex de,hl
    pop hl
    jp enGlob
enGlobEnd:
    ld a,(hl)
    or a
    ret
enGlobFail:
    ld a,1
    or a
    ret
enAll: DB '*',0
enPattern: DW 0
enStar: DW 0
enRetry: DW 0
enFrame: DW 0
enOpen: DB 0
enFlags: DB 0
enDepth: DB 0
enDrive: DB 0
enBudget: DB 0
enNameText: DS 13,0
enInitialDrive: DB 0
enInitialDirectory: DW 0
enInitialDirectories: DS 16,0
enInput: DS MAX_PATH,0
enMask: DS MAX_PATH,0
enPath: DS MAX_PATH,0
enResult: DS ENUM_ENTRY,0
enFrames: DS ENUM_MAX_DEPTH*16,0
enumEnd:
    ASSERT $ <= DOS_SERVICE_END
    END
