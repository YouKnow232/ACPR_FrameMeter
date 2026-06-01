#include "hooks.h"
#include <cstdint>
#include <iostream>
#include <windows.h>
#include "frameMeter/gameStateStore.h"
#include "gearLoader/ggxxacpr_c.h"

constexpr intptr_t act_update_w_fn = 0x2AE0f0;
constexpr intptr_t inject_addr_1 = 0x2ae1d4;
constexpr intptr_t inject_addr_2 = 0x2ae1dd;
constexpr uint8_t jmpOpCode = 0xE9;

template <typename... Args>
inline void ERR(Args&&... args) {
    (std::cout << "[FrameMeter] ERROR: " << ... << args) << std::endl;
}
// TODO implement this hook in BaseMod
int32_t __stdcall ActUpdateHook() {
    GGXXACPR_Entity* entity;
    uint32_t actInfo;
    asm(
        ""
        : "=D" (entity)  // EDI
        , "=S" (actInfo) // ESI
    );
    if (entity->playerIndex) {
        GameStateStore::GetInstance().P2ActInfo = actInfo;
    } else {
        GameStateStore::GetInstance().P1ActInfo = actInfo;
    }
    return 1;
}

void InstallActUpdateHook() {
    intptr_t hook = reinterpret_cast<intptr_t>(ActUpdateHook);

    intptr_t base = reinterpret_cast<intptr_t>(GetModuleHandle(NULL));
    intptr_t inject1 = base + inject_addr_1;
    intptr_t inject2 = base + inject_addr_2;
    intptr_t relativeJmp1 = hook - inject1 - 5;
    intptr_t relativeJmp2 = hook - inject2 - 5;

    DWORD oldProtect;
    WINBOOL success = VirtualProtect(
        reinterpret_cast<void*>(inject1),
        inject2 - inject1 + 6,
        PAGE_EXECUTE_READWRITE,
        &oldProtect
    );
    if (!success) {
        ERR("Virtual Protect failed");
    }

    std::byte payload1[5], payload2[5];
    payload1[0] = static_cast<std::byte>(jmpOpCode);
    payload2[0] = static_cast<std::byte>(jmpOpCode);
    memcpy(&payload1[1], &relativeJmp1, sizeof(relativeJmp1));
    memcpy(&payload2[1], &relativeJmp2, sizeof(relativeJmp2));
    memcpy(reinterpret_cast<void*>(inject1), payload1, sizeof(payload1));
    memcpy(reinterpret_cast<void*>(inject2), payload2, sizeof(payload2));

    success = VirtualProtect(
        reinterpret_cast<void*>(inject1),
        inject2 - inject1 + 6,
        oldProtect,
        &oldProtect
    );

    if (!success) {
        ERR("Virtual Protect revert failed");
    }
}

void InstallCustomHooks() {
    InstallActUpdateHook();
}
