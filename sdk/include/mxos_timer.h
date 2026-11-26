/* Владение таймером и измерение в локальной странице IO.SYS. */
#ifndef MXOS_TIMER_H
#define MXOS_TIMER_H
#include "mxos_types.h"

#define MXOS_TIMER_RAW     0
#define MXOS_TIMER_CLOCK   1
#define MXOS_TIMER_RELEASE 2
#define MXOS_TIMER_QUERY   3

/* Результат AF, бит 0 — CF. RAW передаёт настройку счётчиков владельцу. */
mxos_u16 mxos_timer_request(mxos_u8 operation);
/* Возвращает 0 для MMIO, 1 для IN/OUT. Записывает базовый адрес или порт. */
mxos_u8 mxos_pit_get_port(mxos_u16 *base) MXOS_FASTCALL;
/* После настройки счётчика владельцем. Прямой CALL локального входа IO.SYS. */
mxos_u16 mxos_pit_read(void);
#endif
