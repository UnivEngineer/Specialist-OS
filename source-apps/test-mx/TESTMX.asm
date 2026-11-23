; Метаданные нативного системного теста; историческое тело не изменяется.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS         = IMAGE_FLAG_MAIN
IMAGE_API           = 0
IMAGE_MEMORY_POLICY = IMAGE_POLICY_EXCLUSIVE
IMAGE_BSS_SIZE      = IMAGE_BSS_UNKNOWN
    ORG 0
    INCBIN "../../assets/programs/Test-MX.COM"
    END
