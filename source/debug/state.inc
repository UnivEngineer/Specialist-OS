; Рабочая память и размещение программы
; Порядок включения задан главным .asm; см. README.md.

; Собственные переменные
INPUT_BUF     = APP_MEMORY_END - 0180h
INPUT_BUF_END = INPUT_BUF + MAX_PATH - 1
    ASSERT INPUT_BUF + MAX_PATH <= APP_MEMORY_END

    STRUCT MON2_VARS_1
Directive:      DS    1
FirstParam:     DS    2
SecondParam:    DS    2
ThirdParam:     DS    2
ParseTail:         DS    2
SearchLength:         DS    1
SearchByte1:         DS    1
SearchByte2:         DS    1
SearchBytes34:         DS    2
SearchCursor:         DS    2
SearchRemainingCopy:         DS    1 ; записывается, далее не используется
Reserved91:         DS    4
InputCharacter:         DS    1
Reserved96:         DS    1
BreakpointAddress:         DS    2
BreakpointOriginalByte:         DS    1
SavedRst7Vector:         DS    3
SavedHL:         DS    2 ; снимок регистра; X не читает байт по этому адресу
SavedDE:         DS    2
SavedBC:         DS    2
SavedAF:         DS    2
SavedSP:         DS    2
ReservedA7:         DS    4
LegacyMemoryTop:         DS    2 ; 07EFFh; в текущем коде не читается
    ENDS

; Остаток старого обмена через байты ОЗУ F0E5h-F0E7h, не аппаратные порты.
    STRUCT MON2_VARS_2
LegacyIoReserved:         DS    5
LegacyIoData:         DS    1
LegacyIoStatus:         DS    1
LegacyIoControl:         DS    1
    ENDS

;----------------------------------------------------------------------------
; Начало программы
    ORG   00000h

; Адреса блоков переменных
vars1       MON2_VARS_1     = APP_MEMORY_END - 080h
vars2       MON2_VARS_2     = APP_MEMORY_END - 0B0h
v_fileDescr FILE_DESCRIPTOR = APP_MEMORY_END - 0100h
    ASSERT INPUT_BUF + MAX_PATH <= v_fileDescr

; Буфер списка каталога под верхними рабочими буферами DEBUG.
v_fileInfo  FILE_INFO = APP_MEMORY_END - 01000h
DIR_BUFFER_SIZE = (INPUT_BUF - v_fileInfo) / FILE_INFO_SIZE - 1

;----------------------------------------------------------------------------
; Код

