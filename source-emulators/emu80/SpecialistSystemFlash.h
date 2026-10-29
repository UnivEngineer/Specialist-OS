/* Расширение Specialist-OS, 2026. SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include "EmuObjects.h"
#include "At29Flash.h"

// AT29C040A на маппере: адрес в чипе задаётся отображением страниц.
class SpecialistSystemFlash : public AddressableDevice, public IActive {
public:
    SpecialistSystemFlash();
    void init() override;
    void reset() override;
    void shutdown() override;
    void operate() override;
    uint8_t readByte(int addr) override;
    void writeByte(int addr, uint8_t value) override;
    bool setProperty(const std::string&, const EmuValuesList&) override;
    std::string getPropertyStringValue(const std::string&) override;
    static EmuObject* create(const EmuValuesList&) { return new SpecialistSystemFlash(); }
private:
    uint64_t now() const;
    void updateStorage();
    At29Flash m_chip;
    std::string m_imageName;
    bool m_readOnly = false, m_ready = false, m_ioError = false;
};
