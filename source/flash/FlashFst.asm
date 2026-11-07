; Одноразовый инициализатор H:, загружаемый FILEEXEC в страницу DOS.
    INCLUDE "../include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
LOCAL_KIND = 2
    INCLUDE "driver-init.inc"
    END
