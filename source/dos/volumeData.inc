; Отдельно от v_drives: это драйверы файловой системы, не физических блоков.
; Все восемь букв по умолчанию используют прежний формат MXOS.
v_volumes:         DW fatVolumeDriver, fatVolumeDriver, fatVolumeDriver, fatVolumeDriver
                   DW fatVolumeDriver, fatVolumeDriver, fatVolumeDriver, fatVolumeDriver
v_volumeContexts:  BLOCK 16, 0
v_volumeTable:     DW fatVolumeDriver + VOLUME_DRIVER_HEADER
v_volumeContext:   DW 0
