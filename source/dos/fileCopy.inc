; Resident facade. FILEOPS owns the recursive engine in the shared DOS arena.
; No user data or on-disk formats are changed by loading the engine.
cliDeleteTree:
    call fileFindAny
    ret c
    ld hl,v_tmpFileDescr
    ld de,cliRequest.source
    ld bc,FILE_DESCR_SIZE
    call cliCopyBytes
    ld a,(v_drive)
    ld (cliRequest.sourceDrive),a
    ld hl,(v_directory)
    ld (cliRequest.sourceDirectory),hl
    ld hl,cliRequest
    ld a,11
    call fileopsRun
    ret nc
    jp dosIoError

;----------------------------------------------------------------------------
; MXOS
;
; Копировать объект через загружаемый FILEOPS.
;
; вход:
;   hl = адрес подготовленного FILE_COPY_REQUEST в доступной ядру памяти. Буфер обмена
;   и запрос не перекрываются.
;
; выход:
;   cf = 0 и a = 0 при успехе, при cf = 1 регистр a различает ошибку и существующее
;   назначение. af, bc, de и hl рабочие.
;----------------------------------------------------------------------------

fileCopy:
    xor a
    jp fileopsRun

;----------------------------------------------------------------------------
; MXOS
;
; Запустить обработчик FILEOPS в режиме перемещения.
;
; вход:
;   hl = адрес подготовленного FILE_COPY_REQUEST в доступной ядру памяти. Буфер обмена
;   и запрос не перекрываются.
;
; выход:
;   cf = 0 и a = 0 при успехе, при cf = 1 регистр a различает ошибку и существующее
;   назначение. af, bc, de и hl рабочие.
;----------------------------------------------------------------------------

fileCopyForMove:
    ld a,1
fileopsRun:
    IFDEF BANKED_DOS_BUILD
    push af
    push hl
    call fileopsEnsure
    pop hl
    pop bc
    jp nc,fileopsRunReady
    ld a,FILE_OP_ERROR
    ret
fileopsRunReady:
    ld a,b
    inc a
    ld (fileopsActive),a
    ld a,b
    call DOS_FILEOPS_ADDR
    push af
    xor a
    ld (fileopsActive),a
    pop af
    ret
    ELSE
    jp blockArgument
    ENDIF

    IFDEF BANKED_DOS_BUILD
fileopsEnsure:
    ld a,(fileopsActive)
    or a
    jp nz,overlayBusy
    ld hl,fileopsOverlay
    jp overlayEnsure
    INCLUDE "overlays.inc"
    ENDIF
