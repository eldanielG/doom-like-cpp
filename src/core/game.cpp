#include "core/game.h"

#include <algorithm>

#include "render/renderer.h"
#include "world/world.h"

namespace core
{
namespace
{
constexpr int kScorePerElimination = 100;
}

GameState CreateGameState()
{
    GameState game{};
    game.depthBuffer.assign(render::kScreenWidth, 0.0f);
    ResetGame(game);
    return game;
}

void ResetGame(GameState& game)
{
    if (game.depthBuffer.size() != render::kScreenWidth)
    {
        game.depthBuffer.assign(render::kScreenWidth, 0.0f);
    }

    game.player = entities::MakePlayer(world::ChoosePlayerSpawnPoint());
    game.weapon = entities::MakeWeaponState();
    game.targets = entities::MakeTargets(game.player);
    game.score = 0;
    game.hitCount = 0;
    game.destroyedCount = 0;
    game.survivalTime = 0.0f;
    game.isGameOver = false;
}

void UpdateGame(GameState& game, float deltaTime)
{
    if (IsKeyPressed(KEY_R))
    {
        ResetGame(game);
    }

    if (game.isGameOver)
    {
        return;
    }

    game.survivalTime += deltaTime;
    game.bestSurvivalTime = std::max(game.bestSurvivalTime, game.survivalTime);

    entities::UpdatePlayer(game.player, deltaTime);
    entities::UpdateWeapon(game.weapon, deltaTime);

    for (entities::Target& target : game.targets)
    {
        entities::UpdateTarget(target, deltaTime);
    }

    entities::HandleTargetRespawns(game.player, game.targets, deltaTime);

    const int destroyedBeforeShot = game.destroyedCount;
    if (entities::TryFireWeapon(game.weapon) &&
        entities::TryHitTargets(game.player, game.targets, game.hitCount, game.destroyedCount))
    {
        game.weapon.hitMarker = 1.0f;

        const int eliminatedThisShot = game.destroyedCount - destroyedBeforeShot;
        if (eliminatedThisShot > 0)
        {
            game.score += eliminatedThisShot * kScorePerElimination;
            game.bestScore = std::max(game.bestScore, game.score);
        }
    }

    if (entities::TryDamagePlayerFromTargets(game.player, game.targets) && game.player.health == 0)
    {
        game.isGameOver = true;
    }
}

void DrawGame(GameState& game)
{
    render::DrawWorld(game.player, game.depthBuffer);
    render::DrawTargets(game.player, game.targets, game.depthBuffer);
    render::DrawWeapon(game.player, game.weapon);
    render::DrawMiniMap(game.player, game.targets);
    render::DrawHud(
        game.player,
        game.weapon,
        game.score,
        game.bestScore,
        game.hitCount,
        game.destroyedCount,
        entities::CountAliveTargets(game.targets),
        game.survivalTime,
        game.bestSurvivalTime,
        game.isGameOver);
}
} // namespace core
