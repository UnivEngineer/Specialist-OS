;----------------------------------------------------------------------------
; MXOS
; �������� ����� � ���������� (�����������)
;
; �� ������
;  ? - ���
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

getch3:     ; ���� ����� ���� ���/LAT - �� ������������ ��� ��� CAPS LOCK,
            ; ���� ����� Shit+���/LAT - �� ����������� ����� ���
            call    setKeybMode91
            ld      a, 0F8h
            ld      (IO_KEYB_B), a
            ld      a, (IO_KEYB_A)           
            push    af
            call    setKeybMode82
            pop     af
            and     8
            jp nz,  getch_capsLock

            ; ������ KOI-7 / KOI-8
            ld      a, (bios_vars.koi7)
            cpl
            ld      (bios_vars.koi7), a

            ;ld      a, 3Ah  ; �������� ��������� ����������
            ld      a, 0BAh  ; �������� �������� ����������
            ld      (bios_vars.keyLocks), a

            ; �������� ������
            call    beep

            ; ������ �������
            ld      a, 5Fh
            ld      (bios_vars.beepFreq),a

            jp      getch_retry

; ---------------------------------------------------------------------------
; CAPS LOCK ��� KOI-7

getch_capsLock:
            ; ����������� capsLock
            ld      a, (bios_vars.keyLocks)
            xor     80h
            ld      (bios_vars.keyLocks), a

            ; ������ �������
            ld      a, (bios_vars.beepFreq)
            xor     10h
            ld      (bios_vars.beepFreq),a

            ; ����� ��� ������� �������� ������, �� ����� � ����� �������
            call    beep

            ; ���� ��������� �������
            jp      getch_retry

; ---------------------------------------------------------------------------
; CAPS LOCK ��� KOI-7 ��� ���/LAT ��� KOI-8

getch_rc:   ; � ������ KOI-8 �� ����������� ���������
            ld    a, (bios_vars.koi7)
            inc   a

            ld    a, (bios_vars.keyLocks) ; ��������� � getch_RusLat
            jp nz,  getch_RusLat

            ; � ������ KOI-7 �� ���������� �������
            jp    getch_capsLock    

; ---------------------------------------------------------------------------

loc_C5A8:   ; ������� KOI-7
            ld    b, a

            ld    a, (IO_KEYB_B)
            and   2
            jp nz,  loc_C311

            ld    a, 0A0h
            xor   b
            ld    b, a

            jp    loc_C311

