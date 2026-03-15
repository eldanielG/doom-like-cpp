#include "core/game.h"

#include <algorithm>

#include "core/tuning.h"
#include "render/renderer.h"
#include "world/world.h"

namespace core
{
namespace
{
constexpr int kPauseOptionCount = 4;

enum PauseMenuOption
{
    PauseOptionResume = 0,
    PauseOptionRestart,
    PauseOptionQuitToTitle,
    PauseOptionQuitGame,
};

bool IsGameplayPauseKeyPressed()
{
    return IsKeyPressed(KEY_P);
}

bool IsMenuBackKeyPressed()
{
    return IsKeyPressed(KEY_ESCAPE);
}

bool IsTitleStartKeyPressed()
{
    return GetKeyPressed() != 0;
}

bool IsRestartKeyPressed()
{
    return IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
}

void ResumeGameplay(GameState& game)
{
    game.phase = GamePhase::Gameplay;
    game.pauseMode = PauseMode::Quick;
    game.pauseMenuSelection = PauseOptionResume;
}

void OpenQuickPause(GameState& game)
{
    game.phase = GamePhase::Pause;
    game.pauseMode = PauseMode::Quick;
    game.pauseMenuSelection = PauseOptionResume;
}

void OpenPauseMenu(GameState& game)
{
    game.phase = GamePhase::Pause;
    game.pauseMode = PauseMode::Menu;
    game.pauseMenuSelection = PauseOptionResume;
}

void ReturnToTitle(GameState& game)
{
    game.phase = GamePhase::Title;
    game.pauseMode = PauseMode::Quick;
    game.pauseMenuSelection = PauseOptionResume;
}

void StartGameplay(GameState& game)
{
    ResetGame(game);
}

void ActivatePauseMenuSelection(GameState& game)
{
    switch (game.pauseMenuSelection)
    {
    case PauseOptionResume:
        ResumeGameplay(game);
        break;
    case PauseOptionRestart:
        StartGameplay(game);
        break;
    case PauseOptionQuitToTitle:
        ReturnToTitle(game);
        break;
    case PauseOptionQuitGame:
        game.shouldQuit = true;
        break;
    default:
        break;
    }
}
} // namespace

GameState CreateGameState()
{
    GameState game{};
    InitializeAudio(game.audio);
    game.depthBuffer.assign(render::kScreenWidth, 0.0f);
    ResetGame(game);
    game.phase = GamePhase::Title;
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
    game.pauseMode = PauseMode::Quick;
    game.pauseMenuSelection = PauseOptionResume;
    game.phase = GamePhase::Gameplay;
}

void UpdateGame(GameState& game, float deltaTime)
{
    if (game.phase == GamePhase::Title)
    {
        if (IsMenuBackKeyPressed())
        {
            game.shouldQuit = true;
        }
        else if (IsTitleStartKeyPressed())
        {
            StartGameplay(game);
        }

        return;
    }

    if (game.phase == GamePhase::Pause)
    {
        if (game.pauseMode == PauseMode::Quick)
        {
            if (IsGameplayPauseKeyPressed())
            {
                ResumeGameplay(game);
            }
            else if (IsMenuBackKeyPressed())
            {
                OpenPauseMenu(game);
            }
        }
        else
        {
            if (IsGameplayPauseKeyPressed() || IsMenuBackKeyPressed())
            {
                ResumeGameplay(game);
            }
            else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
            {
                game.pauseMenuSelection = (game.pauseMenuSelection + kPauseOptionCount - 1) % kPauseOptionCount;
            }
            else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
            {
                game.pauseMenuSelection = (game.pauseMenuSelection + 1) % kPauseOptionCount;
            }
            else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            {
                ActivatePauseMenuSelection(game);
            }
        }

        return;
    }

    if (game.phase == GamePhase::GameOver)
    {
        if (IsRestartKeyPressed())
        {
            StartGameplay(game);
        }
        else if (IsMenuBackKeyPressed())
        {
            ReturnToTitle(game);
        }

        return;
    }

    if (IsGameplayPauseKeyPressed())
    {
        OpenQuickPause(game);
        return;
    }

    if (IsMenuBackKeyPressed())
    {
        OpenPauseMenu(game);
        return;
    }

    if (IsKeyPressed(KEY_R))
    {
        ResetGame(game);
    }

    game.survivalTime += deltaTime;
    game.bestSurvivalTime = std::max(game.bestSurvivalTime, game.survivalTime);
    game.difficultyLevel = tuning::CalculateDifficultyLevel(game.survivalTime);

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
        entities::AddScreenShake(game.player, tuning::kWeapon.fireScreenShake);
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
        game.weapon.hitMarker = tuning::kWeapon.hitMarkerStrength;

        const int eliminatedThisShot = game.destroyedCount - destroyedBeforeShot;
        if (eliminatedThisShot > 0)
        {
            game.score += eliminatedThisShot * tuning::kScoring.scorePerElimination;
            game.bestScore = std::max(game.bestScore, game.score);
        }
    }

    entities::TryDamagePlayerFromTargets(game.player, game.targets);

    if (game.player.health < playerHealthBeforeDamage)
    {
        PlayPlayerHurtSound(game.audio);
    }

    if (game.player.health == 0)
    {
        game.phase = GamePhase::GameOver;
        PlayGameOverSound(game.audio);
    }
}

void DrawGame(GameState& game)
{
    const Camera2D gameplayCamera = render::BuildGameplayCamera(game.player);
    BeginMode2D(gameplayCamera);
    render::DrawWorld(game.player, game.depthBuffer);
    render::DrawTargets(game.player, game.targets, game.depthBuffer);
    if (game.phase != GamePhase::Title)
    {
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
            game.phase == GamePhase::GameOver);
    }
    EndMode2D();

    if (game.phase == GamePhase::Title)
    {
        render::DrawTitleOverlay();
    }
    else if (game.phase == GamePhase::Pause)
    {
        render::DrawPauseOverlay(game.pauseMode, game.pauseMenuSelection);
    }
}
} // namespace core
