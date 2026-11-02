; Строковые дескрипторы и трёхбайтные расширения файлов.
t_romDriverInfo: DB "ROM",0,"MX2/banked",0
    IF RAMD_BUFFERED
t_ramDriverInfo: DB "RAMD",0,"Per-byte",0
    ELSE
t_ramDriverInfo: DB "RAMD",0,"Legacy",0
    ENDIF
aFat:            DB "FAT"
; cmpFileExt последовательно продвигает DE на три байта.
; Расширения должны лежать подряд, без нулевых терминаторов.
aBat:            DB "BAT"
aCom:            DB "COM"
aExe:            DB "EXE"
    ASSERT aCom == aBat+3 && aExe == aCom+3
pathRomCom:      DB "A:ROM.COM",0
