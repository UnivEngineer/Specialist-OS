#include "mxos_queue.h"
static MXOS_COMMAND_QUEUE *active;
void mxos_route_call(mxos_u16 handle,const MXOS_MODULE_REGS *input,MXOS_REGS *result)
{
    result->entry=MXOS_ROUTE_CALL_ADDR;
    result->hl=handle;
    result->de=(mxos_u16)input;
    result->af=result->bc=0;
    mxos_bios_call(result);
}
static void send(void)
{
    MXOS_CALL_PACKET r;
    mxos_u16 bytes,rounded;
    if (!active->used) return;
    bytes=active->used;
    if (active->target!=MXOS_QUEUE_DLL && bytes>20 && (bytes&31)>=8) {
        rounded=(bytes+31)&0xffe0;
        if (rounded<=active->capacity) { *active->next=0; bytes=rounded; }
    }
    r.operation=active->target==MXOS_QUEUE_DOS ? MXOS_DOS_COMMAND_BATCH : MXOS_BIOS_COMMAND_BATCH; r.hl=(mxos_u16)active->buffer;
    r.bc=bytes; r.de=0; r.af=(mxos_u16)active->bank<<8;
    if (active->target==MXOS_QUEUE_DLL) {
        MXOS_MODULE_REGS packet;
        MXOS_REGS call;
        packet.hl=r.hl; packet.de=0; packet.bc=bytes; packet.af=r.af;
        mxos_route_call(active->handle,&packet,&call);
        if (call.af&MXOS_CF) active->error=call.af>>8;
    } else if (mxos_call_packet(&r)&MXOS_CF) active->error=r.af>>8;
    active->used=0; active->next=active->buffer;
}
mxos_u8 mxos_command_open(MXOS_COMMAND_QUEUE *q,void *buffer,mxos_u16 capacity,mxos_u16 max_command)
{
    MXOS_REGS r;
    if (capacity<3 || max_command<3 || max_command>255 || max_command>capacity || (mxos_u16)buffer+capacity<(mxos_u16)buffer) return MXOS_MEM_ERR_RANGE;
    r.entry=MXOS_MEM_INFO; mxos_bios_call(&r);
    q->buffer=q->next=buffer; q->allocation=capacity; q->capacity=capacity>256 ? 256 : capacity; q->max_command=max_command;
    q->target=MXOS_QUEUE_BIOS; q->handle=0; q->used=0; q->error=0; q->bank=r.af>>8; active=q;
    mxos_queue_bind(send); return 0;
}
mxos_u8 mxos_command_select(MXOS_COMMAND_QUEUE *q,mxos_u8 target,mxos_u16 handle)
{
    if (target>MXOS_QUEUE_DLL || (target==MXOS_QUEUE_DLL && !handle)) return MXOS_MEM_ERR_RANGE;
    if (target!=MXOS_QUEUE_DLL) handle=0;
    if (q->target!=target || q->handle!=handle) {
        mxos_queue_flush();
        q->target=target; q->handle=handle;
        q->capacity=(target!=MXOS_QUEUE_DLL && q->allocation>256) ? 256 : q->allocation;
    }
    return q->error;
}
mxos_u8 mxos_module_scope(mxos_u16 handle,void (*controller)(void))
{
    MXOS_CALL_PACKET r;
    r.operation=MXOS_DOS_MODULE_SCOPE; r.hl=handle; r.de=(mxos_u16)controller;
    r.af=r.bc=0; return mxos_call_packet(&r)&MXOS_CF ? r.af>>8 : 0;
}
mxos_u8 *mxos_command_reserve(MXOS_COMMAND_QUEUE *q,mxos_u16 bytes)
{
    if (bytes<3 || (q->target!=MXOS_QUEUE_DLL && bytes>255) || bytes>q->capacity) { q->error=MXOS_MEM_ERR_RANGE; return 0; }
    if (q->capacity-q->used<bytes) mxos_queue_flush();
    return q->next;
}
void mxos_command_commit(MXOS_COMMAND_QUEUE *q,mxos_u8 *end)
{
    q->next=end; q->used=end-q->buffer;
    if (q->capacity-q->used<q->max_command) mxos_queue_flush();
}
void mxos_command_close(MXOS_COMMAND_QUEUE *q)
{
    mxos_queue_flush(); mxos_queue_bind(0); active=0;
}
