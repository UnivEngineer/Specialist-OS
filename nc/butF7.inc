;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� F7. �������� ����� � ����� �� ����������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

; ���������� sp   � ��������� ���   ������.

prepareTapeLoad:
            ld          hl, 0
            add         hl, sp
            inc         hl
            inc         hl
            ld          (vars.savedSP), hl

            ld          hl, tapeErrorHandler
            ld          (bios_vars.tapeError), hl

            ld          a, 0Fh
            ld          (IO_KEYB_C),a
            ld          a, 0FEh
            ld          (IO_KEYB_A),a
            ret

; ---------------------------------------------------------------------------

but_F7:     ; ������ ����
            call        drawWindow1

            ; ������� "LOADING FROM TAPE TO A:"
            CENTER_LINE 23, 5Eh
            ld          hl, aLoadingFromTapeTo
            call        bios_printString       
            call        printSelDrive
            
            ; ���������� sp � ��������� ����������� ������
            call        prepareTapeLoad   

loc_D2AA:
            ; ���� ����� � D9 D9 D9
            ld          a, 0FFh           
            ld          c, 3
            jp          loc_D2B3
loc_D2B1:
            ld          a, 8
loc_D2B3:
            call        bios_tapeRead
            cp          0D9h 
            jp nz,      loc_D2AA
            dec c
            jp nz,      loc_D2B1

            ; ������ ���
            ld          hl, vars.tempFileDescr
            ld          d, h
            ld          e, l
            ld          c, FILE_NAME_LENGTH + 3 ; ������ ������ ����� 9 ��������, ����� - FILE_NAME_LENGTH + 3 ��������
loc_D2C6:
            ld          a, 8
            call        bios_tapeRead
            cp          ' '
            jp c,       tapeReadAppendName
            ld          (hl), a
            inc         hl
            dec         c
            jp nz,      loc_D2C6

loc_D2D6:
            ld         (hl), 0 ; �������� ���� �������� � ����������� �����

            call        printInvFile

            ; ���������� ����� � ������ ��������� ����� ��������
            ld          a, 0FFh
            call        tapeReadWord2
            ld          (vars.tapeLoadAddr), hl

            ; ������ �������� ����� ��������
            ex          de, hl
            call        tapeReadWord

            ; ������� ������ ����� (hl = hl - de)
            ld          a, l
            sub         e
            ld          l, a
            ld          a, h
            sbc         d
            ld          h, a
            ld          (vars.tempFileDescr.size), hl

            ; ������ ������ �� ������ 0000
            inc         hl
            ex          de, hl
            ld          hl, 0
            ld          (vars.tempFileDescr.loadAddress), hl           
loc_D2F8:   ld          a, 8
            call        bios_tapeRead
            ld          (hl), a
            inc         hl
            dec         de
            ld          a, d
            or          e
            jp nz,      loc_D2F8

            ; ����� CRC
            call        tapeReadWord
            ld          (vars.tapeSaveCRC), hl

            ; ������� CRC
            ld          hl, (vars.tempFileDescr.size)
            ex          de, hl
            ld          hl, 0
            add         hl, de
            ex          de, hl
            ld          hl, 0
            call        bios_calcCS
            ld          hl, (vars.tapeSaveCRC)

            ; ���� ��������� CRC �� ����� ���������, �������
            ld          a, b
            cp          h
            jp nz,      F7_crcError
            ld          a, c
            cp          l
            jp nz,      F7_crcError

            ; �������� ���� �� ������� ����
            call        F7_saveFile

            ; �������������� ����� � ���������� ��������
            call        loadAndPrintA
            call        loadAndPrintB
            jp          but_F7

; ---------------------------------------------------------------------------

F7_saveFile:
            ld          hl, (vars.tapeLoadAddr)
            push        hl
            ld          hl, vars.tempFileDescr
            jp          saveFileInt

; ---------------------------------------------------------------------------

F7_crcError:
            call        drawWindow2

            ; ����� "ERROR LOADING FROM TAPE"
            COLOR       COLOR_DIALOG_ERR
            CENTER_LINE 23, 80h
            ld          hl, aErrorLoadingTa
            call        bios_printString

            ; ����� ������ �� vars.input
            COLOR       COLOR_DIALOG
            ld          hl, 5090h
            ld          (bios_vars.cursorY), hl
            ld          hl, vars.input
            call        printStringInv

loc_D358:   ; ���� ������� ESC
            call        bios_keyScan
            cp          1Bh
            jp nz,      loc_D358

            jp          mainReload

;----------------------------------------------------------------------------
; ������ ����� � �����
;----------------------------------------------------------------------------

tapeReadWord:
            ld          a, 8
tapeReadWord2:
            call        bios_tapeRead
            ld          l, a
            ld          a, 8
            call        bios_tapeRead
            ld          h, a
            ret


; ---------------------------------------------------------------------------

; ���� ��� ����� �� ����� ���� ������, ��� FILE_NAME_LENGTH + 3, ��������� ��� ���������
tapeReadAppendName:
            ld          (hl), ' '
            inc         hl
            dec         c
            jp nz,      tapeReadAppendName
            jp          loc_D2D6

