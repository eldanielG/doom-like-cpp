#pragma once

#include <array>

#include "entities/player.h"
#include "raylib.h"

namespace entities
{
enum class PickupType : unsigned char
{
    HealthPack,
    RapidFire,
    ScoreBonus,
};

constexpr int kPickupCount = 5;

struct Pickup
{
    Vector2 position;
    float bobPhase;
    float respawnTimer;
    int spawnIndex;
    PickupType type;
    bool active;
};

struct PickupCollectionResult
{
    bool collected = false;
    PickupType type = PickupType::HealthPack;
    int healthRestored = 0;
    int scoreDelta = 0;
    bool rapidFireActivated = false;
};

std::array<Pickup, kPickupCount> MakePickups();
void UpdatePickups(std::array<Pickup, kPickupCount>& pickups, float deltaTime);
PickupCollectionResult TryCollectPickups(Player& player, WeaponState& weapon, std::array<Pickup, kPickupCount>& pickups);
const char* GetPickupLabel(PickupType type);
} // namespace entities
