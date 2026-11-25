                jp setInputHook         ; C8E7: обработчик ожидания клавиатуры
                ASSERT $ == 0C8EAh
                jp blockIO             ; C8EA: физический запрос DOS -> BIOS
                ASSERT $ == 0C8EDh
                jp FONT_ENGINE_RAWZ     ; C8ED: ASCIIZ без управляющих кодов
                ASSERT $ == 0C8F0h
                jp FONT_ENGINE_LIM  ; C8F0: HL=буфер, BC=байты
                jp bankText.fromBank    ; C8F3: исполнить запрос TEXT_PRINT из банка
                ASSERT $ == 0C8F6h
                jp printStringRawPseudo ; C8F6: HL=буфер, BC=байты
