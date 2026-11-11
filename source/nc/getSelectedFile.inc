; HL=FILE_INFO выбранного элемента через индекс активной панели.
; BC/DE сохраняются; на пустой панели возвращается FF-терминатор кэша.
getSelectedFile:
        push de
        ld hl,state.panelA_curFile
        call readBytePanel
        ld e,a
        ld d,0
        call fileIndexBase
        add hl,de
        call indexedFile
        pop de
        ret
