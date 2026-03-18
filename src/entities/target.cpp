#include "entities/target.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "core/tuning.h"
#include "render/renderer.h"
#include "world/world.h"

namespace
{
constexpr float kPi = core::tuning::kPi;
constexpr float kMaxDeltaTime = 1.0f / 30.0f;

const std::array<Vector2, 4> kTargetPatrolOffsets = {{
    {core::tuning::kEnemy.patrolDistance, 0.0f},
    {-core::tuning::kEnemy.patrolDistance, 0.0f},
    {0.0f, core::tuning::kEnemy.patrolDistance},
    {0.0f, -core::tuning::kEnemy.patrolDistance},
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
        core::tuning::kEnemy.minimumSpawnDistanceFromPlayer);
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
    const world::TargetSpawnPoint& spawnPoint = world::GetTargetSpawnPoints()[spawnIndex];
    const core::tuning::EnemyVariantTuning archetype = core::tuning::GetEnemyVariantTuning(spawnPoint.type);
    target.position = spawnPoint.position;
    target.patrolStart = target.position;
    target.patrolEnd = ChooseTargetPatrolPoint(target.position);
    target.lastSeenPlayerPosition = target.position;
    target.projectilePosition = target.position;
    target.projectileVelocity = Vector2{0.0f, 0.0f};
    target.hitFlash = 0.0f;
    target.hitReaction = 0.0f;
    target.destroyFlash = 0.0f;
    target.respawnTimer = 0.0f;
    target.respawnFlash = 1.0f;
    target.shootCooldown = core::tuning::kEnemy.initialShootCooldown;
    target.spawnAttackGraceTimer = core::tuning::kEnemy.respawnAttackGraceDuration;
    target.attackFlash = 0.0f;
    target.chaseVisibilityGraceTimer = 0.0f;
    target.moveSpeed = archetype.patrolMoveSpeed;
    target.health = archetype.maxHealth;
    target.spawnIndex = spawnIndex;
    target.type = spawnPoint.type;
    target.size = archetype.size;
    target.projectileActive = false;
    target.chasingPlayer = false;
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
    const float aimWindow = std::atan2(targetRadius, targetDistance) * core::tuning::kEnemy.aimAssist;

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

bool CanDamagePlayer(const entities::Target& target, const entities::Player& player, float& distanceSquared)
{
    if (target.destroyed || player.health <= 0)
    {
        return false;
    }

    const Vector2 toPlayer = {
        player.position.x - target.position.x,
        player.position.y - target.position.y,
    };
    distanceSquared = LengthSquared(toPlayer);

    if (distanceSquared <= 0.0001f)
    {
        return true;
    }

    const float playerDistance = std::sqrt(distanceSquared);
    const float damageRange = world::kPlayerRadius + (target.size * 0.35f) + core::tuning::kEnemy.damagePadding;

    if (playerDistance > damageRange)
    {
        return false;
    }

    const Vector2 toPlayerDirection = {
        toPlayer.x / playerDistance,
        toPlayer.y / playerDistance,
    };
    const render::RayHit wallHit = render::CastRay(target.position, toPlayerDirection);

    return wallHit.distance + world::kPlayerRadius >= playerDistance;
}

bool HasLineOfSightToPlayer(
    const entities::Target& target,
    const entities::Player& player,
    float maximumDistance,
    float& playerDistance,
    Vector2& toPlayerDirection)
{
    const Vector2 toPlayer = {
        player.position.x - target.position.x,
        player.position.y - target.position.y,
    };
    const float distanceSquared = LengthSquared(toPlayer);

    if (distanceSquared <= 0.0001f)
    {
        playerDistance = 0.0f;
        toPlayerDirection = Vector2{0.0f, 0.0f};
        return false;
    }

    playerDistance = std::sqrt(distanceSquared);
    if (playerDistance > maximumDistance)
    {
        return false;
    }

    toPlayerDirection = {
        toPlayer.x / playerDistance,
        toPlayer.y / playerDistance,
    };
    const render::RayHit wallHit = render::CastRay(target.position, toPlayerDirection);
    return wallHit.distance + world::kPlayerRadius >= playerDistance;
}

void FireProjectile(entities::Target& target, Vector2 direction, const core::tuning::EnemyDifficultyTuning& difficulty)
{
    const core::tuning::EnemyVariantTuning archetype = core::tuning::GetEnemyVariantTuning(target.type);
    target.projectileActive = true;
    target.projectilePosition = target.position;
    target.projectileVelocity = {
        direction.x * archetype.projectileSpeed,
        direction.y * archetype.projectileSpeed,
    };
    target.shootCooldown = archetype.attackCooldown * difficulty.attackCooldownMultiplier;
    target.attackFlash = 1.0f;
}

bool MoveTarget(entities::Target& target, Vector2 movementStep)
{
    bool moved = false;

    const Vector2 nextX = {target.position.x + movementStep.x, target.position.y};
    if (world::CanMoveTo(nextX))
    {
        target.position.x = nextX.x;
        moved = moved || (std::fabs(movementStep.x) > 0.0f);
    }

    const Vector2 nextY = {target.position.x, target.position.y + movementStep.y};
    if (world::CanMoveTo(nextY))
    {
        target.position.y = nextY.y;
        moved = moved || (std::fabs(movementStep.y) > 0.0f);
    }

    return moved;
}

void UpdateProjectile(entities::Target& target, entities::Player& player, float deltaTime)
{
    if (!target.projectileActive)
    {
        return;
    }

    const core::tuning::EnemyVariantTuning archetype = core::tuning::GetEnemyVariantTuning(target.type);
    const float stepDistance = archetype.projectileSpeed * deltaTime;
    const Vector2 projectileDirection = Normalize(target.projectileVelocity);
    const render::RayHit wallHit = render::CastRay(target.projectilePosition, projectileDirection);

    if (wallHit.distance <= stepDistance)
    {
        target.projectileActive = false;
        return;
    }

    target.projectilePosition.x += target.projectileVelocity.x * deltaTime;
    target.projectilePosition.y += target.projectileVelocity.y * deltaTime;

    const Vector2 toPlayer = {
        player.position.x - target.projectilePosition.x,
        player.position.y - target.projectilePosition.y,
    };
    const float hitRadius = world::kPlayerRadius + archetype.projectileRadius;

    if (LengthSquared(toPlayer) <= (hitRadius * hitRadius))
    {
        ApplyDamage(player, archetype.projectileDamage);
        target.projectileActive = false;
    }
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
        spawnPosition,
        spawnPosition,
        Vector2{0.0f, 0.0f},
        core::tuning::kEnemy.baseSize,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        core::tuning::GetEnemyVariantTuning(TargetType::Standard).patrolMoveSpeed,
        core::tuning::GetEnemyVariantTuning(TargetType::Standard).maxHealth,
        -1,
        TargetType::Standard,
        false,
        false,
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

void UpdateTarget(Target& target, Player& player, float deltaTime, int difficultyLevel)
{
    const float clampedDeltaTime = ClampDeltaTime(deltaTime);
    const core::tuning::EnemyDifficultyTuning difficulty = core::tuning::GetEnemyDifficultyTuning(difficultyLevel);
    const core::tuning::EnemyVariantTuning archetype = core::tuning::GetEnemyVariantTuning(target.type);
    target.hitFlash = std::max(0.0f, target.hitFlash - (core::tuning::kEnemy.hitFlashRecovery * clampedDeltaTime));
    target.hitReaction = std::max(0.0f, target.hitReaction - (core::tuning::kEnemy.reactionRecovery * clampedDeltaTime));
    target.destroyFlash = std::max(0.0f, target.destroyFlash - (core::tuning::kEnemy.destroyFlashRecovery * clampedDeltaTime));
    target.respawnFlash = std::max(0.0f, target.respawnFlash - (core::tuning::kEnemy.respawnFlashRecovery * clampedDeltaTime));
    target.attackFlash = std::max(0.0f, target.attackFlash - (core::tuning::kEnemy.attackFlashRecovery * clampedDeltaTime));

    UpdateProjectile(target, player, clampedDeltaTime);

    if (target.destroyed)
    {
        return;
    }

    target.shootCooldown = std::max(0.0f, target.shootCooldown - clampedDeltaTime);
    target.spawnAttackGraceTimer = std::max(0.0f, target.spawnAttackGraceTimer - clampedDeltaTime);
    target.chaseVisibilityGraceTimer = std::max(0.0f, target.chaseVisibilityGraceTimer - clampedDeltaTime);

    const Vector2 toPlayer = {
        player.position.x - target.position.x,
        player.position.y - target.position.y,
    };
    float chasePlayerDistance = 0.0f;
    Vector2 chaseDirection = {0.0f, 0.0f};
    const bool hasChaseLineOfSight = HasLineOfSightToPlayer(
        target,
        player,
        core::tuning::kEnemy.chaseStopDistance,
        chasePlayerDistance,
        chaseDirection);
    const bool canStartChase = hasChaseLineOfSight && chasePlayerDistance <= core::tuning::kEnemy.chaseStartDistance;
    const bool canKeepChasing = hasChaseLineOfSight && chasePlayerDistance <= core::tuning::kEnemy.chaseStopDistance;

    if (target.chasingPlayer)
    {
        if (canKeepChasing)
        {
            target.lastSeenPlayerPosition = player.position;
            target.chaseVisibilityGraceTimer = core::tuning::kEnemy.chaseLostSightGraceTime;
        }
        else if (target.chaseVisibilityGraceTimer <= 0.0f)
        {
            target.chasingPlayer = false;
        }
    }
    else if (canStartChase)
    {
        target.chasingPlayer = true;
        target.lastSeenPlayerPosition = player.position;
        target.chaseVisibilityGraceTimer = core::tuning::kEnemy.chaseLostSightGraceTime;
    }

    if (!target.projectileActive)
    {
        float playerDistance = 0.0f;
        Vector2 toPlayerDirection = {0.0f, 0.0f};
        if (HasLineOfSightToPlayer(target, player, core::tuning::kEnemy.attackRange, playerDistance, toPlayerDirection) &&
            target.shootCooldown <= 0.0f &&
            target.spawnAttackGraceTimer <= 0.0f)
        {
            FireProjectile(target, toPlayerDirection, difficulty);
        }
    }

    Vector2 movementStep = {0.0f, 0.0f};
    bool reversePatrolDirection = false;

    if (target.chasingPlayer)
    {
        const Vector2 chaseGoal = canKeepChasing ? player.position : target.lastSeenPlayerPosition;
        const Vector2 toChaseGoal = {
            chaseGoal.x - target.position.x,
            chaseGoal.y - target.position.y,
        };
        const float chaseGoalDistanceSquared = LengthSquared(toChaseGoal);

        if (chaseGoalDistanceSquared > 0.0025f)
        {
            const Vector2 chaseMoveDirection = Normalize(toChaseGoal);
            const float chaseMoveSpeed = archetype.chaseMoveSpeed * difficulty.chaseSpeedMultiplier;
            movementStep.x = chaseMoveDirection.x * chaseMoveSpeed * clampedDeltaTime;
            movementStep.y = chaseMoveDirection.y * chaseMoveSpeed * clampedDeltaTime;

            if (LengthSquared(movementStep) > chaseGoalDistanceSquared)
            {
                movementStep = toChaseGoal;
            }
        }
    }
    else
    {
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

        movementStep = Normalize(toGoal);
        const float patrolMoveSpeed = target.moveSpeed * difficulty.patrolSpeedMultiplier;
        movementStep.x *= patrolMoveSpeed * clampedDeltaTime;
        movementStep.y *= patrolMoveSpeed * clampedDeltaTime;

        if (LengthSquared(movementStep) > distanceSquared)
        {
            movementStep = toGoal;
        }

        reversePatrolDirection = true;
    }

    if (!MoveTarget(target, movementStep) && reversePatrolDirection)
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

TargetHitResult TryHitTargets(
    const Player& player,
    std::array<Target, kTargetCount>& targets,
    int& hitCount,
    int& destroyedCount,
    int difficultyLevel)
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
        return TargetHitResult{};
    }

    Target& target = targets[bestTargetIndex];
    TargetHitResult result{};
    result.hit = true;
    result.type = target.type;
    target.health = std::max(0, target.health - core::tuning::kWeapon.damagePerShot);
    target.hitFlash = 1.0f;
    target.hitReaction = 1.0f;
    ++hitCount;

    if (target.health == 0)
    {
        const core::tuning::EnemyDifficultyTuning difficulty = core::tuning::GetEnemyDifficultyTuning(difficultyLevel);
        target.destroyed = true;
        target.destroyFlash = 1.0f;
        target.respawnTimer = difficulty.respawnDelay;
        target.respawnFlash = 0.0f;
        target.spawnAttackGraceTimer = 0.0f;
        ++destroyedCount;
        result.destroyed = true;
        result.scoreDelta = core::tuning::GetEliminationScore(target.type, difficultyLevel);
    }

    return result;
}

bool TryDamagePlayerFromTargets(Player& player, const std::array<Target, kTargetCount>& targets)
{
    int closestTargetIndex = -1;
    float closestDistanceSquared = std::numeric_limits<float>::max();

    for (int index = 0; index < kTargetCount; ++index)
    {
        float distanceSquared = 0.0f;

        if (!CanDamagePlayer(targets[index], player, distanceSquared))
        {
            continue;
        }

        if (distanceSquared < closestDistanceSquared)
        {
            closestDistanceSquared = distanceSquared;
            closestTargetIndex = index;
        }
    }

    if (closestTargetIndex < 0)
    {
        return false;
    }

    return ApplyDamage(player, core::tuning::GetEnemyVariantTuning(targets[closestTargetIndex].type).contactDamage);
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
