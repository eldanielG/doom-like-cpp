#include "core/game.h"
#include "render/renderer.h"
#include "raylib.h"

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(render::kScreenWidth, render::kScreenHeight, "doom-like-cpp");
    SetTargetFPS(60);

    core::GameState game = core::CreateGameState();

    while (!WindowShouldClose())
    {
        core::UpdateGame(game, GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);
        core::DrawGame(game);
        EndDrawing();
    }

    core::DestroyGameState(game);
    CloseWindow();
    return 0;
}
