#pragma once

#include <cstdint>

class GameStateStore {
    public:
    static GameStateStore& GetInstance() {
        static GameStateStore instance;
        return instance;
    }

    uint32_t P1ActInfo;
    uint32_t P2ActInfo;
    uint8_t P1Hitstop;
    uint8_t P2Hitstop;

    protected:
    GameStateStore() = default;
    GameStateStore(const GameStateStore&) = default;
    GameStateStore(GameStateStore&&) = default;
    GameStateStore& operator=(const GameStateStore&) = default;
    GameStateStore& operator=(GameStateStore&&) = default;
    ~GameStateStore() = default;
};
