"""Сверить штатную пробу МХ-ДОС с исходными ODI и сохранёнными снимками."""

import argparse
from collections import Counter, defaultdict
import json
import os
from pathlib import Path
import runpy
import sys

from campaign_media import ROOT, sha
from lafan_fdc import CATALOG, GEOMETRY, TEMPLATE, checked_disks, load_template, state_pc
from lafan_native import ROM_SHA256

sys.path.insert(0, str(ROOT / "scripts/images"))
from executable import checksum


RUNNER = runpy.run_path(str(Path(__file__).with_name("check-lafan-mxdos.py")))


def load(path):
    return json.loads(path.read_bytes())


def require(condition, path, message):
    if not condition:
        raise ValueError(f"{path}: {message}")


def verify_artifacts(path, case, disk, build, counts):
    proof = load(path)
    require(proof["disk_sha256"] == sha(disk) == case["disk_sha256"], path, "Хеш исходной ODI")
    require(proof["emulator_sha256"] == build["executableSha256"], path, "Сборка Emu80")
    require(proof["rom_sha256"] == ROM_SHA256, path, "Архивный ROM")
    require(proof["header_hex"] == case["header"].hex(), path, "Полный исходный заголовок")
    require(proof["payload_sha256"] == case["payload_sha256"], path, "Исходное тело")
    for record in proof.get("frames", {}).values():
        frame = path.parent / record["file"]
        content = frame.read_bytes()
        require(content.startswith(b"\x89PNG\r\n\x1a\n") and sha(content) == record["sha256"], frame, "Подпись PNG")
        counts["validated_png"] += 1
    boot = proof.get("boot")
    if boot:
        resident = (path.parent / boot["file"]).read_bytes()
        require(len(resident) == 5120 and sha(resident) == boot["body_sha256"], path, "Снимок резидента")
        require(sha(disk[:5120]) == boot["expected_sha256"], path, "Исходный первый трек")
        differences = sum(a != b for a, b in zip(resident, disk[:5120], strict=True))
        require(differences == boot["mismatching_bytes"], path, "Различия в резиденте")
        counts["validated_resident"] += 1
    entry = proof.get("entry")
    if entry:
        raw = (path.parent / entry["file"]).read_bytes()
        require(len(raw) == entry["bytes"] == len(case["payload"]), path, "Размер снимка тела")
        require(sha(raw) == entry["actual_sha256"], path, "Снимок тела")
        differences = sum(a != b for a, b in zip(raw, case["payload"], strict=True))
        require(differences == entry["mismatching_bytes"], path, "Различия в теле")
        counts["validated_payload"] += 1
    tested = proof.get("checksum", {})
    if "copied_body_file" in tested:
        raw = (path.parent / tested["copied_body_file"]).read_bytes()
        require(len(raw) == len(case["payload"]) and sha(raw) == tested["copied_body_sha256"], path, "Копия тела перед отказом")
        require((raw == case["payload"]) == tested["copied_body_matches"], path, "Совпадение копии тела")
        require(checksum(raw) == tested["actual"], path, "Сумма снята у другой копии тела")
        counts["validated_rejected_body"] += 1
    return proof


def verify_decision(path, proof, case, disk, counts):
    boot = proof["boot"]
    require(state_pc(boot["guard"]) == RUNNER["BOOT_STATUS"], path, "Адрес проверки ВГ93")
    require((boot["status"], boot["next_sector"], boot["mismatching_bytes"], boot["branch_pc"]) == (1, 6, 0, 0xE400),
            path, "Штатный вход F6 в МХ-ДОС")
    selected = proof["selected"]
    require([item["odi_offset"] for item in selected] == list(case["path"]), path, "Путь по файловым таблицам")
    for step in selected:
        start = step["odi_offset"]
        require(bytes.fromhex(step["observed_header"])[:30] == disk[start:start + 30], path, "Выделенный заголовок D252h")
        counts["validated_selected_header"] += 1
    require(proof["keys"][-1] == {"key": "f6", "pc": RUNNER["KEY_ACCEPTED"], "code": 5, "prompt": "mxdos"},
            path, "Принятый код F6")
    require(proof["run_prompt_pc"] == RUNNER["RUN_CONFIRM"], path, "Штатное подтверждение ВК")
    decision = proof["type_decision"]
    accepted = case["type"] in ("EXE", "COM")
    require(decision["pc"] == RUNNER["TYPE_DECISION"] and decision["accepted"] == accepted,
            path, "Фактическая проверка типа")
    require(decision["type"] == case["type"] and decision["header"] == case["header"][:30].hex(),
            path, "Тип и заголовок в момент проверки")
    tested = proof.get("checksum")
    if tested:
        require(accepted and tested["pc"] == RUNNER["ROM_COMPARE"], path, "Адрес штатной сверки суммы")
        branch = RUNNER["CHECKSUM_BRANCH"]
        require(tested["return_pc"] == tested["branch_pc"] == branch, path, "Условный переход после ROM")
        require(tested["declared"] == int.from_bytes(case["header"][28:30], "little"), path, "Сумма в заголовке")
        require(tested["accepted"] == (tested["declared"] == tested["actual"]), path, "Решение ROM о сумме")
        counts["verified_checksum_decision"] += 1
    if accepted and proof["outcome"] in ("entry_payload_verified", "entry_payload_mismatch"):
        trampoline = proof["trampoline"]
        require(trampoline == {"pc": RUNNER["RUN_TRAMPOLINE"], "target": case["load"], "opcode": 0xE9},
                path, "Штатная команда PCHL")
        require(state_pc(proof["entry"]["state"]) == case["load"], path, "Фактический вход")
        exact = proof["entry"]["actual_sha256"] == case["payload_sha256"] and proof["entry"]["mismatching_bytes"] == 0
        require(exact == (proof["outcome"] == "entry_payload_verified"), path, "Достоверность результата загрузки")
        require("first-state" in proof["frames"] and proof["first_state"], path, "Первый снятый игровой этап")
    elif not accepted:
        require(proof["outcome"] == "type_rejected" and proof["branch_pc"] == RUNNER["DOS_REJECT"], path, "Ветка отказа")
        require("rejected" in proof["frames"], path, "Кадр штатного отказа")
    elif proof["outcome"] == "checksum_rejected":
        require(tested and not tested["accepted"] and tested["destination_pc"] == RUNNER["RAMFOS_ENTRY"], path,
                "Штатный возврат при несовпавшей сумме")
        require("checksum-rejected" in proof["frames"], path, "Кадр возврата")
    else:
        require("branch_pc" in proof and "load-obstacle" in proof["frames"], path, "Точка остановки загрузки")
    counts["verified_type_decision"] += 1


def with_checksum_supplement(index, out, source):
    supplement = load(source)
    fields = ("build", "profile", "rom_sha256", "geometry", "catalog_sha256", "disks", "config_sha256", "options_sha256")
    require(all(index[name] == supplement[name] for name in fields), source, "Другие входы дополнительного протокола")
    replacements = {(row["disk"], row["offset"]): row for row in supplement["cases"] if row["outcome"] == "checksum_rejected"}
    rows, changed = [], []
    identity = ("file", "disk", "disk_sha256", "offset", "header_hex", "type", "load", "end", "payload_sha256")
    for old in index["cases"]:
        key = old["disk"], old["offset"]
        new = replacements.pop(key, None)
        if new:
            require(all(old[name] == new[name] for name in identity), source, "Другое тело или заголовок дополнительной пробы")
            require(old["outcome"] in ("load_unconfirmed", "checksum_rejected"), source, "Расходятся исходные выводы о загрузке")
            links = [Path(os.path.relpath(source.parent / name, out)).as_posix() for name in new["attempts"]]
            chosen = Path(os.path.relpath(source.parent / new["protocol"], out)).as_posix()
            rows.append(dict(old, outcome=new["outcome"], protocol=chosen, attempts=list(dict.fromkeys(old["attempts"] + links))))
            changed.append(chosen)
        else:
            rows.append(old)
    require(changed and not replacements, source, "Дополнительный отказ отсутствует в полном индексе")
    return dict(index, cases=rows, counts=dict(Counter(row["outcome"] for row in rows)),
                checksum_supplement=dict(index=Path(os.path.relpath(source, out)).as_posix(),
                                         runner_sha256=supplement["runner_sha256"], protocols=changed))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, help="Каталог с пятью исходными ODI")
    parser.add_argument("--template", type=Path, default=TEMPLATE)
    parser.add_argument("--out", type=Path, default=RUNNER["OUT"])
    parser.add_argument("--attempt", help="Имя каталога attempt-* для проверки исторического индекса")
    parser.add_argument("--subset", action="store_true", help="Разрешить индекс выборочного прогона")
    parser.add_argument("--checksum-supplement", type=Path,
                        help="Уточнить отказ по сумме отдельным results.json с теми же входами")
    args = parser.parse_args()
    build = load_template(args.template)
    require(sha((args.template / "specmx/spmx.rom").read_bytes()) == ROM_SHA256, args.template, "Архивный ROM")
    disks = checked_disks(args.archive)
    sources = {(case["disk"], case["offset"]): case for case in RUNNER["source_cases"](disks)}
    path = args.out / args.attempt / "index.json" if args.attempt else args.out / "results.json"
    index = load(path)
    if args.checksum_supplement:
        index = with_checksum_supplement(index, args.out, args.checksum_supplement.resolve())
    require(index["build"] == build and index["rom_sha256"] == ROM_SHA256, path, "Сборка и ROM в индексе")
    require(index["catalog_sha256"] == sha(CATALOG.read_bytes()) and index["geometry"] == list(GEOMETRY), path, "Реестр и геометрия")
    require(index["disks"] == {name: sha(content) for name, content in disks.items()}, path, "Хеши пяти исходных ODI")
    keys = {(row["disk"], row["offset"]) for row in index["cases"]}
    require(len(keys) == len(index["cases"]), path, "В индексе повторяются заголовки")
    require(keys <= sources.keys() if args.subset else keys == sources.keys(), path, "Покрытие исходного каталога")
    outcomes = Counter(row["outcome"] for row in index["cases"])
    require(outcomes == index["counts"], path, "Итоги индекса расходятся")
    counts, types = Counter(), defaultdict(Counter)
    checksum_mismatches = []
    for row in index["cases"]:
        case = sources[row["disk"], row["offset"]]
        require(row["protocol"] in row["attempts"], path, "Выбранный протокол не в списке попыток")
        proofs = {}
        for relative in row["attempts"]:
            report = args.out / relative
            proofs[relative] = verify_artifacts(report, case, disks[case["disk"]], build, counts)
        proof = proofs[row["protocol"]]
        require(all(proof[name] == row[name] for name in ("disk", "offset", "type", "payload_sha256", "outcome")),
                path, "Строка индекса расходится с протоколом")
        verify_decision(args.out / row["protocol"], proof, case, disks[case["disk"]], counts)
        if row["type"] in ("EXE", "COM"):
            declared = int.from_bytes(case["header"][28:30], "little")
            calculated = checksum(case["payload"])
            if declared != calculated:
                checksum_mismatches.append(dict(file=row["file"], header_sum=declared, body_sum=calculated))
            else:
                counts["supported_type_checksum_matches"] += 1
        types[row["type"]][row["outcome"]] += 1
        counts["verified_case"] += 1
    if args.checksum_supplement:
        path = args.out / "results.json"
        RUNNER["write_json"](path, index)
    summary = dict(index=path.as_posix(), cases=len(keys), counts=counts,
                   checksum_mismatches=checksum_mismatches,
                   outcomes={kind: statuses for kind, statuses in sorted(types.items())})
    RUNNER["write_json"](path.with_name("validation.json"), summary)
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
