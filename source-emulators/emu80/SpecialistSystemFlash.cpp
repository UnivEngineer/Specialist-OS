/* Расширение Specialist-OS, 2026. SPDX-License-Identifier: GPL-3.0-or-later */
#include "SpecialistSystemFlash.h"
#include "Emulation.h"
#include "Globals.h"
#include "PalFile.h"
#include <stdexcept>

SpecialistSystemFlash::SpecialistSystemFlash() { pause(); }

uint64_t SpecialistSystemFlash::now() const {
    const uint64_t hz = g_emulation->getFrequency();
    const uint64_t ticks = g_emulation->getCurClock();
    return (ticks / hz) * 1000000 + (ticks % hz) * 1000000 / hz;
}

void SpecialistSystemFlash::init() {
    PalFile file;
    if (!file.open(m_imageName, m_readOnly ? "r" : "r+"))
        throw std::runtime_error("Cannot open system flash image: " + m_imageName);
    const auto size = file.getSize();
    if (size != 128 * 1024 && size != 256 * 1024 && size != 512 * 1024)
        throw std::runtime_error("System ROM/flash image must be 128, 256 or 512 KiB");
    // Образ представляет весь чип, BPB может описывать меньший том.
    m_chip = At29Flash(static_cast<unsigned>(size));
    for (auto& value : m_chip.bytes()) value = file.read8();
    PalFile protection;
    if (protection.open(m_imageName + ".sdp", "r")) {
        if (protection.getSize() != 1) throw std::runtime_error("Invalid system flash SDP file");
        const uint8_t value = protection.read8();
        if (value > 1) throw std::runtime_error("Invalid system flash SDP state");
        m_chip.restoreProtection(value != 0);
    }
    m_ready = true;
    m_curClock = g_emulation->getCurClock();
    resume();
}

void SpecialistSystemFlash::reset() { /* Сброс CPU не отключает питание чипа. */ }
void SpecialistSystemFlash::shutdown() { updateStorage(); pause(); }
void SpecialistSystemFlash::operate() {
    updateStorage();
    m_curClock += g_emulation->getFrequency() / 1000;
}

void SpecialistSystemFlash::updateStorage() {
    if (!m_ready) return;
    m_chip.advance(now());
    unsigned page;
    if (!m_chip.takeDirtyPage(page) || m_readOnly) return;
    PalFile file;
    if (!file.open(m_imageName, "r+")) { m_ioError = true; return; }
    file.seek(page);
    for (unsigned i = 0; i < m_chip.pageSize(); ++i) file.write8(m_chip.bytes()[page + i]);
    PalFile protection;
    if (!protection.open(m_imageName + ".sdp", "w")) { m_ioError = true; return; }
    protection.write8(m_chip.protectedData() ? 1 : 0);
}

uint8_t SpecialistSystemFlash::readByte(int addr) {
    updateStorage();
    return m_chip.read(static_cast<unsigned>(addr), now());
}

void SpecialistSystemFlash::writeByte(int addr, uint8_t value) {
    if (m_readOnly) return; // Перемычка J4: /WE запрещён.
    updateStorage();
    m_chip.beginWrite(static_cast<unsigned>(addr), now());
    m_chip.endWrite(value, now());
}

bool SpecialistSystemFlash::setProperty(const std::string& name, const EmuValuesList& values) {
    if (AddressableDevice::setProperty(name, values)) return true;
    if (name == "image") { m_imageName = values[0].asString(); return true; }
    if (name == "readOnly" && (values[0].asString() == "yes" || values[0].asString() == "no")) {
        m_readOnly = values[0].asString() == "yes"; return true;
    }
    return false;
}

std::string SpecialistSystemFlash::getPropertyStringValue(const std::string& name) {
    if (name == "image") return m_imageName;
    if (name == "capacity") return std::to_string(m_chip.bytes().size());
    if (name == "pageSize") return std::to_string(m_chip.pageSize());
    if (name == "ioError") return m_ioError ? "yes" : "no";
    return AddressableDevice::getPropertyStringValue(name);
}
