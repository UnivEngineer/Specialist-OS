; Компактные имена и метаданные: NEAR-объекты принадлежат клиенту.
; Вход/выход — kdRegs, AF/BC/DE/HL по исходному контракту функции.
; Указатели в возвращаемых регистрах переводятся обратно в страницу клиента.
kmInfo:
    ld hl,fileLoadInfo
    ld bc,(FILE_DESCRIPTOR << 8)+1
    jp kmDescriptor
kmAttr:
    ld hl,fileGetSetAttr
    ld bc,(FILE_NAME_LENGTH+3) << 8
    jp kmDescriptor
kmDelete:
    ld hl,fileDelete
    ld bc,(FILE_NAME_LENGTH+3) << 8
kmDescriptor:
    ld (kdEntry),hl
    ld a,c
    ld (kmWrite),a
    ld a,b
    ld (kmSize),a
    ld c,b
    ld b,0
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld de,KD_OBJECT
    call kdFetch
    jp c,kdBad
    ld hl,KD_OBJECT
    ld (kdRegs),hl
    call kdInvoke
    call c,dosGetError
    call kdCapture
    ld a,(kmWrite)
    or a
    jp z,kdRestorePointer
    ld a,(kmSize)
    ld c,a
    ld b,0
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,KD_OBJECT
    call kdStore
    jp c,kdBad
    jp kdRestorePointer

kmRename:
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld de,KD_OBJECT
    ld bc,FILE_NAME_LENGTH+3
    call kdFetch
    jp c,kdBad
    ld hl,(kdRegs+2)
    ld (kmSecond),hl
    ld de,KD_PATH
    call kdFetch
    jp c,kdBad
    ld hl,fileRename
    ld (kdEntry),hl
    ld hl,KD_OBJECT
    ld (kdRegs),hl
    ld hl,KD_PATH
    ld (kdRegs+2),hl
    call kdInvoke
    call c,dosGetError
    call kdCapture
    ld hl,(kmSecond)
    ld (kdRegs+2),hl
    jp kdRestorePointer

; fileNamePrepare пишет ровно 11 байт, а при ошибке только первый ноль.
; Остальные байты выходного буфера сохраняются, включая отказ разбора пути.
kmName:
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld de,KD_PATH
    ld bc,MAX_PATH
    call kdFetch
    jp c,kdBad
    ld hl,KD_PATH
    ld b,MAX_PATH
kmNameScan:
    ld a,(hl)
    or a
    jp z,kmNameReady
    inc hl
    dec b
    jp nz,kmNameScan
    jp kdBad
kmNameReady:
    ld hl,(kdRegs+2)
    ld (kmSecond),hl
    ld de,KD_OBJECT
    ld bc,FILE_NAME_LENGTH+3
    call kdFetch
    jp c,kdBad
    ld hl,fileNamePrepare
    ld (kdEntry),hl
    ld hl,KD_PATH
    ld (kdRegs),hl
    ld hl,KD_OBJECT
    ld (kdRegs+2),hl
    call kdInvoke
    call c,dosGetError
    call kdCapture
    ld hl,(kdRegs)
    ld de,KD_PATH
    call sub_hl_de
    ex de,hl
    ld hl,(kdClientObject)
    add hl,de
    ld (kdRegs),hl
    ld hl,(kmSecond)
    ld (kdRegs+2),hl
    ex de,hl
    ld hl,KD_OBJECT
    ld bc,FILE_NAME_LENGTH+3
    call kdStore
    jp c,kdBad
    jp kdFinish

; Массив номеров дисков — максимум 8 байт; возвращаем только заполненные.
kmDrives:
    ld hl,(kdRegs+4)
    ld a,h
    or a
    jp nz,kmDrivesSize
    ld a,l
    cp 8
    jp c,kdBad
kmDrivesSize:
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld bc,8
    call kdRange
    jp c,kdBad
    ld hl,KD_OBJECT
    call fileGetDrives
    call c,dosGetError
    call kdCapture
    ld a,(kdRegs+6)
    and 1
    jp nz,kdRestorePointer
    ld a,(kdRegs+5)
    ld c,a
    ld b,0
    ld hl,(kdClientObject)
    add hl,bc
    ld (kdRegs),hl
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,KD_OBJECT
    call kdStore
    jp c,kdBad
    jp kdFinish
kmWrite: DB 0
kmSize: DB 0
kmSecond: DW 0
