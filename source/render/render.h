#pragma once

#include "baseMod/baseMod.hpp"
#include "frameMeter/frameMeter.h"

enum class Font {
    BLACK_BORDERED,
    WHITE_BORDERED,
    BLACK,
    WHITE
};

void Draw(FrameMeter& frameMeter, BaseMod::Api* bmApi);
void DrawSpriteTest(BaseMod::Api* api);
void DrawRunSumText(BaseMod::Api* bmApi, std::string text,
    float x, float y, float z,
    float alpha, float size,
    bool rightAligned, Font font);
void LoadGlyphAtlasResources();
