#ifndef MXOS_QUEUE_H
#define MXOS_QUEUE_H
#include "mxos.h"
/* One bound queue per execution bank. The caller owns this object and buffer.
 * Write <=max_command bytes at next, then commit the resulting end pointer.
 * For a larger record, reserve its known size before writing it. */
typedef struct {
    mxos_u8 *buffer, *next;
    mxos_u16 capacity, allocation, max_command, used;
    mxos_u8 bank, error, target;
    mxos_u16 handle;
} MXOS_COMMAND_QUEUE;
mxos_u8 mxos_command_open(MXOS_COMMAND_QUEUE *q,void *buffer,mxos_u16 capacity,mxos_u16 max_command);
mxos_u8 mxos_command_select(MXOS_COMMAND_QUEUE *q,mxos_u8 target,mxos_u16 handle);
mxos_u8 mxos_module_scope(mxos_u16 handle,void (*controller)(void));
/* Inside a module scope. Input stays local; result receives the DLL's CPU registers. */
void mxos_route_call(mxos_u16 handle,const MXOS_MODULE_REGS *input,MXOS_REGS *result);
mxos_u8 *mxos_command_reserve(MXOS_COMMAND_QUEUE *q,mxos_u16 bytes);
void mxos_command_commit(MXOS_COMMAND_QUEUE *q,mxos_u8 *end);
void mxos_command_close(MXOS_COMMAND_QUEUE *q);
#endif
