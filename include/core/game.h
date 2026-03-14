#pragma once

#include <array>
#include <vector>

#include "entities/player.h"
#include "entities/target.h"

namespace core
{
struct GameState
{
    entities::Player player{};
    entities::WeaponState weapon{};
    std::array<entities::Target, entities::kTargetCount> targets{};
    std::vector<float> depthBuffer;
    int hitCount = 0;
    int destroyedCount = 0;
};

GameState CreateGameState();
void ResetGame(GameState& game);
void UpdateGame(GameState& game, float deltaTime);
void DrawGame(GameState& game);
} // namespace core
