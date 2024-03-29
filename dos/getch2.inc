;----------------------------------------------------------------------------
; MXOS
; �������� ����� � ����������
;
; �� ������
;  ? - ���
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

getch2:     ; ��������� ��������
            push    hl
            push    bc
            push    de

            ; �������� ��� ������� ������� � ��������� ��� � bios_vars.lastKey
            call    keyScan2
            ld      (bios_vars.lastKey),a

getch_retry:
            ; ��� ����� �������, ��������� ������ ��� ���
            ld      c, 1

loc_C227:   ; ������ ������
            call    drawCursor2

            ; ��������
            ld      a, (bios_vars.cursorDelay)
loc_C22D:   ld      b, 40h
            call    delay_b
            dec     a
            jp nz,  loc_C22D

loc_C236:   ; ������/������� ������
            call    drawCursor
            
            ; �������� �������� �������
            ld      de, 500h

loc_C23C:   ; �������� ��� ������� �������
            call    keyScan2
            ;cp      80h         ; �.�.
            cp      0FFh
            jp nz,  getch2_pressed

            ; ���������� bios_vars.lastKey=0FFh
            ld      (bios_vars.lastKey),a

            ; ��������� 500h ���
            dec     de
            ld      a, d
            or      e
            jp nz,  loc_C23C

            ; ������ �����
            jp      loc_C236

; ---------------------------------------------------------------------------

;    DB 0FFh, 0FFh

; ---------------------------------------------------------------------------

setKeybMode82:
            ld      a, 82h
            ld      (IO_KEYB_MODE),a
            ret

; ---------------------------------------------------------------------------

setKeybMode91:
            ld      a, 91h
            ld      (IO_KEYB_MODE),a
            ret

; ---------------------------------------------------------------------------
; ���� ������ �������

getch2_pressed:
            ; ��������� ���
            ld      b, a

            ; ���� ��������� ������, ������� ���
            ld      a, c
            rrca
            jp c,   loc_C269
            call    drawCursor2
loc_C269:   
            ld      hl, (bios_vars.lastLastKey)   ; h = bios_vars.lastKey, l = bios_vars.lastLastKey
            ld      a, b        ; a = b = ��� ������� �������
            cp      h
            jp nz,    loc_C28D    ; ��� ������� ������� ���������
            cp      l
            jp z,   loc_C295

            ; ������ ������
            call    drawCursor

            ; ����, ���� �������� �������
            ld      a, (bios_vars.antiBsDelay)
            ld      d, a
loc_C27C:   call    keyScan2
            cp      h
            jp nz,  loc_C227    ; ������ ������ ������ �������
            dec     d
            jp nz,  loc_C27C

            ; ����� �������

            ; ������� ������
            call    drawCursor

            jp      loc_C290
; ---------------------------------------------------------------------------

loc_C28D:   ; �������� ������
            call    beep

loc_C290:   ld      l, h
            ld      h, b  ; ��� ������� � ���������� ������� ��������� � bios_vars.lastKey
            ld      (bios_vars.lastLastKey), hl

loc_C295:   ; �� ��������� ������� CAPS LOCK, SHIFT � ���/LAT �� ������
            ld      a, b
            cp      21h
            jp c,   getch_noShift   
            ; �� ������� c ������ 21h-3Fh ������ SHIFT
            cp      40h
            jp c,   getch_shift 
            ; �� ���������� ������� (40h-7Eh) ������ CAPS LOCK, SHIFT � ���/LAT
            cp      7Fh
            jp c,   getch_chars
            ; �� ��������� ������� (7Fh+) CAPS LOCK, SHIFT � ���/LAT �� ������
            jp z,   getch_noShift

            ; ...
            ;nop
            ;nop

            ; ���������, ���� ��� ������� �� 81h
            cp      81h
            jp nz,  getch_noShift

            ; ��� ������� 81h

            ; ���� ���� �� �����
            ld      a, (IO_KEYB_B)
            and     2
            jp nz,  getch3

            ; ��� ������� 81h � ������

            ; �������� ������
            call    beep

            ; CAPS LOCK ��� KOI-7 ��� ���/LAT ��� KOI-8
            jp      getch_rc

; ---------------------------------------------------------------------------
; ������ ���������� Shift + ���/LAT

getch_RusLat:
            ; �� ����� a = bios_vars.keyLocks
            xor     81h ; ���������� ������ � ������� ����
            ld      (bios_vars.keyLocks), a

            ; ������ �������� ������
            ld      a, (bios_vars.beepFreq)
            push    af
            ld      a, 4Fh
            ld      (bios_vars.beepFreq), a
            call    beep
            ld      a, 5Fh
            ld      (bios_vars.beepFreq), a
            call    beep
            pop     af
            ld      (bios_vars.beepFreq), a

            ; ...
            ;nop

            jp      getch_retry

; ---------------------------------------------------------------------------
; �������� �������� ��� �������� �����

getch_chars:
            ; ���� �� ����� CAPS LOCK, ���������� ��� ����
            ld      a, (bios_vars.keyLocks)
            ld      c, a
            and     a
            jp M,   loc_C2EA    ; CAPS LOCK

            ; ���������� ��������� � ��������
            ld      a, b
            xor     20h
            ld      b, a

loc_C2EA:   ; ���� ������������ �� ���/���, �������� � c=1
            ld      a, c
            and     1
            ld      c, a

            ; ���� ���� �����, �������� � a=0
            ld      a, (IO_KEYB_B)
            and     2
            rrca    ; ! ��� �� ���� ����� c ?

            ; ���������� ���������� ������� � ������� ���� a^c==0
            xor     c
            ld      a, b
            jp nz,  loc_C5A8
            add     80h
            jp      loc_C5A8

; ---------------------------------------------------------------------------
; ������ ����� �� �������

getch_shift:
            ; ���� �� ����� ����, ���������� ��� ����
            ld      a, (IO_KEYB_B)
            and     2
            ld      a, b
            jp nz,  loc_C5A8

            ; ������ ����� �� �������
            xor     10h

            ; ���� ���������� 30h ������ �������� �� 5Fh
            cp      20h
            jp nz,  getch_noShift
            ld      a, 5Fh

            ; ����������� getch_noShift

; ---------------------------------------------------------------------------
; ������� ��� ���������

getch_noShift:
            ; ��������� ��� ������� ������� � b     
            ld      b, a

loc_C311:   call    setKeybMode83
            ld      a, 0F7h
            ld      (IO_KEYB_A),a
            ld      a, (IO_KEYB_B)
            cpl
            rrca
            rrca
            rrca
            call    setKeybMode82

            ;nop

            ; �������������� ��� ������� �������
            ld      a, b

popa_ret_2: ; �������������� �������� � �������
            pop     de
            pop     bc
            pop     hl
            ret

