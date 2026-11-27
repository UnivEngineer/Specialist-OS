"""Привязка штатных исторических загрузчиков к точной нагрузке и адресу из каталога."""
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path


LABELS = {
    "entry_and_payload_verified":"вход и SHA", "entry_payload_verified":"вход и SHA",
    "entry_payload_mismatch":"тело изменилось", "checksum_rejected":"отказ суммы",
    "type_rejected":"отказ типа", "load_rejected":"отказ загрузки",
    "rom_read_and_g_entry":"R и G, SHA совпал", "rom_read_error":"ошибка ввода ROM",
    "rom_read_status_uncertain":"итог R не установлен", "monitor_read_not_completed":"R не вернулся",
    "g_entry_unconfirmed":"G без подтверждённого входа", "unconfirmed":"не подтверждено",
    "offset_entry_unrepresentable":"entry отличается от load", "gameplay_verified":"действие на уровне",
    "descriptor_verified":"ядро, тело и PC", "payload_verified_monitor_unconfirmed":"тело и PC, ядро расходится",
    "archive_body_and_entry_verified":"вход и SHA", "data_loaded_no_execution":"данные загружены",
}


def read(path):
    return json.loads(path.read_bytes()) if path.is_file() else {}


def anchor(name):
    return "game-"+hashlib.sha256(name.encode("utf-8")).hexdigest()[:20]


def load_history(out, files, ramfos):
    records = {r["file"].removeprefix("assets/games/"):r for r in files}
    history = defaultdict(list)
    by_container = defaultdict(set)
    by_descriptor = defaultdict(set)
    for name,record in records.items():
        if record["format"]=="cpu_i80":
            by_descriptor[(record["sha256"],record["payload_sha256"])].add(name)
        for route in record["routes"]:
            by_container[route["sha256"]].add(name)

    def add(name, loader, status, path, **properties):
        record = records[name]
        digest = properties.get("payload_sha256")
        if digest and record["payload_sha256"]!=digest:
            raise ValueError(f"Протокол {loader} относится к другому телу: {name}")
        if path and not (out/path).exists():
            raise ValueError(f"Не найдены свидетельства {loader}: {path}")
        history[name].append(dict(loader=loader,status=status,protocol=path,**properties))

    for name,row in ramfos.items():
        add(name,"RAM-диск ROM",row["status"],row["protocol"],rom_sha256=row["rom_sha256"])
    indices = {}
    for path,loader in (("native-monitor/catalog-index.json","Файл STD"),("native-monitor/tape-catalog.json","Лента R/G")):
        index = read(out/path)
        if not index:
            continue
        statuses = Counter()
        for digest,row in index.items():
            names = by_container[digest]
            if not names:
                raise ValueError(f"В реестре нет исторического RKS {digest}")
            folder = Path("native-monitor")/row["evidence"]
            status = row["classification"]
            if loader=="Файл STD" and row.get("entry_body_matches") and row["entry_pc"]==row["expected_entry"]:
                status = "entry_and_payload_verified"
            protocol = folder/"result.json"
            if not (out/protocol).is_file():
                protocol = folder/row["frames"]["after"]["file"] if "after" in row["frames"] else Path(path)
            for name in names:
                add(name,loader,status,protocol.as_posix(),container_sha256=digest,rom_sha256=row["rom_sha256"])
            statuses[status] += 1
        indices[loader] = dict(path=path,total=len(index),counts=dict(statuses))
    for path,loader in (("native-ramfos/rf-index.json","ROM для RF"),("native-ramfos/mx-index.json","Проба MX")):
        index = read(out/path)
        if not index:
            continue
        for row in index["results"]:
            names = {name for route in row["primary_profiles"] for name in route["aliases"]}
            for name in names:
                add(name,loader,row["native_launch"],"native-ramfos/"+row["result"],transport=row["transport"],
                    payload_sha256=row["payload_sha256"],rom_sha256=index["rom_sha256"])
        indices[loader] = dict(path=path,total=len(index["results"]),counts=index["launch_counts"])
    path = "native-fdc-run/results.json"
    index = read(out/path)
    if index:
        for row in index["cases"]:
            add(row["file"],"Дискета F6",row["outcome"],"native-fdc-run/"+row["protocol"],
                payload_sha256=row["payload_sha256"],odi_sha256=row["disk_sha256"],rom_sha256=index["rom_sha256"])
        indices["Дискета F6"] = dict(path=path,total=len(index["cases"]),counts=index["counts"])
    path = "native-monitor/budi-exe/index.json"
    index = read(out/path)
    for row in index.values():
        name = row["source"].removeprefix("assets/games/")
        if records[name]["sha256"]!=row["source_sha256"]:
            raise ValueError(f"Изменился входной EXE {name}")
        after = row.get("brick_after",(row.get("action_proof") or {}).get("after"))
        status = "gameplay_verified" if row.get("g_entry_confirmed") and after==0x48 else "unconfirmed"
        add(name,"EXE → лента",status,path,payload_sha256=row["body_sha256"],container_sha256=row["derived_rks_sha256"])
    path = "native-lik/index.json"
    index = read(out/path)
    if index:
        entries = index["cases"].values()
        for row in entries:
            name = row["file"].removeprefix("assets/games/")
            assert records[name]["sha256"]==row["sha256"] and row["body_loaded_after_i"] and row["lik_checksum_matches"],name
            add(name,"ЛИК",row["classification"],f"native-lik/{row['sha256']}/result.json",
                payload_sha256=row["body_sha256"],author_monitor=row["needs_author_monitor"])
        indices["ЛИК"] = dict(path=path,total=len(entries),counts=dict(Counter(row["classification"] for row in entries)))
    for dirname,loader in (("native-descriptors","Ядро CPU"),("native-descriptors-stock","Базовый MX"),
                           ("native-descriptors-catalog","CPU из реестра")):
        path = dirname+"/index.json"
        index = read(out/path)
        if not index:
            continue
        counts = Counter()
        for row in index["cases"]:
            names = by_descriptor.get((row["cpu_sha256"],row["i80_sha256"]),())
            for name in names:
                add(name,loader,row["outcome"],dirname+"/"+row["protocol"],payload_sha256=row["i80_sha256"],
                    descriptor_sha256=row["cpu_sha256"],monitor=row["monitor_name"],entry=row["entry"],load=row["load"])
                counts[row["outcome"]] += 1
        indices[loader] = dict(path=path,total=sum(counts.values()),counts=dict(counts),archive_size=len(index["cases"]))
    return history,indices


def history_table(files, history, indices, table):
    columns = (("STD / Monitor-2",("Файл STD","Лента R/G","EXE → лента")),("ЛИК",("ЛИК",)),
               ("Описатель CPU/I80",("Ядро CPU","CPU из реестра","Базовый MX")),("RAMFOS",("RAM-диск ROM","ROM для RF","Проба MX")),
               ("MX-DOS",("Дискета F6",)))
    rows = []
    for record in files:
        name = record["file"]
        entries = history.get(name,[])
        if not entries:
            continue
        cells = [f'<a id="{anchor(name)}"></a>{name}']
        for _,loaders in columns:
            matches = [row for loader in loaders for row in entries if row["loader"]==loader]
            links = [f"[{row['loader']} · {LABELS.get(row['status'],row['status'])}]({row['protocol']})" for row in matches]
            cells.append(" · ".join(dict.fromkeys(links)) or "—")
        rows.append(cells)
    return ["# Исторические загрузчики","",
            f"Таблица охватывает {len(rows)} имён игрового каталога. В ячейке указан результат отдельного запуска и ссылка на измерение.","",
            "В колонке «Файл STD» штатный загрузчик файлов Emu80 устанавливает PC после чтения RKS. В колонке «Лента R/G» команды исполняет собственный Monitor-2. Emu80 подаёт байты через перехватчик кассетного чтения. Для трёх современных EXE отдельно показан перенос точного тела в производный RKS.","",
            "«RAM-диск ROM» использует исходный заголовок ODI. «ROM для RF» сравнивает с RUN -RF то же тело через подходящий заголовок. «Проба MX» применяет RAMFOS как аппаратную среду для выбранных MX-нагрузок. Способ упаковки и сведения об ABI находятся в протоколе. «Дискета F6» проходит оригинальные RAMFOS, MX-DOS и каталог на исторической дискете.","",
            "В пробах ЛИК команда I= читает исходный именованный контейнер. Для отмеченных составителем игр запускается его MON-BAI2 командой J=8100. В пробах CPU/I80 «Ядро CPU» использует монитор из дескриптора, «Базовый MX» показывает исходное отображение памяти Emu80. Привязка требует совпадения и SHA-256 файла CPU, и SHA-256 его нагрузки I80.","",
            *table(("Путь","Проб","Индекс"),[(loader,i["total"],f"[машиночитаемый индекс]({i['path']})") for loader,i in indices.items()]),"",
            *table(("Файл",*(title for title,_ in columns)),rows)]
