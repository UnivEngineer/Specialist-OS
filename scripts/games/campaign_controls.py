"""Игровые признаки парных проб, привязанные к SHA-256 нагрузки."""
from campaign_guest import Image, Observation
from campaign_play import ALMAZ_RF, BUDI, OCHKO, action_to, gate

BURAN = "9c155553268a8cf822c1bd9b5179afdd50398bfc84224e98e951d03a672ce517"
BURAN_KOSM = "cd5dab8942952149b722cad38a25412a45848a43bf413562a04d09ac133be762"
ALMAZ = "97add9db4006a824aeb02763861ff47be25c99e1be235a8485a05cfe7e604e5b"
ZOO = "c4a5f493e94a70e79f5f202634cc12fec2b2e6757c067f8bbb99891e169792b0"
FOOTBALL = "9525c859e891f3b227aa0b8762d7601acc724d1f315172abb0fee9cd6aba5ae5"
COPTER = "e6d284efb14e9ceca1487f021d1a71e5039fc51b5c5b69a0adc8ff181664b10c"
TETRIS = "bb5c5f5adab526fe36de6b975d7f369d4646fb7dcd0fece532bc6a4e20e75c09"
MINI_TETRIS = "880db15bb53869da77d8c4356ebbe5ecfaf940a3122123feb5ef28f7aee0ad22"
ELITE_KEYS = {
    "8e3b7eecb3a974c7c4988fe29ed20da23f5838bf6bd0cad53cabe2a27d73661d": "n",
    "2c2cae652daa2b86fd57597dfd6f6d5ea1fd685094a67ee17752a91e483e5bd8": "t",
}
EXOLON = "87b12e88a026de1c37ddc0386c29975e6728524dc87f80b2c11b51a5d4066ff3"
STARTREK = "f2ca42b86e69fb98954a258bc89b9698b330fed37a71bfeee19e614482db33e6"
PACMAN = "b695998b36d08ab2f74241df5b52ac5732d05fb8c58a1b3d3a320a9c503cb4ac"
GOLD = {
    "eb5c99db51a997fbe51136e1715872ce6e7ec4b2a6f351477e0365d566bda919",
    "c3195ccef8aa699daeb0d6aa7d8b140cd708295065a2e22d03020bfaebf7b095",
}
STARWARS1 = {
    "593793b8f9a43ba3ae55fb497b1f172c9ba615ad28d20d5a948d69652dbec832",
    "a8ac2914135e32f47aea3c2408565974dceac8d51a3c615515c93dc911d21bab",
}
STARWARS2 = {
    "fb3cf520959842c98084963749555e0f1e9ec49771f67467e176da7ebc4db8b1": "MON",
    "98959fdba5ab812c245701418785f7e74b6da8d31f2e6b009cce2a2232082b73": "MON",
    "de9287ea25309a25bcc796246d55c1938729823ed76f4f977b9a742b78af2796": "MX",
    "b7dd022fc87f3280a7f54ea1a4d0fc87916d4f2d5cb91a80f8afa6c518f4cd5c": "RF",
}
STARWARS_FLIGHT = {
    "starwars1_turn": dict(title=0x007a, skip=0x0287, flight=0x03e8, control=0x15b9, counter=0x15ce, write=0x0452),
    "starwars2_turn": dict(title=0x00bb, skip=0x02c8, flight=0x06f1, control=0x1ba1, counter=0x1bc1, write=0x0765),
}
ALMAZ2 = ALMAZ_RF | {
    "0f1bd6550a999cea29b6e0aeceda728ed469b6dc3677d4ccb3a7acb94f1dc515",
    "915f47a821005827e8a2422c0c792f29d2c9a258a7c74679af7481a0afd84b08",
    "9709f4bdd38ccd9d2a40ad10a844959842bf060f5f7d0a7786c055e496c0ced2",
    "4099d26e24974f7aeb518bcbe0040142e71bd29ba38b66678e569d70bba7debc",
    "ac7176160b2ca42c6c400015c97369d36c499599c53026d232637ffb80d8a598",
    "7a7495de9e645f1f187542a0b3163d143061388265332eb25cf1bd65cc4cff2b",
    "88b0768d11a67fb4673bf0408bea960b29c53d03704ee1bf57d5f4decaa35a5d",
    "2211d08c76c6867cd5e79cfe2d2fa165997b2f8275d3b9084dda3aa0abc8ed4d",
    "0bb927c615fb7fdc582f9fa59d826af1458f2b8362fbf9ca678f74b4d60c73ed",
    "60efa2f0ff15dac8a1890c85df3e2b08bf719006939e3afba5d2df512067200c",
    "e0d8880622e39ce126b0e47e88ba90f347a32587290ff678d9f2122b17cf1bdd",
    "42e2327636bfd0a5a3c4bab3b3cd27fdb61d9643c777240e7bc81f1533c0a3a3",
    "df73430dd8f6d17a40aab8fedfa75ba21663da13ef0c2f888dbccb5d4b0a6713",
}
BUDI_VARIANTS = {
    BUDI: "budi2_brick",
    "5a228af63c39d2c698771129577483746fbd45a410f27d6bbbf097152116e407": "budi2_brick",
    "5358a417b9117b4c4e74fa1f62eb55a75557f3b254b4e9b384605ea173c4a25c": "budi2_brick",
    "6d4954a8e61794084457ec29c88f34a9c050c3ed6e4bacc5be9b4cfd6fe57fb9": "budi_std_brick",
    "37f5f13905bd4fa2dc4d3480f2d68628b391ec8343386920118df9a695efbb8e": "budi_std_brick",
    "569192f641e7e9bf612d8accd8cd4998c3554ba352a9947f23af0c64ba8348ff": "budi_rf_brick",
    "8e0a843af2d9a0718592bb41ca5d2e81238f4314d1ac187f36458e93240bc217": "budi_rf_brick",
}
BURAN_UP = {
    "7fe73ea47dbf6e04c5287f71cd7eeef8fbef8408e99fb73eafc7d5bbf658867c",
    "3e67785e83523a4cee267d629e5587b8d4350a0b86f2fbe7eed57b392448f0fd",
    "95435f4c04c1466a5f4ffed40694cc61819d34cb5a86bd323eb415c683f4bc39",
    "471e61f6702042ffc007b7b408297108c546a6198a679c4bd185361654260e66",
}
PROBES = {
    "buran_rocket": dict(key="space", windows=(10, 20, 40, 70), settle=120),
    "buran_up_rocket": dict(key="up", windows=(60, 120), settle=120),
    "buran_kosm_rocket": dict(key="up", windows=(60, 120, 160), settle=120),
    "budi2_brick": dict(key="space", windows=(20, 60, 100), settle=120),
    "budi_std_brick": dict(key="q", windows=(20, 60, 100), settle=120),
    "budi_rf_brick": dict(key="home", windows=(20, 60, 100), settle=120),
    "almaz_coords": dict(key="right", windows=(60, 120), settle=120),
    "almaz_player": dict(key="right", windows=(60, 120, 240), settle=120),
    "zoo_player": dict(key="right", windows=(120, 120, 120, 360), settle=120),
    "football_paddle": dict(key="right", windows=(60, 120, 120), settle=120),
    "copter_rocket": dict(key="r", windows=(40, 80, 80, 120), settle=120),
    "tetris_drop": dict(key="space", windows=(30, 50, 100), settle=120),
    "mini_tetris_turn": dict(key="rpt", windows=(120, 180, 180, 120), settle=120),
    "elite_speed": dict(key="space", windows=(60, 120, 120, 200, 400), settle=120),
    "exolon_player": dict(key="right", windows=(60, 120, 120, 300), settle=120),
    "startrek_torpedo": dict(key="enter", windows=(120,), settle=(200, 700, 1500, 3000, 5000)),
    "ochko_draw": dict(key="enter", windows=(120,), settle=(300, 700, 1200)),
    "pacman_down": dict(key="5", windows=(80, 160, 160, 200, 200), settle=120),
    "gold_right": dict(key="right", windows=(60, 120, 160, 220), settle=120),
    "starwars1_turn": dict(key="right", windows=(120, 180, 320, 600), settle=240),
    "starwars2_turn": dict(key="right", windows=(120, 180, 320, 600), settle=240, proof_revision=2),
}


def tetris_blocks(path):
    # 10×20 знакомест. Сплошной тайл занимает 112 пикселей, призрак — около 22.
    colors = {"blue_blocks": (0, 0, 192), "white_blocks": (192, 192, 192)}
    blocks = {name: [] for name in colors}
    with Image.open(path) as image:
        if image.size != (384, 256):
            return blocks
        frame = image.convert("RGB")
        for row in range(20):
            for col in range(10):
                x, y = 111 + 16 * col, 6 + 12 * row
                hist = {rgb: count for count, rgb in frame.crop((x, y, x + 16, y + 12)).getcolors(192)}
                for name, color in colors.items():
                    if hist.get(color, 0) >= 100:
                        blocks[name].append([col, row])
    return blocks


def observe(guest, probe, frame):
    observed = {}
    if probe == "buran_rocket":
        observed["rocket_8008"] = guest.read(0x8008, 1)[0]
        observed["rocket_track_8020"] = guest.read(0x8020, 2).hex()
    elif probe in ("buran_up_rocket", "buran_kosm_rocket"):
        observed["rocket_8808"] = guest.read(0x8808, 1)[0]
        observed["rocket_track_8820"] = guest.read(0x8820, 2).hex()
    elif probe in BUDI_VARIANTS.values():
        observed["brick_8853"] = guest.read(0x8853, 1)[0]
        observed["holes_8d00"] = guest.read(0x8d00, 5).hex()
    elif probe == "almaz_player":
        row = guest.read(0x81c0, 32)
        observed["map_81c0"] = row.hex()
        observed["player_cells"] = [0x81c0 + i for i, tile in enumerate(row) if tile & 15 == 9]
    elif probe == "almaz_coords":
        observed["player_xy"] = list(guest.read(0x29b9, 2))
    elif probe == "zoo_player":
        observed["player_cell"] = int.from_bytes(guest.read(0x0352, 2), "little")
        observed["mode_handler"] = int.from_bytes(guest.read(0x07ea, 2), "little")
    elif probe == "football_paddle":
        observed["ball"] = list(guest.read(0x0853, 3))
        observed["paddle"] = list(guest.read(0x0862, 3))
    elif probe == "copter_rocket":
        y, x, sprite, active = guest.read(0x001b, 4)
        observed.update(rocket_y=y, rocket_x=x, rocket_sprite=sprite, rocket_active=active)
    elif probe == "tetris_drop":
        observed.update(tetris_blocks(frame))
    elif probe == "mini_tetris_turn":
        shape, y, x = guest.read(0x60fd, 3)
        observed.update(shape=shape, figure_x=x, figure_y=y)
    elif probe == "elite_speed":
        observed["speed_808c"] = guest.read(0x808c, 1)[0]
        observed["acceleration_key_8043"] = guest.read(0x8043, 1)[0]
    elif probe == "exolon_player":
        observed["player_xy"] = list(guest.read(0x2090, 2))
    elif probe == "startrek_torpedo":
        observed["torpedo_variable"] = guest.read(0x6bbf, 6).hex()
        observed["input_statement"] = int.from_bytes(guest.read(0x0237, 2), "little")
    elif probe == "ochko_draw":
        observed["hand_variable"] = guest.read(0x2df2, 6).hex()
        observed["input_statement"] = int.from_bytes(guest.read(0x0237, 2), "little")
    elif probe == "pacman_down":
        y, x = guest.read(0x3821, 2)
        observed.update(player_xy=[x, y], score_units=guest.read(0x21c2, 1)[0] - ord("0"))
    elif probe == "gold_right":
        y, x = guest.read(0x179a, 2)
        observed["player_xy"] = [x, y]
    elif probe in STARWARS_FLIGHT:
        addresses = STARWARS_FLIGHT[probe]
        observed["turn_control"] = int.from_bytes(guest.read(addresses["control"], 1), signed=True)
        observed["turn_counter"] = guest.read(addresses["counter"], 1)[0]
        if probe == "starwars2_turn":
            observed["docking_flag"] = guest.read(0x1be2, 1)[0]
    return observed


def buran_start(guest, snapshot, _probe):
    # Исторический монитор, меню и игровой цикл цветного Buran-std.rks.
    gate(guest, 0xc1b5, 14000)
    snapshot(guest, "title")
    for index, selection in enumerate((0xb2, 0xbc), 1):
        guest.points(0x00f1)
        if guest.key("down", 500, 0) != 0x00f1:
            raise Observation("Буран не обработал стрелку вниз в меню")
        gate(guest, 0xc1b5, 5000)
        if guest.read(0x01be, 1) != bytes([selection]):
            raise Observation("В меню Бурана выделен другой пункт")
        snapshot(guest, f"menu-{index}")
    guest.points(0x0145)
    if guest.key("space", 500, 0) != 0x0145:
        raise Observation("Буран не обработал запуск игры")
    gate(guest, 0x03bc, 8000)
    if guest.read(0x8008, 1) != b"\x00":
        raise Observation("Буран начинает пробу с уже запущенной ракетой")


def budi_dig_start(guest, snapshot, probe):
    # Вход RAMFOS 0000h ведёт на запрос диапазона, минуя старый цикл 0070h.
    gate(guest, 0x0120 if probe == "budi_rf_brick" else 0x02a0, 30000)
    snapshot(guest, "title")
    if probe == "budi_rf_brick":
        action_to(guest, "0", 0x012d)
    elif probe == "budi2_brick":
        guest.points(0x03a1)
        guest.key("space", 500, 0)
    gate(guest, 0x03a1, 40000)
    if guest.regs()["hl"] != 0x8852:
        raise Observation("BUDI не дошёл до начальной клетки игрока 8852h")
    snapshot(guest, "first-level")
    action_to(guest, "right", 0x0485)
    gate(guest, 0x03a1)
    if guest.regs()["hl"] != 0x886e or guest.read(0x8852, 2) != b"\x00\x09":
        raise Observation("В BUDI игрок или кирпич вне исходной позиции для копания")


def almaz_move_start(guest, snapshot, probe):
    gate(guest, 0x0038, 45000)
    observed = snapshot(guest, "first-level", probe="almaz_player")
    if observed["player_cells"] != [0x81c1]:
        raise Observation("Первый уровень Алмаза-2 начинается с другой карты")


def zoo_start(guest, snapshot, probe):
    gate(guest, 0x0864)
    if guest.read(0x07ea, 2) != b"\x87\x08":
        raise Observation("В ZOO не достигнута демонстрация с приглашением начать игру")
    snapshot(guest, "demo", probe=probe)
    action_to(guest, "space", 0x087a)
    gate(guest, 0x07c2)
    gate(guest, 0x07f2)
    stage = snapshot(guest, "first-level", probe=probe)
    if stage["player_cell"] != 0x141e or stage["mode_handler"] != 0x0398:
        raise Observation("В ZOO не подтверждены начало уровня и позиция игрока 141Eh")


def copter_start(guest, snapshot, probe):
    gate(guest, 0x059c)
    snapshot(guest, "class-prompt")
    action_to(guest, "2", 0x05d3)
    gate(guest, 0x05c4)
    # Начальная ракета задана самой нагрузкой. Дождаться сброса флага по её собственному циклу.
    guest.points()
    for _ in range(10):
        if guest.read(0x001e, 1) == b"\x00":
            break
        guest.advance(250)
    gate(guest, 0x0344)
    stage = snapshot(guest, "first-level", probe=probe)
    if stage["rocket_active"] != 0:
        raise Observation("Начало пробы Copter уже содержит запущенную ракету")


def elite_start(guest, snapshot, probe):
    guest.points()
    guest.advance(10000)
    snapshot(guest, "commander-prompt")
    new_commander = ELITE_KEYS[guest.result["payload_sha256"]]
    guest.key(new_commander, 500, 800)
    snapshot(guest, "fire-or-space")
    guest.key("space", 500, 800)
    snapshot(guest, "docked")
    guest.key("f1", 500, 800)
    snapshot(guest, "launch-animation")
    guest.advance(3600)
    if guest.read(0x2748, 3) != b"\x32\x8c\x80" or guest.read(0x808c, 1) != b"\x0c":
        raise Observation("В Elite не подтверждён выход на полёт с начальной скоростью 0Ch")


def exolon_start(guest, snapshot, probe):
    gate(guest, 0x7b00, 20000)
    snapshot(guest, "menu")
    guest.points()
    guest.key("1", 500, 1500)
    for delay in (400, 600, 1200, 2000):
        guest.advance(delay)
    gate(guest, 0x7557)
    stage = snapshot(guest, "first-level", probe=probe)
    if stage["player_xy"] != [0x10, 0x70] or guest.read(0x1fbd, 3) != b"\x22\x90\x20":
        raise Observation("В Exolon не подтверждена начальная позиция игрока")


def startrek_start(guest, snapshot, probe):
    def character(key, value, delay=700):
        action_to(guest, key, 0x1982, 3500)
        if guest.regs()["af"] >> 8 != value:
            raise Observation("Строковый ввод StarTrek вернул другой код клавиши")
        guest.points()
        guest.advance(delay)

    gate(guest, 0x197f)
    guest.advance(120)
    snapshot(guest, "intro")
    character("enter", 0x0d)
    for letter in "spok":
        character(letter, ord(letter.upper()))
    character("enter", 0x0d, 7000)
    guest.advance(26000)
    snapshot(guest, "orders")
    guest.key("caps", 2000, 2500)
    character("d", 0x64, 2500)
    character("enter", 0x0d, 20000)
    snapshot(guest, "command-prompt", probe=probe)
    character("t", 0x74)
    character("enter", 0x0d, 3500)
    snapshot(guest, "course-prompt", probe=probe)
    character("1", 0x31)
    if guest.read(0x0237, 2) != b"\xe2\x45" or guest.read(0x6bbf, 6) != bytes.fromhex("005000002084"):
        raise Observation("Перед пуском StarTrek не подтверждены курс и запас из 10 торпед")


def ochko_start(guest, snapshot, probe):
    def ask(name, statement):
        gate(guest, 0x0485, 30000)
        if int.from_bytes(guest.read(0x0237, 2), "little") != statement:
            raise Observation("В «Очко» интерпретатор дошёл до другого оператора INPUT")
        snapshot(guest, name, probe=probe)

    def type_answer(value, submit=True):
        for key in value:
            gate(guest, 0xc1b5)
            action_to(guest, key, 0x0488)
            gate(guest, 0x0485)
        gate(guest, 0xc1b5)
        if submit:
            action_to(guest, "enter", 0x07d7)

    for name, statement, answer in (("players", 0x23e5, "1"), ("bank", 0x2434, "100"), ("bet", 0x2547, "10")):
        ask(name, statement)
        type_answer(answer)
    ask("another-card", 0x2615)
    type_answer("d", submit=False)
    if guest.read(0x2df2, 6) != bytes.fromhex("005600006084"):
        raise Observation("Перед добором «Очко» не подтверждены исходные 14 очков")


def pacman_start(guest, snapshot, probe):
    guest.points()
    guest.advance(4000)
    snapshot(guest, "press-zero")
    guest.key("0", 180, 320)
    snapshot(guest, "menu")
    guest.key("p", 180, 320)
    guest.advance(4000)
    stage = snapshot(guest, "maze", probe=probe)
    if stage["player_xy"] != [0xaa, 0x78] or stage["score_units"] != 0:
        raise Observation("В Pacman не подтверждена начальная позиция на свободном коридоре")


def starwars_start(guest, snapshot, probe):
    addresses = STARWARS_FLIGHT[probe]
    guest.points()
    guest.advance(2880)
    snapshot(guest, "rank")
    guest.key("1")
    gate(guest, addresses["title"], 12000)
    snapshot(guest, "logo")
    action_to(guest, "space", addresses["skip"], 8000)
    gate(guest, addresses["flight"], 16000)
    stage = snapshot(guest, "crosshair", probe=probe)
    if stage.get("docking_flag", 0) != 0 or not -0x10 <= stage["turn_control"] <= 0x10:
        raise Observation("StarWars не дошла до свободного полёта")


def buran_kosm_start(guest, snapshot, probe):
    guest.points()
    guest.advance(2880)
    snapshot(guest, "title")
    guest.key("0")
    snapshot(guest, "instructions-skipped")
    guest.key("1")
    gate(guest, 0x041d, 30000)
    snapshot(guest, "aim", probe=probe)


PREPARE = {
    "buran_rocket": buran_start, "almaz_player": almaz_move_start, "zoo_player": zoo_start,
    "buran_kosm_rocket": buran_kosm_start,
    "copter_rocket": copter_start,
    "elite_speed": elite_start, "exolon_player": exolon_start,
    "startrek_torpedo": startrek_start,
    "ochko_draw": ochko_start,
    "pacman_down": pacman_start,
    **dict.fromkeys(STARWARS_FLIGHT, starwars_start),
    **dict.fromkeys(BUDI_VARIANTS.values(), budi_dig_start),
}


def game_probe(job, scenario):
    payload = job["record"]["payload_sha256"]
    profile = job["profile"]
    if profile == "MON" and payload == BURAN:
        return "buran_rocket"
    if profile in ("MON", "RF") and payload == BURAN_KOSM:
        return "buran_kosm_rocket"
    if profile in ("MON", "RF") and payload in BURAN_UP and scenario["action_keys"] == ["up"]:
        return "buran_up_rocket"
    budi = BUDI_VARIANTS.get(payload)
    if (budi == "budi_rf_brick" and profile == "RF") or (budi in ("budi_std_brick", "budi2_brick") and profile == "MON"):
        return budi
    if profile == "MON" and payload == ALMAZ:
        return "almaz_coords"
    if profile == "MON" and payload == ZOO:
        return "zoo_player"
    if profile == "MON" and payload == FOOTBALL:
        return "football_paddle"
    if profile == "MON" and payload == COPTER:
        return "copter_rocket"
    if profile == "MON" and payload == TETRIS and scenario["start_keys"] == ["enter"]:
        return "tetris_drop"
    if profile == "MON" and payload == MINI_TETRIS and scenario["start_keys"] == ["5"]:
        return "mini_tetris_turn"
    if profile == "MON" and payload in ELITE_KEYS:
        return "elite_speed"
    if profile == "MX" and payload == EXOLON:
        return "exolon_player"
    if profile == "MON" and payload == STARTREK:
        return "startrek_torpedo"
    if profile == "MON" and payload == OCHKO:
        return "ochko_draw"
    if profile == "MON" and payload == PACMAN:
        return "pacman_down"
    if profile == "MON" and payload in GOLD:
        return "gold_right"
    if profile == "MON" and payload in STARWARS1:
        return "starwars1_turn"
    if STARWARS2.get(payload) == profile:
        return "starwars2_turn"
    if payload in ALMAZ2:
        return "almaz_player"
    return None


def object_proof(probe, phases, checkpoints):
    if probe in ("buran_rocket", "buran_up_rocket", "buran_kosm_rocket"):
        address, pc = (0x8008, 0x03d0) if probe == "buran_rocket" else (0x8808, 0x0431)
        field = f"rocket_{address:04x}"
        track = []
        for phase, view in phases.items():
            values = [cp[phase][field] for cp in checkpoints]
            if values == [0, 0, 1, 1] and view["stable_delta_pixels"] == 1:
                x, y, _, _ = view["roi_exclusive"]
                track.append(dict(phase=phase, x=x, y=y, values=values))
        if (checkpoints[0]["before"][field] == 0 and len(track) >= 2
                and all(a["x"] == b["x"] and a["y"] > b["y"] for a, b in zip(track, track[1:]))):
            return dict(game_object="Ракета Бурана", action=PROBES[probe]["key"], address=address, increment_pc=pc,
                        before=0, track=track)
    elif probe in BUDI_VARIANTS.values():
        changes = []
        for phase, view in phases.items():
            values = [cp[phase]["brick_8853"] for cp in checkpoints]
            if values == [9, 9, 0x48, 0x48] and view["stable_delta_pixels"]:
                changes.append(dict(phase=phase, values=values, hole_records=[cp[phase]["holes_8d00"] for cp in checkpoints]))
        if changes:
            return dict(game_object="Кирпич в первом уровне BUDI", action=PROBES[probe]["key"], address=0x8853,
                        before=9, after=0x48, changes=changes)
    elif probe == "almaz_player":
        movement = []
        for phase, view in phases.items():
            positions = [cp[phase]["player_cells"] for cp in checkpoints]
            left, right = positions[2:]
            if (positions[:2] == [[0x81c1], [0x81c1]] and left == right and len(left) == 1
                    and 0x81c1 < left[0] <= 0x81c5 and view["stable_delta_pixels"]):
                movement.append(dict(phase=phase, positions=positions))
        if movement:
            return dict(game_object="Игрок на карте Алмаза-2", action="right", map_address=0x81c0,
                        start_cell=0x81c1, tile=9, movement=movement)
    elif probe == "almaz_coords":
        movement = []
        start = checkpoints[0]["before"]["player_xy"]
        for phase, view in phases.items():
            positions = [cp[phase]["player_xy"] for cp in checkpoints]
            x, y = positions[2]
            if (positions[:2] == [start, start] and positions[2] == positions[3]
                    and y == start[1] and 0 < x - start[0] <= 5 and view["stable_delta_pixels"]):
                movement.append(dict(phase=phase, positions=positions))
        if movement:
            return dict(game_object="Игрок на карте Алмаза", action="right", address=0x29b9,
                        start_xy=start, x_update_pc=0x2384, movement=movement)
    elif probe == "zoo_player":
        movement = []
        for phase, view in phases.items():
            values = [cp[phase]["player_cell"] for cp in checkpoints]
            handlers = [cp[phase]["mode_handler"] for cp in checkpoints]
            if (values[:2] == [0x141e, 0x141e] and values[2] == values[3] and values[2] in (0x142e, 0x143e)
                    and handlers == [0x0398] * 4 and view["stable_delta_pixels"]):
                movement.append(dict(phase=phase, values=values))
        if movement:
            return dict(game_object="Игрок в первом уровне ZOO", action="right", address=0x0352,
                        start_cell=0x141e, step=0x10, movement=movement)
    elif probe == "football_paddle":
        start = checkpoints[0]["before"]["paddle"]
        movement = []
        for phase, view in phases.items():
            paddles = [cp[phase]["paddle"] for cp in checkpoints]
            ball = [cp[phase]["ball"] for cp in checkpoints]
            if (paddles[:2] == [start, start] and paddles[2] == paddles[3] and paddles[2][0] == start[0]
                    and paddles[2][1:] != start[1:] and view["stable_delta_pixels"]):
                movement.append(dict(phase=phase, paddles=paddles, ball=ball))
        if movement:
            return dict(game_object="Ракетка на поле Football", action="right", screen_address=0x0862,
                        mask_address=0x0864, x_update_pc=0x05e9, start=start, movement=movement)
    elif probe == "copter_rocket":
        flight = []
        for phase, view in phases.items():
            active = [cp[phase]["rocket_active"] for cp in checkpoints]
            y = [cp[phase]["rocket_y"] for cp in checkpoints]
            x = [cp[phase]["rocket_x"] for cp in checkpoints]
            if active == [0, 0, 1, 1] and x[2] == x[3] and y[2] == y[3] and view["stable_delta_pixels"]:
                flight.append(dict(phase=phase, active=active, x=x[2], y=y[2]))
        if len(flight) >= 2 and any(a["x"] == b["x"] and a["y"] > b["y"] for a, b in zip(flight, flight[1:])):
            return dict(game_object="Ракета Copter", action="r", active_address=0x001e,
                        coordinate_address=0x001b, launch_pc=0x0388, flight=flight)
    elif probe == "tetris_drop":
        start = checkpoints[0]["before"]["blue_blocks"]
        if len(start) != 4 or any(row > 4 for _, row in start):
            return None
        offset = 19 - max(row for _, row in start)
        shifted = [[col, row + offset] for col, row in start]
        drops = []
        for phase, view in phases.items():
            idle = checkpoints[0][phase]["blue_blocks"]
            placed = checkpoints[2][phase]["blue_blocks"]
            next_piece = checkpoints[2][phase]["white_blocks"]
            if (len(idle) == 4 and all(row <= 7 for _, row in idle) and placed == shifted
                    and len(next_piece) == 4 and all(row <= 4 for _, row in next_piece)
                    and view["stable_delta_pixels"]):
                drops.append(dict(phase=phase, idle=idle, placed=placed, next_piece=next_piece))
        if drops:
            return dict(game_object="Первая фигура Tetris", action="space", start=start, drops=drops,
                        proof_kind="tile_grid", board_region_exclusive=[111, 6, 271, 246], tile_size=[16, 12])
    elif probe == "mini_tetris_turn":
        before = checkpoints[0]["before"]
        shape = before["shape"]
        if not (0 <= shape <= 0x6c and shape % 4 == 0 and 0xb0 <= before["figure_x"] <= 0xb9):
            return None
        turns = []
        for phase, view in phases.items():
            shapes = [cp[phase]["shape"] for cp in checkpoints]
            aligned = all(cp[phase]["figure_x"] == before["figure_x"] for cp in checkpoints)
            if (aligned and shapes[:2] == [shape, shape] and shapes[2] == shapes[3]
                    and shapes[2] >> 4 == shape >> 4 and shapes[2] != shape and view["stable_delta_pixels"]):
                turns.append(dict(phase=phase, shapes=shapes))
        if turns:
            return dict(game_object="Фигура мини-тетриса", action="rpt", shape_address=0x60fd,
                        rotation_pc=0x63f0, initial_shape=shape, turns=turns)
    elif probe == "elite_speed":
        samples = []
        for phase, view in phases.items():
            speeds = [cp[phase]["speed_808c"] for cp in checkpoints]
            key = [cp[phase]["acceleration_key_8043"] for cp in checkpoints]
            if (speeds[:2] == [0x0c, 0x0c] and 0x0c < speeds[2] == speeds[3] <= 0x28
                    and view["stable_delta_pixels"]):
                samples.append(dict(phase=phase, speeds=speeds, acceleration_key=key))
        if samples:
            return dict(game_object="Движение корабля в Elite", action="space", speed_address=0x808c,
                        speed_write_pc=0x2748, start_speed=0x0c, samples=samples)
    elif probe == "exolon_player":
        movement = []
        for phase, view in phases.items():
            positions = [cp[phase]["player_xy"] for cp in checkpoints]
            x, y = positions[2]
            if (positions[:2] == [[0x10, 0x70], [0x10, 0x70]] and positions[2] == positions[3]
                    and y == 0x70 and 0x10 < x <= 0x20 and view["stable_delta_pixels"]):
                movement.append(dict(phase=phase, positions=positions))
        if movement:
            return dict(game_object="Персонаж в первом уровне Exolon", action="right", coord_address=0x2090,
                        coord_write_pc=0x1fbd, start_xy=[0x10, 0x70], movement=movement)
    elif probe == "startrek_torpedo":
        flight = []
        for phase, view in phases.items():
            amounts = [cp[phase]["torpedo_variable"] for cp in checkpoints]
            if amounts == ["005000002084"] * 2 + ["005000001084"] * 2 and view["stable_delta_pixels"]:
                flight.append(dict(phase=phase, variable_values=amounts,
                                   input_statements=[cp[phase]["input_statement"] for cp in checkpoints]))
        if flight:
            return dict(game_object="Фотонная торпеда StarTrek", action="enter", course=1,
                        proof_kind="interpreter_variable", basic_variable="P", variable_address=0x6bc1,
                        start_amount=10, remaining=9, flight=flight)
    elif probe == "ochko_draw":
        draws = []
        for phase, view in phases.items():
            sums = [cp[phase]["hand_variable"] for cp in checkpoints]
            if sums == ["005600006084"] * 2 + ["005600004085"] * 2 and view["stable_delta_pixels"]:
                draws.append(dict(phase=phase, variable_values=sums,
                                  input_statements=[cp[phase]["input_statement"] for cp in checkpoints]))
        if draws:
            return dict(game_object="Дополнительная карта в «Очко»", action="enter", answer="D",
                        proof_kind="interpreter_variable", basic_variable="V", variable_address=0x2df4,
                        score_before=14, score_after=24, draws=draws)
    elif probe == "pacman_down":
        movement = []
        for phase, view in phases.items():
            positions = [cp[phase]["player_xy"] for cp in checkpoints]
            scores = [cp[phase]["score_units"] for cp in checkpoints]
            x, y = positions[2]
            if (positions[:2] == [[0xaa, 0x78], [0xaa, 0x78]] and positions[2] == positions[3]
                    and x == 0xaa and 0x78 < y <= 0x90 and view["stable_delta_pixels"]):
                movement.append(dict(phase=phase, positions=positions, score_units=scores))
        if movement:
            return dict(game_object="Колобок на поле Pacman", action="5", screen_address=0x3821,
                        score_units_address=0x21c2, start_xy=[0xaa, 0x78], movement=movement)
    elif probe == "gold_right":
        start = checkpoints[0]["before"]["player_xy"]
        movement = []
        for phase, view in phases.items():
            positions = [cp[phase]["player_xy"] for cp in checkpoints]
            x, y = positions[2]
            if (positions[:2] == [start, start] and positions[2] == positions[3] and y == start[1]
                    and 0 < x - start[0] <= 0x30 and view["stable_delta_pixels"]):
                movement.append(dict(phase=phase, positions=positions))
        if movement:
            return dict(game_object="Персонаж на уровне Gold", action="right", x_address=0x179b,
                        y_address=0x179a, x_update_pc=0x1339, start_xy=start, movement=movement)
    elif probe in STARWARS_FLIGHT:
        addresses = STARWARS_FLIGHT[probe]
        before = checkpoints[0]["before"]
        turns = []
        for phase, view in phases.items():
            controls = [cp[phase]["turn_control"] for cp in checkpoints]
            angles = [cp[phase]["turn_counter"] for cp in checkpoints]
            open_space = all(cp[phase].get("docking_flag", 0) == 0 for cp in checkpoints)
            if (open_space and controls[:2] == [before["turn_control"]] * 2 and angles[0] == angles[1]
                    and 1 <= (angles[0] - angles[2]) % 256 <= 0x30
                    and angles[2] == angles[3] and -0x10 <= controls[2] == controls[3] < controls[0]
                    and view["stable_delta_pixels"]):
                turns.append(dict(phase=phase, turn_control=controls, turn_counter=angles))
        if turns:
            edition = "StarWars" if probe == "starwars1_turn" else "StarWars2"
            return dict(game_object=f"Корабль в свободном полёте {edition}", action="right",
                        controls_address=addresses["control"], turn_counter_address=addresses["counter"],
                        counter_write_pc=addresses["write"],
                        before=before["turn_counter"], turns=turns)
    return None
