                jp  installDriver       ; C860: Установить драйвер накопителя
j_diskDriver:   jp  sysDiskDriver          ; C863: Драйвер выбранного диска
                DS 3,0 ; Removed MXOS file-data entry; use FILEHANDLE.
                jp  FONT_ENGINE_RAW       ; C869: [MXOS2] Вывод символа на экран (только реальные символы)
                jp  printDecWord        ; C86C: [MXOS2] Вывод числа экран в десятичной форме
                jp  setRAMDDriver       ; C86F: [MXOS2] Установить драйвер доступа к ДОЗУ
                DS 3,0 ; MXOS DOS API uses MXCALL, no historical file JMP.
                jp  upperCase           ; C875: [MXOS2] Перевод кода символа КОИ-8 в верхний регистр
                jp  strcmp              ; C878: [MXOS2] Сравнение строк, чувствительное к регистру
                jp  stricmp             ; C87B: [MXOS2] Сравнение строк, нечувствительное к регистру
                jp  sub_hl_de           ; C87E: [MXOS2] Вычитание hl и de: hl = hl - de
                jp printCharBody
                jp printChar_c0A
                jp calcCursorAd_0
