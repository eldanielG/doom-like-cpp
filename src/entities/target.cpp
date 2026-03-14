#include "entities/target.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "render/renderer.h"
#include "world/world.h"

namespace
{
constexpr float kPi = 3.1415926535f;
constexpr float kMaxDeltaTime = 1.0f / 30.0f;
constexpr float kTargetHitFlashRecovery = 4.0f;
constexpr float kTargetReactionRecovery = 7.0f;
constexpr float kTargetDestroyFlashRecovery = 2.2f;
constexpr float kTargetRespawnDelay = 1.8f;
constexpr float kTargetRespawnFlashRecovery = 3.6f;
constexpr float kTargetAimAssist = 1.35f;
constexpr float kMinimumTargetSpawnDistanceFromPlayer = 2.75f;
constexpr float kTargetMoveSpeed = 0.9f;
constexpr float kTargetPatrolDistance = 1.25f;
constexpr float kTargetBaseSize = 0.8f;
constexpr int kTargetStartHealth = 2;

const std::array<Vector2, 4> kTargetPatrolOffsets = {{
    {kTargetPatrolDistance, 0.0f},
    {-kTargetPatrolDistance, 0.0f},
    {0.0f, kTargetPatrolDistance},
    {0.0f, -kTargetPatrolDistance},
}};

float NormalizeRelativeAngle(float angle)
{
    while (angle > kPi)
    {
        angle -= 2.0f * kPi;
    }

    while (angle < -kPi)
    {
        angle += 2.0f * kPi;
    }

    return angle;
}

float LengthSquared(Vector2 value)
{
    return (value.x * value.x) + (value.y * value.y);
}

Vector2 Normalize(Vector2 value)
{
    const float lengthSquared = LengthSquared(value);

    if (lengthSquared <= 0.0f)
    {
        return Vector2{0.0f, 0.0f};
    }

    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    return Vector2{value.x * inverseLength, value.y * inverseLength};
}

Vector2 Lerp(Vector2 from, Vector2 to, float amount)
{
    return Vector2{
        from.x + ((to.x - from.x) * amount),
        from.y + ((to.y - from.y) * amount),
    };
}

float ClampDeltaTime(float deltaTime)
{
    return std::min(deltaTime, kMaxDeltaTime);
}

std::array<bool, world::kTargetSpawnPointCount> CollectUsedTargetSpawnPoints(
    const std::array<entities::Target, entities::kTargetCount>& targets)
{
    std::array<bool, world::kTargetSpawnPointCount> usedSpawnPoints{};

    for (const entities::Target& target : targets)
    {
        if (target.destroyed || target.spawnIndex < 0 || target.spawnIndex >= world::kTargetSpawnPointCount)
        {
            continue;
        }

        usedSpawnPoints[target.spawnIndex] = true;
    }

    return usedSpawnPoints;
}

int ChooseTargetSpawnIndex(const entities::Player& player, const std::array<entities::Target, entities::kTargetCount>& targets)
{
    const std::array<bool, world::kTargetSpawnPointCount> usedSpawnPoints = CollectUsedTargetSpawnPoints(targets);
    return world::ChooseSpawnIndex(
        world::GetTargetSpawnPoints(),
        usedSpawnPoints,
        &player.position,
        kMinimumTargetSpawnDistanceFromPlayer);
}

bool IsPatrolPathValid(Vector2 startPoint, Vector2 endPoint)
{
    constexpr int kPatrolCheckSteps = 6;

    for (int step = 0; step <= kPatrolCheckSteps; ++step)
    {
        const float amount = static_cast<float>(step) / static_cast<float>(kPatrolCheckSteps);
        const Vector2 samplePoint = Lerp(startPoint, endPoint, amount);

        if (!world::CanMoveTo(samplePoint))
        {
            return false;
        }
    }

    return true;
}

Vector2 ChooseTargetPatrolPoint(Vector2 spawnPoint)
{
    std::array<int, kTargetPatrolOffsets.size()> candidateIndices{};
    int candidateCount = 0;

    for (int index = 0; index < static_cast<int>(kTargetPatrolOffsets.size()); ++index)
    {
        const Vector2 patrolPoint = {
            spawnPoint.x + kTargetPatrolOffsets[index].x,
            spawnPoint.y + kTargetPatrolOffsets[index].y,
        };

        if (!IsPatrolPathValid(spawnPoint, patrolPoint))
        {
            continue;
        }

        candidateIndices[candidateCount++] = index;
    }

    if (candidateCount == 0)
    {
        return spawnPoint;
    }

    const int offsetIndex = candidateIndices[GetRandomValue(0, candidateCount - 1)];
    return Vector2{
        spawnPoint.x + kTargetPatrolOffsets[offsetIndex].x,
        spawnPoint.y + kTargetPatrolOffsets[offsetIndex].y,
    };
}

void SpawnTarget(entities::Target& target, int spawnIndex)
{
    target.position = world::GetTargetSpawnPoints()[spawnIndex];
    target.patrolStart = target.position;
    target.patrolEnd = ChooseTargetPatrolPoint(target.position);
    target.hitFlash = 0.0f;
    target.hitReaction = 0.0f;
    target.destroyFlash = 0.0f;
    target.respawnTimer = 0.0f;
    target.respawnFlash = 1.0f;
    target.moveSpeed = kTargetMoveSpeed;
    target.health = kTargetStartHealth;
    target.spawnIndex = spawnIndex;
    target.movingToPatrolEnd = true;
    target.destroyed = false;
}

bool CanHitTarget(const entities::Player& player, const entities::Target& target, float& targetDistance)
{
    if (target.destroyed)
    {
        return false;
    }

    const Vector2 toTarget = {
        target.position.x - player.position.x,
        target.position.y - player.position.y,
    };
    const float distanceSquared = LengthSquared(toTarget);

    if (distanceSquared <= 0.0001f)
    {
        return false;
    }

    targetDistance = std::sqrt(distanceSquared);
    const float targetAngle = std::atan2(toTarget.y, toTarget.x);
    const float angleDifference = NormalizeRelativeAngle(targetAngle - player.angle);
    const float targetRadius = target.size * 0.35f;
    const float aimWindow = std::atan2(targetRadius, targetDistance) * kTargetAimAssist;

    if (std::fabs(angleDifference) > aimWindow)
    {
        return false;
    }

    const Vector2 targetDirection = {
        toTarget.x / targetDistance,
        toTarget.y / targetDistance,
    };
    const render::RayHit wallHit = render::CastRay(player.position, targetDirection);

    if (wallHit.distance + targetRadius < targetDistance)
    {
        return false;
    }

    return true;
}
} // namespace

namespace entities
{
Target MakeTarget(Vector2 spawnPosition)
{
    return Target{
        spawnPosition,
        spawnPosition,
        spawnPosition,
        kTargetBaseSize,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        kTargetMoveSpeed,
        kTargetStartHealth,
        -1,
        true,
        true,
    };
}

std::array<Target, kTargetCount> MakeTargets(const Player& player)
{
    std::array<Target, kTargetCount> targets{};

    for (int index = 0; index < kTargetCount; ++index)
    {
        targets[index] = MakeTarget(Vector2{0.0f, 0.0f});
        const int spawnIndex = ChooseTargetSpawnIndex(player, targets);
        SpawnTarget(targets[index], spawnIndex);
    }

    return targets;
}

void UpdateTarget(Target& target, float deltaTime)
{
    const float clampedDeltaTime = ClampDeltaTime(deltaTime);
    target.hitFlash = std::max(0.0f, target.hitFlash - (kTargetHitFlashRecovery * clampedDeltaTime));
    target.hitReaction = std::max(0.0f, target.hitReaction - (kTargetReactionRecovery * clampedDeltaTime));
    target.destroyFlash = std::max(0.0f, target.destroyFlash - (kTargetDestroyFlashRecovery * clampedDeltaTime));
    target.respawnFlash = std::max(0.0f, target.respawnFlash - (kTargetRespawnFlashRecovery * clampedDeltaTime));

    if (target.destroyed)
    {
        return;
    }

    const Vector2 goalPoint = target.movingToPatrolEnd ? target.patrolEnd : target.patrolStart;
    const Vector2 toGoal = {
        goalPoint.x - target.position.x,
        goalPoint.y - target.position.y,
    };
    const float distanceSquared = LengthSquared(toGoal);

    if (distanceSquared <= 0.0025f)
    {
        target.movingToPatrolEnd = !target.movingToPatrolEnd;
        return;
    }

    Vector2 movementStep = Normalize(toGoal);
    movementStep.x *= target.moveSpeed * clampedDeltaTime;
    movementStep.y *= target.moveSpeed * clampedDeltaTime;

    if (LengthSquared(movementStep) > distanceSquared)
    {
        movementStep = toGoal;
    }

    const Vector2 nextPosition = {
        target.position.x + movementStep.x,
        target.position.y + movementStep.y,
    };

    if (world::CanMoveTo(nextPosition))
    {
        target.position = nextPosition;
    }
    else
    {
        target.movingToPatrolEnd = !target.movingToPatrolEnd;
    }
}

void HandleTargetRespawns(const Player& player, std::array<Target, kTargetCount>& targets, float deltaTime)
{
    const float clampedDeltaTime = ClampDeltaTime(deltaTime);

    for (Target& target : targets)
    {
        if (!target.destroyed)
        {
            continue;
        }

        target.respawnTimer = std::max(0.0f, target.respawnTimer - clampedDeltaTime);

        if (target.respawnTimer > 0.0f)
        {
            continue;
        }

        const int spawnIndex = ChooseTargetSpawnIndex(player, targets);
        SpawnTarget(target, spawnIndex);
    }
}

bool TryHitTargets(const Player& player, std::array<Target, kTargetCount>& targets, int& hitCount, int& destroyedCount)
{
    int bestTargetIndex = -1;
    float bestTargetDistance = std::numeric_limits<float>::max();

    for (int index = 0; index < kTargetCount; ++index)
    {
        float targetDistance = 0.0f;

        if (!CanHitTarget(player, targets[index], targetDistance))
        {
            continue;
        }

        if (targetDistance < bestTargetDistance)
        {
            bestTargetDistance = targetDistance;
            bestTargetIndex = index;
        }
    }

    if (bestTargetIndex < 0)
    {
        return false;
    }

    Target& target = targets[bestTargetIndex];
    target.health = std::max(0, target.health - 1);
    target.hitFlash = 1.0f;
    target.hitReaction = 1.0f;
    ++hitCount;

    if (target.health == 0)
    {
        target.destroyed = true;
        target.destroyFlash = 1.0f;
        target.respawnTimer = kTargetRespawnDelay;
        target.respawnFlash = 0.0f;
        ++destroyedCount;
    }

    return true;
}

int CountAliveTargets(const std::array<Target, kTargetCount>& targets)
{
    int aliveCount = 0;

    for (const Target& target : targets)
    {
        if (!target.destroyed)
        {
            ++aliveCount;
        }
    }

    return aliveCount;
}
} // namespace entities
