#include "entities/player.h"

#include <algorithm>
#include <cmath>

#include "core/tuning.h"
#include "world/world.h"

namespace
{
constexpr float kMaxDeltaTime = 1.0f / 30.0f;

float NormalizeAngle(float angle)
{
    while (angle < 0.0f)
    {
        angle += 2.0f * core::tuning::kPi;
    }

    while (angle >= 2.0f * core::tuning::kPi)
    {
        angle -= 2.0f * core::tuning::kPi;
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

float ClampDeltaTime(float deltaTime)
{
    return std::min(deltaTime, kMaxDeltaTime);
}

float SmoothValue(float current, float target, float sharpness, float deltaTime)
{
    const float blend = 1.0f - std::exp(-sharpness * deltaTime);
    return current + ((target - current) * blend);
}

Vector2 SmoothVector(Vector2 current, Vector2 target, float sharpness, float deltaTime)
{
    return Vector2{
        SmoothValue(current.x, target.x, sharpness, deltaTime),
        SmoothValue(current.y, target.y, sharpness, deltaTime),
    };
}

void MovePlayer(entities::Player& player, Vector2 movementStep)
{
    const Vector2 nextX = {player.position.x + movementStep.x, player.position.y};
    if (world::CanMoveTo(nextX))
    {
        player.position.x = nextX.x;
    }
    else
    {
        player.velocity.x = 0.0f;
    }

    const Vector2 nextY = {player.position.x, player.position.y + movementStep.y};
    if (world::CanMoveTo(nextY))
    {
        player.position.y = nextY.y;
    }
    else
    {
        player.velocity.y = 0.0f;
    }
}
} // namespace

namespace entities
{
Player MakePlayer(Vector2 spawnPosition)
{
    return Player{
        spawnPosition,
        Vector2{0.0f, 0.0f},
        0.0f,
        0.0f,
        core::tuning::kPlayer.fovRadians,
        core::tuning::kPlayer.maxHealth,
        0.0f,
        0.0f,
        0.0f,
    };
}

WeaponState MakeWeaponState()
{
    return WeaponState{
        0.0f,
        0.0f,
        0.0f,
        0.0f,
    };
}

void UpdatePlayerFeedback(Player& player, float deltaTime)
{
    const float clampedDeltaTime = ClampDeltaTime(deltaTime);
    player.invulnerabilityTimer = std::max(0.0f, player.invulnerabilityTimer - clampedDeltaTime);
    player.damageFlash = std::max(0.0f, player.damageFlash - (core::tuning::kPlayer.damageFlashRecovery * clampedDeltaTime));
    player.screenShake = std::max(0.0f, player.screenShake - (core::tuning::kPlayer.screenShakeRecovery * clampedDeltaTime));
}

void UpdatePlayer(Player& player, float deltaTime)
{
    const float clampedDeltaTime = ClampDeltaTime(deltaTime);
    UpdatePlayerFeedback(player, clampedDeltaTime);

    float turnInput = 0.0f;

    if (IsKeyDown(KEY_LEFT))
    {
        turnInput -= 1.0f;
    }

    if (IsKeyDown(KEY_RIGHT))
    {
        turnInput += 1.0f;
    }

    const float targetTurnVelocity = turnInput * core::tuning::kPlayer.turnSpeed;
    player.turnVelocity = SmoothValue(player.turnVelocity, targetTurnVelocity, core::tuning::kPlayer.turnSharpness, clampedDeltaTime);
    player.angle = NormalizeAngle(player.angle + (player.turnVelocity * clampedDeltaTime));

    const Vector2 forward = {std::cos(player.angle), std::sin(player.angle)};
    const Vector2 right = {-forward.y, forward.x};
    Vector2 movementInput = {0.0f, 0.0f};

    if (IsKeyDown(KEY_W))
    {
        movementInput.x += forward.x;
        movementInput.y += forward.y;
    }

    if (IsKeyDown(KEY_S))
    {
        movementInput.x -= forward.x;
        movementInput.y -= forward.y;
    }

    if (IsKeyDown(KEY_D))
    {
        movementInput.x += right.x;
        movementInput.y += right.y;
    }

    if (IsKeyDown(KEY_A))
    {
        movementInput.x -= right.x;
        movementInput.y -= right.y;
    }

    const Vector2 movementDirection = Normalize(movementInput);
    const Vector2 targetVelocity = {
        movementDirection.x * core::tuning::kPlayer.moveSpeed,
        movementDirection.y * core::tuning::kPlayer.moveSpeed,
    };
    const float moveSharpness = (LengthSquared(movementDirection) > 0.0f) ?
        core::tuning::kPlayer.moveSharpness :
        (core::tuning::kPlayer.moveSharpness * core::tuning::kPlayer.idleMoveSharpnessMultiplier);

    player.velocity = SmoothVector(player.velocity, targetVelocity, moveSharpness, clampedDeltaTime);
    const Vector2 movementStep = {
        player.velocity.x * clampedDeltaTime,
        player.velocity.y * clampedDeltaTime,
    };

    MovePlayer(player, movementStep);
}

void UpdateWeapon(WeaponState& weapon, float deltaTime)
{
    const float clampedDeltaTime = ClampDeltaTime(deltaTime);

    weapon.shotCooldown = std::max(0.0f, weapon.shotCooldown - clampedDeltaTime);
    weapon.recoil = std::max(0.0f, weapon.recoil - (core::tuning::kWeapon.recoilRecovery * clampedDeltaTime));
    weapon.muzzleFlash = std::max(0.0f, weapon.muzzleFlash - (core::tuning::kWeapon.muzzleFlashRecovery * clampedDeltaTime));
    weapon.hitMarker = std::max(0.0f, weapon.hitMarker - (core::tuning::kWeapon.hitMarkerRecovery * clampedDeltaTime));
}

bool TryFireWeapon(WeaponState& weapon)
{
    const bool firePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE);

    if (!firePressed || weapon.shotCooldown > 0.0f)
    {
        return false;
    }

    weapon.shotCooldown = core::tuning::kWeapon.cooldown;
    weapon.recoil = core::tuning::kWeapon.fireRecoil;
    weapon.muzzleFlash = core::tuning::kWeapon.fireMuzzleFlash;
    return true;
}

bool ApplyDamage(Player& player, int damage)
{
    if (damage <= 0 || player.health <= 0 || player.invulnerabilityTimer > 0.0f)
    {
        return false;
    }

    player.health = std::max(0, player.health - damage);
    player.invulnerabilityTimer = core::tuning::kPlayer.invulnerabilityDuration;
    player.damageFlash = 1.0f;
    AddScreenShake(player, core::tuning::kPlayer.damageScreenShake);

    if (player.health == 0)
    {
        player.velocity = Vector2{0.0f, 0.0f};
        player.turnVelocity = 0.0f;
    }

    return true;
}

void AddScreenShake(Player& player, float amount)
{
    if (amount <= 0.0f)
    {
        return;
    }

    player.screenShake = std::min(core::tuning::kPlayer.maxScreenShake, player.screenShake + amount);
}
} // namespace entities
