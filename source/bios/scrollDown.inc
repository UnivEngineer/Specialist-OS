; Прокрутка вниз на 10 пикселей. AF/BC сохранены, HL/DE разрушены.
            push bc
            push af
            ld hl, 09000h
            ld de, 0BFF6h
            ld bc, 0900Ah
            ld a, 246
            jp scrollCopyFill
