; Прямые входы резидента: C=физическая страница, DE=сектор<<8, HL=main.
; Ровно 256 байт; AF/BC/DE/HL разрушены, SP и main восстановлены.
; Код резидентен; проверка диапазонов выполняется Driver.Validate.
cpmInitRead:
    ld (0FFD2h),hl
    ex de,hl
    ld (0FFD0h),hl
    ld a,c
    ld (0FFD6h),a
    ld a,0FFh
    ld (0FFD7h),a
    jp cpmInitCommon
cpmInitWrite:
    ld (0FFD0h),hl
    ex de,hl
    ld (0FFD2h),hl
    ld a,0FFh
    ld (0FFD6h),a
    ld a,c
    ld (0FFD7h),a
cpmInitCommon:
    ld hl,0
    add hl,sp
    ld (0FFD9h),hl
    ld a,16
    ld (0FFD4h),a
    jp cpmCopySector
