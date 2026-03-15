#pragma once

#include <array>
#include <vector>

#include "entities/player.h"
#include "entities/target.h"
#include "raylib.h"

namespace core
{
enum class PauseMode : unsigned char;
}

namespace render
{
constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

struct RayHit
{
    float distance;
    float wallU;
    int mapX;
    int mapY;
    bool hitOnVerticalSide;
};

Camera2D BuildGameplayCamera(const entities::Player& player);
RayHit CastRay(Vector2 rayOrigin, Vector2 rayDirection);
void DrawWorld(const entities::Player& player, std::vector<float>& depthBuffer);
void DrawTargets(
    const entities::Player& player,
    const std::array<entities::Target, entities::kTargetCount>& targets,
    const std::vector<float>& depthBuffer);
void DrawWeapon(const entities::Player& player, const entities::WeaponState& weapon);
void DrawMiniMap(const entities::Player& player, const std::array<entities::Target, entities::kTargetCount>& targets);
void DrawHud(
    const entities::Player& player,
    const entities::WeaponState& weapon,
    int difficultyLevel,
    int score,
    int bestScore,
    int hitCount,
    int destroyedCount,
    int aliveCount,
    float survivalTime,
    float bestSurvivalTime,
    bool isGameOver);
void DrawTitleOverlay();
void DrawPauseOverlay(core::PauseMode pauseMode, int selectedOption);
} // namespace render
