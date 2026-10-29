/* Расширение Specialist-OS, 2026. SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include "Specialist.h"

// Независимые источники цвета ММХ: MX-регистр и STD-выходы ВВ55.
class SpecialistMx2VideoRam : public SpecVideoRam {
public:
    explicit SpecialistMx2VideoRam(int size) : SpecVideoRam(size) {}
    void initConnections() override;
    void writeByte(int addr, uint8_t value) override;
    void reset() override;
    static EmuObject* create(const EmuValuesList& args) {
        return args[0].isInt() ? new SpecialistMx2VideoRam(args[0].asInt()) : nullptr;
    }
private:
    // В STD тот же адрес управляет выбором ROM/RAM и 4/8 цветами.
    void setMxLatch(uint8_t value) { if (m_mxMode) m_mxLatch = value; }
    void setMemMode(int mode) { m_mxMode = mode != 3; }
    uint8_t m_mxLatch = 0x70;
    bool m_mxMode = true;
};
