"""Статические проверки исходного проекта Almaz2 без запуска эмуляторов.

Сначала: powershell.exe -NoProfile -File scripts/build/build-almaz2.ps1 All
Затем: python scripts/games/check-almaz2-source.py [--pit-clock-hz 2500000]
"""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from math import gcd


ROOT = Path(__file__).resolve().parents[2]
TABLES = (
    ("t_sound_stone", 0x09D4, 1), ("t_sound_smoke", 0x09F5, 3),
    ("t_sound_step", 0x0AF6, 0), ("t_sound_diamond", 0x0AF9, 2),
    ("t_sound_diamond_explosion", 0x0AFE, 3),
    ("t_music_intro", 0x0B13, 4), ("t_music_level", 0x0B51, 4),
)
PALETTE = bytes.fromhex("00 10 48 70 40 b0 e4 f0 50 a0 20 80 00 10 48 70")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def word(data, address):
    return int.from_bytes(data[address:address + 2], "little")


def load(stem):
    data = (ROOT / "bin" / (stem + ".exe")).read_bytes()
    symbols = dict((name, int(value, 16)) for name, value in re.findall(
        r"(?m)^(\w+): EQU 0x([0-9A-Fa-f]+)",
        (ROOT / "bin" / (stem + ".sym")).read_text()))
    return data, symbols


def calls_from_listing(stem):
    listing = (ROOT / "bin" / (stem + ".lst")).read_text(encoding="utf-8")
    return [int(address, 16) for address in re.findall(
        r"(?im)^\s*\d+\+?\s+([0-9a-f]{4})\s+(?:[0-9a-f]{2}\s+)+>?\s*call\b", listing)]


def check_time_print_stack(data, symbols, calls):
    # timer_print_value снимает BC, положенный timer_draw. CALL прямо
    # в этот хвост снимет адрес возврата, затем RET уйдёт в сохранённый HL.
    tail = symbols["timer_print_value"]
    require(data[0x0163:0x0165] == bytes.fromhex("c1 c9"), "TIME tail stack contract changed")
    require(all(word(data, address + 1) != tail for address in calls),
            "Direct CALL into TIME tail consumes the return address as BC")
    if symbols["app_realtime_enabled"]:
        bridge = symbols["level_timer_print"]
        require(data[bridge:bridge + 4] == bytes((0xC5, 0xC3, tail & 255, tail >> 8)),
                "Standalone TIME printer must PUSH BC and JMP to shared tail")
        reset_calls = [address for address in calls if symbols["level_timer_reset"] <= address < bridge]
        require(any(word(data, address + 1) == bridge for address in reset_calls),
                "TIME reset does not use the balanced stack adapter")


def events(reference, start):
    result = []
    while reference[start] != 0xFF:
        delay = reference[start]
        start += 1
        if delay:
            result.append((delay, reference[start]))
            start += 1
        else:
            result.append((0, 0))
    return result


def check_hud_panels(data, symbols, color):
    u16 = lambda value: value.to_bytes(2, "little")
    final_char = symbols["hud_time_last_char"]
    require(data[0x0160] == 0xCD and word(data, 0x0161) == final_char,
            "Last TIME digit does not append a blank through the HUD adapter")
    printer = symbols["hud_print_char" if color else "app_bios_print_char"]
    require(data[final_char:final_char+8] == b"\xcd" + u16(printer) + b"\x0e\x20\xc3" + u16(printer),
            "TIME adapter must print the digit, tail-print a blank and keep the stack balanced")
    panel = symbols["hud_clear_panels"]
    position = symbols["hud_screen_position"]
    require(data[position:position+6] == b"\xcd" + u16(panel) + bytes.fromhex("21 0b 00"),
            "HUD panels must be cleared before SCREEN sets its cursor")
    rectangle = symbols["hud_clear_rectangle"]
    body = data[panel:rectangle]
    require(body[:4] == bytes.fromhex("f5 c5 d5 e5") and body[-5:] == bytes.fromhex("e1 d1 c1 f1 c9"),
            "Panel clearing must preserve all registers and flags")
    written = set()
    for start, width in ((0x9403, 8), (0xA403, 9)):
        require(b"\x21" + u16(start) + b"\x06" + bytes((width,)) + b"\xcd" + u16(rectangle) in body,
                "HUD rectangle position or width differs")
        written.update(start+column*256+row for column in range(width) for row in range(10))
    require(len(written) == 170 and all(0x9000 <= a < 0xC000 and 3 <= a % 256 <= 12 for a in written),
            "HUD clearing reaches gameplay rows or leaves the screen")
    clear = (b"\xaf\x0e\x0a\x77\x2c\x0d\xc2" + u16(symbols["hud_clear_row"]) +
             b"\x2e\x03\x24\x05\xc2" + u16(symbols["hud_clear_column"]) + b"\xc9")
    require(data[rectangle:rectangle+len(clear)] == clear,
            "HUD clear loop must write zero over ten rows without carrying into another column")


def check_sparse_animation(data, symbols):
    # При малом числе объектов поиск по 64 адресам не должен участвовать
    # в выборе: все кандидаты рисуются в исходных точках логики текущим банком.
    budget = symbols["app_animation_quota"]
    direct = symbols["v_animation_direct"]
    count = symbols["v_animation_static_count"]
    left = symbols["v_animation_quota_left"]
    quota = symbols["v_animation_quota"]
    u16 = lambda value: value.to_bytes(2, "little")
    prefix = data[symbols["animation_step"]:symbols["animation_scan"]]
    selection = (b"\xaf\x32" + u16(direct) + b"\x3a" + u16(count) + b"\xb7\xc8\x47" +
                 b"\xaf\x32" + u16(count) + b"\x3a" + u16(quota) +
                 b"\xb8\xd2" + u16(symbols["animation_direct_step"]))
    require(selection in prefix,
            "Sparse animation must use direct mode when count <= current quota")
    require(b"\xaf\x32" + u16(count) in prefix, "Static census not reset for next pass")
    start = symbols["animation_direct_step"]
    require(data[start:start+6] == b"\x3e\x01\x32" + u16(direct) + b"\xc9",
            "Direct mode changes the sprite bank or enters the map search")
    body = data[symbols["animation_static_draw"]:symbols["animation_static_immediate"]]
    counting = (b"\x3a" + u16(count) + b"\xfe" + bytes((budget+1,)) +
                b"\xd2" + u16(symbols["animation_static_counted"]) + b"\x3c\x32" + u16(count))
    require(counting in body, "Static census saturation differs")
    deferred = symbols["animation_static_deferred"]
    guarded_draw = (b"\x3a" + u16(left) + b"\xb7\xca" + u16(deferred) +
                    b"\x3d\x32" + u16(left) + b"\xf1\xc3" + u16(symbols["tile_draw"]))
    require(guarded_draw in body, "Direct animation exceeds quota or does not restore AF")
    require(b"\x3a" + u16(direct) + b"\xb7\xca" + u16(deferred) in body,
            "Dense mode draws static objects twice")
    require(data[deferred:deferred+2] == b"\xf1\xc9", "Deferred static call unbalances stack")
    banks = [word(data, address+1) for address in (0x0043, 0x0062, 0x0083, 0x00A4)]
    require(banks == [0x0D80, 0x0F00, 0x1080, 0x0C00], "Normal step banks do not advance")
    # Регрессия: любой набор, помещающийся даже в минимальную квоту,
    # обновляется во всех последовательных проходах с четырьмя фазами.
    # Внезапный рост числа кандидатов не может превысить текущую квоту.
    minimum = symbols["app_animation_min_quota"]
    for dynamic in range(346):
        quota = max(budget-dynamic, minimum)
        for previous in range(budget+2):
            direct_mode = 0 < previous <= quota
            for actual in (0, 3, quota, quota+1, 345):
                drawn = min(actual, quota) if direct_mode else 0
                require(drawn <= quota, "Sparse transition exceeded quota")
                if 0 < previous == actual <= quota:
                    require(drawn == actual, "Fitting static set skipped a frame")
    for offset in range(4):
        phases = [(step+offset) % 4 for step in range(16)]
        require(all((b-a) % 4 == 1 for a, b in zip(phases, phases[1:])),
                "Sparse object phase does not advance every step")


def check_fast_logic(data, symbols, reference, pit, buffered, quota, calls):
    enabled = bool(symbols["app_fast_enabled"])
    if not enabled:
        require(data[0x0271] == 0xCD and "fast_cell_update" not in symbols,
                "Disabled optimization must keep the original cell entry")
        return
    u16 = lambda value: value.to_bytes(2, "little")
    require(data[0x0271:0x0274] == b"\xc3" + u16(symbols["fast_cell_update"]),
            "Fast dispatcher entry hook differs")
    require([a for a in calls if word(data, a+1) == symbols["cell_update"]] ==
            [0x0054, 0x0075, 0x0096, 0x00B7], "Fast dispatcher called outside the bounded game scans")
    require(data[0x0308:0x03C2] == reference[0x0308:0x03C2] and
            data[0x0495:0x054A] == reference[0x0495:0x054A] and
            data[0x054D:0x0557] == reference[0x054D:0x0557],
            "Original gravity, rolling or movement-state fallback was modified")
    pointer, flags = symbols["v_fast_cell_pointer"], symbols["v_cell_flags"]
    services = ((b"\xcd" + u16(symbols["audio_update"])) if pit else b"")
    if buffered:
        services += b"\xcd" + u16(symbols["input_service"])
    prefix = (services + bytes.fromhex("79 0f 0f 0f 6f e6 01 f6 80 67 7d e6 e0 b0 6f 22") +
              u16(pointer) + bytes.fromhex("7e 5f e6 f0 32") + u16(flags) + bytes.fromhex("7b e6 0f 87 21") +
              u16(symbols["t_fast_handlers"]) + bytes.fromhex("85 6f 5e 23 56 eb e9"))
    start = symbols["fast_cell_update"]
    require(data[start:start+len(prefix)] == prefix,
            "Fast cell read, type dispatch or per-cell sound/input service differs")
    for y in range(1, 16):
        for x in range(1, 24):
            rotated = ((y >> 3) | (y << 5)) & 255
            packed = (((rotated & 1) | 0x80) << 8) | ((rotated & 0xE0) | x)
            require(packed == 0x8000+y*32+x, "Fast map pointer differs from original addressing")
    supported = (b"\x2a" + u16(pointer) + bytes.fromhex("11 1f 00 19 7e e6 0f c8 23 7e e6 0f c8 23 7e e6 0f c9"))
    start = symbols["fast_supported"]
    require(data[start:start+len(supported)] == supported, "Support guard must only read three lower cells")
    for label, original, fallback in (("fast_rock_update", "rock_update", "rock_check_below"),
                                       ("fast_diamond_update", "diamond_update", "diamond_check_below")):
        body = (b"\x3a" + u16(flags) + b"\xb7\xc2" + u16(symbols[original]) +
                b"\xcd" + u16(symbols["fast_supported"]) + b"\xca" + u16(symbols[fallback]))
        start = symbols[label]
        require(data[start:start+len(body)] == body, "Stateful or unsupported objects bypass original physics")
    start = symbols["fast_diamond_update"] + len(body)
    require(data[start:start+5] == b"\x3e\x05\xc3" + u16(symbols["animation_static_draw" if quota else "tile_draw"]),
            "Supported diamond skipped its animation quota hook")
    require(data[symbols["fast_no_update"]] == 0xC9, "Supported rock must not write unchanged map state")
    table = symbols["t_fast_handlers"]
    targets = ("smoke_update", "exit_update", "fast_no_update", "fast_no_update", "fast_rock_update",
               "fast_diamond_update", "square_update", "butterfly_update", "fungus_update", "player_update")
    targets += ("fast_no_update",)*6
    require(table % 32 == 0 and [word(data, table+2*t) for t in range(16)] == [symbols[n] for n in targets],
            "Fast handler table differs")
    # Дифференциальное условие: при состоянии объекта 0 исходная физика
    # может сдвинуть его только в свободную нижнюю клетку либо через свободную
    # диагональ. Три занятые нижние клетки исключают все три варианта;
    # поддержка, враги и старшие флаги соседей не меняют этого условия.
    for below in range(256):
        for left in range(16):
            for right in range(16):
                accelerated = all(v & 15 for v in (left, below, right))
                if accelerated:
                    for side_left, side_right in ((0, 0), (0, 3), (3, 0), (3, 3)):
                        # Независимое правило исходных обработчиков: сначала
                        # падение, затем левое перекатывание, затем правое.
                        if below & 15 == 0:
                            original_action = "fall"
                        elif left == 0 and side_left == 0:
                            original_action = "roll_left"
                        elif right == 0 and side_right == 0:
                            original_action = "roll_right"
                        else:
                            original_action = "stay"
                        require(original_action == "stay", "Fast support guard suppressed a physical action")


def check_command_input(data, symbols, matrix, pit, calls):
    enabled = bool(symbols["app_optimizations_enabled"])
    if not enabled:
        require("input_read_command" not in symbols and
                word(data, 0x0166) == symbols["app_bios_keycheck"], "Original command scan was not restored")
        return
    target = symbols["input_read_command"]
    require(word(data, 0x0166) == symbols["input_game_command"], "Game commands still use the general BIOS scanner")
    permitted = {symbols["music_keycheck"]} if pit else set()
    require({address for address in calls if word(data, address+1) == symbols["app_bios_keycheck"]} == permitted,
            "Only the historical music player may call the general nonblocking BIOS scanner")
    if not pit:
        require(word(data, 0x09AA) == target, "Software sound abort does not use the command scanner")
    u16 = lambda name: symbols[name].to_bytes(2, "little")
    # Вызов из игры и блокирующий ввод редактора сходятся в одном dispatch.
    # Из музыки/эффектов чистый сканер не совершает переходов в редактор.
    at = symbols["input_game_command"]
    require(data[at:at+6] == b"\xcd" + u16("input_read_command") + b"\xc3" + u16("input_editor_dispatch"),
            "Game editor dispatch is not isolated from sound callbacks")
    at = symbols["input_editor_getch"]
    require(data[at:at+3] == b"\xcd" + u16("app_bios_getch"), "Editor lost blocking BIOS input")
    at = symbols["input_editor_dispatch"]
    dispatch = (bytes((0xFE, symbols["app_key_editor_new"], 0xCA)) + u16("game_new_editor_map") +
                bytes((0xFE, symbols["app_key_editor_current"], 0xCA)) + u16("game_edit_current_map") + b"\xc9")
    require(data[at:at+len(dispatch)] == dispatch, "Editor keys can leak into tile selection")
    at = symbols["input_editor_release"]
    release = b"\xcd" + u16("input_read_command") + bytes.fromhex("fe ff c2") + u16("input_editor_release") + b"\xc9"
    require(data[at:at+len(release)] == release, "Editor entry does not wait for key release")
    for entry, adapter, destination in ((3, "editor_new", "editor_create_map"), (6, "editor_existing", "editor_open")):
        name = ("audio_" if pit else "input_") + adapter
        require(data[entry] == 0xC3 and word(data, entry+1) == symbols[name], "Editor entry adapter missing")
        at = symbols[name]
        body = bytes.fromhex("31 ff 7f")
        if pit:
            body += b"\xcd" + u16("audio_init")
        body += b"\xcd" + u16("input_editor_release") + b"\xcd" + u16("input_initialize") + b"\xc3" + u16(destination)
        require(data[at:at+len(body)] == body, "Editor transition can leak stack, held keys or sound")
    if pit:
        at = symbols["audio_editor_getch"]
        require(data[at:at+6] == b"\xcd" + u16("audio_stop") + b"\xc3" + u16("input_editor_getch"),
                "PIT editor input bypasses editor hotkeys")
    require(word(data, 0x0819) == symbols["audio_editor_getch" if pit else "input_editor_getch"],
            "Editor blocking-input hook missing")
    expected = (bytes.fromhex("3e 82 32 e3 ff 3a e1 ff f6 03 fe ff c8 c5 06 ff 3e 0f 32 e2 ff 3e fe 32 e0 ff 3a e1 ff 4f e6 40 c2") +
                u16("input_command_exit") + bytes.fromhex("06 2d 79 e6 80 c2") +
                u16("input_command_store") + bytes.fromhex("06 1f 3e fb 32 e0 ff 3a e1 ff e6 20 c2") +
                u16("input_command_space") + bytes.fromhex("06 7a 3e df 32 e0 ff 3a e1 ff e6 04 c2") +
                u16("input_command_next") + bytes.fromhex("06 20 3e ff 32 e0 ff 3e 07 32 e2 ff 3a e1 ff e6 40 c2") +
                u16("input_command_editor_current") + bytes.fromhex("06 3b"))
    editor_probes = []
    for key, next_label in (("CURRENT", "input_command_editor_new"), ("NEW", "input_command_done")):
        code = symbols["app_key_editor_" + key.lower()]
        a_port = symbols["EDITOR_" + key + "_PORT_A"]
        c_port = symbols["EDITOR_" + key + "_PORT_C"]
        expected += (bytes((0x3E, a_port)) + bytes.fromhex("32 e0 ff") + bytes((0x3E, c_port)) +
                     bytes.fromhex("32 e2 ff 3a e1 ff e6 80 c2") + u16(next_label) + bytes((6, code)))
        editor_probes.append((a_port, c_port, 0x80, code))
    expected += bytes.fromhex("af 32 e0 ff 32 e2 ff 78 c1 c9")
    require(len(expected) == 145 and data[target:target+145] == expected,
            "Command scan changed matrix masks, restoration, register preservation or quick exit")
    # Независимая привязка семи команд к таблице BIOS, затем все сочетания
    # команд и стрелок. Редактор приоритетнее, стрелки игнорируются.
    probes = ((0xFE, 0x0F, 0x40, 0x2D), (0xFE, 0x0F, 0x80, 0x1F),
              (0xFB, 0x0F, 0x20, 0x7A), (0xDF, 0x0F, 0x04, 0x20), (0xFF, 0x07, 0x40, 0x3B)) + tuple(editor_probes)
    positions = {}
    for row, values in enumerate(matrix):
        for index, code in enumerate(values):
            positions[code] = (11-index, row+2)
    commands = [probe[3] for probe in probes]
    for a_port, c_port, mask, code in probes:
        column, row = positions[code]
        selected = (0xFF ^ a_port) | ((0x0F ^ c_port) << 8)
        require(selected == 1 << column and mask == 1 << row, "Command matrix differs from BIOS")
    arrows = (8, 0x18, 0x19, 0x1A)
    for bits in range(1 << (len(commands) + len(arrows))):
        pressed = {code for bit, code in enumerate(commands + list(arrows)) if bits & (1 << bit)}
        actual = 0xFF
        for a_port, c_port, mask, code in probes:
            selected = (0xFF ^ a_port) | ((0x0F ^ c_port) << 8)
            raw = 0xFF
            for key in pressed:
                column, row = positions[key]
                if selected & (1 << column):
                    raw &= ~(1 << row)
            if raw & mask == 0:
                actual = code
        eligible = pressed & set(commands)
        if symbols["app_key_editor_new"] in eligible:
            wanted = symbols["app_key_editor_new"]
        elif symbols["app_key_editor_current"] in eligible:
            wanted = symbols["app_key_editor_current"]
        else:
            wanted = max(eligible, key=lambda key: positions[key]) if eligible else 0xFF
        require(actual == wanted, "Commands lose priority or are masked by held arrows")


def check_legacy_music(data, symbols, reference):
    u16 = lambda name: symbols[name].to_bytes(2, "little")
    # Точный исходный поток команд с перенесёнными адресами/ABI. Проверка
    # охватывает BIOS-опрос, BIOS-beep, паузы и сохранение BC/DE/HL.
    expected = (bytes.fromhex("7e 23 fe 00 c2") + u16("music_test_end") + b"\xcd" + u16("sound_rest") +
                b"\xc3" + u16("music_sequence") + bytes.fromhex("fe ff c2") + u16("music_play_note") +
                bytes.fromhex("c9 32") + u16("app_beep_frequency") + bytes.fromhex("7e 23 32") +
                u16("app_beep_duration") + b"\xcd" + u16("music_beep") + b"\xcd" +
                u16("app_bios_keycheck") + bytes.fromhex("fe 1f c8 c3") + u16("music_sequence") +
                bytes.fromhex("e5 d5 c5 cd") + u16("app_bios_beep") + bytes.fromhex("c1 d1 e1 c9"))
    at = symbols["music_sequence"]
    require(len(expected) == 49 and data[at:at+49] == expected, "Historical music engine differs")
    at = symbols["audio_music"]
    wrapper = b"\xcd" + u16("audio_stop") + b"\xcd" + u16("music_sequence")
    if symbols["app_frame_pacing_enabled"]:
        wrapper += b"\xcd" + u16("level_timer_sync")
    wrapper += b"\xc9"
    require(data[at:at+len(wrapper)] == wrapper, "Music must stop PIT effects and resync level TIME")
    for operand in (0x0010, 0x0216, 0x023B):
        require(word(data, operand) == symbols["audio_music"], "A music caller bypasses legacy player")
    for name, start, end in (("t_music_intro", 0x0B13, 0x0B51), ("t_music_level", 0x0B51, 0x0B74)):
        at = symbols[name]
        require(data[at:at+end-start] == reference[start:end], "Music bytes must match the original without PIT conversion")
    require(data[0x09C8:0x09D4] == reference[0x09C8:0x09D4], "Historical music pause changed")


def check_audio_exit(data, symbols):
    u16 = lambda name: symbols[name].to_bytes(2, "little")
    at = symbols["audio_game_exit"]
    require(data[at:at+6] == b"\xcd" + u16("audio_stop") + b"\xc3" + u16("app_bios_return"),
            "Game exit must stop its effect and return through the selected BIOS ABI")
    require(not any(name in symbols for name in ("audio_shutdown", "audio_save_bios_sound", "v_audio_bios_saved")),
            "Application must not duplicate BIOS warm-reboot sound initialization")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pit-clock-hz", type=int, choices=(2000000, 2500000))
    args = parser.parse_args()
    for stride in range(1, 256, 2):
        require(gcd(stride, 512) == 1 and len({(stride * i) % 512 for i in range(512)}) == 512,
                "Animation address order is not a permutation")
    for total in range(65536, 65536 + 10000):
        require(divmod(total, 10000) == (6 + (total - 60000) // 10000, (total - 60000) % 10000),
                "Real-time carry decomposition differs")
    reference = (ROOT / "assets/games/Almaz-2.exe").read_bytes()
    bios_source = (ROOT / "source/dos/DOS.asm").read_text(encoding="utf-8")
    row = bios_source.split("v_keybTbl:", 1)[1].splitlines()[1]
    bios_codes = [int(value, 16) for value in re.findall(r"([0-9A-Fa-f]+)h", row)]
    matrix = [[int(value, 16) for value in re.findall(r"([0-9A-Fa-f]+)h", line)]
              for line in bios_source.split("v_keybTbl:", 1)[1].splitlines()[1:7]]
    require(all(len(row) == 12 for row in matrix), "Unexpected BIOS key matrix dimensions")
    for key, expected in ((8, (0xEF, 0x0F)), (0x18, (0xFB, 0x0F)),
                          (0x19, (0xFF, 0x0D)), (0x1A, (0xFF, 0x0E))):
        column = 11 - bios_codes.index(key)
        actual = ((0xFF ^ (1 << column), 0x0F) if column < 8 else
                  (0xFF, 0x0F ^ (1 << (column - 8))))
        require(actual == expected, f"Arrow matrix differs from BIOS: {key:02X}")
    data, _ = load("Almaz2-reference")
    require(data == reference, "Historical monochrome round-trip differs")
    report = {"reference_sha256": sha256(data).hexdigest(), "pit_clock_hz": None,
              "variants": [], "runtime_emulation": False}

    for stem, color, pit, ramfos in (
        ("Almaz2-base", False, False, False),
        ("Almaz2-mx-soft", True, False, False),
        ("Almaz2X", True, True, False),
        ("Almaz2-ramfos-soft", True, False, True),
        ("Almaz2-ramfos-pit", True, True, True),
    ):
        data, symbols = load(stem)
        calls = calls_from_listing(stem)
        require(calls and all(data[address] == 0xCD or data[address] & 0xC7 == 0xC4 for address in calls),
                f"{stem}: invalid listing CALL inventory")
        check_time_print_stack(data, symbols, calls)
        buffered = bool(symbols["app_input_enabled"])
        paced = bool(symbols["app_frame_pacing_enabled"])
        realtime = bool(symbols["app_realtime_enabled"])
        quota = bool(symbols["app_animation_enabled"])
        phases = bool(symbols["app_phases_enabled"])
        fast = bool(symbols["app_fast_enabled"])
        optimized = bool(symbols["app_optimizations_enabled"])
        require(buffered == quota == phases == fast == optimized and paced == realtime == (optimized and pit),
                f"{stem}: optimization group is inconsistent")
        check_fast_logic(data, symbols, reference, pit, buffered, quota, calls)
        check_command_input(data, symbols, matrix, pit, calls)
        require(len(data) == symbols["game_end"] < 0x7F00, f"{stem}: size/stack overlap")
        require(data[0x0C00:0x3400] == reference[0x0C00:0x3400], f"{stem}: graphics/maps changed")
        require(word(data, 0x0134) == (0xC64E if ramfos else 0x8FF1), f"{stem}: frequency ABI")
        require(word(data, 0x0137) == (0xC64F if ramfos else 0x8FF2), f"{stem}: duration ABI")
        require(word(data, 0x0140) == (0xC63E if ramfos else 0x8FFC), f"{stem}: cursor ABI")
        require(word(data, 0x0166) == (symbols["input_game_command"] if optimized else
                                     (0xC81B if ramfos else 0xC812)), f"{stem}: keyboard ABI")
        require(data[0x013C:0x013F] == bytes.fromhex("21 0b 50"), f"{stem}: TIME baseline")
        require(word(data, 0x019C) == symbols["hud_screen_position"], f"{stem}: SCREEN baseline")
        check_hud_panels(data, symbols, color)
        require(word(data, 0x06EA) == (symbols["input_take_direction"] if buffered else
                                     (0xC81B if ramfos else 0xC812)), f"{stem}: player input hook")
        if buffered:
            address = symbols["input_poll"]
            body = data[address:symbols["input_poll_done"]]
            require(body.count(bytes.fromhex("3a e1 ff")) == 4, f"{stem}: four matrix reads")
            require(all(bytes((0x3E, mask)) in body for mask in (0xFB, 0xEF, 0x0E, 0x0D)),
                    f"{stem}: arrow column selection")
            require(symbols["v_input_pending"] < 0x7F00, f"{stem}: input buffer range")
            require(data[symbols["input_service"] + 3] == symbols["app_input_stride"] - 1,
                    f"{stem}: input polling interval")
        else:
            require("input_poll" not in symbols, f"{stem}: input module was not excluded")
        for address in (0x0046, 0x0065, 0x0086, 0x00A7):
            if quota or phases or paced:
                target = "animation_begin_bank" if quota or phases else "frame_begin_bank"
                require(data[address] == 0xCD and word(data, address + 1) == symbols[target],
                        f"{stem}: phase pacing at {address:04X}")
            else:
                require(data[address] == 0x22, f"{stem}: original phase bank store")
        if paced:
            require(pit and 1 <= symbols["app_step_hz"] <= 50, f"{stem}: pacing configuration")
            require(word(data, 0x0945) == symbols["frame_map_drawn"], f"{stem}: reset on map redraw")
            address = symbols["audio_next_duration"]
            require(word(data, address + 1) == symbols["frame_clock_sample_saved"],
                    f"{stem}: clock sample before audio reload")
            require(word(data, symbols["audio_stop"] + 1) == symbols["frame_clock_idle"],
                    f"{stem}: idle clock service")
            require(symbols["v_step_overruns"] + 2 <= symbols["game_end"] and
                    any(word(data, address + 1) == symbols["frame_record_work"] for address in calls),
                    f"{stem}: frame workload statistics missing")
        else:
            require("frame_begin_bank" not in symbols, f"{stem}: pacing module was not excluded")
        require(data[0x06A8:0x06D8] == reference[0x06A8:0x06D8], f"{stem}: gameplay RNG changed")
        for address in (0x054A, 0x067D):
            require(word(data, address + 1) == symbols["animation_static_draw" if quota else "tile_draw"],
                    f"{stem}: stationary animation hook")
        for address in (0x04C1, 0x0511, 0x0538, 0x0554, 0x06A3):
            require(word(data, address + 1) == symbols["tile_draw"], f"{stem}: immediate change drawing")
        if quota:
            check_sparse_animation(data, symbols)
            budget, minimum = symbols["app_animation_quota"], symbols["app_animation_min_quota"]
            chooser = symbols["animation_choose_quota"]
            require(data[chooser:chooser+2] == bytes((0x0E, budget)), f"{stem}: initial animation budget")
            if budget != minimum:
                table = symbols["t_animation_dynamic"]
                require(table % 256 == 0, f"{stem}: dynamic classifier alignment")
                expected = bytes(255 if (raw & 15) in (6, 7) or
                                 (raw & 0xF0 and (raw & 15) in (0, 4, 5)) else 0
                                 for raw in range(256))
                require(data[table:table+256] == expected, f"{stem}: dynamic classification")
                body = data[chooser:symbols["animation_count_done"]]
                require(bytes.fromhex("21 21 80") in body and b"\x06\x0f" in body and
                        b"\xfe\x18" in body and b"\xc6\x09" in body and
                        bytes((0xFE, minimum, 0xC8)) in body,
                        f"{stem}: census area or saturation bound differs")
                # Все размеры нагрузки и все позиции: насыщение обязано дать
                # ту же квоту, что полный подсчёт, без переполнения байта.
                cells = [y*32+x for y in range(1, 16) for x in range(1, 24)]
                require(len(set(cells)) == 345 and cells[0] == 33 and cells[-1] == 503,
                        "Logic area differs from dynamic census")
                for count in range(len(cells)+1):
                    for dynamic_raw in (0x06, 0xF7, 0x10, 0x74, 0x85):
                        test_map = bytearray([0xF6]*512)  # Вне логики не считать.
                        for index, cell in enumerate(cells):
                            test_map[cell] = dynamic_raw if index < count else 0x05
                        remaining = budget
                        for cell in cells:
                            remaining = (remaining + data[table + test_map[cell]]) & 255
                            if remaining == minimum:
                                break
                        require(remaining == max(budget-count, minimum),
                                f"{stem}: saturated quota at {count} dynamic cells")
        if phases:
            require(data[0x0892] == 0xCD and word(data, 0x0893) == symbols["animation_sprite_bank"],
                    f"{stem}: visual phase hook")
        else:
            require(data[0x0892] == 0x2A, f"{stem}: original sprite bank access")
        if quota or phases:
            address = symbols["t_animation_banks"]
            require(address % 8 == 0 and [word(data, address + i) for i in range(0, 8, 2)] ==
                    [0x0C00, 0x0D80, 0x0F00, 0x1080], f"{stem}: animation bank table")
            body = data[symbols["animation_initialize"]:symbols["animation_random"]]
            require(b"\xf6\x01" in body, f"{stem}: odd animation permutation stride")
        else:
            require("animation_begin_bank" not in symbols, f"{stem}: animation module not excluded")
        if realtime:
            require(paced and data[0x0040:0x0043] == bytes(3), f"{stem}: legacy TIME disabled")
            require(symbols["v_level_timer_last"] < 0x7F00, f"{stem}: real-time timer state")
            require(data[0x026C] == 0xCD and word(data, 0x026D) == symbols["level_timer_convert"],
                    f"{stem}: conversion once per level load")
            level_limits = []
            for level in range(16):
                fraction, units, tens, hundreds = reference[0x1200+level*4:0x1204+level*4]
                budget = ((hundreds*100+tens*10+units)*5+fraction)*4
                hz = symbols["app_step_hz"]
                seconds = (budget+hz-1)//hz
                remainder = (hz-budget%hz)*10000//hz if budget%hz else 0
                address = symbols["t_real_time_limits"]+level*4
                require((word(data, address), word(data, address+2)) == (seconds, remainder),
                        f"{stem}: level {level+1} time conversion")
                require(0 <= (seconds*10000-remainder)*hz-budget*10000 < hz,
                        f"{stem}: level time budget was rounded down")
                level_limits.append({"level": level+1, "display_seconds": seconds,
                                     "initial_elapsed_ticks": remainder, "logic_steps": budget})
        else:
            require(data[0x0040] == 0xCD and word(data, 0x0041) == symbols["timer_decrement"],
                    f"{stem}: legacy TIME hook")
            require(data[0x026C] == 0x32 and "t_real_time_limits" not in symbols,
                    f"{stem}: original level load time")
        if color:
            require(data[0x0886] == 0xC3 and word(data, 0x0887) == symbols["tile_draw_color"], f"{stem}: color hook")
            address = symbols["t_tile_colors"]
            require(data[address:address + 16] == PALETTE and address % 16 == 0, f"{stem}: palette")
        else:
            require(data[0x0886:0x0889] == bytes.fromhex("f5 e6 0f"), f"{stem}: monochrome renderer")
            require("t_tile_colors" not in symbols, f"{stem}: color code was not excluded")

        tones = 0
        if pit:
            pit_clock_hz = symbols["app_pit_clock_hz"]
            require(args.pit_clock_hz is None or pit_clock_hz == args.pit_clock_hz, "PIT input differs from requested check")
            require(report["pit_clock_hz"] is None or report["pit_clock_hz"] == pit_clock_hz, "PIT variants use different clocks")
            report["pit_clock_hz"] = pit_clock_hz
            check_legacy_music(data, symbols, reference)
            require(word(data, 0x098C) == symbols["audio_start"], f"{stem}: nonblocking effect hook")
            require(word(data, 0x0272) == symbols["fast_cell_update" if fast else "audio_read_cell"], f"{stem}: game service hook")
            require(word(data, 0x094A) == symbols["audio_read_cell"], f"{stem}: redraw service hook")
            require(word(data, 0x000D + 3) == symbols["audio_music"], f"{stem}: intro music hook")
            require(word(data, 0x003E) == symbols["audio_game_exit"], f"{stem}: exit cleanup")
            check_audio_exit(data, symbols)
            require(0x09D4 <= symbols["audio_update"] < symbols["v_audio_active"] < 0x0B74,
                    f"{stem}: resident audio overlaps state")
            for digit in range(10):
                cursor = symbols["t_countdown_sounds"] + 9 * digit
                delay = 0x30 + digit
                period = 119 + 30 * delay
                expected = ((period * (pit_clock_hz // 1000) + 1000) // 2000,
                            (93 + delay * period + 199) // 200)
                require(data[cursor] == 2, f"{stem}: countdown priority")
                require((word(data, cursor + 1), word(data, cursor + 3)) == expected,
                        f"{stem}: countdown tone {digit}")
                require(data[cursor + 5:cursor + 9] == bytes(4), f"{stem}: countdown end")
            for name, address, priority in TABLES:
                if name.startswith("t_music_"):
                    continue
                cursor = symbols[name]
                require(data[cursor] == priority, f"{stem}: {name} priority")
                cursor += 1
                for delay, repeat in events(reference, address):
                    actual = (word(data, cursor), word(data, cursor + 2))
                    if delay:
                        period = 119 + 30 * delay
                        expected = ((period * (pit_clock_hz // 1000) + 1000) // 2000,
                                    (93 + repeat * period + 199) // 200)
                        tones += 1
                    else:
                        expected = (0, (98401 + 199) // 200)
                    require(actual == expected, f"{stem}: {name} conversion at {cursor:04X}")
                    require(0 < actual[1] < 0x8000, f"{stem}: duration/sign detection overflow")
                    cursor += 4
                require(data[cursor:cursor + 4] == bytes(4), f"{stem}: {name} termination")
        else:
            require("audio_update" not in symbols, f"{stem}: PIT module was not excluded")
            require(word(data, 0x09B6) == (0xC851 if ramfos else 0xC170), f"{stem}: software sound ABI")
            for name, address, _ in TABLES:
                require(symbols[name] == address, f"{stem}: legacy sound table moved")
                require(events(data, address) == events(reference, address), f"{stem}: legacy sound differs")
            if buffered:
                require(word(data, 0x0272) == symbols["fast_cell_update" if fast else "input_read_cell"] and
                        word(data, 0x094A) == symbols["input_read_cell"], f"{stem}: software input service")
        report["variants"].append({"file": stem + ".exe", "bytes": len(data), "color": color,
                                   "optimizations": optimized,
                                   "pit": pit, "ramfos": ramfos, "input_buffer": buffered,
                                   "frame_pacing": paced, "step_hz": symbols["app_step_hz"] if paced else None,
                                   "real_seconds": realtime, "animation_quota": symbols["app_animation_quota"] if quota else None,
                                   "animation_min_quota": symbols["app_animation_min_quota"] if quota else None,
                                   "sparse_animation_checked": quota,
                                   "random_visual_phases": phases,
                                   "time_print_stack_checked": True,
                                   "fast_logic": fast,
                                   "level_limits": level_limits if realtime else None,
                                   "tone_records_checked": tones,
                                   "sha256": sha256(data).hexdigest()})

    path = ROOT / ".codex-tmp/almaz2-source-checks.json"
    path.parent.mkdir(exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"OK: byte-exact recovery, five build modes, ABI, original sequences, PIT conversion; {path}")


if __name__ == "__main__":
    main()
