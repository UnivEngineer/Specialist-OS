printChar_position:
            ; Как в standalone PAGED: вычисление адреса в горячем пути.
            ; При X >= 190 оставляем штатный перенос перед рисованием.
            ld hl,(bios_vars.cursorY)
            ld a,h
            cp 190
            jp nc,printChar_positionWrap
            rrca
            rrca
            and 3Fh
            add a,90h
printChar_positionReady:
            inc h
            inc h
            inc h
            ld (bios_vars.cursorY),hl
            ld b,a
            ld a,l
            sub 7
            ld c,a
            ret
printChar_positionWrap:
            ld c,10
            call calcCursorAd_0
            jp printChar_positionReady
