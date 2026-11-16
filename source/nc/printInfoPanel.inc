; F3: render all immediate information before the slow FAT free-space scan.
printInfoPanel:
            COLOR COLOR_BORDER
            call drawPanelTop
            ld hl,g_infoPanel
            call draw
printInfoContents:
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
            ; Longest label is "Driver ": one common, byte-aligned value X.
            ld a,7
            call ncTextAdvance
            add a,7 ; Left inset 4 plus upward rounding to a screen byte.
            and 0FCh
            ld (pipDetailX),a
            ; Fixed field limits for the 4/6/8px layouts; no value wrapping.
            ld a,(ncFontWidth)
            ld b,15
            ld c,22
            cp 8
            jp z,pip_fieldLimits
            ld b,21
            ld c,29
            cp 6
            jp z,pip_fieldLimits
            ld b,36
            ld c,44
pip_fieldLimits:
            ld a,b
            ld (pipDetailChars),a
            ld a,c
            ld (pipDescriptionChars),a
            COLOR COLOR_INFOHEADER
            ld hl,0411h
            call setCursorPosPanel
            ld hl,aCommanderVer
            call ncPrintPanelText
            ld hl,041Bh
            call setCursorPosPanel
            ld hl,aCopyright
            call ncPrintPanelText
            ld a,50
            ld de,65504
            ld hl,aMainRAM
            call pip_printNumber
            call bios_getMemTop
            inc hl
            ex de,hl
            ld a,60
            ld hl,aMainFree
            call pip_printNumber
            ld a,(bios_upper.ramPageCount)
            ld h,a
            ld l,0
            call ncMath.div_hl_2
            call ncMath.div_hl_2
            ex de,hl
            ld a,70
            ld hl,aBankRAM
            call pip_printNumber
            ld a,(bios_upper.ramPageCount)
            ld e,a
            ld d,0
            ld a,80
            ld hl,aBankPages
            call pip_printNumber
            ld a,MEM_OP_RAMDISK
            ld c,MEM_RAMDISK_STATUS
            MXOS_CALL MXOS_DOS_MEMORY
            ld a,b
            inc a ; DOS page plus the RAMDISK pool.
            ld e,a
            ld d,0
            push de
            ld a,90
            ld hl,aReservedPages
            call pip_printNumber
            pop de
            ld a,(bios_upper.ramPageCount)
            sub e
            ld e,a
            ld a,100
            ld hl,aFreePages
            call pip_printNumber
            ld b,3
            call bios_diskDriver
            ld (vars.diskInfoPtr),hl
            push hl
            ld bc,DISK_INFO.isValid
            add hl,bc
            ld a,(hl)
            pop hl
            cp DISK_VALID
            jp c,pip_printDiskError
            ld a,1
            ld (pipHasVolume),a
            ld bc,DISK_INFO.blockSize
            add hl,bc
            ld c,(hl)
            inc hl
            ld b,(hl)
            ld h,b
            ld l,c
            call pip_unitsKiB
            ld a,134
            ld hl,aDiskTotal
            call pip_printDiskNumber
            ; Publish the free-space label now; leave its numeric field blank.
            ld a,144
            ld hl,aDiskFree
            call pip_printDiskLabel
            jp pip_printDriver
pip_printDiskError:
            ld hl,048Bh ; One error row centered in the disk block.
            call setCursorPosPanel
            COLOR COLOR_INFOTEXT
            ld hl,aDrive
            call ncPrintPanelText
            call printCurDrive
            ld hl,aNotFormatted
            call ncPrintPanelText
pip_printDriver:
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
            ld a,(pipHasVolume)
            add a,2 ; Fixed rows: optional Label, Driver, Type.
            ld c,a
            ld d,0
pip_descriptionCount:
            ld a,(hl)
            or a
            jp z,pip_descriptionCounted
            ld a,c
            cp 6 ; At most six rows fit between the divider and lower border.
            jp z,pip_descriptionCounted
            inc c
            inc d
            ld a,(pipDescriptionChars)
            ld b,a
pip_descriptionCountLine:
            ld a,(hl)
            or a
            jp z,pip_descriptionCounted
            inc hl
            dec b
            jp nz,pip_descriptionCountLine
            jp pip_descriptionCount
pip_descriptionCounted:
            ld a,c
            ld (pipRows),a
            ld a,d
            ld (pipDescriptionRows),a
pip_metadataCounted:
            ; Region Y=161..226: center fixed fields and wrapped description.
            ; First baseline = 202 - 5 * row count (glyphs occupy Y-7..Y).
            ld a,(pipRows)
            ld b,a
            add a,a
            add a,a
            add a,b
            ld b,a
            ld a,202
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
            ld de,aDriverUnknown
pip_driverType:
            ld hl,aDiskTechnology
            call pip_printDetail
            ld hl,(pipDriverDescription)
            ld a,(pipDescriptionRows)
            or a
            jp z,pip_freeSpaceLast
            ld d,a
pip_descriptionPrint:
            call pip_nextInfoRow
            COLOR COLOR_INFONUMBER
            ld a,(pipDescriptionChars)
            ld b,a
            call ncPrintTextLim
            dec d
            jp nz,pip_descriptionPrint
pip_freeSpaceLast:
            ; No other screen text is delayed by this potentially slow request.
            ld a,(ncInfoFreeDeferred)
            or a
            ret nz
            ld a,(pipHasVolume)
            or a
            ret z
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
            ld a,144
            jp pip_printNumberOnly
pip_volumeLabel:
            ld hl,(vars.diskInfoPtr)
            ld de,DISK_INFO.volumeLabel
            add hl,de
            ret
; HL=label -> A=width in BIOS half-pixels; BC/HL destroyed.
pip_labelWidth:
            ld b,0
pip_countLabel:
            ld a,(hl)
            inc hl
            or a
            jp z,pip_countLabelEnd
            inc b
            jp pip_countLabel
pip_countLabelEnd:
            ld a,b
            jp ncTextAdvance
pipHasVolume: DB 0
pipRows: DB 0
pipDetailX: DB 0
pipDetailChars: DB 0
pipDescriptionChars: DB 0
pipDescriptionRows: DB 0
ncInfoDrive: DB 0FFh
pipDriverTech: DB 0
pipDriverName: DW 0
pipDriverDescription: DW 0
ncInfoFreeDeferred: DB 0

; Sequential metadata rows; only the description can occupy several rows.
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

; A=Y, DE=number, HL=label. Number X=8..55, label X=64, byte-aligned.
; Field width 12/8/6 chars for 4/6/8px; leading spaces right-align digits.
pip_printNumber:
            push hl
            push af
            call pip_printNumberOnly
            pop af
            pop hl
pip_printLabel:
            push hl
            ld l,a
            ld h,32
            call setCursorPosPanel
            pop hl
            COLOR COLOR_INFOTEXT
            jp ncPrintPanelText
pip_printDiskLabel:
            call pip_printLabel
            jp printCurDrive
pip_printNumberOnly:
            ld l,a
            ld h,4
            call setCursorPosPanel
            ld b,' '
            push de
            call ncFormatDecimal
            pop de
            ld a,(ncFontWidth)
            ld b,6
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
            call printSpaces
            ld hl,ncDecimalText
            jp bios_printStringReal
pip_printDiskNumber:
            call pip_printNumber
            jp printCurDrive ; Same color as the label, no color seam.

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
            call pip_labelWidth
            ld b,a
            ld a,(pipDetailX)
            sub b
            ld hl,(bios_vars.cursorY)
            ld h,a
            call setCursorPosPanel
            pop hl
            pop de
            COLOR COLOR_INFOTEXT
            call ncPrintPanelText
            ex de,hl
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
