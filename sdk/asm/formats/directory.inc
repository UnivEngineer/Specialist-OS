; MXOS
; Дескриптор каталога FAT и файлового API.

    IFNDEF MXOS_FORMAT_DIRECTORY_INC
    DEFINE MXOS_FORMAT_DIRECTORY_INC

FILE_DESCR_SIZE  = 32  ; размер дескриптора файла в каталоге (байт)
FILE_NAME_LENGTH = 8   ; длина имени файла  (байт)

    STRUCT  FILE_DESCRIPTOR                 ; 32 байта
name            BLOCK   FILE_NAME_LENGTH    ; смещение 00h, 8 байт  - имя файла
ext             BLOCK   3                   ; смещение 08h, 3 байта - расширение имени файла
attrib          BLOCK   1                   ; смещение 0Bh, 1 байт  - атрибуты файла
ntReserved      BLOCK   1                   ; смещение 0Ch, 1 байт  - используются в Windows NT
createTimeTenth BLOCK   1                   ; смещение 0Dh, 1 байт  - время создания файла, десятки миллисекунд (0-199)
createTime      BLOCK   2                   ; смещение 0Eh, 2 байта - время создания файла, секунды*2 (0-43200)
createDate      BLOCK   2                   ; смещение 10h, 2 байта - дата  создания файла
accessDate      BLOCK   2                   ; смещение 12h: дата последнего доступа FAT16
firstClusterHi  BLOCK   2                   ; смещение 14h, 2 байта - номер первого кластера в FAT, старшее слово
writeTime       BLOCK   2                   ; смещение 16h, 2 байта - время модификации файла, секунды*2 (0-43200)
writeDate       BLOCK   2                   ; смещение 18h, 2 байта - дата  модификации файла
firstCluster    BLOCK   2                   ; смещение 1Ah, 2 байта - номер первого кластера в FAT, младшее слово
size            BLOCK   4                   ; смещение 1Ch, 4 байта - размер файла в байтах
    ENDS

    ASSERT FILE_DESCRIPTOR == FILE_DESCR_SIZE

    ENDIF
