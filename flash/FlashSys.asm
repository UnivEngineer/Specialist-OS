;----------------------------------------------------------------------------
; MXOS
; FLASHSYS.COM - драйвер перезаписываемого системного ROM-диска
;
; Функции драйвера (номер в регистре b):
;   b == 1 - записать сектор (256 байт, номер сектора в de, буфер в hl);
;   b == 2 - прочитать сектор (256 байт, номер сектора в de, буфер в hl);
;   b == 3 - получить размер диска в секторах (de) и DISK_INFO (hl).
;
; Системная ПЗУ отображается в 0000h-7FFFh страницами по 32 КБ:
;   FFFCh - вернуть основное ОЗУ;
;   FFFEh - подключить страницу ROM-диска, D0-D3 = номер страницы.
; Регистр FFF8h здесь не используется: его D0 выбирает ROM/RAM только в
; стандартном режиме "Специалиста", а D1 переключает 4/8 цветов.
;
; Поддерживаются AT29C010A (128 КБ, физическая страница записи 128 байт),
; AT29C020 и AT29C040A (256/512 КБ, страница записи 256 байт).
; Размер физической страницы определяется по BPB уже загруженного системного
; диска: том 128 КБ содержит 0200h логических секторов.
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

DRIVE             = 0       ; A:
DISK_SECTORS_MAX  = 2048    ; 512 КБ / 256 байт
ROM_PAGE_MASK     = 00Fh
ROM_ADDR_HI_MASK  = 07Fh
ROM_128K_SECTORS_HI = 2     ; старший байт числа секторов 0200h

;----------------------------------------------------------------------------

    ; Отладочное размещение. Окно 0DC00h-0DDFFh штатно зарезервировано для
    ; TAPE.COM, поэтому до встраивания драйвера в DOS одновременно можно
    ; использовать FlashSys и Flash.com, но не внешний магнитофонный драйвер.
    ORG     0DC00h

    ; Заменяем встроенный драйвер системного ROM-диска A:.
    ld      a, DRIVE
    ld      hl, Driver
    call    bios_installDriver
    ld      (v_diskInfo), hl
    call    InstallProtection
    ret

;----------------------------------------------------------------------------

; Устанавливаем защитные обёртки стандартных файловых функций. Благодаря
; этому подтверждение работает не только в NC.COM, но и в любой программе,
; использующей штатный BIOS ABI. Повторный запуск драйвера не поддерживается,
; как и у остальных драйверов, загружаемых по адресу 0DE00h.

InstallProtection:
    ld      hl, (bios_fileCreate + 1)
    ld      (CallFileCreate + 1), hl
    ld      hl, (bios_fileDelete + 1)
    ld      (CallFileDelete + 1), hl
    ld      hl, (bios_fileRename + 1)
    ld      (CallFileRename + 1), hl

    ld      hl, ProtectedFileCreate
    ld      (bios_fileCreate + 1), hl
    ld      hl, ProtectedFileDelete
    ld      (bios_fileDelete + 1), hl
    ld      hl, ProtectedFileRename
    ld      (bios_fileRename + 1), hl
    ret

;----------------------------------------------------------------------------
; Подтверждение изменения файлов системного диска A:.
;
; Выход:
;   CF=0 - разрешить операцию;
;   CF=1 - отменить операцию.
;
; Сохраняет:
;   bc, de, hl.
;
; Разрушает:
;   af.

ConfirmSystemChange:
    push    hl
    push    de
    push    bc

    ld      e, 2
    call    bios_fileGetSetDrive
    or      a
    jp nz,  ConfirmAllowed

    ; Строка помещается в командную строку NC. После операции NC перерисует её.
    ld      hl, 001F3h
    ld      (bios_vars.cursorY), hl
    ld      hl, aConfirmSystemChange
    call    bios_printString

ConfirmRelease:
    call    bios_keyScan
    inc     a
    jp nz,  ConfirmRelease

ConfirmWait:
    call    bios_keyScan
    cp      01Bh
    jp z,   ConfirmDenied
    cp      00Dh
    jp nz,  ConfirmWait

ConfirmAllowed:
    pop     bc
    pop     de
    pop     hl
    or      a
    ret

ConfirmDenied:
    pop     bc
    pop     de
    pop     hl
    scf
    ret

ProtectedFileCreate:
    call    ConfirmSystemChange
    ret c
    xor     a
    ld      (bios_vars.diskWriteErr), a
CallFileCreate:
    call    0
    jp      CheckWriteResult

ProtectedFileDelete:
    call    ConfirmSystemChange
    ret c
    xor     a
    ld      (bios_vars.diskWriteErr), a
CallFileDelete:
    call    0
    jp      CheckWriteResult

ProtectedFileRename:
    call    ConfirmSystemChange
    ret c
    xor     a
    ld      (bios_vars.diskWriteErr), a
CallFileRename:
    call    0
    jp      CheckWriteResult

; Сохраняем штатный результат файловой функции, если физический драйвер не
; обнаружил ошибку. При ошибке показываем сообщение и возвращаем CF=1.
CheckWriteResult:
    push    af
    ld      a, (bios_vars.diskWriteErr)
    or      a
    jp z,   CheckWriteOk
    pop     af

    push    hl
    push    de
    push    bc
    ld      hl, 001F3h
    ld      (bios_vars.cursorY), hl
    ld      hl, aWriteError
    call    bios_printString
    call    bios_getch
    pop     bc
    pop     de
    pop     hl
    scf
    ret

CheckWriteOk:
    pop     af
    ret

aConfirmSystemChange:
    DB      "SYSTEM DISK: MODIFY? ENTER/ESC", 0
aWriteError:
    DB      "DISK WRITE ERROR", 0

;----------------------------------------------------------------------------

Driver:
    ld      a, b
    cp      3
    jp z,   FuncSize

    push    hl
    push    de
    push    bc

    dec     a
    jp z,   FuncWrite
    dec     a
    jp z,   FuncRead

Exit:
    ; При любом выходе основное ОЗУ должно быть подключено обратно.
    push    af
    ld      (IO_PAGE_RAM), a
    pop     af
    pop     bc
    pop     de
    pop     hl
    ret

;----------------------------------------------------------------------------
; Функция 3 - размер накопителя.
;
; Выход:
;   de = число секторов;
;   hl = адрес DISK_INFO.

FuncSize:
    ld      hl, (v_diskInfo)
    ld      de, DISK_INFO.isValid
    add     hl, de
    ld      a, (hl)

    ld      de, DISK_SECTORS_MAX
    cp      DISK_VALID
    jp nz,  fsExit

    ld      hl, (v_diskInfo)
    ld      de, DISK_INFO.totalSectors
    add     hl, de
    ld      e, (hl)
    inc     hl
    ld      d, (hl)

fsExit:
    ld      hl, (v_diskInfo)
    ret

;----------------------------------------------------------------------------
; Вычисление страницы ПЗУ и адреса сектора внутри окна 0000h-7FFFh.
;
; Вход:
;   de = номер 256-байтного сектора.
;
; Выход:
;   v_page = номер 32-КБ страницы;
;   de     = адрес начала сектора в отображённой странице.
;
; Разрушает:
;   af.

PrepareAddress:
    ld      a, e
    add     a, a              ; CF = бит 7 младшего байта номера сектора
    ld      a, d
    rla                       ; A = номер сектора / 128
    and     ROM_PAGE_MASK
    ld      (v_page), a

    ld      d, e
    ld      e, 0
    ld      a, d
    and     ROM_ADDR_HI_MASK
    ld      d, a
    ret

;----------------------------------------------------------------------------
; Функция 2 - чтение сектора.
;
; Вход:
;   de = номер сектора;
;   hl = адрес буфера.

FuncRead:
    call    PrepareAddress

; Горячий цикл: 16 байт, 73 такта/байт, около 9,34 мс на сектор при 2 МГц.
; На байт: одно чтение ПЗУ, одна запись ОЗУ и два переключения маппера.
ReadLoop:
    ld      a, (v_page)
    ld      (IO_PAGE_ROM), a
    ld      a, (de)
    ld      (IO_PAGE_RAM), a
    ld      (hl), a
    inc     hl
    inc     e
    jp nz,  ReadLoop

    or      a
    jp      Exit

;----------------------------------------------------------------------------
; Функция 1 - запись сектора.
;
; Вход:
;   de = номер сектора;
;   hl = адрес буфера.

FuncWrite:
    ; AT29C010A имеет 128-байтную физическую страницу. Системный том на таком
    ; чипе содержит 0200h логических секторов, поэтому сектор делится пополам.
    push    hl
    push    de
    ld      hl, (v_diskInfo)
    ld      de, DISK_INFO.totalSectors + 1
    add     hl, de
    ld      a, (hl)
    pop     de
    pop     hl

    ld      c, 0
    cp      ROM_128K_SECTORS_HI
    jp nz,  fwChunkReady
    ld      c, 080h
fwChunkReady:
    ld      a, c
    ld      (v_chunkEnd), a
    ld      (v_chunkCount), a

    call    PrepareAddress
    call    ProgramChunk
    jp c,   Exit

    ; Для 128-КБ чипа записываем вторую 128-байтную половину сектора.
    ld      a, (v_chunkEnd)
    or      a
    jp z,   Exit
    xor     a
    ld      (v_chunkEnd), a
    call    ProgramChunk
    jp      Exit

;----------------------------------------------------------------------------
; Запись одной физической страницы AT29C.
;
; Вход:
;   de = адрес первого байта в отображённой странице ПЗУ;
;   hl = адрес первого байта в ОЗУ;
;   v_chunkEnd = значение E после последнего байта (080h или 00h).
;
; Выход:
;   de и hl указывают за записанный блок.
;
; Разрушает:
;   af, bc.

ProgramChunk:
    ; Software Data Protection: AAh@5555h, 55h@2AAAh, A0h@5555h.
    ld      a, (v_page)
    ld      (IO_PAGE_ROM), a
    ld      a, 0AAh
    ld      (05555h), a
    ld      a, 055h
    ld      (02AAAh), a
    ld      a, 0A0h
    ld      (05555h), a
    ld      (IO_PAGE_RAM), a

; Горячий цикл: 22 байта, 100 тактов/байт = 50 мкс при 2 МГц.
; Это меньше максимального tBLC=150 мкс. На байт: одно чтение ОЗУ, одна
; запись ПЗУ и два переключения маппера.
ProgramLoop:
    ld      a, (hl)
    ld      c, a
    ld      a, (v_page)
    ld      (IO_PAGE_ROM), a
    ld      a, c
    ld      (de), a
    ld      (IO_PAGE_RAM), a
    inc     hl
    inc     e
    ld      a, (v_chunkEnd)
    cp      e
    jp nz,  ProgramLoop

    ; DATA polling последнего записанного байта. Худший путь (768 проверок)
    ; длится около 16,5 мс и превышает максимальное tWC=10 мс.
    dec     e
    ld      a, (v_page)
    ld      (IO_PAGE_ROM), a
    push    hl
    ld      b, 3
    ld      l, 0
PollLoop:
    ld      a, (de)
    xor     c
    and     080h
    jp z,   PollDone
    dec     l
    jp nz,  PollLoop
    dec     b
    jp nz,  PollLoop

    ; Тайм-аут: это ожидаемый результат при аппаратной защите /WE или ПЗУ.
    ld      (IO_PAGE_RAM), a
    pop     hl
    inc     e
    jp      WriteFailed

PollDone:
    ld      (IO_PAGE_RAM), a
    pop     hl
    inc     e

    ; DATA polling подтверждает окончание внутреннего цикла, после него
    ; дополнительно сравниваем каждый байт физической страницы.
    push    hl
    push    de
    ld      a, (v_chunkCount)
    ld      b, a
VerifyLoop:
    dec     hl
    dec     e
    ld      a, (hl)
    ld      c, a
    ld      a, (v_page)
    ld      (IO_PAGE_ROM), a
    ld      a, (de)
    ld      (IO_PAGE_RAM), a
    cp      c
    jp nz,  VerifyFailed
    dec     b
    jp nz,  VerifyLoop

    pop     de
    pop     hl
    or      a
    ret

VerifyFailed:
    pop     de
    pop     hl

WriteFailed:
    ld      a, 1
    ld      (bios_vars.diskWriteErr), a
    scf
    ret

;----------------------------------------------------------------------------

v_diskInfo: DW 0
v_page:     DB 0
v_chunkEnd: DB 0
v_chunkCount: DB 0

;----------------------------------------------------------------------------

    ASSERT_DONT_FIT 0DE00h

    END
