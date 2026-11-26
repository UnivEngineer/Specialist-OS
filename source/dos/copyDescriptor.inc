;----------------------------------------------------------------------------
; MXOS
; Копирование дескриптора файла (FILE_DESCRIPTOR)
;
; вход:
;  de = адрес откуда
;  hl = адрес куда
;
; выход:
;  de = адрес следующего дескриптора
;  hl = адрес следующего дескриптора
;
; 2022-02-11 SpaceEngineer
;----------------------------------------------------------------------------

copyDescriptor:
            ; Копируем FILE_DESCR_SIZE байт из de в hl
            ld      b, FILE_DESCR_SIZE
cfd_loop:   ld      a, (de)
            ld      (hl), a
            inc     hl
            inc     de
            dec     b
            jp nz,  cfd_loop
            ret

;----------------------------------------------------------------------------
; MXOS
; Скопировать имя, атрибуты и полный размер из FILE_DESCRIPTOR в FILE_INFO.
;
; вход:
;   DE = адрес FILE_DESCRIPTOR, HL = адрес буфера на одну запись FILE_INFO.
;
; выход:
;   Запись FILE_INFO заполнена. HL = адрес следующей записи, DE сохранён.
;   AF, BC и флаги разрушены.
;
; 2022-02-16 SpaceEngineer
;----------------------------------------------------------------------------

copyDescriptorCompact:
            push    de

            ; Копируем имя файла + расширение + байт атрибутов
            ld      a,(de)
            cp      5
            jp nz,  cfdc_first
            ld      a,0E5h
cfdc_first:
            ld      (hl),a
            inc     hl
            inc     de
            DUP FILE_NAME_LENGTH + 3
            ld      a,(de)
            ld      (hl),a
            inc     hl
            inc     de
            EDUP

            ex hl,de
            ld bc,FILE_DESCRIPTOR.size-FILE_DESCRIPTOR.attrib-1
            add hl,bc
            ex hl,de
            ld b,4
cfdc_size:
            ld      a, (de)
            ld      (hl), a
            inc     hl
            inc     de
            dec     b
            jp nz,  cfdc_size

            pop     de
            ret
