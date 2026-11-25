"""Измерение точного остатка CPM32 в видимом тестовом MX2."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import runpy
import struct

RUNTIME_NAME = "19782"
SP = 0x7800
PC = 0x1000
PACKET = 0x1800
WATCH_PORTS = (0xFFFC,0xFFFD,0xFFFE)
SKIP = 1_000_000
SENTINEL = bytes.fromhex("a55a")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkout", required=True, type=Path)
    parser.add_argument("--runtime", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    root = args.checkout.resolve()
    folder = args.out.resolve()
    folder.mkdir(parents=True,exist_ok=True)
    runtime = args.runtime.resolve()
    assert runtime.name == RUNTIME_NAME
    os.environ["MXOS_EMU80_SPEED"] = "32"
    tools = runpy.run_path(str(root/"scripts/emu80/benchmark-disks.py"))
    env = tools["Matrix"].__init__.__globals__
    env.update(OUT=folder,RUNTIME=runtime,
               mcp_owner=lambda:tools["shell"](
                   "$p=Get-NetTCPConnection -LocalPort 19782 -State Listen -ErrorAction SilentlyContinue; "
                   "if($p){$p[0].OwningProcess};exit 0"))
    symbols=runpy.run_path(str(root/"scripts/emu80/benchmark-mxcall.py"))["symbols"]
    t=symbols(root/"bin/banked/TRANSPORT.sym")
    matrix=tools["Matrix"]()
    m=matrix.m
    m.url="http://127.0.0.1:19782/mcp"
    box=root/"bin/MEM.sys"
    system=root/"images/C-Flash-512k-MMX-System-Emu80.bin"
    mem=box.read_bytes()
    system_sha=sha(system)
    segments=[struct.unpack_from("<HHH",mem,6+6*i) for i in range(mem[5])]

    def selector(page):
        if page&128:
            return 0xFFFE,page&15
        return (0xFFFD,page-1) if page else (0xFFFC,0)

    def select(page):
        addr,value=selector(page)
        m.write(addr,bytes([value]))

    def emit_selector(page):
        port,value=selector(page)
        return b'\x21'+struct.pack('<H',port)+b'\x36'+bytes([value])

    def run_case(caller,src_bank,dst_bank,count,src=0x2003,dst=0x4007):
        select(src_bank)
        if src_bank&128:
            payload=m.read(src,count)
        else:
            payload=bytes((i*73+(i>>8)+count)&255 for i in range(count))
            if count:
                m.write(src-1,b"\xA5"+payload+b"\x5A")
        select(dst_bank)
        m.write(dst-1,b"\xA5"+bytes(count)+b"\x5A")
        select(caller)
        m.write(PACKET,struct.pack("<BHBHH",src_bank,src,dst_bank,dst,count))
        symbol="bankCopyPhysical" if src_bank&128 else "mx_memcpyFar"
        fn=t[symbol]
        m.write(PC,b"\xcd"+struct.pack("<H",fn))
        m.stop_at(PC+3)
        for addr in WATCH_PORTS:
            m.call("bp_set",addr=addr,type="write",skipCount=SKIP)
        m.call("regs_set",pc=PC,sp=SP,hl=PACKET,de=0x3456,bc=0x1234)
        start=m.call("cpu_state")["cpu_ticks"]
        state=m.until(PC+3,10)
        ticks=state["cpu_ticks"]-start
        regs={k:v["dec"] for k,v in state["registers"].items()}
        assert (regs["sp"],regs["hl"],regs["de"],regs["bc"])==(SP,PACKET,0x3456,0x1234)
        assert not regs["af"]&1
        ports={bp["addr"]:bp["hitCount"] for bp in m.call("bp_list")["breakpoints"] if bp["type"]=="write"}
        assert set(ports)=={"FFFC","FFFD","FFFE"}
        m.call("bp_clear")
        assert m.read(t["bankPage"],1)[0]==caller
        assert m.read(0xFFDF,1)==bytes([1 if count else 0])
        if count%32:
            expected=emit_selector(src_bank)+b'\x0a\x03'+emit_selector(dst_bank)+b'\x12\x13'
            expected+=b'\x21\xdd\xff\x35\xc2\xc0\xff'+emit_selector(caller)
            expected+=b'\xc3'+struct.pack('<H',t["bankCopyOK"])+b'\x00\x00\x01'
            assert m.read(0xFFC0,32)==expected
        select(dst_bank)
        assert m.read(dst-1,count+2)==b"\xA5"+payload+b"\x5A"
        if count and not src_bank&128:
            select(src_bank)
            assert m.read(src-1,count+2)==b"\xA5"+payload+b"\x5A"
        return dict(caller=caller,src_bank=src_bank,dst_bank=dst_bank,count=count,src=src,dst=dst,ticks=ticks,
                    mapper_writes=ports)

    try:
        matrix.start("cpm32-remainder",system=system)
        cpu=m.call("sys_info")["cpu"]
        assert cpu["speed_factor"]==32 and not cpu["full_throttle"] and cpu["frequency_hz"]==2_000_000
        m.call("debug_break")
        local_size=32
        local_start,_,local_offset=segments[0]
        entry=t.get("MEM_COPY_TINY_ENTRY",t.get("MEM_COPY_TINY_TEMPLATE"))
        assert entry is not None
        for page in (0,1):
            select(page)
            offset=local_offset+entry-local_start
            assert m.read(entry,local_size)==mem[offset:offset+local_size], page
        select(6)
        for addr,size,offset in segments:
            if addr!=0xFFC0:
                m.write(addr,mem[offset:offset+size])
        m.write(t["bankPage"],b"\x06")
        m.write(t["bankPageCount"],b"\x10")
        window=(root/"bin/banked/WINDOW.bin").read_bytes()
        lengths=[0,*range(1,32),*range(64,96),32,33,96,255,256,257,511,512,513]
        cases=[(6,6,0,count) for count in lengths]
        cases += [(0,3,0,count) for count in (1,31,32,77,128)]
        cases += [(1,3,2,count) for count in (7,31,32,77,128)]
        cases += [(1,0x80,2,count) for count in (1,13,31,32,77,128)]
        cases += [(1,3,2,count,0xFF00-count,0xFF00-count) for count in (1,13,31,32,77)]
        rows=[]
        for case in cases:
            m.write(0xFFC0,window)
            rows.append(run_case(*case))
        (folder/"measured.json").write_text(json.dumps(
            dict(cpu=cpu,system_sha=system_sha,mem_sha=hashlib.sha256(mem).hexdigest(),rows=rows),indent=2)+"\n")
        print("PASS",len(rows),"physical cases",flush=True)
    finally:
        matrix.stop()


if __name__=="__main__":
    main()
