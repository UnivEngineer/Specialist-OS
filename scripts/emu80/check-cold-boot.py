"""Проверить холодный запуск новой игры с сохранением PID, MCP и видимого окна Qt."""
import argparse
import json
from pathlib import Path
import tempfile

from mcp_pool import FLASH_PROTECTION, Machine, ROOT, lease


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port",type=int,choices=range(19781,19797),default=19796)
    parser.add_argument("--out",type=Path,default=ROOT/"docs/games/mcp-pool/check-cold.json")
    args = parser.parse_args()
    system = ROOT/"images/C-Flash-512k-MMX-System-Emu80.bin"
    floppy = ROOT/"images/A-Floppy-720k-Programs.img"
    original = (ROOT/"images/H-Flash-2M-Games.bin").read_bytes()
    rows = []
    with tempfile.TemporaryDirectory() as tmp:
        disk = Path(tmp)/"H.bin"
        def seed(value):
            image = bytearray(original)
            image[0x1212] = value
            disk.write_bytes(image)
        seed(0xa0)
        with lease(port=args.port,system=system,disk=disk,floppy=floppy,artifact_root=args.out.parent/"cold-slots") as slot:
            m = Machine(slot.url)
            for index in range(3):
                if index:
                    seed(0xa0+index)
                    slot.cold_boot(system=system,disk=disk,floppy=floppy)
                m.call("debug_break")
                m.call("emu_reset")
                # Одна запись без последовательности SDP не должна программировать C:.
                byte = m.read(0x10,1)[0]
                m.write(0x10,bytes([byte^255]))
                m.call("emu_run_for",ms=20)
                m.call("emu_reset")
                assert m.read(0x10,1)==bytes([byte]) and slot.protection==FLASH_PROTECTION
                assert (slot.media_dir/"system-flash.bin").read_bytes()==system.read_bytes()
                m.write(0xfffc,b"\x00")
                # Маркеры в MAIN и двух разных банках расширенной памяти.
                edits = [(0xfffc,0),(0xfffd,0),(0xfffd,7)]
                observed = []
                for reg,value in edits:
                    m.write(reg,bytes([value]))
                    observed.append(m.read(0x6251,1)[0])
                    m.write(0x6251,b"\xce")
                m.write(0xfffc,b"\0")
                m.write(0xffe7,b"\x90")
                m.write(0xffe6,b"\x60")
                m.write(0xffe5,b"\x12")
                media = m.read(0xffe4,1)[0]
                hwnd = slot.hwnd
                rows.append(dict(cold_boots=slot.cold_boots,pid=slot.pid,hwnd=hwnd,main_and_banks=observed,
                                 disk_byte=media,media_dir=str(slot.media_dir),port=slot.port,
                                 protection=slot.protection,unauthorized_system_write_ignored=True))
                assert media==0xa0+index and slot.verify_owner(),rows
                assert observed==[0,0,0] and hwnd==rows[0]["hwnd"],rows
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(rows,indent=2)+"\n","utf-8")
    print(json.dumps(rows,indent=2))


if __name__=="__main__":
    main()
