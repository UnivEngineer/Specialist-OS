"""Проверить передачу управления и штатный возврат из четырёх сред RUN через NC."""
import json
from pathlib import Path
import struct
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "scripts/games"), str(ROOT / "scripts/images"), str(ROOT / "scripts/emu80")]
from campaign_guest import Guest
from campaign_media import DISK, SYSTEM, read_tree, trial_disk
from executable import Image, decode, encode
from mcp_pool import WorkerMachine

OUT = ROOT / "docs/compat/run-environments"
ENTRY = 0x2000
# Приложение читает строку аргументов по DE и возвращается через RET.
BODY = bytes.fromhex("1A FE 58 C2 09 20 3E 01 C9 3E 00 C9")


def trial(machine, files, disk, kind, option, profile, core):
    name = option.strip("-") or "default"
    folder = OUT / name
    folder.mkdir(parents=True, exist_ok=True)
    observation = {}
    slot = machine.start(disk=disk, system=SYSTEM, floppy=ROOT / "images/A-Floppy-720k-Programs.img")
    guest = Guest(slot, folder, ROOT / "bin", "NATIVE", files, observation)
    assert guest.shell(reset=True), name
    command = "C:/DOS/RUN.COM H:/PROBE." + kind
    if option:
        command += " " + option
    command += " X"
    guest.line(command, "command")
    run_code = decode(files["DOS/RUN.COM"], "com").data
    def in_run():
        return guest.read(0, 32) == run_code[:32]
    guest.points(0)
    guest.tap("enter")
    assert (guest.regs()["pc"] == 0 and in_run()) or guest.seek(0, 16000, in_run), command
    guest.points(ENTRY)
    def at_entry():
        return guest.read(ENTRY, len(BODY)) == BODY
    if not ((guest.regs()["pc"] == ENTRY and at_entry()) or guest.seek(ENTRY, 16000, at_entry)):
        guest.frame("entry-failed")
        raise AssertionError((command, guest.regs(), guest.read(ENTRY, len(BODY)).hex()))
    regs = guest.regs()
    args = guest.read(regs["de"], 2)
    assert args[:1].upper() == b"X" and args[1] in (0, 13), (command, args)
    if core:
        raw = files["DOS/" + core]
        assert guest.read(0xc800, 0x36) == raw[0x800:0x836], command
    else:
        assert guest.read(guest.bios["biosProfile"], 1)[0] == profile, command
    guest.points()
    observation.update(command=command, entry=ENTRY, sp=regs["sp"], args=args.hex(), core=core, profile=profile)
    guest.call("debug_step", mode="into")
    observation["return_path"] = "reset" if core else "ret"
    observation["returned_shell"] = guest.shell(reset=bool(core))
    assert observation["returned_shell"], (command, guest.regs())
    (folder / "result.json").write_text(json.dumps(observation, ensure_ascii=False, indent=2), encoding="utf8")
    print(command, observation["returned_shell"], flush=True)


def main():
    files = {name.upper(): body for name, body in read_tree(SYSTEM.read_bytes())[0].items()}
    machine = WorkerMachine(16, None, OUT / "slots")
    try:
        with tempfile.TemporaryDirectory() as tmp:
            # Частный чистый H: с геометрией штатного диска. Все записи и файлы
            # испытания создаются здесь, независимо от заполненности игрового H:.
            empty = Path(tmp) / "seed.bin"
            seed = bytearray(DISK.read_bytes())
            sector = struct.unpack_from("<H", seed, 11)[0]
            fat = sector * struct.unpack_from("<H", seed, 14)[0]
            assert seed[16] == 1
            seed[fat+4:] = bytes(len(seed)-fat-4)
            root = fat + sector * struct.unpack_from("<H", seed, 22)[0]
            entries = struct.unpack_from("<H", seed, 17)[0]
            for index in range(entries):
                seed[root+32*index] = 0xe5
            empty.write_bytes(seed)
            for kind, option, profile, core in (("EXE", "", 1, None), ("RKS", "-STD", 2, None),
                    ("RKS", "-MON", None, "MONITOR2.MON"), ("RKS", "-RF", None, "RAMFOS.MON")):
                data = encode(Image(BODY, ENTRY, ENTRY), kind.lower())
                image, _ = trial_disk(data, kind, empty)
                disk = Path(tmp) / "H.bin"
                disk.write_bytes(image)
                trial(machine, files, disk, kind, option, profile, core)
    finally:
        machine.close()


if __name__ == "__main__":
    main()
