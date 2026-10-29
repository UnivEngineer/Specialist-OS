/* Расширение Specialist-OS, 2026. SPDX-License-Identifier: GPL-3.0-or-later */
#include "SpecialistMx2VideoRam.h"

void SpecialistMx2VideoRam::initConnections() {
    SpecVideoRam::initConnections();
    REG_INPUT("mx2ColorLatch", SpecialistMx2VideoRam::setMxLatch);
    REG_INPUT("mx2MemMode", SpecialistMx2VideoRam::setMemMode);
}

void SpecialistMx2VideoRam::writeByte(int addr, uint8_t value) {
    // Базовый setCurColor вызывается ВВ55 при сканировании клавиатуры.
    // В MX он не должен менять аппаратную защёлку цвета FFF8h-FFFBh.
    if (m_mxMode) setCurColorMx(m_mxLatch);
    SpecVideoRam::writeByte(addr, value);
}

void SpecialistMx2VideoRam::reset() {
    SpecVideoRam::reset();
    m_mxLatch = 0x70;
    m_mxMode = true;
}
