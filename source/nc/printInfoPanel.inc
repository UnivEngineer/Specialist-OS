; F3: render all immediate information before the slow FAT free-space scan.
printInfoPanel:
            call pip_prepareInfo
            call ncResetPanelBorder
            call drawPanelTop
            ld hl,g_infoPanel
            call draw
            jp pip_drawContents
; Synchronous queries drain the graphics queue. Read the metadata together
; before writing any F3 rows. The slow free-space scan still follows the paint.
pip_prepareInfo:
            xor a
            ld (pipHasVolume),a
            ; The info panel follows the opposite (active file) panel's drive.
            ld hl,state.panelB_drive
            ld a,(state.activePanel)
            or a
            jp z,pip_infoDrive
            dec hl
pip_infoDrive:
            ld a,(hl)
            ld (ncInfoDrive),a
            call pip_readMain
            ex de,hl
            ld (pipMainFree),hl
            ld a,MEM_OP_RAMDISK
            ld c,MEM_RAMDISK_STATUS
            MXOS_CALL MXOS_DOS_MEMORY
            ld a,b
            inc a ; DOS page plus the RAMDISK pool.
    IF NC_BANKED
            inc a ; This NC's resident EXEC page.
    ENDIF
            ld (pipReservedPages),a
            ld b,3
            call bios_diskDriver
            ld (vars.diskInfoPtr),hl
            ex de,hl
            ld (pipDiskBlocks),hl
            ex de,hl
            ld bc,DISK_INFO.isValid
            add hl,bc
            ld a,(hl)
            cp DISK_VALID
            jp c,pip_readDriver
            ld a,1
            ld (pipHasVolume),a
pip_readDriver:
            ld hl,0
            ld b,0FFh
            call bios_diskDriver
            jp c,pip_driverUnknown
            ld a,h
            or l
            jp nz,pip_driverName
pip_driverUnknown:
            ld b,0
            ld hl,aDriverUnknown
pip_driverName:
            ld (pipDriverName),hl
            ld a,b
            ld (pipDriverTech),a
pip_findDescription:
            ld a,(hl)
            inc hl
            or a
            jp nz,pip_findDescription
            ld (pipDriverDescription),hl
            ret

printInfoContents:
            call pip_prepareInfo
pip_drawContents:
            ; All tables share X=64px (56px at 8x8), with an 8px right inset.
            ld a,(ncFontWidth)
            ld c,32 ; BIOS half-pixels.
            cp 8
            jp nz,pip_narrowFields
            ld c,28
            ld b,16
            jp pip_fieldLimits
pip_narrowFields:
            ld b,20
            cp 6
            jp z,pip_fieldLimits
            ld b,30
pip_fieldLimits:
            ld a,c
            ld (pipDetailX),a
            ld a,b
            ld (pipDetailChars),a
            COLOR COLOR_INFOHEADER
            ld e,17
            ld hl,aCommanderVer
            call ncPrintPanelCentered
            ld e,27
            ld hl,aCopyright
            call ncPrintPanelCentered
            ld a,46
            ld de,65504
            ld hl,aMainRAM
            call pip_printNumber
            ld hl,(pipMainFree)
            ex de,hl
            ld a,56
            ld hl,aMainFree
            call pip_printNumber
            ld a,(bios_upper.ramPageCount)
            ld h,a
            ld l,0
            call ncMath.div_hl_2
            call ncMath.div_hl_2
            ex de,hl
            ld a,66
            ld hl,aBankRAM
            call pip_printNumber
            ld a,(bios_upper.ramPageCount)
            ld e,a
            ld d,0
            ld a,76
            ld hl,aBankPages
            call pip_printNumber
            ld a,(pipReservedPages)
            ld e,a
            ld d,0
            push de
            ld a,86
            ld hl,aReservedPages
            call pip_printNumber
            pop de
            ld a,(bios_upper.ramPageCount)
            sub e
            ld e,a
            ld a,96
            ld hl,aFreePages
            call pip_printNumber
            ld a,(pipHasVolume)
            or a
            jp z,pip_printDiskError
            ld hl,(pipDiskBlocks)
            ex de,hl
            ld hl,(vars.diskInfoPtr)
            ld bc,DISK_INFO.blockSize
            add hl,bc
            ld c,(hl)
            inc hl
            ld b,(hl)
            ld h,b
            ld l,c
            call pip_unitsKiB
            ld a,120
            ld hl,aDiskTotal
            call pip_printDiskNumber
            ; Publish the free-space label now; leave its numeric field blank.
            ld a,130
            ld hl,aDiskFree
            call pip_printDiskLabel
            jp pip_printDriver
pip_printDiskError:
            ld hl,047Dh ; One error row centered in the disk block.
            call setCursorPosPanel
            COLOR COLOR_INFOTEXT
            ld hl,aDrive
            call ncPrintPanelText
            call pip_printInfoDrive
            ld hl,aNotFormatted
            call ncPrintPanelText
pip_printDriver:
            ld hl,(pipDriverDescription)
            ld a,(pipHasVolume)
            add a,2 ; Fixed rows: optional Label, Driver, Type.
            ld c,a
            xor a
            ld (pipHasDescription),a
            or (hl)
            jp z,pip_descriptionCounted
            inc c ; A description occupies at most one clipped line.
            ld a,1
            ld (pipHasDescription),a
pip_descriptionCounted:
            ld a,c
            ld (pipRows),a
pip_metadataCounted:
            ; Region Y=141..203: center fixed fields and one clipped description.
            ; First baseline = 180 - 5 * row count (glyphs occupy Y-7..Y).
            ld a,(pipRows)
            ld b,a
            add a,a
            add a,a
            add a,b
            ld b,a
            ld a,180
            sub b
            ld l,a
            ld h,4
            call setCursorPosPanel
            ld a,(pipHasVolume)
            or a
            jp z,pip_drawDriver
            call pip_volumeLabel
            ex de,hl
            ld hl,aVolumeLabel
            call pip_printDetail
            call pip_nextInfoRow
pip_drawDriver:
            ld hl,(pipDriverName)
            ex de,hl
            ld hl,aDiskDriver
            call pip_printDetail
            call pip_nextInfoRow
            ld a,(pipDriverTech)
            ld de,aTypeROM
            cp DISK_TECH_ROM
            jp z,pip_driverType
            ld de,aTypeRAM
            cp DISK_TECH_RAM
            jp z,pip_driverType
            ld de,aTypeFlash
            cp DISK_TECH_FLASH
            jp z,pip_driverType
            ld de,aTypeFloppy
            cp DISK_TECH_FLOPPY
            jp z,pip_driverType
            ld de,aTypeSystemFlash
            cp DISK_TECH_MX_SYSTEM_FLASH
            jp z,pip_driverType
            ld de,aTypeSystemRom
            cp DISK_TECH_MX_SYSTEM_ROM
            jp z,pip_driverType
            ld de,aTypeVv55Flash
            cp DISK_TECH_MX_VV55_FLASH
            jp z,pip_driverType
            ld de,aTypeVv55Rom
            cp DISK_TECH_MX_VV55_ROM
            jp z,pip_driverType
            ld de,aDriverUnknown
pip_driverType:
            ld hl,aDiskTechnology
            call pip_printDetail
            ld a,(pipHasDescription)
            or a
            jp z,pip_infoEnd
            ld hl,(pipDriverDescription)
            call pip_printDescription
pip_infoEnd:
            call pip_printFont
pip_freeSpaceLast:
            ; No other screen text is delayed by this potentially slow request.
            ld a,(ncInfoFreeDeferred)
            or a
            ret nz
            ld a,(pipHasVolume)
            or a
            ret z
    IF NC_BANKED
            call ncBankFlush
    ENDIF
            MXOS_CALL MXOS_DOS_GET_DRIVE_FREE_SPACE
            ret c
            ld hl,(vars.diskInfoPtr)
            ld bc,DISK_INFO.clusterBytes
            add hl,bc
            ld c,(hl)
            inc hl
            ld b,(hl)
            ld h,b
            ld l,c
            call pip_unitsKiB
            ld a,130
            jp pip_printNumberOnly
pip_printDescription:
            call pip_nextInfoRow
            ex de,hl
            ld hl,pipDriverInfoLabel
            jp pip_printDetail
pip_printFont:
    IF NC_BANKED
            ld hl,io_vars.fontName
            ld de,pipFontName
            call ncGfxReadBytes8
            ld hl,pipFontName
    ELSE
            ld hl,io_vars.fontName
    ENDIF
            ld de,pipFontText
            ld b,8
pip_fontNameLoop:
            ld a,(hl)
            or a
            jp z,pip_fontNameEnd
            cp ' '
            jp z,pip_fontNameEnd
            ld (de),a
            inc hl
            inc de
            dec b
            jp nz,pip_fontNameLoop
pip_fontNameEnd:
            ld a,e
            cp pipFontText & 0FFh
            jp nz,pip_fontNameKnown
            ld hl,aDriverUnknown
            call pip_fontAppend
pip_fontNameKnown:
            ld hl,pipFontComma
            call pip_fontAppend
            push de
            ld hl,(pipFontBytes)
            ex de,hl
            ld b,0
            call ncFormatDecimal
            pop de
            call pip_fontAppend
            ld hl,pipFontUnit8
            ld a,(ncFontWidth)
            cp 8
            jp z,pip_fontUnitReady
            ld a,e
            sub pipFontText & 0FFh
            add a,6 ; Length of " bytes".
            ld b,a
            ld a,(pipDetailChars)
            cp b
            jp c,pip_fontUnitReady
            ld hl,pipFontUnit
pip_fontUnitReady:
            call pip_fontAppend
            xor a
            ld (de),a
            ld hl,041DBh ; Baseline 219, matching the established font section.
            call setCursorPosPanel
            ld de,pipFontText
            ld hl,pipFontLabel
            jp pip_printDetail
pip_fontAppend:
            ld a,(hl)
            or a
            ret z
            ld (de),a
            inc hl
            inc de
            jp pip_fontAppend
pipDriverInfoLabel:     DB 'Info',0
pipFontLabel:           DB 'Font',0
pipFontComma:           DB ', ',0
pipFontUnit:            DB ' bytes',0
pipFontUnit8:           DB ' B',0
pipFontBytes:           DW 0
pipFontText:            DS 28,0
    IF NC_BANKED
pipFontName: DS 8,0
    ENDIF
pip_volumeLabel:
            ld hl,(vars.diskInfoPtr)
            ld de,DISK_INFO.volumeLabel
            add hl,de
            ret
pipHasVolume:           DB 0
pipMainFree:            DW 0
pipDiskBlocks:          DW 0
pipReservedPages:       DB 0
pipRows:                DB 0
pipDetailX:             DB 0
pipDetailChars:         DB 0
pipHasDescription:      DB 0
ncInfoDrive:            DB 0FFh
pipDriverTech:          DB 0
pipDriverName:          DW 0
pipDriverDescription:   DW 0
ncInfoFreeDeferred:     DB 0

; Sequential metadata rows; the description is a single centered line.
; Preserve the original string pointer in HL and the value pointer in DE.
pip_nextInfoRow:
            push hl
            ld hl,(bios_vars.cursorY)
            ld a,l
            add a,10
            ld l,a
            ld h,4
            call setCursorPosPanel
            pop hl
            ret

; A=Y, DE=number, HL=label. Numeric field starts at X=8 and is byte-aligned.
; Field width 12/8/5 chars for 4/6/8px; leading spaces right-align digits.
pip_readMain:
            ld bc,MEM_RESIDENT_FONT
            ld a,MEM_OP_RESIDENT
            MXOS_CALL MXOS_DOS_MEMORY
            jp c,pip_readMainFallback
            ex de,hl
            ld (pipFontBytes),hl
            ex de,hl
            inc bc
            ld d,b
            ld e,c
    IF NC_BANKED
            ; The scope pins GFX while NC draws. FILE_EXEC can evict it.
            ; This is the upper limit with FONT retained, from address 0.
            push hl
            ld hl,GFX_BASE
            call bios_cmp_hl_de
            pop hl
            ret nz
            ld a,h
            or l
            ld de,APP_MEMORY_END
            ret z
            ex de,hl
    ENDIF
            ret
pip_readMainFallback:
            ld hl,0
            ld (pipFontBytes),hl
            call bios_getMemTop
            inc hl
            ex de,hl
            ret
pip_printNumber:
            push hl
            push af
            call pip_printNumberOnly
            pop af
            pop hl
pip_printLabel:
            push hl
            ld l,a
            ld a,(pipDetailX)
            ld h,a
            call setCursorPosPanel
            pop hl
            COLOR COLOR_INFOTEXT
            jp ncPrintPanelText
pip_printDiskLabel:
            call pip_printLabel
pip_printInfoDrive:
            ld a,(ncInfoDrive)
            jp printDrive
pip_printNumberOnly:
            ld l,a
            ld h,4
            call setCursorPosPanel
            ld b,' '
            push de
            call ncFormatDecimal
            pop de
            ld a,(ncFontWidth)
            ld b,5
            cp 8
            jp z,pip_numberPadding
            ld b,8
            cp 6
            jp z,pip_numberPadding
            ld b,12
pip_numberPadding:
            ld a,b
            sub 5
            ld b,a
            COLOR COLOR_INFONUMBER
            call nz,printSpaces
            ld hl,ncDecimalText
            jp bios_printStringReal
pip_printDiskNumber:
            call pip_printNumber
            jp pip_printInfoDrive ; Same color as the label, no color seam.

; DE=count, HL=power-of-two bytes/unit (256..32768) -> DE=KiB.
; Never forms the intermediate byte total, which would overflow 16 bits.
pip_unitsKiB:
            ld a,h
            cp 4
            ret z
            jp c,pip_unitsDivide
            call ncMath.div_hl_2
            ex de,hl
            add hl,hl
            ex de,hl
            jp pip_unitsKiB
pip_unitsDivide:
            add hl,hl
            ex de,hl
            call ncMath.div_hl_2
            ex de,hl
            jp pip_unitsKiB

; HL=label, DE=value. Right-align labels before the shared value column.
; Values are truncated to the fixed field length for the current font.
pip_printDetail:
            push de
            push hl
            ld hl,(bios_vars.cursorY)
            ld e,l
            pop hl
            COLOR COLOR_INFOTEXT
            ld b,4
            ld a,(ncFontWidth)
            cp 8
            jp nz,pip_detailAlign
            ld b,2 ; X=4..51, leaving four pixels before the value at X=56.
pip_detailAlign:
            ld c,24
            ld a,UI_ALIGN_RIGHT
            call ncPrintPanelAligned
            pop hl
            push hl
            ld hl,(bios_vars.cursorY)
            ld a,(pipDetailX)
            ld h,a
            call setCursorPosPanel
            pop hl
            COLOR COLOR_INFONUMBER
            ld a,(pipDetailChars)
            ld b,a
            jp ncPrintTextLim
