#pragma once

#include <array>

#include "entities/player.h"
#include "raylib.h"

namespace entities
{
constexpr int kTargetCount = 3;

enum class TargetType : unsigned char
{
    Scout,
    Standard,
    Tank,
};

struct Target
{
    Vector2 position;
    Vector2 patrolStart;
    Vector2 patrolEnd;
    Vector2 lastSeenPlayerPosition;
    Vector2 projectilePosition;
    Vector2 projectileVelocity;
    float size;
    float hitFlash;
    float hitReaction;
    float destroyFlash;
    float respawnTimer;
    float respawnFlash;
    float shootCooldown;
    float spawnAttackGraceTimer;
    float attackFlash;
    float chaseVisibilityGraceTimer;
    float moveSpeed;
    int health;
    int spawnIndex;
    TargetType type;
    bool projectileActive;
    bool chasingPlayer;
    bool movingToPatrolEnd;
    bool destroyed;
};

struct TargetHitResult
{
    bool hit = false;
    bool destroyed = false;
    TargetType type = TargetType::Standard;
    int scoreDelta = 0;
};

Target MakeTarget(Vector2 spawnPosition);
std::array<Target, kTargetCount> MakeTargets(const Player& player);
void UpdateTarget(Target& target, Player& player, float deltaTime, int difficultyLevel);
void HandleTargetRespawns(const Player& player, std::array<Target, kTargetCount>& targets, float deltaTime);
TargetHitResult TryHitTargets(
    const Player& player,
    std::array<Target, kTargetCount>& targets,
    int& hitCount,
    int& destroyedCount,
    int difficultyLevel);
bool TryDamagePlayerFromTargets(Player& player, const std::array<Target, kTargetCount>& targets);
int CountAliveTargets(const std::array<Target, kTargetCount>& targets);
} // namespace entities
