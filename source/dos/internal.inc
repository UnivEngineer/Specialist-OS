; MXOS
; Внутренние точки входа и пакеты ядра DOS.

    IFNDEF MXOS_DOS_INTERNAL_INC
    DEFINE MXOS_DOS_INTERNAL_INC

    INCLUDE "../../sdk/asm/formats/directory.inc"

; Локальная таблица DOS в её странице; приложения используют общий MXOS_CALL_REGS.
    IFDEF DOS_REFERENCE_BUILD
DOS_ENTRY_BASE = 04000h  ; Только линковка BIOS и CPU-стенды, не штатная DOS.
    ELSE
DOS_ENTRY_BASE = 00100h
    ENDIF
dos_dosBoot           = DOS_ENTRY_BASE+000h
dos_fileList          = DOS_ENTRY_BASE+003h
dos_fileGetSetDrive   = DOS_ENTRY_BASE+006h
dos_fileDelete        = DOS_ENTRY_BASE+009h
dos_fileRename        = DOS_ENTRY_BASE+00Ch
dos_fileLoadInfo      = DOS_ENTRY_BASE+00Fh
dos_fileGetSetAttr    = DOS_ENTRY_BASE+012h
dos_fileNamePrepare   = DOS_ENTRY_BASE+015h
dos_fileExec          = DOS_ENTRY_BASE+018h
dos_getDriveFreeSpace = DOS_ENTRY_BASE+01Bh
dos_dosUnsupported    = DOS_ENTRY_BASE+01Eh
dos_mountVolume       = DOS_ENTRY_BASE+021h
dos_flushVolume       = DOS_ENTRY_BASE+024h
dos_fileAssociation   = DOS_ENTRY_BASE+027h
dos_fileChangeDrive   = DOS_ENTRY_BASE+02Ah
dos_fileCopy          = DOS_ENTRY_BASE+02Dh
dos_fileGetDrives     = DOS_ENTRY_BASE+030h
dos_dosGetError       = DOS_ENTRY_BASE+033h
dos_logicalSectorIO   = DOS_ENTRY_BASE+036h
dos_fileHandle        = DOS_ENTRY_BASE+039h
dos_directoryService  = DOS_ENTRY_BASE+03Ch
dos_cliService        = DOS_ENTRY_BASE+03Fh
dos_setShell          = DOS_ENTRY_BASE+042h

;-----------------------------------------------------------------------
; Внутренний контекст загрузки COM/EXE: IMAGE_LOAD_CONTEXT, 32 байта
;-----------------------------------------------------------------------

; Временный внутренний контекст родного загрузчика C854h/C848h.
; Он действителен до следующей файловой операции и не является записью FAT.
    STRUCT IMAGE_LOAD_CONTEXT
prefix          BLOCK  6
loadAddress     DW     0
entryAddress    DW     0
bodySize        DW     0
bodyChecksum    DW     0
headerChecksum  DW     0
skip            DB     0
_reserved       DB     0
tail            DW     0
_reserved2      BLOCK  6
nextCluster     DW     0
fileSize        DW     0
fileSizeHi      DW     0
    ENDS
    ASSERT IMAGE_LOAD_CONTEXT == FILE_DESCR_SIZE

;-----------------------------------------------------------------------
; Внутренний курсор FAT для FILE_HANDLE; самостоятельного публичного вызова нет.
;-----------------------------------------------------------------------
; Четыре байта в памяти вызывающей программы: кластер и смещение внутри него.
; DE=адрес курсора, HL=буфер, BC=число байтов, A=1 запись / 2 чтение.
; DOS обновляет курсор и при частичном отказе; размер файла здесь не хранится.
; Вызывающий ограничивает чтение EOF и заранее выделяет цепочку для записи.

    STRUCT FILE_STREAM_CURSOR
cluster     DW 0               ; Текущий кластер FAT-цепочки.
offset      DW 0               ; Смещение в кластере, не абсолютная позиция.
    ENDS

    ENDIF
