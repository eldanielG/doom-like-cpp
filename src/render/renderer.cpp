#include "render/renderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

#include "world/world.h"

namespace
{
constexpr float kPi = 3.1415926535f;
constexpr float kShadeStartDistance = 1.0f;
constexpr float kShadeEndDistance = 11.0f;

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

void BreakTime(float timeSeconds, int& minutes, int& seconds, int& tenths)
{
    const float clampedTime = std::max(0.0f, timeSeconds);
    const int totalSeconds = static_cast<int>(clampedTime);
    minutes = totalSeconds / 60;
    seconds = totalSeconds % 60;
    tenths = static_cast<int>(clampedTime * 10.0f) % 10;
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

Color SampleWallColor(const render::RayHit& hit, float wallV)
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

void DrawWallColumn(int column, int wallTop, int wallBottom, const render::RayHit& hit)
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

void DrawTarget(
    const entities::Player& player,
    const entities::Target& target,
    const std::vector<float>& depthBuffer)
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
    const int screenX = static_cast<int>(((screenOffset + 1.0f) * 0.5f) * static_cast<float>(render::kScreenWidth));

    if (screenX < 0 || screenX >= render::kScreenWidth || perpendicularDistance >= depthBuffer[screenX])
    {
        return;
    }

    int spriteSize = static_cast<int>((static_cast<float>(render::kScreenHeight) * target.size) / perpendicularDistance);
    spriteSize = std::clamp(spriteSize, 18, 280);

    const int centerY = render::kScreenHeight / 2;
    const float outerRadius = static_cast<float>(spriteSize) * 0.5f;
    const float middleRadius = outerRadius * 0.62f;
    const float innerRadius = outerRadius * 0.24f;

    if (target.destroyed)
    {
        if (target.destroyFlash <= 0.0f)
        {
            return;
        }

        const float burstSpread = outerRadius * (0.25f + ((1.0f - target.destroyFlash) * 0.9f));
        const unsigned char alpha = static_cast<unsigned char>(220.0f * target.destroyFlash);
        const Color burstColor = {255, 164, 104, alpha};
        const Color emberColor = {255, 222, 178, alpha};

        DrawCircle(screenX, centerY, outerRadius * 0.24f, emberColor);
        DrawCircle(screenX - static_cast<int>(burstSpread), centerY - static_cast<int>(burstSpread * 0.15f), outerRadius * 0.16f, burstColor);
        DrawCircle(screenX + static_cast<int>(burstSpread), centerY, outerRadius * 0.18f, burstColor);
        DrawCircle(screenX, centerY - static_cast<int>(burstSpread * 0.75f), outerRadius * 0.14f, emberColor);
        DrawLine(screenX - static_cast<int>(outerRadius * 0.7f), centerY, screenX + static_cast<int>(outerRadius * 0.7f), centerY, burstColor);
        return;
    }

    const float reactionScale = 1.0f + (0.18f * target.hitReaction);
    const float reactionSquash = 1.0f - (0.12f * target.hitReaction);
    const int liftedCenterY = centerY - static_cast<int>(target.hitReaction * 12.0f);
    const Color respawnBaseColor = LerpColor(Color{168, 214, 255, 255}, Color{220, 214, 196, 255}, 1.0f - target.respawnFlash);
    const Color boardColor = LerpColor(respawnBaseColor, Color{255, 132, 96, 255}, target.hitFlash);
    const Color ringColor = LerpColor(Color{150, 54, 42, 255}, Color{255, 220, 170, 255}, target.hitFlash);

    DrawEllipse(screenX, liftedCenterY, outerRadius * reactionScale, outerRadius * reactionSquash, boardColor);
    DrawEllipse(screenX, liftedCenterY, middleRadius * reactionScale, middleRadius * reactionSquash, ringColor);
    DrawEllipse(screenX, liftedCenterY, innerRadius * reactionScale, innerRadius * reactionSquash, Color{42, 18, 14, 255});
    DrawRectangle(screenX - (spriteSize / 16), liftedCenterY + (spriteSize / 2), spriteSize / 8, spriteSize / 2, Color{88, 66, 48, 255});
    DrawCircleLines(screenX, liftedCenterY, outerRadius * (1.02f + (target.hitFlash * 0.06f)), Color{255, 234, 198, 140});

    if (target.respawnFlash > 0.0f)
    {
        const Color respawnRingColor = {170, 224, 255, static_cast<unsigned char>(170.0f * target.respawnFlash)};
        DrawCircleLines(screenX, liftedCenterY, outerRadius * (1.18f + ((1.0f - target.respawnFlash) * 0.2f)), respawnRingColor);
    }

    if (target.attackFlash > 0.0f)
    {
        const Color attackRingColor = {255, 192, 118, static_cast<unsigned char>(170.0f * target.attackFlash)};
        DrawCircleLines(screenX, liftedCenterY, outerRadius * (1.22f + ((1.0f - target.attackFlash) * 0.08f)), attackRingColor);
    }
}

void DrawProjectile(
    const entities::Player& player,
    const entities::Target& target,
    const std::vector<float>& depthBuffer)
{
    if (!target.projectileActive)
    {
        return;
    }

    const Vector2 toProjectile = {
        target.projectilePosition.x - player.position.x,
        target.projectilePosition.y - player.position.y,
    };
    const float distanceSquared = LengthSquared(toProjectile);

    if (distanceSquared <= 0.0001f)
    {
        return;
    }

    const float projectileDistance = std::sqrt(distanceSquared);
    const float angleDifference = NormalizeRelativeAngle(std::atan2(toProjectile.y, toProjectile.x) - player.angle);
    const float halfFov = player.fov * 0.5f;

    if (std::fabs(angleDifference) > (halfFov + 0.2f))
    {
        return;
    }

    const float perpendicularDistance = projectileDistance * std::cos(angleDifference);

    if (perpendicularDistance <= 0.05f)
    {
        return;
    }

    const float screenOffset = std::tan(angleDifference) / std::tan(halfFov);
    const int screenX = static_cast<int>(((screenOffset + 1.0f) * 0.5f) * static_cast<float>(render::kScreenWidth));

    if (screenX < 0 || screenX >= render::kScreenWidth || perpendicularDistance >= depthBuffer[screenX])
    {
        return;
    }

    const int screenY = render::kScreenHeight / 2;
    const float projectileSize = std::clamp((static_cast<float>(render::kScreenHeight) * 0.12f) / perpendicularDistance, 5.0f, 18.0f);
    DrawCircle(screenX, screenY, projectileSize, Color{255, 188, 96, 220});
    DrawCircle(screenX, screenY, projectileSize * 0.45f, Color{255, 244, 206, 255});
}
} // namespace

namespace render
{
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

        if (world::IsWall(mapX, mapY))
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

void DrawWorld(const entities::Player& player, std::vector<float>& depthBuffer)
{
    if (depthBuffer.size() != kScreenWidth)
    {
        depthBuffer.assign(kScreenWidth, 0.0f);
    }

    const Color ceilingColor = {40, 42, 58, 255};
    const Color floorColor = {58, 36, 28, 255};
    DrawRectangle(0, 0, kScreenWidth, kScreenHeight / 2, ceilingColor);
    DrawRectangle(0, kScreenHeight / 2, kScreenWidth, kScreenHeight / 2, floorColor);

    const Vector2 forward = {std::cos(player.angle), std::sin(player.angle)};
    const float planeScale = std::tan(player.fov * 0.5f);
    const Vector2 cameraPlane = {-forward.y * planeScale, forward.x * planeScale};

    for (int column = 0; column < kScreenWidth; ++column)
    {
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

void DrawTargets(
    const entities::Player& player,
    const std::array<entities::Target, entities::kTargetCount>& targets,
    const std::vector<float>& depthBuffer)
{
    std::array<int, entities::kTargetCount> targetOrder{};

    for (int index = 0; index < entities::kTargetCount; ++index)
    {
        targetOrder[index] = index;
    }

    std::sort(targetOrder.begin(), targetOrder.end(), [&player, &targets](int left, int right) {
        const Vector2 toLeft = {
            targets[left].position.x - player.position.x,
            targets[left].position.y - player.position.y,
        };
        const Vector2 toRight = {
            targets[right].position.x - player.position.x,
            targets[right].position.y - player.position.y,
        };

        return LengthSquared(toLeft) > LengthSquared(toRight);
    });

    for (int index : targetOrder)
    {
        DrawTarget(player, targets[index], depthBuffer);
    }

    std::sort(targetOrder.begin(), targetOrder.end(), [&player, &targets](int left, int right) {
        const Vector2 toLeft = {
            targets[left].projectilePosition.x - player.position.x,
            targets[left].projectilePosition.y - player.position.y,
        };
        const Vector2 toRight = {
            targets[right].projectilePosition.x - player.position.x,
            targets[right].projectilePosition.y - player.position.y,
        };

        return LengthSquared(toLeft) > LengthSquared(toRight);
    });

    for (int index : targetOrder)
    {
        DrawProjectile(player, targets[index], depthBuffer);
    }
}

void DrawWeapon(const entities::Player& player, const entities::WeaponState& weapon)
{
    const float speedRatio = std::clamp(std::sqrt(LengthSquared(player.velocity)) / entities::kMoveSpeed, 0.0f, 1.0f);
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

void DrawMiniMap(const entities::Player& player, const std::array<entities::Target, entities::kTargetCount>& targets)
{
    constexpr int kMiniMapScale = 18;
    constexpr int kMiniMapPadding = 16;

    const world::Map& map = world::GetMap();

    for (int y = 0; y < world::kMapHeight; ++y)
    {
        for (int x = 0; x < world::kMapWidth; ++x)
        {
            const Color tileColor = (map[y][x] == 0) ? Color{24, 26, 34, 200} : Color{220, 214, 198, 255};
            DrawRectangle(
                kMiniMapPadding + (x * kMiniMapScale),
                kMiniMapPadding + (y * kMiniMapScale),
                kMiniMapScale - 1,
                kMiniMapScale - 1,
                tileColor);
        }
    }

    const int playerX = kMiniMapPadding + static_cast<int>(player.position.x * static_cast<float>(kMiniMapScale));
    const int playerY = kMiniMapPadding + static_cast<int>(player.position.y * static_cast<float>(kMiniMapScale));
    const int lookX = playerX + static_cast<int>(std::cos(player.angle) * static_cast<float>(kMiniMapScale));
    const int lookY = playerY + static_cast<int>(std::sin(player.angle) * static_cast<float>(kMiniMapScale));

    for (const entities::Target& target : targets)
    {
        const int targetX = kMiniMapPadding + static_cast<int>(target.position.x * static_cast<float>(kMiniMapScale));
        const int targetY = kMiniMapPadding + static_cast<int>(target.position.y * static_cast<float>(kMiniMapScale));

        if (target.destroyed)
        {
            DrawLine(targetX - 4, targetY - 4, targetX + 4, targetY + 4, Color{110, 110, 110, 220});
            DrawLine(targetX - 4, targetY + 4, targetX + 4, targetY - 4, Color{110, 110, 110, 220});
            continue;
        }

        const Color targetColor = LerpColor(
            LerpColor(Color{168, 214, 255, 255}, Color{232, 198, 92, 255}, 1.0f - target.respawnFlash),
            Color{255, 120, 92, 255},
            target.hitFlash);
        DrawCircle(targetX, targetY, 5.0f, targetColor);
    }

    DrawCircle(playerX, playerY, 4.0f, RED);
    DrawLine(playerX, playerY, lookX, lookY, RED);
}

void DrawHud(
    const entities::Player& player,
    const entities::WeaponState& weapon,
    int difficultyLevel,
    int score,
    int bestScore,
    int hitCount,
    int destroyedCount,
    int aliveCount,
    float survivalTime,
    float bestSurvivalTime,
    bool isGameOver)
{
    constexpr int kHudWidth = 248;
    constexpr int kHudHeight = 200;
    constexpr int kHudPadding = 16;

    const int hudX = kScreenWidth - kHudWidth - kHudPadding;
    const int hudY = kHudPadding;
    const Color panelColor = {10, 12, 18, 185};
    const Color borderColor = {100, 108, 120, 220};
    const Color primaryTextColor = {235, 231, 220, 255};
    const Color secondaryTextColor = {182, 190, 204, 255};
    int currentMinutes = 0;
    int currentSeconds = 0;
    int currentTenths = 0;
    int bestMinutes = 0;
    int bestSeconds = 0;
    int bestTenths = 0;

    BreakTime(survivalTime, currentMinutes, currentSeconds, currentTenths);
    BreakTime(bestSurvivalTime, bestMinutes, bestSeconds, bestTenths);

    DrawRectangle(hudX, hudY, kHudWidth, kHudHeight, panelColor);
    DrawRectangleLines(hudX, hudY, kHudWidth, kHudHeight, borderColor);
    DrawText(TextFormat("FPS %03i", GetFPS()), hudX + 12, hudY + 10, 20, primaryTextColor);
    DrawText(TextFormat("POS %.1f %.1f", player.position.x, player.position.y), hudX + 12, hudY + 34, 18, secondaryTextColor);

    const float healthRatio = std::clamp(static_cast<float>(player.health) / static_cast<float>(entities::kPlayerMaxHealth), 0.0f, 1.0f);
    const Color healthColor = LerpColor(Color{196, 62, 62, 255}, Color{114, 214, 132, 255}, healthRatio);
    DrawText(TextFormat("HP %03i / %03i", player.health, entities::kPlayerMaxHealth), hudX + 12, hudY + 56, 18, primaryTextColor);
    DrawRectangle(hudX + 12, hudY + 80, kHudWidth - 24, 10, Color{34, 18, 20, 255});
    DrawRectangle(hudX + 13, hudY + 81, static_cast<int>(static_cast<float>(kHudWidth - 26) * healthRatio), 8, healthColor);
    DrawText(TextFormat("SCORE %04i | BEST %04i", score, bestScore), hudX + 12, hudY + 96, 18, secondaryTextColor);
    DrawText(TextFormat("TIME %02i:%02i.%01i", currentMinutes, currentSeconds, currentTenths), hudX + 12, hudY + 116, 18, secondaryTextColor);
    DrawText(TextFormat("BEST %02i:%02i.%01i", bestMinutes, bestSeconds, bestTenths), hudX + 12, hudY + 136, 18, secondaryTextColor);
    DrawText(TextFormat("INTENSITY LVL %02i", difficultyLevel), hudX + 12, hudY + 156, 18, secondaryTextColor);
    DrawText(TextFormat("HITS %02i | ELIMS %02i | ALIVE %02i", hitCount, destroyedCount, aliveCount), hudX + 12, hudY + 176, 18, secondaryTextColor);

    if (player.damageFlash > 0.0f)
    {
        const Color damageOverlay = {120, 18, 18, static_cast<unsigned char>(70.0f * player.damageFlash)};
        DrawRectangle(0, 0, kScreenWidth, kScreenHeight, damageOverlay);
    }

    const char* controlsText = isGameOver ?
        "Game over | R restart" :
        "WASD move | arrows turn | mouse1/space fire | R reset";
    DrawText(controlsText, 16, kScreenHeight - 28, 18, Color{220, 220, 220, 190});

    const int crosshairX = kScreenWidth / 2;
    const int crosshairY = kScreenHeight / 2;
    const Color crosshairColor = LerpColor(Color{240, 236, 220, 180}, Color{255, 158, 120, 255}, weapon.hitMarker);
    DrawLine(crosshairX - 9, crosshairY, crosshairX - 3, crosshairY, crosshairColor);
    DrawLine(crosshairX + 3, crosshairY, crosshairX + 9, crosshairY, crosshairColor);
    DrawLine(crosshairX, crosshairY - 9, crosshairX, crosshairY - 3, crosshairColor);
    DrawLine(crosshairX, crosshairY + 3, crosshairX, crosshairY + 9, crosshairColor);

    if (weapon.hitMarker > 0.0f)
    {
        const int markerSpread = 16 + static_cast<int>((1.0f - weapon.hitMarker) * 6.0f);
        DrawLine(crosshairX - markerSpread, crosshairY - markerSpread, crosshairX - 6, crosshairY - 6, crosshairColor);
        DrawLine(crosshairX + markerSpread, crosshairY - markerSpread, crosshairX + 6, crosshairY - 6, crosshairColor);
        DrawLine(crosshairX - markerSpread, crosshairY + markerSpread, crosshairX - 6, crosshairY + 6, crosshairColor);
        DrawLine(crosshairX + markerSpread, crosshairY + markerSpread, crosshairX + 6, crosshairY + 6, crosshairColor);
    }

    if (isGameOver)
    {
        constexpr int kGameOverWidth = 390;
        constexpr int kGameOverHeight = 158;
        const int boxX = (kScreenWidth - kGameOverWidth) / 2;
        const int boxY = (kScreenHeight - kGameOverHeight) / 2;

        DrawRectangle(boxX, boxY, kGameOverWidth, kGameOverHeight, Color{12, 8, 10, 220});
        DrawRectangleLines(boxX, boxY, kGameOverWidth, kGameOverHeight, Color{160, 88, 88, 230});
        DrawText("GAME OVER", boxX + 96, boxY + 18, 34, Color{240, 220, 220, 255});
        DrawText(TextFormat("Score %04i | Best %04i", score, bestScore), boxX + 46, boxY + 66, 22, Color{220, 214, 206, 255});
        DrawText(TextFormat("Time %02i:%02i.%01i | Best %02i:%02i.%01i",
            currentMinutes,
            currentSeconds,
            currentTenths,
            bestMinutes,
            bestSeconds,
            bestTenths), boxX + 24, boxY + 96, 22, Color{220, 214, 206, 255});
        DrawText("Pressione R para reiniciar", boxX + 50, boxY + 126, 22, Color{220, 214, 206, 255});
    }
}
} // namespace render
