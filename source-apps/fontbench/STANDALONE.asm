    IFDEF FONT_PAGE6
    INCLUDE "legacy-main.inc"
    ELSE
    IFDEF FONT_LEGACY
    INCLUDE "legacy-main.inc"
    ELSE
    INCLUDE "grid-main.inc"
    ENDIF
    ENDIF
