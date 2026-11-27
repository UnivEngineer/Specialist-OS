"""Сверить архивные носители, заголовки RAMFOS, результаты его загрузчика и сохранённые кадры."""
import argparse
from collections import Counter
import csv
import json
from pathlib import Path
import shutil
import struct
import sys
import zipfile

from campaign_media import ROOT, sha

sys.path.insert(0,str(ROOT/"scripts/images"))
from executable import checksum
from game_inventory import GAMES, LAFAN, lafan_directory


def read(path):
    return json.loads(path.read_bytes())


def copy_checked(source, destination, expected):
    if sha(source.read_bytes())!=expected:
        raise ValueError(f"Хеш источника расходится с протоколом: {source}")
    if destination.is_file():
        if sha(destination.read_bytes())!=expected:
            raise ValueError(f"Хеш сохранённого эталона расходится с источником: {destination}")
    else:
        destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(source,destination)


def verify_ramfos(root, index, sources, archive, counts):
    seen = set()
    windows = set()
    for row in index["results"]:
        case = read(root/row["result"])
        name = case["file"]
        assert name==row["file"] and name not in seen,name
        seen.add(name)
        windows.add((case["pid"],case["hwnd"]))
        source = sources[name]
        body = (GAMES/name).read_bytes()
        disk = (archive/source["Disk"]).read_bytes()
        pos = int(source["DirectoryOffset"],0)
        header = disk[pos:pos+32]
        load,end,stored_sum = struct.unpack_from("<HHH",header,24)
        assert header.hex()==source["HeaderHex"]==case["header_hex"],name
        assert header[3:15].hex()==case["listing"]["printed_hex"] and header[:30].hex()==case["listing"]["selected_header_hex"],name
        assert len(body)==end-load+1 and case["load"]==load and case["end"]==end,name
        assert case["payload_sha256"]==source["PayloadSHA256"]==sha(body),name
        assert case["odi_sha256"]==source["DiskSHA256"]==sha(disk),name
        assert case["calculated_checksum"]==checksum(body) and case["archive_checksum"]==stored_sum,name
        assert sha((header[:30]+body+b"\x00\x00\x08").ljust(8*65536,b"\x00"))==case["ram_disk_sha256"],name
        status = case["status"]
        assert row["status"]==status,name
        if header[12:15] not in (b"EXE",b"COM"):
            assert status=="type_rejected" and not case["type_decision"]["flags"]["Z"],name
            assert case["stop"]["registers"]["pc"]==0xfa3d,name
        elif stored_sum!=checksum(body):
            assert status=="checksum_rejected" and case["loaded_payload_sha256"]==sha(body),name
            assert not case["checksum_decision"]["flags"]["Z"] and case["stop"]["registers"]["pc"]==0xfe8e,name
        else:
            assert status=="entry_and_payload_verified" and case["entry_payload_sha256"]==sha(body),name
            assert case["stop"]["registers"]["pc"]==load==case["returns"][-1]["stack_target"],name
        for frame in case["frames"].values():
            file = (root/row["result"]).parent/frame["file"]
            assert sha(file.read_bytes())==frame["sha256"],file
            counts["native_ramfos_png"] += 1
        counts[status] += 1
    assert seen==sources.keys()
    counts["ramfos_windows"] = len(windows)


def verify_lik(fixture, counts):
    archive = fixture/"lik/lik_spc1.zip"
    if not archive.is_file():
        return
    assert sha(archive.read_bytes())=="766be678e8a0e3467ba2d27ffcc12a641d7d7bc8227f9e91daed124babffe592"
    with zipfile.ZipFile(archive) as source:
        for name in ("MON-BAI2.RKS","LIK3.RSS"):
            assert (fixture/"lik"/name.lower()).read_bytes()==source.read("SYSTEM/"+name)
        listing = GAMES/"std/manuals/games_lik_rks/LIST.TXT"
        assert listing.read_bytes()==source.read("GAMES/LIST.TXT")
        with (GAMES/"catalog.csv").open(encoding="utf-8-sig",newline="") as stream:
            for row in csv.DictReader(stream):
                if "/games_lik_rks/" in row["Origin"].replace("\\","/"):
                    basename = row["Origin"].split("/")[-1].upper()
                    assert (GAMES/row["File"]).read_bytes()==source.read("GAMES/"+basename),row["File"]
                    counts["lik_archive_names"] += 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out",type=Path,default=ROOT/"docs/compat/game-campaign")
    parser.add_argument("--archive",type=Path,help="Сохранить пять исходных ODI в fixtures/native/odi")
    parser.add_argument("--platforms",type=Path,help="Сохранить исходные конфигурации и ПЗУ spec и specmx из dist")
    args = parser.parse_args()
    out = args.out.resolve()
    fixture = out/"fixtures/native"
    archive = fixture/"odi"
    counts = Counter()
    with LAFAN.open(encoding="utf-8-sig",newline="") as stream:
        sources = {row["File"]:row for row in csv.DictReader(stream)}
    disks = {row["Disk"]:row["DiskSHA256"] for row in sources.values()}
    if args.archive:
        for name,digest in disks.items():
            copy_checked(args.archive/name,archive/name,digest)
    assert lafan_directory(archive)==LAFAN.read_text("utf-8")
    counts["odi_images"] = len(disks)
    report = read(out/"results.json")
    index_path = out/report["native_ramfos_index"]
    index = read(index_path)
    assert index["complete"]
    verify_ramfos(index_path.parent,index,sources,archive,counts)
    pilot = read(out/"native-monitor/index.json")
    ramfos = read(index_path.parent/index["results"][0]["result"])
    platform_files = {"spec/spec_m2.rom":pilot["rom_sha256"],"spec/spec.conf":pilot["config_sha256"],
                      "spec/spec.html":pilot["help_sha256"],"specmx/spmx.rom":ramfos["rom_sha256"]}
    if args.platforms:
        for name in ("spec/spec_wx.inc","specmx/spmx.conf","specmx/spmx.html"):
            platform_files[name] = sha((args.platforms/name).read_bytes())
        for name,digest in platform_files.items():
            copy_checked(args.platforms/name,fixture/"dist"/name,digest)
    for name,digest in platform_files.items():
        assert sha((fixture/"dist"/name).read_bytes())==digest,name
    for case in report["native_odi"]:
        assert sha((archive/case["disk"]).read_bytes()[:5120])==case["sectors_sha256"]==case["memory_sha256"]
        assert case["disk_sha256"]==case["final_disk_sha256"]==disks[case["disk"]]
        assert case["mismatching_bytes"]==0
        counts["firmware_f6"] += 1
    verify_lik(fixture,counts)
    result = dict(counts, fixture=fixture.relative_to(out).as_posix(),
                  files={p.relative_to(fixture).as_posix():sha(p.read_bytes()) for p in sorted(fixture.rglob("*")) if p.is_file()})
    (out/"native-verification.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(counts))


if __name__=="__main__":
    main()
