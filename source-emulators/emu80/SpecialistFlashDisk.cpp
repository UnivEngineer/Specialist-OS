/* Emu80 v4; © Viktor Pykhonin, 2016-2026; расширение Specialist-OS, 2026.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "SpecialistFlashDisk.h"
#include "Emulation.h"
#include "Globals.h"
#include "PalFile.h"
#include <stdexcept>

SpecialistFlashDisk::SpecialistFlashDisk() { pause(); }

uint64_t SpecialistFlashDisk::now() const {
    const uint64_t hz = g_emulation->getFrequency();
    const uint64_t ticks = g_emulation->getCurClock();
    return (ticks / hz) * 1000000 + (ticks % hz) * 1000000 / hz;
}

void SpecialistFlashDisk::init() {
    PalFile file;
    if (!file.open(m_imageName, m_readOnly ? "r" : "r+"))
        throw std::runtime_error("Cannot open flash image: " + m_imageName);
    if (file.getSize() != static_cast<int64_t>(m_chipCount * At29Flash::Size))
        throw std::runtime_error("Flash image size must match chipCount * 524288");
    for (unsigned chip = 0; chip < m_chipCount; ++chip)
        for (auto& value : m_chips[chip].bytes()) value = file.read8();
    PalFile protection;
    if (protection.open(m_imageName + ".sdp", "r")) {
        if (protection.getSize() != m_chipCount)
            throw std::runtime_error("Invalid AT29 SDP state file");
        for (unsigned chip = 0; chip < m_chipCount; ++chip) {
            const uint8_t state = protection.read8();
            if (state > 1) throw std::runtime_error("Invalid AT29 SDP state");
            m_chips[chip].restoreProtection(state != 0);
        }
    }
    m_ready = true;
    m_curClock = g_emulation->getCurClock();
    resume();
}

void SpecialistFlashDisk::reset() {
    // Сброс CPU не является отключением питания и не прерывает программирование.
    m_aInput = m_bInput = m_cLowInput = m_cHighInput = true;
    updatePins();
}

void SpecialistFlashDisk::shutdown() { updateStorage(); pause(); }

void SpecialistFlashDisk::operate() {
    updateStorage();
    m_curClock += g_emulation->getFrequency() / 1000;
}

void SpecialistFlashDisk::updateStorage() {
    if (!m_ready) return;
    PalFile file;
    bool changed = false;
    for (unsigned chip = 0; chip < m_chipCount; ++chip) {
        m_chips[chip].advance(now());
        unsigned page;
        if (!m_chips[chip].takeDirtyPage(page) || m_readOnly) continue;
        if (!file.isOpen() && !file.open(m_imageName, "r+")) { m_ioError = true; continue; }
        file.seek(chip * At29Flash::Size + page);
        for (unsigned i = 0; i < 256; ++i) file.write8(m_chips[chip].bytes()[page + i]);
        changed = true;
    }
    if (changed) {
        PalFile protection;
        if (!protection.open(m_imageName + ".sdp", "w")) { m_ioError = true; return; }
        for (unsigned chip = 0; chip < m_chipCount; ++chip)
            protection.write8(m_chips[chip].protectedData() ? 1 : 0);
    }
}

void SpecialistFlashDisk::updatePins() {
    const uint8_t c = m_c | (m_cLowInput ? 0x0F : 0) | (m_cHighInput ? 0xF0 : 0);
    const uint8_t b = m_bInput ? 0xFF : m_b;
    // ИР22 прозрачен при STB=1; выход сохраняется после спада STB.
    if (c & 0x20) m_highAddress = b;
    const unsigned chip = (c >> 3) & 3;
    // ИД14: /WE активен только при /RD=1 и /WR=0, /OE — при обратной паре.
    const bool write = (c & 0xC0) == 0x80 && !m_aInput && !m_bInput && !m_readOnly;
    if (m_writeActive && (!write || chip != m_writeChip)) {
        if (m_writeChip < m_chipCount) m_chips[m_writeChip].endWrite(m_aInput ? 0xFF : m_a, now());
        m_writeActive = false;
    }
    if (write && !m_writeActive) {
        m_writeChip = chip;
        if (chip < m_chipCount)
            m_chips[chip].beginWrite(b | (unsigned(m_highAddress) << 8) | (unsigned(c & 7) << 16), now());
        m_writeActive = true;
    }
    m_effectiveC = c;
    updateStorage();
}

uint8_t SpecialistFlashDisk::getPortA() {
    updateStorage();
    if (!m_aInput || (m_effectiveC & 0xC0) != 0x40) return 0xFF;
    const unsigned chip = (m_effectiveC >> 3) & 3;
    if (chip >= m_chipCount) return 0xFF;
    const unsigned address = (m_bInput ? 0xFF : m_b) | (unsigned(m_highAddress) << 8) | (unsigned(m_effectiveC & 7) << 16);
    return m_chips[chip].read(address, now());
}

void SpecialistFlashDisk::setPortA(uint8_t v) { m_a = v; updatePins(); }
void SpecialistFlashDisk::setPortB(uint8_t v) { m_b = v; updatePins(); }
void SpecialistFlashDisk::setPortC(uint8_t v) { m_c = v; updatePins(); }
void SpecialistFlashDisk::setPortAMode(bool v) { m_aInput = v; updatePins(); }
void SpecialistFlashDisk::setPortBMode(bool v) { m_bInput = v; updatePins(); }
void SpecialistFlashDisk::setPortCLoMode(bool v) { m_cLowInput = v; updatePins(); }
void SpecialistFlashDisk::setPortCHiMode(bool v) { m_cHighInput = v; updatePins(); }

bool SpecialistFlashDisk::setProperty(const std::string& name, const EmuValuesList& values) {
    if (Ppi8255Circuit::setProperty(name, values)) return true;
    if (name == "image") { m_imageName = values[0].asString(); return true; }
    if (name == "chipCount" && values[0].isInt() && values[0].asInt() >= 1 && values[0].asInt() <= 4) {
        m_chipCount = values[0].asInt(); return true;
    }
    if (name == "readOnly" && (values[0].asString() == "yes" || values[0].asString() == "no")) {
        m_readOnly = values[0].asString() == "yes"; return true;
    }
    return false;
}

std::string SpecialistFlashDisk::getPropertyStringValue(const std::string& name) {
    if (name == "image") return m_imageName;
    if (name == "ioError") return m_ioError ? "yes" : "no";
    return Ppi8255Circuit::getPropertyStringValue(name);
}
