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
    ld a,1
    ld (shortName),a
    ; An explicit drive path bypasses the A:\FONTS prefix.
    push hl
    inc hl
    ld a,(hl)
    pop hl
    cp ':'
    ld de,path+9
    jp nz,fontPathReady
    ld de,path
fontPathReady:
    ld b,MAX_PATH-18          ; prefix + optional size directory + .fnt + NUL
    xor a
    ld (hasExtension),a
copyArgument:
    ld a,(hl)
    or a
    jp z,pathDone
    cp ' '
    jp z,pathDone
    cp '/'
    jp nz,notSlash
    ld a,05Ch
notSlash:
    cp 05Ch
    jp z,qualifiedName
    cp ':'
    jp nz,nameCharacter
qualifiedName:
    push af
    xor a
    ld (shortName),a
    pop af
nameCharacter:
    cp '.'
    jp nz,notDot
    ld (hasExtension),a
notDot:
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,copyArgument
    jp usage
pathDone:
    ld (argTail),hl
    ld a,(hasExtension)
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
    call bios_fileGetSetDrive
    ld (oldDrive),a
    ld hl,request
    FONT_STAMP 0
    call loadEngine
    FONT_STAMP 2
    push af
    ld a,(oldDrive)
    call bios_fileSelectDrive
    pop af
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
    ld hl,(staging+60)
    ld bc,0100h+MEM_RESIDENT_FONT
    ld a,MEM_OP_RESIDENT
    MXOS_CALL MXOS_DOS_MEMORY
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
    ld de,staging+16
    add hl,de
    ex de,hl
    ld hl,staging+16
    ld bc,FONT_ENGINE_BASE
    call bios_memcpy_bc_hl
    FONT_STAMP 12
    ld hl,(lowerBase)
    ex de,hl
    call FONT_ENGINE_INIT
    FONT_STAMP 14
committed:
    call saveImported
    jp c,error
    ld a,(preparedError)
    or a
    jp nz,error
    ld hl,installed
    jp bios_printStringZ
validate:
    ld hl,(request+FILE_PATH_IO.actual)
    ld de,64
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
    ld de,16
    add hl,de
    ex de,hl
    ld hl,(request+FILE_PATH_IO.actual)
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,staging
    ld de,staging+13
    call bios_calcCS
    ld hl,(staging+14)
    ld d,b
    ld e,c
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,(imageBytes)
    ld de,staging+15
    add hl,de
    ex de,hl
    ld hl,staging+16
    call bios_calcCS
    ld hl,(staging+12)
    ld d,b
    ld e,c
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,staging+48
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
    ld hl,(staging+56)
    call bios_cmp_hl_de
    jp nz,badImage
    ; Lower allocation is opaque to FONT. MEM requires page-sized blocks;
    ; bitmap geometry and the transformation belong to the initializer.
    ld hl,(staging+60)
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

; Bare names search A:\FONTS, then the three cell-size directories.
; Qualified paths are literal. Only NOT_FOUND permits another candidate:
; malformed images and I/O failures must retain their actual error.
loadEngine:
    call bios_fileLoadPath
    ret nc
    cp DOS_ERR_NOT_FOUND
    jp nz,loadEngineFailed
    ld a,(shortName)
    or a
    jp z,loadEngineMissing
    ; Insert four bytes before the basename, including its NUL in the move.
    ld hl,path+9
    ld b,1
loadNameEnd:
    ld a,(hl)
    or a
    jp z,moveNameStart
    inc hl
    inc b
    jp loadNameEnd
moveNameStart:
    ld d,h
    ld e,l
    inc de
    inc de
    inc de
    inc de
moveNameBack:
    ld a,(hl)
    ld (de),a
    dec hl
    dec de
    dec b
    jp nz,moveNameBack
    ld hl,03878h             ; 'x8'
    ld (path+10),hl
    ld a,05Ch
    ld (path+12),a
    ld a,'6'
    call loadSizeDirectory
    ret nc
    cp DOS_ERR_NOT_FOUND
    jp nz,loadEngineFailed
    ld a,'8'
    call loadSizeDirectory
    ret nc
    cp DOS_ERR_NOT_FOUND
    jp nz,loadEngineFailed
    ld a,'4'
loadSizeDirectory:
    ld (path+9),a
    ld hl,request
    jp bios_fileLoadPath
loadEngineMissing:
    ld a,DOS_ERR_NOT_FOUND
loadEngineFailed:
    scf
    ret
    IFDEF FONT_BENCH
    INCLUDE "../include/timing-pit.inc"
benchStamps: DS 20,0
    ENDIF
    INCLUDE "import.inc"
    INCLUDE "export.inc"
    INCLUDE "prepared-file.inc"
lowerBase: DW 0
magic: DB 0CDh,020h,'M','X',1,IMAGE_FLAG_MAIN
engineMagic: DB 'FNT1',FONT_ENGINE_VERSION
extension: DB '.fnt',0
help: DB 'FONT NAME|PATH [-FILE] [-IMPORT FILE.PF [-OUT PATH]]',13,10,0
installed: DB 'FONT ENGINE INSTALLED.',13,10,0
failed: DB 'FONT ERROR: ',0
errorSeparator: DB ' - ',0
errorCode: DB 0
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
msgIO: DB 'DISK READ OR WRITE FAILED',0
msgMissing: DB 'ENGINE OR REQUIRED FILE NOT FOUND',0
msgExists: DB 'OUTPUT FILE ALREADY EXISTS',0
msgFull: DB 'DISK FULL',0
msgDirFull: DB 'DIRECTORY FULL',0
msgAccess: DB 'FILE ACCESS DENIED',0
msgArgument: DB 'INVALID ARGUMENT, SIZE OR FORMAT',0
msgCorrupt: DB 'DAMAGED FILESYSTEM',0
msgDriver: DB 'DISK DRIVER NOT INSTALLED',0
msgUnsupported: DB 'UNSUPPORTED FORMAT OR OPERATION',0
msgImage: DB 'NOT A VALID FONT ENGINE',0
msgBank: DB 'INVALID MEMORY BANK',0
msgStack: DB 'BANK CALL STACK ERROR',0
msgRange: DB 'MEMORY ADDRESS OUT OF RANGE',0
msgMemory: DB 'NOT ENOUGH MAIN MEMORY',0
msgOwner: DB 'MEMORY OWNER MISMATCH',0
msgHandle: DB 'INVALID MEMORY HANDLE',0
msgActive: DB 'MEMORY RESOURCE IS BUSY',0
msgUnknown: DB 'UNKNOWN ERROR',0
newline: DB 13,10,0
imageBytes: DW 0
hasExtension: DB 0
shortName: DB 0
oldDrive: DB 0
path: DB 'A:',05Ch,'FONTS',05Ch
    DS MAX_PATH-9,0
request: DW path,staging,FONT_ENGINE_END-FONT_ENGINE_BASE+16,0
    DS FILE_DESCRIPTOR,0
    DS (-$-16) & 255,0 ; Body starts on a page boundary after its DLL header.
staging: DS FONT_ENGINE_END-FONT_ENGINE_BASE+16,0
preStage = $ ; Uninitialized MAIN workspace, not zeroes in FONT.COM on disk.
    ASSERT preStage <= FONT_IMPORT_ADDR
programEnd = FONT_INIT_END
    ASSERT programEnd <= 06400h
