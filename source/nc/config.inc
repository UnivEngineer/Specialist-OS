; Both configuration layers share this reader/parser. Missing keys retain the
; preceding layer; duplicate extension rules update their existing entry.
ncConfigInit:
    IF NC_FILE_COLORS
    ld hl,ncDefaultColorRules
    ld de,ncDefaultColorRulesEnd
    ld bc,vars.fileColorRules
    call bios_memcpy_bc_hl
    ld hl,vars.fileColorRules+ncDefaultColorRulesEnd-ncDefaultColorRules-1
    ELSE
    ld hl,vars.fileColorRules
    ld (hl),0
    ENDIF
    ld (ncConfigRuleEnd),hl
    ret
ncConfigLoad:
    call ncConfigInit
    ld hl,ncConfigPath
; HL=path, CF/A=read status. Paths and input reuse NC buffers; selected drive
; is restored even if the parsed configuration changes either panel's CWD.
ncConfigRead:
    push hl
    ld e,0
    MXOS_CALL MXOS_DOS_FILE_GET_SET_DRIVE
    push af
    pop bc
    pop hl
    push bc
    call ncConfigSetPath
    ld hl,NC_SORT_KEYS
    ld (ncStateIO+FILE_PATH_IO.buffer),hl
    ld hl,2047
    ld (ncStateIO+FILE_PATH_IO.count),hl
    ld hl,ncStateIO
    call fileReadAll
    jp c,ncConfigLoaded
    ld hl,(ncStateIO+FILE_PATH_IO.actual)
    ld de,NC_SORT_KEYS
    add hl,de
    ld (hl),0
    ld hl,NC_SORT_KEYS
    call ncConfigParse
    xor a
ncConfigLoaded:
    push af
    pop bc
    pop af
    push bc
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    pop af
    ret
; Shared by the reader and state writer: HL=short filename -> MAX_PATH buffer.
ncConfigSetPath:
    ld de,vars.pathArg
ncConfigPathCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    or a
    jp nz,ncConfigPathCopy
    ld hl,vars.pathArg
    ld (ncStateIO+FILE_PATH_IO.path),hl
    ret

; HL=mutable NUL-terminated configuration. CR, LF, CRLF, blank/comment lines.
; Only valid, present parameters change the accumulated settings.
ncConfigParse:
ncConfigLine:
    ld (ncConfigCursor),hl
ncConfigLineEnd:
    ld a,(hl)
    or a
    jp z,ncConfigLastLine
    inc hl
    cp 13
    jp z,ncConfigSplit
    cp 10
    jp nz,ncConfigLineEnd
ncConfigSplit:
    dec hl
    ld (hl),0
    inc hl
    ld (ncConfigNext),hl
    call ncConfigDirective
    ld hl,(ncConfigNext)
    jp ncConfigLine
ncConfigLastLine:
    jp ncConfigDirective
ncConfigDirective:
    call ncConfigToken
    ret c
    ld hl,ncConfigDirectives
    call ncConfigLookup
    ret c
    ld (ncConfigKind),a
    cp 15
    jp nc,ncConfigDrive
    cp 4
    jp nc,ncStateConfig
    cp 3
    jp z,ncConfigSort
    call ncConfigToken
    ret c
    ld hl,ncConfigColors
    call ncConfigLookup
    ret c
    ld (ncConfigColor),a
    ld b,a
    ld a,(ncConfigKind)
    or a
    jp z,ncConfigExtensions
    dec a
    ld a,b
    jp nz,ncConfigDefault
    ld (ncFileSystemColor),a
    ret
ncConfigDefault:
    ld (ncFileDefaultColor),a
    ret
ncConfigSort:
    call ncConfigToken
    ret c
    ld a,b
    cp 1
    ret nz
    ld a,(de)
    sub '0'
    cp 3
    ret nc
    ld (ncDefaultSort),a
    ld (state.panelA_sort),a
    ld (state.panelB_sort),a
    ret
; The installed drive list is prepared before either configuration layer.
; Accept A..H with an optional colon. Missing drives keep the previous default.
ncConfigDrive:
    call ncConfigToken
    ret c
    ld a,b
    dec a
    jp z,ncConfigDriveLetter
    dec a
    ret nz
    inc de
    ld a,(de)
    cp ':'
    ret nz
    dec de
ncConfigDriveLetter:
    ld a,(de)
    sub 'A'
    cp 8
    ret nc
    ld c,a
    call findDrive
    ret c
    ld a,(ncConfigKind)
    sub 15
    ld e,a
    ld d,0
    ld hl,state.panelA_drive
    add hl,de
    ld (hl),c
    ld hl,state.panelA_dirLow
    add hl,de
    ld (hl),0
    ld hl,state.panelA_dirHigh
    add hl,de
    ld (hl),0
    ld hl,vars.pathValidA
    add hl,de
    ld (hl),0
    ret
ncConfigExtensions:
    call ncConfigToken
    ret c
    ld a,b
    cp 4
    jp nc,ncConfigExtensions
    ; Build a padded 8.3 extension, then update/append a single keyed rule.
    ld hl,vars.pathArg
    ld c,3
ncConfigExtension:
    ld a,' '
    inc b
    dec b
    jp z,ncConfigExtensionByte
    ld a,(de)
    inc de
    dec b
ncConfigExtensionByte:
    ld (hl),a
    inc hl
    dec c
    jp nz,ncConfigExtension
    ld hl,vars.fileColorRules
ncConfigFindRule:
    ld a,(hl)
    or a
    jp z,ncConfigAppendRule
    push hl
    ld de,vars.pathArg
    ld b,3
ncConfigCompareRule:
    ld a,(de)
    cp (hl)
    jp nz,ncConfigNextRule
    inc hl
    inc de
    dec b
    jp nz,ncConfigCompareRule
    pop de
    jp ncConfigRuleColor
ncConfigNextRule:
    pop hl
    ld de,4
    add hl,de
    jp ncConfigFindRule
ncConfigAppendRule:
    ld a,l
    sub vars.fileColorRules & 0FFh
    cp 4*32
    jp nc,ncConfigExtensions ; Full table still permits later overrides.
    ld de,vars.pathArg
    ld c,3
ncConfigAppendByte:
    ld a,(de)
    inc de
    ld (hl),a
    inc hl
    dec c
    jp nz,ncConfigAppendByte
    push hl
    inc hl
    ld (ncConfigRuleEnd),hl
    ld (hl),0
    pop hl
ncConfigRuleColor:
    ld a,(ncConfigColor)
    ld (hl),a
    jp ncConfigExtensions

; Next uppercase token, DE=start, B=length, CF=end/comment; cursor retained.
ncConfigToken:
    ld hl,(ncConfigCursor)
ncConfigWhite:
    ld a,(hl)
    cp ' '
    jp z,ncConfigWhiteNext
    cp 9
    jp nz,ncConfigTokenStart
ncConfigWhiteNext:
    inc hl
    jp ncConfigWhite
ncConfigTokenStart:
    or a
    jp z,ncConfigNoToken
    cp '#'
    jp z,ncConfigNoToken
    cp '"'
    jp z,ncConfigQuoted
    ld d,h
    ld e,l
    ld b,0
ncConfigTokenByte:
    ld a,(hl)
    or a
    jp z,ncConfigTokenReady
    cp ' '
    jp z,ncConfigTokenEnd
    cp 9
    jp z,ncConfigTokenEnd
    call bios_upperCase
    ld (hl),a
    inc hl
    inc b
    jp nz,ncConfigTokenByte
    dec b ; Saturate oversized tokens so an invalid extension cannot wrap.
    jp ncConfigTokenByte
ncConfigTokenEnd:
    ld (hl),0
    inc hl
    jp ncConfigTokenReady
; Quoting keeps literal leading '#' distinct from a comment, notably for a
; saved selected FAT name. Backslashes are ordinary path characters.
ncConfigQuoted:
    inc hl
    ld d,h
    ld e,l
    ld b,0
ncConfigQuotedByte:
    ld a,(hl)
    or a
    jp z,ncConfigBadQuote
    cp '"'
    jp z,ncConfigTokenEnd
    call bios_upperCase
    ld (hl),a
    inc hl
    inc b
    jp nz,ncConfigQuotedByte
    dec b
    jp ncConfigQuotedByte
ncConfigBadQuote:
    ld a,1
    scf
    ret
ncConfigTokenReady:
    ld (ncConfigCursor),hl
    or a
    ret
ncConfigNoToken:
    scf
    ret

; DE=uppercase token, HL=ASCIIZ name / value table, empty-name terminator.
ncConfigLookup:
    ld a,(hl)
    or a
    jp z,ncConfigNoToken
    push de
ncConfigCompare:
    ld a,(de)
    cp (hl)
    jp nz,ncConfigSkipName
    inc de
    inc hl
    or a
    jp nz,ncConfigCompare
    ld a,(hl)
    pop de
    or a
    ret
ncConfigSkipName:
    ld a,(hl)
    inc hl
    or a
    jp nz,ncConfigSkipName
    inc hl
    pop de
    jp ncConfigLookup
ncDefaultSort:          DB 2
ncFileDefaultColor:     DB COLOR_FILE
ncFileSystemColor:      DB 071h
ncConfigCursor:         DW 0
ncConfigNext:           DW 0
ncConfigRuleEnd:        DW 0
ncConfigKind:           DB 0
ncConfigColor:          DB 0
ncConfigPath:           DB BOOT_VOLUME_LETTER,':',05Ch,'DOS',05Ch,'NC.CFG',0
ncConfigDirectives:
    DB 'FILECOLORUSER',0,0,'FILECOLORSYS',0,1,'FILECOLORDEF',0,2
    DB 'DEFAULTSORTMODE',0,3
    DB 'DEFAULTLEFTDRIVE',0,15,'DEFAULTRIGHTDRIVE',0,16
ncKeyActive:            DB 'ACTIVEPANEL',0,4
ncKeyLeftPath:          DB 'LEFTPATH',0,5
ncKeyRightPath:         DB 'RIGHTPATH',0,6
ncKeyLeftSort:          DB 'LEFTSORT',0,7
ncKeyRightSort:         DB 'RIGHTSORT',0,8
ncKeyLeftInfo:          DB 'LEFTINFO',0,9
ncKeyRightInfo:         DB 'RIGHTINFO',0,10
ncKeyLeftSelected:      DB 'LEFTSELECTED',0,11
ncKeyRightSelected:     DB 'RIGHTSELECTED',0,12
ncKeyLeftFirst:         DB 'LEFTFIRST',0,13
ncKeyRightFirst:        DB 'RIGHTFIRST',0,14
    DB 0
ncConfigColors:
    DB 'BLACK',0,001h,'BLUE',0,011h,'GREEN',0,021h,'CYAN',0,031h
    DB 'RED',0,041h,'MAGENTA',0,051h,'BROWN',0,061h,'LIGHTGRAY',0,071h
    DB 'DARKGRAY',0,081h,'LIGHTBLUE',0,091h,'LIGHTGREEN',0,0A1h
    DB 'LIGHTCYAN',0,0B1h,'LIGHTRED',0,0C1h,'LIGHTMAGENTA',0,0D1h
    DB 'YELLOW',0,0E1h,'WHITE',0,0F1h,0
