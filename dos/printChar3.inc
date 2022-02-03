;+---------------------------------------------------------------------------
; MXOS
; Обработка служебных кодов при выводе символа
;
; На входе
;  a - символ
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printChar3: ; Код 7
            sub   7
            jp z, beep
            ; Код 8
            dec   a
            jp z, printChar_c08
            ; Код 10
            sub   2
            jp z, printChar_c0A
            ; Код 12
            sub   2
            jp z, printChar_c0C
            ; Код 13
            dec   a
            jp z, printChar_c0D
            ; Код 24
            sub   11
            jp z, printChar_c18
            ; Код 25
            dec   a
            jp z, printChar_c19
            ; Код 26
            dec   a
            jp z, printChar_c1A
            ; Продолжение, если код 31
            sub   5
            ret NZ      
            ; Код 31 очищает экран
            call  clearScreen

            ; Продолжение в printChar_c0C

; ---------------------------------------------------------------------------
; Обработка кода 0Ch. Курсор в левый верхний угол экрана

printChar_c0C:    ; bios_vars.cursorY = 8
            ld    l, 8

            ; Продолжение в printChar_c0D

; ---------------------------------------------------------------------------
; Обработка кода 0Dh. Курсор в начало строки

printChar_c0D:    ; bios_vars.cursorX = 0
            xor   a
saveVCursorHA:    ld    h, a
saveVCursor:      ld    (bios_vars.cursorY), hl
            ret

; ---------------------------------------------------------------------------
; Обработка кода 8h. Влево

printChar_c08:    ; cursorX -= 3
            ld    a, h
            sub   3

            ; Если нет переноса, сохранить положение курсора и выйти
            jp nc,  saveVCursorHA

            ; cursorX = 189
            ld    h, 189
            ld    (bios_vars.cursorY), hl

            ; Продолжение в printChar_c19

; ---------------------------------------------------------------------------
; Обработка кода 19h. Вверх

printChar_c19:    ; Если cursorY < 18, то выйти. Иначе cursorY -= 10
            ld    a, l
            sub   0Ah
            ret c 
            cp    8
            ret c 

saveVCursorLA:    ; Сохранить положение курсора и выйти
            ld    l, a
            jp    saveVCursor

; ---------------------------------------------------------------------------
; Обработка кода 18h. Вправо

printChar_c18:    ; Если cursorX+3 >= 192, то перейти на printChar_c0A
            ; Иначе cursorX += 3
            ld    a, h
            add   a,3
            cp    0C0h
            jp c, saveVCursorHA
            jp    printChar_c0A

