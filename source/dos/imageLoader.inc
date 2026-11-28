;----------------------------------------------------------------------------
; MXOS
; Открыть нативный образ и проверить заголовок и размеры до размещения.
; вход: HL = имя/дескриптор. imageOpen допускает v2, imageOpenModule также v3.
; выход: CF=0, HL=вход, DE=загрузка, BC=длина тела (для v3 адреса относительные).
; При CF=1 причину содержит v_dosError. AF изменён. Курсор жив до следующего
; файлового запроса, imageSaveContext/imageRestoreContext сохраняют его.
;----------------------------------------------------------------------------
imageOpen:
    IFDEF BANKED_DOS_BUILD
    xor a
    jp imageOpenKind
imageOpenModule:
    ld a,1
imageOpenKind:
    ld (relocAccepted),a
    ENDIF
    call imageOpenHeader
    ret c
imageOpenLength:
    ld a,(v_imageHeader+29)
    or a
    jp nz,imageError
    ld de,IMAGE_HEADER_SIZE
    IFDEF BANKED_DOS_BUILD
    ld a,(v_imageHeader+4)
    cp IMAGE_RELOC_VERSION
    jp nz,imageBodyLength
    ld hl,(v_imageHeader+IMAGE_RELOC_COUNT)
    add hl,hl
    jp c,imageError
    ld de,IMAGE_RELOC_HEADER
    add hl,de
    jp c,imageError
    ex de,hl
    ENDIF
imageBodyLength:
    ld hl,(v_tmpFileDescr+10)
    jp imageLength
imageLength:
    ld a,h
    or l
    jp z,imageError
    add hl,de
    jp c,imageError
    push hl
    ld hl,(v_tmpFileDescr+28)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp nz,imageError
    ld hl,(v_tmpFileDescr+6)
    push hl
    ld hl,(v_tmpFileDescr+10)
    ex de,hl
    pop hl
    add hl,de
    jp c,imageError
    push hl
    ld hl,(v_tmpFileDescr+8)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp c,imageError
    jp z,imageError
    ld hl,(v_tmpFileDescr+6)
    ex de,hl
    call cmp_hl_de
    jp c,imageError
    ; Reserve the entire declared interval, including workspace below load.
    ld hl,(v_imageHeader+IMAGE_MEMORY_BEGIN)
    push hl
    ld hl,(v_tmpFileDescr+6)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp c,imageBeginReady
    jp nz,imageError
imageBeginReady:
    ld hl,(v_tmpFileDescr+6)
    push hl
    ld hl,(v_tmpFileDescr+10)
    ex de,hl
    pop hl
    add hl,de
    push hl
    ld hl,(v_tmpFileDescr+IMAGE_BSS)
    ex de,hl
    pop hl
    ld a,d
    and e
    inc a
    jp z,imageEndCheck
    add hl,de
    jp c,imageError
imageEndCheck:
    push hl
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp c,imageRequirements
    jp nz,imageError
imageRequirements:
    ex de,hl
    push hl
    ld hl,(v_imageHeader+IMAGE_MIN_HEAP)
    ex de,hl
    pop hl
    add hl,de
    jp c,imageError
imageResult:
    ld hl,(v_tmpFileDescr+6)
    ex de,hl
    ld hl,(v_tmpFileDescr+10)
    ld b,h
    ld c,l
    ld hl,(v_tmpFileDescr+8)
    or a
    ret
    IFDEF BANKED_DOS_BUILD
; CF=1 при ошибке чтения или проверки заголовка. При CF=0 значение A=0
; обозначает нативный образ, A=1 передаёт запуск программы совместимости в RUN.
imageClassify:
    ld a,1
    ld (relocAccepted),a
    call imageOpenHeader
    ret c
    ld a,(v_imageHeader+5)
    or a
    jp z,imageLegacy
    cp IMAGE_FLAG_MAIN
    jp nz,imageNative
    ld a,(v_imageHeader+IMAGE_MODE)
    cp IMAGE_MODE_STD
    jp nc,imageLegacy
imageNative:
    xor a
    ret
imageLegacy:
    ld a,1
    or a
    ret

imageOpenExec:
    call imageClassify
    ret c
    or a
    jp nz,imageError
    jp imageOpenLength
    ENDIF

imageOpenHeader:
    call fileLoadInfo
    ret c
    ld hl,(v_tmpFileDescr+30)
    ld a,h
    or l
    jp nz,imageError
    ld de,IMAGE_HEADER_SIZE+1
    ld hl,(v_tmpFileDescr+28)
    call cmp_hl_de
    jp c,imageError
    call imageReadPrefix
    ret c
    ld de,v_tmpFileDescr
    ld bc,16
    call moveBytesForward
    jp imageValidateHeader

imageError:
    ld a,DOS_ERR_BAD_IMAGE
    jp dosSetError

; Заголовок читается общим потоком; курсор сохраняется отдельно от кэша.
imageReadPrefix:
    ld hl,0
    ld (v_streamOffset),hl
    ld hl,(v_tmpFileDescr+26)
    ex de,hl
    ld bc,IMAGE_HEADER_SIZE
    ld hl,v_imageHeader
    ld a,2
    call fatStream
    ret c
    IFDEF BANKED_DOS_BUILD
    ld a,(v_imageHeader+4)
    cp IMAGE_RELOC_VERSION
    jp nz,imagePrefixDone
    ld a,(relocAccepted)
    or a
    jp z,imageError
    ld bc,IMAGE_RELOC_HEADER-IMAGE_HEADER_SIZE
    ld hl,v_imageHeader+IMAGE_HEADER_SIZE
    ld a,2
    call fatStream
    ret c
imagePrefixDone:
    ENDIF
    call imageSaveCursor
    ld hl,v_imageHeader
    or a
    ret
imageSaveCursor:
    ex de,hl
    ld (v_tmpFileDescr+26),hl
    ld hl,(v_streamOffset)
    ld (v_tmpFileDescr+20),hl
    ret
;----------------------------------------------------------------------------
; MXOS
; Прочитать тело нативного файла и применить таблицу v3 один раз.
; вход: после открытия и размещения DE = локальный приёмник либо KD_DATA
; при kdFarActive (kdPage:kdFarCurrent). Область целиком принадлежит клиенту.
; выход: CF=0, HL=фактический вход. При CF=1 A=ошибка, содержимое приватного
; резерва неопределено. AF/BC/DE изменены. Вызов точки входа выполняет клиент.
;----------------------------------------------------------------------------
imageLoad:
    ex de,hl
    ld (v_input_end),hl
    push hl
    ld hl,(v_tmpFileDescr+20)
    ld (v_streamOffset),hl
    ld hl,(v_tmpFileDescr+10)
    ld b,h
    ld c,l
    ld hl,(v_tmpFileDescr+26)
    ex de,hl
    pop hl
    ld a,2
    call fatStream
    ret c
    IFDEF BANKED_DOS_BUILD
    ld a,(v_imageHeader+4)
    cp IMAGE_RELOC_VERSION
    jp z,imageBodyDone
    ENDIF
    call fatStreamFinish
    ret c
imageBodyDone:
    push hl
    call imageSaveCursor
    pop hl
    IFDEF BANKED_DOS_BUILD
    ld a,(kdFarActive)
    or a
    jp z,imageLocalSum
    call kiLoadSum
    jp imageBodyChecked
imageLocalSum:
    ENDIF
    dec hl
    ex de,hl
    ld hl,(v_input_end)
    call calcCS
    IFDEF BANKED_DOS_BUILD
    call imageCheckSum
imageBodyChecked:
    ret c
    ld a,(v_imageHeader+4)
    cp IMAGE_RELOC_VERSION
    jp z,relocReadTable
    or a
    ret
    ENDIF
imageCheckSum:
    ld hl,(v_tmpFileDescr+12)
    ld d,b
    ld e,c
    call cmp_hl_de
    jp nz,imageError
    ld hl,(v_tmpFileDescr+8)
    or a
    ret
; v_imageHeader использует временный хвост dosModuleName (dosModule.inc).
imageMagic:
    MXOS_IMAGE_SIGNATURE
    DB 0

; Full header remains separate from the FAT descriptor and its stream cursor.
imageValidateHeader:
    ld hl,v_imageHeader
    ld de,imageMagic
    ld b,4
imageMagicLoop:
    ld a,(de)
    cp (hl)
    jp nz,imageError
    inc hl
    inc de
    dec b
    jp nz,imageMagicLoop
    ld a,(hl)
    cp 2
    IFDEF BANKED_DOS_BUILD
    jp z,imageVersionReady
    cp IMAGE_RELOC_VERSION
    ENDIF
    jp nz,imageError
imageVersionReady:
    inc hl
    ld a,(hl)
    cp IMAGE_FLAG_MAIN
    jp z,imageFlagsReady
    cp IMAGE_FLAG_BANK+1
    jp nc,imageError
imageFlagsReady:
    IFDEF BANKED_DOS_BUILD
    ld a,(v_imageHeader+4)
    cp IMAGE_RELOC_VERSION
    jp nz,imageFixedHeader
    call relocCheckHeader
    ret c
    jp imageCommonHeader
imageFixedHeader:
    ENDIF
    ld hl,v_imageHeader
    ld de,v_imageHeader+IMAGE_HEADER_CHECKSUM-1
    call calcCS
    ld hl,(v_imageHeader+IMAGE_HEADER_CHECKSUM)
    ld d,b
    ld e,c
    call cmp_hl_de
    jp nz,imageError
    ld a,(v_imageHeader+IMAGE_HEADER_BYTES)
    cp IMAGE_HEADER_SIZE
    jp nz,imageError
imageCommonHeader:
    ld a,(v_imageHeader+IMAGE_POLICY)
    cp IMAGE_POLICY_TRANSIENT+1
    jp nc,imageError
    cp IMAGE_POLICY_TRANSIENT
    jp nz,imageMemoryPolicyReady
    ld a,(v_imageHeader+4)
    cp 2
    jp nz,imageError
    ld a,(v_imageHeader+5)
    cp IMAGE_FLAG_MAIN
    jp nz,imageError
imageMemoryPolicyReady:
    ld a,(v_imageHeader+IMAGE_API_VERSION)
    or a
    jp z,imageApiReady
    cp 2
    jp nz,imageError
imageApiReady:
    ld a,(v_imageHeader+IMAGE_OPTIONS)
    cp 2
    jp z,imageError
    cp 4
    jp nc,imageError
    ld a,(v_imageHeader+IMAGE_MODE)
    cp 5
    jp nc,imageError
    ld hl,(v_imageHeader+IMAGE_BSS)
    ld a,h
    and l
    inc a
    ret nz
    ld a,(v_imageHeader+IMAGE_POLICY)
    or a
    ret z
    jp imageError


imageSaveContext:
    IFDEF BANKED_DOS_BUILD
    ld hl,v_imageHeader+IMAGE_HEADER_SIZE
    ld de,relocSavedExtension
    ld bc,IMAGE_RELOC_HEADER-IMAGE_HEADER_SIZE
    call moveBytesForward
    ENDIF
    ld a,(v_drive)
    ld (imageSavedDrive),a
    ld hl,(v_directory)
    ld (imageSavedDirectory),hl
    ld hl,v_directories
    ld de,imageSavedDirectories
    ld bc,16
    call moveBytesForward
    ld hl,v_tmpFileDescr
    ld de,imageSavedContext
    ld bc,32
    call moveBytesForward
    ld hl,v_imageHeader
    ld de,imageSavedContext+32
    ld bc,IMAGE_HEADER_SIZE
    jp moveBytesForward
imageRestoreContext:
    IFDEF BANKED_DOS_BUILD
    ld hl,relocSavedExtension
    ld de,v_imageHeader+IMAGE_HEADER_SIZE
    ld bc,IMAGE_RELOC_HEADER-IMAGE_HEADER_SIZE
    call moveBytesForward
    ENDIF
    ld a,(imageSavedDrive)
    call fileSelectDrive
    ld hl,(imageSavedDirectory)
    ld (v_directory),hl
    ld hl,imageSavedDirectories
    ld de,v_directories
    ld bc,16
    call moveBytesForward
    ld hl,imageSavedContext
    ld de,v_tmpFileDescr
    ld bc,32
    call moveBytesForward
    ld hl,imageSavedContext+32
    ld de,v_imageHeader
    ld bc,IMAGE_HEADER_SIZE
    jp moveBytesForward
    IFDEF BANKED_DOS_BUILD
imageSavedContext     = DOS_IMAGE_CONTEXT_ADDR
imageSavedDrive       = imageSavedContext+64
imageSavedDirectory   = imageSavedContext+65
imageSavedDirectories = imageSavedContext+67
    ASSERT imageSavedDirectories+16 == DOS_IMAGE_HEADER_ADDR
    ELSE
imageSavedContext:      DS 32+IMAGE_HEADER_SIZE,0
imageSavedDrive:        DB 0
imageSavedDirectory:    DW 0
imageSavedDirectories:  DS 16,0
    ENDIF
