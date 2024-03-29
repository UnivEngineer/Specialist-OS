;----------------------------------------------------------------------------
; MXOS NC.COM
; ������ ����� �� �����
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

tapeWriteWord:
            ld    a, l
            call  bios_tapeWrite
            ld    a, h
            jp    bios_tapeWrite

;----------------------------------------------------------------------------
; ������ ����� ������ �� ������ hl ������ bc+1 �� �����
;----------------------------------------------------------------------------

tapeWriteBlock:
            inc    bc
loc_D410:   call   checkBreak
            ld     a, (hl)
            call   bios_tapeWrite
            inc    hl
            dec    bc
            ld     a, b
            or     c
            jp nz, loc_D410
            ret

;----------------------------------------------------------------------------
; ������ ����� ���� �� �����
; 200h �����, ����� E6h
;----------------------------------------------------------------------------

tapeWritePilot:
            ld     bc, 200h
loc_D422:   call   checkBreak
            xor    a
            call   bios_tapeWrite
            dec    bc
            ld     a, b
            or     c
            jp nz, loc_D422

            ld    a, 0E6h
            jp    bios_tapeWrite

; ---------------------------------------------------------------------------

checkBreak: ld     a, (IO_KEYB_B)
            and    a
            jp p,  tapeErrorHandler
            ret

