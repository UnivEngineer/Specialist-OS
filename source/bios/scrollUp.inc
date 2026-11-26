; Прокрутка вверх на 10 пикселей. AF/BC сохранены, HL/DE разрушены.
            push bc
            push af
            ld hl, MX_SCREEN_BASE+10
            ld de, MX_SCREEN_END
            ld bc, MX_SCREEN_BASE
            xor a
            jp scrollCopyFill
