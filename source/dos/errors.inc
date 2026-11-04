; Единый статус DOS: CF остаётся быстрым признаком отказа.
; Внутренний setter: A=код, BC/DE/HL сохраняются, CF=1.
dosSetError:
            ld (v_dosError),a
            scf
            ret
; C8C9h: A=последняя причина, CF=1 при ненулевом коде.
; BC/DE/HL сохраняются. Чтение не сбрасывает причину.
dosGetError:
            ld a,(v_dosError)
            or a
            ret z
            scf
            ret
dosIoError:
            ld a,(v_dosError)
            or a
            scf
            ret nz
            ld a,DOS_ERR_IO
            jp dosSetError
dosCorrupt:
            ld a,DOS_ERR_CORRUPT
            jp dosSetError
v_dosError: DB 0

    IFDEF DOS_SPLIT_BUILD
; Явное успешное завершение публичной операции вне диспетчера томов.
dosClearError:
            xor a
            ld (v_dosError),a
            ret
    ENDIF

; Зарезервированный вход ABI: числовой отказ без вывода текста.
dosUnsupported:
            ld a,DOS_ERR_UNSUPPORTED
            jp dosSetError
