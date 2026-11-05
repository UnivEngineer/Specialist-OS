; Резидентные данные и строки MXOS.
    INCLUDE "volumeData.inc"

v_findCluster:      DW 0  ; Используется в fileFindClusterFirst/fileFindClusterNext
v_fileFirstCluster: DW 0  ; Первый кластер созданного файла
v_cachedDescrPtr:   DW 0  ; Используется в findFile, createFile
v_cachedSector:     DW 0  ; Используется в findFile, createFile
v_newDescrPtr:      DW 0  ; Адрес дескриптора созданного файла
v_foundDescrPtr:    DW 0  ; Адрес дескриптора найденного файла
v_batPtr:           DW 0
v_batDirectory:     DW 0
v_bootAutoexec:     DB 0  ; Адрес буфра, где находится содержимое BAT файла
v_dirFirstFile      DW 0  ; Используется в fileList
v_dirMaxFiles       DW 0  ; Используется в fileList
v_dirListedFiles    DW 0  ; Используется в fileList
v_dirTotalFiles     DW 0  ; Используется в fileList

; Фейковое "системное время". Пока что просто счетчик, увеличивающийся при обращении.
; В будущем может быть заменен на реальное время с RTC или таймера.
v_fakeSystemTime:   DW 0

pathFontFnt:        DB "A:",05Ch,"FONTKOI.FNT",0
pathNcCom:          DB "A:",05Ch,"NC.COM",0
pathAutoexecBat:    DB "A:",05Ch,"AUTOEXEC.BAT",0
pathFlashSysCom:    DB "A:",05Ch,"FLASHSYS.COM",0
pathFormatBat:      DB "A:",05Ch,"FORMAT.BAT",0

    IF BOOT_FROM_TAPE
aATape_com:         DB "A:TAPE.COM",0
txtLoadingFromTape: DB 0Ah,"LOADING FROM TAPE...",0
    ENDIF

txtBiosVer:         DB 0Ch,"MXOS2",0Ah,0
txtRAM:             DB 0Ah,"RAM: ",0
txtKB:              DB " KB",0Ah,0
txtBadCommand:      DB "BAD COMMAND",0

; Имена BAT и команды живут отдельно, их метаданные нужны только до запуска.
; Хвосты 32-байтных дескрипторов перекрываются со следующими буферами.
; fileLoadExit копирует вперёд из v_tmpFileDescr: назначение всегда ниже.
; После запуска дочерней команды BAT заново читает свои метаданные с диска.
v_batFileDescr:      BLOCK FILE_NAME_LENGTH+3,0
v_curFileDescr:      BLOCK FILE_NAME_LENGTH+3,0
v_tmpFileDescr      FILE_DESCRIPTOR ; Независимая копия найденной записи FAT
    ASSERT v_batFileDescr+FILE_DESCR_SIZE <= v_tmpFileDescr+FILE_DESCR_SIZE
    ASSERT v_curFileDescr+FILE_DESCR_SIZE <= v_tmpFileDescr+FILE_DESCR_SIZE

; Буфер для копирования изображения символов при использовании шрифта из ПЗУ

    ; Проверка - DOS.SYS не должен вылезать за эти пределы
    IF NEW_MEMORY_MAP==0
        ASSERT_DONT_FIT 0D000h  ; с этого адреса начинается NC.COM
    ENDIF

; PATH принадлежит окружению DOS и переживает перезагрузку оболочки.
v_cliPath: DB "A:",05Ch,0
           DS MAX_PATH-4,0
