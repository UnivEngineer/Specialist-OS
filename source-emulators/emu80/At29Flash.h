/* Emu80 v4; © Viktor Pykhonin, 2016-2026; расширение Specialist-OS, 2026.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include <array>
#include <cstdint>
#include <vector>

// Одна AT29C040A. Время передаётся в микросекундах эмулируемой машины.
// begin/end соответствуют спаду и подъёму /WE или /CE при /OE=1.
class At29Flash {
public:
    static const unsigned Size = 512 * 1024;
    At29Flash();
    void beginWrite(unsigned address, uint64_t now);
    void endWrite(uint8_t value, uint64_t now);
    uint8_t read(unsigned address, uint64_t now);
    void advance(uint64_t now);
    std::vector<uint8_t>& bytes() { return m_bytes; }
    bool busy() const { return m_loading || m_programming; }
    bool protectedData() const { return m_protected; }
    void restoreProtection(bool value) { m_protected = value; }
    bool takeDirtyPage(unsigned& page);
    unsigned errors() const { return m_errors; }
private:
    void dataByte(unsigned address, uint8_t value, uint64_t now);
    void commandByte(unsigned address, uint8_t value, uint64_t now);
    std::vector<uint8_t> m_bytes;
    std::array<uint8_t, 256> m_page;
    unsigned m_pageBase = 0, m_address = 0, m_lastAddress = 0;
    unsigned m_sequence = 0, m_errors = 0;
    uint64_t m_deadline = 0, m_commandDeadline = 0, m_finish = 0;
    uint8_t m_lastData = 0xFF;
    bool m_loading = false, m_programming = false, m_pulse = false;
    bool m_acceptPulse = false, m_protected = false, m_authorized = false;
    bool m_enableProtection = false, m_disableProtection = false;
    bool m_discard = false, m_identification = false, m_toggle = false, m_dirty = false;
};
