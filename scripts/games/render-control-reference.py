"""Собрать читаемый справочник игровых проб из карточек и манифестов H:."""
import argparse
import csv
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "images"))
from game_inventory import GAMES, ROOT, apply_lafan_headers, manifests, routes, source_record


GROUPS = {
    "families": "Семейства",
    "std": "Игры «Специалиста»",
    "mx": "Игры из подборки MX",
    "unknown": "Остальные архивные варианты",
}
ARCHIVE_ROUTES = {
    "original": "исходный контейнер",
    "after_tape_header": "RKS после удаления ленточного префикса",
    "repacked": "проверочный контейнер с игровым телом",
}


def command(name, profiles, explicit_flags):
    flags = [explicit_flags.get(profile, "" if profile == "NATIVE" else "-" + profile) for profile in profiles]
    return ", ".join(f"`RUN H:{name}{(' ' + arguments) if arguments else ''}`" for arguments in flags)


def file_line(path, record, disk_entries, profiles, explicit_flags):
    display = f"[{path}]({path})"
    if record.get("blocked"):
        return f"- {display} — {record['blocked']}."
    if record["load"] is None:
        return f"- {display} — адрес загрузки требуется установить по архивному описанию."
    mode = record["catalog"]["RunMode"]
    if mode in profiles and mode not in ("AUTO", "NATIVE"):
        compatible = {"STD", "MON"} if mode in ("STD", "MON") else {mode}
        profiles = [mode, *(profile for profile in profiles if profile != mode
                            and (profile in compatible or profile in explicit_flags))]
    choices = []
    for route in routes(record, disk_entries):
        for disk in route["disk_names"]:
            manifest, name = disk["manifest"], disk["name"]
            choices.append(f"{command(name, profiles, explicit_flags)} на [{manifest}](../{manifest})")
        if route["recipe"] == "archive_as_named":
            name = "ARCHIVE." + route["kind"].upper()
            choices.append(f"{command(name, profiles, explicit_flags)} для проверки исходного архива")
        elif not route["disk_names"]:
            name = "PROBE." + route["kind"].upper()
            description = ARCHIVE_ROUTES[route["recipe"]]
            if route["recipe"] == "original" and record.get("container_sum_ok") is False:
                description += " с несовпадающей контрольной суммой"
            choices.append(f"{command(name, profiles, explicit_flags)} ({description})")
    return f"- {display} — " + ". ".join(choices) + "."


def instructions(label, keys, text):
    sequence = " → ".join(f"`{key}`" for key in keys)
    return f"**{label}.** " + (f"{sequence}. " if sequence else "") + text


def render(cards, records, disks):
    count = len(records)
    actions = sum(len(card["files"]) for card in cards if card["action_keys"])
    output = [
        "# Справочник игровых проб",
        "",
        "В карточках указаны логические имена клавиш Emu80 MCP. Выполняйте действия на указанной сцене и после отпускания предыдущей клавиши. Условия входа, раскладки и наблюдения приведены в [описании игровых проб](playbook.md).",
        "",
        "Команды с дисковым именем взяты из манифестов H:. В остальных примерах `PROBE` обозначает имя тестового контейнера, а `ARCHIVE` — проверку архивной упаковки. Для переупаковки используют диапазон загрузки и точку входа исходного файла. `RUN` без ключа режима в карточке NATIVE использует флаг MAIN в заголовке EXE.",
        "",
        f"В справочнике {count} вариантов. У {actions} из них описана последовательность действий. Остальные карточки содержат настройки, выход, назначение архивного материала или наблюдавшееся препятствие. Степень проверки эффекта указана в основании карточки.",
        "",
        "Фраза «другой работник воспроизвёл» означает, что второй работник получил тот же игровой эффект с чистого старта. Повторы одного работника и сравнение ввода с контрольным ожиданием указаны отдельно, с привязкой к проверенному файлу и режиму.",
        "",
        "Справочник собирается командой `python scripts/games/render-control-reference.py` из [карточек](controls.json) и [каталога](catalog.csv).",
        "",
        " · ".join(f"[{title}](#{title.lower().replace(' ', '-').replace('«', '').replace('»', '')})" for title in GROUPS.values()),
        "",
    ]
    for group, title in GROUPS.items():
        output.extend((f"## {title}", ""))
        selected = [card for card in cards if card["files"][0].split("/", 1)[0] == group]
        for card in sorted(selected, key=lambda card: (Path(card["files"][0]).name.casefold(), card["files"][0])):
            names = card["files"]
            output.extend((f"### {Path(names[0]).name}", ""))
            for path in names:
                record = records[path]
                output.append(file_line(path, record, disks.get(record["file"].lower(), []),
                                        card["profiles"], card.get("run_flags", {})))
            output.extend((
                "",
                instructions("Начало", card["start_keys"], card["start_when"]),
                "",
                instructions("Действие", card["action_keys"], card["action_when"]),
                "",
                "**Наблюдение.** " + card["expect"],
                "",
                instructions("Выход", card["exit_keys"], card["exit_when"]),
                "",
                "**Основание.** " + card["source"],
                "",
            ))
    return "\n".join(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Сверить опубликованный справочник с карточками")
    args = parser.parse_args()
    with (GAMES / "catalog.csv").open(encoding="utf-8-sig", newline="") as stream:
        rows = {row["File"]: row for row in csv.DictReader(stream)}
    cards = json.loads((GAMES / "controls.json").read_text(encoding="utf-8"))["cards"]
    names = [path for card in cards for path in card["files"]]
    if len(names) != len(set(names)):
        raise ValueError("Файл повторяется в нескольких карточках")
    missing = sorted(rows.keys() - set(names))
    if missing:
        raise ValueError("В справочнике отсутствуют " + ", ".join(missing))
    records = {path: source_record(rows[path]) for path in names}
    apply_lafan_headers(records.values())
    result = render(cards, records, manifests())
    target = GAMES / "controls-reference.md"
    if args.check:
        if not target.exists() or target.read_text(encoding="utf-8") != result:
            parser.error(f"Обновите справочник {target.relative_to(ROOT)}")
    else:
        target.write_text(result, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
