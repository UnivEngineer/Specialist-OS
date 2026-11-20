; Captions and field layouts belong to the shell in both profiles.
; A=Shift state (0 pressed), HL=field list, BC=its serialized size.
ncToolbarGetText:
    ld e,0
    or a
    jp nz,ncToolbarTextMode
    ld e,4
ncToolbarTextMode:
    ld a,(ncFontWidth)
    cp 4
    jp z,ncToolbarTextIndex
    ld a,e
    add a,8
    ld e,a
ncToolbarTextIndex:
    ld d,0
    ld hl,ncToolbarFields
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    ex de,hl
    ret
ncToolbarFields:
    DW ncToolbarNormal4,ncToolbarNormal4End-ncToolbarNormal4
    DW ncToolbarShift4,ncToolbarShift4End-ncToolbarShift4
    DW ncToolbarNormal8,ncToolbarNormal8End-ncToolbarNormal8
    DW ncToolbarShift8,ncToolbarShift8End-ncToolbarShift8
    MACRO NC_TOOLBAR_PAIR KEY,TEXT
    UI_TOOLBAR_FIELD 2,0,KEY
    UI_TOOLBAR_FIELD 4,080h,TEXT
    ENDM
ncToolbarNormal4:
    NC_TOOLBAR_PAIR "F1","LeftDisk"
    NC_TOOLBAR_PAIR "F2","RightDsk"
    NC_TOOLBAR_PAIR "F3","DiskInfo"
    NC_TOOLBAR_PAIR "F4","EditFile"
    NC_TOOLBAR_PAIR "F5","CopyFile"
    NC_TOOLBAR_PAIR "F6","MoveFile"
    NC_TOOLBAR_PAIR "F7","MakeDir"
    NC_TOOLBAR_PAIR "F8","Delete"
    DB 0
ncToolbarNormal4End:

ncToolbarShift4:
    NC_TOOLBAR_PAIR "F1","SortLeft"
    NC_TOOLBAR_PAIR "F2","SortRght"
    NC_TOOLBAR_PAIR "F3","Help"
    NC_TOOLBAR_PAIR "F4","NewFile"
    NC_TOOLBAR_PAIR "F5","CopyFile"
    NC_TOOLBAR_PAIR "F6","Rename"
    NC_TOOLBAR_PAIR "F7","MakeDir"
    UI_TOOLBAR_FIELD 3,0,"F10"
    UI_TOOLBAR_FIELD 3,080h,"Exit"
    DB 0
ncToolbarShift4End:

ncToolbarNormal8:
    NC_TOOLBAR_PAIR "F1","Left"
    NC_TOOLBAR_PAIR "F2","Rght"
    NC_TOOLBAR_PAIR "F3","Info"
    NC_TOOLBAR_PAIR "F4","Edit"
    NC_TOOLBAR_PAIR "F5","Copy"
    NC_TOOLBAR_PAIR "F6","Move"
    NC_TOOLBAR_PAIR "F7","MkDr"
    NC_TOOLBAR_PAIR "F8","Del"
    DB 0
ncToolbarNormal8End:

ncToolbarShift8:
    NC_TOOLBAR_PAIR "F1","Sort"
    NC_TOOLBAR_PAIR "F2","Sort"
    NC_TOOLBAR_PAIR "F3","Help"
    NC_TOOLBAR_PAIR "F4","New"
    NC_TOOLBAR_PAIR "F5","Copy"
    NC_TOOLBAR_PAIR "F6","Ren"
    NC_TOOLBAR_PAIR "F7","MkDr"
    UI_TOOLBAR_FIELD 3,0,"F10"
    UI_TOOLBAR_FIELD 3,080h,"Ex"
    DB 0

ncToolbarShift8End:
