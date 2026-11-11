; Маршрутизация основного клиента в DOS. Флаг устанавливает загрузчик
; только после установки хвостов/стеков и успешной инициализации DOS.
; Нулевой флаг оставляет штатную сборку с DOS в основной памяти.
    INCLUDE "../mem/abi.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"

; B=группа; Z=0 только для готового банкового маршрута DOS. AF разрушен.
sysBankSelected:
    ld a,b
    cp MXOS_GROUP_DOS
    jp z,sysBankCheck
    xor a
    ret
sysBankCheck:
    ld a,(v_dosBanked)
    or a
    ret

; BC=операция, DE=MX_REGS в main. Пакет и результаты как у sysCall.
; Транспорт сохраняет SP/страницу/owner/home; неподдержанный запрос
; не изменяет пакет, как и основной диспетчер. AF/BC/DE/HL — результат.
sysBankCall:
    ld h,d
    ld l,e
    push bc
    ld bc,MX_REGS
    add hl,bc
    pop bc
    jp c,sysBadPacket
    ld a,h
    cp 0F8h
    jp c,sysBankPacketReady
    jp nz,sysBadPacket
    ld a,l
    or a
    jp nz,sysBadPacket
sysBankPacketReady:
    jp sysRawRoute
sysBankDoCall:
    push de
    ld a,1
    ld hl,BANK_DOS_GATE
    call bankCallPacket
    push af
    ld a,(BANK_FRAME_STATUS)
    or a
    jp nz,sysBankRejected
    pop af
    jp sysReturned
sysBankRejected:
    pop af
    inc sp
    inc sp
    ret
v_dosBanked: DB 0
