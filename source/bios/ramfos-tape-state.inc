; Адреса RAMFOS, которые приложение может проверить напрямую.
ramfosTapeMode:    DB '0'
ramfosTapePresets: DW 322Ch,1812h,211Bh,0F09h,1812h,0B05h
ramfosTapePresetsEnd:
    ASSERT ramfosTapeMode == 0C65Fh
    ASSERT ramfosTapePresetsEnd == 0C66Ch
