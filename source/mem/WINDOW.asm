    INCLUDE "../include/mxos.inc"
    INCLUDE "abi.inc"
    ORG BANK_WINDOW
; HL=порт маппера, A=физический банк. Ни одной стековой команды.
; Один переключатель вместо двух оставляет байт идентификатора профиля.
windowSelect:
    ld (hl),a
    jp BANK_LANDING
    DS BANK_PACKET-$,0
    DS BANK_PACKET_SIZE,0
    DS BANK_PROFILE-$,0
    DB BANK_PROFILE_COMMAND
    ASSERT $ == 0FFE0h
