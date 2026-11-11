; Independent MAIN index-sort benchmark. DOS and output are outside PIT intervals.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
PIT_HZ = 2000000
    DEFINE SORTBEN_STANDALONE
    ORG 0
    jp start
    MODULE raw
PREUPPER = 0
    INCLUDE "build/raw-kernels.inc"
    ENDMODULE
    MODULE upper
PREUPPER = 1
    INCLUDE "build/upper-kernels.inc"
    ENDMODULE

start:
    ld hl,0
    add hl,sp
    ld (savedSP),hl
    call bios_getMemTop
    ld de,08AFFh
    call math.sub_hl_de
    jp c,noMemory
    ld sp,08B00h
    ld hl,REPORT
    ld (reportEnd),hl
    xor a
    ld (failed),a
    call bios_setFontEncoding
    ld a,FONT_RENDER_DISABLE
    MXOS_CALL MXOS_BIOS_FONT_RENDER
    ld hl,clearScreen
    call bios_printStringZ
    ld hl,title
    call output
    ld hl,sourcePrompt
    call output
chooseSource:
    call bios_getch
    cp 01Bh
    jp z,exit
    and 0DFh
    cp 'H'
    jp z,sourceH
    cp 'S'
    jp z,sourceS
    cp 13
    jp nz,chooseSource
sourceS:
    xor a
    jp sourceChosen
sourceH:
    ld a,1
sourceChosen:
    ld (sourceKind),a
    ld hl,sizePrompt
    call output
chooseSize:
    call bios_getch
    cp 01Bh
    jp z,exit
    cp 13
    ld b,128
    jp z,sizeChosen
    sub '1'
    cp 5
    jp nc,chooseSize
    ld e,a
    ld d,0
    ld hl,sizes
    add hl,de
    ld b,(hl)
sizeChosen:
    ld a,b
    ld (count),a
    ld a,(sourceKind)
    or a
    jp nz,readSourceH
    call generate
    jp sourceReady
readSourceH:
    call loadH
sourceReady:
    jp c,fail
    ld a,(count)
    or a
    jp z,fail
    call checksum
    ld (sourceSum),hl
    ld hl,crlf
    call output
    ld hl,syntheticText
    ld a,(sourceKind)
    or a
    jp z,sourceDescription
    ld hl,hRootText
sourceDescription:
    call output
    ld hl,countText
    call output
    ld a,(count)
    ld l,a
    ld h,0
    call number
    ld hl,crlf
    call output
    ld a,(count)
    ld (raw.state.panelA_filesCnt),a
    ld (upper.state.panelA_filesCnt),a
    ld a,1
    ld (mode),a
nextMode:
    ld (raw.state.panelA_sort),a
    ld (upper.state.panelA_sort),a
    ld hl,nameTitle
    cp 1
    jp z,modeTitle
    ld hl,extTitle
modeTitle:
    call output
    ; Prepare normalized rows once. Its time is separate from comparisons.
    ld a,1
    ld (raw.benchDivisorHigh),a
    ld hl,raw.benchPrepare
    ld (raw.benchPrepareInvoke+1),hl
    call raw.benchPrepareRun
    ld hl,upperPrepText
    call output
    ld hl,(raw.benchElapsed)
    ld a,1
    call milliseconds
    ld hl,crlf
    call output
    ; Build a stable radix reference and time radix preparation separately.
    ld hl,raw.benchOptimizedPrepare
    ld (raw.benchPrepareInvoke+1),hl
    call raw.benchPrepareRun
    ld hl,radixPrepText
    call output
    ld hl,(raw.benchElapsed)
    ld a,1
    call milliseconds
    ld hl,crlf
    call output
    ld hl,raw.ncRadixSort
    ld (raw.benchInvoke+1),hl
    call raw.benchRun
    ld hl,(raw.benchElapsed)
    ld (radixTicks),hl
    ld hl,raw.FILE_INDEX_A
    ld de,REFERENCE
    ld b,0
copyReference:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,copyReference
    ld hl,rows
    ld (rowCursor),hl
nextRow:
    ld hl,(rowCursor)
    ld a,(hl)
    cp 0FFh
    jp z,showRadix
    ld (rowUpper),a
    inc hl
    ld a,(hl)
    ld (divisor),a
    ld (raw.benchDivisorHigh),a
    ld (upper.benchDivisorHigh),a
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push de
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld (rowCursor),hl
    ex de,hl
    call output
    pop hl
    ld a,(rowUpper)
    or a
    jp nz,runUpper
    ld (raw.benchInvoke+1),hl
    call raw.benchRun
    ld hl,(raw.benchElapsed)
    jp rowFinished
runUpper:
    ld (upper.benchInvoke+1),hl
    call upper.benchRun
    ld hl,(upper.benchElapsed)
rowFinished:
    ld (rowTicks),hl
    call verify
    jp c,fail
    ld hl,(rowTicks)
    ld a,(divisor)
    call milliseconds
    ld hl,okText
    call output
    jp nextRow
showRadix:
    ld hl,radixText
    call output
    ld hl,(radixTicks)
    ld a,1
    call milliseconds
    ld hl,okText
    call output
    ld a,(mode)
    cp 1
    jp nz,advanceMode
    ld hl,betweenText
    call bios_printStringZ
waitBetweenTables:
    call bios_getch
    cp 13
    jp nz,waitBetweenTables
advanceMode:
    ld a,(mode)
    inc a
    ld (mode),a
    cp 3
    jp c,nextMode
    call checksum
    push hl
    ld hl,(sourceSum)
    ex de,hl
    pop hl
    call math.sub_hl_de
    ld a,h
    or l
    jp nz,fail
    ld hl,doneText
    call output
    jp save
fail:
    ld a,1
    ld (failed),a
    ld hl,failText
    call output
save:
    ld hl,(reportEnd)
    ld de,REPORT
    call math.sub_hl_de
    ld (saveIO+FILE_PATH_IO.count),hl
    ld hl,saveIO
    MXOS_CALL MXOS_DOS_FILE_SAVE_PATH
    ld hl,savedText
    jp nc,saveMessage
    ld hl,saveErrorText
saveMessage:
    call bios_printStringZ
    ld hl,waitText
    call bios_printStringZ
waitForExit:
    call bios_getch
exit:
    ld hl,(savedSP)
    ld sp,hl
    ret
noMemory:
    ld hl,noMemoryText
    call bios_printStringZ
    jp exit

; DOS selection, root context and enumeration are deliberately not timed.
loadH:
    ld e,0
    MXOS_CALL MXOS_DOS_FILE_GET_SET_DRIVE
    ld (oldDrive),a
    ld a,7
    ld e,1
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    jp c,loadDriveError
    ld a,DIR_OP_GET
    MXOS_CALL MXOS_DOS_DIRECTORY
    jp c,loadDriveError
    ex de,hl
    ld (oldDirectory),hl
    ld de,0
    ld a,DIR_OP_SET
    MXOS_CALL MXOS_DOS_DIRECTORY
    jp c,loadDirectoryError
    ld hl,raw.benchRaw
    ld bc,0
    ld a,(count)
    ld e,a
    ld d,0
    ld a,DIR_OP_LIST
    MXOS_CALL MXOS_DOS_DIRECTORY
    push af
    ld a,d
    or a
    jp nz,loadRestore
    ld a,(count)
    cp e
    jp c,loadRestore
    ld a,e
    ld (count),a
loadRestore:
    ld hl,(oldDirectory)
    ex de,hl
    ld a,DIR_OP_SET
    MXOS_CALL MXOS_DOS_DIRECTORY
    ld a,(oldDrive)
    ld e,1
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    pop af
    ret
loadDirectoryError:
    push af
    jp loadRestore
loadDriveError:
    push af
    ld a,(oldDrive)
    ld e,1
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    pop af
    ret

; Fixed-width names, four extensions, directories every ninth item, then LFSR.
generate:
    ld hl,raw.benchRaw
    ld a,(count)
    ld b,a
    ld c,0
    ld e,0
generateRow:
    push bc
    push de
    push hl
    ld de,baseName
    ld b,8
generateName:
    ld a,(de)
    ld (hl),a
    inc de
    inc hl
    dec b
    jp nz,generateName
    pop hl
    push hl
    ld de,6
    add hl,de
    ld a,c
    rrca
    rrca
    rrca
    rrca
    and 15
    call hexDigit
    ld (hl),a
    inc hl
    ld a,c
    and 15
    call hexDigit
    ld (hl),a
    inc hl
    ld a,c
    and 3
    ld e,a
    add a,a
    add a,e
    ld e,a
    ld d,0
    push hl
    ld hl,extensions
    add hl,de
    ex de,hl
    pop hl
    ld b,3
generateExtension:
    ld a,(de)
    ld (hl),a
    inc hl
    inc de
    dec b
    jp nz,generateExtension
    pop hl
    ld de,11
    add hl,de
    pop de
    ld a,e
    or a
    ld a,0
    jp nz,generateAttribute
    ld a,16
generateAttribute:
    ld (hl),a
    inc hl
    xor a
    ld (hl),a
    inc hl
    ld (hl),a
    inc hl
    ld (hl),a
    inc hl
    ld (hl),a
    inc hl
    inc e
    ld a,e
    cp 9
    jp c,generateModulo
    ld e,0
generateModulo:
    pop bc
    inc c
    dec b
    jp nz,generateRow
    ld a,05Ah
    ld (random),a
    ld c,0
shuffle:
    ld a,(random)
    add a,a
    jp nc,shuffleRandom
    xor 01Dh
shuffleRandom:
    ld (random),a
    ld hl,count
shuffleModulo:
    cp (hl)
    jp c,shuffleIndex
    sub (hl)
    jp shuffleModulo
shuffleIndex:
    call rawPointer
    push hl
    ld a,c
    call rawPointer
    pop de
    ld b,16
shuffleByte:
    ld a,(de)
    push af
    ld a,(hl)
    ld (de),a
    pop af
    ld (hl),a
    inc hl
    inc de
    dec b
    jp nz,shuffleByte
    inc c
    ld a,(count)
    cp c
    jp nz,shuffle
    or a
    ret
rawPointer:
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    ld de,raw.benchRaw
    add hl,de
    ret
hexDigit:
    add a,'0'
    cp '9'+1
    ret c
    add a,7
    ret

verify:
    ld hl,raw.FILE_INDEX_A
    ld de,REFERENCE
    ld a,(count)
    ld b,a
verifyByte:
    ld a,(de)
    cp (hl)
    scf
    ret nz
    inc hl
    inc de
    dec b
    jp nz,verifyByte
    ld a,(hl)
    cp 0FFh
    scf
    ret nz
    or a
    ret
checksum:
    ld a,(count)
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    ld b,h
    ld c,l
    ld hl,0
    ld de,raw.benchRaw
checksumByte:
    ld a,(de)
    add a,l
    ld l,a
    jp nc,checksumNoCarry
    inc h
checksumNoCarry:
    inc de
    dec bc
    ld a,b
    or c
    jp nz,checksumByte
    ret

output:
    push hl
    push hl
    ld hl,(reportEnd)
    ex de,hl
    pop hl
outputCopy:
    ld a,(hl)
    or a
    jp z,outputCopied
    ld (de),a
    inc hl
    inc de
    jp outputCopy
outputCopied:
    ex de,hl
    ld (reportEnd),hl
    pop hl
    jp bios_printStringZ
character:
    ld (oneChar),a
    ld hl,oneChar
    jp output
number:
    ld (ticks),hl
    xor a
    ld (ticks+2),a
    ld (ticks+3),a
    ld hl,ticks
    ld de,timer.v_number
    ld b,4
numberCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,numberCopy
    jp timer.print_number
milliseconds:
    ld (ticks),hl
    ld hl,0
    ld (ticks+2),hl
    cp 16
    jp nz,millisecondsReady
    ld b,4
millisecondsScale:
    ld hl,ticks
    ld c,4
    or a
millisecondsShift:
    ld a,(hl)
    rla
    ld (hl),a
    inc hl
    dec c
    jp nz,millisecondsShift
    dec b
    jp nz,millisecondsScale
millisecondsReady:
    ld hl,ticks
    call timer.print_time_ms
    ld hl,msText
    jp output

    MODULE math
    DEFINE MATH_DIV
    DEFINE MATH_SUB
    INCLUDE "../../source/include/math.inc"
    ENDMODULE
    MODULE timer
    MACRO LOAD_DE address
    push hl
    ld hl,(address)
    ex de,hl
    pop hl
    ENDM
timing_print = output
timing_printChar = character
timing_div_hl_de = math.div_hl_de
timer_read = raw.pitTimerRead
v_elapsed: DD 0
v_setup: DD 0
v_last: DW 0
    INCLUDE "../../source/include/timing.inc"
    INCLUDE "../../source/include/timing-ms.inc"
    ENDMODULE

    MACRO ROW prepared,divider,method,text
    DB prepared,divider
    DW method,text
    ENDM
rows:
    ROW 0,1,raw.ncSortPass,shellRaw
    ROW 0,16,raw.benchInsertion,insertionRaw
    ROW 0,1,raw.benchMerge,mergeRaw
    ROW 0,1,raw.benchQuick,quickRaw
    ROW 1,1,upper.ncSortPass,shellUpper
    ROW 1,16,upper.benchInsertion,insertionUpper
    ROW 1,1,upper.benchMerge,mergeUpper
    ROW 1,1,upper.benchQuick,quickUpper
    DB 0FFh
sizes: DB 32,64,128,250,255
baseName: DB 'FILE0000'
extensions: DB 'ASMTXTCOM   '
title: DB 'SORTBEN / Intel 8080 / PIT 2 MHz',13,10,0
clearScreen: DB 01Fh,0
sourcePrompt: DB 'Source: S=synthetic(default), H=H: root',13,10,0
sizePrompt: DB 'N: 1=32 2=64 3=128(default) 4=250 5=255',13,10,0
countText: DB 'Records: ',0
syntheticText: DB 'Source: synthetic LFSR, seed 5A',13,10,0
hRootText: DB 'Source: H: root (first N entries)',13,10,0
noMemoryText: DB 'SORTBEN requires free MAIN through 8AFFh',13,10,0
nameTitle: DB 13,10,'NAME 8+3',13,10,0
extTitle: DB 13,10,'EXTENSION 3+8',13,10,0
upperPrepText: DB 'Uppercase preparation: ',0
radixPrepText: DB 'Radix preparation:     ',0
shellRaw: DB 'Shell raw:       ',0
insertionRaw: DB 'Insertion raw:   ',0
mergeRaw: DB 'Merge raw:       ',0
quickRaw: DB 'Quicksort raw:   ',0
shellUpper: DB 'Shell upper:     ',0
insertionUpper: DB 'Insertion upper: ',0
mergeUpper: DB 'Merge upper:     ',0
quickUpper: DB 'Quicksort upper: ',0
radixText: DB 'Radix keys:      ',0
msText: DB ' ms',0
okText: DB ' OK',13,10,0
doneText: DB 13,10,'All indices match. Source checksum OK.',13,10,0
failText: DB 13,10,'BENCHMARK FAILED',13,10,0
savedText: DB 'Saved B:\SORTBEN.TXT',13,10,0
saveErrorText: DB 'Cannot save B:\SORTBEN.TXT',13,10,0
waitText: DB 'Press any key to exit',13,10,0
betweenText: DB 13,10,'Press Enter for the next table',13,10,0
crlf: DB 13,10,0
oneChar: DB 0,0
DECIMAL: DS 11,0
ticks: DW 0,0
savedSP: DW 0
reportEnd: DW REPORT
rowCursor: DW 0
rowTicks: DW 0
radixTicks: DW 0
sourceSum: DW 0
oldDirectory: DW 0
oldDrive: DB 0
sourceKind: DB 0
count: DB 0
mode: DB 0
rowUpper: DB 0
divisor: DB 1
random: DB 05Ah
failed: DB 0
saveIO:
    DW savePath,REPORT,0,0
    DS FILE_DESCRIPTOR,0
savePath: DB 'B:\SORTBEN.TXT',0
    DS MAX_PATH-($-savePath),0
REFERENCE = 07200h
REPORT = 07300h
    ASSERT $ < raw.benchRaw
    END
