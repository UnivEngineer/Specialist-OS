; Общие файловые операции. Диалоги и выбор памяти принадлежат вызывающему.
; Вызовы не реентерабельны, как остальные файловые функции DOS.

;----------------------------------------------------------------------------
; MXOS
;
; Выбрать диск и проверить, что на нём доступен распознанный том.
;
; вход:
;   a = номер диска 0..7.
;
; выход:
;   cf = 0 при доступном томе. При cf = 1 причину можно прочитать через dosGetError.
;   bc, de и hl сохранены.
;----------------------------------------------------------------------------

fileChangeDrive:
            cp      8
            jp nc,  fileOpError
            call    fileSelectDrive
            ret c
            call    fatCheckMounted
            ret c
            jp      dosClearError

;----------------------------------------------------------------------------
; MXOS
;
; Записать индексы установленных накопителей в буфер вызывающего.
;
; вход:
;   hl = буфер минимум на восемь байтов, bc = его ёмкость.
;
; выход:
;   cf = 0, b = число записанных индексов, исходный диск сохранён. cf = 1 при
;   недопустимом диапазоне или ошибке диска. af, bc, de и hl рабочие.
;----------------------------------------------------------------------------

fileGetDrives:
            ld      a, b
            or      a
            jp nz,  fgd_SizeReady
            ld      a, c
            cp      8
            jp c,   fileOpError
fgd_SizeReady:
            ld      bc, 8
            call    fileOpRange
            ret c
    IFDEF BANKED_DOS_BUILD
            ; Готовая геометрия в реестре: опрос списка не монтирует тома.
            jp registryDrives
    ELSE
            call    flushVolume
            ret c
            ld      a, (v_drive)
            push    af
            ld      bc, 0
fgd_Loop:
            push    bc
            push    hl
            ld      a, c
            call    fileSelectDrive
            ld      b, 3
            call    j_diskDriver
            ld      a, d
            or      e
            pop     hl
            pop     bc
            jp z,   fgd_Next
            ld      (hl), c
            inc     hl
            inc     b
fgd_Next:
            inc     c
            ld      a, c
            cp      8
            jp c,   fgd_Loop
            pop     af
            call    fileSelectDrive
            jp      dosClearError

    ENDIF

;----------------------------------------------------------------------------
; MXOS
;
; Проверить размещение буфера целиком в доступной памяти.
;
; вход:
;   hl = начало диапазона, bc = длина в байтах.
;
; выход:
;   cf = 0, если конец допустим и адрес не обернулся. При cf = 1 возвращается ошибка
;   аргумента. hl, bc и de сохранены.
;----------------------------------------------------------------------------

fileOpRange:
            push    hl
            push    de
            add     hl, bc
            jp c,   for_Bad
            call    getAppMemoryEnd
            call    cmp_hl_de
            jp c,   for_Good
            jp z,   for_Good
for_Bad:
            pop     de
            pop     hl
fileOpError:
            ld      a,DOS_ERR_ARGUMENT
            call    dosSetError
            ld      a, FILE_OP_ERROR
            scf
            ret
for_Good:
            pop     de
            pop     hl
    IFNDEF BANKED_DOS_BUILD
            ; Временное размещение ядра в основной памяти: буфер и запрос
            ; не должны перекрывать его, даже находясь ниже bios_vars.
            push hl
            push de
            ld de,dosCoreEnd
            call cmp_hl_de
            jp nc,for_MainMemoryOk
            add hl,bc
            ld de,DOS_CODE_START
            call cmp_hl_de
            jp c,for_MainMemoryOk
            jp z,for_MainMemoryOk
            pop de
            pop hl
            jp fileOpError
for_MainMemoryOk:
            pop de
            pop hl
    ENDIF
            or      a
            ret

; DE=исключительная граница доступного буфера в текущем контексте.
; AF/BC/HL и флаги сохранены. Системный стек проверяется отдельно.
getAppMemoryEnd:
    IFDEF BANKED_DOS_BUILD
            ld de,DOS_BANK_STACK_BOTTOM
            ret
    ELSE
            push af
            push hl
            ld hl,(v_memTop)
            ld de,LEGACY_APP_MEMORY_END-1
            ld a,(v_dosModuleReady)
            or a
            jp z,getAppLimitSelected
            ld de,APP_MEMORY_END-1
getAppLimitSelected:
            call cmp_hl_de
            jp c,getAppEndReady
            ex de,hl
getAppEndReady:
            inc hl
            ex de,hl
            pop hl
            pop af
            ret

    ENDIF
