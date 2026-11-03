; Строковые дескрипторы и трёхбайтные расширения файлов.
t_romDriverInfo: DB "ROM",0,"MX2/banked",0
t_ramDriverInfo: DB "RAMD",0,"Per-byte",0
aFat:            DB "FAT"
; cmpFileExt последовательно продвигает DE на три байта.
; Расширения должны лежать подряд, без нулевых терминаторов.
aBat:            DB "BAT"
aCom:            DB "COM"
    ASSERT aCom == aBat+3
pathRomCom:      DB "A:ROM.COM",0
