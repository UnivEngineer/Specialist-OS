    DEFINE DWC_READ_BUFFER BIOS_TEMP_BUFFER
    INCLUDE "../common/diskWriteChecked.inc"
    UNDEFINE DWC_READ_BUFFER
    INCLUDE "../common/localBlockChecked.inc"
