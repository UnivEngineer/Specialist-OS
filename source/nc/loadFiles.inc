; Каталог панели кэшируется целиком до предела 8-разрядной навигации.
; Вход A=диск, активная панель задаёт отдельный кэш. AF/BC/DE/HL разрушены.
; На попадании кэша дисковый список не читается; выбирается окно указателем.
; Горячий путь со штатным выбором уже активного диска: 867 тактов в CPU-тесте
; 8080 (около 0,43 мс при 2 МГц), 0 обращений к носителю. Вместо него
; перечитывание H: на 140 файлов занимало 491253 такта. Временного буфера
; и копирования окна нет; чтение полного каталога выполняется только на miss.
loadFiles:
            ld      c, a
            ld      e, 1
            call    bios_fileSelectDrive
            push    bc
            call    restorePanelDirectory
            pop     bc
            ld      hl, vars.cacheValidA
            call    readBytePanel
            or      a
            jp z,   lf_ReadCatalog
            ld      hl, vars.cacheDriveA
            call    readBytePanel
            cp      c
            jp z,   lf_Cached

lf_ReadCatalog:
            ld      a, c
            ld      hl, vars.cacheDriveA
            call    writeBytePanel
            call    fileCacheBase
            ld      bc, 0
            ld      de, FILE_CACHE_CAPACITY
            ld      a, DIR_OP_LIST
            call    bios_directory
            jp nc,  lf_ListReady
            ld      de,0
            ld      (hl),0FFh
lf_ListReady:
            ; Запись FAT "." становится явным пунктом корня только в кэше NC.
            push hl
            push de
            call fileCacheBase
            ld a,(hl)
            cp '.'
            jp nz,lf_RootReady
            inc hl
            ld a,(hl)
            cp ' '
            dec hl
            jp nz,lf_RootReady
            ld (hl),05Ch
lf_RootReady:
            pop de
            pop hl

            ; Индексы NC пока байтовые. Больше 255 файлов не оборачиваем в 0.
            ld      a, d
            or      a
            ld      a, e
            jp z,   lf_StoreCount
            ld      a, FILE_CACHE_CAPACITY
lf_StoreCount:
            ld      hl, state.panelA_filesCnt
            call    writeBytePanel
            ld      a, 1
            ld      hl, vars.cacheValidA
            call    writeBytePanel

lf_Cached:
            ; Удаление могло сократить каталог. Пустая панель имеет позиции 0.
            ld      hl, state.panelA_filesCnt
            call    readBytePanel
            ld      c, a
            or      a
            jp z,   lf_StoreCursor
            dec     a
            ld      b, a
            ld      hl, state.panelA_curFile
            call    readBytePanel
            cp      c
            jp c,   lf_CursorReady
            ld      a, b
lf_StoreCursor:
            ld      hl, state.panelA_curFile
            call    writeBytePanel
lf_CursorReady:
            ld      c, a
            ld      hl, state.panelA_firstFile
            call    readBytePanel
            cp      c
            jp c,   lf_View
            jp z,   lf_View
            ld      a, c
            call    writeBytePanel

lf_View:
            ; HL = начало видимого окна, без копирования дескрипторов.
            ld      l, a
            ld      h, 0
            add     hl, hl
            add     hl, hl
            add     hl, hl
            add     hl, hl
            ex      de, hl
            call    fileCacheBase
            add     hl, de
            ld      (vars.fileListPtr), hl
            ret

; HL = база кэша активной панели; A/HL/флаги разрушены, BC/DE сохранены.
fileCacheBase:
            ld      hl, FILE_CACHE_A
            ld      a, (state.activePanel)
            or      a
            ret z
            ld      hl, FILE_CACHE_B
            ret

; Полная перерисовка вызывается после файловых операций и выбора диска.
; Помечает только кэш активной панели; A/HL/флаги разрушены.
invalidateFileCache:
            xor     a
            ld      hl, vars.cacheValidA
            jp      writeBytePanel
