#include "render.h"
#include <cstdint>
#include <unordered_map>
#include "gearLoader/ggxxacpr.hpp"
#include "baseMod/baseMod.hpp"
#include "frameMeter/frameMeter.h"
#include "settings/settings.h"

using Color = uint32_t;

constexpr Color meterBgColor = 0xFF000000;

static std::unordered_map<FrameType, Color> frameTypeColorMap = {
    {FrameType::NONE,               0xFF000000},
    {FrameType::NEUTRAL,            0xFF1B1B1B},
    {FrameType::MOVEMENT,           0xFF41F8FC},
    {FrameType::CH_STATE,           0xFF01B597},
    {FrameType::STARTUP,            0xFF01B597},
    {FrameType::ACTIVE,             0xFFCB2B67},
    {FrameType::ACTIVE_THROW,       0xFFCB2B67},
    {FrameType::RECOVERY,           0xFF006FBC},
    {FrameType::BLOCKSTUN,          0xFFC8C800},
    {FrameType::HITSTUN,            0xFFC8C800},
    {FrameType::TECHABLE_HITSTUN,   0xFF969600},
    {FrameType::KNOCK_DOWN_HITSTUN, 0xFF969600},
};
static std::unordered_map<FrameProperty, Color> framePropertyColorMap = {
    {FrameProperty::DEFAULT,           0x00000000},
    {FrameProperty::INVULN_FULL,       0xFFFFFFFF},
    {FrameProperty::INVULN_STRIKE,     0xFF0080FF},
    {FrameProperty::INVULN_THROW,      0xFFFF8000},
    {FrameProperty::PARRY,             0xFF785000},
    {FrameProperty::GUARD_POINT_FULL,  0xFF785000},
    {FrameProperty::GUARD_POINT_HIGH,  0xFF785000},
    {FrameProperty::GUARD_POINT_LOW,   0xFF785000},
    {FrameProperty::ARMOR,             0xFF785000},
    {FrameProperty::FRC,               0xFFFFFF00},
    {FrameProperty::SLASH_BACK,        0xFFFF0000},
};

// Drawing dimension constants
namespace Dim {
    constexpr int pipHeight = 8;
    constexpr int pipHeightEntity = 5;
    constexpr int pipWidth = 6;
    constexpr int pipSpacing = 1;
    constexpr int borderSize = 1;

    constexpr int propertyHighlightHeight = 2;

    constexpr int textMargin = 4;
    constexpr float textScale = 1.0f;
    constexpr int textHeight = static_cast<int>(15 * textScale);
    constexpr int textAdvantageXOffset = 148;
    constexpr int textTotalXOffset = 260;
};

inline uint32_t Alpha(uint32_t val) {
    return (val & 0xFFFFFF) | (SettingsManager::GetInstance().Opacity << 24);
}
void DrawFramePip(BaseMod::Api* api, Frame& frame, int x, int y, bool isEntityPip) {
    constexpr int maxProperties = 2;
    int pipHeight = isEntityPip ? Dim::pipHeightEntity : Dim::pipHeight;
    int zPos = SettingsManager::GetInstance().zPos;
    uint32_t alpha = (SettingsManager::GetInstance().Opacity & 0xFF) << 24;

    // main pip
    api->NativeFunctions.DrawQuad(
        x, y,
        x + Dim::pipWidth,
        y + pipHeight,
        zPos, Alpha(frameTypeColorMap[frame.Type])
    );

    int topHighlightCount = 0;
    for (int i = 0; i < frame.Properties.size(); i++) {
        if (frame.Properties[i] == FrameProperty::DEFAULT) continue;
        // bot highlight for FRC
        if (frame.Properties[i] == FrameProperty::FRC) {
            api->NativeFunctions.DrawQuad(
                x, y + pipHeight - Dim::propertyHighlightHeight,
                x + Dim::pipWidth,
                y + pipHeight,
                zPos, Alpha(framePropertyColorMap[frame.Properties[i]])
            );
        }
        // top highlights
        else if (topHighlightCount < maxProperties) {
            int highlightWidth = Dim::pipWidth / maxProperties;
            api->NativeFunctions.DrawQuad(
                x + (Dim::pipWidth / maxProperties) * topHighlightCount,
                y,
                x + Dim::pipWidth,
                y + Dim::propertyHighlightHeight,
                zPos, Alpha(framePropertyColorMap[frame.Properties[i]])
            );
            topHighlightCount++;
        }
    }
}

void DrawFrameRunSum(BaseMod::Api* api, int runsum, float x, float y, float z) {
    float alpha = SettingsManager::GetInstance().Opacity / 255.0f;
    if (runsum > 0) {
        int onesDigit = runsum % 10;
        int tensDigit = runsum / 10;
        DrawRunSumText(api, std::to_string(onesDigit), x, y + 1.0f, z, alpha, 1.0f, false, Font::BLACK);
        if (tensDigit > 0) {
            DrawRunSumText(api, std::to_string(tensDigit),
                x - (Dim::pipWidth + Dim::pipSpacing) + 1.0f,
                y + 1.0f, z, alpha, 1.0f, false, Font::BLACK);
        }
    }
}

void DrawMeterGroup(PlayerMeterGroup& group, BaseMod::Api* api, int x, int y, bool flip) {
    auto& settings = SettingsManager::GetInstance();
    int32_t zPos = settings.zPos;
    uint8_t alpha8 = static_cast<uint8_t>(settings.Opacity);
    uint32_t alphaComponent = (settings.Opacity & 0xFF) << 24;

    // background
    api->NativeFunctions.DrawQuad(
        x - Dim::borderSize,
        y - Dim::borderSize,
        x + (Dim::pipWidth + Dim::pipSpacing) * group.PlayerMeter.Frames.size() + (Dim::borderSize - Dim::pipSpacing),
        y + Dim::pipHeight + Dim::borderSize,
        zPos, meterBgColor & alphaComponent
    );

    int pipX = x;
    for (int i = 0; i < group.PlayerMeter.Frames.size(); i++) {
        DrawFramePip(api, group.PlayerMeter.FrameAt(i), pipX, y, false);
        if (settings.DisplayRunSums)
            DrawFrameRunSum(api, group.PlayerMeter.RunSumAt(i), pipX, y, zPos);
        // Draw frame divider | TODO: Model frame dividers in the FrameMeter class
        if (
            group.PlayerMeter.MoveStartAt(i) &&
            group.PlayerMeter.FrameAt(i).Type != FrameType::NEUTRAL &&
            group.PlayerMeter.FrameAt(i-1).Type != FrameType::NONE &&
            group.PlayerMeter.FrameAt(i-1).Type != FrameType::NEUTRAL &&
            i != 0
        ) {
            api->NativeFunctions.DrawQuad(pipX - 1, y - 1, pipX, y + Dim::pipHeight + 1, zPos - 1, 0xFFFFFFFF);
            api->NativeFunctions.DrawQuad(pipX - 1, y - 1, pipX + 1, y, zPos - 1, 0xFFFFFFFF);
            api->NativeFunctions.DrawQuad(pipX - 1, y + Dim::pipHeight, pipX + 1, y + Dim::pipHeight + 1, zPos - 1, 0xFFFFFFFF);
        }
        pipX += Dim::pipWidth + Dim::pipSpacing;
    }

    // Entity meter
    if (!group.HideEntityMeter) {
        int pipX = x;
        int entityY = flip ?
            y + Dim::pipHeight + Dim::pipSpacing :
            y - Dim::pipHeightEntity - Dim::pipSpacing;

        // background
        api->NativeFunctions.DrawQuad(
            x - Dim::borderSize,
            entityY - Dim::borderSize,
            x + (Dim::pipWidth + Dim::pipSpacing) * group.EntityMeter.Frames.size() + (Dim::borderSize - Dim::pipSpacing),
            entityY + Dim::pipHeightEntity + Dim::borderSize,
            zPos + 1, meterBgColor & alphaComponent
        );

        for (auto& frame : group.EntityMeter.Frames) {
            DrawFramePip(api, frame, pipX, entityY, true);
            pipX += Dim::pipWidth + Dim::pipSpacing;
        }
    }

    // Draw text
    int textY = flip ?
        y + Dim::pipHeight + (Dim::pipSpacing + Dim::pipHeightEntity + Dim::borderSize + Dim::textMargin) :
        y - (Dim::pipSpacing + Dim::pipHeightEntity + Dim::borderSize + Dim::textMargin) - Dim::textHeight;
    std::string startup = "STARTUP: " + (group.Startup == -1 ? "-" : std::to_string(group.Startup));
    std::string advantage = "ADV: " + (group.DisplayAdvantage ? std::to_string(group.Advantage) : "-");
    std::string total = "TOTAL: " + (group.Total == -1 ? "-" : std::to_string(group.Total));

    api->NativeFunctions.RenderCockpitFontText(
        startup.c_str(),
        x, textY, zPos, alpha8, Dim::textScale
    );
    api->NativeFunctions.RenderCockpitFontText(
        advantage.c_str(),
        x + Dim::textAdvantageXOffset, textY, zPos, alpha8, Dim::textScale
    );
    api->NativeFunctions.RenderCockpitFontText(
        total.c_str(),
        x + Dim::textTotalXOffset, textY, zPos, alpha8, Dim::textScale
    );
}

void DrawFrameMeter(FrameMeter& frameMeter, BaseMod::Api* api) {
    int length = (Dim::pipWidth + Dim::pipSpacing) * frameMeter.size() - Dim::pipSpacing + Dim::borderSize * 2;
    int x = (640 - length) / 2;
    int y = SettingsManager::GetInstance().yPos;
    DrawMeterGroup(frameMeter.P1Meter, api, x, y, false);
    DrawMeterGroup(frameMeter.P2Meter, api, x, y + Dim::pipHeight + Dim::pipSpacing, true);
}
inline void DrawGaugeNumbersForPlayer(BaseMod::Api* api, ggxxacpr::Player player) {
    enum {
        HEALTH, GUARD, BURST, TENSION,
        EDDIE,
        ZAPPA,
        ROBO_HEAT,
        ROBO_GAIN,
        ABA,
        HOS_ACTUAL,
        HOS_TARGET,
    };
    struct GaugeNumberPositions { int X, Y, Z; };
    static const GaugeNumberPositions dim[11] = {
        {104, 47,  269},
        {128, 68,  274},
        {254, 113, 268},
        {135, 440, 55 },
        {155, 421, 57 }, // EDDIE
        {255, 427, 58 }, // ZAPPA
        {255, 428, 57 }, // ROBO Heat
        {259, 420, 57 }, // ROBO Gain
        {220, 421, 64 }, // ABA
        {147, 427, 65 }, // HOS Actual
        {147, 415, 65 }, // HOS Target
    };

    constexpr int midPointX = 320.0f;
    constexpr uint32_t backdropWhite = 0xC0FFFFFF;
    constexpr uint32_t backdropBlack = 0xC0000000;
    
    if (!player.isValid()) return;

    bool isP1 = !player.playerIndex();
    bool textAlignment = isP1;
    int offsetFactor = isP1 ? -1 : 1;

    DrawRunSumText(api, std::to_string(player.health()), midPointX + dim[HEALTH].X * offsetFactor, dim[HEALTH].Y, dim[HEALTH].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
    DrawRunSumText(api, std::to_string(player.guardBar()), midPointX + dim[GUARD].X * offsetFactor, dim[GUARD].Y, dim[GUARD].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
    DrawRunSumText(api, std::to_string(player.burstMeter()), midPointX + dim[BURST].X * offsetFactor, dim[BURST].Y, dim[BURST].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
    DrawRunSumText(api, std::to_string(player.tension()), midPointX + dim[TENSION].X * offsetFactor, dim[TENSION].Y, dim[TENSION].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);

    if (!player.getRaw()->playerEntityDataPtr) return;

    auto& charSpec = player.getRaw()->playerEntityDataPtr->characterSpecific;
    switch(player.id()) {
        case ggxxacpr::EntityId::EDDIE:
            DrawRunSumText(api, std::to_string(charSpec.Eddie.EddieGauge), midPointX + dim[EDDIE].X * offsetFactor, dim[EDDIE].Y, dim[EDDIE].Z, 1.0f, 1.0f, !textAlignment, Font::BLACK_BORDERED);
            break;
        case ggxxacpr::EntityId::ZAPPA:
            if (charSpec.Zappa.Summon == 4)
                DrawRunSumText(api, std::to_string(charSpec.Zappa.RaouTimer), midPointX + dim[ZAPPA].X * offsetFactor, dim[ZAPPA].Y, dim[ZAPPA].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            break;
        case ggxxacpr::EntityId::ROBO_KY:
            DrawRunSumText(api, std::to_string(charSpec.RoboKy.HeatGain / 111 + 30), midPointX + dim[ROBO_GAIN].X * offsetFactor, dim[ROBO_GAIN].Y, dim[ROBO_GAIN].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            DrawRunSumText(api, std::to_string(charSpec.RoboKy.Heat), midPointX + dim[ROBO_HEAT].X * offsetFactor, dim[ROBO_HEAT].Y, dim[ROBO_HEAT].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            break;
        case ggxxacpr::EntityId::ABA:
            if (charSpec.ABA.InstallMode > 0)
                DrawRunSumText(api, std::to_string(charSpec.ABA.BloodGauge), midPointX + dim[ABA].X * offsetFactor, dim[ABA].Y, dim[ABA].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            break;
        case ggxxacpr::EntityId::ORDER_SOL:
            DrawRunSumText(api, std::to_string(charSpec.OrderSol.ChargeLevelTarget), midPointX + dim[HOS_TARGET].X * offsetFactor, dim[HOS_TARGET].Y, dim[HOS_TARGET].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            DrawRunSumText(api, std::to_string(charSpec.OrderSol.ChargeLevelActual), midPointX + dim[HOS_ACTUAL].X * offsetFactor, dim[HOS_ACTUAL].Y, dim[HOS_ACTUAL].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            break;
    }
}
void DrawGaugeNumbers(BaseMod::Api* api) {
    DrawGaugeNumbersForPlayer(api, api->GameData.GetPlayer(0));
    DrawGaugeNumbersForPlayer(api, api->GameData.GetPlayer(1));
}

void Draw(FrameMeter& frameMeter, BaseMod::Api* bmApi) {
    if (SettingsManager::GetInstance().Display)
        DrawFrameMeter(frameMeter, bmApi);

    // Draw HSD gauges
    // Draw stun gauge

    if (SettingsManager::GetInstance().DisplayGaugeNumbers)
        DrawGaugeNumbers(bmApi);
}
