; Одноразовый инициализатор H:, загружаемый FILE_EXEC в страницу DOS.
    INCLUDE "../common/system.inc"
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
LOCAL_KIND  = 1
    INCLUDE "driver-init.inc"
    END
