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
; Проеобразование дескриптора файла в компактную форму
; FILE_DESCRIPTOR -> FILE_INFO
;
; вход:
;  de = адрес откуда
;  hl = адрес куда
;
; выход:
;  hl = адрес следующего описателя
;
;  de - сохраняется
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
            ld      b, FILE_NAME_LENGTH + 3
cfdc_loop:  ld      a, (de)
            ld      (hl), a
            inc     hl
            inc     de
            dec     b
            jp nz,  cfdc_loop

            ; В компактной записи адреса загрузки больше нет.
            xor a
            ld (hl),a
            inc hl
            ld (hl),a
            inc hl
            ex hl,de
            ld bc,FILE_DESCRIPTOR.size-FILE_DESCRIPTOR.attrib-1
            add hl,bc
            ex hl,de
            ld      a, (de)
            ld      (hl), a
            inc     hl
            inc     de
            ld      a, (de)
            ld      (hl), a
            inc     hl

            pop     de
            ret
