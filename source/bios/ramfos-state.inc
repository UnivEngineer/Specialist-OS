; Исторические слова RAMFOS, доступные приложениям по прямым адресам.
ramfos_cursorPos:   DW 8
    DS 12,0
ramfos_beepConfig:  DW 0A05h
ramfos_oldBeep:     DW 0
    ASSERT ramfos_cursorPos == 0C63Eh
    ASSERT ramfos_beepConfig == 0C64Ch
    ASSERT ramfos_oldBeep == 0C64Eh
