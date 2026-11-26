; Прокрутка вниз на 10 пикселей. AF/BC сохранены, HL/DE разрушены.
            push bc
            push af
            ld hl, MX_SCREEN_BASE
            ld de, MX_SCREEN_END-10
            ld bc, MX_SCREEN_BASE+10
            ld a, 246
            jp scrollCopyFill
