;+---------------------------------------------------------------------------
; MXOS
; ��������� ����
;
; �� �����
;  hl - ��� ������ � ������� [����:]����[ ���������]
;
; �� ������
;  �f - ������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

fileExec:   ; ���� ����� ���������� ���������
            ld      de, CMD_LINE
            
            ; ���� ������ ������ ������ �������, ������� c CF
            ld      a, (hl)
            cp      ' '
            jp      c, stc_ret

            ; ������������ ������ ��� ������ (+2 �����������, ����� 82h)
            ld      c, 80h        

fileExec_0: ; ���� ������ ������ �������, �������
            ld      a, (hl)
            cp      20h
            jp c,   fileExec_1

            ; �������� ������� �� hl � de
            ld      (de),a
            inc     hl
            inc     de

            ; ��������� 80h ���
            dec     c
            jp nz,  fileExec_0

fileExec_1: ; � ����� ����� �������� 13, 0
            ld      a, 0Dh
            ld      (de), a
            inc     de
            xor     a
            ld      (de), a

            ; ������� �� ����� 0Ah
            call    printCharA

            ; ������� �� ����� ��� �����
            ld      hl, CMD_LINE
            call    j_printString

            ; ���������� ����� �����
            ld      hl, CMD_LINE
            ld      de, v_curFileDescr
            call    fileNamePrepare

            ; ���������� � ����� ������� ����� ��������� ������ (������ ������ ����������)
            push    hl

            ; ���� ��� BAT ����
            ld      de, aBat ; "BAT"
            call    cmpFileExt
            jp z,   execBat

            ; ���� ��� COM ��� EXE ����
            call    cmpFileExt
            jp z,   execExeCom
            call    cmpFileExt
            jp nz,  execError

execExeCom:
            ; ����������� �� ����� ������� ����� ��������� ������ (������ ������ ����������)
            pop     de

            ; ��������� ����
            ld      hl, v_curFileDescr
            call    fileLoad
            jp c,   badCommand

            ; ����� ��������� ������
            push    de

            ; ������� �� ����� 0Ah
            ;call printCharA

            ; hl = ��������� �� ����� �������� (= ����� �������) �����
            ld      de, FILE_DESCRIPTOR.loadAddress   ; �������� ���� � ������� ��������
            add     hl, de

            ; de = ����� �������� (= ����� �������) �����
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

            ; ����� ��������
            ld      hl, defAppReturn
            ex      (sp), hl

            ; ������ ��������� (������� �� de)
            ex      de, hl  ; hl <--> de
            jp      (hl)    ; pc <-- hl

; ---------------------------------------------------------------------------

defAppReturn:
            or  a
            ret

; ---------------------------------------------------------------------------

execError:
            pop     de
badCommand:
            ld      hl, txtBadCommand; "\nBAD COMMAND or FILE NAME"
            call    j_printString
stc_ret:
            scf
            ret

; ---------------------------------------------------------------------------

printCharA:
            ld  c, 0Ah
            jp  j_printChar

