; COPY/MOVE: HL=FILE_COPY_REQUEST клиента, результат CF/A как у общего CLI.
; Междисковый обмен использует собственную пачку DOS на 4096 байт.
; Поля buffer/bufferSize внешнего запроса не используются, возвращаются нулями.
kcCopy:
    ld hl,fileCopy
    jp kcPrepare
kcMove:
    ld hl,cliMoveRequest
kcPrepare:
    ld (kdEntry),hl
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld de,KD_OBJECT
    ld bc,FILE_COPY_REQUEST
    call kdFetch
    jp c,kcBad
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
    ld hl,KD_CHUNK
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
    ld hl,0
    ld (KD_PATH+FILE_COPY_REQUEST.buffer),hl
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
kcHook: DW 0
kcRegs: DS MX_REGS,0
