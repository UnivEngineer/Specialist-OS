;----------------------------------------------------------------------------
; MXOS NC.COM
; ������ �������� � ���������� ������
; [hl + state.activePanel] = a
;
; ����:
;   hl = ����� ���������� ������ A (state.panelA_info, state.panelA_drive � �.�.)
;   a  = �������� ����������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

writeBytePanel:
            push  bc
            push  hl
            push  af
            ld    c, a
            ld    a, (state.activePanel)
            and   a
            jp z, loc_D97E
            inc   hl
loc_D97E:   ld    (hl), c
            pop   af
            pop   hl
            pop   bc
            ret

;----------------------------------------------------------------------------
; MXOS NC.COM
; ������ �������� �� ���������� ������
; a = [hl + state.activePanel]
;
; ����:
;   hl = ����� ���������� ������ A (state.panelA_info, state.panelA_drive � �.�.)
;
; �����:
;   a = �������� ����������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

readBytePanel:
            push  hl
            ld    a, (state.activePanel)
            and   a
            jp z, loc_D98C
            inc   hl
loc_D98C:   ld    a, (hl)
            pop   hl
            ret

