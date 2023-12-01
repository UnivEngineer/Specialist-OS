;----------------------------------------------------------------------------
; MXOS - BIOS и DOS
;
; 2013-12-12 Дизассемблировано и доработано vinxru
; 2022-01-31 Доработано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

; -----------------------------------------------------------------------
; Код
; Стандартные подпрограммы должны иметь фиксированный адрес для
; совместимости. Они начинаются с ORG_PAD xxxx. В случае, если
; предыдущая продпрограмма залезает на эту, транслятор выдаст ошибку.
; -----------------------------------------------------------------------

	ORG     0C000h
	INCLUDE "jmps_c000.inc"

    ORG_PAD 0C010h
	INCLUDE "clearScreen.inc"

    ORG_PAD 0C037h
	INCLUDE "printChar.inc"

    ORG_PAD 0C045h
	INCLUDE "printChar5.inc" ; Продолжается в drawChar
	INCLUDE "drawChar.inc"
	INCLUDE "printChar3.inc"

    ORG_PAD 0C170h
	INCLUDE "beep.inc"

    ORG_PAD 0C18Fh
	INCLUDE "delay_l.inc"

    ORG_PAD 0C196h
	INCLUDE "printChar4.inc" ; Продолжается в scrollUp
	INCLUDE "scrollUp.inc"
	INCLUDE "keyScan.inc"
	INCLUDE "getch2.inc"
	INCLUDE "calcCursorAddr.inc"

    ; тут есть место 15 байт

    ORG_PAD 0C337h
	INCLUDE "getch.inc"
	INCLUDE "calcCursorAddr2.inc"
	INCLUDE "drawCursor.inc"

    ; тут есть место 6 байт

    ORG_PAD 0C377h
    INCLUDE "tape.inc"
	INCLUDE "cmp_hl_de_2.inc"
	INCLUDE "sbb_de_hl_to_hl.inc"
	INCLUDE "memmove_bc_hl.inc"
	INCLUDE "memset_de_20_b.inc"

    ORG_PAD 0C3D0h
    jp  t_tapeWrite

    IF RAMFOS_COMPATIBILITY
	INCLUDE "strToHex.inc"
    ENDIF

    ; тут есть место 15 байт

    ORG_PAD 0C427h
	INCLUDE "cmp_hl_de.inc"

    ORG_PAD 0C42Dh
	INCLUDE "memcpy_bc_hl.inc"

    ORG_PAD 0C438h
	INCLUDE "printString1.inc"
	INCLUDE "printChar6.inc"
    
    ORG_PAD 0C443h
    INCLUDE "drawCursor2.inc"

    ; тут есть место 29 байт

    ORG_PAD 0C478h
    jp  t_tapeReadError

;---------------------------------------------------------------------------
; Константы и Переменные
;---------------------------------------------------------------------------

; Начальные значения переменных хранятся здесь, и при сбросе
; копируются на адрес bios_vars (=8FDFh) функцией reboot1
initVars        BIOS_VARIABLES
initVarsEnd:

; Таблица кодов клавиш
v_keybTbl:
    DB 81h, 0Ch, 19h, 1Ah, 09h, 1Bh, 20h, 08h, 80h, 18h, 0Ah, 0Dh, 0, 0, 0, 0
    DB 71h, 7Eh, 73h, 6Dh, 69h, 74h, 78h, 62h, 60h, 2Ch, 2Fh, 7Fh, 0, 0, 0, 0
    DB 66h, 79h, 77h, 61h, 70h, 72h, 6Fh, 6Ch, 64h, 76h, 7Ch, 2Eh, 0, 0, 0, 0
    DB 6Ah, 63h, 75h, 6Bh, 65h, 6Eh, 67h, 7Bh, 7Dh, 7Ah, 68h, 3Ah, 0, 0, 0, 0
    DB 3Bh, 31h, 32h, 33h, 34h, 35h, 36h, 37h, 38h, 39h, 30h, 2Dh, 0, 0, 0, 0
    DB 00h, 01h, 02h, 03h, 04h, 05h, 06h, 07h, 8Ah, 8Bh, 8Ch, 1Fh, 0, 0, 0, 0

;---------------------------------------------------------------------------
; Код
;---------------------------------------------------------------------------

    ;ORG_PAD 0C500h
	INCLUDE "printer.inc"
	INCLUDE "printString.inc"
	INCLUDE "reboot1.inc"
	INCLUDE "reboot2.inc"
	INCLUDE "getch3.inc"
	INCLUDE "printChar2.inc"	; Продолжается в scrollDown
	INCLUDE "scrollDown.inc"
	INCLUDE "scrollUp2.inc"
	INCLUDE "checkRAMD.inc"

    ; тут есть место 17 байт

;---------------------------------------------------------------------------
; Точки входа 0C800h
;---------------------------------------------------------------------------

	ORG_PAD 0C800h
	INCLUDE "jmps_c800.inc"
	INCLUDE "setGetCursorPos.inc"
	INCLUDE "setGetMemTop.inc"
	INCLUDE "printHex.inc"
	INCLUDE "input.inc"
	INCLUDE "calcCS.inc"
	INCLUDE "reboot3.inc"
	INCLUDE "fileExecBat.inc"
	INCLUDE "fileExec.inc"
	INCLUDE "driverFFC0.inc"
	INCLUDE "driver.inc"
	INCLUDE "installDriver.inc"
	INCLUDE "fileGetSetDrive.inc"
    INCLUDE "strcmp.inc"
	INCLUDE "fatCache.inc"
	INCLUDE "fatFindCluster.inc"
    INCLUDE "fatReadWriteCluster.inc"
    INCLUDE "fatGetFreeSpace.inc"
    INCLUDE "fatReadBootSector.inc"
	INCLUDE "fileCreate.inc"
	INCLUDE "fileFind.inc"
	INCLUDE "fileLoad.inc"
	INCLUDE "fileDelete.inc"
	INCLUDE "fileRename.inc"
	INCLUDE "fileGetSetAttr.inc"
	INCLUDE "fileGetSetAddr.inc"
	INCLUDE "fileGetInfoAddr.inc"
	INCLUDE "fileList.inc"
	INCLUDE "fileNamePrepare.inc"
    INCLUDE "copyDescriptor.inc"
	INCLUDE "printDecWord.inc"
    INCLUDE "math.inc"

;---------------------------------------------------------------------------
; Константы и Переменные
;---------------------------------------------------------------------------

v_drive:            DB 1            ; Текущий накопитель
v_findCluster:      DW 0            ; Используется в fileFindClusterFirst/fileFindClusterNext
v_fileFirstCluster: DW 0            ; Первый кластер созданного файла
v_input_start:      DW 0            ; Используется в input, createFile
v_input_end:        DW 0            ; Используется в input, createFile
v_cachedDescrPtr:   DW 0            ; Используется в findFile, createFile
v_cachedSector:     DW 0            ; Используется в findFile, createFile
v_newDescrPtr:      DW 0            ; Адрес дескриптора созданного файла
v_foundDescrPtr:    DW 0            ; Адрес дескриптора найденного файла
v_batPtr:           DW 0            ; Адрес буфра, где находится содержимое BAT файла
v_memTop:           DW bios_vars-1  ; Максимальный доступный программам адрес в памяти (в оригинале почему-то 0FAFFh)
v_dirFirstFile      DW 0            ; Используется в fileList
v_dirMaxFiles       DW 0            ; Используется в fileList
v_dirListedFiles    DW 0            ; Используется в fileList
v_dirTotalFiles     DW 0            ; Используется в fileList

; Фейковое "системное время". Пока что просто счетчик, увеличивающийся при обращении.
; В будущем может быть заменен на реальное время с RTC или таймера.
v_fakeSystemTime:   DW 0

; Адреса драйверов для 8 накопителей. Начальные значения:
; = diskDriver для накопителей A: и B: (встроенный драйвер ROM и RAM диска),
; = diskDriverDummy для отальных (пустой драйвер).
v_drives:           DW diskDriver,      diskDriver      ; A:, B:
                    DW diskDriverDummy, diskDriverDummy ; C:, D:
                    DW diskDriverDummy, diskDriverDummy ; E:, F:
                    DW diskDriverDummy, diskDriverDummy ; G:, H:

; Информация о текущем накопителе
v_diskInfo          DISK_INFO

pathFontFnt:        DB "A:FONT.FNT",0
pathNcCom:          DB "A:NC.COM",0
pathAutoexecBat:    DB "A:AUTOEXEC.BAT",0
pathFormatBat:      DB "A:FORMAT.BAT",0
aBat:               DB "BAT"
aCom:               DB "COM"
aExe:               DB "EXE"
aFat:               DB "FAT"

    IF BOOT_FROM_TAPE
aATape_com:         DB "A:TAPE.COM",0
txtLoadingFromTape: DB 0Ah,"LOADING FROM TAPE...",0
    ENDIF

txtBiosVer:         DB 0Ch,"BIOS 5.00",0Ah,0
txtRAM:             DB 0Ah,"RAM: ",0
txtKB:              DB " KB",0Ah,0
txtBadCommand:	    DB 0Ah,"BAD COMMAND OR FILE NAME",0

v_tmpFileDescr 		FILE_DESCRIPTOR ; Копия дескриптора текущего файла
v_curFileDescr 		FILE_DESCRIPTOR ; Копия дескриптора текущего файла
v_batFileDescr 		FILE_DESCRIPTOR ; Копия дескриптора текущего BAT файла

; Буфер для копирования изображения символов при использовании шрифта из ПЗУ
v_char:             BLOCK   13, 0FFh

    ; Проверка - DOS.SYS не должен вылезать за эти пределы
    IF NEW_MEMORY_MAP==0
        ASSERT_DONT_FIT 0D000h  ; с этого адреса начинается NC.COM
    ENDIF

    IF LOAD_FONT
	    INCLUDE "initFont.inc"
    ENDIF

    END

