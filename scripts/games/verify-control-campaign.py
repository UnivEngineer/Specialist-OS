"""Проверка сохранённых парных протоколов и цветных кадров без запуска Emu80."""

import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
from pathlib import Path
from uuid import uuid4

from campaign_guest import Image
from campaign_media import ROOT, sha
from PIL import ImageChops


ROOT_OUT = ROOT / "docs/compat/game-campaign/paired"
SDP = {"system-flash.bin.sdp": "01", "flash-disk.bin.sdp": None}
FRAME_FIELDS = {"ticks", "elapsed_ticks", "frame", "png_sha256"}
ARMS = ("idle-1", "idle-2", "input-1", "input-2")
INSTRUCTION_TICKS = 18


def load(path):
    return json.loads(path.read_text(encoding="utf-8"))


def require(condition, path, message):
    if not condition:
        raise ValueError(f"{path}: {message}")


def rgb(path):
    with Image.open(path) as source:
        return source.convert("RGB")


def pixel_difference(first, second):
    red, green, blue = ImageChops.difference(first, second).split()
    return ImageChops.lighter(ImageChops.lighter(red, green), blue).point([0] + [255] * 255)


def frame_rgb(directory, arm, phase):
    checkpoint = arm["checkpoints"][phase]
    return rgb(directory / arm["arm"] / checkpoint["frame"])


def verify_phase(directory, arms, phase, view, counts):
    samples = [arm["checkpoints"][phase] for arm in arms]
    elapsed = [sample["ticks"] - arm["checkpoints"]["before"]["ticks"]
               for arm, sample in zip(arms, samples, strict=True)]
    require(elapsed == view["elapsed_ticks"], directory, f"Такты {phase} расходятся со снимками")
    require(max(elapsed) - min(elapsed) == view["tick_spread"], directory, f"Разброс тактов {phase}")
    timed = all(sample["stopped_by_timer"] and not sample["exit"] for sample in samples)
    require(view["same_budget"] == (timed and view["tick_spread"] <= INSTRUCTION_TICKS),
            directory, f"Допуск времени {phase}")
    if "mask_sha256" not in view:
        return
    a1, a2, b1, b2 = (frame_rgb(directory, arm, phase) for arm in arms)
    before = frame_rgb(directory, arms[0], "before")
    diff_idle = pixel_difference(a1, a2)
    diff_input = pixel_difference(b1, b2)
    repeat = ImageChops.invert(ImageChops.lighter(diff_idle, diff_input))
    mask = ImageChops.darker(repeat, pixel_difference(a1, b1))
    autonomous = ImageChops.darker(ImageChops.invert(diff_idle), pixel_difference(before, a1))
    require(sha(mask.tobytes()) == view["mask_sha256"], directory, f"Маска {phase}")
    for field, difference in (("autonomous_pixels", autonomous), ("baseline_disagreement_pixels", diff_idle),
                              ("input_disagreement_pixels", diff_input), ("stable_delta_pixels", mask)):
        require(difference.histogram()[255] == view[field], directory, f"Счётчик {phase}/{field}")
    counts["rebuilt_differential"] += 1
    if "mask" in view:
        path = directory / view["mask"]
        with Image.open(path) as source:
            require(source.mode == "L" and source.tobytes() == mask.tobytes(), path, "PNG маски")
            require(list(source.getbbox()) == view["roi_exclusive"], path, "Область различий")
        counts["validated_mask"] += 1
    if "roi_strip" in view:
        path = directory / view["roi_strip"]
        crop = view.get("roi_strip_crop_exclusive", view["roi_exclusive"])
        width, height = crop[2] - crop[0], crop[3] - crop[1]
        strip = rgb(path)
        require(strip.size == (3 * width, height), path, "Размер полосы кадров")
        for index, frame in enumerate((before, a1, b1)):
            part = strip.crop((index * width, 0, (index + 1) * width, height))
            require(part.tobytes() == frame.crop(crop).tobytes(), path, f"Пиксели части {index + 1}")
        counts["validated_crop"] += 1


def verify_object(path, result, arms, protected, counts):
    proof = result["object_proof"]
    require(proof["game_object"] and proof["action"], path, "У объекта нет имени или действия")
    require(all(all(fields.values()) for fields in result["pre_match"].values()), path, "Подготовка расходится")
    checkpoints = {arm["arm"]: arm["checkpoints"] for arm in arms}
    matches = result.get("matching_arms", (("idle-1", "input-1"), ("idle-2", "input-2")))
    for phase, fields in result["pre_match"].items():
        reference = checkpoints["idle-1"][phase]
        observed = {name: frames[phase] for name, frames in checkpoints.items()}
        for name in (fields.keys() & reference.keys()) - FRAME_FIELDS:
            require(all(frame[name] == reference[name] for frame in observed.values()), path, f"Подготовка {phase}/{name}")
        if "ticks_by_pair" in fields:
            elapsed = {name: frame["ticks"] - checkpoints[name]["entry"]["ticks"] for name, frame in observed.items()}
            require(all(elapsed[idle] == elapsed[active] for idle, active in matches), path, f"Такты подготовки {phase}")
    phases = {sample["phase"] for values in proof.values() if isinstance(values, list)
              for sample in values if isinstance(sample, dict) and "phase" in sample}
    require(phases, path, "У объекта нет измеренной фазы")
    for phase in phases:
        view = result["phases"][phase]
        require(view["same_budget"] and view["stable_delta_pixels"] > 0, path, f"Не измерена разница {phase}")
        require(view["input_disagreement_pixels"] == view["baseline_disagreement_pixels"] == 0, path, f"Пары {phase} расходятся")
        states = [{name: value for name, value in cp[phase].items() if name not in FRAME_FIELDS} for cp in checkpoints.values()]
        require(states[0] == states[1] and states[2] == states[3], path, f"Игровое состояние {phase} расходится")
        counts["verified_object_phase" if protected else "historical_object_phase"] += 1
    plans = []
    for index, arm in enumerate(arms):
        events = [event for event in arm["events"] if "pressed" in event]
        require(events and all(event["pressed"] == (index >= 2) for event in events), path, "Клавиша подана в неверном плече")
        plans.append([{name: value for name, value in event.items() if name != "pressed"} for event in events])
    require(all(plan == plans[0] for plan in plans), path, "Интервалы ввода расходятся")
    counts["verified_game_object" if protected else "historical_game_object"] += 1


def verify_result(path):
    result = load(path)
    directory = path.parent / result["evidence"]
    arms = result["arms"]
    require(result["id"] == path.parent.name, path, "Идентификатор маршрута")
    if result["status"] != "infrastructure_error":
        require(tuple(arm["arm"] for arm in arms) == ARMS, path, "Порядок A/A/B/B")
    protected = len(arms) == 4 and all(arm.get("initial_flash_protection") == SDP for arm in arms)
    counts = Counter(result=1)
    counts["flash_protection_verified" if protected else "flash_protection_unconfirmed"] += 1
    if protected:
        require(result["context"].get("flash_protection") == SDP, path, "SDP в описании среды")
    windows = {(arm.get("pid"), arm.get("hwnd"), arm.get("port")) for arm in arms}
    if protected and len(windows) == 1 and all(next(iter(windows))):
        boots = [arm["boot_on_lease"] for arm in arms]
        require(boots == sorted(set(boots)), path, "Порядок холодных стартов в окне")
        counts["same_window_four_starts"] += 1
    for arm in arms:
        if arm.get("initial_media"):
            media = arm["initial_media"]
            require(media["flash-disk.bin"] == result["game_disk_sha256"], path, "Хеш H:")
            require(media["system-flash.bin"] == result["context"]["system_sha256"], path, "Хеш C:")
            if "floppy_seed_sha256" in result["context"]:
                require(media["floppy720-test.img"] == result["context"]["floppy_seed_sha256"], path, "Хеш A:")
            counts["initial_media_verified"] += 1
        for checkpoint in arm.get("checkpoints", {}).values():
            frame = directory / arm["arm"] / checkpoint["frame"]
            if "png_sha256" in checkpoint:
                require(sha(frame.read_bytes()) == checkpoint["png_sha256"], frame, "Хеш PNG")
            image = rgb(frame)
            require(list(image.size) == checkpoint["size"], frame, "Размер кадра")
            require(sha(image.tobytes()) == checkpoint["rgb_sha256"], frame, "Хеш RGB")
            counts["validated_png"] += 1
    for phase, view in result.get("phases", {}).items():
        verify_phase(directory, arms, phase, view, counts)
    archived = directory / "result.json"
    if protected or archived.is_file():
        require(load(archived) == result, archived, "Неизменяемая копия протокола расходится с текущей")
        counts["archived_result"] += 1
    if result["status"] == "object_verified":
        verify_object(path, result, arms, protected, counts)
    return counts


def verify_index(directory, counts, partial):
    path = directory / "index.json"
    index = load(path)
    require(index.get("flash_protection") == SDP, path, "Штатное состояние SDP в индексе")
    cases = index["cases"]
    require(len({row["id"] for row in cases}) == len(cases), path, "Повторяющийся маршрут")
    require(Counter(row["status"] for row in cases) == index["counts"], path, "Итоги индекса расходятся со строками")
    for row in cases:
        counts["index_" + row["selection"]] += 1
        if row["selection"] != "applicable" or not row.get("flash_protection_verified"):
            continue
        archive = directory / row["paired_archive"]
        result = load(archive)
        require(result["id"] == row["id"] == archive.parent.parent.name, archive, "Идентификатор в индексе")
        require(result["version"] == row["paired_version"], archive, "Версия в индексе")
        require(all(arm["initial_flash_protection"] == SDP for arm in result["arms"]), archive, "SDP в четырёх стартах")
        if not partial:
            require(load(directory / row["paired"]) == result, archive, "Индекс ссылается на прежнюю попытку")
        counts["index_protected"] += 1
        if row["status"] == result["status"]:
            counts["index_verified"] += 1
            counts["index_object_verified"] += result["status"] == "object_verified"
    if not partial:
        require(counts["index_applicable"] == counts["index_protected"] == counts["index_verified"], path, "Не все применимые пары проверены")
        require(not counts["index_deferred"], path, "Остались отложенные первичные пробы")
        require(not index["counts"].get("infrastructure_error"), path, "Остались ошибки инфраструктуры")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT_OUT, help="Каталог парных протоколов")
    parser.add_argument("--partial", action="store_true", help="Проверить кадры незавершённой очереди")
    args = parser.parse_args()
    counts = Counter()
    with ThreadPoolExecutor(max_workers=6) as pool:
        for result in pool.map(verify_result, sorted(args.out.glob("*/result.json"))):
            counts.update(result)
    notes = args.out / "semantic-notes.json"
    if notes.is_file():
        for observation in load(notes)["observations"]:
            for item in observation["frames"]:
                frame = args.out / item["path"]
                require(sha(frame.read_bytes()) == item["sha256"], frame, "Хеш исторического кадра")
                counts["verified_semantic_png"] += 1
    verify_index(args.out, counts, args.partial)
    summary = {"generated_utc": datetime.now(timezone.utc).isoformat(), **dict(counts)}
    data = json.dumps(summary, ensure_ascii=False, indent=2) + "\n"
    output = args.out / "validation.json"
    temporary = output.with_suffix(f".{uuid4().hex}.tmp")
    temporary.write_text(data, encoding="utf-8")
    temporary.replace(output)
    print(data, end="")


if __name__ == "__main__":
    main()
