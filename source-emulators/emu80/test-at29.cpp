// Регрессии автомата AT29C040A; запускать также с отключённым NDEBUG.
#include "At29Flash.h"
#include <cassert>
#include <iostream>

static void pulse(At29Flash& chip, unsigned addr, unsigned data, uint64_t t) {
    chip.beginWrite(addr, t); chip.endWrite(static_cast<uint8_t>(data), t + 1);
}
static void unlock(At29Flash& chip, uint64_t t) {
    pulse(chip, 0x5555, 0xAA, t);
    pulse(chip, 0x2AAA, 0x55, t + 50);
    pulse(chip, 0x5555, 0xA0, t + 100);
}
int main() {
    At29Flash a, b;
    assert(a.read(0, 0) == 0xFF);
    unlock(a, 0);
    for (unsigned i = 0; i < 256; ++i) pulse(a, 0x7FF00 + i, i, 150 + i * 50);
    assert(a.busy());
    assert((a.read(0x7FFFF, 13000) & 0x80) == 0); // DQ7 инвертирован.
    const auto status = a.read(0x7FFFF, 13000);
    assert(((status ^ a.read(0x7FFFF, 13000)) & 0x40) != 0);
    assert(b.read(0, 13000) == 0xFF && !b.busy());
    a.advance(24000);
    for (unsigned i = 0; i < 256; ++i) assert(a.read(0x7FF00 + i, 24000) == i);
    assert(a.protectedData());
    // Запись без SDP не меняет защищённый сектор, но запускает таймеры.
    pulse(a, 0x7FFFF, 0, 25000);
    assert(a.busy());
    a.advance(36000);
    assert(a.read(0x7FFFF, 36000) == 255);
    // Моделируется перезапись с 0 на 1 и стирание незагруженных байтов в FF.
    unlock(a, 37000);
    pulse(a, 0x7FFFF, 0x80, 37150);
    a.advance(48000);
    assert(a.read(0x7FFFF, 48000) == 0x80 && a.read(0x7FF00, 48000) == 0xFF);
    // Короткий/оборванный код SDP не разрешает защищённую запись.
    pulse(a, 0x5555, 0xAA, 50000);
    pulse(a, 0x7FFFF, 0x12, 50200);
    a.advance(61000);
    assert(a.read(0x7FFFF, 61000) == 0x80);
    // Смена сектора внутри загрузки отвергается целиком.
    unlock(a, 62000);
    pulse(a, 0x100, 1, 62150);
    pulse(a, 0x200, 2, 62200);
    a.advance(73000);
    assert(a.read(0x100, 73000) == 0xFF && a.errors() != 0);
    // Спад /WE до tBLC удерживает загрузку даже при длинном импульсе.
    pulse(b, 0x10, 0x12, 0);
    b.beginWrite(0x11, 151);
    b.endWrite(0x34, 300);
    b.advance(11000);
    assert(b.read(0x10, 11000) == 0x12 && b.read(0x11, 11000) == 0x34);
    // Идентификатор не должен попадать в данные.
    pulse(b, 0x5555, 0xAA, 12000);
    pulse(b, 0x2AAA, 0x55, 12050);
    pulse(b, 0x5555, 0x90, 12100);
    assert(b.read(0, 12101) == 0x1F && b.read(1, 12101) == 0xA4);
    pulse(b, 0x5555, 0xAA, 12200);
    pulse(b, 0x2AAA, 0x55, 12250);
    pulse(b, 0x5555, 0xF0, 12300);
    assert(b.read(0x10, 12301) == 0x12);
    // SDP disable применяется после программирования и сохраняет данные.
    uint64_t t = 80000;
    for (const auto& item : {std::array<unsigned, 2>{{0x5555, 0xAA}}, {{0x2AAA, 0x55}},
                             {{0x5555, 0x80}}, {{0x5555, 0xAA}}, {{0x2AAA, 0x55}}, {{0x5555, 0x20}}}) {
        pulse(a, item[0], item[1], t); t += 50;
    }
    pulse(a, 0x300, 0x5A, t);
    a.advance(92000);
    assert(!a.protectedData() && a.read(0x300, 92000) == 0x5A);
    pulse(a, 0x300, 0xA5, 93000);
    a.advance(104000);
    assert(a.read(0x300, 104000) == 0xA5);
    std::cout << "AT29: program/polling/SDP/timeout/erase/boundaries/independent chips OK\n";
}
