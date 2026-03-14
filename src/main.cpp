#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "raylib.h"

namespace
{
constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

constexpr int kMapWidth = 12;
constexpr int kMapHeight = 12;

constexpr float kPi = 3.1415926535f;
constexpr float kPlayerRadius = 0.18f;
constexpr float kMoveSpeed = 3.4f;
constexpr float kMoveSharpness = 14.0f;
constexpr float kTurnSpeed = 2.35f;
constexpr float kTurnSharpness = 18.0f;
constexpr float kMaxDeltaTime = 1.0f / 30.0f;
constexpr float kPlayerFov = 66.0f * (kPi / 180.0f);
constexpr float kShadeStartDistance = 1.0f;
constexpr float kShadeEndDistance = 11.0f;
constexpr float kWeaponCooldown = 0.18f;
constexpr float kWeaponRecoilRecovery = 7.5f;
constexpr float kWeaponFlashRecovery = 12.0f;
constexpr float kTargetHitFlashRecovery = 4.0f;
constexpr float kTargetAimAssist = 1.35f;
const Vector2 kPlayerSpawn = {1.5f, 1.5f};
const Vector2 kTargetSpawn = {8.5f, 1.5f};

using TileRow = std::array<int, kMapWidth>;
using Map = std::array<TileRow, kMapHeight>;

// 1 = wall, 0 = empty space.
const Map kMap = {{
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
    {{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}},
    {{1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1}},
    {{1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1}},
    {{1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1}},
    {{1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1}},
    {{1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1}},
    {{1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1}},
    {{1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1}},
    {{1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1}},
    {{1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1}},
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
}};

struct Player
{
    Vector2 position;
    Vector2 velocity;
    float angle;
    float turnVelocity;
    float fov;
};

struct RayHit
{
    float distance;
    float wallU;
    int mapX;
    int mapY;
    bool hitOnVerticalSide;
};

struct WeaponState
{
    float recoil;
    float shotCooldown;
    float muzzleFlash;
};

struct Target
{
    Vector2 position;
    float size;
    float hitFlash;
};

RayHit CastRay(Vector2 rayOrigin, Vector2 rayDirection);

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

int CellFromWorld(float value)
{
    return static_cast<int>(std::floor(value));
}

float Fract(float value)
{
    return value - std::floor(value);
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

Color ScaleColor(Color color, float brightness)
{
    const float clampedBrightness = std::clamp(brightness, 0.0f, 1.0f);

    return Color{
        static_cast<unsigned char>(static_cast<float>(color.r) * clampedBrightness),
        static_cast<unsigned char>(static_cast<float>(color.g) * clampedBrightness),
        static_cast<unsigned char>(static_cast<float>(color.b) * clampedBrightness),
        color.a,
    };
}

Color LerpColor(Color from, Color to, float amount)
{
    const float clampedAmount = std::clamp(amount, 0.0f, 1.0f);

    return Color{
        static_cast<unsigned char>(static_cast<float>(from.r) + ((static_cast<float>(to.r) - static_cast<float>(from.r)) * clampedAmount)),
        static_cast<unsigned char>(static_cast<float>(from.g) + ((static_cast<float>(to.g) - static_cast<float>(from.g)) * clampedAmount)),
        static_cast<unsigned char>(static_cast<float>(from.b) + ((static_cast<float>(to.b) - static_cast<float>(from.b)) * clampedAmount)),
        255,
    };
}

bool IsWall(int mapX, int mapY)
{
    if (mapX < 0 || mapX >= kMapWidth || mapY < 0 || mapY >= kMapHeight)
    {
        return true;
    }

    return kMap[mapY][mapX] != 0;
}

bool CanMoveTo(Vector2 position)
{
    const std::array<Vector2, 4> samplePoints = {{
        {position.x - kPlayerRadius, position.y - kPlayerRadius},
        {position.x + kPlayerRadius, position.y - kPlayerRadius},
        {position.x - kPlayerRadius, position.y + kPlayerRadius},
        {position.x + kPlayerRadius, position.y + kPlayerRadius},
    }};

    for (const Vector2& point : samplePoints)
    {
        if (IsWall(CellFromWorld(point.x), CellFromWorld(point.y)))
        {
            return false;
        }
    }

    return true;
}

void MovePlayer(Player& player, Vector2 movementStep)
{
    const Vector2 nextX = {player.position.x + movementStep.x, player.position.y};
    if (CanMoveTo(nextX))
    {
        player.position.x = nextX.x;
    }
    else
    {
        player.velocity.x = 0.0f;
    }

    const Vector2 nextY = {player.position.x, player.position.y + movementStep.y};
    if (CanMoveTo(nextY))
    {
        player.position.y = nextY.y;
    }
    else
    {
        player.velocity.y = 0.0f;
    }
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
}

void UpdateTarget(Target& target, float deltaTime)
{
    const float clampedDeltaTime = ClampDeltaTime(deltaTime);
    target.hitFlash = std::max(0.0f, target.hitFlash - (kTargetHitFlashRecovery * clampedDeltaTime));
}

bool TryHitTarget(const Player& player, Target& target)
{
    const Vector2 toTarget = {
        target.position.x - player.position.x,
        target.position.y - player.position.y,
    };
    const float distanceSquared = LengthSquared(toTarget);

    if (distanceSquared <= 0.0001f)
    {
        return false;
    }

    const float targetDistance = std::sqrt(distanceSquared);
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
    const RayHit wallHit = CastRay(player.position, targetDirection);

    if (wallHit.distance + targetRadius < targetDistance)
    {
        return false;
    }

    target.hitFlash = 1.0f;
    return true;
}

void TryFireWeapon(const Player& player, WeaponState& weapon, Target& target, int& hitCount)
{
    const bool firePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE);

    if (!firePressed || weapon.shotCooldown > 0.0f)
    {
        return;
    }

    weapon.shotCooldown = kWeaponCooldown;
    weapon.recoil = 1.0f;
    weapon.muzzleFlash = 1.0f;

    if (TryHitTarget(player, target))
    {
        ++hitCount;
    }
}

RayHit CastRay(Vector2 rayOrigin, Vector2 rayDirection)
{
    const float epsilon = 0.0001f;

    int mapX = CellFromWorld(rayOrigin.x);
    int mapY = CellFromWorld(rayOrigin.y);

    const float deltaDistanceX =
        (std::fabs(rayDirection.x) < epsilon) ? std::numeric_limits<float>::max() : std::fabs(1.0f / rayDirection.x);
    const float deltaDistanceY =
        (std::fabs(rayDirection.y) < epsilon) ? std::numeric_limits<float>::max() : std::fabs(1.0f / rayDirection.y);

    int stepX = 0;
    int stepY = 0;
    float sideDistanceX = 0.0f;
    float sideDistanceY = 0.0f;

    if (rayDirection.x < 0.0f)
    {
        stepX = -1;
        sideDistanceX = (rayOrigin.x - static_cast<float>(mapX)) * deltaDistanceX;
    }
    else
    {
        stepX = 1;
        sideDistanceX = (static_cast<float>(mapX) + 1.0f - rayOrigin.x) * deltaDistanceX;
    }

    if (rayDirection.y < 0.0f)
    {
        stepY = -1;
        sideDistanceY = (rayOrigin.y - static_cast<float>(mapY)) * deltaDistanceY;
    }
    else
    {
        stepY = 1;
        sideDistanceY = (static_cast<float>(mapY) + 1.0f - rayOrigin.y) * deltaDistanceY;
    }

    bool hitOnVerticalSide = false;

    while (true)
    {
        if (sideDistanceX < sideDistanceY)
        {
            sideDistanceX += deltaDistanceX;
            mapX += stepX;
            hitOnVerticalSide = true;
        }
        else
        {
            sideDistanceY += deltaDistanceY;
            mapY += stepY;
            hitOnVerticalSide = false;
        }

        if (IsWall(mapX, mapY))
        {
            break;
        }
    }

    float distance = 0.0f;

    if (hitOnVerticalSide)
    {
        const float divisor = (std::fabs(rayDirection.x) < epsilon) ? epsilon : rayDirection.x;
        distance = (static_cast<float>(mapX) - rayOrigin.x + static_cast<float>(1 - stepX) * 0.5f) / divisor;
    }
    else
    {
        const float divisor = (std::fabs(rayDirection.y) < epsilon) ? epsilon : rayDirection.y;
        distance = (static_cast<float>(mapY) - rayOrigin.y + static_cast<float>(1 - stepY) * 0.5f) / divisor;
    }

    if (distance < epsilon)
    {
        distance = epsilon;
    }

    float wallU = 0.0f;

    if (hitOnVerticalSide)
    {
        wallU = Fract(rayOrigin.y + (distance * rayDirection.y));
        if (rayDirection.x > 0.0f)
        {
            wallU = 1.0f - wallU;
        }
    }
    else
    {
        wallU = Fract(rayOrigin.x + (distance * rayDirection.x));
        if (rayDirection.y < 0.0f)
        {
            wallU = 1.0f - wallU;
        }
    }

    return RayHit{distance, wallU, mapX, mapY, hitOnVerticalSide};
}

Color ShadeWall(Color baseColor, float distance, bool hitOnVerticalSide)
{
    const float shadeFactor = std::clamp(
        (distance - kShadeStartDistance) / (kShadeEndDistance - kShadeStartDistance),
        0.0f,
        1.0f);

    float brightness = 1.0f - (0.72f * shadeFactor);

    if (hitOnVerticalSide)
    {
        brightness *= 0.82f;
    }

    return ScaleColor(baseColor, brightness);
}

Color SampleWallColor(const RayHit& hit, float wallV)
{
    constexpr float kBrickColumns = 3.25f;
    constexpr float kBrickRows = 8.0f;
    constexpr float kMortarThickness = 0.08f;

    const Color brickDarkColor = {124, 70, 54, 255};
    const Color brickLightColor = {182, 106, 84, 255};
    const Color mortarColor = {84, 76, 68, 255};

    const float brickRow = wallV * kBrickRows;
    const float rowIndex = std::floor(brickRow);
    const float rowFrac = Fract(brickRow);

    const float rowOffset = (static_cast<int>(rowIndex) & 1) ? 0.5f : 0.0f;
    const float brickColumn = (hit.wallU * kBrickColumns) + rowOffset;
    const float columnIndex = std::floor(brickColumn);
    const float columnFrac = Fract(brickColumn);

    if (rowFrac < kMortarThickness || rowFrac > (1.0f - kMortarThickness) ||
        columnFrac < kMortarThickness || columnFrac > (1.0f - kMortarThickness))
    {
        return ShadeWall(mortarColor, hit.distance, hit.hitOnVerticalSide);
    }

    const float brickSeed = static_cast<float>((hit.mapX * 13) + (hit.mapY * 29)) + (columnIndex * 5.0f) + (rowIndex * 7.0f);
    const float brickVariation = 0.5f + (0.5f * std::sin(brickSeed));
    Color brickColor = LerpColor(brickDarkColor, brickLightColor, brickVariation);

    const float edgeDepth = 1.0f - std::fabs((columnFrac * 2.0f) - 1.0f);
    const float rowDepth = 1.0f - std::fabs((rowFrac * 2.0f) - 1.0f);
    const float reliefShade = 0.76f + (0.18f * edgeDepth) + (0.06f * rowDepth);
    brickColor = ScaleColor(brickColor, reliefShade);

    return ShadeWall(brickColor, hit.distance, hit.hitOnVerticalSide);
}

void DrawWallColumn(int column, int wallTop, int wallBottom, const RayHit& hit)
{
    const int wallHeight = wallBottom - wallTop + 1;
    const int sliceTarget = std::clamp(wallHeight / 4, 8, 64);
    const int sliceCount = std::max(1, std::min(wallHeight, sliceTarget));

    for (int sliceIndex = 0; sliceIndex < sliceCount; ++sliceIndex)
    {
        const int sliceTop = wallTop + ((sliceIndex * wallHeight) / sliceCount);
        const int sliceBottom = wallTop + ((((sliceIndex + 1) * wallHeight) / sliceCount) - 1);
        const float wallV = (static_cast<float>(sliceIndex) + 0.5f) / static_cast<float>(sliceCount);
        const Color wallColor = SampleWallColor(hit, wallV);

        DrawLine(column, sliceTop, column, std::max(sliceTop, sliceBottom), wallColor);
    }
}

void DrawWorld(const Player& player, std::array<float, kScreenWidth>& depthBuffer)
{
    const Color ceilingColor = {40, 42, 58, 255};
    const Color floorColor = {58, 36, 28, 255};
    DrawRectangle(0, 0, kScreenWidth, kScreenHeight / 2, ceilingColor);
    DrawRectangle(0, kScreenHeight / 2, kScreenWidth, kScreenHeight / 2, floorColor);

    const Vector2 forward = {std::cos(player.angle), std::sin(player.angle)};
    const float planeScale = std::tan(player.fov * 0.5f);
    const Vector2 cameraPlane = {-forward.y * planeScale, forward.x * planeScale};

    for (int column = 0; column < kScreenWidth; ++column)
    {
        // Sampling from the center of each column makes the projection a bit more stable.
        const float cameraX = (2.0f * (static_cast<float>(column) + 0.5f) / static_cast<float>(kScreenWidth)) - 1.0f;
        const Vector2 rayDirection = {
            forward.x + (cameraPlane.x * cameraX),
            forward.y + (cameraPlane.y * cameraX),
        };

        const RayHit hit = CastRay(player.position, rayDirection);
        const int wallHeight = static_cast<int>(static_cast<float>(kScreenHeight) / hit.distance);
        depthBuffer[column] = hit.distance;

        int wallTop = (kScreenHeight / 2) - (wallHeight / 2);
        int wallBottom = (kScreenHeight / 2) + (wallHeight / 2);

        if (wallTop < 0)
        {
            wallTop = 0;
        }

        if (wallBottom >= kScreenHeight)
        {
            wallBottom = kScreenHeight - 1;
        }

        DrawWallColumn(column, wallTop, wallBottom, hit);
    }
}

void DrawTarget(const Player& player, const Target& target, const std::array<float, kScreenWidth>& depthBuffer)
{
    const Vector2 toTarget = {
        target.position.x - player.position.x,
        target.position.y - player.position.y,
    };
    const float distanceSquared = LengthSquared(toTarget);

    if (distanceSquared <= 0.0001f)
    {
        return;
    }

    const float targetDistance = std::sqrt(distanceSquared);
    const float angleDifference = NormalizeRelativeAngle(std::atan2(toTarget.y, toTarget.x) - player.angle);
    const float halfFov = player.fov * 0.5f;

    if (std::fabs(angleDifference) > (halfFov + 0.2f))
    {
        return;
    }

    const float perpendicularDistance = targetDistance * std::cos(angleDifference);

    if (perpendicularDistance <= 0.05f)
    {
        return;
    }

    const float screenOffset = std::tan(angleDifference) / std::tan(halfFov);
    const int screenX = static_cast<int>(((screenOffset + 1.0f) * 0.5f) * static_cast<float>(kScreenWidth));

    if (screenX < 0 || screenX >= kScreenWidth || perpendicularDistance >= depthBuffer[screenX])
    {
        return;
    }

    int spriteSize = static_cast<int>((static_cast<float>(kScreenHeight) * target.size) / perpendicularDistance);
    spriteSize = std::clamp(spriteSize, 18, 280);

    const int centerY = kScreenHeight / 2;
    const float outerRadius = static_cast<float>(spriteSize) * 0.5f;
    const float middleRadius = outerRadius * 0.62f;
    const float innerRadius = outerRadius * 0.24f;

    const Color boardColor = LerpColor(Color{220, 214, 196, 255}, Color{255, 132, 96, 255}, target.hitFlash);
    const Color ringColor = LerpColor(Color{150, 54, 42, 255}, Color{255, 220, 170, 255}, target.hitFlash);

    DrawCircle(screenX, centerY, outerRadius, boardColor);
    DrawCircle(screenX, centerY, middleRadius, ringColor);
    DrawCircle(screenX, centerY, innerRadius, Color{42, 18, 14, 255});
    DrawRectangle(screenX - (spriteSize / 16), centerY + (spriteSize / 2), spriteSize / 8, spriteSize / 2, Color{88, 66, 48, 255});
}

void DrawWeapon(const Player& player, const WeaponState& weapon)
{
    const float speedRatio = std::clamp(std::sqrt(LengthSquared(player.velocity)) / kMoveSpeed, 0.0f, 1.0f);
    const float bobTime = static_cast<float>(GetTime()) * 7.5f;
    const float bobX = std::sin(bobTime) * 12.0f * speedRatio;
    const float bobY = std::fabs(std::cos(bobTime * 0.5f)) * 10.0f * speedRatio;
    const float recoilLift = weapon.recoil * 18.0f;
    const float recoilShift = weapon.recoil * 4.0f;

    const int centerX = (kScreenWidth / 2) + static_cast<int>(bobX + recoilShift);
    const int baseY = kScreenHeight - 8 + static_cast<int>(bobY - recoilLift);

    DrawEllipse(centerX, baseY - 8, 170, 24, Color{0, 0, 0, 70});
    DrawRectangle(centerX - 90, baseY - 108, 180, 88, Color{48, 50, 58, 255});
    DrawRectangle(centerX - 64, baseY - 132, 128, 24, Color{84, 88, 98, 255});
    DrawRectangle(centerX - 34, baseY - 154, 68, 22, Color{148, 146, 136, 255});
    DrawTriangle(
        Vector2{static_cast<float>(centerX - 90), static_cast<float>(baseY - 44)},
        Vector2{static_cast<float>(centerX - 124), static_cast<float>(baseY + 4)},
        Vector2{static_cast<float>(centerX - 66), static_cast<float>(baseY + 4)},
        Color{36, 38, 44, 255});
    DrawTriangle(
        Vector2{static_cast<float>(centerX + 90), static_cast<float>(baseY - 44)},
        Vector2{static_cast<float>(centerX + 124), static_cast<float>(baseY + 4)},
        Vector2{static_cast<float>(centerX + 66), static_cast<float>(baseY + 4)},
        Color{36, 38, 44, 255});
    DrawRectangle(centerX - 52, baseY - 98, 104, 14, Color{128, 96, 70, 255});
    DrawRectangle(centerX - 16, baseY - 126, 32, 92, Color{120, 84, 58, 255});
    DrawRectangle(centerX - 30, baseY - 122, 60, 8, Color{224, 216, 196, 255});

    if (weapon.muzzleFlash > 0.0f)
    {
        const float flashSize = 26.0f * weapon.muzzleFlash;
        const int flashY = baseY - 162;
        const Color flashColor = {255, 214, 128, static_cast<unsigned char>(180.0f * weapon.muzzleFlash)};

        DrawTriangle(
            Vector2{static_cast<float>(centerX - flashSize), static_cast<float>(flashY + (flashSize * 0.35f))},
            Vector2{static_cast<float>(centerX), static_cast<float>(flashY - flashSize)},
            Vector2{static_cast<float>(centerX + flashSize), static_cast<float>(flashY + (flashSize * 0.35f))},
            flashColor);
        DrawCircle(centerX, flashY, flashSize * 0.45f, Color{255, 246, 214, 220});
    }
}

void DrawMiniMap(const Player& player, const Target& target)
{
    constexpr int kMiniMapScale = 18;
    constexpr int kMiniMapPadding = 16;

    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            const Color tileColor = (kMap[y][x] == 0) ? Color{24, 26, 34, 200} : Color{220, 214, 198, 255};
            DrawRectangle(
                kMiniMapPadding + (x * kMiniMapScale),
                kMiniMapPadding + (y * kMiniMapScale),
                kMiniMapScale - 1,
                kMiniMapScale - 1,
                tileColor);
        }
    }

    const int targetX = kMiniMapPadding + static_cast<int>(target.position.x * static_cast<float>(kMiniMapScale));
    const int targetY = kMiniMapPadding + static_cast<int>(target.position.y * static_cast<float>(kMiniMapScale));
    const int playerX = kMiniMapPadding + static_cast<int>(player.position.x * static_cast<float>(kMiniMapScale));
    const int playerY = kMiniMapPadding + static_cast<int>(player.position.y * static_cast<float>(kMiniMapScale));
    const int lookX = playerX + static_cast<int>(std::cos(player.angle) * static_cast<float>(kMiniMapScale));
    const int lookY = playerY + static_cast<int>(std::sin(player.angle) * static_cast<float>(kMiniMapScale));

    DrawCircle(targetX, targetY, 5.0f, LerpColor(Color{232, 198, 92, 255}, Color{255, 120, 92, 255}, target.hitFlash));
    DrawCircle(playerX, playerY, 4.0f, RED);
    DrawLine(playerX, playerY, lookX, lookY, RED);
}

void DrawHud(const Player& player, int hitCount)
{
    constexpr int kHudWidth = 220;
    constexpr int kHudHeight = 86;
    constexpr int kHudPadding = 16;

    const int hudX = kScreenWidth - kHudWidth - kHudPadding;
    const int hudY = kHudPadding;
    const Color panelColor = {10, 12, 18, 185};
    const Color borderColor = {100, 108, 120, 220};
    const Color primaryTextColor = {235, 231, 220, 255};
    const Color secondaryTextColor = {182, 190, 204, 255};

    DrawRectangle(hudX, hudY, kHudWidth, kHudHeight, panelColor);
    DrawRectangleLines(hudX, hudY, kHudWidth, kHudHeight, borderColor);
    DrawText(TextFormat("FPS %03i", GetFPS()), hudX + 12, hudY + 10, 20, primaryTextColor);
    DrawText(TextFormat("POS %.1f %.1f", player.position.x, player.position.y), hudX + 12, hudY + 34, 18, secondaryTextColor);
    DrawText(TextFormat("HITS %02i", hitCount), hudX + 12, hudY + 56, 18, secondaryTextColor);
    DrawText("WASD move | arrows turn | mouse1/space fire", 16, kScreenHeight - 28, 18, Color{220, 220, 220, 190});

    const int crosshairX = kScreenWidth / 2;
    const int crosshairY = kScreenHeight / 2;
    const Color crosshairColor = {240, 236, 220, 180};
    DrawLine(crosshairX - 9, crosshairY, crosshairX - 3, crosshairY, crosshairColor);
    DrawLine(crosshairX + 3, crosshairY, crosshairX + 9, crosshairY, crosshairColor);
    DrawLine(crosshairX, crosshairY - 9, crosshairX, crosshairY - 3, crosshairColor);
    DrawLine(crosshairX, crosshairY + 3, crosshairX, crosshairY + 9, crosshairColor);
}
} // namespace

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(kScreenWidth, kScreenHeight, "doom-like-cpp");
    SetTargetFPS(60);

    Player player{
        kPlayerSpawn,
        Vector2{0.0f, 0.0f},
        0.0f,
        0.0f,
        kPlayerFov,
    };
    WeaponState weapon{
        0.0f,
        0.0f,
        0.0f,
    };
    Target target{
        kTargetSpawn,
        0.8f,
        0.0f,
    };
    std::array<float, kScreenWidth> depthBuffer{};
    int hitCount = 0;

    while (!WindowShouldClose())
    {
        const float frameTime = GetFrameTime();

        UpdatePlayer(player, frameTime);
        UpdateWeapon(weapon, frameTime);
        UpdateTarget(target, frameTime);
        TryFireWeapon(player, weapon, target, hitCount);

        BeginDrawing();
        ClearBackground(BLACK);

        DrawWorld(player, depthBuffer);
        DrawTarget(player, target, depthBuffer);
        DrawWeapon(player, weapon);
        DrawMiniMap(player, target);
        DrawHud(player, hitCount);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
