"""Собрать справочник результатов из завершённых проб, не запуская эмулятор."""
import argparse
import array
from collections import Counter, defaultdict
import hashlib
import json
import math
import os
from pathlib import Path
from statistics import median
import uuid
import wave

from campaign_media import ROOT
from campaign_history import anchor, history_table, load_history


def read(path):
    return json.loads(path.read_text("utf-8"))


def atomic_text(path, body):
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_suffix("."+uuid.uuid4().hex+".tmp")
    temp.write_text(body,encoding="utf-8")
    temp.replace(path)


def save(path, lines):
    atomic_text(path,"\n".join(lines)+"\n")


def document(path, obj):
    atomic_text(path,json.dumps(obj, ensure_ascii=False, indent=2)+"\n")


def relative(path, directory):
    try:
        return Path(os.path.relpath(path,directory)).as_posix()
    except ValueError:
        return Path(path).resolve().as_posix()


def ramfos_catalog(out, records):
    directory = out/"native-ramfos"
    for result in sorted(directory.glob("attempt-*/index.json"), key=lambda p:p.stat().st_mtime_ns, reverse=True):
        index = read(result)
        if not index["complete"]:
            continue
        rows = {}
        for item in index["results"]:
            path = result.parent/item["result"]
            trial = read(path)
            name = item["file"]
            source = records["assets/games/"+name]
            if (name in rows or trial["file"]!=name or item["status"]!=trial["status"]
                    or trial["payload_sha256"]!=source["payload_sha256"]):
                raise ValueError(f"Расходятся исходник и протокол RAMFOS: {path}")
            rows[name] = dict(status=trial["status"], protocol=relative(path,out), load=trial["load"],
                              rom_sha256=trial["rom_sha256"], emulator_sha256=trial["emulator_sha256"],
                              odi_sha256=trial["odi_sha256"], header_hex=trial["header_hex"])
        return rows,relative(result,out)
    return {},None


def extra_controls(out):
    directory = out/"controls-extra"
    index = directory/"evidence.json"
    items = read(index)["games"] if index.is_file() else {}
    proofs = {}
    for row in items.values():
        if not row["independent_reproduction"]:
            continue
        workers = {}
        for person,observation in row["workers"].items():
            path = directory/observation["result"]
            raw = path.read_bytes()
            if hashlib.sha256(raw).hexdigest()!=observation["evidence_sha256"]:
                raise ValueError(f"Изменился протокол игрового действия: {path}")
            case = json.loads(raw)
            if (case["worker"]!=person or case["payload_sha256"]!=row["payload_sha256"]
                    or any(not arm["control_file"] for arm in case["arms"])):
                raise ValueError(f"Не подтверждён повтор игрового действия: {path}")
            workers[person] = [relative(path,out)]
        example = case["arms"][0]
        proofs[(row["container_sha256"],row["profile"])] = dict(workers=workers,object=row["strategy"],
               payload_sha256=row["payload_sha256"],media=example["initial_media"],
               protection=example["initial_flash_protection"],emulator=example["emulator_sha256"])
    return proofs


def table(headers, rows):
    def escape(value):
        return str(value).replace("|", "\\|").replace("\n"," ")
    rows = [list(map(escape, row)) for row in [headers, *rows]]
    widths = [max(3,*map(len, column)) for column in zip(*rows)]
    rows.insert(1, ["-"*width for width in widths])
    return ["| " + " | ".join(value.ljust(width) for value,width in zip(row,widths)) + " |" for row in rows]


def audio_stats(path):
    with wave.open(str(path)) as w:
        if w.getsampwidth() != 2 or w.getnchannels() != 1:
            raise ValueError("Ожидался одноканальный PCM16: " + str(path))
        rate = w.getframerate()
        samples = array.array("h", w.readframes(w.getnframes()))
    low, high = min(samples, default=0), max(samples, default=0)
    n = len(samples)
    mean = sum(samples)/max(1,n)
    rms = math.sqrt(sum((s-mean)**2 for s in samples)/max(1,n))
    tones = []
    active, windows = 0, 0
    length = max(1, rate//40)
    for pos in range(0,n,length):
        window = samples[pos:pos+length]
        if len(window)<length:
            continue
        windows += 1
        lo,hi = min(window),max(window)
        if lo==hi:
            continue
        active += 1
        center = (lo+hi)/2
        rising = [i for i in range(1,len(window)) if window[i-1]<center<=window[i]]
        if len(rising)>=5:
            periods = [j-i for i,j in zip(rising,rising[1:])]
            typical = median(periods)
            if sum(abs(t-typical)<=max(1,typical*.08) for t in periods)>=len(periods)*.85:
                tones.append(round(rate/typical))
    return dict(samples=n, duration_ms=round(n*1000/rate, 1), rate=rate, minimum=low, maximum=high,
                frames_25ms=windows, active_frames=active, regular_tone_frames=len(tones),
                regular_tone_hz=[min(tones),max(tones)] if tones else None,
                ac_rms=round(rms,1), at_int16_limit=sum(s in (-32768,32767) for s in samples),
                transitions=sum(a!=b for a,b in zip(samples,samples[1:])))


def verify_review(out, result, review):
    if not review:
        return None
    evidence = out/"trials"/result["id"]/review["evidence"]
    if review["evidence"]!=result["evidence"]:
        old = read(evidence/"observation.json")
        if (any(old.get(k)!=result.get(k) for k in ("id","profile","container_sha256","payload_sha256","game_disk_sha256","emulator_sha256"))
                or old["context"]["system_sha256"]!=result["context"]["system_sha256"]
                or old.get("boot_on_lease",1)>1 and old.get("initial_flash_protection")!=result["context"].get("flash_protection")):
            return None
    digests = review["frame_sha256"]
    if not digests:
        return None
    for name, expected in digests.items():
        frame = evidence / (name+".png")
        if not frame.exists() or hashlib.sha256(frame.read_bytes()).hexdigest()!=expected:
            raise ValueError(f"Не совпадает хеш кадра оценки {frame}")
    # При повторе могут измениться фаза анимации и состав экрана. Оценка
    # прежнего прогона остаётся доступна по ссылке на его собственные кадры.
    return dict(review, current=review["evidence"] == result["evidence"])


def adskok_trace(out):
    for folder in sorted((out/"adskok-trace").glob("attempt-*"), key=lambda p:p.stat().st_mtime, reverse=True):
        fixed = folder/"repaired/result.json"
        previous = folder/"previous/result.json"
        if fixed.is_file() and previous.is_file():
            tested,regression = read(fixed),read(previous)
            player = tested.get("player_sprite")
            if (player and player["after"][0]>player["before"][0] and tested.get("control_file")
                    and regression.get("game_stage")=="draw_attempts_return_unreached"):
                return {"archive":relative(folder/"archive/result.json",out), "previous":relative(previous,out),
                        "released":relative(fixed,out), "sha256":tested["container_sha256"],
                        "system_sha256":tested["initial_media"]["system-flash.bin"]}
        paths = {label:folder/label/"result.json" for label in ("archive","first-patch","repaired")}
        if not all(path.is_file() for path in paths.values()):
            continue
        observed = {label:read(path) for label,path in paths.items()}
        if ([r.get("game_stage") for r in observed.values()] ==
                ["title_input_unreached","direction_flag_written","draw_attempts_return_unreached"]
                and all(r.get("control_file") for r in observed.values())):
            return {label:path.relative_to(out).as_posix() for label,path in paths.items()}
    return {}


def defect(record, trace):
    notes = []
    if record.get("container_defect"):
        notes.append("Дефект контейнера RKS. " + record["container_defect"])
    if record["catalog"]["Status"]=="broken":
        notes.append(record["catalog"]["Notes"])
    if record["file"].endswith("/adskok/originals/Adskok.rks.bin"):
        notes.append("Счётчик очистки 0A00h затрагивает лишнюю память")
        if trace:
            notes.append("Архив попал в Monitor-2 до первого опроса пробела. " + f"[Трасса]({trace['archive']})")
    if trace and record["file"].endswith("/adskok/repaired/Adskok.rks"):
        archived = trace.get("previous") or trace["repaired"]
        notes.append("Прежний выпуск останавливался на проверке операнда в 7252h. Оригинальный CALL сохранён в исправленном выпуске. "
                     f"[Прежний выпуск]({archived})")
    if record.get("directory",{}).get("checksum_ok") is False:
        notes.append("Сумма в каталоге ODI расходится с содержимым секторов")
    return notes


def route_name(route):
    label = {"archive_as_named":"архив", "original":"оригинал", "repacked":"упаковка",
             "manifest":"образ", "manifest_auto":"образ", "after_tape_header":"после заголовка"}[route["recipe"]]
    return route["kind"].upper() + " " + label


def paired_trial(out, result, reviews):
    folder = out / "paired" / result["id"]
    published = folder / "result.json"
    if not published.is_file():
        return None
    raw = published.read_bytes()
    pair = json.loads(raw)
    a, b = result["context"], pair["context"]
    material = ("system_sha256","disk_seed_sha256","kernels","floppy_seed_sha256","profile_sha256")
    if (any(pair.get(k) != result.get(k) for k in ("profile","container_sha256","payload_sha256","game_disk_sha256","emulator_sha256"))
            or any(a.get(k) != b[k] for k in material if k in b)
            or any(a["build_inputs"].get(k) != v for k,v in b["build_inputs"].items())):
        return None
    if a.get("flash_protection") and any(arm.get("initial_flash_protection")!=a["flash_protection"]
            for arm in pair.get("arms",[])):
        return None
    review = reviews.get(result["id"])
    if review:
        roi = ROOT / review["roi"].replace("\\","/")
        archive = roi.parent / "result.json"
        current = review["result_sha256"]==hashlib.sha256(raw).hexdigest() or (
            roi.parent.name==pair["evidence"] and archive.is_file()
            and hashlib.sha256(archive.read_bytes()).hexdigest()==review["result_sha256"])
        if current:
            if not roi.is_file() or hashlib.sha256(roi.read_bytes()).hexdigest()!=review["roi_sha256"]:
                raise ValueError(f"Не совпадает ROI парной оценки {roi}")
            review = dict(review, roi=os.path.relpath(roi,out).replace("\\","/"))
        else:
            review = None
    proof = pair.get("object_proof") or {}
    samples = [row.get("phase") for row in proof.get("samples",[]) + proof.get("flight",[]) if isinstance(row,dict)]
    phases = list(dict.fromkeys(samples + pair.get("visual_phases",[])))
    phase = next((pair["phases"][name] for name in phases if pair["phases"].get(name,{}).get("roi_strip")), {})
    roi = folder / pair["evidence"] / phase["roi_strip"] if phase else None
    archive = folder / pair["evidence"] / "result.json"
    return dict(status=pair["status"], protocol=(archive if archive.is_file() else published).relative_to(out).as_posix(),
                object=proof or None, keys=pair["scenario_keys"], action=proof.get("action") or (pair.get("probe_plan") or {}).get("key"), review=review,
                roi=roi.relative_to(out).as_posix() if roi else None)


def stage(t):
    if t.get("entry_check"):
        return "вход, изменено тело"
    return {"entry_and_payload_verified":"вход", "rejected_header_or_range":"отказ заголовка/адреса",
            "rejected_body":"отказ тела"}.get(t["launch"],"вход не подтверждён")


def action(t):
    p = t.get("paired") or {}
    if t["input"]=="verified" or t.get("independent") or p.get("status")=="object_verified" or t.get("adskok_gameplay"):
        return "действие на уровне"
    review = p.get("review") or {}
    if review.get("input_action_confirmed"):
        response = {"level_action":"действие на уровне","menu_input":"ввод в меню","text_input":"текстовый ввод"}.get(review.get("stage"))
        if response:
            return response
    return "изменение кадра" if p.get("status")=="repeatable_visual_effect" else "не подтверждено"


def screen(t):
    if t["launch"]!="entry_and_payload_verified":
        return "—"
    response = action(t)
    if t["play"]=="verified" or response=="действие на уровне":
        return "игровой"
    if response in ("ввод в меню","текстовый ввод"):
        return "диалог"
    review = (t.get("paired") or {}).get("review") or {}
    if review.get("stage")=="level_action":
        return "поле видно, управление не подтверждено"
    if review.get("stage") in ("menu_input","text_input"):
        return "диалог виден, управление не подтверждено"
    return "не установлен"


def control_obstacle(t):
    pair = t["paired"] or {}
    reviewed = (pair.get("review") or {}).get("basis")
    if reviewed:
        return reviewed
    status = {"no_repeatable_visual_effect":"На одинаковых гостевых тактах реакция на клавишу не повторилась в двух опытах.",
              "stage_unconfirmed":"Не удалось достичь согласованного начального состояния сценария.",
              "baseline_unconfirmed":"Не получено сопоставимое исходное состояние для контрольного и опытного вводов.",
              "prestate_mismatch":"До ввода разошлись такты или состояние повторных запусков.",
              "exit_observed":"До завершения игровой пробы наблюдался переход по адресу выхода.",
              "infrastructure_error":"Парная проба завершилась с ошибкой инфраструктуры.",
              "repeatable_visual_effect":"Изменение кадра повторилось, управляемый объект пока не установлен."}.get(pair.get("status"),"")
    visual = (t["graphics"] or {}).get("visual","")
    return " ".join(x for x in (status,visual) if x) or t["obstacle"] or "Способ действия на уровне пока не установлен."


def kernel_name(profile):
    return {"MON":"DOS/MONITOR2.MON","RF":"DOS/RAMFOS.MON"}.get(profile, "DOS/BIOS.SYS")


def observe(out, result, reviews, paired_reviews):
    frames = result.get("frames",{})
    folder = Path("trials")/result["id"]/result["evidence"]
    audio = {name:dict(item,**audio_stats(out/folder/item["file"])) for name,item in result.get("audio",{}).items()}
    scene_frames = {k:v for k,v in frames.items() if k in ("title","before","idle","after","dealt","moved")}
    observed = [v["colors"] for v in scene_frames.values()]
    inspection = {}
    for phase, spans in result.get("integrity",{}).items():
        inspection[phase] = sorted({a for span in spans for a in span["differences"]})
    review = verify_review(out,result,reviews.get(result["id"]))
    ret = out/"returns"/result["id"]/"result.json"
    trace = read(ret) if ret.is_file() else {}
    verified_ret = trace.get("verified_ret") and trace["container_sha256"]==result["container_sha256"] and trace["primary_version"]==result["version"]
    checked = out/"entry-checks"/result["id"]/"result.json"
    entry = read(checked) if checked.is_file() else {}
    at_entry = entry.get("checks",{}).get("entry")
    audio_probe = out/"action-audio"/result["id"]/"result.json"
    matched_audio = read(audio_probe) if audio_probe.is_file() else {}
    if (matched_audio.get("primary_version")==result["version"]
            and matched_audio.get("media",{}).get("flash-disk.bin")==result["game_disk_sha256"]):
        sound_action = dict(protocol=(audio_probe.parent/matched_audio["evidence"]/"result.json").relative_to(out).as_posix(),
                            key=matched_audio["key"], comparison=matched_audio["comparison"])
    else:
        sound_action = None
    return dict(
        id=result["id"], container_sha256=result["container_sha256"], payload_sha256=result["payload_sha256"],
        profile=result["profile"], system_sha256=result["context"]["system_sha256"],
        kernel_sha256=result["context"]["kernels"][kernel_name(result["profile"])],
        json=(folder/"observation.json").as_posix(), current=(folder.parent/"result.json").as_posix(),
        state=result["outcome"], launch=result["launch"], play=result["playable"], input=result["input"],
        graphics=review, colors=sorted(set(observed)),
        palettes={phase:info["palette"] for phase,info in scene_frames.items()}, audio=audio, audio_action=sound_action, kernel_differences=inspection,
        exit="ret_instruction" if verified_ret else result.get("exit"),
        return_trace=ret.relative_to(out).as_posix() if verified_ret else None,
        recovery=result.get("recovery"), shell=result.get("returned_shell"),
        control_file=result.get("control_file"), obstacle=result.get("error") or result.get("obstacle"),
        source=result["file"], recipe=result["recipe"], action_proof=result.get("action_proof"),
        entry_check=checked.relative_to(out).as_posix() if at_entry and entry["payload_sha256"]==result["payload_sha256"] and entry["system_sha256"]==result["context"]["system_sha256"] else None,
        pid=result.get("pid"), hwnd=result.get("hwnd"), cold_boot=result.get("boot_on_lease"),
        paired=paired_trial(out,result,paired_reviews), fixture=result["context"].get("fixture"),
        disk_seed_sha256=result["context"]["disk_seed_sha256"], emulator_sha256=result.get("emulator_sha256"),
        scenario_keys={k:(result.get("scenario") or {}).get(k) for k in ("start_keys","action_keys","exit_keys")},
        floppy_sha256=result["context"].get("floppy_seed_sha256"), profile_sha256=result["context"].get("profile_sha256"),
        aliases=result["aliases"], game_disk_sha256=result.get("game_disk_sha256"),
        media_changes=[k for k,v in result.get("media_after",{}).items() if
            {"system-flash.bin":result["context"]["system_sha256"], "flash-disk.bin":result["game_disk_sha256"],
             "floppy720-test.img":result["context"].get("floppy_seed_sha256",v)}[k]!=v],
        keys_run=result.get("events",[]), retries=len(result.get("attempts",[]))-1)


def audio_label(trial):
    samples = [v for v in trial.get("audio",{}).values() if v["samples"]]
    return ("сигнал" if any(v["varied"] for v in samples) else "тишина") if samples else "—"


def audio_quality(sample):
    if sample["truncated"]:
        return "переполнение буфера"
    if not sample["samples"]:
        return "пустая запись"
    if not sample["transitions"]:
        return "постоянный уровень"
    if sample["at_int16_limit"]:
        return "достигает предела PCM16"
    if sample["transitions"] < 10:
        return "менее 10 фронтов за запись"
    return "переменный сигнал в диапазоне PCM16"


def independent_pilot(out):
    notes = []
    if (out/"budi2-independent/result.json").is_file():
        notes += ["BUDI2 в Monitor-2 принимает пробел. После шага вправо при свободной клетке слева и кирпиче под ней второй пробел меняет байт кирпича с 09h на 48h. СТР приводит в Monitor-2, для NC нужен Reset. [Независимый повтор](budi2-independent/result.json).",""]
    almaz = list((out/"almaz-independent/trials").glob("*/result.json"))
    if len(almaz)==2:
        links = [f"[{read(p)['file'].split('/')[-1]}]({p.relative_to(out).as_posix()})" for p in sorted(almaz)]
        notes += ["В RAMFOS Right смещает персонажа даже на повреждённой карте оригинального «Алмаза-2». Удержание СТР приводит к F800h и возврату в NC. Вариант с восстановленной картой проверен отдельно. " + " · ".join(links) + ".",""]
    if (out/"ochko-independent/result.json").is_file():
        notes += ["У ранее неразмеченной игры «Очко» проверены ввод числа игроков 1, банка 100, ставки 10 и ответ Д. После Enter видна раздача. [Независимый повтор](ochko-independent/result.json).",""]
    return ["## Пилот","",*notes] if notes else []


def kernel_label(trial):
    checks = dict(trial["kernel_differences"])
    recovered = checks.pop("recovered",None)
    dos = checks.pop("dos_after",None)
    dirty = {a for addresses in checks.values() for a in addresses}
    base = ("Δ "+str(len(dirty)) if dirty else "цел") if checks else "—"
    if recovered or dos:
        base += " / Δ после"
    return base


def exit_label(s):
    return dict(vector_C800="C800h",vector_C000="C000h",vector_F800="F800h",vector_F854="F854h",
                ret_instruction="RET проверен", stack_return="адрес возврата, SP+2",
                return_address_other_sp="адрес возврата с иным SP").get(s,"?")


def group_name(rec):
    path = rec["file"].split("/")
    if path[2]=="families":
        return "Семейства", "families"
    if rec.get("directory"):
        disk = Path(rec["directory"]["disk"]).stem
        return "Диск " + disk, disk
    return {"std":("Классический Специалист","std"),"mx":("Игры MX","mx"),
            "unknown":("Прочие архивы","unknown")}[path[2]]


def single(out, result, t, record):
    key = result["id"]
    evidence = "../"+str(Path(t["json"]).parent.as_posix())+"/"
    name = record["file"].removeprefix("assets/games/")
    lines = ["# "+name, "", f"Режим **{t['profile']}**. Команда гостя `{result.get('command','—')}`. "
             f"[Протокол MCP]({evidence}observation.json) · [попытки и итог](../{t['current']}).","",
             "## Воспроизведение","","```powershell",
             *(["python -X utf8 scripts/games/replay-trial.py '"+relative(out/t["json"],ROOT)+"'"]
               if t["fixture"] else ["python -X utf8 scripts/games/run-campaign.py --trial "+key+" --force"]),"```","",
             f"- Исходный файл `{record['sha256']}`.",
             f"- Контейнер `{t['container_sha256']}`, нагрузка `{t['payload_sha256']}`.",
             f"- Системная flash `{t['system_sha256']}`, активное ядро `{t['kernel_sha256']}`.",
             f"- Профиль MX2, CPU 2 МГц, раскладка smart, индивидуальные A:/C:/H:. Скорость и PID записаны в протоколе.",""]
    key_log = " → ".join(e.get("command") or e.get("key","?") for e in result.get("events",[]))
    lines += ["Команды и нажатия в протоколе — "+key_log,"","## Наблюдения","",
              f"Запуск — **{stage(t)}**. Управляемый экран — **{screen(t)}**. Реакция на ввод — **{action(t)}**.",""]
    if record.get("source_path_note"):
        lines += [record["source_path_note"],""]
    if t["entry_check"]:
        entry = read(out/t["entry_check"])["checks"]
        lines += [f"Перед PUSH переходника отличались {entry['before_push']['differences']} байт, "
                  f"в момент входа — {entry['entry']['differences']}. [Сравнение нагрузки и регистров](../{t['entry_check']}).",""]
    reason = next((v["reason"] for v in record.get("profile_reasons",[]) if v["profile"]==t["profile"]),None)
    if reason:
        lines += ["Основание профиля — "+reason+".",""]
    static = [(group,f"{item['pc']:04X}h",f"{item['target']:04X}h",item["operation"],item["reason"])
              for group,items in record.get("evidence",{}).items() for item in items[:3]]
    if static:
        lines += table(("Улика","PC","Операнд","Операция","Основание"),static)+[""]
    if len(t["aliases"])>1:
        lines += ["Тот же контейнер использован для " + ", ".join(f"`{v}`" for v in t["aliases"]) + ".",""]
    if t["obstacle"]:
        lines += [t["obstacle"],""]
    card = result.get("scenario")
    if card:
        lines += ["### Карточка этой попытки","",
                  *table(("Этап","Клавиши smart","Условие"),[(label," → ".join(card[key+"_keys"]) or "—",card[condition])
                        for label,key,condition in (("Старт","start","start_when"),("Действие","action","action_when"),
                                                    ("Выход","exit","exit_when"))]),"",card["source"],""]
    if t["action_proof"]:
        lines += ["Проверено действие на уровне. Машинные значения сохранены в поле `action_proof`.",""]
    if t.get("adskok_gameplay"):
        lines += [f"Стрелка вправо изменила байт направления в 6B71h и положение спрайта на первом поле. [Трасса RUN -MON](../{t['adskok_gameplay']}).",""]
    if t.get("independent"):
        rows = [(worker," · ".join(f"[протокол {n}](../{p})" for n,p in enumerate(paths,1)))
                for worker,paths in t["independent"].items()]
        lines += ["Эффект независимо воспроизведён двумя работниками.","",*table(("Работник","Пробы с чистого старта"),rows),""]
    pair = t["paired"]
    if pair:
        review = pair["review"] or {}
        links = [f"[протокол четырёх холодных запусков](../{pair['protocol']})"]
        roi = pair["roi"] if pair["object"] and pair["roi"] else review.get("roi") or pair["roi"]
        if roi:
            links.append(f"[фрагмент до действия, без ввода и с вводом](../{roi})")
        lines += [f"Парная проверка `{pair['status']}`. " + " · ".join(links) + ".", ""]
        if review:
            lines += [review["basis"],""]
        if pair["object"]:
            proof = pair["object"]
            lines += [f"Объект — {proof.get('game_object','см. объект в протоколе')}. Клавиша — `{pair['action'] or 'см. протокол'}`.",""]
    if t["graphics"]:
        review=t["graphics"]
        for key in ("visual","action_interpretation","palette","audio"):
            if review.get(key):
                lines += [review[key],""]
        if not review["current"]:
            lines += [f"[Кадры оценённой попытки](../trials/{t['id']}/{review['evidence']}/observation.json).",""]
    elif result["launch"].startswith("rejected"):
        lines += ["Изображение игры, палитра и звук недоступны после отказа RUN. В протоколе сохранён экран загрузчика и результат возврата.",""]
    else:
        lines += ["Визуальная оценка артефактов и соответствия эталонной палитре для этой попытки отсутствует.",""]
    images = [f"[{k}]({evidence}{v['file']})" for k,v in result.get("frames",{}).items()]
    lines += [" · ".join(images),""]
    if t["palettes"]:
        sets = {}
        for phase,counts in t["palettes"].items():
            rgb = tuple(sorted(color for color,_ in counts))
            sets.setdefault(rgb,[]).append(phase)
        lines += table(("Кадры","Наблюдаемые цвета RGB"),[(", ".join(phases)," ".join(colors)) for colors,phases in sets.items()])+[""]
        if not (t["graphics"] or {}).get("palette"):
            lines += ["Соответствие цветов авторской палитре для этой сцены пока не установлено.",""]
    if t["audio"]:
        sound_rows = [(f"[{k}]({evidence}{v['file']})",v["rate"],v["duration_ms"],
                       f"{v['minimum']}…{v['maximum']}", v["ac_rms"],v["transitions"],v["at_int16_limit"],
                       f"{v['active_frames']}/{v['frames_25ms']}",
                       "…".join(map(str,v["regular_tone_hz"])) if v["regular_tone_hz"] else "—",audio_quality(v))
                      for k,v in t["audio"].items()]
        lines += table(("PCM 1×","Гц","мс","Диапазон","RMS без DC","Фронты","Пределы int16","Активные окна 25 мс","Периодические участки, Гц","Оценка записи"),sound_rows)+[""]
    if t["return_trace"]:
        lines += [f"[Пошаговая проверка инструкции возврата и стека](../{t['return_trace']}).",""]
    if t["audio_action"]:
        sound = t["audio_action"]
        names = {"repeatable_waveform_difference":"сигнал с вводом воспроизводимо отличается",
                 "repeatable_signal_on_input":"при вводе появляется переменный сигнал",
                 "same_waveform":"сигнал совпал во всех четырёх записях",
                 "inconclusive":"различие от ввода пока не подтверждено"}
        protocol = "../"+sound["protocol"]
        base = protocol.rsplit("/",1)[0]
        lines += [f"Звук клавиши `{sound['key']}` по четырём холодным запускам на 1×. [Протокол и хеши PCM]({protocol}).","",
                  *table(("Фаза","Сравнение","Без ввода","С вводом"),[
                      (label,names[sound["comparison"][phase]["outcome"]],f"[WAV]({base}/idle-1/{phase}.wav)",f"[WAV]({base}/input-1/{phase}.wav)")
                      for phase,label in (("action","Удержание"),("release","После отпускания"))]),""]
    if t["media_changes"]:
        lines += ["После пробы изменилось содержимое рабочих копий "+", ".join(t["media_changes"])+". Хеши до и после приведены в протоколе.",""]
    if t.get("rechecks"):
        rows = []
        for row in t["rechecks"]:
            diag = row.get("diagnostic",{})
            labels = {True:"✓",False:"отказ",None:"—"}
            rows.append((f"[повтор](../{row['protocol']})","✓" if row.get("control_file") else "отказ",
                         labels[diag.get("type_again")],labels[diag.get("second_reset")],labels[diag.get("cold_same_media")],
                         ", ".join(f"{a:04X}h" for a in row.get("kernel_after",[])) or ("совпало" if "kernel_after" in row else "—")))
        lines += ["### Повтор проверки оболочки","",*table(("Протокол","Первый TYPE","Повтор TYPE без Reset","После второго Reset","После холодного старта","Ядро после выхода"),rows),""]
    lines += [f"Переход выхода **{exit_label(t['exit'])}**. Восстановление **{t['recovery'] or '—'}**. "
              f"Оболочка **{t['shell'] or '—'}**. TYPE H:CHECK.TXT — **{'прочитан' if t['control_file'] else 'не подтверждено'}**.",""]
    for phase,addrs in t["kernel_differences"].items():
        state = ("адреса отличий " + ", ".join(f"{a:04X}h" for a in addrs[:20]) +
                 (f" и ещё {len(addrs)-20}" if len(addrs)>20 else "")) if addrs else "выбранные области и векторы совпали"
        lines += [f"- `{phase}` — {state}."]
    save(out/"cases"/(t["id"]+".md"), lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT/"docs/compat/game-campaign")
    parser.add_argument("--inventory", type=Path, default=ROOT/"docs/compat/game-inventory.json")
    parser.add_argument("--reference", type=Path, help="Каталог кампании для отдельной эталонной сборки")
    args = parser.parse_args()
    out = args.out.resolve()
    audit = read(args.inventory)
    cards = read(ROOT/"assets/games/controls.json")["cards"]
    reviews = read(out/"reviews.json") if (out/"reviews.json").exists() else {}
    paired_reviews = read(out/"paired-reviews.json") if (out/"paired-reviews.json").is_file() else {}
    workers_file = out/"playbook-independent-protected/workers.json"
    workers = read(workers_file) if workers_file.is_file() else {}
    independent = {(row["container_sha256"],row["profile"]):row for row in workers.get("verified",[])+workers.get("direct",[])}
    extras = extra_controls(out)
    lookup = {rec["file"]:rec for rec in audit["files"]}
    historical,ramfos_index = ramfos_catalog(out,lookup)
    history,history_indices = load_history(out,audit["files"],historical)
    almaz_native = max((out/"native-ramfos/almaz2").glob("attempt-*/index.json"),key=lambda p:p.stat().st_mtime_ns,default=None)
    pairs_file = out/"paired/validation.json"
    validation = read(pairs_file) if pairs_file.is_file() else {}
    rechecks = defaultdict(list)
    for name in ("recovery-probe","rechecks"):
        candidate = out/name/"summary.json"
        if candidate.is_file():
            for key,rows in read(candidate).items():
                for row in rows:
                    if row.get("evidence"):
                        protocol = Path(name)/row["evidence"]/"observation.json"
                        rechecks[key].append(dict(row,protocol=protocol.as_posix()))
    files, trials, groups = [], {}, defaultdict(list)
    adskok = adskok_trace(out)
    odi_file = out/"native-odi"/"results.json"
    odi = read(odi_file) if odi_file.is_file() else []
    missing, errors, defects, differences = [], [], [], []
    for rec in audit["files"]:
        name = rec["file"].removeprefix("assets/games/")
        group, slug = group_name(rec)
        label = Path(name).name
        record = dict(file=name, source_sha256=rec["sha256"], payload_sha256=rec["payload_sha256"],
                      load=rec["load"], entry=rec["entry"], entry_source=rec.get("entry_source"),
                      note=rec.get("blocked") or rec.get("source_path_note"), defects=defect(rec,adskok),
                      prior=rec["previous_audit"], historical=history.get(name,[]), native_ramfos=historical.get(name), trials=[])
        if record["defects"]:
            defects.append((name, ". ".join(record["defects"])))
        for route in rec["routes"]:
            for profile in rec["profiles"]:
                key = route["id"]+"-"+profile
                record["trials"].append(key)
                if key not in trials and (out/"trials"/key/"result.json").is_file():
                    result = read(out/"trials"/key/"result.json")
                    if result.get("boot_on_lease",1)==1 or result.get("initial_flash_protection")=={
                            "system-flash.bin.sdp":"01","flash-disk.bin.sdp":None}:
                        trial = observe(out,result,reviews,paired_reviews)
                        if (adskok.get("released") and trial["profile"]=="MON"
                                and trial["container_sha256"]==adskok["sha256"]
                                and trial["system_sha256"]==adskok["system_sha256"]):
                            trial["adskok_gameplay"] = adskok["released"]
                        proof = independent.get((trial["container_sha256"],profile))
                        if (proof and proof["payload_sha256"]==trial["payload_sha256"]
                                and proof["initial_media"]["flash-disk.bin"]==trial["game_disk_sha256"]
                                and proof.get("emulator_sha256",trial["emulator_sha256"])==trial["emulator_sha256"]
                                and proof["initial_media"]["system-flash.bin"]==trial["system_sha256"]
                                and proof["initial_media"]["floppy720-test.img"]==trial["floppy_sha256"]
                                and proof["initial_flash_protection"]==result["context"].get("flash_protection")):
                            trial["independent"] = {}
                            for who in ("first","repeat"):
                                source = proof[who]
                                paths = source.get("results") or [source["result"]]
                                trial["independent"][source["worker"]] = [relative(ROOT/p,out) for p in paths]
                        extra = extras.get((trial["container_sha256"],profile))
                        if (extra and extra["payload_sha256"]==trial["payload_sha256"] and extra["emulator"]==trial["emulator_sha256"]
                                and extra["media"]=={"flash-disk.bin":trial["game_disk_sha256"],"system-flash.bin":trial["system_sha256"],
                                                     "floppy720-test.img":trial["floppy_sha256"]}
                                and extra["protection"]==result["context"].get("flash_protection")):
                            trial.setdefault("independent",{}).update(extra["workers"])
                            trial["extra_object"] = extra["object"]
                        trial["rechecks"] = rechecks[key]
                        trials[key] = trial
                        single(out,result,trial,lookup[result["file"]])
                t = trials.get(key)
                mode = route_name(route)+"/"+profile
                if not t:
                    groups[(group,slug)].append((label,mode,"очередь","—","—","—","—","—","—"))
                    continue
                support = next((c for c in cards if name in c["files"] and profile in c["profiles"]),None)
                if t["launch"]=="entry_and_payload_verified" and action(t)!="действие на уровне":
                    missing.append((name,mode,f"[{action(t)}](cases/{key}.md)",control_obstacle(t)))
                link = f"../cases/{key}.md"
                playing = action(t) if t["launch"]=="entry_and_payload_verified" else "—"
                palette = "/".join(map(str,t["colors"])) if t["colors"] else "—"
                recovery = "Reset" if t["recovery"]=="reset" else "оболочка" if t["recovery"]=="game_exit" else "?"
                if t["launch"].startswith("rejected") and t["recovery"]=="game_exit":
                    recovery = "оболочка после отказа"
                if t["control_file"]:
                    recovery += " ✓"
                elif any(r.get("diagnostic",{}).get("second_reset") for r in t["rechecks"]):
                    recovery += " без H:, второй Reset ✓"
                groups[(group,slug)].append((f"[{label}]({link})",mode,stage(t),screen(t),playing,palette,audio_label(t),
                                                kernel_label(t),f"{exit_label(t['exit'])} → {recovery}"))
        if not rec["routes"]:
            groups[(group,slug)].append((label,"—","данные","—",rec["blocked"],"—","—","—","—"))
        files.append(record)
    for (group,slug), rows in groups.items():
        save(out/"catalog"/(slug+".md"),["# "+group,"","[Сводка и обозначения](../RESULTS.md). Откройте имя файла для кадров, записи и команды повтора.","",
            *table(("Вариант","Контейнер/среда","Запуск","Управляемый экран","Реакция","Цветов","Звук","Ядро","Выход и TYPE"),rows)])
    overview = []
    for rec in files:
        runs = [trials[k] for k in rec["trials"] if k in trials]
        catalog_routes = {r["id"]:r for r in lookup["assets/games/"+rec["file"]]["routes"]}
        links = []
        for row in runs:
            route = catalog_routes[row["id"].rsplit("-",1)[0]]
            label = route_name(route)+"/"+row["profile"]
            links.append(f"[{label}](cases/{row['id']}.md)")
        entered = [row for row in runs if row["launch"]=="entry_and_payload_verified"]
        rank = {"действие на уровне":4,"ввод в меню":3,"текстовый ввод":3,"изменение кадра":2}
        if entered:
            best = max(entered,key=lambda t:rank.get(action(t),0))
            outcome = "вход, "+action(best)
        elif runs:
            outcome = " · ".join(dict.fromkeys(stage(t) for t in runs))
        else:
            outcome = "данные уровней" if not rec["trials"] else "в очереди"
        read_back = f"{sum(bool(t['control_file']) for t in runs)}/{len(runs)}" if runs else "—"
        history_link = f"[{len(rec['historical'])} проб](historical.md#{anchor(rec['file'])})" if rec["historical"] else "—"
        overview.append((rec["file"],outcome,read_back," · ".join(links) or "—",history_link))
    save(out/"historical.md",history_table(files,history,history_indices,table))
    save(out/"catalog.md",["# Сводная таблица по именам каталога","",
         "Каждая строка соответствует одной записи catalog.csv. Показана наиболее продвинутая стадия среди её путей запуска. "
         "Доля TYPE — число проб, в которых оболочка прочла контрольный файл при основной проверке. Каждый профиль ведёт к своим кадрам, записи и протоколу.","",
         "Ссылка в последней колонке открывает результаты оригинальных Monitor-2, RAMFOS и MX-DOS для данного файла.","",
         *table(("Файл","Лучший этап RUN","TYPE","Пробы контейнеров и сред","Исторические пути"),overview)])
    counts = Counter()
    for key,t in trials.items():
        t["controlled_screen"],t["input_class"] = screen(t),action(t)
        for sample in t["audio"].values():
            sample["signal_quality"] = audio_quality(sample)
        counts["entry"] += t["launch"]=="entry_and_payload_verified"
        counts["action"] += action(t)=="действие на уровне"
        counts["paired"] += bool(t["paired"])
        counts["read"] += bool(t["control_file"])
        counts["reviewed"] += bool(t["graphics"])
        counts["sound"] += audio_label(t)=="сигнал"
        counts["with_pcm"] += bool(t["audio"])
        counts["action_audio"] += bool(t["audio_action"])
        if t["state"]=="infrastructure_error":
            errors.append((f"[{t['source']}](cases/{key}.md)",t["obstacle"]))
        if "Δ" in kernel_label(t):
            addresses = sorted({a for diff in t["kernel_differences"].values() for a in diff})
            head = ", ".join(f"{a:04X}h" for a in addresses[:6])
            suffix = f" и ещё {len(addresses)-6}" if len(addresses)>6 else ""
            differences.append((f"[{t['source']}](cases/{key}.md)",t["profile"],head+suffix))
    comparisons = []
    if args.reference:
        ref_summary = args.reference/"results.json"
        ref_trials = read(ref_summary)["trials"] if ref_summary.is_file() else {}
        for key,t in trials.items():
            path = args.reference/"trials"/key/"result.json"
            if not path.exists():
                continue
            ref = read(path)
            condition = None
            if (t["profile"] != ref["profile"] or t["container_sha256"] != ref["container_sha256"]
                    or t["payload_sha256"] != ref["payload_sha256"] or ref.get("emulator_sha256") != t["emulator_sha256"]
                    or ref["context"]["disk_seed_sha256"] != t["disk_seed_sha256"]
                    or ref["context"].get("floppy_seed_sha256")!=t["floppy_sha256"]
                    or ref["context"].get("profile_sha256")!=t["profile_sha256"]
                    or ref.get("game_disk_sha256")!=t["game_disk_sha256"]
                    or any((ref.get("scenario") or {}).get(k)!=v for k,v in t["scenario_keys"].items())):
                continue
            if ref["context"]["system_sha256"]==t["system_sha256"]:
                continue
            elif ref.get("launch")=="entry_and_payload_verified" and t["launch"]!="entry_and_payload_verified":
                condition = "требуется повтор запуска"
            elif (ref.get("input")=="verified" or ref_trials.get(key,{}).get("input_class")=="действие на уровне") and t["input_class"]!="действие на уровне":
                condition = "требуется повтор игрового действия"
            elif ref.get("control_file") and not t["control_file"]:
                condition = "требуется повтор TYPE"
            elif ref.get("recovery")=="game_exit" and t["recovery"] in ("reset","reset_failed"):
                condition = "требуется повтор штатного выхода"
            elif any(t["kernel_differences"].get(phase) and spans and all(not s["differences"] for s in spans)
                     for phase,spans in ref.get("integrity",{}).items()):
                condition = "требуется сверка изменившихся областей ядра и векторов"
            elif any(phase in t["audio"] and old.get("varied") and not t["audio"][phase].get("varied") and
                     min(old["samples"],t["audio"][phase]["samples"])>=old["rate"]//4
                     for phase,old in ref.get("audio",{}).items()):
                condition = "требуется повтор записи PCM в той же фазе"
            elif max(ref_trials.get(key,{}).get("colors",[]) or [0])>=4 and t["colors"] and max(t["colors"])<=2:
                condition = "требуется сверка цветных кадров на одинаковой сцене"
            if condition:
                baseline = path.parent/ref["evidence"]/"observation.json"
                comparisons.append(dict(trial=key,stage=condition,reference=relative(baseline,out),
                    observed=t["json"], profile=t["profile"], container_sha256=t["container_sha256"],
                    payload_sha256=t["payload_sha256"], keys=t["keys_run"], baseline_keys=ref.get("events",[]),
                    repeat_reference="python -X utf8 scripts/games/replay-trial.py '"+relative(baseline,ROOT)+"'",
                    repeat_candidate="python -X utf8 scripts/games/replay-trial.py '"+relative(out/t["json"],ROOT)+"'",
                    baseline_system=ref["context"]["system_sha256"],baseline_kernel=ref["context"]["kernels"][kernel_name(t["profile"])],
                    candidate_system=t["system_sha256"],candidate_kernel=t["kernel_sha256"]))
    def catalog_count(check):
        return sum(any(key in trials and check(trials[key]) for key in rec["trials"]) for rec in files)
    summary = dict(audit=audit["summary"], completed_pairs=len(trials), **counts, infrastructure_errors=len(errors),
                   without_card=sum(not any(rec["file"] in c["files"] for c in cards) for rec in files),
                   catalog_with_action_keys=len({n for c in cards if c["action_keys"] for n in c["files"]}),
                   unresolved_variants=len({name for name,*_ in missing}), phase_differences=len(differences),
                   catalog_entered=catalog_count(lambda t:t["launch"]=="entry_and_payload_verified"),
                   catalog_action=catalog_count(lambda t:action(t)=="действие на уровне"),
                   primary_windows=len({(t["pid"],t["hwnd"]) for t in trials.values() if t["hwnd"]}),
                   primary_cold_starts=sum(bool(t["cold_boot"]) for t in trials.values()),
                   independently_repeated=sum(bool(t.get("independent")) for t in trials.values()),
                   catalog_without_action_keys=len(files)-len({n for c in cards if c["action_keys"] for n in c["files"]}),
                   exact_ret=sum(t["exit"]=="ret_instruction" for t in trials.values()),
                   modified_at_entry=sum(bool(t["entry_check"]) for t in trials.values()),
                   changed_media=sum(bool(t["media_changes"]) for t in trials.values()),
                   native_ramfos=dict(Counter(row["status"] for row in historical.values())),
                   historical_names=len(history),
                   type_after=dict(Counter(t["recovery"] for t in trials.values() if t["control_file"])),
                   launches=dict(Counter(t["launch"] for t in trials.values())),
                   controls=dict(Counter(action(t) for t in trials.values() if t["launch"]=="entry_and_payload_verified")))
    document(out/"results.json", dict(schema=1,summary=summary,catalog=files,trials=trials,comparisons=comparisons,
                                     native_odi=odi, native_ramfos_index=ramfos_index, historical_indices=history_indices))
    if comparisons:
        notes = ["# Кандидаты на регрессию","","Для каждой пары приведены два протокола и две команды повтора. Побайтовые входные данные и исходная карточка восстанавливаются по протоколу.",""]
        for c in comparisons:
            notes += ["## "+c["trial"],"",f"{c['stage']}. Среда {c['profile']}, SHA-256 нагрузки `{c['payload_sha256']}`, контейнера `{c['container_sha256']}`.","",
                      *table(("Сборка","Активное ядро","Системная flash","Свидетельства"),[
                          ("Эталон",c["baseline_kernel"],c["baseline_system"],f"[протокол]({c['reference']})"),
                          ("Кандидат",c["candidate_kernel"],c["candidate_system"],f"[протокол]({c['observed']})")]),"",
                      "Ввод эталона — "+" → ".join(e.get("command") or e.get("key","?") for e in c["baseline_keys"])+".","",
                      "Ввод кандидата — "+" → ".join(e.get("command") or e.get("key","?") for e in c["keys"])+".","",
                      "```powershell",c["repeat_reference"],c["repeat_candidate"],"```",""]
        save(out/"regressions.md",notes)
    modes = defaultdict(list)
    for key,t in trials.items():
        modes[key.rsplit("-",1)[0]].append(t)
    mode_rows = []
    profile_observations = []
    examples = {"samantha.com.bin","StarTrek-std.rks","Buran-ramfos.cpu"}
    for kind,runs in modes.items():
        if len(runs)>1:
            row = [runs[0]["source"].removeprefix("assets/games/"),kind.split("-",1)[0].upper()]
            for profile in ("MON","STD","RF","MX","NATIVE"):
                t = next((t for t in runs if t["profile"]==profile),None)
                row.append(f"[{stage(t)}, {action(t)}](cases/{t['id']}.md)" if t else "—")
            mode_rows.append(row)
            if Path(runs[0]["source"]).name in examples:
                for t in runs:
                    pair = t.get("paired") or {}
                    note = (pair.get("review") or {}).get("basis") or (t.get("graphics") or {}).get("visual")
                    if note:
                        profile_observations.append((f"[{Path(t['source']).name}](cases/{t['id']}.md)",
                                                     t["id"].split("-",1)[0].upper()+"/"+t["profile"],action(t),note))
    save(out/"profiles.md",["# Один контейнер в разных средах","","Клавиши, ожидаемый ABI и состояние при возврате приведены в каждой пробе.","",
                            *table(("Исходник","Формат","MON","STD","RF","MX","NATIVE"),mode_rows)])
    save(out/"previously-uncovered.md", ["# Текущие файлы без точного совпадения в прежнем аудите","",
        *table(("Путь из каталога","Причина"),[(n.removeprefix("assets/games/"),
        "Отличается SHA-256" if lookup[n]["previous_audit"]=="changed_or_unreadable" else "Расширение "+Path(n).suffix)
        for n in audit["previously_uncovered"]])])
    save(out/"unknown-controls.md",["# Управление с неполным подтверждением","","Таблица включает пробы, в которых RUN передал управление телу игры, а действие на уровне ещё не подтверждено.","",
         *table(("Вариант","Проба","Достигнутый этап","Последнее наблюдение"),missing)])
    lines = ["# Игры «Специалиста» в MXOS2","",
        f"В каталоге **{len(files)}** записей и **{audit['summary']['distinct_payloads']}** различных нагрузок. "
        f"Закончено **{len(trials)}/{audit['summary']['container_profile_pairs']}** пар «контейнер × среда». "
        f"Вход с побайтно сверенной нагрузкой наблюдался в **{counts['entry']}** пробах. "
        f"Действие на уровне подтверждено в **{counts['action']}** пробах, данные контрольного файла прочитаны в **{counts['read']}**. "
        f"При подсчёте по именам каталога получено **{summary['catalog_entered']}** вариантов со входом и **{summary['catalog_action']}** с проверенным игровым действием.","",
        "Текущая системная flash запускает игры через RUN. MON устанавливает Monitor-2 и карту STD, RF — адаптер RAMFOS и карту MX. "
        "MX, STD и NATIVE проверены для соответствующих записей реестра. " +
        ("Результаты сопоставлены с отдельно указанной системной сборкой." if args.reference else
         "Отдельная реализация будущего совместимого ядра пока отсутствует. Подтверждённых регрессий между реализациями нет."),"",
        f"[Все {len(files)} строк одной таблицей](catalog.md) · [машиночитаемые результаты](results.json) · [пропуски прежнего аудита ({len(audit['previously_uncovered'])})](previously-uncovered.md) · "
        f"[пробы с неполным подтверждением управления](unknown-controls.md) · [один контейнер в нескольких средах](profiles.md) · "
        f"[справочник клавиш]({relative(ROOT/'assets/games/playbook.md',out)}) · [команды стенда]({relative(ROOT/'scripts/games/README.md',out)}).","",
        *(["Названия на диске и свежие пробы трёх «Буранов» и StarTrek с KOI-7 сохранены отдельно. "
           "[Команды, пуски, заставки и проверка STD в Emu80](buran-startrek/README.md).",""]
          if (out/"buran-startrek/README.md").is_file() else []),
        *independent_pilot(out),
        *(["Пилот повторён в первоначальной среде. У BUDI2 и «Очка» совпали адресные результаты и рисунок шести игровых кадров, передний план имел цвет #FFFFFF в STD и #C0C0C0 в MON. "
           "[Сопоставление полей памяти, событий и кадров](native-monitor/index.json).",""] if (out/"native-monitor/index.json").is_file() else []),
        *(["Оригинальные нагрузки «Алмаза-2» и его исправление запускались также из каталога настоящего ПЗУ RAMFOS. Протоколы различают архивный COM и производные заголовки для точных тел двух версий RF. "
           f"[Движение и возврат в файловое меню ПЗУ]({relative(almaz_native,out)}).",""] if almaz_native else []),
        *(["На 1× звук эмулируемого динамика STD снят до регулятора громкости. Калибровка импульсами PC5 и четыре старта на игру доступны для сравнения с динамиком MX2. "
           "[WAV и численные характеристики BUDI2 и «Очка»](native-monitor/speaker/index.json).",""] if (out/"native-monitor/speaker/index.json").is_file() else []),
        *([f"Два работника независимо повторили действия в **{summary['independently_repeated']}** вариантах «контейнер × среда». "
           f"[Первая серия]({workers_file.relative_to(out).as_posix()})."
           + (" [Дополнительные действия с координатами и кадрами каждого работника](controls-extra/evidence.json)." if extras else ""),""] if workers else []),
        "## Охват проверок","",*table(("Этап","Число проб"),[
            ("Вход с неизменной нагрузкой",counts["entry"]),("Вход с изменением нагрузки перед первой инструкцией",summary["modified_at_entry"]),
            ("Отказ заголовка или диапазона",summary["launches"].get("rejected_header_or_range",0)),("Отказ тела",summary["launches"].get("rejected_body",0)),
            ("Четыре синхронных прогона",counts["paired"]),("Действие на игровом уровне",counts["action"]),
            ("Просмотр игровых кадров или экрана остановки",counts["reviewed"]),("Захват PCM при 1×",counts["with_pcm"]),
            ("Парный захват PCM действия при 1×",counts["action_audio"]),
            ("Переменный звук в записанных интервалах",counts["sound"]),("Выполнение RET проверено по опкоду и стеку",summary["exact_ret"]),
            ("Команда TYPE прочитала точный маркер",counts["read"])]),"",
        *([f"Проверены **{validation['initial_media_verified']}** исходных комплектов носителей и **{validation['validated_png']}** парных RGB-кадров. "
           f"В **{validation['same_window_four_starts']}** пробах все четыре старта прошли в одном видимом окне. "
           f"Для {validation['verified_game_object']} проб подтверждено изменение игрового объекта по измерениям состояния. "
           "[Индекс четырёх запусков](paired/index.json) · [пересчёт кадров, масок и координат](paired/validation.json).",""] if validation else []),
        f"В справочнике указаны клавиши действия для **{summary['catalog_with_action_keys']}** имён. Несколько профилей и путей запуска могут давать разные результаты для одного имени. "
        f"У **{summary['catalog_entered']-summary['catalog_action']}** вариантов с побайтно подтверждённым входом пока нет подтверждённого действия на уровне ни в одном проверенном профиле.","",
        "## Каталог","","Разделы сгруппированы по расположению исходников. В «Игры MX» входят все варианты из `assets/games/mx/`, в том числе запуски в RAMFOS и Monitor-2. Для каждого контейнера и среды есть отдельная строка. Состав штатного H: задают [манифест Games2M](../../../assets/H-Flash-2M-Games.txt) и [манифест Games64k](../../../assets/H-ROM-64k-Games.txt).","",*table(("Раздел","Строк в таблице, включая разные пути запуска"),
            [(f"[{name}](catalog/{slug}.md)",len(rows)) for (name,slug),rows in groups.items()]),"",
        f"Исторические пути запуска проверены отдельно для **{len(history)}** имён. [Таблица оригинальных загрузчиков с протоколами](historical.md).","",
        "В графе «Реакция» отдельно указаны изменение игрового состояния, ввод в меню, текстовый ввод и изменение кадра. "
        "Число цветов измерено по RGB-кадрам. Оценка видимых артефактов и палитры находится в протоколе игры. "
        "Звук описывает интервалы захвата PCM на 1×. В протоколе указаны длительность, частота, амплитуда и отсчёты на цифровом пределе. Слуховая оценка звучания остаётся открытой. Для большей части каталога отсутствует эталон звучания и палитры. Δ означает расхождение с ожидаемыми байтами в проверявшихся областях. "
        "Знак ✓ после возврата означает, что команда TYPE прочитала верное содержимое H:CHECK.TXT.","",
        "«Архив» означает передачу исходных байтов RUN под исходным расширением. Для записей Lafan с EXI, EX2, MG2 и MGK вторая проба использует адрес загрузки из каталога ODI и предполагаемый вход с начала тела. Специальный загрузчик этих четырёх типов пока не установлен. Уровни Zoo зарегистрированы как данные.","",
        *(["## Архивные дискеты Lafan","",
           "В заголовках ODI находятся 47 EXE, 29 COM и 97 файлов четырёх других типов. Документация и код RAMFOS определяют запуск по адресу загрузки для EXE и COM. "
           "[Адреса, типы и исторические источники](lafan-loading.md).",""] if (out/"lafan-loading.md").is_file() else []),
        *([f"На оригинальном ROM RAMFOS проверена аппаратная загрузка F6 с {len(odi)} дискет. При сверке пяти секторов с памятью E400h–F7FFh суммарно различалось {sum(r['mismatching_bytes'] for r in odi)} байт. "
           f"В {sum(r['outcome']=='firmware_error' for r in odi)} пробах ROM перешёл к обработчику ошибки при проверке статуса ВГ93. "
           "[Протоколы F6 со статусом контроллера и хешами секторов](native-odi/results.json).",""] if odi else []),
        *([f"Через файловое меню того же ПЗУ отдельно проверены **{len(historical)}** записей исходных дискет на RAM-диске. "
           f"ПЗУ передало управление с полным совпадением SHA-256 в **{summary['native_ramfos'].get('entry_and_payload_verified',0)}** случаях, "
           f"отклонило **{summary['native_ramfos'].get('checksum_rejected',0)}** по архивной сумме и **{summary['native_ramfos'].get('type_rejected',0)}** по типу. "
           f"[Протоколы Enter по всем записям]({ramfos_index}).",""] if historical else []),
        *(["Отдельная сборка Emu80 воспроизводит ожидание следующего сектора в многосекторном чтении ВГ93. С включённой поправкой ROM передал управление MX-DOS V3.6. F6 в его окне открывает исходную запись с выбранной дискеты. "
           "[Проверка состояния ВГ93, RNF и D0](native-fdc-model/results.json) · [выбор исходных записей в MX-DOS](native-fdc-run/results.json).",""] if (out/"native-fdc-run/results.json").is_file() else []),
        *(["[Различия исходного RAMFOS и RUN -RF, клавиши и команды повторов](native-ramfos/differences.md).",""] if (out/"native-ramfos/differences.md").is_file() else []),
        *(["## Наблюдения в разных средах","",*table(("Вариант","Среда","Действие","Кадр"),profile_observations),""] if profile_observations else []),
        "## Сравнение сборок","",*(["[Кандидаты с хешами, клавишами и командами воспроизведения](regressions.md)."] if comparisons else []),
        *([f"- [{c['trial']}](cases/{c['trial']}.md) — {c['stage']}. [Эталон]({c['reference']})." for c in comparisons]
        or ["Кандидатов для повторной проверки между двумя сопоставимыми сборками в этом отчёте нет."]),"",
        "## Дефекты архивных игр, производных версий и контрольных сумм","",*table(("Вариант","Свидетельство"),defects),"",
        "## Расхождения в памяти во время игры","",*table(("Вариант","Среда","Различающиеся адреса"),differences),"",
        "## Проверка оболочки","",f"Точный маркер прочитан после возврата в оболочку в **{summary['type_after'].get('game_exit',0)}** пробах и после аппаратного Reset в **{summary['type_after'].get('reset',0)}**. "
        "Таблица ниже перечисляет исключения в основной очереди.","",
        *table(("Проба","Среда","Возврат"),[(f"[{t['source']}](cases/{key}.md)",t["profile"],t["recovery"]) for key,t in trials.items() if not t["control_file"]]),"",
        *(["У двух вариантов Peregon в MX при более раннем повторе после Reset команда TYPE выводила No media при исходных хешах носителей. В основной очереди маркер прочитан. [Ранние повторы на чистых слотах](peregon-recheck/trials/).",""] if (out/"peregon-recheck/trials").is_dir() else []),
        *(["В четырёх повторных пробах Skachki с исправленной моделью AT29 точные нагрузки запустились, образы A:, C: и H: сохранились. "
           "Контрольный файл прочитан для `std/Skachki.rks` в STD и MON и для архивной `unknown/games_rks/Skachki.rks` в MON. "
           "У архивной версии в MX чтение после первого Reset снова не прошло. [Отдельный повтор на новой сборке](at29-sdp-skachki/results.json).",""]
          if (out/"at29-sdp-skachki/results.json").is_file() else []),
        *(["В трёх дополнительных стартах архивной Skachki в MX контрольный файл прочитан после второго Reset. "
           "Образы сохранились во всех трёх стартах. [Протоколы двух сбросов](at29-sdp-skachki/rechecks/summary.json).",""]
          if (out/"at29-sdp-skachki/rechecks/summary.json").is_file() else []),
        *(["После поправки границы 150 мкс ещё раз пройдены все четыре маршрута «Скачек». На архивном MX новый повтор подтвердил чтение после второго Reset. "
           "[Маршруты и хеши итоговой сборки](at29-sdp-skachki-final/results.json) · [Повтор Reset](at29-sdp-skachki-final/rechecks/summary.json).",""]
          if (out/"at29-sdp-skachki-final/results.json").is_file() else []),
        *([f"[Дополнительные аппаратные сбросы и холодный старт с сохранёнными носителями]({name}/summary.json)." for name in ("recovery-probe","rechecks") if (out/name/"summary.json").is_file()]),"",
        "## Изменённые носители","",*table(("Проба","Среда","Изменившиеся рабочие копии"),[
            (f"[{t['source']}](cases/{key}.md)",t["profile"],", ".join(t["media_changes"])) for key,t in trials.items() if t["media_changes"]]),"",
        "## Нагрузка перед входом","",*([f"[{t['source']}](cases/{key}.md) — [сравнение до PUSH и непосредственно в точке входа]({t['entry_check']})."
            for key,t in trials.items() if t["entry_check"]] or ["Отдельных проверок изменения тела перед входом в этом каталоге нет."]),"",
        "## Проблемы и проверки стенда","",f"После чистого повтора ошибок инфраструктуры осталось **{len(errors)}**. "
        "При пилоте исправлены гонка чтения пути завершающегося процесса и сверка BIOS после выхода из банка NC. Из проверки исключены подтверждённые штатные изменения шрифта и переменных. "
        f"[8 машин]({relative(ROOT/'docs/games/mcp-pool/check-8.json',out)}) · [16 машин]({relative(ROOT/'docs/games/mcp-pool/check-16.json',out)}).",
        *(["","В промежуточном режиме холодного запуска расходился флаг защиты системной flash. Без него два запуска Xonix изменили по 31 479 байт рабочей копии C:. После восстановления штатного SDP 01 три повтора сохранили все носители. В протоколах учитывается защита при старте. [Исправленные повторы Xonix](xonix-sdp/summary.json) · "
           f"[проверка аппаратной записи и неизменного HWND]({relative(ROOT/'docs/games/mcp-pool/check-cold.json',out)})."] if (out/"xonix-sdp/summary.json").is_file() else []),
        *(["","[Аварийная остановка, возобновление очереди и обрыв MCP](queue-test/results.json)."] if (out/"queue-test/results.json").is_file() else []),
        "",*table(("Проба","Причина"),errors)]
    save(out/"RESULTS.md", lines)
    print(json.dumps(summary,ensure_ascii=False,indent=2))


if __name__=="__main__":
    main()
