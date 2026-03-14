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
constexpr float kMoveSpeed = 3.0f;
constexpr float kTurnSpeed = 1.8f;

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
    float angle;
    float fov;
};

struct RayHit
{
    float distance;
    bool hitOnVerticalSide;
};

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

int CellFromWorld(float value)
{
    return static_cast<int>(std::floor(value));
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

    const Vector2 nextY = {player.position.x, player.position.y + movementStep.y};
    if (CanMoveTo(nextY))
    {
        player.position.y = nextY.y;
    }
}

void UpdatePlayer(Player& player, float deltaTime)
{
    if (IsKeyDown(KEY_LEFT))
    {
        player.angle -= kTurnSpeed * deltaTime;
    }

    if (IsKeyDown(KEY_RIGHT))
    {
        player.angle += kTurnSpeed * deltaTime;
    }

    player.angle = NormalizeAngle(player.angle);

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
    const Vector2 movementStep = {
        movementDirection.x * kMoveSpeed * deltaTime,
        movementDirection.y * kMoveSpeed * deltaTime,
    };

    MovePlayer(player, movementStep);
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

    return RayHit{distance, hitOnVerticalSide};
}

void DrawWorld(const Player& player)
{
    const Color ceilingColor = {40, 42, 58, 255};
    const Color floorColor = {58, 36, 28, 255};
    const Color wallLightColor = {214, 200, 176, 255};
    const Color wallDarkColor = {166, 148, 126, 255};

    DrawRectangle(0, 0, kScreenWidth, kScreenHeight / 2, ceilingColor);
    DrawRectangle(0, kScreenHeight / 2, kScreenWidth, kScreenHeight / 2, floorColor);

    const Vector2 forward = {std::cos(player.angle), std::sin(player.angle)};
    const float planeScale = std::tan(player.fov * 0.5f);
    const Vector2 cameraPlane = {-forward.y * planeScale, forward.x * planeScale};

    for (int column = 0; column < kScreenWidth; ++column)
    {
        // Camera plane raycasting avoids fish-eye distortion without extra correction.
        const float cameraX = (2.0f * static_cast<float>(column) / static_cast<float>(kScreenWidth)) - 1.0f;
        const Vector2 rayDirection = {
            forward.x + (cameraPlane.x * cameraX),
            forward.y + (cameraPlane.y * cameraX),
        };

        const RayHit hit = CastRay(player.position, rayDirection);
        const int wallHeight = static_cast<int>(static_cast<float>(kScreenHeight) / hit.distance);

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

        const Color wallColor = hit.hitOnVerticalSide ? wallDarkColor : wallLightColor;
        DrawLine(column, wallTop, column, wallBottom, wallColor);
    }
}

void DrawMiniMap(const Player& player)
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

    const int playerX = kMiniMapPadding + static_cast<int>(player.position.x * static_cast<float>(kMiniMapScale));
    const int playerY = kMiniMapPadding + static_cast<int>(player.position.y * static_cast<float>(kMiniMapScale));
    const int lookX = playerX + static_cast<int>(std::cos(player.angle) * static_cast<float>(kMiniMapScale));
    const int lookY = playerY + static_cast<int>(std::sin(player.angle) * static_cast<float>(kMiniMapScale));

    DrawCircle(playerX, playerY, 4.0f, RED);
    DrawLine(playerX, playerY, lookX, lookY, RED);
}
} // namespace

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(kScreenWidth, kScreenHeight, "doom-like-cpp");
    SetTargetFPS(60);

    Player player{
        Vector2{2.5f, 2.5f},
        0.0f,
        70.0f * (kPi / 180.0f),
    };

    while (!WindowShouldClose())
    {
        UpdatePlayer(player, GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);

        DrawWorld(player);
        DrawMiniMap(player);

        DrawText("WASD move | Left/Right turn", 16, kScreenHeight - 32, 20, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
