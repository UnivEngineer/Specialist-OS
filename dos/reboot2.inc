;+---------------------------------------------------------------------------
; MXOS
; Перезагрузка (продолжение)
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

reboot2:
            ; Очищаем экран
            call    clearScreen

    IF BOOT_FROM_TAPE
            ; Проверяем, нажата ли какая нибудь клавиша
            call    j_keyScan
            push    af
    ENDIF

            ; Выводим версию ОС на экран
            ld      hl, txtBiosVer
            call    printString1

            ; Определяем размер ДОЗУ
            ld      hl, RAMD_PAGE_END
            ld      a, (bios_vars.maxRamPage)
            call    checkRAMD

            ; Выводим объем ДОЗУ на экран
            ld      hl, txtRAM
            call    printString1

            ; Вычисляем объем ДОЗУ в байтах
            ld      a, (bios_vars.maxRamPage)
            ld      h, 0
            ld      l, a
            inc     l       ; hl = объём в страницах
            inc     l       ; +1 страница - основная память
            add     hl, hl
            add     hl, hl
            add     hl, hl
            add     hl, hl
            add     hl, hl
            add     hl, hl  ; hl = hl * 64
            ex      hl, de
            call    printDecWord

            ld      hl, txtKB
            call    printString1

        IF BOOT_FROM_TAPE==0

            ; Продолжаем загрузку системы
            jp      j_reboot3

        ELSE

            ; Если не нажата ни одна клавиша, продолжаем загрузку системы
            pop     af
            inc     a
            jp z,   j_reboot3

            ; Загружаем драйвер магнитофона
            ld      hl, aATape_com
            call    fileExec

            ; Загружаем программу с ленты (без контроля КС) и запускаем
            ld      hl, txtLoadingFromTape
            call    printString1
            call    t_tapeLoadInt
            ld      hl, (bios_vars.tapeAddr)
            jp      (hl)

        ENDIF
