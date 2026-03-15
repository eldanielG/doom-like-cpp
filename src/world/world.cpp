#include "world/world.h"

#include <array>
#include <cmath>
#include <fstream>
#include <string>

#include "entities/pickup.h"
#include "entities/target.h"

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

const std::array<world::TargetSpawnPoint, world::kTargetSpawnPointCount> kTargetSpawnPoints = {{
    {{1.5f, 1.5f}, entities::TargetType::Scout},
    {{5.5f, 1.5f}, entities::TargetType::Standard},
    {{9.5f, 1.5f}, entities::TargetType::Scout},
    {{1.5f, 3.5f}, entities::TargetType::Standard},
    {{7.5f, 3.5f}, entities::TargetType::Tank},
    {{3.5f, 5.5f}, entities::TargetType::Standard},
    {{8.5f, 5.5f}, entities::TargetType::Scout},
    {{5.5f, 7.5f}, entities::TargetType::Tank},
    {{1.5f, 9.5f}, entities::TargetType::Scout},
    {{9.5f, 9.5f}, entities::TargetType::Standard},
}};

const std::array<world::PickupSpawnPoint, world::kPickupSpawnPointCount> kPickupSpawnPoints = {{
    {{3.5f, 1.5f}, entities::PickupType::HealthPack},
    {{7.5f, 1.5f}, entities::PickupType::RapidFire},
    {{5.5f, 5.5f}, entities::PickupType::ScoreBonus},
    {{3.5f, 9.5f}, entities::PickupType::HealthPack},
    {{7.5f, 9.5f}, entities::PickupType::RapidFire},
}};

constexpr std::array<const char*, 3> kMapFilePaths = {{
    "assets/maps/arena.txt",
    "../assets/maps/arena.txt",
    "../../assets/maps/arena.txt",
}};

const world::Map kFallbackMap = {{
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

bool ParseMapRow(const std::string& line, world::TileRow& row)
{
    if (line.size() != world::kMapWidth)
    {
        return false;
    }

    for (int column = 0; column < world::kMapWidth; ++column)
    {
        const char tile = line[static_cast<std::size_t>(column)];

        if (tile != '0' && tile != '1')
        {
            return false;
        }

        row[static_cast<std::size_t>(column)] = tile - '0';
    }

    return true;
}

world::Map LoadMapFromFile()
{
    for (const char* filePath : kMapFilePaths)
    {
        std::ifstream mapFile(filePath);
        if (!mapFile.is_open())
        {
            continue;
        }

        world::Map loadedMap{};
        std::string line;
        int row = 0;

        while (std::getline(mapFile, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            if (line.empty())
            {
                continue;
            }

            if (row >= world::kMapHeight || !ParseMapRow(line, loadedMap[static_cast<std::size_t>(row)]))
            {
                TraceLog(LOG_WARNING, "Invalid map layout in %s. Falling back to the built-in map.", filePath);
                return kFallbackMap;
            }

            ++row;
        }

        if (row != world::kMapHeight)
        {
            TraceLog(LOG_WARNING, "Incomplete map layout in %s. Falling back to the built-in map.", filePath);
            return kFallbackMap;
        }

        TraceLog(LOG_INFO, "Loaded map layout from %s", filePath);
        return loadedMap;
    }

    TraceLog(LOG_WARNING, "Map file not found. Falling back to the built-in map.");
    return kFallbackMap;
}

const world::Map& GetLoadedMap()
{
    static const world::Map loadedMap = LoadMapFromFile();
    return loadedMap;
}
} // namespace

namespace world
{
const Map& GetMap()
{
    return GetLoadedMap();
}

const std::array<Vector2, kPlayerSpawnPointCount>& GetPlayerSpawnPoints()
{
    return kPlayerSpawnPoints;
}

const std::array<TargetSpawnPoint, kTargetSpawnPointCount>& GetTargetSpawnPoints()
{
    return kTargetSpawnPoints;
}

const std::array<PickupSpawnPoint, kPickupSpawnPointCount>& GetPickupSpawnPoints()
{
    return kPickupSpawnPoints;
}

bool IsWall(int mapX, int mapY)
{
    if (mapX < 0 || mapX >= kMapWidth || mapY < 0 || mapY >= kMapHeight)
    {
        return true;
    }

    return GetMap()[static_cast<std::size_t>(mapY)][static_cast<std::size_t>(mapX)] != 0;
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
