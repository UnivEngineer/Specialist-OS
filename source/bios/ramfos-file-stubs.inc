; Совместимые ответы при отсутствии файловой службы RAMFOS.
; Диалог завершает работу как при отмене. Операции над файлом возвращают отказ.
ramfosCancel:
    xor a
    ret
ramfosReadFailure:
    ld c,0FFh
ramfosNoFile:
    ld a,1
    or a
    scf
    ret
ramfosFileType:
ramfosDeviceNoop:
    ret
biosCompatExit:
    ld hl,APP_MEMORY_END-1
    ld (ramfos_memTop),hl
    jp reboot3
