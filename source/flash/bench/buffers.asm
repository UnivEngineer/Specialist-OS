; Только стенд DiskBen RAW: BANK-буфер 2001h, scratch 9000h, драйвер B000h.
; BENCH_DOS=0: локальный memcpy moveBytesForward; =1: DOS B600h + memcpy_far.
; Адреса измерителя и транспорта задаёт хост из замороженных SYM.
    INCLUDE "../../include/mxos.inc"
    ORG 0A000h
entry:
    push bc
    push de
    push hl
    ld (userBuffer),hl
    ex de,hl
    ld (sector),hl
    ex de,hl
    ld a,b
    ld (operation),a
    cp 4
    jp z,physicalOnly
    cp 1
    jp z,write
    cp 2
    jp z,read
    ld a,DOS_ERR_ARGUMENT
    scf
    jp done
write:
    IF BENCH_DOS
    ld (copyToDos+1),hl
    ld hl,copyToDos
    call mx_memcpyFar
    jp c,done
    ELSE
    ld hl,(userBuffer)
    ld de,09000h
    ld bc,256
    call moveBytesForward
    ENDIF
physicalOnly:
    call physical
    jp done
read:
    call physical
    jp c,done
    IF BENCH_DOS
    ld hl,(userBuffer)
    ld (copyFromDos+4),hl
    ld hl,copyFromDos
    call mx_memcpyFar
    ELSE
    ld hl,(userBuffer)
    ex de,hl
    ld hl,09000h
    ld bc,256
    call moveBytesForward
    xor a
    ENDIF
done:
    pop hl
    pop de
    pop bc
    ret
physical:
    ld hl,(sector)
    ex de,hl
    ld a,(operation)
    ld b,a
    ld c,7
    IF BENCH_DOS
    ld a,DOS_BANK_PAGE
    ld hl,DOS_BANK_DRIVER_ADDR
    jp TRANSPORT_BANK_CALL
    ELSE
    ld hl,09000h
    jp 0B000h
    ENDIF

; Тестовые замены двух нетактуемых циклов DiskBen. Обрабатывают ровно 256
; байт даже при L!=0, в отличие от прежнего перебора только младшего байта.
fill:
    ld hl,(WORKER_BUFFER_POINTER)
    ld a,(WORKER_OPERATION)
    cp 1
    ld c,0
    jp z,fillReady
    dec c
fillReady:
    ld b,0
fillByte:
    ld a,(WORKER_PASS)
    xor l
    xor c
    ld (hl),a
    inc hl
    dec b
    jp nz,fillByte
    ret
validate:
    ld b,0
validateByte:
    ld a,(WORKER_PASS)
    xor l
    cp (hl)
    jp nz,WORKER_MISMATCH
    inc hl
    dec b
    jp nz,validateByte
    jp WORKER_VALIDATE_RESUME
userBuffer:     DW 0
sector:         DW 0
operation:      DB 0
copyToDos:
    DB 0
    DW 0
    DB DOS_BANK_PAGE
    DW DOS_VERIFY_BUFFER,256
copyFromDos:
    DB DOS_BANK_PAGE
    DW DOS_VERIFY_BUFFER
    DB 0
    DW 0,256
    INCLUDE "../../common/move8080.inc"
end:
    ASSERT $ <= 0B000h
    END
