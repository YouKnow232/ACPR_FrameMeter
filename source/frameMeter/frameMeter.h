#pragma once

#include <vector>
#include "gameStateStore.h"
#include "gearLoader/ggxxacpr_c.h"
#include "baseMod/baseMod.hpp"


enum class FrameType {
    NONE,
    NEUTRAL,
    MOVEMENT,
    CH_STATE,
    STARTUP,
    ACTIVE,
    ACTIVE_THROW,
    RECOVERY,
    BLOCKSTUN,
    HITSTUN,
    TECHABLE_HITSTUN,
    KNOCK_DOWN_HITSTUN,
};
enum class FrameProperty {
    DEFAULT,
    INVULN_FULL,
    INVULN_STRIKE,
    INVULN_THROW,
    PARRY,
    GUARD_POINT_FULL,
    GUARD_POINT_HIGH,
    GUARD_POINT_LOW,
    ARMOR,
    FRC,
    SLASH_BACK,
};

class Frame {
public:

    FrameType Type;
    std::vector<FrameProperty> Properties;
};
class FrameArray {
public:
    FrameArray(int size)
        : Frames(size)
        , RunSums(size)
        , MoveStarts(size, false) { }
    std::vector<Frame> Frames;
    std::vector<int> RunSums;
    std::vector<bool> MoveStarts;

    Frame& FrameAt(int index);
    int RunSumAt(int index);
    bool MoveStartAt(int index);
    void SetRunSumAt(int index, int value);
    void SetMoveStartAt(int index, bool value);
    void ClearFrame(int index);

    Frame& operator[](int index) {
        return FrameAt(index);
    }
};

class PlayerMeterGroup {
public:
    PlayerMeterGroup(int size)
        : PlayerMeter(size)
        , EntityMeter(size)
    { }

    FrameArray PlayerMeter;
    FrameArray EntityMeter;
    bool HideEntityMeter = true;

    int Startup = 0;
    int Total = 0;
    int Advantage = 0;
    bool DisplayAdvantage = false;
};

class FrameMeter {
public:
    FrameMeter(int size = 80)
        : _size(size)
        , P1Meter(size)
        , P2Meter(size)
        , _frameIds(size)
    {
        _paused = true;
        _index = 0;
        _currentReplayFrame = 0;
        _forwardErasureCount = 2;
    }
    
    PlayerMeterGroup P1Meter;
    PlayerMeterGroup P2Meter;
    
    void Update(BaseMod::Api* api, GameStateStore& actUpdate);
    int size() { return _size; }
    
    private:
    std::vector<int> _frameIds;
    bool _paused = true;
    int _index = 0;
    int _size;
    int _currentReplayFrame = 0;
    int _forwardErasureCount = 2;

    void ClearMeters();
    void Rewind(int frameId);
    void UpdateIndividualMeter(BaseMod::Api* api, GGXXACPR_Entity* player, GameStateStore& actUpdate);
    void UpdateIndividualEntityMeter(GGXXACPR_Entity* player, GGXXACPR_Entity* rootEntity);
    void UpdateAdvantage();
};
