;+---------------------------------------------------------------------------
; MXOS
; Загрузка шрифта в память
;
; Разработано vinxru
; Доработано SpaceEngineer
;----------------------------------------------------------------------------

initFont:   ; Изменяем команду перехода сюда на reboot2
            ld      hl, reboot2
            ld      (onceInitFont+1), hl
            push    hl

    IF EMBED_FONT == 0

            ; Шрифт грузится из файла
            ; Устанавливаем блочный драйвер ДОЗУ
            ld      b, 0
            call    setRAMDDriver

            ; Ищем и загружаем файл A:FONTKOI.FNT на адрес FONT_ADDR
            ld      hl, pathFontFnt
            ld      de, v_curFileDescr
            call    fileNamePrepare   ; подготовка имени файла и переключение накопителя
            ex      de, hl
            ld      de, FONT_ADDR
            jp      fileLoad2

    ELSE
            ; Шрифт встроен в хвост программы
            ; Копируем его на адрес FONT_ADDR
            ld      hl, font
            ld      de, fontEnd
            ld      bc, FONT_ADDR
            jp      memcpy_bc_hl

font:
    INCBIN "../../assets/system-rom/FONTKOI.FNT"
fontEnd:

    ENDIF
