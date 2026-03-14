#include "core/game.h"

#include "render/renderer.h"
#include "world/world.h"

namespace core
{
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
    game.hitCount = 0;
    game.destroyedCount = 0;
}

void UpdateGame(GameState& game, float deltaTime)
{
    if (IsKeyPressed(KEY_R))
    {
        ResetGame(game);
    }

    entities::UpdatePlayer(game.player, deltaTime);
    entities::UpdateWeapon(game.weapon, deltaTime);

    for (entities::Target& target : game.targets)
    {
        entities::UpdateTarget(target, deltaTime);
    }

    entities::HandleTargetRespawns(game.player, game.targets, deltaTime);

    if (entities::TryFireWeapon(game.weapon) &&
        entities::TryHitTargets(game.player, game.targets, game.hitCount, game.destroyedCount))
    {
        game.weapon.hitMarker = 1.0f;
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
        game.hitCount,
        game.destroyedCount,
        entities::CountAliveTargets(game.targets));
}
} // namespace core
