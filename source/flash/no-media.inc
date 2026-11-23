;----------------------------------------------------------------------------
; MXOS
;
; Зарегистрировать выбранный драйвер с нулевой ёмкостью и службой No media.
; вход: registration — заполненный дескриптор установщика ROM или flash.
; выход: cf/a — результат регистрации; AF/BC/DE/HL рабочие. Носитель не меняется.
;----------------------------------------------------------------------------
installNoMedia:
    ld hl,0
    ld (registration+DRIVER_INSTALL.blocks),hl
    ld (registration+DRIVER_INSTALL.allocationStep),hl
    ld hl,noMediaService
    ld (registration+DRIVER_INSTALL.image),hl
    ld hl,noMediaServiceEnd-noMediaService
    ld (registration+DRIVER_INSTALL.imageSize),hl
    jp installDriver

; B=4 успешно: у пустой службы нет отложенных записей. Иначе CF=1/A=No media.
; BC/DE/HL сохранены. Тело копируется и может исполняться в любой странице.
noMediaService:
    ld a,b
    cp 4
    ret z
    ld a,DOS_ERR_NO_MEDIA
    scf
    ret
noMediaServiceEnd:
