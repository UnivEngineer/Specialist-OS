; Предустановки цветов:

; 0 = vinxru

; 1 = новый

; 2 = Norton Commander

COLOR_PRESET = 1

COLOR_TRUNCATED = 0C1h



COLOR_CMDLINE    =  070h        ; Цвет командной строки

COLOR_CMDSCREEN  =  COLOR_BIOS  ; Цвет командного экрана (когда панели спрятаны)



    IF COLOR_PRESET == 0



COLOR_BORDER     =  0F1h    ; Цвет рамки

COLOR_PANELNAME  =  0A1h    ; Цвет заголовка панели (NAME)

COLOR_FILE       =  0B1h    ; Цвет файлов

COLOR_INFOLINE   =  0B1h    ; Цвет строки информации о текущем файле

COLOR_CURSOR     =  00Bh    ; Цвет выделенного файла

COLOR_DIALOG     =  007h    ; Цвет окон диалогов F1-F9

COLOR_DIALOG_ERR =  047h    ; Цвет текста на окне с сообщением об ошибке

COLOR_INFOHEADER =  0A1h    ; Заголовок информационной панели

COLOR_INFONUMBER =  0E1h    ; Цифры на информационной панели

COLOR_INFOTEXT   =  0F1h    ; Текст на информационной панели

COLOR_HELP_F     =  040h    ; Цвет функциональных клавиш в строке подсказки

COLOR_HELP_TEXT  =  071h    ; Цвет текста в строке подсказки



    ELSEIF COLOR_PRESET == 1



COLOR_BORDER     =  0F1h    ; Цвет рамки

COLOR_PANELNAME  =  0E1h    ; Цвет заголовка панели (NAME)

COLOR_FILE       =  0B1h    ; Цвет файлов

COLOR_INFOLINE   =  0B1h    ; Цвет строки информации о текущем файле

COLOR_CURSOR     =  003h    ; Цвет выделенного файла

COLOR_DIALOG     =  007h    ; Цвет окон диалогов F1-F9

COLOR_DIALOG_ERR =  047h    ; Цвет текста на окне с сообщением об ошибке

COLOR_INFOHEADER =  0E1h    ; Заголовок информационной панели

COLOR_INFONUMBER =  0E1h    ; Цифры на информационной панели

COLOR_INFOTEXT   =  0F1h    ; Текст на информационной панели

COLOR_HELP_F     =  060h    ; Цвет функциональных клавиш в строке подсказки

COLOR_HELP_TEXT  =  030h    ; Цвет текста в строке подсказки



    ELSE



COLOR_BORDER     =  0B1h    ; Цвет рамки

COLOR_PANELNAME  =  0E1h    ; Цвет заголовка панели (NAME)

COLOR_FILE       =  0B1h    ; Цвет файлов

COLOR_INFOLINE   =  0B1h    ; Цвет строки информации о текущем файле

COLOR_CURSOR     =  003h    ; Цвет выделенного файла

COLOR_DIALOG     =  007h    ; Цвет окон диалогов F1-F9

COLOR_DIALOG_ERR =  047h    ; Цвет текста на окне с сообщением об ошибке

COLOR_INFOHEADER =  0B1h    ; Заголовок информационной панели

COLOR_INFONUMBER =  0E1h    ; Цифры на информационной панели

COLOR_INFOTEXT   =  0B1h    ; Текст на информационной панели

COLOR_HELP_F     =  070h    ; Цвет функциональных клавиш в строке подсказки

COLOR_HELP_TEXT  =  030h    ; Цвет текста в строке подсказки



    ENDIF



;---------------------------------------------------------------------------

; Макросы

;---------------------------------------------------------------------------



; Когда цвет включен, макрос вставляет такой код

    MACRO COLOR x

        IF ENABLE_COLOR

            IF x == COLOR_INFOTEXT

                call ncColorText

            ELSEIF x == COLOR_INFONUMBER

                call ncColorNumber

            ELSEIF x == COLOR_CMDLINE

                call ncColorCommand

            ELSEIF x == COLOR_DIALOG

                call ncColorDialog

            ELSEIF x == COLOR_FILE

                call ncColorFile

            ELSE

                ld a, x

                ld (IO_COLOR), a

                ld (ncCurrentColor), a

            ENDIF

        ENDIF

    ENDM



; Макрос для нахождения координат строки длиной W, отцентрированной горизонтально

    MACRO CENTER_LINE w, y

        ld a,w

        ld l,y

        call ncCenterLine

    ENDM



;---------------------------------------------------------------------------

; Код

;---------------------------------------------------------------------------



