    IFNDEF MXOS_VERSION
    DEFINE MXOS_VERSION    "7.0"
    DEFINE MXOS_REVISION   "0"
    DEFINE NC_VERSION      "3.0"
    DEFINE NC_REVISION     "0"
    DEFINE EDIT_VERSION    "6.0"
    DEFINE EDIT_REVISION   "0"
    DEFINE FORMAT_VERSION  "1.0"
    DEFINE FORMAT_REVISION "0"

    ; Сборщик передаёт десятичный BUILD. Прямой запуск SjASMPlus оставляет 0.
    LUA ALLPASS
    local build = os.getenv("SPECIALIST_VERSION_BUILD") or "0"
    assert(build:match("^%d+$") and tonumber(build) <= 4294967295, "Invalid BUILD number")
    sj.insert_define("VERSION_BUILD", string.format("%q", build))
    ENDLUA
    ENDIF
