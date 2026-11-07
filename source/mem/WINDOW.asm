    INCLUDE "abi.inc"
    ORG BANK_WINDOW
; A=физическая страница для RAM, для main значение несущественно.
; Между переключением и landing нет стековых команд.
windowMain:
    ld (BANK_IO_MAIN),a
    jp BANK_LANDING
windowRam:
    ld (BANK_IO_RAM),a
    jp BANK_LANDING
    ASSERT $ == BANK_PACKET
    DS BANK_PACKET_SIZE,0
    ASSERT $ == 0FFE0h
