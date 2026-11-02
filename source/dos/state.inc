; Резидентные данные и строки MXOS.
    INCLUDE "volumeData.inc"

v_drive:            DB 1  ; Текущий накопитель
v_findCluster:      DW 0  ; Используется в fileFindClusterFirst/fileFindClusterNext
v_fileFirstCluster: DW 0  ; Первый кластер созданного файла
v_input_start:      DW 0  ; Используется в input, createFile
v_input_end:        DW 0  ; Используется в input, createFile
v_cachedDescrPtr:   DW 0  ; Используется в findFile, createFile
v_cachedSector:     DW 0  ; Используется в findFile, createFile
v_newDescrPtr:      DW 0  ; Адрес дескриптора созданного файла
v_foundDescrPtr:    DW 0  ; Адрес дескриптора найденного файла
v_batPtr:           DW 0
v_bootAutoexec:     DB 0  ; Адрес буфра, где находится содержимое BAT файла
v_memTop:           DW bios_vars-1 ; Максимальный доступный программам адрес
v_dirFirstFile      DW 0  ; Используется в fileList
v_dirMaxFiles       DW 0  ; Используется в fileList
v_dirListedFiles    DW 0  ; Используется в fileList
v_dirTotalFiles     DW 0  ; Используется в fileList

; Фейковое "системное время". Пока что просто счетчик, увеличивающийся при обращении.
; В будущем может быть заменен на реальное время с RTC или таймера.
v_fakeSystemTime:   DW 0

; Адреса драйверов для 8 накопителей. Начальные значения:
; = diskDriver для накопителей A: и B: (встроенный драйвер ROM и RAM диска),
; = diskDriverDummy для отальных (пустой драйвер).
v_drives:           DW diskDriver,      diskDriver  ; A:, B:
                    DW diskDriverDummy, diskDriverDummy  ; C:, D:
                    DW diskDriverDummy, diskDriverDummy  ; E:, F:
                    DW diskDriverDummy, diskDriverDummy  ; G:, H:

; Информация о текущем накопителе
v_diskInfo          DISK_INFO

pathFontFnt:        DB "A:FONT.FNT",0
pathNcCom:          DB "A:NC.COM",0
pathAutoexecBat:    DB "A:AUTOEXEC.BAT",0
pathFlashSysCom:    DB "A:FLASHSYS.COM",0
pathFormatBat:      DB "A:FORMAT.BAT",0

    IF BOOT_FROM_TAPE
aATape_com:         DB "A:TAPE.COM",0
txtLoadingFromTape: DB 0Ah,"LOADING FROM TAPE...",0
    ENDIF

txtBiosVer:         DB 0Ch,"BIOS 5.00",0Ah,0
txtRAM:             DB 0Ah,"RAM: ",0
txtKB:              DB " KB",0Ah,0
txtBadCommand:      DB 0Ah,"BAD COMMAND OR FILE NAME",0

v_tmpFileDescr 		FILE_DESCRIPTOR ; Копия дескриптора текущего файла
v_curFileDescr 		FILE_DESCRIPTOR ; Копия дескриптора текущего файла
v_batFileDescr 		FILE_DESCRIPTOR ; Копия дескриптора текущего BAT файла

; Буфер для копирования изображения символов при использовании шрифта из ПЗУ
v_char:             BLOCK   13, 0FFh

    ; Проверка - DOS.SYS не должен вылезать за эти пределы
    IF NEW_MEMORY_MAP==0
        ASSERT_DONT_FIT 0D000h  ; с этого адреса начинается NC.COM
    ENDIF
