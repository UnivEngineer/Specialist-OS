; Рабочая память и размещение программы
; Порядок включения задан главным .asm; см. README.md.

; Собственные переменные
INPUT_BUF     = 8F60h
INPUT_BUF_END = 8F6Dh

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
    ORG   0F100h

; Адреса блоков переменных
vars1       MON2_VARS_1     = 08F80H
vars2       MON2_VARS_2     = 0F0E0H
v_fileDescr FILE_DESCRIPTOR = 0F900H

; Буфер для листинга директории
; В новой карте памяти можно расположить его
; перед Монитором - там больше места
    IF  NEW_MEMORY_MAP
v_fileInfo  FILE_INFO = 0E800H
DIR_BUFFER_SIZE = ($ - v_fileInfo) / FILE_INFO_SIZE - 1
    ELSE
v_fileInfo  FILE_INFO = v_fileDescr + FILE_DESCRIPTOR_SIZE
DIR_BUFFER_SIZE = (0FB00h - v_fileInfo) / FILE_INFO_SIZE - 1
    ENDIF

;----------------------------------------------------------------------------
; Код

