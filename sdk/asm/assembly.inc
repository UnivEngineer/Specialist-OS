; MXOS
; Макросы размещения и проверки адресов SjASMPlus.

    IFNDEF MXOS_ASSEMBLY_INC
    DEFINE MXOS_ASSEMBLY_INC

; Макрос для заполнения памяти от текущего адреса до указанного
    MACRO ORG_PAD adr
         IF $ > adr
           ; вывод сообщения об ошибке
           ASSERT 0
           DISPLAY /l, "Error! ORG_PAD failed! ", $, " is more than ", adr
         ELSE
           ; заполнение памяти
           BLOCK adr-$, 0FFh
         ENDIF
         ORG adr
    ENDM

    MACRO ORG_PAD0 adr
         IF $ > adr
           ; вывод сообщения об ошибке
           ASSERT 0
           DISPLAY /l, "Error! ORG_PAD0 failed! ", $, " is more than ", adr
         ELSE
           ; заполнение памяти
           BLOCK adr-$, 0
         ENDIF
         ORG adr
    ENDM

; Макросы для проверки текущего адреса
    MACRO ASSERT_EQUAL adr
         IF $ != adr
           ASSERT 0
           DISPLAY /l, "Error! Entry point has been shifted (", $, " != ", adr, ")"
         ENDIF
    ENDM

    MACRO ASSERT_DONT_FIT adr
         IF $ > adr
           ASSERT 0
           DISPLAY /l, "Error! Image did not fit (", $, " > ", adr, ")"
         ENDIF
    ENDM

    ENDIF
