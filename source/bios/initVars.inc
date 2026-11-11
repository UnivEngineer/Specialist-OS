; Начальные значения BIOS, копируемые при холодном старте.
initVars        BIOS_VARIABLES
initVarsEnd:

; Только новые поля верхнего блока, без копии буферов строки/драйверов.
initTextVars:
    DW FONT_ADDR
    DB 021h,0,0FFh,020h,0E0h,RAMD_MAX_PAGE,0
initTextVarsEnd:
