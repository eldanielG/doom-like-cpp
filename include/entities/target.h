#pragma once

#include <array>

#include "entities/player.h"
#include "raylib.h"

namespace entities
{
constexpr int kTargetCount = 3;

struct Target
{
    Vector2 position;
    Vector2 patrolStart;
    Vector2 patrolEnd;
    float size;
    float hitFlash;
    float hitReaction;
    float destroyFlash;
    float respawnTimer;
    float respawnFlash;
    float moveSpeed;
    int health;
    int spawnIndex;
    bool movingToPatrolEnd;
    bool destroyed;
};

Target MakeTarget(Vector2 spawnPosition);
std::array<Target, kTargetCount> MakeTargets(const Player& player);
void UpdateTarget(Target& target, float deltaTime);
void HandleTargetRespawns(const Player& player, std::array<Target, kTargetCount>& targets, float deltaTime);
bool TryHitTargets(const Player& player, std::array<Target, kTargetCount>& targets, int& hitCount, int& destroyedCount);
int CountAliveTargets(const std::array<Target, kTargetCount>& targets);
} // namespace entities
