; COPY/MOVE: HL=FILE_COPY_REQUEST клиента, результат CF/A как у общего CLI.
; Междисковый обмен остаётся в DOS. Буфер клиента проверяется по контракту,
; рабочая пачка локальная, до 4096 байт; публичные адрес/ёмкость сохранены.
kcCopy:
    xor a
    ld hl,fileCopy
    jp kcPrepare
kcMove:
    ld a,1
    ld hl,cliMoveRequest
kcPrepare:
    ld (kcMode),a
    ld (kdEntry),hl
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld de,KD_OBJECT
    ld bc,FILE_COPY_REQUEST
    call kdFetch
    jp c,kcBad
    ld hl,(KD_OBJECT+FILE_COPY_REQUEST.buffer)
    ld (kcBuffer),hl
    ld hl,(KD_OBJECT+FILE_COPY_REQUEST.bufferSize)
    ld (kcSize),hl
    ld b,h
    ld c,l
    ; Перенос записи внутри одного тома не использует буфер данных.
    ld a,(kcMode)
    or a
    jp z,kcCheckBuffer
    ld hl,KD_OBJECT+FILE_COPY_REQUEST.sourceDrive
    ld a,(hl)
    inc hl
    cp (hl)
    jp z,kcBufferReady
kcCheckBuffer:
    ld a,b
    or a
    jp z,kcBad
    ld hl,(kcBuffer)
    call kdRange
    jp c,kcBad
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,(kcBuffer)
    ld a,FILE_COPY_REQUEST
    call kdOverlap
    jp c,kcBad
kcBufferReady:
    ld hl,(KD_OBJECT+FILE_COPY_REQUEST.onExists)
    ld (kcHook),hl
    ld bc,1
    call kdRange
    jp c,kcBad
    ld a,h
    or l
    ld hl,0
    jp z,kcHookReady
    ld hl,kcConfirm
kcHookReady:
    ld (KD_OBJECT+FILE_COPY_REQUEST.onExists),hl
    ld hl,KD_DATA
    ld (KD_OBJECT+FILE_COPY_REQUEST.buffer),hl
    ld hl,(kcSize)
    ld a,h
    cp KD_CHUNK/256
    jp c,kcSizeReady
    ld hl,KD_CHUNK
kcSizeReady:
    ld (KD_OBJECT+FILE_COPY_REQUEST.bufferSize),hl
    ld hl,KD_OBJECT
    ld (kdRegs),hl
    call kdInvoke
    call kdCapture
    ld hl,KD_OBJECT
    call kcStoreRequest
    jp c,kcBad
    jp kdRestorePointer
kcBad:
    call fileOpError
    jp kdError

; HL=текущий запрос общего COPY, в том числе дочерний запрос дерева.
; Передаём снимок клиенту с его исходными NEAR-полями. AF/BC/DE/HL разрушены.
kcStoreRequest:
    ld de,KD_PATH
    ld bc,FILE_COPY_REQUEST
    call moveBytesForward
    ld hl,(kcBuffer)
    ld (KD_PATH+FILE_COPY_REQUEST.buffer),hl
    ld hl,(kcSize)
    ld (KD_PATH+FILE_COPY_REQUEST.bufferSize),hl
    ld hl,(kcHook)
    ld (KD_PATH+FILE_COPY_REQUEST.onExists),hl
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,KD_PATH
    ld bc,FILE_COPY_REQUEST
    jp kdStore

; HL=локальный запрос; callback видит снимок по исходному адресу клиента.
; Менять запрос в обработчике нельзя; ответ — только A и CF.
kcConfirm:
    call kcStoreRequest
    ret c
    ld hl,(kdClientObject)
    ld (kcRegs),hl
    ld hl,(kcHook)
    ld b,h
    ld c,l
    ld a,(kdPage)
    ld hl,bankCallback
    ld de,kcRegs
    jp bankCallPacket
kcMode: DB 0
kcBuffer: DW 0
kcSize: DW 0
kcHook: DW 0
kcRegs: DS MX_REGS,0
