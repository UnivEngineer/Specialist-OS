; Резидентные данные и строки MXOS.
    INCLUDE "volumeData.inc"

v_findCluster:      DW 0  ; Используется в fileFindClusterFirst/fileFindClusterNext
v_fileFirstCluster: DW 0  ; Первый кластер созданного файла
v_cachedDescrPtr:   DW 0  ; Используется в findFile, createFile
v_cachedSector:     DW 0  ; Используется в findFile, createFile
v_newDescrPtr:      DW 0  ; Адрес дескриптора созданного файла
v_foundDescrPtr:    DW 0  ; Адрес дескриптора найденного файла
v_batEcho:          DB 1  ; BAT печатает команды, пока не выполнено ECHO OFF.
v_execDrive:        DB 0
v_bootAutoexec:     DB 0  ; Адрес буфра, где находится содержимое BAT файла
v_dirFirstFile      DW 0  ; Осталось пропустить в fileList
v_dirMaxFiles       DW 0  ; Остаток ёмкости fileList
v_dirTotalFiles     DW 0  ; Используется в fileList

; Фейковое "системное время". Пока что просто счетчик, увеличивающийся при обращении.
; В будущем может быть заменен на реальное время с RTC или таймера.
v_fakeSystemTime:   DW 0

pathNcCom:          DB "A:",05Ch,"NC.COM",0
pathAutoexecBat:    DB "A:",05Ch,"AUTOEXEC.BAT",0
pathFormatBat:      DB "A:",05Ch,"FORMAT.BAT",0



txtBadCommand:      DB "BAD COMMAND OF FILE NAME",0

; Имя команды и временная копия записи живут отдельно.
; Хвосты 32-байтных дескрипторов перекрываются со следующими буферами.
; fileLoadExit копирует вперёд из v_tmpFileDescr: назначение всегда ниже.
v_curFileDescr:      BLOCK FILE_NAME_LENGTH+3,0
v_tmpFileDescr      FILE_DESCRIPTOR ; Независимая копия найденной записи FAT
    ASSERT v_curFileDescr+FILE_DESCR_SIZE <= v_tmpFileDescr+FILE_DESCR_SIZE

; PATH принадлежит окружению DOS и переживает перезагрузку оболочки.
v_cliPath: DB "A:",05Ch,0
           DS MAX_PATH-4,0
