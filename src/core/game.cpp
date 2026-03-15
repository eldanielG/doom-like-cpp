#include "core/game.h"

#include <algorithm>

#include "render/renderer.h"
#include "world/world.h"

namespace core
{
namespace
{
constexpr int kScorePerElimination = 100;
constexpr float kDifficultyStepInterval = 20.0f;
constexpr int kMaxDifficultyLevel = 6;

int CalculateDifficultyLevel(float survivalTime)
{
    const int difficultySteps = static_cast<int>(survivalTime / kDifficultyStepInterval);
    return std::min(1 + difficultySteps, kMaxDifficultyLevel);
}
}

GameState CreateGameState()
{
    GameState game{};
    InitializeAudio(game.audio);
    game.depthBuffer.assign(render::kScreenWidth, 0.0f);
    ResetGame(game);
    return game;
}

void DestroyGameState(GameState& game)
{
    ShutdownAudio(game.audio);
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
    game.difficultyLevel = 1;
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
    game.difficultyLevel = CalculateDifficultyLevel(game.survivalTime);

    const int playerHealthBeforeDamage = game.player.health;
    entities::UpdatePlayer(game.player, deltaTime);
    entities::UpdateWeapon(game.weapon, deltaTime);

    for (entities::Target& target : game.targets)
    {
        entities::UpdateTarget(target, game.player, deltaTime, game.difficultyLevel);
    }

    entities::HandleTargetRespawns(game.player, game.targets, deltaTime);

    const bool firedShot = entities::TryFireWeapon(game.weapon);
    if (firedShot)
    {
        PlayShootSound(game.audio);
    }

    const int destroyedBeforeShot = game.destroyedCount;
    if (firedShot &&
        entities::TryHitTargets(
            game.player,
            game.targets,
            game.hitCount,
            game.destroyedCount,
            game.difficultyLevel))
    {
        PlayHitSound(game.audio);
        game.weapon.hitMarker = 1.0f;

        const int eliminatedThisShot = game.destroyedCount - destroyedBeforeShot;
        if (eliminatedThisShot > 0)
        {
            game.score += eliminatedThisShot * kScorePerElimination;
            game.bestScore = std::max(game.bestScore, game.score);
        }
    }

    entities::TryDamagePlayerFromTargets(game.player, game.targets);

    if (game.player.health < playerHealthBeforeDamage)
    {
        PlayPlayerHurtSound(game.audio);
    }

    if (!game.isGameOver && game.player.health == 0)
    {
        game.isGameOver = true;
        PlayGameOverSound(game.audio);
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
        game.difficultyLevel,
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
