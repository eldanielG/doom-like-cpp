#include "entities/player.h"

#include <algorithm>
#include <cmath>

#include "world/world.h"

namespace
{
constexpr float kPi = 3.1415926535f;
constexpr float kMoveSharpness = 14.0f;
constexpr float kTurnSpeed = 2.35f;
constexpr float kTurnSharpness = 18.0f;
constexpr float kMaxDeltaTime = 1.0f / 30.0f;
constexpr float kPlayerFov = 66.0f * (kPi / 180.0f);
constexpr float kWeaponCooldown = 0.18f;
constexpr float kWeaponRecoilRecovery = 7.5f;
constexpr float kWeaponFlashRecovery = 12.0f;
constexpr float kWeaponHitMarkerRecovery = 10.0f;

float NormalizeAngle(float angle)
{
    while (angle < 0.0f)
    {
        angle += 2.0f * kPi;
    }

    while (angle >= 2.0f * kPi)
    {
        angle -= 2.0f * kPi;
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
        kPlayerFov,
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

void UpdatePlayer(Player& player, float deltaTime)
{
    const float clampedDeltaTime = ClampDeltaTime(deltaTime);
    float turnInput = 0.0f;

    if (IsKeyDown(KEY_LEFT))
    {
        turnInput -= 1.0f;
    }

    if (IsKeyDown(KEY_RIGHT))
    {
        turnInput += 1.0f;
    }

    const float targetTurnVelocity = turnInput * kTurnSpeed;
    player.turnVelocity = SmoothValue(player.turnVelocity, targetTurnVelocity, kTurnSharpness, clampedDeltaTime);
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
        movementDirection.x * kMoveSpeed,
        movementDirection.y * kMoveSpeed,
    };
    const float moveSharpness = (LengthSquared(movementDirection) > 0.0f) ? kMoveSharpness : (kMoveSharpness * 1.5f);

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
    weapon.recoil = std::max(0.0f, weapon.recoil - (kWeaponRecoilRecovery * clampedDeltaTime));
    weapon.muzzleFlash = std::max(0.0f, weapon.muzzleFlash - (kWeaponFlashRecovery * clampedDeltaTime));
    weapon.hitMarker = std::max(0.0f, weapon.hitMarker - (kWeaponHitMarkerRecovery * clampedDeltaTime));
}

bool TryFireWeapon(WeaponState& weapon)
{
    const bool firePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE);

    if (!firePressed || weapon.shotCooldown > 0.0f)
    {
        return false;
    }

    weapon.shotCooldown = kWeaponCooldown;
    weapon.recoil = 1.0f;
    weapon.muzzleFlash = 1.0f;
    return true;
}
} // namespace entities
