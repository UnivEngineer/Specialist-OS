;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша ENTER (продолжение)
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

Enter_nextExt:
      pop   hl
Enter_2:
      ld    a, (de)
      inc   de
      cp    0Dh
      jp z, Enter_extLoop
      inc   a
      jp    nz, Enter_2
      jp    Enter_stdExec

