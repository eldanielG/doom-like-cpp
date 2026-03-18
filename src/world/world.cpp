#include "world/world.h"

#include <array>
#include <cmath>
#include <fstream>
#include <string>

#include "entities/pickup.h"
#include "entities/target.h"

namespace
{
using FilePathCandidates = std::array<const char*, 3>;

const std::array<Vector2, world::kPlayerSpawnPointCount> kArenaPlayerSpawns = {{
    {1.5f, 1.5f},
    {5.5f, 1.5f},
    {9.5f, 1.5f},
    {3.5f, 5.5f},
    {1.5f, 9.5f},
    {9.5f, 9.5f},
}};

const std::array<world::TargetSpawnPoint, world::kTargetSpawnPointCount> kArenaTargetSpawns = {{
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

const std::array<world::PickupSpawnPoint, world::kPickupSpawnPointCount> kArenaPickupSpawns = {{
    {{3.5f, 1.5f}, entities::PickupType::HealthPack},
    {{7.5f, 1.5f}, entities::PickupType::RapidFire},
    {{5.5f, 5.5f}, entities::PickupType::ScoreBonus},
    {{3.5f, 9.5f}, entities::PickupType::HealthPack},
    {{7.5f, 9.5f}, entities::PickupType::RapidFire},
}};

const std::array<Vector2, world::kPlayerSpawnPointCount> kCorridorsPlayerSpawns = {{
    {1.5f, 1.5f},
    {5.5f, 1.5f},
    {8.5f, 1.5f},
    {1.5f, 6.5f},
    {10.5f, 8.5f},
    {6.5f, 10.5f},
}};

const std::array<world::TargetSpawnPoint, world::kTargetSpawnPointCount> kCorridorsTargetSpawns = {{
    {{3.5f, 1.5f}, entities::TargetType::Scout},
    {{10.5f, 1.5f}, entities::TargetType::Standard},
    {{1.5f, 3.5f}, entities::TargetType::Standard},
    {{7.5f, 3.5f}, entities::TargetType::Scout},
    {{4.5f, 4.5f}, entities::TargetType::Tank},
    {{8.5f, 5.5f}, entities::TargetType::Standard},
    {{3.5f, 6.5f}, entities::TargetType::Scout},
    {{9.5f, 7.5f}, entities::TargetType::Tank},
    {{4.5f, 8.5f}, entities::TargetType::Standard},
    {{9.5f, 10.5f}, entities::TargetType::Scout},
}};

const std::array<world::PickupSpawnPoint, world::kPickupSpawnPointCount> kCorridorsPickupSpawns = {{
    {{5.5f, 1.5f}, entities::PickupType::HealthPack},
    {{5.5f, 3.5f}, entities::PickupType::RapidFire},
    {{7.5f, 6.5f}, entities::PickupType::ScoreBonus},
    {{2.5f, 8.5f}, entities::PickupType::HealthPack},
    {{6.5f, 10.5f}, entities::PickupType::RapidFire},
}};

const FilePathCandidates kArenaMapFilePaths = {{
    "assets/maps/arena.txt",
    "../assets/maps/arena.txt",
    "../../assets/maps/arena.txt",
}};

const FilePathCandidates kCorridorsMapFilePaths = {{
    "assets/maps/corridors.txt",
    "../assets/maps/corridors.txt",
    "../../assets/maps/corridors.txt",
}};

const world::Map kArenaFallbackMap = {{
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

const world::Map kCorridorsFallbackMap = {{
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
    {{1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1}},
    {{1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1}},
    {{1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1}},
    {{1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1}},
    {{1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1}},
    {{1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1}},
    {{1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1}},
    {{1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1}},
    {{1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1}},
    {{1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1}},
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

world::Map LoadMapFromFile(const FilePathCandidates& filePaths, const world::Map& fallbackMap, const char* levelId)
{
    for (const char* filePath : filePaths)
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
                TraceLog(LOG_WARNING, "Invalid map layout for level %s in %s. Falling back to built-in layout.", levelId, filePath);
                return fallbackMap;
            }

            ++row;
        }

        if (row != world::kMapHeight)
        {
            TraceLog(LOG_WARNING, "Incomplete map layout for level %s in %s. Falling back to built-in layout.", levelId, filePath);
            return fallbackMap;
        }

        TraceLog(LOG_INFO, "Loaded level %s layout from %s", levelId, filePath);
        return loadedMap;
    }

    TraceLog(LOG_WARNING, "Map file for level %s not found. Falling back to built-in layout.", levelId);
    return fallbackMap;
}

world::LevelDefinition BuildArenaLevel()
{
    return world::LevelDefinition{
        "arena",
        "Arena",
        LoadMapFromFile(kArenaMapFilePaths, kArenaFallbackMap, "arena"),
        kArenaPlayerSpawns,
        kArenaTargetSpawns,
        kArenaPickupSpawns,
    };
}

world::LevelDefinition BuildCorridorsLevel()
{
    return world::LevelDefinition{
        "corridors",
        "Corridors",
        LoadMapFromFile(kCorridorsMapFilePaths, kCorridorsFallbackMap, "corridors"),
        kCorridorsPlayerSpawns,
        kCorridorsTargetSpawns,
        kCorridorsPickupSpawns,
    };
}

std::array<world::LevelDefinition, world::kLevelCount> BuildLevelCatalog()
{
    return {{
        BuildArenaLevel(),
        BuildCorridorsLevel(),
    }};
}

std::array<world::LevelDefinition, world::kLevelCount>& GetLevelCatalog()
{
    static std::array<world::LevelDefinition, world::kLevelCount> levelCatalog = BuildLevelCatalog();
    return levelCatalog;
}

int WrapLevelIndex(int levelIndex)
{
    const int levelCount = world::kLevelCount;
    int wrappedIndex = levelIndex % levelCount;

    if (wrappedIndex < 0)
    {
        wrappedIndex += levelCount;
    }

    return wrappedIndex;
}

int& GetCurrentLevelIndexStorage()
{
    static int currentLevelIndex = 0;
    return currentLevelIndex;
}
} // namespace

namespace world
{
int GetLevelCount()
{
    return kLevelCount;
}

int GetCurrentLevelIndex()
{
    return GetCurrentLevelIndexStorage();
}

const char* GetCurrentLevelDisplayName()
{
    return GetCurrentLevel().displayName;
}

const LevelDefinition& GetCurrentLevel()
{
    return GetLevelCatalog()[static_cast<std::size_t>(GetCurrentLevelIndexStorage())];
}

void SetCurrentLevel(int levelIndex)
{
    GetCurrentLevelIndexStorage() = WrapLevelIndex(levelIndex);
}

void CycleCurrentLevel(int direction)
{
    if (direction == 0)
    {
        return;
    }

    SetCurrentLevel(GetCurrentLevelIndexStorage() + direction);
}

const Map& GetMap()
{
    return GetCurrentLevel().map;
}

const std::array<Vector2, kPlayerSpawnPointCount>& GetPlayerSpawnPoints()
{
    return GetCurrentLevel().playerSpawns;
}

const std::array<TargetSpawnPoint, kTargetSpawnPointCount>& GetTargetSpawnPoints()
{
    return GetCurrentLevel().targetSpawns;
}

const std::array<PickupSpawnPoint, kPickupSpawnPointCount>& GetPickupSpawnPoints()
{
    return GetCurrentLevel().pickupSpawns;
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
    const int spawnIndex = ChooseSpawnIndex(GetPlayerSpawnPoints(), blockedPoints, nullptr, 0.0f);
    return GetPlayerSpawnPoints()[static_cast<std::size_t>(spawnIndex)];
}
} // namespace world
