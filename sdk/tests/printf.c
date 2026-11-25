#include <stdio.h>
#include "mxos.h"
#include "mxos_runtime.h"

#define TEXT64 "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-"
#define TEXT256 TEXT64 TEXT64 TEXT64 TEXT64
#define TEXT300 TEXT256 "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefgh"

static char mixed_format[] = "[%d/%u/%s/%c/%%]\r\n";
static char local_buffer[300];

int main(void)
{
    MXOS_FAR_ADDRESS main_buffer;

    if (printf("") != 0)
        return 1;
    if (printf(mixed_format, -32768, 65535u, "MXOS", 'X') != 25)
        return 2;
    if (printf("A%cB%cC%cD", 0, '\n', '\r') != 7)
        return 3;
    if (printf(TEXT256 TEXT256 "<%s>" TEXT256 "!", TEXT256) != 1027)
        return 4;
    if (mxos_home_page) {
        if (mxos_main_arena((void *)0x7400, sizeof local_buffer) & MXOS_CF)
            return 5;
        if (mxos_malloc_far(MXOS_ALLOC_MAIN, sizeof local_buffer, &main_buffer) & MXOS_CF)
            return 6;
        mxos_printf_buffer(local_buffer, main_buffer.offset, sizeof local_buffer);
        if (printf("%s%s", TEXT300, TEXT300) != 600)
            return 7;
        if (printf("!%c!", 0) != 3)
            return 8;
        mxos_printf_buffer(local_buffer, 0xff00, sizeof local_buffer);
        if (printf("%s", TEXT256) != -1)
            return 9;
        mxos_printf_buffer(local_buffer, 0x8c00 - sizeof local_buffer + 1, sizeof local_buffer);
        if (printf("!") != -1)
            return 10;
        mxos_printf_buffer(local_buffer, 0x00ff, sizeof local_buffer);
        if (printf("!") != -1)
            return 11;
        mxos_printf_buffer(0, 0, 0);
        if (mxos_free_far(&main_buffer) & MXOS_CF)
            return 12;
        if (printf("%s", TEXT256) != 256)
            return 13;
    }
    if (printf("\r\npage=%u\r\n", (unsigned int)mxos_home_page) !=
        (mxos_home_page < 10 ? 10 : 11))
        return 14;
    return 0;
}
