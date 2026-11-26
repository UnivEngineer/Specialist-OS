; FONT: stage a fixed-slot engine, invoke its disposable initializer, publish.
; DE is the normal MAIN application argument string. PRE reads go directly
; into reserved memory; a failed read is repaired before any text output.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../include/font-init.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
; Optional PIT instrumentation: no emitted bytes in the production build.
; One local timestamp per phase; preserve the phase result registers/flags.
    MACRO FONT_STAMP offset
    IFDEF FONT_BENCH
    push af
    push hl
    call pitTimerRead
    ld (benchStamps+offset),hl
    pop hl
    pop af
    ENDIF
    ENDM
    ORG 0
start:
    call rememberCommand
    jp c,usage
    IFDEF FONT_BENCH
    call pitTimerInit
    jp c,error
    ENDIF
    ex de,hl
skipSpace:
    ld a,(hl)
    cp ' '
    jp nz,argument
    inc hl
    jp skipSpace
argument:
    or a
    jp z,usage
    ld de,relativeEnginePath
    call fontExpandShortName
    jp c,fontPathReady
    ld c,0
    jp pathDone
fontPathReady:
    ld b,MAX_PATH-(relativeEnginePath-path)-5 ; Reserve .fnt and its terminator.
    ld c,0
copyArgument:
    ld a,(hl)
    or a
    jp z,pathDone
    cp ' '
    jp z,pathDone
    cp '/'
    jp z,fontSeparator
    cp 05Ch
    jp nz,notSlash
fontSeparator:
    ld c,0
    ld a,05Ch
notSlash:
    cp '.'
    jp nz,notDot
    ld c,a
notDot:
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,copyArgument
    jp usage
pathDone:
    ld (argTail),hl
    ld a,c
    ld (typedExtension),a
    or a
    jp nz,terminatePath
    ld hl,extension
    ld b,4
appendExtension:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,appendExtension
terminatePath:
    xor a
    ld (de),a
    call parseOption
    jp c,usage
    ld e,0
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    ld (oldDrive),a
    FONT_STAMP 0
    call fontReadEngine
    FONT_STAMP 2
    jp c,error
    call validate
    jp c,error
    call checkWorkspace
    jp c,error
    FONT_STAMP 4
    call loadInitializer
    jp c,error
    call importFont
    jp c,error
    FONT_STAMP 6
    ; Reserve only after the executable and optional flat input are checked.
    ; PRE is the sole destructive I/O phase; failed reads are regenerated.
    ld hl,(workspaceEnd)
    ex de,hl
    ld hl,(staging+IMAGE_HEADER_SIZE+44)
    ld bc,0100h+MEM_RESIDENT_FONT
    ld a,MEM_OP_RESIDENT
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    jp c,error
    ld (lowerBase),hl
    FONT_STAMP 8
    call loadPrepared
    jp nc,resourcesReady
    ; A failed direct read may have destroyed the old planes. Rebuild the
    ; NEW table before any text output, then publish the new engine.
    ld (preparedError),a
    call generateResources
resourcesReady:
    FONT_STAMP 10
    ; The resident image is already complete. No file access, allocation or
    ; output is allowed between this copy and publication of the entry points.
    ld hl,(imageBytes)
    ld de,staging+IMAGE_HEADER_SIZE
    add hl,de
    ex de,hl
    ld hl,staging+IMAGE_HEADER_SIZE
    ld bc,FONT_ENGINE_BASE
    call bios_memcpy_bc_hl
    FONT_STAMP 12
    ld hl,(lowerBase)
    ex de,hl
    call FONT_ENGINE_INIT
    FONT_STAMP 14
committed:
    ld a,3
    MXOS_CALL_REGS MXOS_BIOS_GLYPH_CACHE
    ld hl,request+FILE_PATH_IO.descriptor
    ld de,request+FILE_PATH_IO.descriptor+8
    ld bc,io_vars.fontName
    call bios_memcpy_bc_hl
    ld hl,resumeCommand
    xor a
    MXOS_CALL_REGS MXOS_DOS_FONT_CONFIG
    ld a,b
    or a
    jp nz,fontExportReady
    call saveImported
    jp c,error
fontExportReady:
    ld a,(preparedError)
    or a
    jp nz,error
    ld hl,installed
    jp bios_printStringZ
;----------------------------------------------------------------------------
; MXOS
;
; Открыть движок для FONT или ассоциации CLI.EXT.
; вход: relativeEnginePath = путь ASCIIZ с расширением; typedExtension != 0,
;   если расширение ввёл вызывающий. oldDrive содержит исходный диск.
; выход: cf=0, staging содержит файл, request — его путь, размер и дескриптор.
;   cf=1/a — ошибка чтения. Восстанавливает диск, af/bc/de/hl рабочие.
;----------------------------------------------------------------------------
fontReadEngine:
    ld hl,relativeEnginePath
    ld a,(hl)
    cp 05Ch
    jp z,fontReadSelected
    cp '.'
    jp z,fontReadSelected
    inc hl
    ld a,(hl)
    dec hl
    cp ':'
    jp z,fontReadSelected
    ld a,(typedExtension)
    or a
    jp z,fontReadLibrary
    call fontReadSelected
    ret nc
    cp DOS_ERR_NOT_FOUND
    scf
    ret nz
fontReadLibrary:
    ld hl,path
fontReadSelected:
    ld (request+FILE_PATH_IO.path),hl
    ld hl,request
    jp loadPath
;----------------------------------------------------------------------------
; MXOS
;
; Развернуть короткое имя штатного шрифта в путь относительно C:\FONTS.
;
; вход:
;   hl = начало аргумента, имя заканчивается пробелом или нулём.
;   de = начало свободной области длиной не менее 10 байтов.
;
; выход:
;   cf = 0 при совпадении, hl указывает на разделитель, de — за записанным
;   путём без нуля. При cf = 1 hl, de и буфер сохранены. af и bc разрушены.
;----------------------------------------------------------------------------
fontExpandShortName:
    ld bc,fontAliases
fontAliasNext:
    ld a,(bc)
    or a
    jp z,fontAliasMiss
    push hl
fontAliasCompare:
    ld a,(bc)
    or a
    jp z,fontAliasEnd
    push de
    ld d,a
    ld a,(hl)
    call bios_upperCase
    cp d
    pop de
    jp nz,fontAliasMismatch
    inc hl
    inc bc
    jp fontAliasCompare
fontAliasEnd:
    ld a,(hl)
    cp ' '+1
    jp nc,fontAliasMismatch
    pop af
    inc bc
fontAliasCopy:
    ld a,(bc)
    or a
    ret z
    ld (de),a
    inc bc
    inc de
    jp fontAliasCopy
fontAliasMismatch:
    pop hl
fontAliasSkipName:
    ld a,(bc)
    inc bc
    or a
    jp nz,fontAliasSkipName
fontAliasSkipPath:
    ld a,(bc)
    inc bc
    or a
    jp nz,fontAliasSkipPath
    jp fontAliasNext
fontAliasMiss:
    scf
    ret
fontAliases:
    DB "ROTATE",0,"6x8",05Ch,"Rotate",0
    DB "FAST8",0,"8x8",05Ch,"Fast",0
    DB "CPM4",0,"4x8",05Ch,"Packed",0
    DB 0

rememberCommand:
    push hl
    push bc
    push de
    ex de,hl
    ld de,resumeArguments
    ld b,MAX_PATH-(resumeArguments-resumeCommand)
rememberCommandByte:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    or a
    jp z,rememberCommandDone
    dec b
    jp nz,rememberCommandByte
    scf
rememberCommandDone:
    pop de
    pop bc
    pop hl
    ret
resumeCommand: DB BOOT_VOLUME_LETTER,':',05Ch,'DOS',05Ch,'FONT.COM '
resumeArguments:
    DS MAX_PATH-(resumeArguments-resumeCommand),0
validate:
    ld hl,(request+FILE_PATH_IO.actual)
    ld de,IMAGE_HEADER_SIZE+48
    call bios_cmp_hl_de
    jp c,badImage
    ld hl,staging
    ld de,magic
    ld b,6
magicLoop:
    ld a,(de)
    cp (hl)
    jp nz,badImage
    inc hl
    inc de
    dec b
    jp nz,magicLoop
    ld hl,(staging+6)
    ld de,FONT_ENGINE_BASE
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,(staging+8)
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,(staging+10)
    ld (imageBytes),hl
    ld de,FONT_ENGINE_END-FONT_ENGINE_BASE
    call bios_cmp_hl_de
    jp c,sizeFits
    jp nz,badImage
sizeFits:
    ld de,48
    call bios_cmp_hl_de
    jp c,badImage
    ld de,IMAGE_HEADER_SIZE
    add hl,de
    ex de,hl
    ld hl,(request+FILE_PATH_IO.actual)
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,staging
    ld de,staging+IMAGE_HEADER_CHECKSUM-1
    call bios_calcCS
    ld hl,(staging+IMAGE_HEADER_CHECKSUM)
    ld d,b
    ld e,c
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,(imageBytes)
    ld de,staging+IMAGE_HEADER_SIZE-1
    add hl,de
    ex de,hl
    ld hl,staging+IMAGE_HEADER_SIZE
    call bios_calcCS
    ld hl,(staging+12)
    ld d,b
    ld e,c
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,staging+IMAGE_HEADER_SIZE+32
    ld de,engineMagic
    ld b,5
engineMagicLoop:
    ld a,(de)
    cp (hl)
    jp nz,badImage
    inc hl
    inc de
    dec b
    jp nz,engineMagicLoop
    ld hl,(imageBytes)
    ex de,hl
    ld hl,(staging+IMAGE_HEADER_SIZE+40)
    call bios_cmp_hl_de
    jp nz,badImage
    ; Lower allocation is opaque to FONT. MEM requires page-sized blocks;
    ; bitmap geometry and the transformation belong to the initializer.
    ld hl,(staging+IMAGE_HEADER_SIZE+44)
    ld a,l
    or a
    jp nz,badImage
    xor a
    ret
badImage:
    ld a,DOS_ERR_BAD_IMAGE
    scf
    ret
needsMemory:
    ld a,MEM_ERROR_NOMEM
    scf
    ret
error:
    ld (errorCode),a
    ld hl,failed
    call bios_printStringZ
    ld a,(errorCode)
    call bios_printHexByte
    ld hl,errorSeparator
    call bios_printStringZ
    ld hl,errorMessages
errorFind:
    ld a,(hl)
    cp 0FFh
    jp z,errorUnknown
    ld b,a
    ld a,(errorCode)
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    cp b
    jp nz,errorFind
    ex de,hl
    jp errorExplain
errorUnknown:
    ld hl,msgUnknown
errorExplain:
    call bios_printStringZ
    ld hl,newline
    call bios_printStringZ
    ld a,(errorCode)
    scf
    ret
usage:
    ld hl,help
    jp bios_printStringZ

    IFDEF FONT_BENCH
    INCLUDE "../include/timing-pit.inc"
benchStamps: DS 20,0
    ENDIF
    INCLUDE "import.inc"
    INCLUDE "export.inc"
    INCLUDE "prepared-file.inc"
lowerBase: DW 0
magic: MXOS_IMAGE_SIGNATURE
    DB IMAGE_FLAG_MAIN
engineMagic:        DB 'FNT1',FONT_ENGINE_VERSION
extension:          DB '.fnt',0
help:               DB 'FONT NAME|PATH [-FILE] [-IMPORT FILE.PF [-OUT PATH]]',13,10,0
installed:          DB 'FONT ENGINE INSTALLED',13,10,0
failed:             DB 'FONT ERROR: ',0
errorSeparator:     DB ' - ',0
errorCode:          DB 0
; Public DOS and MEM status codes; keep the number for diagnostics.
errorMessages:
    DB DOS_ERR_IO
    DW msgIO
    DB DOS_ERR_NOT_FOUND
    DW msgMissing
    DB DOS_ERR_EXISTS
    DW msgExists
    DB DOS_ERR_FULL
    DW msgFull
    DB DOS_ERR_DIR_FULL
    DW msgDirFull
    DB DOS_ERR_ACCESS
    DW msgAccess
    DB DOS_ERR_ARGUMENT
    DW msgArgument
    DB DOS_ERR_CORRUPT
    DW msgCorrupt
    DB DOS_ERR_NO_DRIVER
    DW msgDriver
    DB DOS_ERR_UNSUPPORTED
    DW msgUnsupported
    DB DOS_ERR_BAD_IMAGE
    DW msgImage
    DB MEM_ERROR_BANK
    DW msgBank
    DB MEM_ERROR_STACK
    DW msgStack
    DB MEM_ERROR_RANGE
    DW msgRange
    DB MEM_ERROR_NOMEM
    DW msgMemory
    DB MEM_ERROR_OWNER
    DW msgOwner
    DB MEM_ERROR_HANDLE
    DW msgHandle
    DB MEM_ERROR_ACTIVE
    DW msgActive
    DB 0FFh
msgIO:              DB 'DISK READ OR WRITE FAILED',0
msgMissing:         DB 'ENGINE OR REQUIRED FILE NOT FOUND',0
msgExists:          DB 'OUTPUT FILE ALREADY EXISTS',0
msgFull:            DB 'DISK FULL',0
msgDirFull:         DB 'DIRECTORY FULL',0
msgAccess:          DB 'FILE ACCESS DENIED',0
msgArgument:        DB 'INVALID ARGUMENT, SIZE OR FORMAT',0
msgCorrupt:         DB 'DAMAGED FILESYSTEM',0
msgDriver:          DB 'DISK DRIVER NOT INSTALLED',0
msgUnsupported:     DB 'UNSUPPORTED FORMAT OR OPERATION',0
msgImage:           DB 'NOT A VALID FONT ENGINE',0
msgBank:            DB 'INVALID MEMORY BANK',0
msgStack:           DB 'BANK CALL STACK ERROR',0
msgRange:           DB 'MEMORY ADDRESS OUT OF RANGE',0
msgMemory:          DB 'NOT ENOUGH MAIN MEMORY',0
msgOwner:           DB 'MEMORY OWNER MISMATCH',0
msgHandle:          DB 'INVALID MEMORY HANDLE',0
msgActive:          DB 'MEMORY RESOURCE IS BUSY',0
msgUnknown:         DB 'UNKNOWN ERROR',0
newline:            DB 13,10,0
imageBytes:         DW 0
typedExtension:     DB 0
oldDrive:           DB 0
path:               DB BOOT_VOLUME_LETTER,':',05Ch,'FONTS',05Ch
relativeEnginePath:
    DS MAX_PATH-(relativeEnginePath-path),0
request: DW path,staging,FONT_ENGINE_END-FONT_ENGINE_BASE+IMAGE_HEADER_SIZE,0
    DS FILE_DESCRIPTOR,0
    INCLUDE "../include/file-io.inc"
    DS (-$-IMAGE_HEADER_SIZE) & 255,0 ; Body starts on a page boundary after its DLL header.
staging: DS FONT_ENGINE_END-FONT_ENGINE_BASE+IMAGE_HEADER_SIZE,0
preStage = $ ; Uninitialized MAIN workspace, not zeroes in FONT.COM on disk.
    ASSERT preStage <= FONT_IMPORT_ADDR
programEnd = FONT_INIT_END
    ASSERT programEnd <= 06400h
