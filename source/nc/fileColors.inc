; NC-style palette. Yellow is reserved for future marked-file selections.
COLOR_DIRECTORY = 0F1h
COLOR_EXECUTABLE = 0A1h
COLOR_ARCHIVE = 0D1h
COLOR_SYSTEM_FILE = 071h

; HL=FILE_INFO -> A=normal palette. BC/DE/HL preserved. No DOS queries.
ncFileColor:
    push hl
    push bc
    push de
    ld de,FILE_INFO.attrib
    add hl,de
    ld a,(hl)
    ld (ncFileColorAttrib),a
    and 10h
    jp nz,ncFileDirectory
    ld de,FILE_INFO.ext-FILE_INFO.attrib
    add hl,de
    ld a,(hl)
    call bios_upperCase
    ld b,a
    inc hl
    ld a,(hl)
    call bios_upperCase
    ld c,a
    inc hl
    ld a,(hl)
    call bios_upperCase
    ld d,a
    ld hl,vars.fileColorRules
ncFileColorRule:
    ld a,(hl)
    or a
    jp z,ncFileDefault
    inc hl
    cp b
    jp nz,ncFileSkip3
    ld a,(hl)
    inc hl
    cp c
    jp nz,ncFileSkip2
    ld a,(hl)
    inc hl
    cp d
    jp nz,ncFileSkip1
    ld a,(hl)
    jp ncFileColorPop ; Explicit extension rules override System/Hidden.
ncFileSkip3:
    inc hl
ncFileSkip2:
    inc hl
ncFileSkip1:
    inc hl
    jp ncFileColorRule
ncFileDirectory:
    ld a,COLOR_DIRECTORY
    jp ncFileColorPop
ncFileDefault:
    ld a,(ncFileColorAttrib)
    and 6
    ld a,(ncFileDefaultColor)
    jp z,ncFileColorPop
    ld a,(ncFileSystemColor)
ncFileColorPop:
    pop de
    pop bc
    pop hl
    ret
ncFileColorAttrib: DB 0
ncDefaultColorRules:
    DB 'COM',COLOR_EXECUTABLE,'EXE',COLOR_EXECUTABLE,'BAT',COLOR_EXECUTABLE
    DB 'RKS',COLOR_EXECUTABLE,'MON',COLOR_EXECUTABLE,'FNT',COLOR_EXECUTABLE
    DB 'ZIP',COLOR_ARCHIVE,'ARJ',COLOR_ARCHIVE,'RAR',COLOR_ARCHIVE
    DB 'LZH',COLOR_ARCHIVE,'ARC',COLOR_ARCHIVE,'TAR',COLOR_ARCHIVE
    DB 'GZ ',COLOR_ARCHIVE,'7Z ',COLOR_ARCHIVE
    DB 0
ncDefaultColorRulesEnd:

; Absolute screen address of a cell -> HL=software palette entry.
; BC/DE destroyed. Three columns reserved per panel; ascending column, then Y.
ncRowColorAt:
    ld a,h
    sub SCROLL_LEFT
    ld c,0
    cp 24
    jp c,ncRowColorPanel
    sub 24
    ld c,3*P_FILE_LIST_H
ncRowColorPanel:
    ld d,a
    ld a,(ncColumnStep)
    rrca
    rrca
    ld e,a
    ld a,d
ncRowColorColumn:
    sub e
    jp c,ncRowColorY
    ld d,a
    ld a,c
    add a,P_FILE_LIST_H
    ld c,a
    ld a,d
    jp ncRowColorColumn
ncRowColorY:
    ld a,l
    sub SCROLL_TOP
ncRowColorRow:
    sub 10
    jp c,ncRowColorReady
    inc c
    jp ncRowColorRow
ncRowColorReady:
    ld b,0
    ld hl,vars.rowColors
    add hl,bc
    ret

; HL=record, DE=relative BIOS coordinates. Store the normal color even when
; a retained cursor is printed inversely. BC/DE/HL preserved.
ncPaintFileColor:
    push hl
    push de
    push bc
    call ncFileColor
    push af
    ld a,d
    rrca
    rrca
    add a,090h
    ld h,a
    ld a,e
    sub 8
    ld l,a
    call activePanelPos
    call ncRowColorAt
    pop af
    ld (hl),a
    ld b,a
    ld a,(bios_vars.inverse)
    or a
    ld a,b
    jp z,ncPaintColorReady
    ld a,COLOR_CURSOR
ncPaintColorReady:
    call ncSetColor
    pop bc
    pop de
    pop hl
    ret

; HL/BC=absolute source/destination, DE=width/height, as uiCopyTextRect.
; Colors are captured before any table or bitmap writes by the common API.
    IF NC_BITMAP_SCROLL
ncCopyColoredText:
    ld (ncColorCopyRequest),hl
    push hl
    ld h,b
    ld l,c
    ld (ncColorCopyRequest+2),hl
    pop hl
    ld a,d
    ld (ncColorCopyRequest+4),a
    ld a,e
    ld (ncColorCopyRequest+5),a
    push bc
    call ncRowColorAt
    ld (ncColorCopyRequest+6),hl
    pop hl
    call ncRowColorAt
    ld (ncColorCopyRequest+8),hl
    ld hl,ncColorCopyRequest
    call uiCopyTextRectColors
    ld (ncCurrentColor),a
    ret
ncColorCopyRequest: DS 10,0
    ENDIF
