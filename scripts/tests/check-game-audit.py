"""Проверить различение вызовов, регистров аппаратуры и использованных указателей 8080."""
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
analyse = runpy.run_path(str(ROOT/"scripts/games/audit-mxos-compat.py"))["analyse"]


def evidence(data):
    return analyse({"load":0x100,"entry":0x100},bytes.fromhex(data),{0xc037},environment=True)["evidence"]


def check():
    direct = evidence("3a 00 f8 c3 00 f8")
    assert [(e["kind"],e["pc"],e["operation"]) for e in direct["hardware"]] == [("std_ppi",0x100,"read")]
    assert [(e["kind"],e["pc"],e["operation"]) for e in direct["abi"]] == [("ramfos_f800_vector",0x103,"jump")]
    unused = evidence("21 00 f8 21 00 80 7e c9")
    assert not unused["abi"] and not unused["hardware"] and unused["pointer"][0]["pair"]=="HL"
    used = evidence("21 00 f8 7e 23 77 c9")
    assert [(e["target"],e["pointer_pc"],e["pc"],e["operation"]) for e in used["hardware"]] == [
        (0xf800,0x100,0x103,"read"),(0xf801,0x100,0x105,"write")]
    regs = evidence("3a ec ff 32 f8 ff 3a fc ff 32 fd ff 3a 3e c6 cd 37 c0 c9")
    assert [e["kind"] for e in regs["hardware"]] == ["mx_pit","mx_color","mx_page"]
    assert [e["kind"] for e in regs["abi"]] == ["ramfos_variable","legacy_monitor_entry"]
    indirect = evidence("21 37 c0 e9")
    assert [(e["kind"],e["pc"],e["pointer_pc"]) for e in indirect["abi"]]==[("legacy_monitor_entry",0x103,0x100)]
    word = evidence("22 4e c6 c9")["abi"]
    assert [(e["target"],e["effective_address"],e["reason"]) for e in word]==[
        (0xc64e,0xc64e,"v_oldBeep"),(0xc64e,0xc64f,"v_oldBeep+1")]
    assert all(e["reason"] for group in regs.values() for e in group)
    print("LDA, JMP, замена HL, чтение/запись через HL, ВИ53, цвет, запись страницы и ABI проверены.")


if __name__=="__main__":
    check()
