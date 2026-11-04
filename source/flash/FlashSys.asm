;----------------------------------------------------------------------------
; MXOS
; FLASHSYS.COM - драйвер системного flash-диска AT29C010A/C020/C040A
;
; Номер функции передаётся в регистре b:
;   b == 1 - записать сектор (256 байт, номер сектора в de, буфер в hl);
;   b == 2 - прочитать сектор (256 байт, номер сектора в de, буфер в hl);
;   b == 5 - записать сектор, затем прочитать и сравнить все 256 байт;
;   b == 3 - размер диска в de, DISK_INFO в hl, bc=0 (линейное выделение);
;   b == 0FFh - адрес имени и описания в hl (две строки с нулём).
;
; Для чтения и записи CF=0 означает успех, CF=1 - ошибку. После загрузки
; страницы ожидается завершение по DATA polling. Полное сравнение выполняется
; в функции B=5; обычная запись B=1 не выполняет полного сравнения.
;
; Системная ПЗУ отображается в 0000h-7FFFh страницами по 32 КБ:
;   FFFCh - вернуть основное ОЗУ;
;   FFFEh - подключить страницу ROM/flash-диска, D0-D3 = номер страницы.
; Регистр FFF8h здесь не используется: его D0 выбирает ROM/RAM только в
; стандартном режиме "Специалиста", а D1 переключает 4/8 цветов.
; Чтение в 8000h и выше выполняется одним отображением ROM через memcpy DOS;
; в нижнее ОЗУ — парами через BC. Стек и исполняемый код находятся выше 8000h.
; Для функций 1/2/5 BC, DE, HL сохраняются; AF и флаги разрушаются.
;
; Поддерживаются AT29C010A (128 КБ, физическая страница записи 128 байт),
; AT29C020 и AT29C040A (256/512 КБ, страница записи 256 байт).
; Ёмкость и физическая страница определяются по software ID, независимо от BPB.
; COM загружается с 0000h; ID выполняется временным кодом в 8200h–82FFh,
; после успешного ID резидент копируется в F600h–F73Fh.
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

DRIVE             = 0       ; A:
ROM_PAGE_MASK     = 00Fh
ROM_ADDR_HI_MASK  = 07Fh

    ; Варианты нижнего чтения для воспроизводимого сравнения на стенде.
    ; 0 — старый цикл; 1 — пара через BC (штатный); 2 — через верхний буфер.
    IFNDEF FLASHSYS_READ_MODE
FLASHSYS_READ_MODE = 1
    ENDIF
    ASSERT FLASHSYS_READ_MODE >= 0 && FLASHSYS_READ_MODE <= 2
    IF FLASHSYS_READ_MODE == 2
    ; Буфер принадлежит стенду/вызывающей системе, не общему драйверу.
    ASSERT FLASHSYS_READ_BUFFER >= 08000h && FLASHSYS_READ_BUFFER <= 08E00h
    ENDIF

;----------------------------------------------------------------------------

    ; F600h-F73Fh — FLASHSYS, F740h-F7FFh — RAMD.COM.
    ; FlashSys, RAMD и внешний Flash в F800h-F9FFh работают одновременно.
RESIDENT_ADDRESS = SYSTEM_FLASH_ADDR
SYSTEM_FLASH = 1

    ORG 0
    jp Install
resident_file:
    DISP RESIDENT_ADDRESS

Driver:
    ld      a, b
    cp      0FFh
    jp z,   FuncInfo
    cp      5
    jp z,   FuncWriteChecked
    cp      3
    jp z,   FuncSize

    push    hl
    push    de
    push    bc

    ld      a, d
SectorLimit:
    cp      0                     ; старший байт физической ёмкости
    jp nc,  InvalidFunction
    ld      a, b
    dec     a
    jp z,   FuncWrite
    dec     a
    jp z,   FuncRead
InvalidFunction:
    scf

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
; Функция 3 - определение объёма накопителя.
;
; Выход:
;   de = число секторов;
;   hl = адрес DISK_INFO.

FuncSize:
    ld      de, 0                 ; физическая ёмкость, патчит установщик
    ld      hl, 0                 ; DISK_INFO, патчит установщик
    ld      bc, 0
    or      a
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
; Функция 2 - чтение сектора из системной flash-памяти.
;
; Вход:
;   de = номер сектора;
;   hl = адрес буфера.

FuncRead:
    call    PrepareAddress

    ; Верхняя половина ОЗУ доступна при подключённом ROM. Код memcpy и
    ; стек также выше 8000h; нижний стек не допускается контрактом MXOS.
    ld      a, h
    or      a
    jp m,   ReadMapped

    IF FLASHSYS_READ_MODE == 2
    push    hl                  ; назначение в нижней памяти
    ld      hl, FLASHSYS_READ_BUFFER
    call    ReadMappedCopy
    ld      (IO_PAGE_RAM), a
    pop     bc
    ld      hl, FLASHSYS_READ_BUFFER
    ld      de, FLASHSYS_READ_BUFFER + 256
    call    bios_memcpy_bc_hl
    or      a
    jp      Exit
    ENDIF

    IF FLASHSYS_READ_MODE == 1
    ld      a, (v_page)
    ld      b, a
; Пара: 19 байт, 94 такта/2 байта; 256 чтений/записей и 256
; переключений на сектор. B=банк, C=первый байт; входные BC на стеке.
ReadPair:
    ld      a, b
    ld      (IO_PAGE_ROM), a
    ld      a, (de)
    ld      c, a
    inc     e
    ld      a, (de)
    ld      (IO_PAGE_RAM), a
    ld      (hl), c
    inc     hl
    ld      (hl), a
    inc     hl
    inc     e
    jp nz,  ReadPair
    ELSE
    IF FLASHSYS_READ_MODE == 0

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
    ENDIF
    ENDIF

    or      a
    jp      Exit

ReadMapped:
    call    ReadMappedCopy
    or      a
    jp      Exit

; DE=адрес ROM-сектора, HL=верхнее назначение. AF/BC/DE/HL разрушаются.
; Копирует 256 байт новым движком DOS, оставляет ROM подключённым.
; На сектор 256 чтений/записей, одно подключение ROM; RAM вернёт вызывающий.
; Пары движка: 64 такта/2 байта (назад), плюс постоянная настройка.
ReadMappedCopy:
    ld      b, h
    ld      c, l
    ex      de, hl
    ld      d, h
    inc     d
    ld      e, l
    ld      a, (v_page)
    ld      (IO_PAGE_ROM), a
    jp      bios_memcpy_bc_hl

;----------------------------------------------------------------------------
; Функция 1 - запись сектора в системную flash-память.
;
; Вход:
;   de = номер сектора;
;   hl = адрес буфера.

FuncWrite:
    ; Размер страницы определяется ID физического чипа, а не размером тома.
    ld      a, (v_halfPage)
    ld      (v_chunkEnd), a

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
; Запись одной физической страницы AT29C и ожидание готовности.
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
    ; Отключение программной защиты от записи: AAh@5555h, 55h@2AAAh, A0h@5555h.
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

    ; Ожидание окончания программирования по DQ7 последнего байта. Худший путь (768 проверок)
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

    or      a
    ret

WriteFailed:
    ld      a, 1
    ld      (bios_vars.diskWriteErr), a
    scf
    ret

;----------------------------------------------------------------------------


; B=0FFh: HL=две строки, B=технология, C=возможности (DISK_CAP_*).
FuncInfo:
    ld      bc, DISK_TECH_FLASH * 256 + DISK_CAP_READ + DISK_CAP_WRITE + DISK_CAP_VERIFY
    ld      hl, DriverInfo
    or      a
    ret
DriverInfo:        DB "FLASHSYS",0
DriverDescription: DB "000K mapped",0

; B=5: общий синхронный цикл B=1, B=2 и сравнения в DOS.
; BC/DE/HL сохраняются; CF=1 и diskWriteErr=1 при отказе.
FuncWriteChecked:
    push    bc
    ld      bc, Driver
    call    bios_diskWriteChecked
    pop     bc
    ret

v_page:         DB 0
v_chunkEnd:     DB 0
v_halfPage:     DB 0

;----------------------------------------------------------------------------

resident_end:
    ASSERT_DONT_FIT RAMD_RESIDENT_ADDR
    ENT
resident_file_end:
    INCLUDE "flash-install.inc"


    END
