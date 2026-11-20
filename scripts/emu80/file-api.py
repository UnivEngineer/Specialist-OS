"""Host-side whole-file convenience for tests; only FILEHANDLE is invoked."""
import struct

def main_call(machine, symbols, operation, hl=0, a=0):
    """Isolated ABI fixture only; this intentionally replaces MAIN test code."""
    machine.write(0x2f80,struct.pack('<4H',hl,0,0,(a<<8)|2))
    machine.write(0x2800,b'\xcd\x00\xf8\xc3\x00\x8b')
    machine.stop_at(0x8b00)
    machine.call('regs_set',pc=0x2800,sp=0x8fdf,bc=operation,de=0x2f80)
    state=machine.until(0x8b00,90,page=0)
    af=state['registers']['af']['dec']
    if af&1:raise AssertionError((hex(operation),af>>8,state))
    return state

def path_io(machine, call, symbols, address, writing=False, handle=0x2e00):
    word=lambda n:struct.pack('<H',n)
    original=bytearray(machine.read(address,40))
    path,buffer,count=struct.unpack_from('<3H',original)
    request=bytearray(64)
    request[41]=14 if writing else 1
    request[49:51]=word(path)
    machine.write(handle,request)
    call(symbols['MXOS_DOS_FILEHANDLE'],hl=handle,a=symbols['FILE_OP_OPEN_PATH'])
    opened=machine.read(handle,64)
    size=int.from_bytes(opened[28:32],'little')
    if not writing and size>count:
        call(symbols['MXOS_DOS_FILEHANDLE'],hl=handle,a=5)
        raise ValueError('Whole-file test buffer is too small')
    count=count if writing else size
    machine.write(handle+49,word(buffer)+word(count))
    call(symbols['MXOS_DOS_FILEHANDLE'],hl=handle,a=2 if writing else 1)
    actual=machine.read(handle+51,2)
    call(symbols['MXOS_DOS_FILEHANDLE'],hl=handle,a=5)
    original[6:8]=actual
    original[8:40]=machine.read(handle,32)
    machine.write(address,original)
