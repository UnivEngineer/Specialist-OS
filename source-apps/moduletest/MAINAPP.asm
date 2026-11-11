; Нативная main-программа: затирает нижний RUN, завершает работу через RET.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 0000h
entry:
    ld hl,message
    call bios_printString
    ld hl,055AAh
    ld (05200h),hl
    ret
message: DB 'Native main application',13,10,0
