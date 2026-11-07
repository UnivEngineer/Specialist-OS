; Общие файловые операции. Диалоги и выбор памяти принадлежат вызывающему.
; Вызовы не реентерабельны, как остальные файловые функции DOS.

; A=диск 0..7; BC/DE/HL сохранены. CF=ошибка выбора/монтирования.
fileChangeDrive:
            cp      8
            jp nc,  fileOpError
            call    fileSelectDrive
            ret c
            ld      a,(v_diskInfo.isValid)
            cp      DISK_VALID
    IFDEF DOS_SPLIT_BUILD
            jp z,   dosClearError
    ELSE
            ret z
    ENDIF
            add     a,DOS_ERR_NO_DRIVER
            jp      dosSetError

; HL=буфер минимум на 8 номеров, BC=ёмкость в байтах.
; Выход B=число установленных дисков, CF=ошибка параметров.
; Исходный диск восстановлен. AF/BC/DE/HL разрушены.
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
    IFDEF DOS_SPLIT_BUILD
            jp      dosClearError
    ELSE
            xor     a
            ret
    ENDIF

    ENDIF

; HL=начало, BC=число байтов; только обычное ОЗУ, без оборота адреса.
; HL/BC/DE сохранены; AF/флаги разрушены, CF=ошибка.
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
    IFDEF DOS_SPLIT_BUILD
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
    ENDIF
            or      a
            ret

; DE=исключительная граница программы, минимум memtop+1 и начала DOSMOD.
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
