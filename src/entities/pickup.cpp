#include "entities/pickup.h"

#include <algorithm>
#include <cmath>

#include "core/tuning.h"
#include "world/world.h"

namespace
{
constexpr float kMaxDeltaTime = 1.0f / 30.0f;

float ClampDeltaTime(float deltaTime)
{
    return std::min(deltaTime, kMaxDeltaTime);
}

float LengthSquared(Vector2 value)
{
    return (value.x * value.x) + (value.y * value.y);
}

entities::Pickup MakePickup(const world::PickupSpawnPoint& spawnPoint, int spawnIndex)
{
    const bool isValidSpawn = world::CanMoveTo(spawnPoint.position);
    return entities::Pickup{
        spawnPoint.position,
        static_cast<float>(spawnIndex) * 0.85f,
        0.0f,
        spawnIndex,
        spawnPoint.type,
        isValidSpawn,
    };
}
} // namespace

namespace entities
{
std::array<Pickup, kPickupCount> MakePickups()
{
    std::array<Pickup, kPickupCount> pickups{};
    const auto& spawnPoints = world::GetPickupSpawnPoints();

    for (int index = 0; index < kPickupCount; ++index)
    {
        pickups[index] = MakePickup(spawnPoints[static_cast<std::size_t>(index)], index);
    }

    return pickups;
}

void UpdatePickups(std::array<Pickup, kPickupCount>& pickups, float deltaTime)
{
    const float clampedDeltaTime = ClampDeltaTime(deltaTime);

    for (Pickup& pickup : pickups)
    {
        pickup.bobPhase += clampedDeltaTime;

        if (pickup.active)
        {
            continue;
        }

        pickup.respawnTimer = std::max(0.0f, pickup.respawnTimer - clampedDeltaTime);

        if (pickup.respawnTimer <= 0.0f && world::CanMoveTo(pickup.position))
        {
            pickup.active = true;
        }
    }
}

PickupCollectionResult TryCollectPickups(Player& player, WeaponState& weapon, std::array<Pickup, kPickupCount>& pickups)
{
    PickupCollectionResult result{};
    const float collectDistance = world::kPlayerRadius + core::tuning::kPickup.collectRadius;
    const float collectDistanceSquared = collectDistance * collectDistance;

    for (Pickup& pickup : pickups)
    {
        if (!pickup.active)
        {
            continue;
        }

        const Vector2 toPickup = {
            pickup.position.x - player.position.x,
            pickup.position.y - player.position.y,
        };

        if (LengthSquared(toPickup) > collectDistanceSquared)
        {
            continue;
        }

        switch (pickup.type)
        {
        case PickupType::HealthPack:
        {
            if (player.health >= core::tuning::kPlayer.maxHealth)
            {
                continue;
            }

            const int previousHealth = player.health;
            player.health = std::min(core::tuning::kPlayer.maxHealth, player.health + core::tuning::kPickup.healthRestore);
            result.healthRestored = player.health - previousHealth;
            break;
        }
        case PickupType::RapidFire:
            weapon.rapidFireTimer = std::max(weapon.rapidFireTimer, core::tuning::kPickup.rapidFireDuration);
            result.rapidFireActivated = true;
            break;
        case PickupType::ScoreBonus:
            result.scoreDelta = core::tuning::kPickup.scoreBonus;
            break;
        }

        pickup.active = false;
        pickup.respawnTimer = core::tuning::kPickup.respawnDelay;
        result.collected = true;
        result.type = pickup.type;
        return result;
    }

    return result;
}

const char* GetPickupLabel(PickupType type)
{
    switch (type)
    {
    case PickupType::HealthPack:
        return "HEALTH PACK";
    case PickupType::RapidFire:
        return "RAPID FIRE";
    case PickupType::ScoreBonus:
    default:
        return "SCORE BONUS";
    }
}
} // namespace entities
