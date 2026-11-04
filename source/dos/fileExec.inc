;+---------------------------------------------------------------------------
; MXOS
; Запустить файл
;
; На входе
;  hl - ком строка в формате [Диск:]файл[ аргументы]
;
; На выходе
;  сf - ошибка
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileExec:   ; Сюда будем записывать результат
            ld      de, CMD_LINE
            
            ; Если первый символ меньше пробела, выходим c CF
            ld      a, (hl)
            cp      ' '
            jp      c, stc_ret

            ; Максимальный размер ком строки (+2 терминатора, итого 82h)
            ld      c, 80h        

fileExec_0: ; Если символ меньше пробела, выходим
            ld      a, (hl)
            cp      20h
            jp c,   fileExec_1

            ; Копируем симовол из hl в de
            ld      (de),a
            inc     hl
            inc     de

            ; Повторяем 80h раз
            dec     c
            jp nz,  fileExec_0

fileExec_1: ; В конец имени помещаем 13, 0
            ld      a, 0Dh
            ld      (de), a
            inc     de
            xor     a
            ld      (de), a

            ; Выводим на экран 0Ah
            call    printCharA

            ; Выводим на экран имя файла
            ld      hl, CMD_LINE
            call    j_printString

            ; Подготовка имени файла
            ld      hl, CMD_LINE
            ld      de, v_curFileDescr
            call    fileNamePrepare

            ; Запоминаем в стеке текущий адрес командной строки (начало списка аргументов)
            push    hl

            ; Голая команда — только A:ИМЯ.COM. Диск аргументов не меняем.
            ld a,(v_curFileDescr + FILE_DESCRIPTOR.ext)
            cp ' '
            jp nz,execExplicitName
            ld a,(CMD_LINE+1)
            cp ':'
            jp z,execExplicitName
            ld hl,04F43h             ; ASCII CO
            ld (v_curFileDescr + FILE_DESCRIPTOR.ext),hl
            ld a,'M'
            ld (v_curFileDescr + FILE_DESCRIPTOR.ext+2),a
            ld a,(v_drive)
            push af
            xor a
            call fileSelectDrive
            jp execImageOpen

execExplicitName:
            ; Если это BAT файл
            ld      de, aBat
            call    cmpFileExt
            jp z,   execBat

            ; EXE/RKS выбирает NC.EXT; DOS непосредственно исполняет только COM.
            call    cmpFileExt
            jp nz,  execError
execExeCom:
            ld a,(v_drive)
            push af
execImageOpen:
            ld hl,v_curFileDescr
            ld a,16
            call imageOpen
            jp c,execImageError
            push de
            ld h,b
            ld l,c
            add hl,de
            jp c,execRangeError
            ld a,d
            cp 0C0h
            call getAppMemoryEnd
            jp c,execCheckEnd
            cp SYSTEM_PROGRAM_ADDR >> 8
            jp c,execRangeError
            cp SYSTEM_FLASH_ADDR >> 8
            ld de,SYSTEM_PROGRAM_END
            jp c,execCheckEnd
            pop de
            push de
            ld a,d
            cp DISK_DRIVER_ADDR >> 8
            jp c,execRangeError
            ld de,DISK_DRIVER_ADDR + 0200h
execCheckEnd:
            call cmp_hl_de
            jp c,execRangeOK
            jp nz,execRangeError
execRangeOK:
            pop de
            ld a,d
            cp DISK_DRIVER_ADDR >> 8
            jp nc,execDriverImage
            call imageLoad
            jp c,execImageError
            call execRestoreDrive
            jp execTransfer
execDriverImage:
            ; Драйвер читается сначала в нижнее ОЗУ: его прежний код
            ; ещё может обслуживать текущий накопитель до конца чтения.
            push de
            ld de,0
            call imageLoad
            pop de
            jp c,execImageError
            push hl
            ex de,hl
            ld (v_input_start),hl
            pop hl
            call execRestoreDrive
            push hl
            ld hl,(v_input_start)
            ld b,h
            ld c,l
            ld hl,(v_tmpFileDescr+10)
            ex de,hl
            ld hl,0
            call memcpy_bc_hl
            pop hl
execTransfer:
            ex de,hl
            pop hl
            ex de,hl
            ld bc,defAppReturn
            push bc
            jp (hl)
; Исходный диск на стеке под адресом возврата. HL=вход программы сохраняется.
execRestoreDrive:
            pop bc
            pop af
            push bc
            jp fileSelectDrive
execRangeError:
            pop de
execImageError:
            pop af
            call fileSelectDrive
            jp execError

; ---------------------------------------------------------------------------

defAppReturn:
            or  a
            ret

; ---------------------------------------------------------------------------

execError:
            pop     de
badCommand:
            ld      hl, txtBadCommand; "\nBAD COMMAND or FILE NAME"
            call    j_printString
            ld      a, (v_bootAutoexec)
            or      a
            call z, j_getch          ; при автозагрузке не задерживать fallback
stc_ret:
            scf
            ret

; ---------------------------------------------------------------------------

printCharA:
            ld  c, 0Ah
            jp  j_printChar

