    ASSERT $ == BANK_DOS_GATE
    jp kernelDispatch
    jp kernelLocalDispatch
;----------------------------------------------------------------------------
; MXOS
;
; Таблица входов запуска DOS из банкового транспорта.
;
; вход:
;   Выбрана страница DOS и установлен стек. dosBoot вызывает kernelCold,
;   dosBoot+3 вызывает kernelWarm с режимом в c.
;
; выход:
;   Управление возвращается вызывающему с результатом выбранной подпрограммы.
;----------------------------------------------------------------------------

dosBoot:
    jp kernelCold
    jp kernelWarm
;----------------------------------------------------------------------------
; MXOS
;
; Холодный запуск файлового ядра и выбор системного тома A:.
;
; вход:
;   Образ DOS загружен, транспорт и стек установлены. memoryRamFresh
;   содержит признак состояния RAM-диска, переданный загрузчиком.
;
; выход:
;   cf = 0: a = 0, выбран A:, v_dosModuleReady = 1, AUTOEXEC разрешён.
;   cf = 1: a = ошибка подготовки драйверов или выбора A:.
;   Начальное состояние памяти, модулей и кэша установлено заново.
;   bc, de, hl и прочие флаги не сохраняются.
;----------------------------------------------------------------------------

kernelCold:
    call dosInitVariables
    call resetAllSectorCaches
    ld a,1
    ld (v_bootAutoexec),a
    xor a
    ld (fontConfigValid),a
    ld (fontConfigPending),a
    ld (fontConfigRestoring),a
    call memoryInit
    call moduleInit
    call registryBoot
    ret c
kernelNoFlash:
    ld hl,kernelVersion
    call bankDosPrintString
    ld a,SYSTEM_DRIVE
    call fileChangeDrive
    ret c
    ld a,1
    ld (v_dosModuleReady),a
    xor a
    ret
;----------------------------------------------------------------------------
; MXOS
;
; Начальная установка блока переменных DOS при холодном запуске.
;
; вход:
;   Страница DOS доступна для записи. Шаблон начинается по dosColdDefaults.
;
; выход:
;   [dosWorkStart, dosDefaultsEnd) содержит копию шаблона,
;   [dosDefaultsEnd, dosZeroEnd) заполнен нулями. memoryRamFresh сохранён.
;   a = 0, bc = 0, hl = dosZeroEnd, z = 1, cf = 0. de изменяется.
;----------------------------------------------------------------------------

dosInitVariables:
    ld hl,dosColdDefaults
    ld de,dosWorkStart
    ld bc,dosDefaultsEnd-dosWorkStart
    call moveBytesForward
    ld hl,dosDefaultsEnd
    ld bc,dosZeroEnd-dosDefaultsEnd
    jp cliZero

j_reboot3:
    jp dosBoot
kernelVersion:      DB "DOS ",'0'+DOS_VERSION_MAJOR,'.','0'+DOS_VERSION_MINOR,0Dh,0Ah,0

;----------------------------------------------------------------------------
; MXOS
;
; Возврат в окружение DOS после отбрасывания стеков программ.
;
; вход:
;   c = 1 перед AUTOEXEC, c = 0 при входе в оболочку. Ядро и тома
;   уже инициализированы, вызывающий предоставил рабочий стек.
;
; выход:
;   cf = 0: сброшены текущая очередь, контекст BAT и обработчик CLI,
;   завершён сброс файлового кэша на носитель.
;   cf = 1: отказ при возврате владения драйвером или сбросе кэша.
;   af, bc, de, hl не сохраняются.
;----------------------------------------------------------------------------

kernelWarm:
    ; Очередь со старого стека нужно отвязать до физического копирования.
    ld hl,0
    call mx_queueBind
    xor a
    ld (v_batDepth),a
    call batDiscard
    ld hl,BAT_BUFFER-BAT_FRAME_SIZE
    ld (v_batFrame),hl
    ld a,c
    ld (v_bootAutoexec),a
    call klReclaim
    ret c
    call klInvalidate
    xor a
    ld (kdBusy),a
    ld (kdFarActive),a
    ld (ksDirect),a
    ld (diActive),a
    call moduleWarm
    ld a,(v_bootAutoexec)
    or a
    call nz,fontConfigReset
    call memoryPublishTop
    ld hl,0
    call cliSetObserver
    jp flushVolume
