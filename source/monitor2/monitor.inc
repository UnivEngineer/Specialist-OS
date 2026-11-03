; Восстановлено из originals/Monitor2.mon; адреса и байты сохранены.


; Стандартная таблица переходов Monitor-2; контракт отличается от MXOS.
monitor2_vectors:
    jp monitor2_restart                        ; C800h
    jp loader_getch                            ; C803h
    jp loader_tapeRead                         ; C806h
    jp LCEC8                                   ; C809h
    jp loader_tapeWrite                        ; C80Ch
    jp LCB1A                                   ; C80Fh
    jp LCE5C                                   ; C812h
    jp LCE9D                                   ; C815h
    jp LCE1D                                   ; C818h
    jp LCE5C                                   ; C81Bh
    jp LCA7C                                   ; C81Eh
    ret                                        ; C821h
    nop                                        ; C822h
    nop                                        ; C823h
    jp LC990                                   ; C824h
    jp LC99B                                   ; C827h
    jp LCC89                                   ; C82Ah
    ret                                        ; C82Dh
    nop                                        ; C82Eh
    nop                                        ; C82Fh
    jp LC8CE                                   ; C830h
    jp LC8D2                                   ; C833h

; Перезапуск: SP=7FFFh; системный вектор ошибки и верхняя граница памяти.
monitor2_restart:
    ld sp,07FFFh                               ; C836h
    ld hl,monitor2_vectors                     ; C839h
    ld (08FE1h),hl                             ; C83Ch
    ld hl,07EFFh                               ; C83Fh
    ld (08FABh),hl                             ; C842h
    call monitor2_command                      ; C845h
    jp monitor2_restart                        ; C848h
monitor2_command:
    ld hl,LCF52                                ; C84Bh
    call LCE1D                                 ; C84Eh
    call LCD49                                 ; C851h
    call LCF43                                 ; C854h
    call LCCE1                                 ; C857h
    cp 044h                                    ; C85Ah
    jp z,LCE2F                                 ; C85Ch
    cp 04Dh                                    ; C85Fh
    jp z,LCDA5                                 ; C861h
    cp 04Ch                                    ; C864h
    jp z,LCCAD                                 ; C866h
    cp 04Bh                                    ; C869h
    jp z,LCC7A                                 ; C86Bh
    cp 054h                                    ; C86Eh
    jp z,LCC3B                                 ; C870h
    cp 058h                                    ; C873h
    jp z,LC8D6                                 ; C875h
    cp 057h                                    ; C878h
    jp z,LC99B                                 ; C87Ah
    cp 052h                                    ; C87Dh
    jp z,LC990                                 ; C87Fh
    cp 043h                                    ; C882h
    jp z,LCC46                                 ; C884h
    cp 048h                                    ; C887h
    jp z,LC8B9                                 ; C889h
    cp 04Eh                                    ; C88Ch
    jp z,LCC16                                 ; C88Eh
    cp 049h                                    ; C891h
    jp z,LC9B2                                 ; C893h
    cp 047h                                    ; C896h
    jp z,LC924                                 ; C898h
    cp 046h                                    ; C89Bh
    jp z,LCC32                                 ; C89Dh
    cp 053h                                    ; C8A0h
    jp z,LCB61                                 ; C8A2h
    cp 04Fh                                    ; C8A5h
    jp z,LCA80                                 ; C8A7h
    jp LCE14                                   ; C8AAh
LC8AD:
    ld a,(hl)                                  ; C8ADh
    ld c,a                                     ; C8AEh
    cp 000h                                    ; C8AFh
    ret z                                      ; C8B1h
    call LCEE5                                 ; C8B2h
    inc hl                                     ; C8B5h
    jp LC8AD                                   ; C8B6h
LC8B9:
    push hl                                    ; C8B9h
    add hl,de                                  ; C8BAh
    call LCC6C                                 ; C8BBh
    call LCE29                                 ; C8BEh
    ld a,e                                     ; C8C1h
    cpl                                        ; C8C2h
    ld e,a                                     ; C8C3h
    ld a,d                                     ; C8C4h
    cpl                                        ; C8C5h
    ld d,a                                     ; C8C6h
    inc de                                     ; C8C7h
    pop hl                                     ; C8C8h
    add hl,de                                  ; C8C9h
    call LCC6C                                 ; C8CAh
    ret                                        ; C8CDh
LC8CE:
    ld (08FABh),hl                             ; C8CEh
    ret                                        ; C8D1h
LC8D2:
    ld hl,(08FABh)                             ; C8D2h
    ret                                        ; C8D5h
LC8D6:
    ld hl,08FA4h                               ; C8D6h
    ld de,monitor2_registerNames               ; C8D9h
    ld c,004h                                  ; C8DCh
LC8DE:
    push bc                                    ; C8DEh
    call LC903                                 ; C8DFh
    pop bc                                     ; C8E2h
    dec c                                      ; C8E3h
    jp nz,LC8DE                                ; C8E4h
    call LC918                                 ; C8E7h
    ld hl,(08F9Dh)                             ; C8EAh
    call LCC6C                                 ; C8EDh
    call LC918                                 ; C8F0h
    ld hl,(08F97h)                             ; C8F3h
    call LCC6C                                 ; C8F6h
    call LC918                                 ; C8F9h
    ld hl,(08FA5h)                             ; C8FCh
    call LCC6C                                 ; C8FFh
    ret                                        ; C902h
LC903:
    ld b,(hl)                                  ; C903h
    dec hl                                     ; C904h
    ld c,(hl)                                  ; C905h
    dec hl                                     ; C906h
    push bc                                    ; C907h
    call LC918                                 ; C908h
    ld a,b                                     ; C90Bh
    call LCE9D                                 ; C90Ch
    call LC918                                 ; C90Fh
    pop bc                                     ; C912h
    ld a,c                                     ; C913h
    call LCE9D                                 ; C914h
    ret                                        ; C917h
LC918:
    ex de,hl                                   ; C918h
    push bc                                    ; C919h
    call LCE29                                 ; C91Ah
    call LCE1D                                 ; C91Dh
    inc hl                                     ; C920h
    pop bc                                     ; C921h
    ex de,hl                                   ; C922h
    ret                                        ; C923h
LC924:
    ld a,e                                     ; C924h
    or a                                       ; C925h
    jp nz,LC92E                                ; C926h
    ld a,d                                     ; C929h
    or a                                       ; C92Ah
    jp z,LC951                                 ; C92Bh
LC92E:
    push hl                                    ; C92Eh
    ex de,hl                                   ; C92Fh
    ld (08F97h),hl                             ; C930h
    ld a,(hl)                                  ; C933h
    ld (hl),0FFh                               ; C934h
    ld (08F99h),a                              ; C936h
    ld hl,00038h                               ; C939h
    ld de,08F9Ah                               ; C93Ch
    ld bc,LC95D                                ; C93Fh
    call LC95A                                 ; C942h
    ld (hl),0C3h                               ; C945h
    call LC958                                 ; C947h
    call LC958                                 ; C94Ah
    ld (hl),b                                  ; C94Dh
    dec hl                                     ; C94Eh
    ld (hl),c                                  ; C94Fh
    pop hl                                     ; C950h
LC951:
    call LC957                                 ; C951h
    jp monitor2_restart                        ; C954h
LC957:
    jp (hl)                                    ; C957h
LC958:
    inc hl                                     ; C958h
    inc de                                     ; C959h
LC95A:
    ld a,(hl)                                  ; C95Ah
    ld (de),a                                  ; C95Bh
    ret                                        ; C95Ch
LC95D:
    ld (08F9Dh),hl                             ; C95Dh
    ex de,hl                                   ; C960h
    ld (08F9Fh),hl                             ; C961h
    push bc                                    ; C964h
    pop hl                                     ; C965h
    ld (08FA1h),hl                             ; C966h
    push af                                    ; C969h
    pop hl                                     ; C96Ah
    ld (08FA3h),hl                             ; C96Bh
    ld hl,00000h                               ; C96Eh
    add hl,sp                                  ; C971h
    inc hl                                     ; C972h
    inc hl                                     ; C973h
    ld (08FA5h),hl                             ; C974h
    ld hl,(08F97h)                             ; C977h
    ld a,(08F99h)                              ; C97Ah
    ld (hl),a                                  ; C97Dh
    ld hl,08F9Ah                               ; C97Eh
    ld de,00038h                               ; C981h
    call LC95A                                 ; C984h
    call LC958                                 ; C987h
    call LC958                                 ; C98Ah
    jp monitor2_restart                        ; C98Dh
LC990:
    ld a,l                                     ; C990h
    or a                                       ; C991h
    jp z,LCA1A                                 ; C992h
    ld (08FFFh),a                              ; C995h
    jp LCA1A                                   ; C998h
LC99B:
    push hl                                    ; C99Bh
    push de                                    ; C99Ch
    push bc                                    ; C99Dh
    call LCC89                                 ; C99Eh
    ld h,b                                     ; C9A1h
    ld l,c                                     ; C9A2h
    ld (08F93h),hl                             ; C9A3h
    pop bc                                     ; C9A6h
    ld a,c                                     ; C9A7h
    or a                                       ; C9A8h
    jp z,LCAD8                                 ; C9A9h
    ld (08FFEh),a                              ; C9ACh
    jp LCAD8                                   ; C9AFh
LC9B2:
    ld a,l                                     ; C9B2h
    or a                                       ; C9B3h
    jp z,LC9BA                                 ; C9B4h
    ld (08FFFh),a                              ; C9B7h
LC9BA:
    ld hl,LCF81                                ; C9BAh
    call LCE1D                                 ; C9BDh
    ld hl,08FC1h                               ; C9C0h
    ld de,08FD0h                               ; C9C3h
    call LCB1A                                 ; C9C6h
    ld (hl),000h                               ; C9C9h
LC9CB:
    xor a                                      ; C9CBh
    ld (08FF3h),a                              ; C9CCh
    ld a,0FFh                                  ; C9CFh
    call loader_tapeRead                       ; C9D1h
    cp 0D9h                                    ; C9D4h
    jp nz,LC9CB                                ; C9D6h
    ld c,002h                                  ; C9D9h
LC9DB:
    ld a,008h                                  ; C9DBh
    call loader_tapeRead                       ; C9DDh
    cp 0D9h                                    ; C9E0h
    jp nz,LC9CB                                ; C9E2h
    dec c                                      ; C9E5h
    jp nz,LC9DB                                ; C9E6h
    ld hl,08FB0h                               ; C9E9h
    ld de,08FC0h                               ; C9ECh
    call LC414                                 ; C9EFh
    ld hl,LCF8E                                ; C9F2h
    call LCE1D                                 ; C9F5h
    ld hl,08FB0h                               ; C9F8h
    call LCE1D                                 ; C9FBh
    ld a,(08FC1h)                              ; C9FEh
    or a                                       ; CA01h
    jp z,LCA1A                                 ; CA02h
    ld hl,08FB0h                               ; CA05h
    ld de,08FC1h                               ; CA08h
LCA0B:
    ld c,(hl)                                  ; CA0Bh
    ld a,(de)                                  ; CA0Ch
    cp c                                       ; CA0Dh
    jp nz,LC9CB                                ; CA0Eh
    or a                                       ; CA11h
    jp z,LCA1A                                 ; CA12h
    inc hl                                     ; CA15h
    inc de                                     ; CA16h
    jp LCA0B                                   ; CA17h
LCA1A:
    call LC3F9                                 ; CA1Ah
    ld a,008h                                  ; CA1Dh
    call loader_tapeRead                       ; CA1Fh
    ld l,a                                     ; CA22h
    ld a,008h                                  ; CA23h
    call loader_tapeRead                       ; CA25h
    ld h,a                                     ; CA28h
    ld (08F8Ah),hl                             ; CA29h
    ex de,hl                                   ; CA2Ch
    ld (08F8Ch),hl                             ; CA2Dh
    ex de,hl                                   ; CA30h
    ld hl,(08FE3h)                             ; CA31h
    call LCC89                                 ; CA34h
    ld hl,(08F8Ah)                             ; CA37h
    ld e,c                                     ; CA3Ah
    ld d,b                                     ; CA3Bh
    call loader_compareHLDE                    ; CA3Ch
    jp nz,LCA6C                                ; CA3Fh
    call LCF43                                 ; CA42h
    ld hl,LCF60                                ; CA45h
    call LCE1D                                 ; CA48h
    ld hl,(08F8Ah)                             ; CA4Bh
    call LCC6C                                 ; CA4Eh
    ld hl,LCF96                                ; CA51h
    call LCE1D                                 ; CA54h
    ld hl,(08FE3h)                             ; CA57h
    call LCC6C                                 ; CA5Ah
    ld hl,LCFA9                                ; CA5Dh
    call LCE1D                                 ; CA60h
    ld hl,(08F8Ch)                             ; CA63h
    call LCC6C                                 ; CA66h
    jp LCA72                                   ; CA69h
LCA6C:
    ld hl,LCFBC                                ; CA6Ch
    call LCE1D                                 ; CA6Fh
LCA72:
    push hl                                    ; CA72h
    ld hl,0FF03h                               ; CA73h
    ld (hl),00Dh                               ; CA76h
    ld (hl),00Ch                               ; CA78h
    pop hl                                     ; CA7Ah
    ret                                        ; CA7Bh
LCA7C:
    ld hl,(08FFCh)                             ; CA7Ch
    ret                                        ; CA7Fh
LCA80:
    push hl                                    ; CA80h
    push de                                    ; CA81h
    push bc                                    ; CA82h
    call LCC89                                 ; CA83h
    ld h,b                                     ; CA86h
    ld l,c                                     ; CA87h
    ld (08F93h),hl                             ; CA88h
    ld hl,LCF81                                ; CA8Bh
    call LCE1D                                 ; CA8Eh
    ld hl,08FB0h                               ; CA91h
    ld de,08FC0h                               ; CA94h
    call LCB1A                                 ; CA97h
    ld hl,08FB0h                               ; CA9Ah
LCA9D:
    ld a,(hl)                                  ; CA9Dh
    cp 00Dh                                    ; CA9Eh
    inc hl                                     ; CAA0h
    jp nz,LCA9D                                ; CAA1h
    dec hl                                     ; CAA4h
    ld (hl),000h                               ; CAA5h
    pop bc                                     ; CAA7h
    ld a,c                                     ; CAA8h
    or a                                       ; CAA9h
    jp z,LCAB0                                 ; CAAAh
    ld (08FFEh),a                              ; CAADh
LCAB0:
    call LCB0E                                 ; CAB0h
    ld a,0E6h                                  ; CAB3h
    call loader_tapeWrite                      ; CAB5h
    ld c,003h                                  ; CAB8h
LCABA:
    ld a,0D9h                                  ; CABAh
    call loader_tapeWrite                      ; CABCh
    dec c                                      ; CABFh
    jp nz,LCABA                                ; CAC0h
    ld hl,08FB0h                               ; CAC3h
LCAC6:
    ld a,(hl)                                  ; CAC6h
    or a                                       ; CAC7h
    jp z,LCAD2                                 ; CAC8h
    call loader_tapeWrite                      ; CACBh
    inc hl                                     ; CACEh
    jp LCAC6                                   ; CACFh
LCAD2:
    call LCB0E                                 ; CAD2h
    call LCB0E                                 ; CAD5h
LCAD8:
    call LCB0E                                 ; CAD8h
    ld a,0E6h                                  ; CADBh
    call loader_tapeWrite                      ; CADDh
    pop de                                     ; CAE0h
    pop hl                                     ; CAE1h
    ld a,l                                     ; CAE2h
    call loader_tapeWrite                      ; CAE3h
    ld a,h                                     ; CAE6h
    call loader_tapeWrite                      ; CAE7h
    ld a,e                                     ; CAEAh
    call loader_tapeWrite                      ; CAEBh
    ld a,d                                     ; CAEEh
    call loader_tapeWrite                      ; CAEFh
LCAF2:
    ld a,(hl)                                  ; CAF2h
    call loader_tapeWrite                      ; CAF3h
    call loader_compareHLDE                    ; CAF6h
    jp z,LCB00                                 ; CAF9h
    inc hl                                     ; CAFCh
    jp LCAF2                                   ; CAFDh
LCB00:
    ld hl,(08F93h)                             ; CB00h
    ld a,l                                     ; CB03h
    call loader_tapeWrite                      ; CB04h
    ld a,h                                     ; CB07h
    call loader_tapeWrite                      ; CB08h
    jp LCA72                                   ; CB0Bh
LCB0E:
    ld c,0FFh                                  ; CB0Eh
LCB10:
    ld a,000h                                  ; CB10h
    call loader_tapeWrite                      ; CB12h
    dec c                                      ; CB15h
    jp nz,LCB10                                ; CB16h
    ret                                        ; CB19h
LCB1A:
    ld b,h                                     ; CB1Ah
    ld c,l                                     ; CB1Bh
LCB1C:
    call loader_getch                          ; CB1Ch
    cp 008h                                    ; CB1Fh
    jp z,LCB40                                 ; CB21h
    cp 00Dh                                    ; CB24h
    jp z,LCB5F                                 ; CB26h
    ld (08F95h),a                              ; CB29h
    call loader_compareHLDE                    ; CB2Ch
    jp z,LCB1C                                 ; CB2Fh
    ld a,(08F95h)                              ; CB32h
    ld (hl),a                                  ; CB35h
    push bc                                    ; CB36h
    ld c,a                                     ; CB37h
    call LCEC8                                 ; CB38h
    pop bc                                     ; CB3Bh
    inc hl                                     ; CB3Ch
    jp LCB1C                                   ; CB3Dh
LCB40:
    ld a,h                                     ; CB40h
    cp b                                       ; CB41h
    jp nz,LCB4A                                ; CB42h
    ld a,l                                     ; CB45h
    cp c                                       ; CB46h
    jp z,LCB1C                                 ; CB47h
LCB4A:
    dec hl                                     ; CB4Ah
    push bc                                    ; CB4Bh
    ld c,008h                                  ; CB4Ch
    call LCEC8                                 ; CB4Eh
    ld c,07Fh                                  ; CB51h
    call LCEC8                                 ; CB53h
    ld c,008h                                  ; CB56h
    call LCEC8                                 ; CB58h
    pop bc                                     ; CB5Bh
    jp LCB1C                                   ; CB5Ch
LCB5F:
    ld (hl),a                                  ; CB5Fh
    ret                                        ; CB60h
LCB61:
    push hl                                    ; CB61h
    push bc                                    ; CB62h
    ld hl,08F60h                               ; CB63h
    ld c,000h                                  ; CB66h
LCB68:
    ld a,(hl)                                  ; CB68h
    cp 02Ch                                    ; CB69h
    jp nz,LCB6F                                ; CB6Bh
    inc c                                      ; CB6Eh
LCB6F:
    inc hl                                     ; CB6Fh
    cp 00Dh                                    ; CB70h
    jp nz,LCB68                                ; CB72h
    dec c                                      ; CB75h
    ld a,c                                     ; CB76h
    ld (08F89h),a                              ; CB77h
    cp 001h                                    ; CB7Ah
    pop bc                                     ; CB7Ch
    pop hl                                     ; CB7Dh
    jp z,LCB99                                 ; CB7Eh
    push hl                                    ; CB81h
    push de                                    ; CB82h
    push bc                                    ; CB83h
    ld hl,(08F87h)                             ; CB84h
    ld c,l                                     ; CB87h
    ld b,h                                     ; CB88h
    call LCCE9                                 ; CB89h
    ld a,l                                     ; CB8Ch
    ld (08F8Bh),a                              ; CB8Dh
    ld hl,08F8Ch                               ; CB90h
    ld (hl),e                                  ; CB93h
    inc hl                                     ; CB94h
    ld (hl),c                                  ; CB95h
    pop bc                                     ; CB96h
    pop de                                     ; CB97h
    pop hl                                     ; CB98h
LCB99:
    ld a,c                                     ; CB99h
    ld (08F8Ah),a                              ; CB9Ah
LCB9D:
    push hl                                    ; CB9Dh
    ld hl,08F8Ah                               ; CB9Eh
    ld (08F8Eh),hl                             ; CBA1h
    pop hl                                     ; CBA4h
    call loader_compareHLDE                    ; CBA5h
    ret z                                      ; CBA8h
    call LCE4D                                 ; CBA9h
    ld a,(08F89h)                              ; CBACh
    ld (08F90h),a                              ; CBAFh
    ld b,a                                     ; CBB2h
LCBB3:
    ld c,(hl)                                  ; CBB3h
    push hl                                    ; CBB4h
    ld hl,(08F8Eh)                             ; CBB5h
    ld a,(hl)                                  ; CBB8h
    cp c                                       ; CBB9h
    jp nz,LCC07                                ; CBBAh
    inc hl                                     ; CBBDh
    ld (08F8Eh),hl                             ; CBBEh
    pop hl                                     ; CBC1h
    inc hl                                     ; CBC2h
    dec b                                      ; CBC3h
    jp nz,LCBB3                                ; CBC4h
    push hl                                    ; CBC7h
    push de                                    ; CBC8h
    push bc                                    ; CBC9h
    push af                                    ; CBCAh
    ld a,(08F89h)                              ; CBCBh
LCBCE:
    dec hl                                     ; CBCEh
    dec a                                      ; CBCFh
    jp nz,LCBCE                                ; CBD0h
    call LCC6C                                 ; CBD3h
    dec hl                                     ; CBD6h
    dec hl                                     ; CBD7h
    ld a,(08F89h)                              ; CBD8h
    add a,004h                                 ; CBDBh
    ld b,a                                     ; CBDDh
LCBDE:
    call LCC0C                                 ; CBDEh
    inc hl                                     ; CBE1h
    dec b                                      ; CBE2h
    jp nz,LCBDE                                ; CBE3h
    ld hl,LCF74                                ; CBE6h
    call LC8AD                                 ; CBE9h
    ld a,(08F89h)                              ; CBECh
    ld b,a                                     ; CBEFh
    add a,b                                    ; CBF0h
    add a,b                                    ; CBF1h
    dec a                                      ; CBF2h
    ld e,a                                     ; CBF3h
    ld c,05Fh                                  ; CBF4h
LCBF6:
    call LCEE5                                 ; CBF6h
    dec e                                      ; CBF9h
    jp nz,LCBF6                                ; CBFAh
    call LCF43                                 ; CBFDh
    pop af                                     ; CC00h
    pop bc                                     ; CC01h
    pop de                                     ; CC02h
    pop hl                                     ; CC03h
    jp LCB9D                                   ; CC04h
LCC07:
    pop hl                                     ; CC07h
    inc hl                                     ; CC08h
    jp LCB9D                                   ; CC09h
LCC0C:
    ld a,(hl)                                  ; CC0Ch
    push bc                                    ; CC0Dh
    call LCE9D                                 ; CC0Eh
    call LCE29                                 ; CC11h
    pop bc                                     ; CC14h
    ret                                        ; CC15h
LCC16:
    ld a,(hl)                                  ; CC16h
    cp c                                       ; CC17h
    jp z,LCC2A                                 ; CC18h
    call LCC6C                                 ; CC1Bh
    ld a,(hl)                                  ; CC1Eh
    push bc                                    ; CC1Fh
    call LCE9D                                 ; CC20h
    call LCE4D                                 ; CC23h
    call LCF43                                 ; CC26h
    pop bc                                     ; CC29h
LCC2A:
    call loader_compareHLDE                    ; CC2Ah
    ret z                                      ; CC2Dh
    inc hl                                     ; CC2Eh
    jp LCC16                                   ; CC2Fh
LCC32:
    ld (hl),c                                  ; CC32h
    call loader_compareHLDE                    ; CC33h
    ret z                                      ; CC36h
    inc hl                                     ; CC37h
    jp LCC32                                   ; CC38h
LCC3B:
    ld a,(hl)                                  ; CC3Bh
    ld (bc),a                                  ; CC3Ch
    call loader_compareHLDE                    ; CC3Dh
    ret z                                      ; CC40h
    inc hl                                     ; CC41h
    inc bc                                     ; CC42h
    jp LCC3B                                   ; CC43h
LCC46:
    ld a,(bc)                                  ; CC46h
    cp (hl)                                    ; CC47h
    jp z,LCC63                                 ; CC48h
    push bc                                    ; CC4Bh
    call LCC6C                                 ; CC4Ch
    ld a,(hl)                                  ; CC4Fh
    call LCE9D                                 ; CC50h
    call LCE29                                 ; CC53h
    pop bc                                     ; CC56h
    push bc                                    ; CC57h
    ld a,(bc)                                  ; CC58h
    call LCE9D                                 ; CC59h
    call LCE4D                                 ; CC5Ch
    call LCF43                                 ; CC5Fh
    pop bc                                     ; CC62h
LCC63:
    call loader_compareHLDE                    ; CC63h
    ret z                                      ; CC66h
    inc hl                                     ; CC67h
    inc bc                                     ; CC68h
    jp LCC46                                   ; CC69h
LCC6C:
    push bc                                    ; CC6Ch
    ld a,h                                     ; CC6Dh
    call LCE9D                                 ; CC6Eh
    ld a,l                                     ; CC71h
    call LCE9D                                 ; CC72h
    call LCE29                                 ; CC75h
    pop bc                                     ; CC78h
    ret                                        ; CC79h
LCC7A:
    call LCC89                                 ; CC7Ah
    push bc                                    ; CC7Dh
    ld hl,LCF60                                ; CC7Eh
    call LCE1D                                 ; CC81h
    pop hl                                     ; CC84h
    call LCC6C                                 ; CC85h
    ret                                        ; CC88h
LCC89:
    ld bc,00000h                               ; CC89h
LCC8C:
    ld a,(hl)                                  ; CC8Ch
    add a,c                                    ; CC8Dh
    ld c,a                                     ; CC8Eh
    push af                                    ; CC8Fh
    call loader_compareHLDE                    ; CC90h
    jp z,LCCAB                                 ; CC93h
    pop af                                     ; CC96h
    ld a,b                                     ; CC97h
    adc a,(hl)                                 ; CC98h
    ld b,a                                     ; CC99h
    call LCCA0                                 ; CC9Ah
    jp LCC8C                                   ; CC9Dh
LCCA0:
    call loader_compareHLDE                    ; CCA0h
    jp nz,LCCA9                                ; CCA3h
    inc sp                                     ; CCA6h
    inc sp                                     ; CCA7h
    ret                                        ; CCA8h
LCCA9:
    inc hl                                     ; CCA9h
    ret                                        ; CCAAh
LCCAB:
    pop af                                     ; CCABh
    ret                                        ; CCACh
LCCAD:
    call LCC6C                                 ; CCADh
    ld b,010h                                  ; CCB0h
LCCB2:
    ld c,(hl)                                  ; CCB2h
    ld a,(08F85h)                              ; CCB3h
    or a                                       ; CCB6h
    jp z,LCCBC                                 ; CCB7h
    and c                                      ; CCBAh
    ld c,a                                     ; CCBBh
LCCBC:
    ld a,c                                     ; CCBCh
    cp 020h                                    ; CCBDh
    jp c,LCCC7                                 ; CCBFh
    cp 07Fh                                    ; CCC2h
    jp c,LCCC9                                 ; CCC4h
LCCC7:
    ld c,02Eh                                  ; CCC7h
LCCC9:
    call LCEC8                                 ; CCC9h
    call LCE29                                 ; CCCCh
    call loader_compareHLDE                    ; CCCFh
    ret z                                      ; CCD2h
    dec b                                      ; CCD3h
    inc hl                                     ; CCD4h
    jp nz,LCCB2                                ; CCD5h
    call LCE4D                                 ; CCD8h
    call LCF43                                 ; CCDBh
    jp LCCAD                                   ; CCDEh
LCCE1:
    ld bc,08F60h                               ; CCE1h
    ld a,(bc)                                  ; CCE4h
    ld (08F80h),a                              ; CCE5h
    inc bc                                     ; CCE8h
LCCE9:
    call LCD10                                 ; CCE9h
    ld (08F81h),hl                             ; CCECh
    call LCD10                                 ; CCEFh
    ld (08F83h),hl                             ; CCF2h
    call LCD10                                 ; CCF5h
    ld (08F85h),hl                             ; CCF8h
    ld l,c                                     ; CCFBh
    ld h,b                                     ; CCFCh
    ld (08F87h),hl                             ; CCFDh
    ld hl,(08F85h)                             ; CD00h
    ld c,l                                     ; CD03h
    ld b,h                                     ; CD04h
    ld hl,(08F83h)                             ; CD05h
    ex de,hl                                   ; CD08h
    ld hl,(08F81h)                             ; CD09h
    ld a,(08F80h)                              ; CD0Ch
    ret                                        ; CD0Fh
LCD10:
    ld hl,00000h                               ; CD10h
LCD13:
    ld a,(bc)                                  ; CD13h
    cp 00Dh                                    ; CD14h
    ret z                                      ; CD16h
    cp 02Ch                                    ; CD17h
    jp z,LCD29                                 ; CD19h
    call LCD2B                                 ; CD1Ch
    add hl,hl                                  ; CD1Fh
    add hl,hl                                  ; CD20h
    add hl,hl                                  ; CD21h
    add hl,hl                                  ; CD22h
    add a,l                                    ; CD23h
    ld l,a                                     ; CD24h
    inc bc                                     ; CD25h
    jp LCD13                                   ; CD26h
LCD29:
    inc bc                                     ; CD29h
    ret                                        ; CD2Ah
LCD2B:
    cp 020h                                    ; CD2Bh
    jp c,LCE14                                 ; CD2Dh
    cp 03Ah                                    ; CD30h
    jp c,LCD44                                 ; CD32h
    and 05Fh                                   ; CD35h
    cp 041h                                    ; CD37h
    jp c,LCE14                                 ; CD39h
    cp 047h                                    ; CD3Ch
    jp nc,LCE14                                ; CD3Eh
    sub 037h                                   ; CD41h
    ret                                        ; CD43h
LCD44:
    or 010h                                    ; CD44h
    sub 030h                                   ; CD46h
    ret                                        ; CD48h
LCD49:
    push hl                                    ; CD49h
    ld hl,08F60h                               ; CD4Ah
    push de                                    ; CD4Dh
    ld e,l                                     ; CD4Eh
    ld d,h                                     ; CD4Fh
    push bc                                    ; CD50h
    ld bc,08F80h                               ; CD51h
LCD54:
    call loader_getch                          ; CD54h
    cp 021h                                    ; CD57h
    jp c,LCD63                                 ; CD59h
    nop                                        ; CD5Ch
    nop                                        ; CD5Dh
    nop                                        ; CD5Eh
    nop                                        ; CD5Fh
    nop                                        ; CD60h
    nop                                        ; CD61h
    nop                                        ; CD62h
LCD63:
    nop                                        ; CD63h
    nop                                        ; CD64h
    nop                                        ; CD65h
    nop                                        ; CD66h
    nop                                        ; CD67h
    nop                                        ; CD68h
    nop                                        ; CD69h
    cp 008h                                    ; CD6Ah
    jp z,LCD87                                 ; CD6Ch
    cp 00Dh                                    ; CD6Fh
    jp z,LCDA0                                 ; CD71h
    ld (hl),a                                  ; CD74h
    ld c,a                                     ; CD75h
    call LCEC8                                 ; CD76h
    inc hl                                     ; CD79h
    ld a,h                                     ; CD7Ah
    cp b                                       ; CD7Bh
    jp nz,LCD54                                ; CD7Ch
    ld a,l                                     ; CD7Fh
    cp c                                       ; CD80h
    jp nz,LCD54                                ; CD81h
    jp LCE59                                   ; CD84h
LCD87:
    call loader_compareHLDE                    ; CD87h
    jp z,LCD54                                 ; CD8Ah
    dec hl                                     ; CD8Dh
    ld c,008h                                  ; CD8Eh
    call LCEC8                                 ; CD90h
    ld c,07Fh                                  ; CD93h
    call LCEC8                                 ; CD95h
    ld c,008h                                  ; CD98h
    call LCEC8                                 ; CD9Ah
    jp LCD54                                   ; CD9Dh
LCDA0:
    ld (hl),a                                  ; CDA0h
    pop bc                                     ; CDA1h
    pop de                                     ; CDA2h
    pop hl                                     ; CDA3h
    ret                                        ; CDA4h
LCDA5:
    call LCF3B                                 ; CDA5h
LCDA8:
    call LCF43                                 ; CDA8h
    call LCC6C                                 ; CDABh
    ld a,(hl)                                  ; CDAEh
    call LCE9D                                 ; CDAFh
    call LCE29                                 ; CDB2h
    call loader_getch                          ; CDB5h
    call LCE56                                 ; CDB8h
    cp 01Ah                                    ; CDBBh
    jp z,LCDC9                                 ; CDBDh
    cp 008h                                    ; CDC0h
    jp z,LCDCD                                 ; CDC2h
    call LCDD1                                 ; CDC5h
    ld (hl),a                                  ; CDC8h
LCDC9:
    inc hl                                     ; CDC9h
    jp LCDA8                                   ; CDCAh
LCDCD:
    dec hl                                     ; CDCDh
    jp LCDA8                                   ; CDCEh
LCDD1:
    push bc                                    ; CDD1h
    jp LCDD9                                   ; CDD2h
    push bc                                    ; CDD5h
    call loader_getch                          ; CDD6h
LCDD9:
    call LCDEA                                 ; CDD9h
    rlca                                       ; CDDCh
    rlca                                       ; CDDDh
    rlca                                       ; CDDEh
    rlca                                       ; CDDFh
    ld b,a                                     ; CDE0h
    call loader_getch                          ; CDE1h
    call LCDEA                                 ; CDE4h
    or b                                       ; CDE7h
    pop bc                                     ; CDE8h
    ret                                        ; CDE9h
LCDEA:
    cp 020h                                    ; CDEAh
    jp c,LCE59                                 ; CDECh
    cp 03Ah                                    ; CDEFh
    jp c,LCE09                                 ; CDF1h
    and 05Fh                                   ; CDF4h
    cp 041h                                    ; CDF6h
    jp c,LCE14                                 ; CDF8h
    cp 047h                                    ; CDFBh
    jp nc,LCE14                                ; CDFDh
    push af                                    ; CE00h
    ld c,a                                     ; CE01h
    call LCEC8                                 ; CE02h
    pop af                                     ; CE05h
    sub 037h                                   ; CE06h
    ret                                        ; CE08h
LCE09:
    or 010h                                    ; CE09h
    push af                                    ; CE0Bh
    ld c,a                                     ; CE0Ch
    call LCEC8                                 ; CE0Dh
    pop af                                     ; CE10h
    sub 030h                                   ; CE11h
    ret                                        ; CE13h
LCE14:
    ld hl,monitor2_messages                    ; CE14h
    call LCE1D                                 ; CE17h
    jp LCE59                                   ; CE1Ah
LCE1D:
    ld a,(hl)                                  ; CE1Dh
    ld c,a                                     ; CE1Eh
    cp 000h                                    ; CE1Fh
    ret z                                      ; CE21h
    call LCEC8                                 ; CE22h
    inc hl                                     ; CE25h
    jp LCE1D                                   ; CE26h
LCE29:
    ld c,020h                                  ; CE29h
    call LCEC8                                 ; CE2Bh
    ret                                        ; CE2Eh
LCE2F:
    call LCC6C                                 ; CE2Fh
    ld b,010h                                  ; CE32h
LCE34:
    ld a,(hl)                                  ; CE34h
    call LCE9D                                 ; CE35h
    call LCE29                                 ; CE38h
    call loader_compareHLDE                    ; CE3Bh
    ret z                                      ; CE3Eh
    dec b                                      ; CE3Fh
    inc hl                                     ; CE40h
    jp nz,LCE34                                ; CE41h
    call LCE4D                                 ; CE44h
    call LCF43                                 ; CE47h
    jp LCE2F                                   ; CE4Ah
LCE4D:
    call LCE5C                                 ; CE4Dh
    cp 01Fh                                    ; CE50h
    ret nz                                     ; CE52h
    call loader_getch                          ; CE53h
LCE56:
    cp 01Fh                                    ; CE56h
    ret nz                                     ; CE58h
LCE59:
    jp monitor2_restart                        ; CE59h
LCE5C:
    call LCE65                                 ; CE5Ch
    push af                                    ; CE5Fh
    call loader_keybMode82                     ; CE60h
    pop af                                     ; CE63h
    ret                                        ; CE64h
LCE65:
    push hl                                    ; CE65h
    push bc                                    ; CE66h
    call loader_keybMode91                     ; CE67h
    ld a,(0FF00h)                              ; CE6Ah
    cp 0FFh                                    ; CE6Dh
    jp nz,LCE82                                ; CE6Fh
    ld a,(0FF02h)                              ; CE72h
    or 0F0h                                    ; CE75h
    cp 0FFh                                    ; CE77h
    jp z,LC248                                 ; CE79h
    ld h,a                                     ; CE7Ch
    ld l,0FFh                                  ; CE7Dh
    jp LCE85                                   ; CE7Fh
LCE82:
    ld l,a                                     ; CE82h
    ld h,0FFh                                  ; CE83h
LCE85:
    ld c,0FBh                                  ; CE85h
LCE87:
    inc c                                      ; CE87h
    add hl,hl                                  ; CE88h
    jp c,LCE87                                 ; CE89h
    ld l,c                                     ; CE8Ch
    call loader_keybMode82                     ; CE8Dh
    ld a,(0FF01h)                              ; CE90h
    or 003h                                    ; CE93h
    cp 0FFh                                    ; CE95h
    jp nz,LC235                                ; CE97h
    jp LC248                                   ; CE9Ah
LCE9D:
    push de                                    ; CE9Dh
    push bc                                    ; CE9Eh
    call LCEAD                                 ; CE9Fh
    ld c,d                                     ; CEA2h
    call LCEC8                                 ; CEA3h
    ld c,e                                     ; CEA6h
    call LCEC8                                 ; CEA7h
    pop bc                                     ; CEAAh
    pop de                                     ; CEABh
    ret                                        ; CEACh
LCEAD:
    ld e,a                                     ; CEADh
    rrca                                       ; CEAEh
    rrca                                       ; CEAFh
    rrca                                       ; CEB0h
    rrca                                       ; CEB1h
    call LCEBC                                 ; CEB2h
    ld d,a                                     ; CEB5h
    ld a,e                                     ; CEB6h
    call LCEBC                                 ; CEB7h
    ld e,a                                     ; CEBAh
    ret                                        ; CEBBh
LCEBC:
    and 00Fh                                   ; CEBCh
    cp 00Ah                                    ; CEBEh
    jp c,LCEC5                                 ; CEC0h
    add a,007h                                 ; CEC3h
LCEC5:
    add a,030h                                 ; CEC5h
    ret                                        ; CEC7h
LCEC8:
    push af                                    ; CEC8h
    ld a,c                                     ; CEC9h
    cp 007h                                    ; CECAh
    call z,loader_beep                         ; CECCh
    cp 020h                                    ; CECFh
    jp c,LCEE0                                 ; CED1h
    push bc                                    ; CED4h
    ld c,07Fh                                  ; CED5h
    call LCEE5                                 ; CED7h
    ld c,008h                                  ; CEDAh
    call LCEE5                                 ; CEDCh
    pop bc                                     ; CEDFh
LCEE0:
    call LCEE5                                 ; CEE0h
    pop af                                     ; CEE3h
    ret                                        ; CEE4h
LCEE5:
    ld a,(08FFCh)                              ; CEE5h
    cp 0F8h                                    ; CEE8h
    call nc,LCEF1                              ; CEEAh
    call loader_printChar                      ; CEEDh
    ret                                        ; CEF0h
LCEF1:
    ld a,c                                     ; CEF1h
    cp 00Ah                                    ; CEF2h
    jp z,LCF02                                 ; CEF4h
    cp 01Ah                                    ; CEF7h
    jp z,LCF02                                 ; CEF9h
    ld a,(08FFDh)                              ; CEFCh
    cp 0BEh                                    ; CEFFh
    ret c                                      ; CF01h
LCF02:
    push hl                                    ; CF02h
    push de                                    ; CF03h
    push bc                                    ; CF04h
    ld hl,09000h                               ; CF05h
    ld de,0900Ah                               ; CF08h
    ld b,030h                                  ; CF0Bh
LCF0D:
    ld c,078h                                  ; CF0Dh
LCF0F:
    ld a,(de)                                  ; CF0Fh
    ld (hl),a                                  ; CF10h
    inc hl                                     ; CF11h
    inc de                                     ; CF12h
    ld a,(de)                                  ; CF13h
    ld (hl),a                                  ; CF14h
    inc hl                                     ; CF15h
    inc de                                     ; CF16h
    dec c                                      ; CF17h
    jp nz,LCF0F                                ; CF18h
    ld a,(08FFAh)                              ; CF1Bh
    ld c,010h                                  ; CF1Eh
LCF20:
    ld (hl),a                                  ; CF20h
    inc hl                                     ; CF21h
    inc de                                     ; CF22h
    dec c                                      ; CF23h
    jp nz,LCF20                                ; CF24h
    dec b                                      ; CF27h
    jp nz,LCF0D                                ; CF28h
    ld hl,000F8h                               ; CF2Bh
    ld (08FFCh),hl                             ; CF2Eh
    pop bc                                     ; CF31h
    pop de                                     ; CF32h
    pop hl                                     ; CF33h
    ld a,c                                     ; CF34h
    cp 00Ah                                    ; CF35h
    ret nz                                     ; CF37h
    ld c,01Ah                                  ; CF38h
    ret                                        ; CF3Ah
LCF3B:
    push bc                                    ; CF3Bh
    ld c,019h                                  ; CF3Ch
    call LCEC8                                 ; CF3Eh
    pop bc                                     ; CF41h
    ret                                        ; CF42h
LCF43:
    ld c,00Ah                                  ; CF43h
    call LCEC8                                 ; CF45h
    ret                                        ; CF48h

; Строки Monitor-2: исходные байты КОИ-7 сохранены без перекодирования.
monitor2_messages:
    DB 020h,06Fh,07Bh,069h,062h,06Bh,061h,021h ; CF49h
    DB 000h                                    ; CF51h
LCF52:
    DB 00Ah,02Ah,020h,06Dh,06Fh,06Eh,069h,074h ; CF52h
    DB 06Fh,072h,020h,03Fh,020h,000h           ; CF5Ah
LCF60:
    DB 06Bh,06Fh,06Eh,074h,072h,06Fh,06Ch,078h ; CF60h
    DB 06Eh,061h,071h,020h,073h,075h,06Dh,06Dh ; CF68h
    DB 061h,020h,03Dh,000h                     ; CF70h
LCF74:
    DB 00Dh,020h,020h,020h,020h,020h,020h,020h ; CF74h
    DB 020h,020h,020h,02Eh,000h                ; CF7Ch
LCF81:
    DB 069h,06Dh,071h,020h,066h,061h,06Ah,06Ch ; CF81h
    DB 061h,020h,03Fh,020h,000h                ; CF89h
LCF8E:
    DB 00Ah,066h,061h,06Ah,06Ch,03Ah,020h,000h ; CF8Eh
LCF96:
    DB 00Ah,06Eh,061h,07Eh,061h,06Ch,078h,06Eh ; CF96h
    DB 079h,06Ah,020h,061h,064h,072h,065h,073h ; CF9Eh
    DB 020h,03Dh,000h                          ; CFA6h
LCFA9:
    DB 00Ah,06Bh,06Fh,06Eh,065h,07Eh,06Eh,079h ; CFA9h
    DB 06Ah,020h,020h,061h,064h,072h,065h,073h ; CFB1h
    DB 020h,03Dh,000h                          ; CFB9h
LCFBC:
    DB 00Dh,00Ah,06Fh,07Bh,069h,062h,06Bh,061h ; CFBCh
    DB 020h,070h,072h,069h,020h,077h,077h,06Fh ; CFC4h
    DB 064h,065h,02Eh,020h,020h,020h,020h,020h ; CFCCh
    DB 020h,000h                               ; CFD4h
monitor2_registerNames:
    DB 041h,03Dh,000h,046h,03Dh,000h,042h,03Dh ; CFD6h
    DB 000h,043h,03Dh,000h,044h,03Dh,000h,045h ; CFDEh
    DB 03Dh,000h,048h,03Dh,000h,04Ch,03Dh,000h ; CFE6h
    DB 00Ah,020h,04Dh,028h,048h,04Ch,029h,03Dh ; CFEEh
    DB 000h,050h,043h,03Dh,000h,053h,050h,03Dh ; CFF6h
    DB 000h,000h                               ; CFFEh
