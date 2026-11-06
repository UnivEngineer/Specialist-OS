; FLASHFST.COM выбирает физический драйвер H: в странице DOS.
; COM исполняется в main, аппаратный код остаётся в защищённом банке.
    INCLUDE "../include/mxos.inc"
    ORG 0000h
    ld a,7
    ld c,2
    MXCALL MX_DOS_DRIVERSELECT
    ret
    END
