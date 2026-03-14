#pragma once

#include <array>
#include <cstddef>

#include "raylib.h"

namespace world
{
constexpr int kMapWidth = 12;
constexpr int kMapHeight = 12;
constexpr float kPlayerRadius = 0.18f;
constexpr int kPlayerSpawnPointCount = 6;
constexpr int kTargetSpawnPointCount = 10;

using TileRow = std::array<int, kMapWidth>;
using Map = std::array<TileRow, kMapHeight>;

const Map& GetMap();
const std::array<Vector2, kPlayerSpawnPointCount>& GetPlayerSpawnPoints();
const std::array<Vector2, kTargetSpawnPointCount>& GetTargetSpawnPoints();

bool IsWall(int mapX, int mapY);
bool CanMoveTo(Vector2 position);
bool IsFarEnoughFromPoint(Vector2 spawnPoint, const Vector2* positionToAvoid, float minimumDistance);
Vector2 ChoosePlayerSpawnPoint();

template <std::size_t N>
int ChooseSpawnIndex(
    const std::array<Vector2, N>& spawnPoints,
    const std::array<bool, N>& blockedPoints,
    const Vector2* positionToAvoid,
    float minimumDistance)
{
    std::array<int, N> candidates{};
    int candidateCount = 0;

    auto collectCandidates = [&](bool ignoreDistance) {
        candidateCount = 0;

        for (int index = 0; index < static_cast<int>(N); ++index)
        {
            const Vector2 spawnPoint = spawnPoints[index];

            if (blockedPoints[index] || !CanMoveTo(spawnPoint))
            {
                continue;
            }

            if (!ignoreDistance && !IsFarEnoughFromPoint(spawnPoint, positionToAvoid, minimumDistance))
            {
                continue;
            }

            candidates[candidateCount++] = index;
        }
    };

    collectCandidates(false);

    if (candidateCount == 0)
    {
        collectCandidates(true);
    }

    if (candidateCount == 0)
    {
        return 0;
    }

    return candidates[GetRandomValue(0, candidateCount - 1)];
}
} // namespace world
