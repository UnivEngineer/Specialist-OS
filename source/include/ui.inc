; Формат списков объектов bios_uiDraw. Координаты в пикселях.
; A: бит 7 — абсолютный X; иначе добавляется смещение из ABI-входа A.
; Список заканчивается одним нулевым байтом.
    MACRO G_WINDOW A,X,Y,W,H
        DB A|2, Y, 90h+(X>>3), H-6, (W>>3)-2
    ENDM
    MACRO G_LINE A,X,Y,W
        DB A|1, Y, 90h+(X>>3), (((X&7)+W+7)>>3)-2, 0FFh>>(X&7), (0FF00h>>((W+X)&7)) & 0FFh
    ENDM
    MACRO G_VLINE A,X,Y,H
        DB A|3, Y, 90h+(X>>3), H, 80h>>(X&7)
    ENDM
