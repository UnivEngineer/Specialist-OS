; Прокрутка вверх на 10 пикселей. AF/BC сохранены, HL/DE разрушены.
            push bc
            push af
            ld hl, 0900Ah
            ld de, 0C000h
            ld bc, 09000h
            ld a, 246
            jp scrollCopyFill
