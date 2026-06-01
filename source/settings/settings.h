#pragma once

#include <cstdint>
#include <map>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include "frameMeter/frameMeter.h"

constexpr int _serializationVersion = 1;

class FrameMeterPalette {
    public:
    uint32_t GetColor(FrameType type) {
        int index = static_cast<int>(type);
        if (index < 0 || index > 11) index = 0;
        return _frameTypeColorMap[index];
    }
    uint32_t GetColor(FrameProperty property) {
        int index = static_cast<int>(property);
        if (index < 0 || index > 11) index = 0;
        return _framePropertyColorMap[index];
    }
    void SetColor(FrameType type, uint32_t argb) {
        _frameTypeColorMap[static_cast<int>(type)] = argb;
    }
    void SetColor(FrameProperty property, uint32_t argb) {
        _framePropertyColorMap[static_cast<int>(property)] = argb;
    }

    private:
    uint32_t _frameTypeColorMap[12] = {
        0xFF000000, // FrameType::NONE
        0xFF1B1B1B, // FrameType::NEUTRAL
        0xFF41F8FC, // FrameType::MOVEMENT
        0xFF01B597, // FrameType::CH_STATE
        0xFF01B597, // FrameType::STARTUP
        0xFFCB2B67, // FrameType::ACTIVE
        0xFFCB2B67, // FrameType::ACTIVE_THROW
        0xFF006FBC, // FrameType::RECOVERY
        0xFFC8C800, // FrameType::BLOCKSTUN
        0xFFC8C800, // FrameType::HITSTUN
        0xFF969600, // FrameType::TECHABLE_HITSTUN
        0xFF969600, // FrameType::KNOCK_DOWN_HITSTUN
    };
    uint32_t _framePropertyColorMap[11] = {
        0x00000000, // FrameProperty::DEFAULT
        0xFFFFFFFF, // FrameProperty::INVULN_FULL
        0xFF0080FF, // FrameProperty::INVULN_STRIKE
        0xFFFF8000, // FrameProperty::INVULN_THROW
        0xFF785000, // FrameProperty::PARRY
        0xFF785000, // FrameProperty::GUARD_POINT_FULL
        0xFF785000, // FrameProperty::GUARD_POINT_HIGH
        0xFF785000, // FrameProperty::GUARD_POINT_LOW
        0xFF785000, // FrameProperty::ARMOR
        0xFFFFFF00, // FrameProperty::FRC
        0xFFFF0000, // FrameProperty::SLASH_BACK
    };
};

class SettingsManager {
    public:
    static SettingsManager& GetInstance() {
        static SettingsManager instance;
        return instance;
    }

    public:
    int32_t Display = true;
    int32_t DisplayGaugeNumbers = true;
    int32_t DisplayRunSums = true;
    int32_t RecordHitstop = false;
    int32_t RecordSuperFlash = false;
    int32_t IgnoreDistantProjectiles = false;
    int32_t PauseThreshold = 10;
    int32_t yPos = 380;
    int32_t zPos = 70;
    int32_t Opacity = 255;
    FrameMeterPalette Palette;

    void SetToDefaults() {
        *this = SettingsManager();
    }

    void VisitFields(auto lambda) {
        lambda(Display);
        lambda(DisplayGaugeNumbers);
        lambda(RecordHitstop);
        lambda(RecordSuperFlash);
        lambda(IgnoreDistantProjectiles);
        lambda(PauseThreshold);
        lambda(yPos);
        lambda(zPos);
        lambda(Opacity);
        lambda(Palette);
    }

    void Serialize(const std::string filename) {
        std::filesystem::path path(filename);
        std::ofstream file(path.string(), std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[FrameMeter] Error saving settings file: " << filename.c_str() << std::endl;
            return;
        }
        auto write = [&file](auto&& data) {
            file.write(reinterpret_cast<char*>(&data), sizeof(data));
        };

        int serializationVersion = _serializationVersion;
        write(serializationVersion);
        VisitFields(write);
    }
    // Palette must be serializable
    static_assert(std::is_trivially_copyable_v<FrameMeterPalette>);

    void Deserialize(const std::string filename) {
        std::filesystem::path path(filename);
        if (!std::filesystem::exists(path)) {
            std::cout << "[FrameMeter] No saved settings found." << std::endl;
            return;
        }
        std::ifstream file(path.string(), std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[FrameMeter] Error loading settings file: " << filename.c_str() << std::endl;
            return;
        }
        
        char buffer[32];
        auto load = [&file, &buffer](auto&& data){
            file.read(reinterpret_cast<char*>(&data), sizeof(data));
        };

        int serializedVersion;
        load(serializedVersion);
        if (serializedVersion != _serializationVersion) {
            std::cerr << "[FrameMeter] Serialized settings are not a supported version! They will be ignored" << std::endl;
            return;
        }
        VisitFields(load);
    }

    protected:
    SettingsManager() = default;
    SettingsManager(const SettingsManager&) = default;
    SettingsManager(SettingsManager&&) = default;
    SettingsManager& operator=(const SettingsManager&) = default;
    SettingsManager& operator=(SettingsManager&&) = default;
    ~SettingsManager() = default;
};
static_assert(std::is_trivially_copyable_v<SettingsManager>);
