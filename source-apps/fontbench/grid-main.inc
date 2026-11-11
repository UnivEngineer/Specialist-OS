    INCLUDE "../../source/include/mxos.inc"
    INCLUDE "../../bin/FONTBEN.bios.entries.inc"
    DEFINE FONT_NATIVE_GRID
FONT_CORPUS_SIZE = 1152
textCorpus = 08600h
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 0
entry:
    push de
    call bench_init
    pop hl
    ret c
    call gridArgs
    jp c,gridExit
    call bios_getMemTop
    ld de,textCorpus+FONT_CORPUS_SIZE-1
    call bios_cmp_hl_de
    jp c,gridError
    call engineSave
    call loadCorpus
    jp c,gridError
    ld hl,FONT_ADDR
    ld (bios_upper.charGen),hl
    ld hl,textCorpus
    ld de,gridText
    ld c,24
gridPrepareRow:
    ld b,46
gridPrepareChar:
    ld a,(hl)
    inc hl
    ld (de),a
    inc de
    dec b
    jp nz,gridPrepareChar
    ld a,13
    ld (de),a
    inc de
    ld a,10
    ld (de),a
    inc de
    dec c
    jp nz,gridPrepareRow
    xor a
    ld (de),a
    xor a
    ld (engineId),a
gridEngineNext:
    call engineSelected
    or a
    jp z,gridAdvanceEngine
    ld a,(gridInverse)
    or a
    jp z,gridEngineAllowed
    ld a,(engineId)
    or a
    jp nz,gridInverseError
gridEngineAllowed:
    call engineRecord
    ld de,6
    add hl,de
    ld a,(hl)
    ld (engineWidth),a
    add a,'0'
    ld (gridTitleWidth),a
    ld (gridTotalWidth),a
    ld a,(enginePreEnd)
    ld b,a
    ld a,(enginePreFirst)
    sub b
    cp 0FEh
    ld hl,03633h ; '36' little endian
    jp z,gridTotalReady
    ld hl,03831h ; '18'
gridTotalReady:
    ld a,(gridScalarOnly)
    or a
    jp z,gridTotalStore
    ld a,l
    cp '3'
    ld hl,03231h ; 12 scalar stages for BOTH.
    jp z,gridTotalStore
    ld hl,03630h ; 06 scalar stages for one pre-state.
gridTotalStore:
    ld (gridTotal+1),hl
    ld hl,gridResults
    ld bc,240
gridClearResults:
    ld (hl),0FFh
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,gridClearResults
    xor a
    ld (gridStage),a
    ld a,(enginePreFirst)
    ld (gridPre),a
gridPreNext:
    call engineOpen
    jp c,gridError
    xor a
    ld (gridKind),a
gridKindNext:
    xor a
    ld (gridRoute),a
gridRouteNext:
    call gridClear
    ; Only the native BIOS contract guarantees inversion for every API.
    ld a,(gridInverse)
    ld l,a
    ld h,a
    ld (bios_vars.inverse),hl
    call engineHook
    ld hl,0007h
    ld (bios_vars.cursorY),hl
    ld a,(gridRoute)
    or a
    jp nz,gridBankRun
    call reset_timing
    call gridMainRun
    call timer_sample
    jp gridMeasured
gridBankRun:
    ld a,(gridKind)
    ld (gridControl+4),a
    cp 2
    ld a,(gridRoute)
    jp nc,gridStringRoute
    dec a
    jp gridRouteReady
gridStringRoute:
    sub 3
gridRouteReady:
    ld (gridControl+5),a
    call gridInvoke
    jp c,gridError
    ld hl,(gridControl)
    ld (v_elapsed),hl
    ld hl,(gridControl+2)
    ld (v_elapsed+2),hl
gridMeasured:
    ld hl,0
    ld (bios_vars.inverse),hl
    call engineUnhook
    call gridResultAddress
    ex de,hl
    ld hl,v_elapsed
    ld bc,4
    call gridCopy
    ld hl,gridStage
    inc (hl)
    call gridClearInfo
    ld hl,00F7h ; row 240: engine name, separate from the 64-char status
    ld (bios_vars.cursorY),hl
    call engineName
    call puts
    ld hl,00FFh ; row 248: phase/API/transport/time, never wraps
    ld (bios_vars.cursorY),hl
    ld hl,gridProgress
    call puts
    ld a,(gridStage)
    call gridByteNumber
    ld hl,gridTotal
    call puts
    call gridPrintLabel
    call gridPrintRoute
    ld hl,v_elapsed
    call ticks_to_ms
    call print_number
    ld hl,gridMs
    call puts
gridProgressDone:
    ld a,(gridRoute)
    inc a
    ld b,a
    ld a,(gridKind)
    cp 2
    ld a,b
    jp c,gridScalarNext
    cp 1
    jp nz,gridStringNext
    ld a,3
gridStringNext:
    cp 6
    jp c,gridContinueRoute
    jp gridNextKind
gridScalarNext:
    cp 3
    jp nc,gridNextKind
gridContinueRoute:
    ld (gridRoute),a
    jp gridRouteNext
gridNextKind:
    ld hl,gridKind
    inc (hl)
    ld a,(hl)
    ld b,a
    ld a,(gridScalarOnly)
    or a
    ld a,b
    jp z,gridAllKinds
    cp 2
    jp c,gridKindNext
    jp gridKindsDone
gridAllKinds:
    cp 5
    jp c,gridKindNext
gridKindsDone:
    call engineClose
    ld hl,gridPre
    inc (hl)
    ld a,(enginePreEnd)
    cp (hl)
    jp nz,gridPreNext
    ld a,(gridNoFile)
    or a
    call z,gridSave
    ld a,(gridQuick)
    or a
    jp nz,gridAdvanceEngine
    call bench_clear
    call gridTable
    ld hl,(gridFileResult)
    call puts
    ld hl,gridNextText
    call puts
gridNextKey:
    call bios_getch
    cp 13
    jp nz,gridNextKey
gridAdvanceEngine:
    ld hl,engineId
    inc (hl)
    ld a,(hl)
    cp ENGINE_COUNT
    jp c,gridEngineNext
    jp gridExit
gridInverseError:
    ld hl,gridInverseErrorText
    call puts
    jp gridExit
gridError:
    call engineClose
    ld hl,gridErrorText
    call puts
gridExit:
    ld hl,0
    ld (bios_vars.inverse),hl
    call engineUnhook
    jp bench_restore
; Direct MAIN ABI calls: no transport in the measured interval.
gridMainRun:
    ld hl,gridText
    ld bc,1152
    ld a,(gridKind)
    cp 2
    jp z,bios_printStringZ
    cp 3
    jp z,bios_printStringRawZ
    cp 4
    jp z,bios_printStringRawLim
    or a
    jp nz,gridMainRaw
    ld b,72 ; 72 * 16 = 1152, B/HL сохраняются самим API.
gridMainChar:
    DUP 16
    ld c,(hl)
    call bios_printChar
    inc hl
    EDUP
    dec b
    jp nz,gridMainChar
    ret
gridMainRaw:
    ld b,72
gridMainRawLoop:
    DUP 16
    ld a,(hl)
    call bios_printCharRaw
    inc hl
    EDUP
    dec b
    jp nz,gridMainRawLoop
    ret
; One separately loaded native bank; loading and corpus copy are not timed.
gridInvoke:
    ld a,080h
    ld (gridControl+7),a
    ld hl,gridControl
    ld de,gridArgument
    ld b,4
gridHex:
    ld a,h
    rrca
    rrca
    rrca
    rrca
    and 15
    cp 10
    jp c,gridHexDigit
    add a,7
gridHexDigit:
    add a,'0'
    ld (de),a
    inc de
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    dec b
    jp nz,gridHex
    ld hl,gridBankPath
    MXOS_CALL MXOS_DOS_FILE_EXEC_NATIVE
    ret c
    ld a,(gridControl+6)
    rra
    ret
; Address = results + ((pre*5+kind)*6+route)*4.
gridResultAddress:
    ld a,(gridPre)
    add a,a
    add a,a
    ld b,a
    ld a,(gridPre)
    add a,b
    ld b,a
    ld a,(gridKind)
    add a,b
    add a,a
    ld b,a
    add a,a
    add a,b
    ld b,a
    ld a,(gridRoute)
    add a,b
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    ld de,gridResults
    add hl,de
    ret
gridClear:
    ld hl,09000h
    ld d,48
    xor a
gridClearColumn:
    ld l,0
    ld b,240
gridClearRow:
    ld (hl),a
    inc l
    dec b
    jp nz,gridClearRow
    inc h
    dec d
    jp nz,gridClearColumn
    ret
gridClearInfo:
    ld hl,090F0h
    ld d,48
    xor a
gridInfoColumn:
    ld l,240
    ld b,16
gridInfoRow:
    ld (hl),a
    inc l
    dec b
    jp nz,gridInfoRow
    inc h
    dec d
    jp nz,gridInfoColumn
    ret
gridByteNumber:
    ld l,a
    ld h,0
    ld (v_number),hl
    ld hl,0
    ld (v_number+2),hl
    jp print_number
gridPrintLabel:
    ld a,(gridPre)
    add a,'0'
    call gridPrintChar
    ld a,' '
    call gridPrintChar
    ld a,(gridKind)
    add a,a
    ld e,a
    ld d,0
    ld hl,gridNames
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    jp puts
gridPrintRoute:
    ld a,(gridRoute)
    add a,a
    ld e,a
    ld d,0
    ld hl,gridRouteNames
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    jp puts
gridTable:
    call engineName
    call puts
    ld hl,gridCRLF
    call puts
    ld hl,gridTitle
    call puts
    ld a,(enginePreFirst)
    ld (gridPre),a
gridTablePre:
    xor a
    ld (gridKind),a
gridTableKind:
    call gridPrintLabel
    xor a
    ld (gridRoute),a
gridTableValue:
    call gridResultAddress
    push hl
    ld b,4
gridCheckMissing:
    ld a,(hl)
    inc a
    jp nz,gridPresent
    inc hl
    dec b
    jp nz,gridCheckMissing
    pop hl
    jp gridMissing
gridPresent:
    pop hl
    call ticks_to_ms
    call print_number
    jp gridCellDone
gridMissing:
    ld hl,gridDash
    call puts
gridCellDone:
    ld a,' '
    call gridPrintChar
    ld hl,gridRoute
    inc (hl)
    ld a,(hl)
    cp 6
    jp c,gridTableValue
    ld hl,gridCRLF
    call puts
    ld hl,gridKind
    inc (hl)
    ld a,(hl)
    ld b,a
    ld a,(gridScalarOnly)
    or a
    ld a,b
    jp z,gridTableAllKinds
    cp 2
    jp c,gridTableKind
    jp gridTableKindsDone
gridTableAllKinds:
    cp 5
    jp c,gridTableKind
gridTableKindsDone:
    ld hl,gridPre
    inc (hl)
    ld a,(enginePreEnd)
    cp (hl)
    jp nz,gridTablePre
    ret
gridPuts:
    push af
gridPutsLoop:
    ld a,(hl)
    or a
    jp z,gridPutsDone
    call gridPrintChar
    inc hl
    jp gridPutsLoop
gridPutsDone:
    pop af
    ret
gridPrintChar:
    push af
    push bc
    push de
    push hl
    ld c,a
    ld a,(gridCapture)
    or a
    jp z,gridScreenChar
    ld hl,(gridWrite)
    ld (hl),c
    inc hl
    ld (gridWrite),hl
    jp gridCharDone
gridScreenChar:
    call bios_printChar
gridCharDone:
    pop hl
    pop de
    pop bc
    pop af
    ret
gridSave:
    ld hl,gridFileFailed
    ld (gridFileResult),hl
    ld hl,07C00h
    ld (gridWrite),hl
    ld a,1
    ld (gridCapture),a
    call gridTable
    xor a
    ld (gridCapture),a
    ld e,0
    call bios_fileGetSetDrive
    ld (gridDrive),a
    ld a,1
    call bios_fileSelectDrive
    ret c
    ld a,(engineFileAppend)
    or a
    ld a,FILE_MODE_WRITE | FILE_MODE_CREATE | FILE_MODE_TRUNCATE
    jp z,gridFileMode
    ld a,FILE_MODE_WRITE | FILE_MODE_CREATE | FILE_MODE_APPEND
gridFileMode:
    ld (gridHandle+FH_MODE),a
    ld a,FILE_OP_OPEN
    ld hl,gridHandle
    call bios_fileHandle
    jp c,gridRestoreDrive
    ld hl,(gridWrite)
    ld de,07C00h
    call bios_sub_hl_de
    ld (gridHandle+FH_COUNT),hl
    ld hl,07C00h
    ld (gridHandle+FH_BUFFER),hl
    ld hl,gridHandle
    ld a,FILE_OP_WRITE
    call bios_fileHandle
    push af
    ld hl,gridHandle
    ld a,FILE_OP_CLOSE
    call bios_fileHandle
    jp c,gridCloseFailed
    pop af
    jp c,gridRestoreDrive
    ld a,1
    ld (engineFileAppend),a
    ld hl,gridFileSaved
    ld (gridFileResult),hl
    jp gridRestoreDrive
gridCloseFailed:
    pop af
gridRestoreDrive:
    ld a,(gridDrive)
    jp bios_fileSelectDrive
gridArgs:
    ld a,h
    or l
    ret z
gridArgsNext:
    ld a,(hl)
    inc hl
    or a
    ret z
    cp ' '
    jp z,gridArgsNext
    cp '-'
    jp nz,gridUsage
    ld a,(hl)
    inc hl
    and 0DFh
    cp 'E'
    jp z,gridArgE
    cp 'P'
    jp z,gridArgP
    cp 'S'
    jp z,gridArgS
    cp 'I'
    jp z,gridArgI
    cp 'N'
    jp z,gridArgN
    cp 'Q'
    jp z,gridArgQ
    cp 'K'
    jp z,gridArgK
    cp 'G'
    jp z,gridArgsNext
    jp gridUsage
gridArgS:
    ld a,1
    ld (gridScalarOnly),a
    jp gridArgsNext
gridArgI:
    ld a,0FFh
    ld (gridInverse),a
    ld a,'I'
    ld (gridInverseLabel),a
    jp gridArgsNext
gridArgN:
    ld a,1
    ld (gridNoFile),a
    jp gridArgsNext
gridArgQ:
    ld a,1
    ld (gridQuick),a
    jp gridArgsNext
gridUsage:
    ld hl,gridHelp
    call puts
    scf
    ret
    INCLUDE "engine-args.inc"
gridArgK:
    push hl
    xor a
    call engineSelectAll
    ld a,1
    ld (engineRecords+5),a
    pop hl
    jp gridArgsNext
gridCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,gridCopy
    ret
    INCLUDE "engine-hooks.inc"
    INCLUDE "engine-select.inc"
    INCLUDE "../textbench/standalone.inc"
    INCLUDE "corpus.inc"
gridPre: DB 0
gridKind: DB 0
gridRoute: DB 0
gridStage: DB 0
gridQuick: DB 0
gridScalarOnly: DB 0
gridInverse: DB 0
gridNoFile: DB 0
gridCapture: DB 0
gridDrive: DB 0
gridWrite: DW 0
gridFileResult: DW gridFileSkipped
gridFileSkipped: DB 'Report disabled',13,10,0
gridFileFailed: DB 'Cannot save B:\FONTBEN.TXT',13,10,0
gridFileSaved: DB 'Saved B:\FONTBEN.TXT',13,10,0
gridControl: DW 0,0,0,08000h,0,gridText
    DB 0,0
gridBankPath: DB 'A:',05Ch,'Bench',05Ch,'FontBen',05Ch,'FontBank.com '
gridArgument: DB '0000',0
    DS MAX_PATH-($-gridBankPath),0
gridResults: DS 240,0FFh
gridTitle: DB 'FontBen: 1152 bytes, '
gridTitleWidth: DB '6','x8, ms '
gridInverseLabel: DB 'N',13,10
    DB 'P API              MAIN CMD  TEXT W20  F256 USER',13,10,0
gridProgress: DB 'Stage ',0
gridTotal: DB '/36 '
gridTotalWidth: DB '6','x8 ',0
gridNextText: DB 'Press Enter to continue',13,10,0
gridMs: DB ' ms    ',0
gridCRLF: DB 13,10,0
gridDash: DB '   -',0
gridRouteNames: DW gridRoute0,gridRoute1,gridRoute2,gridRoute3,gridRoute4,gridRoute5
gridRoute0: DB 'MAIN ',0
gridRoute1: DB 'BANK CMD ',0
gridRoute2: DB 'BANK TEXT ',0
gridRoute3: DB 'BANK Win20 ',0
gridRoute4: DB 'BANK Far256 ',0
gridRoute5: DB 'BANK Own ',0
gridNames: DW gridName0,gridName1,gridName2,gridName3,gridName4
gridName0: DB 'Char             ',0
gridName1: DB 'CharRaw          ',0
gridName2: DB 'StringZ          ',0
gridName3: DB 'StringRawZ       ',0
gridName4: DB 'StringRawLim     ',0
gridErrorText: DB 13,10,'FontBen failed (file, bank or FONT_RENDER)',13,10,0
gridInverseErrorText: DB 'Inverse mode requires -E=BIOS',13,10,0
gridHelp: DB 'FontBen -E=BIOS|ALL|STANDALONE|name[,name]',13,10
    DB '-S scalar only; -I inverse (BIOS only)',13,10
    DB '-P=OFF|ON|BOTH  -N no file  -Q no table/Enter',13,10
    DB 'MXOS1 OPT FULL CACHE QUAD COLD WARM ROTATE',13,10
    DB 'MON2 RAMFOS ROTLINE FULLFAST COLPRE',13,10
    DB 'HALF PACKED PAGED REMAP32 BLOCK32 8X8 4RAW 4PACK 4DUP',13,10,0
gridHandle: DB 'FONTBEN TXT'
    DS FH_DRIVE-11,0
    DB 1
    DB FILE_MODE_WRITE | FILE_MODE_CREATE | FILE_MODE_TRUNCATE
    DS FILE_HANDLE-FH_MODE-1,0
gridText: DS 1201,0
    ASSERT $ <= 02000h
    DS 02000h-$,0
programEnd:
