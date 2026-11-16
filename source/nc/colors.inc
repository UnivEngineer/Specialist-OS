; Частые цвета: экономия двух байтов на вызов COLOR.
; Изменяет только A, сохраняет остальные регистры и флаги, как MVI/STA.
; Software mirror avoids reading the write-only color register for masked UI.
    IF ENABLE_COLOR
ncColorText:
            ld      a, COLOR_INFOTEXT
            jp      ncSetColor
ncColorNumber:
            ld      a, COLOR_INFONUMBER
            jp      ncSetColor
ncColorCommand:
            ld      a, COLOR_CMDLINE
            jp      ncSetColor
ncColorDialog:
            ld      a, COLOR_DIALOG
            jp      ncSetColor
ncColorFile:
            ld      a, COLOR_FILE
ncSetColor:
            ld      (IO_COLOR), a
            ld      (ncCurrentColor), a
            ret
    ENDIF
ncCurrentColor: DB COLOR_FILE
