#include "modMenu.h"
#include <cstdint>
#include "baseMod/baseMod.hpp"
#include "settings/settings.h"


BASEMOD_CALL void Defaults() {
    SettingsManager::GetInstance().SetToDefaults();
}

void RegisterModMenu(BaseMod::Api* bmApi) {
    constexpr int numEntries = 10;
    auto& settings = SettingsManager::GetInstance();

    static const char* onOffLabels[2] = {"OFF", "ON"};
    static BaseMod::ModMenuEntry entries[numEntries] = {
        {"DISPLAY", &settings.Display, 0, 1, onOffLabels},
        {"OPACITY", &settings.Opacity, 0, 255},
        {"Y POS", &settings.yPos, 0, 480},
        {"Z POS", &settings.zPos, 0, 800},
        {"RECORD HITSTOP", &settings.RecordHitstop, 0, 1, onOffLabels},
        {"RECORD SUPERFLASH", &settings.RecordSuperFlash, 0, 1, onOffLabels},
        {"IGNORE DISTANT PROJECTILES", &settings.IgnoreDistantProjectiles, 0, 1, onOffLabels},
        {"DISPLAY GAUGE VALUES", &settings.DisplayGaugeNumbers, 0, 1, onOffLabels},
        {"DISPLAY FRAME RUN SUMS", &settings.DisplayRunSums, 0, 1, onOffLabels},
        {"DEFAULT", nullptr, 0, 0, nullptr, Defaults},
    };
    
    bmApi->ModMenu.RegisterMenuTab("FRAME METER", entries, numEntries);
}
