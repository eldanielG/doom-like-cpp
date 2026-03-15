#pragma once

#include <array>
#include <vector>

#include "core/audio.h"
#include "entities/player.h"
#include "entities/target.h"

namespace core
{
struct GameState
{
    AudioState audio{};
    entities::Player player{};
    entities::WeaponState weapon{};
    std::array<entities::Target, entities::kTargetCount> targets{};
    std::vector<float> depthBuffer;
    int difficultyLevel = 1;
    int score = 0;
    int bestScore = 0;
    int hitCount = 0;
    int destroyedCount = 0;
    float survivalTime = 0.0f;
    float bestSurvivalTime = 0.0f;
    bool isGameOver = false;
};

GameState CreateGameState();
void DestroyGameState(GameState& game);
void ResetGame(GameState& game);
void UpdateGame(GameState& game, float deltaTime);
void DrawGame(GameState& game);
} // namespace core
