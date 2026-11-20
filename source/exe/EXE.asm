; EXE.COM: conversion between RAW, RKS and MXOS executable containers.
    INCLUDE "../include/mxos.inc"

EXE_LOAD_ADDR = 0100h
IMAGE_FLAGS = IMAGE_FLAG_MAIN
IMAGE_API = 2
IMAGE_MEMORY_POLICY = IMAGE_POLICY_MANAGED

; Prefix fields shared by v1 and v2.
HDR_VERSION = 4
HDR_FLAGS = 5
HDR_LOAD = 6
HDR_ENTRY = 8
HDR_SIZE = 10
HDR_SUM = 12
HDR_RESERVED = IMAGE_OPTIONS+1
V1_SIZE = 16
RKS_HEADER_SIZE = 4
RKS_TRAILER_SIZE = 2
BLOCK_SIZE = 1024
ARG_LIMIT = CMD_LINE_SIZE-2
OPT_LOAD = 1
OPT_ENTRY = 2
OPT_BEGIN = 4
OPT_END = 8
OPT_POLICY = 16

    MACRO GET_DE address
        push hl
        ld hl,(address)
        ex de,hl
        pop hl
    ENDM

    ORG EXE_LOAD_ADDR

exeStart:
    ld hl,bssStart
    ld b,EXE_WORK.prefix
    call clearBytes
    ld a,(bios_upper.koi7)
    push af
    xor a
    call bios_setFontEncoding
    call convert
exeRestoreEncoding:
    pop af
    jp bios_setFontEncoding

; DE = argument string. HL = exit status, other registers and flags clobbered.
convert:
    ld hl,work.command
    ld b,ARG_LIMIT
.copyArgs:
    ld a,(de)
    or a
    jp z,.copied
    ld (hl),a
    inc de
    inc hl
    dec b
    jp nz,.copyArgs
.copied:
    ld (hl),0
    ld (work.argEnd),hl
    ld hl,banner
    call bios_printString
    ld hl,work.command
    ld (work.argCursor),hl

.parsePaths:
    call nextArg
    jp z,.pathsDone
    ld a,(hl)
    cp '-'
    jp z,.earlyOption
    ex de,hl
    ld hl,(work.sourcePath)
    ld a,h
    or l
    jp z,.source
    ld hl,(work.targetPath)
    ld a,h
    or l
    jp nz,invalidCommand
    ex de,hl
    ld (work.targetPath),hl
    jp .parsePaths
.source:
    ex de,hl
    ld (work.sourcePath),hl
    jp .parsePaths
.earlyOption:
    inc hl
    ld de,rawName
    call sameText
    jp nz,.parsePaths
    ld a,1
    ld (work.rawInput),a
    jp .parsePaths

.pathsDone:
    ld hl,(work.targetPath)
    ld a,h
    or l
    ld hl,usage
    jp z,fail
    ld hl,(work.sourcePath)
    ex de,hl
    ld hl,(work.targetPath)
    call sameText
    jp z,samePath
    ld hl,(work.sourcePath)
    ex de,hl
    ld hl,work.inputHandle
    ld a,FILE_MODE_READ
    call openPath
    jp c,openError

    ld hl,(work.inputHandle+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,tooLarge
    ld hl,(work.inputHandle+FILE_DESCRIPTOR.size)
    ld (work.bodySize),hl
    ld a,h
    or l
    jp z,badSource
    ld hl,IMAGE_BSS_UNKNOWN
    ld (work.header+IMAGE_BSS),hl
    call importHeader
    jp c,badSource
    call scanBody
    jp c,badSource

    ld hl,work.command
    ld (work.argCursor),hl
.parseOptions:
    call nextArg
    jp z,.optionsDone
    ld a,(hl)
    cp '-'
    jp nz,.parseOptions
    inc hl
    call applyOption
    jp nc,.parseOptions
    jp invalidMetadata
.optionsDone:
    call finishHeader
    jp c,invalidMetadata
    ld hl,(work.targetPath)
    call extension
    ld de,outputTypes
    call lookup
    jp c,invalidMetadata
    ld (work.rksOutput),a
    or a
    jp z,.checkTarget
    ld hl,(work.header+HDR_ENTRY)
    GET_DE work.header+HDR_LOAD
    call bios_cmp_hl_de
    jp nz,invalidMetadata

.checkTarget:
    ld hl,(work.targetPath)
    ex de,hl
    ld hl,work.outputHandle
    ld a,FILE_MODE_READ
    call openPath
    jp c,.createTarget
    call closeOutput
    ld a,(work.outputHandle+FH_DRIVE)
    ld hl,work.inputHandle+FH_DRIVE
    cp (hl)
    jp nz,.confirm
    ld hl,(work.outputHandle+FILE_DESCRIPTOR.firstCluster)
    GET_DE work.inputHandle+FILE_DESCRIPTOR.firstCluster
    call bios_cmp_hl_de
    jp z,sameOpenFile
.confirm:
    ld a,(work.overwrite)
    or a
    jp nz,.createTarget
    ld hl,overwritePrompt
    call bios_printString
    call bios_getch
    call bios_upperCase
    ld c,10
    call bios_printChar
    cp 'Y'
    jp nz,declined
.createTarget:
    ld hl,(work.bodyOffset)
    call seekInput
    jp c,badSource
    ld hl,(work.targetPath)
    ex de,hl
    ld hl,work.outputHandle
    ld a,FILE_MODE_WRITE|FILE_MODE_CREATE|FILE_MODE_TRUNCATE
    call openPath
    jp c,badSource

    call writeHeader
    jp c,writeError
    ld hl,(work.bodySize)
    ld (work.remaining),hl
.copyBody:
    call readBlock
    jp c,writeError
    ld de,work.buffer
    call writeOutput
    jp c,writeError
    ld hl,(work.remaining)
    ld a,h
    or l
    jp nz,.copyBody

    ld a,(work.rksOutput)
    or a
    jp z,.close
    ld de,work.bodySum
    ld bc,RKS_TRAILER_SIZE
    call writeOutput
    jp c,writeError
.close:
    call closeOutput
    push af
    call closeInput
    pop af
    ld hl,closeErrorText
    jp c,fail

    ld hl,savedText
    call bios_printString
    ld hl,(work.targetPath)
    call bios_printStringRawZ
    ld hl,loadText
    call bios_printString
    ld hl,(work.header+HDR_LOAD)
    call bios_printHexWord
    ld hl,entryText
    call bios_printString
    ld hl,(work.header+HDR_ENTRY)
    call bios_printHexWord
    ld c,10
    call bios_printChar
    ld hl,0
    ret

invalidCommand:
    ld hl,invalidCommandText
    jp fail
sameOpenFile:
    call closeInput
samePath:
    ld hl,samePathText
    jp fail
openError:
    ld hl,openErrorText
    jp fail
tooLarge:
    ld hl,tooLargeText
    jp closeAndFail
invalidMetadata:
    ld hl,metadataErrorText
    jp closeAndFail
badSource:
    ld hl,readErrorText
    jp closeAndFail
writeError:
    call closeOutput
    ld hl,writeErrorText
closeAndFail:
    push hl
    call closeInput
    pop hl
fail:
    call bios_printString
    ld hl,1
    ret
declined:
    call closeInput
    ld hl,1
    ret

; Tokenize a private copy on the first pass. Later passes skip its NULs.
; Z = end, otherwise HL = ASCIIZ token. AF/BC/DE clobbered.
nextArg:
    ld hl,(work.argCursor)
    GET_DE work.argEnd
.skip:
    call bios_cmp_hl_de
    ret z
    ld a,(hl)
    inc hl
    or a
    jp z,.skip
    cp ' '
    jp z,.skip
    dec hl
    push hl
.scan:
    ld a,(hl)
    or a
    jp z,.end
    cp ' '
    jp z,.split
    inc hl
    jp .scan
.split:
    ld (hl),0
    inc hl
.end:
    ld (work.argCursor),hl
    pop hl
    ld a,1
    or a
    ret

; Import MX v1/v2 or RKS metadata. RAW keeps the initial defaults.
; CF = invalid format or I/O error. AF/BC/DE/HL clobbered.
importHeader:
    ld a,(work.rawInput)
    or a
    ret nz
    ld hl,(work.bodySize)
    ld de,V1_SIZE
    call bios_cmp_hl_de
    jp c,.tryRks
    ld de,IMAGE_HEADER_SIZE
    call bios_cmp_hl_de
    jp c,.probe
    ex de,hl
.probe:
    ld b,h
    ld c,l
    ld de,work.prefix
    call readInput
    ret c
    ld hl,(work.prefix)
    ld de,020CDh
    call bios_cmp_hl_de
    jp nz,.tryRks
    ld hl,(work.prefix+2)
    ld de,'M'+'X'*256
    call bios_cmp_hl_de
    jp nz,.tryRks

    ld a,(work.prefix+HDR_VERSION)
    dec a
    ld hl,V1_SIZE
    jp z,.mxSize
    dec a
    jp nz,invalid
    ld hl,IMAGE_HEADER_SIZE
.mxSize:
    ld (work.bodyOffset),hl
    ex de,hl
    ld hl,(work.bodySize)
    call subtract
    ret c
    GET_DE work.prefix+HDR_SIZE
    call bios_cmp_hl_de
    jp nz,invalid
    ld (work.bodySize),hl
    ld hl,(work.bodyOffset)
    ld de,work.prefix-3
    add hl,de
    ex de,hl
    ld hl,work.prefix
    call bios_calcCS
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld h,b
    ld l,c
    call bios_cmp_hl_de
    jp nz,invalid

    ld hl,(work.prefix+HDR_SUM)
    ld (work.expectedSum),hl
    ld hl,(work.bodyOffset)
    ld a,l
    cp IMAGE_HEADER_SIZE
    jp nz,.v1
    ld a,(work.prefix+IMAGE_HEADER_BYTES)
    cp IMAGE_HEADER_SIZE
    jp nz,invalid
    ld a,(work.prefix+HDR_RESERVED)
    or a
    jp nz,invalid
    ld de,work.prefix+IMAGE_HEADER_SIZE
    jp .copyHeader
.v1:
    ld a,(work.prefix+HDR_FLAGS)
    or a
    jp z,.v1Copy
    ld hl,0
    ld (work.header+IMAGE_BSS),hl
    ld a,IMAGE_API
    ld (work.header+IMAGE_API_VERSION),a
.v1Copy:
    ld de,work.prefix+IMAGE_BSS
.copyHeader:
    ld hl,work.prefix
    ld bc,work.header
    call bios_memcpy_bc_hl
    or a
    ret

.tryRks:
    ld hl,(work.sourcePath)
    call extension
    ld de,rksExtension
    call sameText
    jp nz,.raw
    ld hl,(work.bodySize)
    ld de,RKS_HEADER_SIZE+RKS_TRAILER_SIZE+1
    call bios_cmp_hl_de
    jp c,invalid
    ld hl,0
    call seekInput
    ret c
    ld de,work.prefix
    ld bc,RKS_HEADER_SIZE
    call readInput
    ret c
    ld hl,(work.prefix)
    ld (work.header+HDR_LOAD),hl
    ld (work.header+HDR_ENTRY),hl
    ld hl,(work.prefix+2)
    GET_DE work.prefix
    call subtract
    ret c
    inc hl
    push hl
    ld hl,(work.bodySize)
    ld de,RKS_HEADER_SIZE+RKS_TRAILER_SIZE
    call subtract
    pop de
    call bios_cmp_hl_de
    jp nz,invalid
    ld (work.bodySize),hl
    ld hl,RKS_HEADER_SIZE
    ld (work.bodyOffset),hl
    ld hl,(work.inputHandle+FILE_DESCRIPTOR.size)
    dec hl
    dec hl
    call seekInput
    ret c
    ld de,work.expectedSum
    ld bc,RKS_TRAILER_SIZE
    jp readInput
.raw:
    or a
    ret

; Scan before any target write. BIOS returns the last-byte pointer and
; the carry from adding it to the low sum. Include that byte in the high
; sum for every chunk, then remove its contribution once for the whole file.
; CF = read/checksum error. AF/BC/DE/HL clobbered.
scanBody:
    ld hl,(work.bodySize)
    ld a,h
    or l
    jp z,invalid
    ld (work.remaining),hl
    ld hl,(work.bodyOffset)
    call seekInput
    ret c
.chunk:
    call readBlock
    ret c
    ld hl,work.buffer-1
    add hl,bc
    ex de,hl
    ld hl,work.buffer
    call bios_calcCS
    ld a,b
    adc a,(hl)
    ld b,a
    ld a,(hl)
    ld hl,(work.bodySum)
    add hl,bc
    ld (work.bodySum),hl
    ld e,a
    ld hl,(work.remaining)
    ld a,h
    or l
    jp nz,.chunk

    ld hl,(work.bodySum)
    ld a,l
    sub e
    ld a,h
    sbc a,e
    ld h,a
    ld (work.bodySum),hl
    ld a,(work.bodyOffset)
    or a
    ret z
    GET_DE work.expectedSum
    call bios_cmp_hl_de
    ret z
invalid:
    scf
    ret

; Update remaining and read a nonempty chunk. BC = count, CF = failure.
; AF/DE/HL clobbered.
readBlock:
    ld hl,(work.remaining)
    ld b,h
    ld c,l
    ld a,h
    cp BLOCK_SIZE/256
    jp c,.count
    ld bc,BLOCK_SIZE
.count:
    ld d,b
    ld e,c
    call subtract
    ld (work.remaining),hl
    ld de,work.buffer
    push bc
    call readInput
    pop bc
    ret

; HL = option token following '-'. CF = invalid. All registers clobbered.
applyOption:
    ld de,overwriteName
    call sameText
    jp nz,.tryRaw
    ld a,1
    ld (work.overwrite),a
    or a
    ret
.tryRaw:
    ld de,rawName
    call sameText
    ret z
    push hl
    ld b,'='
    call findByte
    pop de
    jp nz,.value
    ex de,hl
    push hl
    ld b,':'
    call findByte
    pop de
    jp z,invalid
.value:
    ld (hl),0
    inc hl
    push hl
    ex de,hl
    ld de,options
    call lookup
    pop de
    ret c
    cp 3
    jp c,.named
    push af
    call parseHex
    ex de,hl
    pop bc
    ret c
    ld a,b
    sub 3
    ld hl,numericOptions
    add a,a
    add a,l
    ld l,a
    jp nc,.numeric
    inc h
.numeric:
    ld a,(hl)
    inc hl
    ld b,(hl)
    cp IMAGE_OPTIONS
    jp nz,.word
    ld a,d
    or a
    jp nz,invalid
    ld a,e
    cp IMAGE_OPTION_CACHEABLE+1
    jp nc,invalid
    ld a,IMAGE_OPTIONS
.word:
    call headerField
    ld (hl),e
    inc hl
    ld (hl),d
    ld hl,work.overrides
    ld a,(hl)
    or b
    ld (hl),a
    ret

.named:
    ex de,hl
    or a
    jp z,.flags
    dec a
    jp z,.policy
    ld de,modes
    call lookup
    ret c
    ld (work.header+IMAGE_MODE),a
    ret
.flags:
    ld de,flags
    call lookup
    ret c
    ld (work.header+HDR_FLAGS),a
    or a
    jp z,.api
    ld a,IMAGE_API
.api:
    ld (work.header+IMAGE_API_VERSION),a
    ret
.policy:
    ld de,policies
    call lookup
    ret c
    ld (work.header+IMAGE_POLICY),a
    ld a,(work.overrides)
    or OPT_POLICY
    ld (work.overrides),a
    ret

; A = offset within the header, HL = field address. A/flags clobbered.
headerField:
    add a,work.header & 0FFh
    ld l,a
    ld a,work.header >> 8
    adc a,0
    ld h,a
    ret

; Fill metadata defaults, then validate the full image range before writing.
; CF = invalid metadata. AF/BC/DE/HL clobbered.
finishHeader:
    ld a,(work.overrides)
    and OPT_LOAD|OPT_ENTRY
    cp OPT_LOAD
    jp nz,.defaults
    ld hl,(work.header+HDR_LOAD)
    ld (work.header+HDR_ENTRY),hl
.defaults:
    ld a,(work.overrides)
    and OPT_LOAD
    jp nz,.reset
    ld a,(work.bodyOffset)
    cp IMAGE_HEADER_SIZE
    jp z,.keep
.reset:
    ld a,(work.overrides)
    and OPT_POLICY
    jp nz,.defaultBegin
    ld a,(work.header+HDR_FLAGS)
    or a
    jp z,.setPolicy
    ld a,IMAGE_POLICY_MANAGED
.setPolicy:
    ld (work.header+IMAGE_POLICY),a
.defaultBegin:
    ld a,(work.overrides)
    and OPT_BEGIN
    jp nz,.keep
    ld hl,(work.header+HDR_LOAD)
    ld (work.header+IMAGE_MEMORY_BEGIN),hl
.keep:
    ld a,(work.header+HDR_FLAGS)
    or a
    jp nz,.checkLoad
    ld (work.header+IMAGE_API_VERSION),a

.checkLoad:
    ld hl,(work.header+HDR_ENTRY)
    GET_DE work.header+HDR_LOAD
    call bios_cmp_hl_de
    jp c,invalid
    ld hl,(work.bodySize)
    add hl,de
    ret c
    GET_DE work.header+HDR_ENTRY
    call bios_cmp_hl_de
    jp c,invalid
    jp z,invalid
    GET_DE work.header+IMAGE_BSS
    ld a,d
    and e
    inc a
    jp nz,.withBss
    ld a,(work.header+IMAGE_POLICY)
    or a
    jp nz,invalid
    jp .setEnd
.withBss:
    add hl,de
    ret c
.setEnd:
    ld a,(work.overrides)
    and OPT_END
    jp nz,.checkEnd
    ld a,(work.overrides)
    and OPT_LOAD
    jp nz,.defaultEnd
    ld a,(work.bodyOffset)
    cp IMAGE_HEADER_SIZE
    jp nz,.defaultEnd
    GET_DE work.header+IMAGE_MEMORY_END
    call bios_cmp_hl_de
    jp c,.checkBegin
.defaultEnd:
    ld (work.header+IMAGE_MEMORY_END),hl
    jp .checkBegin
.checkEnd:
    GET_DE work.header+IMAGE_MEMORY_END
    call bios_cmp_hl_de
    jp c,.checkBegin
    jp nz,invalid
.checkBegin:
    ld hl,(work.header+HDR_LOAD)
    GET_DE work.header+IMAGE_MEMORY_BEGIN
    call bios_cmp_hl_de
    jp c,invalid
    ld hl,(work.header+IMAGE_MEMORY_END)
    GET_DE work.header+IMAGE_MIN_HEAP
    add hl,de
    ret c

    ld a,(work.header+IMAGE_POLICY)
    cp IMAGE_POLICY_MANAGED+1
    jp nc,invalid
    ld a,(work.header+IMAGE_MODE)
    cp IMAGE_MODE_RF+1
    jp nc,invalid
    ld a,(work.header+IMAGE_API_VERSION)
    or a
    jp z,.checkOptions
    cp IMAGE_API
    jp nz,invalid
.checkOptions:
    ld a,(work.header+IMAGE_OPTIONS)
    cp IMAGE_OPTION_CACHEABLE+1
    jp nc,invalid
    ld a,(work.header+HDR_FLAGS)
    or a
    ret z
    cp IMAGE_FLAG_DRIVER
    ret z
    cp IMAGE_FLAG_BANK
    ret z
    cp IMAGE_FLAG_MAIN
    jp nz,invalid
    ret

; Write the RKS load/end pair or the complete MX v2 header.
; CF = write error. AF/BC/DE/HL clobbered.
writeHeader:
    ld a,(work.rksOutput)
    or a
    jp z,.mx
    ld hl,(work.header+HDR_LOAD)
    ld (work.prefix),hl
    GET_DE work.bodySize
    add hl,de
    dec hl
    ld (work.prefix+2),hl
    ld de,work.prefix
    ld bc,RKS_HEADER_SIZE
    jp writeOutput
.mx:
    ld hl,imageMagic
    ld de,imageMagic+HDR_FLAGS
    ld bc,work.header
    call bios_memcpy_bc_hl
    ld hl,(work.bodySize)
    ld (work.header+HDR_SIZE),hl
    ld hl,(work.bodySum)
    ld (work.header+HDR_SUM),hl
    ld a,IMAGE_HEADER_SIZE
    ld (work.header+IMAGE_HEADER_BYTES),a
    ld hl,work.header
    ld de,work.header+IMAGE_HEADER_CHECKSUM-1
    call bios_calcCS
    ld h,b
    ld l,c
    ld (work.header+IMAGE_HEADER_CHECKSUM),hl
    ld de,work.header
    ld bc,IMAGE_HEADER_SIZE
    jp writeOutput

; HL = handle, DE = path, A = mode. FILEHANDLE returns status in A/CF.
; BC/DE/HL may be clobbered.
openPath:
    push hl
    push af
    push de
    ld b,FILE_HANDLE
    call clearBytes
    pop de
    pop af
    pop hl
    push hl
    push af
    ld b,FILE_NAME_LENGTH+3
.spaces:
    ld (hl),' '
    inc hl
    dec b
    jp nz,.spaces
    ld bc,FH_DRIVE-FILE_NAME_LENGTH-3
    add hl,bc
    push de
    push hl
    ld e,0
    MXOS_CALL MXOS_DOS_FILE_GET_SET_DRIVE
    pop hl
    pop de
    ld (hl),a
    inc hl
    pop af
    ld (hl),a
    ld bc,FH_BUFFER-FH_MODE
    add hl,bc
    ld (hl),e
    inc hl
    ld (hl),d
    pop hl
    ld a,FILE_OP_OPEN_PATH
    jp fileHandle

; HL = absolute offset in input. FILEHANDLE returns A/CF, BC/DE/HL clobbered.
seekInput:
    ld (work.inputHandle+FH_SEEK),hl
    ld hl,0
    ld (work.inputHandle+FH_SEEK+2),hl
    xor a
    ld (work.inputHandle+FH_ORIGIN),a
    ld hl,work.inputHandle
    ld a,FILE_OP_SEEK
    jp fileHandle

closeInput:
    ld hl,work.inputHandle
    jp closeHandle
closeOutput:
    ld hl,work.outputHandle
closeHandle:
    ld a,FILE_OP_CLOSE
fileHandle:
    MXOS_CALL MXOS_DOS_FILEHANDLE
    ret

; DE = data, BC = requested bytes. CF = DOS error or short transfer.
; AF/BC/DE/HL clobbered.
readInput:
    ld hl,work.inputHandle
    ld a,FILE_OP_READ
    jp transfer
writeOutput:
    ld hl,work.outputHandle
    ld a,FILE_OP_WRITE
transfer:
    push af
    push hl
    push bc
    ld bc,FH_BUFFER
    add hl,bc
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    pop bc
    ld (hl),c
    inc hl
    ld (hl),b
    pop de
    pop af
    push bc
    push hl
    ex de,hl
    call fileHandle
    pop hl
    pop de
    ret c
    ld b,(hl)
    dec hl
    ld l,(hl)
    ld h,b
    call bios_cmp_hl_de
    ret z
    scf
    ret

; DE = ASCIIZ hex value. HL = result, CF = invalid, AF/BC/DE clobbered.
parseHex:
    ld hl,0
    ld b,4
    ld a,(de)
    or a
    jp z,invalid
.digit:
    call bios_upperCase
    sub '0'
    jp c,invalid
    cp 10
    jp c,.number
    sub 'A'-'0'-10
    cp 10
    jp c,invalid
    cp 16
    jp nc,invalid
.number:
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    or l
    ld l,a
    inc de
    ld a,(de)
    or a
    ret z
    dec b
    jp nz,.digit
    jp invalid

; HL = text, DE = table (value, ASCIIZ, ..., FFh).
; A = value, CF = no match, HL preserved, BC/DE clobbered.
lookup:
    ld a,(de)
    cp 0FFh
    jp z,invalid
    push af
    inc de
    push de
    call sameText
    pop de
    jp z,.found
    pop af
.skip:
    ld a,(de)
    inc de
    or a
    jp nz,.skip
    jp lookup
.found:
    pop af
    or a
    ret

; HL/DE = ASCIIZ strings, Z = equal, HL preserved, AF/C/DE clobbered.
sameText:
    push hl
.next:
    ld a,(hl)
    call bios_upperCase
    ld c,a
    ld a,(de)
    call bios_upperCase
    cp c
    jp nz,.done
    or a
    jp z,.done
    inc hl
    inc de
    jp .next
.done:
    pop hl
    ret

; HL = path, HL returns last dot or terminating NUL. AF/DE clobbered.
extension:
    ld de,0
.next:
    ld a,(hl)
    or a
    jp z,.done
    cp '.'
    jp nz,.advance
    ld d,h
    ld e,l
.advance:
    inc hl
    jp .next
.done:
    ld a,d
    or e
    ret z
    ex de,hl
    ret

; HL = ASCIIZ, B = nonzero byte. HL = match (NZ) or terminator (Z).
; A/flags clobbered.
findByte:
    ld a,(hl)
    or a
    ret z
    cp b
    jp z,.found
    inc hl
    jp findByte
.found:
    or a
    ret

; HL -= DE, CF = borrow. A/flags clobbered, BC/DE preserved.
subtract:
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    ret

; HL = destination, B = nonzero length. AF/B/HL clobbered, flags unspecified.
clearBytes:
    xor a
.next:
    ld (hl),a
    inc hl
    dec b
    jp nz,.next
    ret

imageMagic:
    MXOS_IMAGE_SIGNATURE
options:
    DB 0,"F",0, 1,"P",0, 2,"MODE",0
    DB 3,"L",0, 4,"E",0, 5,"BEGIN",0, 6,"END",0
    DB 7,"B",0, 8,"HEAP",0, 9,"STACK",0, 10,"CACHE",0, 0FFh
numericOptions:
    DB HDR_LOAD,OPT_LOAD, HDR_ENTRY,OPT_ENTRY
    DB IMAGE_MEMORY_BEGIN,OPT_BEGIN, IMAGE_MEMORY_END,OPT_END
    DB IMAGE_BSS,0, IMAGE_MIN_HEAP,0, IMAGE_STACK,0, IMAGE_OPTIONS,0
flags:
    DB IMAGE_FLAG_MAIN,"MAIN",0, IMAGE_FLAG_BANK,"BANK",0
    DB IMAGE_FLAG_DRIVER,"DOS",0, 0,"LEGACY",0, 0FFh
policies:
    DB IMAGE_POLICY_EXCLUSIVE,"EXCLUSIVE",0, IMAGE_POLICY_PRESERVE,"PRESERVE",0
    DB IMAGE_POLICY_MANAGED,"MANAGED",0, 0FFh
modes:
    DB IMAGE_MODE_AUTO,"AUTO",0, IMAGE_MODE_MX,"MX",0, IMAGE_MODE_STD,"STD",0
    DB IMAGE_MODE_MON,"MON",0, IMAGE_MODE_RF,"RF",0, 0FFh
outputTypes:
    DB 1,".RKS",0, 0,".EXE",0, 0,".COM",0, 0,".DLL",0, 0,".FNT",0, 0FFh
rksExtension: DB ".RKS",0
rawName: DB "RAW",0
overwriteName: DB "Y",0

banner: DB "EXE 1.0",10,0
usage:
    DB "EXE source target [-F=MAIN|BANK|DOS|LEGACY] [-Y]",10
    DB "-B=hex -L=hex -E=hex -BEGIN=hex -END=hex",10
    DB "-P=EXCLUSIVE|PRESERVE|MANAGED -HEAP=hex -STACK=hex",10
    DB "-MODE=AUTO|MX|STD|MON|RF -CACHE=0|1 -RAW",10
    DB "All numbers are hexadecimal. Source is preserved.",10,0
invalidCommandText: DB "Invalid arguments",10,0
samePathText: DB "Source and target must differ",10,0
openErrorText: DB "Cannot open source",10,0
tooLargeText: DB "Source is too large",10,0
metadataErrorText: DB "Invalid metadata or arguments",10,0
readErrorText: DB "Invalid source or read error",10,0
writeErrorText: DB "Write failed: target is incomplete",10,0
closeErrorText: DB "Cannot close target",10,0
overwritePrompt: DB "Overwrite file? [Y/N] ",0
savedText: DB "Saved ",0
loadText: DB " LOAD=",0
entryText: DB " ENTRY=",0

    STRUCT EXE_WORK
sourcePath      BLOCK 2
targetPath      BLOCK 2
argCursor       BLOCK 2
argEnd          BLOCK 2
bodySize        BLOCK 2
bodyOffset      BLOCK 2
bodySum         BLOCK 2
expectedSum     BLOCK 2
remaining       BLOCK 2
rawInput        BLOCK 1
overwrite       BLOCK 1
overrides       BLOCK 1
rksOutput       BLOCK 1
header          BLOCK IMAGE_HEADER_SIZE
prefix          BLOCK IMAGE_HEADER_SIZE
inputHandle     BLOCK FILE_HANDLE
outputHandle    BLOCK FILE_HANDLE
command         BLOCK ARG_LIMIT+1
buffer          BLOCK BLOCK_SIZE
    ENDS

bssStart:
work EXE_WORK = bssStart
bssEnd = bssStart+EXE_WORK

IMAGE_BSS_SIZE = bssEnd-bssStart
IMAGE_MEM_END = bssEnd
    ASSERT bssStart == $
    ASSERT EXE_WORK.prefix <= 255
    ASSERT BLOCK_SIZE % 256 == 0
    ASSERT bssEnd <= APP_MEMORY_END
