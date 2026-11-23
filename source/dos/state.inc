; Строки резидентного DOS.
    IFNDEF BANKED_DOS_BUILD
pathNcCom:          DB "C:",05Ch,"DOS",05Ch,"NC_MN.COM",0
pathAutoexecBat:    DB "C:",05Ch,"AUTOEXEC.BAT",0
pathFormatBat:      DB "C:",05Ch,"DOS",05Ch,"FORMAT.BAT",0
    ENDIF

txtBadCommand:      DB "BAD COMMAND OF FILE NAME",0
