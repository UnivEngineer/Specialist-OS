/* Emu80 v4; © Viktor Pykhonin, 2016-2026; расширение Specialist-OS, 2026.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "Ppi8255Circuit.h"
#include "At29Flash.h"
#include <array>

// Обвязка КР580ВВ55 + ИР22 + ИД14, четыре AT29C040A.
class SpecialistFlashDisk : public Ppi8255Circuit, public IActive {
public:
    SpecialistFlashDisk();
    void init() override;
    void reset() override;
    void shutdown() override;
    void operate() override;
    uint8_t getPortA() override;
    void setPortA(uint8_t value) override;
    void setPortB(uint8_t value) override;
    void setPortC(uint8_t value) override;
    void setPortAMode(bool input) override;
    void setPortBMode(bool input) override;
    void setPortCLoMode(bool input) override;
    void setPortCHiMode(bool input) override;
    bool setProperty(const std::string&, const EmuValuesList&) override;
    std::string getPropertyStringValue(const std::string&) override;
    static EmuObject* create(const EmuValuesList&) { return new SpecialistFlashDisk(); }
private:
    uint64_t now() const;
    void updatePins();
    void updateStorage();
    std::array<At29Flash, 4> m_chips;
    std::string m_imageName;
    unsigned m_chipCount = 4;
    uint8_t m_a = 0, m_b = 0, m_c = 0, m_highAddress = 0;
    bool m_aInput = true, m_bInput = true, m_cLowInput = true, m_cHighInput = true;
    bool m_writeActive = false, m_readOnly = false, m_ready = false, m_ioError = false;
    unsigned m_writeChip = 0;
    uint8_t m_effectiveC = 0xFF;
};
