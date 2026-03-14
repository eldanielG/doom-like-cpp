#include "world/world.h"

#include <array>
#include <cmath>

namespace
{
const std::array<Vector2, world::kPlayerSpawnPointCount> kPlayerSpawnPoints = {{
    {1.5f, 1.5f},
    {5.5f, 1.5f},
    {9.5f, 1.5f},
    {3.5f, 5.5f},
    {1.5f, 9.5f},
    {9.5f, 9.5f},
}};

const std::array<Vector2, world::kTargetSpawnPointCount> kTargetSpawnPoints = {{
    {1.5f, 1.5f},
    {5.5f, 1.5f},
    {9.5f, 1.5f},
    {1.5f, 3.5f},
    {7.5f, 3.5f},
    {3.5f, 5.5f},
    {8.5f, 5.5f},
    {5.5f, 7.5f},
    {1.5f, 9.5f},
    {9.5f, 9.5f},
}};

const world::Map kMap = {{
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

int CellFromWorld(float value)
{
    return static_cast<int>(std::floor(value));
}
} // namespace

namespace world
{
const Map& GetMap()
{
    return kMap;
}

const std::array<Vector2, kPlayerSpawnPointCount>& GetPlayerSpawnPoints()
{
    return kPlayerSpawnPoints;
}

const std::array<Vector2, kTargetSpawnPointCount>& GetTargetSpawnPoints()
{
    return kTargetSpawnPoints;
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

bool IsFarEnoughFromPoint(Vector2 spawnPoint, const Vector2* positionToAvoid, float minimumDistance)
{
    if (positionToAvoid == nullptr || minimumDistance <= 0.0f)
    {
        return true;
    }

    const Vector2 offset = {
        spawnPoint.x - positionToAvoid->x,
        spawnPoint.y - positionToAvoid->y,
    };
    const float distanceSquared = (offset.x * offset.x) + (offset.y * offset.y);
    return distanceSquared >= (minimumDistance * minimumDistance);
}

Vector2 ChoosePlayerSpawnPoint()
{
    std::array<bool, kPlayerSpawnPointCount> blockedPoints{};
    const int spawnIndex = ChooseSpawnIndex(kPlayerSpawnPoints, blockedPoints, nullptr, 0.0f);
    return kPlayerSpawnPoints[spawnIndex];
}
} // namespace world
