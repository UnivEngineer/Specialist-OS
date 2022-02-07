DEFC TMP  =  07FFFh 
DEFC IO_RAM  =  0FFFCh 

.org 0E000h

start:      ; Включаем ОЗУ
            ld    (IO_RAM),a

            ; Стек
            ld    sp, TMP

            ; Распаковываем ОС
            ld    de, packedData
            ld    bc, 0C000h
            push  bc
            jp    unmlz

; Разархиватор MegaLZ
; (с) b2m, vinxru, группа mayhem...


unmlz:      ld    a, 80h

loc_2:            ld    (TMP),a           
            ld    a, (de)
            inc   de
            jp    loc_13
; ---------------------------------------------------------------------------

loc_A:            ld    a, m
            inc   hl
            ld    (bc),a
            inc   bc

loc_E:
            ld    a, m
            inc   hl
            ld    (bc),a
            inc   bc

loc_12:
            ld    a, m

loc_13:
            ld    (bc),a
            inc   bc

loc_15:
            ld    a, (TMP)
            add   a, a
            jp nz,  loc_1F
            ld    a, (de)
            inc   de
            RLA

loc_1F:
            jp c, loc_2
            add   a, a
            jp nz,  loc_29
            ld    a, (de)
            inc   de
            RLA

loc_29:
            jp c, loc_4F
            add   a, a
            jp nz,  loc_33
            ld    a, (de)
            inc   de
            RLA

loc_33:
            jp c, loc_43
            ld    hl, 3FFFh
            call  sub_A2
            ld    (TMP),a
            add   hl, bc
            jp    loc_12
; ---------------------------------------------------------------------------

loc_43:
            ld    (TMP),a
            ld    a, (de)
            inc   de
            ld    l, a
            ld    h, 0FFh
            add   hl, bc
            jp    loc_E
; ---------------------------------------------------------------------------

loc_4F:
            add   a, a
            jp nz,  loc_56
            ld    a, (de)
            inc   de
            RLA

loc_56:
            jp c, loc_60
            call  sub_B7
            add   hl, bc
            jp    loc_A
; ---------------------------------------------------------------------------

loc_60:
            ld    h, 0

loc_62:
            inc   h
            add   a, a
            jp nz,  loc_6A
            ld    a, (de)
            inc   de
            RLA

loc_6A:
            jp nc,  loc_62
            push  af
            ld    a, h
            cp    8
            jp nc,  loc_98
            ld    a, 0

loc_76:
            rra
            dec   h
            jp nz,  loc_76
            ld    h, a
            ld    l, 1
            pop   af
            call  sub_A2
            inc   hl
            inc   hl
            push  hl
            call  sub_B7
            ex    de, hl
            ex    (sp), hl
            ex    de, hl
            add   hl, bc

loc_8C:     ld    a, m
            inc   hl
            ld    (bc),a
            inc   bc
            dec   e
            jp nz,  loc_8C
            pop   de
            jp    loc_15

; ---------------------------------------------------------------------------

loc_98:
            pop   af
            ; Конец
            ret

; ---------------------------------------------------------------------------

sub_A2:     add   a, a
            jp nz,  loc_A9
            ld    a, (de)
            inc   de
            RLA

loc_A9:     jp c, loc_B1
            add   hl, hl
            ret c 
            jp    sub_A2

; ---------------------------------------------------------------------------

loc_B1:     add   hl, hl
            inc   l
            ret c 
            jp    sub_A2

; ---------------------------------------------------------------------------

sub_B7:     add   a, a
            jp nz,  loc_BE
            ld    a, (de)
            inc   de
            RLA

loc_BE:     jp c, loc_CA
            ld    (TMP),a
            ld    a, (de)
            inc   de
            ld    l, a
            ld    h, 0FFh
            ret
; ---------------------------------------------------------------------------

loc_CA:     ld    hl, 1FFFh
            call  sub_A2
            ld    (TMP),a
            ld    h, l
            dec   h
            ld    a, (de)
            inc   de
            ld    l, a
            ret

packedData:

.end
