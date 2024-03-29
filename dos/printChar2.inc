;+---------------------------------------------------------------------------
; MXOS
; ����� ������� �� ����� (��� �������)
;
; �� �����
;  � - ������
;
; �� ������
;  �������� �� �����������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

printChar2:
            ; �������� � hl ���������� �������
            ld  hl, (bios_vars.cursorY)

            ; ����������, ���� �� ������� ����� ��������� ESC ������������������ (bios_vars.escMode==0FFh)
            ld  a, (bios_vars.escMode)
            inc a
            jp  nz,  printChar_esc

            ; ��� bios_vars.escMode==0FFh

            ; ���� ��� �������� ������� (>=20h), ��������� � ������
            ld  a, c
            cp  ' '
            jp  nc, printChar_real

            ; ��� ���������
            cp  9
            jp  z, printChar_c9      ; ��� bios_vars.cursorX+=24 � ret

            ; ����� ������ ����� ESC
            cp  1Bh
            jp  nz, printChar3

            ; ����� ESC. �������� ����� ��������� ESC ������������������ (bios_vars.escMode=0)
            xor a
            ld  (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------
; �������������� ������� �� KOI-7 � KOI-8 � ����� �� ����� (��������� ���� 
; ����� ����� ���������)

printChar_real:
            ; ������ ��� ������
            ld  c, a

            ; ��������� � ������, ���� ������� ����� KOI-8 (bios_vars.koi7 == 0)
            ld  a, (bios_vars.koi7)
            inc a
            jp  nz, printChar_real2

            ; ������� ����� KOI-7 (bios_vars.koi7 == 0FFh), ����
            ; �������������� ������� ��� ��������������.

            ; ���� ������ ������ ��� ����� 0C0h ������ �������
            ld  a, c
            cp  0C0h
            ret nc      

            ; ���� ������ ��������� � �������� 60h ... 7Fh, ����������� ��� ��� �� 80h
            cp  60h
            jp  c, printChar_real2
            cp  80h
            jp  nc,  printChar_real2
            add a, 80h
            ld  c, a

; ---------------------------------------------------------------------------
; ����� ������� �� ����� � KOI-8 (��������� ���� ��� �� ����� ���������)

printChar_real2:  
            ; ��� �����������
            ld    a, (bios_vars.inverse)
            and   a

printChar_poly:
            call z, normalVideo

            ; ���� bios_vars.charGen �� ����� 0FFFFh, �� ������������ ������������� �� ������ bios_vars.charGen*8
            ld    hl, (bios_vars.charGen)
            ld    a, h
            inc   a
            jp nz,  printChar_alt
            ld    a, l
            inc   a
            jp nz,  printChar_alt

            ; ����� ���������� ������� �� ���. 
            ; ����� ������� � ��� (de = 0x800 + c*8)
            ld    hl, 100h          
            ld    b,  0
            add   hl, bc
            add   hl, hl
            add   hl, hl
            add   hl, hl
            ex    de, hl

            ; ������������� ����� ��� �������
            ld    hl, v_char
            push  hl

            ; �������� 8 ���� �� ���
            ld    (IO_PAGE_ROM),a
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            inc   hl
            inc   de
            ld    a, (de)
            ld    (hl), a
            ld    (IO_PAGE_RAM),a

            ; ����� �������
            pop   de
            jp    printChar_de

; ---------------------------------------------------------------------------
; ��������� ������ ESC+F

printChar_esc80:; ���� ������ � ����� 0, ��������� ����� ��������� ESC
            ld    a, c
            or    a
            jp z, printChar_esc80_exit

            ; ��������� ����� < 20h
            cp    20h
            jp c, printChar3

            ; ���� ��� >= 80h, �����
            cp    80h
            ret nc      

            ; ���� c >= 40h, ������� �� ����� c+40h
            cp    40h
            jp nc,  printChar_esc80_2

            ; ����� ������� c - 20h
            sub   20h ; ' '
            ld    c, a
            jp    printChar_real2

; ---------------------------------------------------------------------------

printChar_esc80_2:
            ; ���� c >= 40h, ������� �� ����� c+40h
            add   a,40h
            ld    c, a
            jp    printChar_real2

; ---------------------------------------------------------------------------

printChar_esc80_exit:
            dec   a
            ld    (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------
; ������ ������� � ESC-������

printChar_esc:    ; ������� ���� bios_vars.escMode=80h
            ld    a, (bios_vars.escMode)
            cp    80h
            jp z, printChar_esc80

            ; ��������� ����� ��������� ESC ������������������
            push  af
            ld    a, 0FFh
            ld    (bios_vars.escMode),a
            pop   af

            ; ������� ���� bios_vars.escMode==0
            or    a
            jp z, printChar_esc0

            ; ������� ���� ��������� ESC+Y (bios_vars.escMode==1)
            dec   a
            jp z, printChar_esc1

            ; ������� ���� bios_vars.escMode==2
            dec   a
            jp z, printChar_esc2

            ; ������� ���� bios_vars.escMode==3
            dec   a
            jp z, printChar_esc3

            ; �����
            ret

; ---------------------------------------------------------------------------
; ��������� ESC+Y

printChar_esc1:   ; ���� ��� ������ 20h ��� ������ 20h + 24, �� ���������� ��������� Y
            ld    a, c
            cp    20h + 25
            jp nc,  loc_C693
            sub   20h
            jp c, loc_C693

            ; cursorY = 8 + a*10
            add   a, a
            ld    c, a
            add   a, c
            add   a, c
            add   a, c
            add   a, c
            add   a,8
            ld    (bios_vars.cursorY),a

loc_C693:   ; ��������� ������ ����� ��������� � printChar_esc2
            ld    a, 2
            ld    (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------
; ��������� ESC+Y

printChar_esc2:   ; ���� ��� ������ 20h ��� ������ 20h + 63, �� ���������� ��������� X
            ld    a, c
            cp    60h
            ret nc
            sub   20h
            ret c 

            ; cursorX = c*3;
            ld    c, a
            add   a, a
            add   a, c
            ld    (bios_vars.cursorX),a
            ret

; ---------------------------------------------------------------------------
; ��������� ������� ������� ESC ������������������

printChar_esc0:   ; ���� ��� ������ 20h
            ld    a, c
            cp    '(' ; 28h
            jp z, printChar_e28
            cp    ')' ; 29h
            jp z, printChar_e29
            cp    'P' ; 50h
            jp z, printChar_eP
            sub   'A' ; 41h
            jp z, printChar19
            dec   a ; 'b' 42h
            jp z, printChar1A
            dec   a ; 'c' 43h
            jp z, printChar18
            dec   a ; 'd' 44h
            jp z, printChar08
            dec   a ; 'e' 45h
            jp z, printChar1F
            dec   a ; 'F' 46h
            jp z, printChar_eF
            dec   a
            dec   a ; 'h' 48h
            jp z, printChar0C
            dec   a ; 'I' 49h
            jp z, printChar_eI
            dec   a
            dec   a
            sub   14 ; 'Y' 59h
            jp z, setEscMode_eY

            ; ����� �������
            jp    printChar2

; ---------------------------------------------------------------------------
; ��������� ESC+Y.

setEscMode_eY:    ; ��������� ������ ����� ��������� � printChar_esc1 
            ld    a, 1
            ld    (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------

printChar18:
            ld    c, 18h
            jp    printChar2

; ---------------------------------------------------------------------------

printChar08:
            ld    c, 8
            jp    printChar2

; ---------------------------------------------------------------------------

printChar1F:      ld    c, 1Fh
            jp    printChar2

; ---------------------------------------------------------------------------

printChar0C:
            ld    c, 0Ch
            jp    printChar2

; ---------------------------------------------------------------------------
; ��������� ESP+P. ��������� ������ ��������� �� ��������

printChar_eP:
            ld    a, 3
            ld    (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------

printChar19:
            ld    c, 19h
            jp    printChar2

; ---------------------------------------------------------------------------

printChar1A:
            ld    c, 1Ah
            jp    printChar2

; ---------------------------------------------------------------------------
; ����� ��������� �� ��������. ���������� ESC+P

printChar_esc3:
            ; �������, �� ������, ���� ��� �������
            ld    a, c
            or    a
            ret z 

            ; ������ �� ��������
            call  printer

            ; ������ �� ������
            call  printChar2

            ; ��������� � ���� ������ �� ���� 0 ��� ESC
            ld    a, 3
            ld    (bios_vars.escMode),a
            ret

; ---------------------------------------------------------------------------
; ����������� ��������� ESC+I. ����������� ������� ����� � ���������� ������

printChar_eI_2:
            ; ���� cursorY > 18, ���������� ������ ����� � �������
            cp    19
            jp nc,  printChar2

            ; �������������� bios_vars.cursorY=8
            ld    a, 8
            ld    (bios_vars.cursorY),a
            ret

; ---------------------------------------------------------------------------
; ��������� ESC+I. ����������� ������� ����� � ���������� ������

printChar_eI:
            ; ������������ ������ �������� printChar2
            ld    c, 19h

            ; ������������ ����� ����, ������ ���� bios_vars.cursorY == 8.
            ; ����� ��������� �� printChar_eI_2
            ld    a, (bios_vars.cursorY)
            cp    8
            jp nz,  printChar_eI_2

