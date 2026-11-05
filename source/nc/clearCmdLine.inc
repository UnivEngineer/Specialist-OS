; Очистка общей командной строки и перерисовка приглашения с путём.
clearCmdLine:
            ld hl,vars.cmdLine
            ld (vars.cmdLineEnd),hl
            ld (hl),0
            jp redrawCommandLine
