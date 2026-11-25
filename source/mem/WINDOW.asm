    INCLUDE "../include/mxos.inc"
    INCLUDE "abi.inc"
    ORG BANK_WINDOW
; При старте окно обнулено.
    DS 32,0
    ASSERT $ == 0FFE0h
