; Monitor-2: точное восстановление assets/programs/Monitor2.mon.
; Самостоятельное STD-ядро C000h..CFFFh, не компонент ABI MXOS.
; Сборка SjASMPlus --i8080; оригинал не подключается через INCBIN.
    ORG 0C000h
    INCLUDE "loader.inc"
    ASSERT $ == 0C500h
    INCLUDE "font.inc"
    ASSERT $ == 0C800h
    INCLUDE "monitor.inc"
    ASSERT $ == 0D000h
    END
