"""Проверяемые игровые действия. Адресные пробы привязаны к конкретным нагрузкам."""
from campaign_guest import Observation

BUDI = "d918c31aa8c7a23b223a64f53251983a9491b39d93916566d8a5a913f26f1832"
ALMAZ_RF = {
    "e2e0193a23656a4063d7966b912090bd553edf74f1bd2405ec9ef7a8cd48472c",
    "3ff4b12a3a213edfd11a076a375231bcb56ebe51b630aec4a06ec22a9c0eee52",
}
OCHKO = "74b00cb4caa7d4384d34a7f94d3f53c590f3f6ee316e45bb1079c7f8c2110ba5"


def gate(g, pc, ms=12000):
    if g.regs()["pc"] != pc and not g.seek(pc, ms):
        raise Observation(f"Игра не дошла до ожидаемого состояния PC={pc:04X}h")


def action_to(g, key, pc, limit=2400):
    g.points(pc, *g.return_points)
    g.call("kbd_send", keys=[key], action="press")
    try:
        before = g.regs()["pc"]
        actual = g.advance(limit)
        g.keys.append(dict(key=key, action="hold_until_pc", from_pc=before,
                           target_pc=pc, actual_pc=actual, guest_ms_limit=limit))
    finally:
        g.call("kbd_send", keys=[key], action="release")
    if actual != pc:
        g.watch_exit(actual)
        raise Observation(f"{key}: ожидалось {pc:04X}h, остановка {actual:04X}h")


def budi(g):
    gate(g, 0x2a0, 30000)
    g.frame("title")
    g.points(0x3a1)
    g.key("space", 500, 0)
    gate(g, 0x3a1)
    start = g.regs()["hl"]
    g.frame("before")
    if start != 0x8852:
        raise Observation(f"BUDI2: начальная клетка {start:04X}h")
    action_to(g, "right", 0x485)
    g.points()
    gate(g, 0x3a1)
    moved = g.regs()["hl"]
    brick = moved - 28 + 1
    neighbor = g.read(moved-28, 2)
    if moved != start+28 or neighbor != b"\x00\x09":
        raise Observation(f"BUDI2: позиция {moved:04X}h, сосед и кирпич {neighbor.hex()}")
    g.frame("moved")
    action_to(g, "space", 0x5fb)
    hole = g.read(brick, 1)[0]
    if hole != 0x48:
        raise Observation(f"BUDI2: код выкопанного кирпича {hole:02X}h")
    g.points()
    gate(g, 0x3a1)
    g.result.update(input="verified", playable="verified",
                    action_proof=dict(start_cell=start, moved_cell=moved, brick_address=brick,
                                      before=9, after=hole, hole_timer=g.read(0x8d00,5).hex()))
    g.frame("after")


def almaz(g):
    gate(g, 0x38, 45000)
    g.frame("before")
    def player():
        return [0x81c0+i for i,x in enumerate(g.read(0x81c0,32)) if x & 15 == 9]
    start = player()
    if start != [0x81c1]:
        raise Observation(f"Алмаз-2: игрок не найден в начальной клетке, найдено {start}")
    g.points(0x38)
    g.call("kbd_send", keys=["right"], action="press")
    try:
        for ticks in range(1,13):
            g.advance(1100)
            if player() != start:
                break
    finally:
        g.call("kbd_send", keys=["right"], action="release")
    after = player()
    g.keys.append(dict(key="right", action="hold_game_loops", pc=0x38, loops=ticks))
    if len(after) != 1 or not start[0] < after[0] <= start[0]+4:
        raise Observation(f"Алмаз-2: после удержания вправо игрок в {after}")
    g.result.update(input="verified", playable="verified",
                    action_proof=dict(start_cell=start[0], moved_cell=after[0], tile=9))
    g.frame("after")


def ochko(g):
    def prompt(name):
        gate(g, 0x485, 18000)
        pos = int.from_bytes(g.read(0x237,2), "little")
        g.result.setdefault("basic_inputs", []).append(pos)
        g.frame(name)
        return pos
    # Интерпретатор сохраняет ссылку на текущий оператор INPUT. Все три
    # запроса имеют разные адреса, повтор первого запроса не считается ставкой.
    def answer(value):
        for key in value:
            gate(g, 0xc1b5)
            action_to(g, key, 0x488)
            gate(g, 0x485)
        gate(g, 0xc1b5)
        action_to(g, "enter", 0x7d7)
        if g.read(0x1cf,len(value)) != value.encode("ascii"):
            raise Observation("В буфере BASIC другой ответ")
    first = prompt("title")
    answer("1")
    second = prompt("bank")
    if (first,second) != (0x23e5,0x2434):
        raise Observation("После числа игроков BASIC остался у первого INPUT")
    answer("100")
    third = prompt("before")
    if third != 0x2547:
        raise Observation("BASIC не дошёл до отдельного запроса ставки")
    answer("10")
    g.points()
    g.advance(500)
    g.frame("after")
    fourth = prompt("dealt")
    if fourth in (first, second, third):
        raise Observation("Приём ставки требует проверки по кадру раздачи")
    g.result.update(input="verified", playable="verified",
                    action_proof=dict(players=1, bank=100, bet=10, input_statements=[first,second,third,fourth]))


def play(g, record, card):
    h = record["payload_sha256"]
    g.points(*g.return_points)
    if g.watch_exit(g.sample_audio("opening", 480)):
        g.frame("before")
        return
    known_budi2 = record["file"].endswith(("/Budi2-std.exe", "/Budi2-std-fff8.exe")) and g.read(0x3a1,5)==bytes.fromhex("cd1bc8fe20")
    if (h == BUDI or known_budi2) and g.profile in ("MON","STD"):
        budi(g)
    elif h in ALMAZ_RF and g.profile == "RF":
        almaz(g)
    elif h == OCHKO and g.profile == "MON":
        ochko(g)
    else:
        if g.watch_exit(g.advance(2400)):
            g.frame("before")
            return
        title = g.frame("title")
        # Распаковщики и музыкальные заставки иногда оставляют команду NC
        # либо одноцветный экран на несколько секунд. Фиксируем дополнительное
        # ожидание по состоянию кадра, прежде чем пробовать игровые клавиши.
        for ms in (5500,12000):
            if title["colors"]>1 and title["rgb_sha256"]!=g.result["frames"]["command"]["rgb_sha256"]:
                break
            if g.watch_exit(g.advance(ms)):
                return
            title = g.frame("wait-"+str(ms))
        start = card["start_keys"] if card else []
        for key in start:
            if g.watch_exit(g.key(key)):
                return
        if g.watch_exit(g.advance(800)):
            return
        g.frame("before")
        # Контрольное окно той же длительности выявляет самостоятельную анимацию.
        if g.watch_exit(g.advance(300)):
            return
        g.frame("idle")
        for key in (card["action_keys"] if card else ["space", "enter", "1", "right"]):
            if g.watch_exit(g.key(key, 180, 120)):
                return
            if not card and key!="right":
                g.frame("probe-"+key)
        g.frame("after")
        g.result["input"] = "needs_review"
        g.result["obstacle"] = (card["expect"] if card else
            "Назначения клавиш не установлены. Сохранены кадры покоя и пробы Space, Enter, 1, Right.")
    g.points(*g.return_points)
    if g.watch_exit(g.sample_audio("level", 480)):
        return
    g.integrity("playing")
    for key in (card["exit_keys"] if card else ["clear"]):
        if h in ALMAZ_RF and g.profile == "RF":
            action_to(g, key, 0xf800, 5000)
            g.watch_exit(0xf800)
            break
        if g.watch_exit(g.key(key, 400, 300)):
            break
    if not g.exit_seen:
        g.watch_exit(g.advance(700))
    g.frame("exit-observation")
