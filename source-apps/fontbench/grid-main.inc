    INCLUDE "../../source/common/system.inc"
    INCLUDE "../../bin/FONTBEN.bios.entries.inc"
    DEFINE FONT_NATIVE_GRID
    INCLUDE "grid-corpus.inc"
GRID_MIN_MAIN_TOP = 035FFh ; Нижний предел memTop для буфера отчёта в MAIN.
GRID_ROUTE_COUNT  = 5
GRID_FULL_STAGES  = 15
GRID_SCALAR_STAGES = 6
IMAGE_FLAGS      = IMAGE_FLAG_MAIN
    ORG 0
entry:
    push de
    call gridSetModuleDrive
    call bench_init
    pop hl
    jp c,bench_restore
    call gridArgs
    jp c,gridExit
    ld a,(gridPseudoOnly)
    or a
    call nz,gridPseudoSetup
    ld hl,(enginePreFirst)
    ld (gridRequestedPre),hl
    call bios_getMemTop
    ld de,GRID_MIN_MAIN_TOP
    call bios_cmp_hl_de
    jp c,gridError
    call engineSave
    ld a,(gridPhase)
    inc a
    jp nz,gridPreparePhase
    xor a
    ld (gridText+GRID_TEXT_BYTES),a
    jp gridCorpusReady
gridPreparePhase:
    ld hl,gridText+GRID_GLYPHS
    ld de,GRID_STRIDE
    ld b,GRID_PHASE_ROWS
    xor a
gridTerminatePhaseRow:
    ld (hl),a
    add hl,de
    dec b
    jp nz,gridTerminatePhaseRow
gridCorpusReady:
    ld a,(gridPseudoOnly)
    or a
    call nz,gridPseudoCorpus
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
    jp z,gridEngineAllowed
    call engineRecord
    ld de,7
    add hl,de
    ld a,(hl)
    and 80h
    jp z,gridInverseError
gridEngineAllowed:
    ld hl,(gridRequestedPre)
    ld (enginePreFirst),hl
    ld a,(engineId)
    or a
    jp nz,gridSharedMode
    ; BIOS means the installed module, not a mutable legacy render flag.
    ld hl,(FONT_ENGINE_HEADER+12)
    ld a,h
    or l
    ld a,0
    jp z,gridNativeMode
    inc a
    jp gridNativeMode
gridSharedMode:
    call engineRecord
    ld de,7
    add hl,de
    ld a,(hl)
    and 80h
    jp z,gridEngineModeReady
    ld de,-5
    add hl,de
    ld a,(hl)
    and 1
gridNativeMode:
    ld (enginePreFirst),a
    inc a
    ld (enginePreEnd),a
gridEngineModeReady:
    call engineRecord
    ld de,6
    add hl,de
    ld a,(engineId)
    or a
    ld a,(hl)
    jp nz,gridWidthReady
    ld a,(FONT_ENGINE_HEADER+5)
gridWidthReady:
    ld (engineWidth),a
    ld b,a
    ld a,(gridPseudoOnly)
    or a
    ld a,b
    jp nz,gridWidthAccepted
    ld a,(gridPhase)
    inc a
    ld a,b
    jp z,gridWidthAccepted
    cp 8
    jp nz,gridError
gridWidthAccepted:
    add a,'0'
    ld (gridTitleWidth),a
    ld (gridTotalWidth),a
    ld a,(enginePreEnd)
    ld b,a
    ld a,(enginePreFirst)
    sub b
    cp 0FEh
    ld hl,03033h ; '30' little endian
    jp z,gridTotalReady
    ld hl,03531h ; '15'
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
    ld a,(gridPseudoOnly)
    or a
    jp nz,gridTotalPseudo
    ld a,(gridPhase)
    inc a
    jp z,gridTotalRegular
    ld hl,03530h
    ld a,(gridScalarOnly)
    or a
    jp z,gridTotalRegular
    ld h,'2'
    jp gridTotalRegular
gridTotalPseudo:
    ld hl,03230h ; '02'
gridTotalRegular:
    ld (gridTotal+1),hl
    ld hl,gridResults
    ld bc,gridResultsEnd-gridResults
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
    call gridFirstKind
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
    ld hl,00F7h ; row 240: engine + stage; both lines fit 48 cells at 8x8
    ld (bios_vars.cursorY),hl
    call engineName
    call puts
    ld hl,gridProgress
    call puts
    ld a,(gridStage)
    call gridByteNumber
    ld hl,gridTotal
    call puts
    ; Status is outside the timer. Do not wrap at the physical bottom:
    ; wrapping would scroll the test image even though timing is finished.
    ld hl,00FFh ; row 248: phase/API/transport/time
    ld (bios_vars.cursorY),hl
    call gridPrintLabel
    call gridPrintRoute
    ld hl,v_elapsed
    call ticks_to_ms
    call print_number
    ld hl,gridMs
    call puts
gridProgressDone:
    ld a,(gridPhase)
    inc a
    jp nz,gridNextKind
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
    cp GRID_ROUTE_COUNT
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
    ld a,(gridPseudoOnly)
    or a
    jp nz,gridPseudoNext
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
    jp gridKindsDone
gridPseudoNext:
    ld a,(hl)
    cp 6
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
    ; Quick mode leaves the last timing on the bottom line.
    ld a,(bios_vars.cursorX)
    or a
    ld hl,gridCRLF
    call nz,puts
    jp bench_restore
; Direct MAIN ABI calls: no transport in the measured interval.
gridMainRun:
    ld a,(gridPhase)
    inc a
    jp nz,gridPhaseRun
    ld hl,gridText
    ld bc,GRID_TEXT_BYTES
    ld a,(gridKind)
    cp 2
    jp z,bios_printStringZ
    cp 3
    jp z,bios_printStringRawZ
    cp 4
    jp z,bios_printStringRawLim
    or a
    jp nz,gridMainRaw
    ld b,GRID_TEXT_BYTES/GRID_UNROLL ; B/HL сохраняются самим API.
gridMainChar:
    DUP GRID_UNROLL
    ld c,(hl)
    call bios_printChar
    inc hl
    EDUP
    dec b
    jp nz,gridMainChar
    ret
gridMainRaw:
    ld b,GRID_TEXT_BYTES/GRID_UNROLL
gridMainRawLoop:
    DUP GRID_UNROLL
    ld a,(hl)
    call bios_printCharRaw
    inc hl
    EDUP
    dec b
    jp nz,gridMainRawLoop
    ret
    INCLUDE "grid-phase.inc"
;----------------------------------------------------------------------------
; MXOS
; Загрузить исполнителя в отдельный EXEC, вызвать его и освободить банк.
; вход:
;   gridControl+4/+5 = вид API и способ вызова, gridControl+8 = буфер MAIN,
;   gridText содержит GRID_TEXT_BYTES и завершающий нуль.
; выход:
;   CF=0, gridControl+0/+2 = младшее/старшее слово времени. При отказе CF=1,
;   A = код ошибки DOS либо DLL. BC, DE, HL и прочие флаги разрушаются.
;----------------------------------------------------------------------------
gridInvoke:
    ld hl,gridBankPath
    ld a,MODULE_TARGET_BANK
    MXOS_CALL_REGS MXOS_DOS_MODULE_LOAD
    ret c
    push hl
    ld hl,gridText
    ld (gridControl),hl
    ld hl,(gridControl+8)
    ld (gridControl+2),hl
    pop hl
    push hl
    ld de,gridControl
    MXOS_CALL_REGS MXOS_DOS_MODULE_CALL
    jp c,gridReleaseBank
    ld hl,(gridControl+6)
    push hl
    pop af
gridReleaseBank:
    pop hl
    push af
    xor a
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
    jp c,gridReleaseError
    pop af
    ret
gridReleaseError:
    pop hl
    ret
; Адрес = gridResults + ((pre*5+kind)*5+route)*4.
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
    ld b,a
    add a,a
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
    ld a,(gridPhase)
    inc a
    ld hl,gridPhaseTitle
    call nz,puts
    ld a,(gridPhase)
    inc a
    ld hl,gridTitleWidth
    jp nz,gridTableTitleReady
    ld hl,gridTitle
gridTableTitleReady:
    call puts
    ld a,(enginePreFirst)
    ld (gridPre),a
gridTablePre:
    call gridFirstKind
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
    ld a,(gridPseudoOnly)
    or a
    jp nz,gridTableLineEnd
    ld hl,gridRoute
    inc (hl)
    ld a,(hl)
    cp GRID_ROUTE_COUNT
    jp c,gridTableValue
gridTableLineEnd:
    ld hl,gridCRLF
    call puts
    ld hl,gridKind
    inc (hl)
    ld a,(gridPseudoOnly)
    or a
    jp nz,gridTablePseudoNext
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
    jp gridTableKindsDone
gridTablePseudoNext:
    ld a,(hl)
    cp 6
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
    ld hl,(gridControl+8)
    ld (gridWrite),hl
    ld a,1
    ld (gridCapture),a
    call gridTable
    xor a
    ld (gridCapture),a
    ld e,0
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    ld (gridDrive),a
    ld c,MEM_RAMDISK_STATUS
    ld a,MEM_OP_RAMDISK
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ret c
    ld a,e
    MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
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
    MXOS_CALL_REGS MXOS_DOS_FILEHANDLE
    jp c,gridRestoreDrive
    ld hl,(gridControl+8)
    ex de,hl
    ld hl,(gridWrite)
    call bios_sub_hl_de
    ld (gridHandle+FH_COUNT),hl
    ld hl,(gridControl+8)
    ld (gridHandle+FH_BUFFER),hl
    ld hl,gridHandle
    ld a,FILE_OP_WRITE
    MXOS_CALL_REGS MXOS_DOS_FILEHANDLE
    push af
    ld hl,gridHandle
    ld a,FILE_OP_CLOSE
    MXOS_CALL_REGS MXOS_DOS_FILEHANDLE
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
    MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
    ret
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
    cp 'X'
    jp z,gridArgX
    cp 'R'
    jp z,gridArgR
    jp gridUsage
gridArgR:
    ld a,1
    ld (gridPseudoOnly),a
    jp gridArgsNext
gridArgX:
    ld a,(hl)
    sub '0'
    cp 4
    jp nc,gridUsage
    ld (gridPhase),a
    add a,a
    add a,'0'
    ld (gridPhasePixel),a
    inc hl
    jp gridArgsNext
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
    ld hl,engineRecords
    ld bc,ENGINE_COUNT*256+3
gridHelpEngine:
    ld e,(hl)
    inc hl
    ld d,(hl)
    dec hl
    push hl
    ex de,hl
    call puts
    ld a,' '
    call gridPrintChar
    dec c
    jp nz,gridHelpNext
    ld hl,gridCRLF
    call puts
    ld c,3
gridHelpNext:
    pop hl
    ld de,8
    add hl,de
    dec b
    jp nz,gridHelpEngine
    ld hl,gridCRLF
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
; Companion programs and DLLs stay in BENCH/FontBen on the launch drive.
gridSetModuleDrive:
    call appProgramDrive
    ld (gridBankPath),a
    ld (enginePath),a
    ret
    DEFINE APP_COMMAND_ADDR MAIN_COMMAND_ADDR
    INCLUDE "../../sdk/asm/lib/app-drive.inc"
    INCLUDE "engine-hooks.inc"
    INCLUDE "engine-select.inc"
    INCLUDE "../textbench/standalone.inc"
gridRequestedPre:   DW 0
gridPre:            DB 0
gridKind:           DB 0
gridRoute:          DB 0
gridPhase:          DB 0FFh
gridPseudoOnly:     DB 0
gridStage:          DB 0
gridQuick:          DB 0
gridScalarOnly:     DB 0
gridInverse:        DB 0
gridNoFile:         DB 0
gridCapture:        DB 0
gridDrive:          DB 0
gridWrite:          DW 0
gridFileResult:     DW gridFileSkipped
gridFileSkipped:    DB 'Report disabled',13,10,0
gridFileFailed:     DB 'Cannot save FONTBEN.TXT on RAM disk',13,10,0
gridFileSaved:      DB 'Saved FONTBEN.TXT on RAM disk',13,10,0
gridControl:        DW 0,0,0,08000h,0,gridText
    DB 0,0
gridBankPath: DB 'C:',05Ch,'Bench',05Ch,'FontBen',05Ch,'FontBank.dll',0
    DS MAX_PATH-($-gridBankPath),0
gridResults: DS (2*5+1)*GRID_ROUTE_COUNT*4,0FFh
gridResultsEnd:
gridTitle:          DB 'FontBen: 924 bytes, '
gridTitleWidth:     DB '6','x8, ms '
gridInverseLabel:   DB 'N',13,10
    DB 'P API              MAIN CMD  CMDZ NEAR Far1024',13,10,0
gridProgress:           DB ' Stage ',0
gridTotal:              DB '/30 '
gridTotalWidth:         DB '6','x8 ',0
gridNextText:           DB 'Press Enter to continue',13,10,0
gridMs:                 DB ' ms',0
gridCRLF:               DB 13,10,0
gridDash:               DB '   -',0
gridRouteNames:         DW gridRoute0,gridRoute1,gridRoute2,gridRoute3,gridRoute4
gridRoute0:             DB 'MAIN ',0
gridRoute1:             DB 'BANK CMD ',0
gridRoute2:             DB 'BANK CMD+Z ',0
gridRoute3:             DB 'BANK NEAR ',0
gridRoute4:             DB 'BANK Far1024 ',0
gridNames:              DW gridName0,gridName1,gridName2,gridName3,gridName4,gridName5
gridName0:              DB 'Char             ',0
gridName1:              DB 'CharRaw          ',0
gridName2:              DB 'StringZ          ',0
gridName3:              DB 'StringRawZ       ',0
gridName4:              DB 'StringRawLim     ',0
gridName5:              DB 'StringRawPseudo  ',0
gridErrorText:          DB 13,10,'FontBen failed (file, bank or memory)',13,10,0
gridInverseErrorText:   DB 'Inverse mode requires -E=BIOS',13,10,0
gridHelp:               DB 'FontBen -E=ALL|STANDALONE|LEGACY|name[,name]',13,10
    DB '-X0..3 8x8 phase; -R BIOS pseudo',13,10
    DB '-S scalar; -I inverse; -N no file; -Q quiet',13,10
    DB '-P=OFF|ON|BOTH legacy prerender',13,10
    DB 'See BENCH/FONTBEN/README.TXT',13,10,13,10,0
gridHandle: DB 'FONTBEN TXT'
    DS FH_DRIVE-11,0
    DB 1
    DB FILE_MODE_WRITE | FILE_MODE_CREATE | FILE_MODE_TRUNCATE
    DS FILE_HANDLE-FH_MODE-1,0
; Обычная сетка использует CR/LF между строками. При явном позиционировании
; вместо CR записывается NUL, курсор каждой строки задаётся отдельно.
gridText:
gridCorpusOffset = 0
    DUP GRID_PHASE_ROWS
      INCBIN "corpus.koi8",gridCorpusOffset,GRID_GLYPHS
      DB 13,10
gridCorpusOffset = gridCorpusOffset+GRID_GLYPHS
    EDUP
    DB 0
    ASSERT $-gridText == GRID_PHASE_ROWS*GRID_STRIDE+1
    ASSERT $ <= 02000h
    DS 02000h-$,0
programEnd:
