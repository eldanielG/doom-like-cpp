#pragma once

#include <algorithm>

#include "entities/target.h"

namespace core::tuning
{
struct PlayerTuning
{
    float moveSpeed;
    float moveSharpness;
    float idleMoveSharpnessMultiplier;
    float turnSpeed;
    float turnSharpness;
    float fovRadians;
    float collisionRadius;
    int maxHealth;
    float invulnerabilityDuration;
    float damageFlashRecovery;
    float screenShakeRecovery;
    float maxScreenShake;
    float damageScreenShake;
};

struct WeaponTuning
{
    float cooldown;
    int damagePerShot;
    float recoilRecovery;
    float muzzleFlashRecovery;
    float hitMarkerRecovery;
    float fireRecoil;
    float fireMuzzleFlash;
    float fireScreenShake;
    float hitMarkerStrength;
};

struct EnemyTuning
{
    float hitFlashRecovery;
    float reactionRecovery;
    float destroyFlashRecovery;
    float baseRespawnDelay;
    float respawnFlashRecovery;
    float attackFlashRecovery;
    float aimAssist;
    float attackRange;
    float chaseStartDistance;
    float chaseStopDistance;
    float chaseLostSightGraceTime;
    float minimumSpawnDistanceFromPlayer;
    float damagePadding;
    float patrolDistance;
    float baseSize;
    float initialShootCooldown;
};

struct EnemyVariantTuning
{
    float size;
    float patrolMoveSpeed;
    float chaseMoveSpeed;
    float projectileRadius;
    float projectileSpeed;
    float attackCooldown;
    int maxHealth;
    int contactDamage;
    int projectileDamage;
};

struct DifficultyScalingTuning
{
    float stepInterval;
    int maxLevel;
    float patrolSpeedStep;
    float chaseSpeedStep;
    float respawnReductionStep;
    float minimumRespawnFactor;
};

struct EnemyDifficultyTuning
{
    float patrolSpeedMultiplier;
    float chaseSpeedMultiplier;
    float respawnDelay;
};

struct ScoringTuning
{
    int scorePerElimination;
};

inline constexpr float kPi = 3.1415926535f;

inline constexpr PlayerTuning kPlayer{
    3.4f,
    14.0f,
    1.5f,
    2.35f,
    18.0f,
    66.0f * (kPi / 180.0f),
    0.18f,
    100,
    0.7f,
    4.5f,
    3.25f,
    1.1f,
    0.78f,
};

inline constexpr WeaponTuning kWeapon{
    0.18f,
    1,
    7.5f,
    12.0f,
    10.0f,
    1.12f,
    1.0f,
    0.28f,
    1.0f,
};

inline constexpr EnemyTuning kEnemy{
    4.0f,
    7.0f,
    1.8f,
    1.8f,
    3.6f,
    6.0f,
    1.35f,
    5.5f,
    2.65f,
    3.15f,
    2.0f,
    2.75f,
    0.12f,
    1.25f,
    0.8f,
    0.35f,
};

inline constexpr DifficultyScalingTuning kDifficulty{
    20.0f,
    6,
    0.08f,
    0.10f,
    0.07f,
    0.65f,
};

inline constexpr ScoringTuning kScoring{
    100,
};

constexpr EnemyVariantTuning GetEnemyVariantTuning(entities::TargetType type)
{
    switch (type)
    {
    case entities::TargetType::Scout:
        return EnemyVariantTuning{
            0.68f,
            1.15f,
            1.55f,
            0.07f,
            6.1f,
            1.05f,
            1,
            12,
            8,
        };
    case entities::TargetType::Tank:
        return EnemyVariantTuning{
            1.0f,
            0.72f,
            1.0f,
            0.12f,
            5.3f,
            1.28f,
            3,
            28,
            18,
        };
    case entities::TargetType::Standard:
    default:
        return EnemyVariantTuning{
            kEnemy.baseSize,
            0.9f,
            1.2f,
            0.09f,
            5.8f,
            1.15f,
            2,
            20,
            12,
        };
    }
}

constexpr EnemyDifficultyTuning GetEnemyDifficultyTuning(int difficultyLevel)
{
    const int clampedDifficultyLevel = std::clamp(difficultyLevel, 1, kDifficulty.maxLevel);
    const float difficultyStep = static_cast<float>(clampedDifficultyLevel - 1);

    return EnemyDifficultyTuning{
        1.0f + (difficultyStep * kDifficulty.patrolSpeedStep),
        1.0f + (difficultyStep * kDifficulty.chaseSpeedStep),
        kEnemy.baseRespawnDelay * std::max(kDifficulty.minimumRespawnFactor, 1.0f - (difficultyStep * kDifficulty.respawnReductionStep)),
    };
}

constexpr int CalculateDifficultyLevel(float survivalTime)
{
    const int difficultySteps = static_cast<int>(survivalTime / kDifficulty.stepInterval);
    return std::min(1 + difficultySteps, kDifficulty.maxLevel);
}
} // namespace core::tuning
