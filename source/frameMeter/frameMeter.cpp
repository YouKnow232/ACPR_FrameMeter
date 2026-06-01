#include "frameMeter.h"
#include <cstdarg>
#include <iostream>
#include <windows.h>
#include "gameStateStore.h"
#include "gearLoader/ggxxacpr_c.h"
#include "settings/settings.h"


int32_t GetReplayFrame() {
    constexpr intptr_t offset = 0x7D57D8;
    static intptr_t base = reinterpret_cast<intptr_t>(GetModuleHandle(NULL));

    return *reinterpret_cast<int32_t*>(base + offset);
}

// DEBUG
template<typename... Args>
inline void Err(Args&&... args) {
    (std::cout << "[FrameMeter ERROR] " << ... << args) << std::endl;
}
template<typename... Args>
inline void Debug(Args&&... args) {
    (std::cout << "[FrameMeter DEBUG] " << ... << args) << std::endl;
}

inline int WrapIndex(int index, int size) {
    int r = index % size;
    return r < 0 ? r + size : r;
}
inline bool HasFlag(uint32_t state, uint32_t flag) {
    return state & flag;
}
inline bool HasFlagExact(uint32_t state, uint32_t flag) {
    return (state & flag) == flag;
}
inline bool IsIgnorableFrameType(FrameType state) {
    return state == FrameType::NONE || state == FrameType::NEUTRAL;
}
inline bool HasActiveFrame(GGXXACPR_Entity* entity) {
    constexpr uint16_t hitbox = GGXXACPR_ColliderId::COLLIDER_ID_HIT_BOX;

    if (HasFlag(entity->actionState, ACTION_STATE_DISABLE_HITBOXES)) return false;

    if (entity->colliderSetPtr) {
        for (int i = 0; i < entity->boxCount; i++) {
            if (entity->colliderSetPtr[i].boxTypeId == hitbox) {
                return true;
            }
        }
    }
    if (entity->extraColliderSetPtr) {
        for (int i = 0; i < entity->boxCount; i++) {
            if (entity->extraColliderSetPtr[i].boxTypeId == hitbox) {
                return true;
            }
        }
    }

    return false;
}
inline bool HasActiveFrame(GGXXACPR_Entity* player, GGXXACPR_Entity* rootEntity) {
    constexpr uint16_t hitbox = GGXXACPR_ColliderId::COLLIDER_ID_HIT_BOX;

    if (!HasFlag(player->actionState, ACTION_STATE_DISABLE_HITBOXES)) {
        for (int i = 0; i < player->boxCount; i++) {
            if ((player->colliderSetPtr && player->colliderSetPtr[i].boxTypeId == hitbox) ||
                (player->extraColliderSetPtr && player->extraColliderSetPtr[i].boxTypeId == hitbox)
            ) {
                return true;
            }
        }
    }

    for (GGXXACPR_Entity* iEntity = rootEntity->nextPtr; iEntity != rootEntity; iEntity = iEntity->nextPtr) {
        if (iEntity->playerIndex != player->playerIndex ||
            HasFlag(iEntity->actionState, ACTION_STATE_DISABLE_HITBOXES)) { continue; }

        for (int i = 0; i < iEntity->boxCount; i++) {
            if ((player->colliderSetPtr && iEntity->colliderSetPtr[i].boxTypeId == hitbox) || 
                (iEntity->extraColliderSetPtr && iEntity->extraColliderSetPtr[i].boxTypeId == hitbox)
            ) {
                return true;
            }
        }
    }

    // check for command grabs active by mark

    return false;
}
inline bool HasActiveThrow(BaseMod::Api* api, GGXXACPR_Entity* entity) {
    return
        !HasFlag(entity->commandFlags, COMMAND_STATE_DISABLE_THROW) &&
        (api->GameData.GetGlobalThrowFlags() & (entity->playerIndex + 1)) &&
        HasFlag(api->GameData.GetCApi()->GetPlayer(entity->playerIndex == 0)->actionState, ACTION_STATE_IS_IN_HITSTUN);
}
inline bool HasActiveCommandGrab(BaseMod::Api* api, GGXXACPR_Entity* entity) {
    return api->NativeFunctions.GetActiveCommandGrabId(entity) > 0 && entity->mark != 0;
}
inline bool HasAnyHurtboxes(GGXXACPR_Entity* entity) {
    if (entity->colliderSetPtr) {
        for (int i = 0; entity->boxCount; i++) {
            if (entity->colliderSetPtr[i].boxTypeId == COLLIDER_ID_HURT_BOX) {
                return true;
            }
        }
    }
    if (entity->extraColliderSetPtr) {
        for (int i = 0; entity->boxCount; i++) {
            if (entity->extraColliderSetPtr[i].boxTypeId == COLLIDER_ID_HURT_BOX) {
                return true;
            }
        }
    }

    return false;
}

inline bool HasStrikeInvuln(GGXXACPR_Entity* entity) {
    return
        HasFlag(entity->actionState,
            ACTION_STATE_DISABLE_HURTBOXES |
            ACTION_STATE_STRIKE_INVULN |
            ACTION_STATE_PROJECTILE_INVULN) ||
        entity->playerEntityDataPtr->invulnCounter > 0 ||
        !HasAnyHurtboxes(entity);
}
inline bool HasThrowInvuln(GGXXACPR_Entity* entity) {
    return
        !HasFlag(entity->actionState, ACTION_STATE_IS_IN_HITSTUN | ACTION_STATE_IS_IN_BLOCKSTUN) &&
        (
            HasFlag(entity->actionState, ACTION_STATE_THROW_INVULN) ||
            entity->playerEntityDataPtr->throwProtectionTimer > 0
        );
}
inline bool EntityIsOutOfBounds(GGXXACPR_Entity* entity) {
    return entity->yPos < - 50000 || entity->yPos > 1000 ||
        std::abs(entity->xPos) > 80000;
}

Frame& FrameArray::FrameAt(int index) {
    return Frames[WrapIndex(index, Frames.size())];
}
int FrameArray::RunSumAt(int index){
    return RunSums[WrapIndex(index, RunSums.size())];
}
bool FrameArray::MoveStartAt(int index) {
    return MoveStarts[WrapIndex(index, RunSums.size())];
}
void FrameArray::SetRunSumAt(int index, int value){
    RunSums[WrapIndex(index, RunSums.size())] = value;
}
void FrameArray::SetMoveStartAt(int index, bool value) {
    MoveStarts[WrapIndex(index, RunSums.size())] = value;
}
void FrameArray::ClearFrame(int index) {
    int i = WrapIndex(index, Frames.size());
    Frames[i] = Frame{};
    RunSums[i] = 0;
    MoveStarts[i] = false;
}

FrameType DetermineFrameType(BaseMod::Api* api, GGXXACPR_Entity* entity) {
    constexpr int isMove = 0xC05F;
    if (!entity || !entity->playerEntityDataPtr) return FrameType::NONE;
    uint32_t throwFlags = api->GameData.GetGlobalThrowFlags();
    GGXXACPR_Entity* opponent = api->GetCApi()->GameData->GetPlayer(entity->playerIndex == 0);


    if (HasActiveFrame(entity)) {
        return FrameType::ACTIVE;
    }
    else if (HasActiveThrow(api, entity) || HasActiveCommandGrab(api, entity)) {
        return FrameType::ACTIVE_THROW;
    }
    else if (HasFlag(entity->actionState, ACTION_STATE_IS_IN_BLOCKSTUN) ||
        entity->playerEntityDataPtr->slashBackActiveWindow > 0
    ) {
        return FrameType::BLOCKSTUN;
    }
    else if (HasFlag(entity->actionState, ACTION_STATE_IS_IN_HITSTUN)) {
        if (HasFlag(entity->actionState, ACTION_STATE_KNOCKED_DOWN)) {
            return FrameType::KNOCK_DOWN_HITSTUN;
        }
        else if (HasFlag(entity->actionState, ACTION_STATE_IS_AIRBORNE) &&
            entity->playerEntityDataPtr->untechTimer < 0
        ) {
            return FrameType::TECHABLE_HITSTUN;
        }
        else{
            return FrameType::HITSTUN;
        }
    }
    else if (HasFlag(entity->attackState, ATTACK_STATE_IN_RECOVERY)) {
        return FrameType::RECOVERY;
    }
    else if (entity->commandFlags == isMove && !HasFlag(entity->attackState, ATTACK_STATE_IN_RECOVERY)) {
        return FrameType::CH_STATE;
    }
    else if (entity->commandFlags == isMove) {
        return FrameType::STARTUP;
    }
    // 0x1000
    else if (!HasFlag(entity->commandFlags, 0x420) &&
        (HasFlag(entity->commandFlags, COMMAND_STATE_PREJUMP) ||
        HasFlagExact(entity->commandFlags, 0xC00F))
    ) {
        return FrameType::MOVEMENT;
    }
    else {
        return FrameType::NEUTRAL;
    }
}
std::vector<FrameProperty> DetermineFrameProperties(GGXXACPR_Entity* player)
{
    constexpr int AXL_TENHOU_SEKI_UPPER_ACT_ID = 188;
    constexpr int AXL_TENHOU_SEKI_LOWER_ACT_ID = 189;
    constexpr int DIZZY_EX_NECRO_UNLEASHED_ACT_ID = 247;

    std::vector<FrameProperty> output;
    if (!player->playerEntityDataPtr) return output;


    if (player->playerEntityDataPtr->slashBackActiveWindow > 0) {
        output.push_back(FrameProperty::SLASH_BACK);
    }
    if (player->playerEntityDataPtr->frcTime > 0) {
        output.push_back(FrameProperty::FRC);
    }

    if (HasStrikeInvuln(player) && HasThrowInvuln(player)) {
        output.push_back(FrameProperty::INVULN_FULL);
    }
    else if (HasThrowInvuln(player)) {
        output.push_back(FrameProperty::INVULN_THROW);
    }
    else if (HasStrikeInvuln(player)) {
        output.push_back(FrameProperty::INVULN_STRIKE);
    }

    if (HasFlag(player->guardState, GUARD_STATE_ARMOR)) {
        output.push_back(FrameProperty::ARMOR);
    }
    else if (HasFlag(player->guardState, GUARD_STATE_PARRY_1 | GUARD_STATE_PARRY_2)) {
        if (player->id == ENTITY_ID_JAM) {
            // Jam parry flips her Parry2 flag for the rest of her current animation and uses a character specific counter for the active window
            // Jam parry works by swapping out "on guard" interrupt functions (stored at player->0x2C->0xC8). Since this is only called while
            //  She has a guard flag set, we'll need to check them here too.
            if (player->playerEntityDataPtr->characterSpecific.Jam.Parry == -1 &&
                (HasFlag(player->guardState, GUARD_STATE_STAND_BLOCKING | GUARD_STATE_CROUCH_BLOCKING))
            ) {
                output.push_back(FrameProperty::PARRY);
            }
        }
        // Special case for Axl parry and Dizzy EX parry super
        else if (player->id == ENTITY_ID_AXL && player->actId == AXL_TENHOU_SEKI_UPPER_ACT_ID ||
            player->id == ENTITY_ID_AXL && player->actId == AXL_TENHOU_SEKI_LOWER_ACT_ID ||
            player->id == ENTITY_ID_DIZZY && player->actId == DIZZY_EX_NECRO_UNLEASHED_ACT_ID
        ) {
            // These moves are marked as in parry state for their full animation and use a special move specific
            //  variable (Player.Mark) to actually determine if the move should parry.
            if (player->mark == 1) {
                output.push_back(FrameProperty::PARRY);
            }
        }
        else {
            output.push_back(FrameProperty::PARRY);
        }
    }
    else if (HasFlag(player->guardState, GUARD_STATE_GUARD_POINT)) {
        if (HasFlag(player->guardState, GUARD_STATE_STAND_BLOCKING) && HasFlag(player->guardState, GUARD_STATE_CROUCH_BLOCKING)) {
            output.push_back(FrameProperty::GUARD_POINT_FULL);
        }
        else if (HasFlag(player->guardState, GUARD_STATE_STAND_BLOCKING)) {
            output.push_back(FrameProperty::GUARD_POINT_HIGH);
        }
        else if (HasFlag(player->guardState, GUARD_STATE_CROUCH_BLOCKING)) {
            output.push_back(FrameProperty::GUARD_POINT_LOW);
        }
    }

    return output;
}
FrameType DetermineEntityFrameType(GGXXACPR_Entity* rootEntity, int playerIndex) {
    FrameType output = FrameType::NONE;
    auto& settings = SettingsManager::GetInstance();

    for (GGXXACPR_Entity* iEntity = rootEntity->nextPtr; iEntity != rootEntity; iEntity = iEntity->nextPtr) {
        if (iEntity->playerIndex != playerIndex ||
            (settings.IgnoreDistantProjectiles && EntityIsOutOfBounds(iEntity))
        ) {
            continue;
        }

        bool disableHitboxes = HasFlag(iEntity->actionState, ACTION_STATE_DISABLE_HITBOXES);
        for (int i = 0; i < iEntity->boxCount; i++) {
            uint16_t mainBoxType = iEntity->colliderSetPtr ? iEntity->colliderSetPtr[i].boxTypeId : COLLIDER_ID_DUMMY;
            uint16_t extraBoxType = iEntity->extraColliderSetPtr ? iEntity->extraColliderSetPtr[i].boxTypeId : COLLIDER_ID_DUMMY;
            if ((mainBoxType == COLLIDER_ID_HIT_BOX || extraBoxType == COLLIDER_ID_HIT_BOX) && !disableHitboxes) {
                return FrameType::ACTIVE;
            } else if (mainBoxType == COLLIDER_ID_HURT_BOX || extraBoxType == COLLIDER_ID_HURT_BOX) {
                output = FrameType::STARTUP;
            }
        }
    }

    return output;
}

void FrameMeter::UpdateIndividualMeter(BaseMod::Api* api, GGXXACPR_Entity* player, GameStateStore& actUpdate) {
    auto& frameArr = player->playerIndex == 0 ? P1Meter.PlayerMeter : P2Meter.PlayerMeter;
    frameArr[_index] = Frame {
        DetermineFrameType(api, player),
        DetermineFrameProperties(player)
    };
    // record move start
    uint32_t actInfo = player->playerIndex ? actUpdate.P2ActInfo : actUpdate.P1ActInfo;
    if (actInfo != 0 && player->actId == (actInfo & 0xFFFF)
    ) {
        frameArr.SetMoveStartAt(_index, true);
        if (player->playerIndex) {
            actUpdate.P2ActInfo = 0;
        } else {
            actUpdate.P1ActInfo = 0;
        }
    }

    // Forward erasure
    frameArr[_index + _forwardErasureCount] = Frame { };
    frameArr.SetMoveStartAt(_index + _forwardErasureCount, false);
}
void FrameMeter::UpdateIndividualEntityMeter(GGXXACPR_Entity* player, GGXXACPR_Entity* rootEntity) {
    auto& group = player->playerIndex == 0 ? P1Meter : P2Meter;
    auto type = DetermineEntityFrameType(rootEntity, player->playerIndex);
    group.EntityMeter[_index] = Frame {
        type
    };
    group.EntityMeter[_index + _forwardErasureCount] = Frame { }; // Forward erasure

    // Update hide
    group.HideEntityMeter = true;
    for (auto& frame : group.EntityMeter.Frames) {
        if (frame.Type != FrameType::NONE) {
            group.HideEntityMeter = false;
            break;
        }
    }
}

inline void UpdateStartup(PlayerMeterGroup& group, int index) {
    if (
        (
            group.PlayerMeter.FrameAt(index).Type == FrameType::ACTIVE ||
            group.PlayerMeter.FrameAt(index).Type == FrameType::ACTIVE_THROW
        ) &&
        group.PlayerMeter.FrameAt(index - 1).Type != FrameType::ACTIVE &&
        group.PlayerMeter.FrameAt(index - 1).Type != FrameType::ACTIVE_THROW
    ) {
        for (int i = 0; i < group.PlayerMeter.Frames.size(); i++) {
            if (
                group.PlayerMeter.MoveStartAt(index - i) ||
                group.PlayerMeter.FrameAt(index - i - 1).Type == FrameType::NEUTRAL ||
                group.PlayerMeter.FrameAt(index - i - 1).Type == FrameType::NONE
            ) {
                group.Startup = i + 1;
                return;
            }
        }
    }
}
inline void UpdateTotal(PlayerMeterGroup& group, int index) {
    if (group.PlayerMeter.FrameAt(index).Type == FrameType::NEUTRAL &&
        group.PlayerMeter.FrameAt(index - 1).Type != FrameType::NEUTRAL
    ) {
        for (int i = 1; i < group.PlayerMeter.Frames.size(); i++) {
            if (group.PlayerMeter.MoveStartAt(index - i)) {
                group.Total = i;
                return;
            }
            if (group.PlayerMeter.FrameAt(index - i).Type == FrameType::NEUTRAL ||
                group.PlayerMeter.FrameAt(index - i).Type == FrameType::NONE
            ) {
                group.Total = i - 1;
                return;
            }
        }
    }
}

inline void UpdateAdvantage_Helper(PlayerMeterGroup& a, PlayerMeterGroup& b, int index, int size) {
    if (a.PlayerMeter[index].Type == FrameType::NEUTRAL &&
        a.PlayerMeter[index - 1].Type != FrameType::NEUTRAL &&
        b.PlayerMeter[index].Type == FrameType::NEUTRAL
    ) {
        for (int i = 1; i < size; i++) {
            if (b.PlayerMeter[index - i].Type != FrameType::NEUTRAL &&
                b.PlayerMeter[index - i].Type != FrameType::NONE
            ) {
                a.Advantage = 1 - i;
                b.Advantage = i - 1;
                a.DisplayAdvantage = true;
                b.DisplayAdvantage = true;
                return;
            }
        }
    }
}
void FrameMeter::UpdateAdvantage() {
    UpdateAdvantage_Helper(P1Meter, P2Meter, _index, _size);
    UpdateAdvantage_Helper(P2Meter, P1Meter, _index, _size);
}
void UpdateRunSums(PlayerMeterGroup& group, int index) {
    group.PlayerMeter.SetRunSumAt(index + 5, 0); // Forward erasure

    FrameType runType = group.PlayerMeter.FrameAt(index - 1).Type;
    if ((group.PlayerMeter.FrameAt(index).Type != runType) &&
        runType != FrameType::NONE &&
        runType != FrameType::NEUTRAL
    ) {
        for (int i = 1; i < group.PlayerMeter.Frames.size(); i++) {
            if (group.PlayerMeter.FrameAt(index - 1 - i).Type != runType) {
                group.PlayerMeter.SetRunSumAt(index - 1, i > 7 ? i : 0);
                return;
            }
        }
    }

    group.PlayerMeter.RunSums[index] = 0;
}

void FrameMeter::Rewind(int frameId) {
    int i;
    for (i = 1; i < _size; i++) {
        Debug("[Rewinding] i:", _index - i, ", fId:", _frameIds[WrapIndex(_index - i, _size)]);
        auto&& iFrameId = _frameIds[WrapIndex(_index - i, _size)];
        if (iFrameId == 0) { // Assume rewind has completely wrapped to erased frame
            Debug("Rewinding limit: Reseting Frame Meter");
            ClearMeters();
            return;
        }
        if (iFrameId <= frameId) break;

        
        P1Meter.PlayerMeter.ClearFrame(_index - i);
        P1Meter.EntityMeter.ClearFrame(_index - i);
        P2Meter.PlayerMeter.ClearFrame(_index - i);
        P2Meter.EntityMeter.ClearFrame(_index - i);
        iFrameId = 0;
    }

    Debug("Rewinded index from ", _index, " to ", WrapIndex(_index - i, _size));
    _index = WrapIndex(_index + 1 - i, _size);
}

void FrameMeter::Update(BaseMod::Api* api, GameStateStore& actUpdate) {
    auto gameDataApi = api->GetCApi()->GameData;
    GGXXACPR_Entity* P1 = gameDataApi->GetPlayer(0);
    GGXXACPR_Entity* P2 = gameDataApi->GetPlayer(1);
    GGXXACPR_Entity* rootEntity = gameDataApi->GetRootEntity();
    bool replay = gameDataApi->GetMainMenuSelection() == MAIN_MENU_ITEM_REPLAY_THEATER;
    
    if (!P1 || !P2 || !rootEntity) return;
    
    // Rewinding only handled in replay mode. (May need to extend this
    //  to online mode to handle rollbacks if supporting that mode)
    int32_t replayFrame = GetReplayFrame();
    if (replay)
    {
        Debug("Replay frame: ", replayFrame, " | Curren FM frame: ", _currentReplayFrame);
        int frameDiff = replayFrame - _currentReplayFrame;

        if (frameDiff == 0 || replayFrame == 0) {
            Debug("FrameDiff is zero.");
            return;
        }
        if (frameDiff < 0) {
            Debug("Rewinding");
            Rewind(replayFrame);
            _currentReplayFrame = replayFrame;
            return;
        }

        _currentReplayFrame = replayFrame;
    }
    
    // Skip update if either player is frozen in super flash while the opponent doesn't have an active hitbox.
    // The hitbox requirement is for moves that become active while in super flash.
    // Special exception for first freeze frame.
    if ((HasFlag(P1->actionState, ACTION_STATE_SUPER_FLASH) || HasFlag(P2->actionState, ACTION_STATE_SUPER_FLASH)) &&
        !SettingsManager::GetInstance().RecordSuperFlash
    ) {
        // TODO: account for projectile supers that hit during flash

        // Special case for super's that connect during super flash (e.g. Jam 632146S)
        // Rewrite the previous frame to an active frame and recalculate startup
        if (HasFlag(P2->actionState, ACTION_STATE_SUPER_FLASH) && HasActiveFrame(P1, rootEntity))
        {
            P1Meter.PlayerMeter[_index - 1].Type = FrameType::ACTIVE;

            _index = WrapIndex(_index - 1, _size);
            UpdateStartup(P1Meter, _index);
            _index = WrapIndex(_index + 1, _size);
        }
        else if (HasFlag(P1->actionState, ACTION_STATE_SUPER_FLASH) && HasActiveFrame(P2, rootEntity))
        {
            P2Meter.PlayerMeter[_index - 1].Type = FrameType::ACTIVE;

            _index = WrapIndex(_index - 1, _size);
            UpdateStartup(P2Meter, _index);
            _index = WrapIndex(_index + 1, _size);
        }

        return;
    }

    // Hitstop update bailout
    if ((
            P1->hitstopTime > 0 &&
            P2->hitstopTime > 0 ||
            // Check saved hitstop value before update as well. The game handles hitstop before decrementing
            //  the counter, so we'll read hitstop as zero despite the frame still being frozen.
            GameStateStore::GetInstance().P1Hitstop > 0 &&
            GameStateStore::GetInstance().P2Hitstop > 0
        ) &&
        // Certain Supers apply hitstop during super flash. Need to discount this case here.
        !HasFlag(P1->actionState, ACTION_STATE_SUPER_FLASH) &&
        !HasFlag(P1->actionState, ACTION_STATE_SUPER_FLASH) &&
        !SettingsManager::GetInstance().RecordHitstop
    ) {
        return;
    }


    // Pause logic
    if (_paused)
    {
        if (!IsIgnorableFrameType(DetermineFrameType(api, P1)) ||
            !IsIgnorableFrameType(DetermineFrameType(api, P2))
        ) {
            _paused = false;
            ClearMeters();
        } else {
            return;
        }
    }

    // Update each meter
    UpdateIndividualMeter(api, P1, actUpdate);
    UpdateIndividualMeter(api, P2, actUpdate);
    UpdateIndividualEntityMeter(P1, rootEntity);
    UpdateIndividualEntityMeter(P2, rootEntity);
    if (replay) _frameIds[_index] = replayFrame;

    // Check if frame meter should pause
    // TODO: account for frame properties?
    _paused = true;
    for (int i = 0; i < SettingsManager::GetInstance().PauseThreshold; i++)
    {
        FrameType p1FrameType = P1Meter.PlayerMeter[_index - i].Type;
        FrameType p2FrameType = P2Meter.PlayerMeter[_index - i].Type;

        if (!IsIgnorableFrameType(p1FrameType) || !IsIgnorableFrameType(p2FrameType) ||
            !IsIgnorableFrameType(P1Meter.EntityMeter[_index - i].Type) ||
            !IsIgnorableFrameType(P2Meter.EntityMeter[_index - i].Type)
        ) {
            _paused = false;
            break;
        }
    }

    // Labels
    UpdateStartup(P1Meter, _index);
    UpdateStartup(P2Meter, _index);
    UpdateAdvantage();
    UpdateTotal(P1Meter, _index);
    UpdateTotal(P2Meter, _index);
    UpdateRunSums(P1Meter, _index);
    UpdateRunSums(P2Meter, _index);

    _index = WrapIndex(_index + 1, _size);
}

void FrameMeter::ClearMeters() {
    P1Meter.PlayerMeter.Frames.assign(_size, Frame{});
    P1Meter.PlayerMeter.RunSums.assign(_size, 0);
    P1Meter.PlayerMeter.MoveStarts.assign(_size, false);
    P1Meter.EntityMeter.Frames.assign(_size, Frame{});
    P1Meter.Startup = -1;
    P1Meter.Advantage = -1;
    P1Meter.Total = -1;
    P1Meter.DisplayAdvantage = false;
    P1Meter.HideEntityMeter = true;

    P2Meter.PlayerMeter.Frames.assign(_size, Frame{});
    P2Meter.PlayerMeter.RunSums.assign(_size, 0);
    P2Meter.PlayerMeter.MoveStarts.assign(_size, false);
    P2Meter.EntityMeter.Frames.assign(_size, Frame{});
    P2Meter.Startup = -1;
    P2Meter.Advantage = -1;
    P2Meter.Total = -1;
    P2Meter.DisplayAdvantage = false;
    P2Meter.HideEntityMeter = true;

    _index = 0;
}
