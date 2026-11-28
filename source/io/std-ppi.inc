; Совместимая клавиатура Монитора-2 доступна из защищённой области C000h–D7FFh.
; В STD основная страница IO.SYS скрыта аппаратным окном.
    MODULE ioStd
    DEFINE PPI_STD 1
    INCLUDE "main-ppi.inc"
    INCLUDE "key-scan.inc"
    UNDEFINE PPI_STD
    ENDMODULE
