#pragma once

#include <algorithm>

#include "entities/pickup.h"
#include "entities/target.h"

namespace core::tuning
{
struct PlayerTuning
{
    float moveSpeed;
    float moveSharpness;
    float idleMoveSharpnessMultiplier;
    float keyboardTurnSpeed;
    float keyboardTurnSharpness;
    float keyboardTurnReleaseSharpnessMultiplier;
    float fovRadians;
    float collisionRadius;
    int maxHealth;
    int lowHealthWarningThreshold;
    float invulnerabilityDuration;
    float damageFlashRecovery;
    float screenShakeRecovery;
    float maxScreenShake;
    float damageScreenShake;
    float lowHealthPulseSpeed;
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
    float respawnAttackGraceDuration;
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
    float attackCooldownReductionStep;
    float respawnReductionStep;
    float minimumAttackCooldownFactor;
    float minimumRespawnFactor;
};

struct EnemyDifficultyTuning
{
    float patrolSpeedMultiplier;
    float chaseSpeedMultiplier;
    float attackCooldownMultiplier;
    float respawnDelay;
};

struct ScoringTuning
{
    int scoutEliminationScore;
    int standardEliminationScore;
    int tankEliminationScore;
    int difficultyBonusPerLevel;
    int scoreBonusPickup;
};

struct PickupTuning
{
    float collectRadius;
    float healthPackRespawnDelay;
    float rapidFireRespawnDelay;
    float scoreBonusRespawnDelay;
    int healthRestore;
    float rapidFireDuration;
    float rapidFireCooldownMultiplier;
    float feedbackDuration;
};

struct RunTuning
{
    int totalScoreStages;
    int scoreTargetBase;
    int scoreTargetGrowthPerStage;
    float stageAdvanceFeedbackDuration;
};

inline constexpr float kPi = 3.1415926535f;

inline constexpr PlayerTuning kPlayer{
    3.4f,
    14.0f,
    1.5f,
    2.55f,
    19.0f,
    1.65f,
    66.0f * (kPi / 180.0f),
    0.18f,
    100,
    30,
    0.7f,
    4.5f,
    3.25f,
    1.1f,
    0.78f,
    7.5f,
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
    2.15f,
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
    0.6f,
    0.8f,
};

inline constexpr DifficultyScalingTuning kDifficulty{
    28.0f,
    6,
    0.06f,
    0.08f,
    0.04f,
    0.05f,
    0.80f,
    0.72f,
};

inline constexpr ScoringTuning kScoring{
    85,
    100,
    140,
    12,
    80,
};

inline constexpr PickupTuning kPickup{
    0.24f,
    11.0f,
    17.0f,
    19.0f,
    30,
    5.5f,
    0.65f,
    2.0f,
};

inline constexpr RunTuning kRun{
    5,
    250,
    150,
    2.4f,
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
            5.8f,
            1.22f,
            1,
            10,
            6,
        };
    case entities::TargetType::Tank:
        return EnemyVariantTuning{
            1.0f,
            0.72f,
            1.0f,
            0.12f,
            5.3f,
            1.52f,
            3,
            24,
            14,
        };
    case entities::TargetType::Standard:
    default:
        return EnemyVariantTuning{
            kEnemy.baseSize,
            0.9f,
            1.2f,
            0.09f,
            5.8f,
            1.24f,
            2,
            18,
            10,
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
        std::max(kDifficulty.minimumAttackCooldownFactor, 1.0f - (difficultyStep * kDifficulty.attackCooldownReductionStep)),
        kEnemy.baseRespawnDelay * std::max(kDifficulty.minimumRespawnFactor, 1.0f - (difficultyStep * kDifficulty.respawnReductionStep)),
    };
}

constexpr int CalculateDifficultyLevel(float survivalTime)
{
    const int difficultySteps = static_cast<int>(survivalTime / kDifficulty.stepInterval);
    return std::min(1 + difficultySteps, kDifficulty.maxLevel);
}

constexpr int GetEliminationScore(entities::TargetType type, int difficultyLevel)
{
    const int clampedDifficultyLevel = std::clamp(difficultyLevel, 1, kDifficulty.maxLevel);
    const int difficultyBonus = (clampedDifficultyLevel - 1) * kScoring.difficultyBonusPerLevel;

    switch (type)
    {
    case entities::TargetType::Scout:
        return kScoring.scoutEliminationScore + difficultyBonus;
    case entities::TargetType::Tank:
        return kScoring.tankEliminationScore + difficultyBonus;
    case entities::TargetType::Standard:
    default:
        return kScoring.standardEliminationScore + difficultyBonus;
    }
}

constexpr float GetPickupRespawnDelay(entities::PickupType type)
{
    switch (type)
    {
    case entities::PickupType::HealthPack:
        return kPickup.healthPackRespawnDelay;
    case entities::PickupType::RapidFire:
        return kPickup.rapidFireRespawnDelay;
    case entities::PickupType::ScoreBonus:
    default:
        return kPickup.scoreBonusRespawnDelay;
    }
}

constexpr int GetPickupScoreBonus(entities::PickupType type)
{
    switch (type)
    {
    case entities::PickupType::ScoreBonus:
        return kScoring.scoreBonusPickup;
    case entities::PickupType::HealthPack:
    case entities::PickupType::RapidFire:
    default:
        return 0;
    }
}

constexpr int GetScoreStageTarget(int stage)
{
    const int clampedStage = std::max(1, stage);
    int cumulativeTarget = 0;

    for (int currentStage = 1; currentStage <= clampedStage; ++currentStage)
    {
        cumulativeTarget += kRun.scoreTargetBase + ((currentStage - 1) * kRun.scoreTargetGrowthPerStage);
    }

    return cumulativeTarget;
}

constexpr int GetStageDifficultyFloor(int stage)
{
    return std::clamp(stage, 1, kDifficulty.maxLevel);
}
} // namespace core::tuning
