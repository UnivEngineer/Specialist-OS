; Дескриптор оверлея содержит исполняемый JMP, чей операнд заполняет загрузчик.
OV_JUMP   = 2
OV_NAME   = OV_JUMP+3
OV_HOST   = OV_NAME+2
OV_BYTES  = OV_HOST+2
OV_READY  = OV_BYTES+1
OV_ACTIVE = OV_READY+1
OV_END    = OV_ACTIVE+1

;----------------------------------------------------------------------------
; MXOS
; Открыть согласованный с ядром оверлей, проверить привязку и опубликовать вход.
; вход: HL — дескриптор ENUM/FILEOPS в DOS, обработчик не выполняется.
; выход: CF=0 — готов к вызовам, CF=1/A — ошибка DOS. BC/DE/HL рабочие.
; Диск и каталоги восстановлены, готовое тело доступно до вытеснения.
;----------------------------------------------------------------------------
overlayEnsure:
    ld (ovRecord),hl
    ld e,OV_READY
    call ovByte
    or a
    ret nz
    ld hl,(v_directory)
    push hl
    ld a,(v_drive)
    push af
    ld hl,v_directories
    ld de,ovDirectories
    ld bc,16
    call moveBytesForward
    ld e,OV_NAME
    call ovWord
    ld de,v_curFileDescr
    call fileNamePrepare
    jp c,ovLoadDone
    ld hl,v_curFileDescr
    call imageOpenModule
    jp c,ovLoadDone
    ld a,(v_tmpFileDescr+5)
    cp IMAGE_FLAG_DRIVER
    jp nz,ovBadImage
    ; За JMP идёт таблица импортов, целиком внутри инициализированного тела.
    push bc
    ld e,OV_BYTES
    call ovByte
    add a,3
    ld e,a
    ld d,0
    pop hl
    call cmp_hl_de
    jp c,ovBadImage
    call imagePlaceDos
    jp c,ovLoadDone
    call cmp_hl_de
    jp nz,ovBadImage
    ld hl,(v_imageHeader+IMAGE_MEMORY_BEGIN)
    call cmp_hl_de
    jp nz,ovBadImage
    ld de,DOS_ARENA_START
    call cmp_hl_de
    jp c,ovBadImage
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    ld de,DOS_BANK_DRIVER_END
    call cmp_hl_de
    jp c,ovRangeReady
    jp nz,ovBadImage
ovRangeReady:
    ld (ovImageEnd),hl
    ex de,hl
    ld hl,(v_imageHeader+IMAGE_MEMORY_BEGIN)
    call imageClaimDos
    jp c,ovLoadDone
    ld hl,(ovRecord)
    ld de,OV_END
    add hl,de
    push hl
    ld hl,(ovImageEnd)
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    ld hl,(v_tmpFileDescr+6)
    ex de,hl
    call imageLoad
    jp c,ovLoadDone
    ; Пока ready=0, фасад не использует сохранённые адреса.
    ex de,hl
    ld hl,(ovRecord)
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    inc hl                   ; Пропустить код операции JMP.
    ld (hl),e
    inc hl
    ld (hl),d
    ld e,OV_BYTES
    call ovByte
    ld b,a
    ld e,OV_HOST
    call ovWord
    push hl
    ld e,0
    call ovWord
    inc hl
    inc hl
    inc hl
    ex de,hl
    pop hl
ovCheckHost:
    ld a,(de)
    cp (hl)
    jp nz,ovBadImage
    inc hl
    inc de
    dec b
    jp nz,ovCheckHost
    ld hl,(ovRecord)
    ld de,OV_READY
    add hl,de
    ld (hl),1
    xor a
    jp ovLoadDone
ovBadImage:
    ld a,DOS_ERR_BAD_IMAGE
    call dosSetError
ovLoadDone:
    push af
    ld a,(v_dosError)
    push af
    pop bc
    pop de
    pop af
    push de
    push bc
    call fileSelectDrive
    pop bc
    pop de
    pop hl
    ld (v_directory),hl
    push de
    push bc
    ld hl,ovDirectories
    ld de,v_directories
    ld bc,16
    call moveBytesForward
    pop bc
    pop de
    ld a,b
    ld (v_dosError),a
    push de
    pop af
    ret
ovAddress:
    ld hl,(ovRecord)
    ld d,0
    add hl,de
    ret
ovByte:
    call ovAddress
    ld a,(hl)
    ret
ovWord:
    call ovAddress
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    ret
overlayBusy:
    ld a,MEM_ERROR_ACTIVE
    jp dosSetError

fileopsName: DB BOOT_VOLUME_LETTER,':',05Ch,"DOS",05Ch,"FILEOPS.DLL",0
fileopsHost:
    INCLUDE "fileops-host.inc"
fileopsHostEnd:
    ASSERT fileopsHostEnd-fileopsHost < 256

enumName: DB BOOT_VOLUME_LETTER,':',05Ch,"DOS",05Ch,"ENUM.DLL",0
enumHost:
    INCLUDE "enumerate-host.inc"
enumHostEnd:
    ASSERT enumHostEnd-enumHost < 256
