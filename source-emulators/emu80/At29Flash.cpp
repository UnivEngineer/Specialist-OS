/* Emu80 v4; © Viktor Pykhonin, 2016-2026; расширение Specialist-OS, 2026.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "At29Flash.h"
#include <algorithm>

At29Flash::At29Flash() : m_bytes(Size, 0xFF) { m_page.fill(0xFF); }

void At29Flash::advance(uint64_t now) {
    if (m_loading && !m_pulse && now >= m_deadline) {
        m_loading = false;
        m_programming = true;
        m_finish = m_deadline + 10000; // tWC: моделируем предельные 10 мс.
    }
    if (m_programming && now >= m_finish) {
        if (!m_discard) {
            std::copy(m_page.begin(), m_page.end(), m_bytes.begin() + m_pageBase);
            m_dirty = true;
        }
        if (m_enableProtection) m_protected = true;
        if (m_disableProtection) m_protected = false;
        m_programming = m_authorized = m_enableProtection = m_disableProtection = false;
        m_sequence = 0;
    }
    if (!m_loading && !m_programming && m_sequence && now > m_commandDeadline)
        m_sequence = 0;
    if (!m_loading && !m_programming && m_authorized && now > m_commandDeadline) {
        m_authorized = m_enableProtection = m_disableProtection = false;
        ++m_errors;
    }
}

void At29Flash::beginWrite(unsigned address, uint64_t now) {
    // Спад /WE ровно через tBLC ещё принадлежит текущей загрузке.
    if (!(m_loading && !m_pulse && now == m_deadline)) advance(now);
    m_address = address & (Size - 1);
    m_acceptPulse = !m_programming;
    m_pulse = true;
}

void At29Flash::endWrite(uint8_t value, uint64_t now) {
    if (!m_pulse) return;
    m_pulse = false;
    if (!m_acceptPulse) return;
    if (m_loading) dataByte(m_address, value, now);
    else commandByte(m_address, value, now);
}

void At29Flash::dataByte(unsigned address, uint8_t value, uint64_t now) {
    if (!m_loading) {
        m_pageBase = address & ~255u;
        m_page.fill(0xFF);
        m_loading = true;
        m_discard = m_protected && !m_authorized;
    }
    // Смена сектора в одной загрузке недопустима; такой цикл не меняет массив.
    if ((address & ~255u) != m_pageBase) { m_discard = true; ++m_errors; }
    m_page[address & 255] = value;
    m_lastAddress = address;
    m_lastData = value;
    m_deadline = now + 150;
}

void At29Flash::commandByte(unsigned address, uint8_t value, uint64_t now) {
    const unsigned commandAddress = address & 0x7FFF; // В командах значимы A14..A0.
    if (m_authorized) { dataByte(address, value, now); return; }
    if (m_sequence == 0 && commandAddress == 0x5555 && value == 0xAA) m_sequence = 1;
    else if (m_sequence == 1 && commandAddress == 0x2AAA && value == 0x55) m_sequence = 2;
    else if (m_sequence == 2 && commandAddress == 0x5555) {
        m_sequence = 0;
        if (value == 0xA0) { m_authorized = true; m_enableProtection = true; }
        else if (value == 0x80) m_sequence = 3;
        else if (value == 0x90) m_identification = true;
        else if (value == 0xF0) m_identification = false;
        else ++m_errors;
    } else if (m_sequence == 3 && commandAddress == 0x5555 && value == 0xAA) m_sequence = 4;
    else if (m_sequence == 4 && commandAddress == 0x2AAA && value == 0x55) m_sequence = 5;
    else if (m_sequence == 5 && commandAddress == 0x5555 && value == 0x20) {
        m_sequence = 0;
        m_authorized = true;
        m_disableProtection = true;
    } else if (m_sequence == 5 && commandAddress == 0x5555) {
        // Chip erase и boot-block lock пока не поддерживаются, не превращаем их в данные.
        m_sequence = 0;
        ++m_errors;
    } else {
        m_sequence = 0;
        if (!m_identification) dataByte(address, value, now);
    }
    m_commandDeadline = now + 150;
}

uint8_t At29Flash::read(unsigned address, uint64_t now) {
    advance(now);
    address &= Size - 1;
    if (m_loading || m_programming) {
        m_toggle = !m_toggle;
        // Достоверный DATA polling — по последнему загруженному адресу.
        uint8_t result = m_bytes[address] & 0x3F;
        result |= m_toggle ? 0x40 : 0;
        result |= (m_lastData ^ 0x80) & 0x80;
        return result;
    }
    if (m_identification) return (address & 1) ? 0xA4 : 0x1F;
    return m_bytes[address];
}

bool At29Flash::takeDirtyPage(unsigned& page) {
    if (!m_dirty) return false;
    page = m_pageBase;
    m_dirty = false;
    return true;
}
