#include "render.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "baseMod/baseMod.hpp"
#include "gearLoader/gearLoader_c.h"

// DEBUG
template<typename... Args>
inline void Err(Args&&... args) {
    (std::cout << "[FrameMeter ERROR] " << ... << args) << std::endl;
}
template<typename... Args>
inline void Debug(Args&&... args) {
    (std::cout << "[FrameMeter DEBUG] " << ... << args) << std::endl;
}

const char* runSumGlyphAtlasPath = "./mods/FrameMeter/resources/font.bin";
static std::vector<char> runSumAtlasBuffer;


void DrawSpriteTest(BaseMod::Api* api) {
    constexpr int32_t spriteId = 0xA0;
    api->NativeFunctions.RegisterSprites(spriteId, runSumAtlasBuffer.data(), 0);

    GGXXACPR_DrawSpriteParams params;
    params.spriteId = spriteId;
    params.x = 320.0f;
    params.y = 190.0f;
    params.z = 200.0f;
    params.zm_x = 3.0f;
    params.zm_y = 3.0f;
    params.u0 = 0.0f;
    params.v0 = 0.0f;
    params.u1 = 1.0f;
    params.v1 = 1.0f;
    params.alpha = 1.0f;
    params.listType = 1;
    params.attribute = 0xA;
    params.size_x = 65;
    params.size_y = 25;

    api->NativeFunctions.DrawSprite(&params, false);
    params.spriteId++;
    params.y = 290.0f;
    api->NativeFunctions.DrawSprite(&params, false);
}

void DrawRunSumText(BaseMod::Api* bmApi, std::string text,
    float x, float y, float z,
    float alpha, float size, bool rightAligned, Font font
) {
    constexpr int cellPixWidth = 7;
    constexpr int cellPixHeight = 9;
    constexpr int atlasRows = 4;
    constexpr int atlasCols = 16;
    constexpr int texWidth = 98;
    constexpr int texHeight = 34;
    constexpr float pixWidthUV = 1.0f / texWidth;
    constexpr float pixHeightUV = 1.0f / texHeight;
    constexpr float cellUVWidth = static_cast<float>(cellPixWidth - 1) / texWidth;
    constexpr float cellUVHeight = static_cast<float>(cellPixHeight - 1) / texHeight;
    constexpr int invalidCell = 3;
    constexpr int atlasCount = 64;
    constexpr int32_t spriteId = 0xA0;
    
    bmApi->NativeFunctions.RegisterSprites(spriteId, runSumAtlasBuffer.data(), 0);
    
    GGXXACPR_DrawSpriteParams params;
    params.spriteId = spriteId + static_cast<int>(font);
    params.x = x;
    params.y = y;
    params.z = z;
    params.zm_x = size * cellUVWidth;
    params.zm_y = size * cellUVHeight;
    params.alpha = alpha;
    params.listType = 1;
    params.attribute = rightAligned ? 0x4 : 0x5;
    
    auto&& start = rightAligned ? text.end() : text.begin();
    auto&& end   = rightAligned ? text.begin() : text.end();
    for (int i = 0; i < text.size(); i++) {
        char c = rightAligned ? text[text.size() - 1 - i] : text[i];
        int cell = c - ' ';
        if (cell < 0 || cell >= atlasCount) cell = invalidCell;
        int x = cell % atlasCols;
        int y = cell / atlasCols;

        params.u0 = pixWidthUV + x * cellUVWidth;
        params.u1 = params.u0 + cellUVWidth;
        params.v0 = pixHeightUV + y * cellUVHeight;
        params.v1 = params.v0 + cellUVHeight;

        bmApi->NativeFunctions.DrawSprite(&params, false);
        params.x += (cellPixWidth - 3) * size * (rightAligned ? -1 : 1);
    }
}

// TODO call registersprites smarter
void EnsureGlyphAtlas(BaseMod::Api* bmApi) {
    // check if resource already loaded
    auto buffer = runSumAtlasBuffer;
    if (true) {
        bmApi->NativeFunctions.RegisterSprites(0xA0, &buffer, 1);
    }
}

inline void ProcessFileOffsets(char* buffer) {
    int32_t address = reinterpret_cast<int32_t>(buffer);
    int32_t* data = reinterpret_cast<int32_t*>(buffer);
    while (*data != 0xFFFFFFFF) {
        *data += address;
        data++;
    }
    *data = 0;
}

void LoadGlyphAtlasResources() {
    std::ifstream file(runSumGlyphAtlasPath, std::ios::binary);

    if (!file.is_open()) {
        std::cout << "[ERR] File IO failed" << std::endl;
        return;
    }

    file.seekg(0, std::ios::end);
    int32_t size = static_cast<int32_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    if (size < 0) {
        std::cout << "[ERR] file.tellg failed" << std::endl;
        return;
    }

    runSumAtlasBuffer = std::vector<char>(size);

    file.read(runSumAtlasBuffer.data(), size);
    file.close();

    ProcessFileOffsets(runSumAtlasBuffer.data());
}
