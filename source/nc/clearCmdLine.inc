; Очистка общей командной строки и перерисовка приглашения с путём.
clearCmdLine:
            call resetCmdLine
            jp redrawCommandLine

resetCmdLine:
            ld hl,vars.cmdLine
            ld (vars.cmdLineEnd),hl
            ld (hl),0
            ret
