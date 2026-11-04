            include "../../source/include/mxos.inc"
            include "bufferabi.inc"
; В общем окне только сохранение AF, переключения, переход и возврат.
; Основной контекст и диспетчер находятся в непереключённом основном ОЗУ.
            org BUFFER_GATE
            push af
            ld (IO_PAGE_RAM), a
            jp BUFFER_DISPATCH
            assert $ == BUFFER_RETURN
            ld (IO_PAGE_RAMD), a
            pop af
            ret
            assert $ == BUFFER_ADDR
            defs BUFFER_SIZE, 0
            assert $ == 0FFE0h
