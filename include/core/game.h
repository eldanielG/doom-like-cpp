#pragma once

#include <array>
#include <vector>

#include "core/audio.h"
#include "entities/pickup.h"
#include "entities/player.h"
#include "entities/target.h"

namespace core
{
enum class GamePhase : unsigned char
{
    Title,
    Gameplay,
    Pause,
    GameOver,
    Victory,
};

enum class PauseMode : unsigned char
{
    Quick,
    Menu,
};

struct GameState
{
    AudioState audio{};
    std::array<entities::Pickup, entities::kPickupCount> pickups{};
    entities::Player player{};
    entities::WeaponState weapon{};
    std::array<entities::Target, entities::kTargetCount> targets{};
    std::vector<float> depthBuffer;
    GamePhase phase = GamePhase::Title;
    PauseMode pauseMode = PauseMode::Quick;
    int pauseMenuSelection = 0;
    int difficultyLevel = 1;
    int score = 0;
    int bestScore = 0;
    int currentScoreStage = 1;
    int hitCount = 0;
    int destroyedCount = 0;
    float survivalTime = 0.0f;
    float bestSurvivalTime = 0.0f;
    const char* pickupMessage = nullptr;
    float pickupMessageTimer = 0.0f;
    float stageAdvanceTimer = 0.0f;
    bool gameplayCursorHidden = false;
    bool shouldQuit = false;
};

GameState CreateGameState();
void DestroyGameState(GameState& game);
void ResetGame(GameState& game);
void UpdateGame(GameState& game, float deltaTime);
void DrawGame(GameState& game);
} // namespace core
