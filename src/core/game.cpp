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

bool IsTitlePreviousLevelKeyPressed()
{
    return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
}

bool IsTitleNextLevelKeyPressed()
{
    return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
}

bool IsTitleStartKeyPressed()
{
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool IsRestartKeyPressed()
{
    return IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
}

void SyncCursorVisibility(GameState& game)
{
    const bool shouldHideCursor = game.phase == GamePhase::Gameplay;

    if (shouldHideCursor == game.gameplayCursorHidden)
    {
        return;
    }

    if (shouldHideCursor)
    {
        HideCursor();
    }
    else
    {
        ShowCursor();
    }

    game.gameplayCursorHidden = shouldHideCursor;
}

void ResumeGameplay(GameState& game)
{
    game.phase = GamePhase::Gameplay;
    game.pauseMode = PauseMode::Quick;
    game.pauseMenuSelection = PauseOptionResume;
    SyncCursorVisibility(game);
}

void OpenQuickPause(GameState& game)
{
    game.phase = GamePhase::Pause;
    game.pauseMode = PauseMode::Quick;
    game.pauseMenuSelection = PauseOptionResume;
    SyncCursorVisibility(game);
}

void OpenPauseMenu(GameState& game)
{
    game.phase = GamePhase::Pause;
    game.pauseMode = PauseMode::Menu;
    game.pauseMenuSelection = PauseOptionResume;
    SyncCursorVisibility(game);
}

void ReturnToTitle(GameState& game)
{
    game.phase = GamePhase::Title;
    game.pauseMode = PauseMode::Quick;
    game.pauseMenuSelection = PauseOptionResume;
    SyncCursorVisibility(game);
}

void StartGameplay(GameState& game)
{
    ResetGame(game);
}

void AdvanceScoreStage(GameState& game)
{
    ++game.currentScoreStage;
    game.stageAdvanceTimer = tuning::kRun.stageAdvanceFeedbackDuration;

    if (game.currentScoreStage > tuning::kRun.totalScoreStages)
    {
        game.phase = GamePhase::Victory;
        SyncCursorVisibility(game);
        return;
    }

    game.difficultyLevel = std::max(game.difficultyLevel, tuning::GetStageDifficultyFloor(game.currentScoreStage));
}

void TryAdvanceProgression(GameState& game)
{
    while (game.phase == GamePhase::Gameplay &&
           game.currentScoreStage <= tuning::kRun.totalScoreStages &&
           game.score >= tuning::GetScoreStageTarget(game.currentScoreStage))
    {
        AdvanceScoreStage(game);
    }
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
    SyncCursorVisibility(game);
    return game;
}

void DestroyGameState(GameState& game)
{
    if (game.gameplayCursorHidden)
    {
        ShowCursor();
        game.gameplayCursorHidden = false;
    }

    ShutdownAudio(game.audio);
}

void ResetGame(GameState& game)
{
    if (game.depthBuffer.size() != render::kScreenWidth)
    {
        game.depthBuffer.assign(render::kScreenWidth, 0.0f);
    }

    game.pickups = entities::MakePickups();
    game.player = entities::MakePlayer(world::ChoosePlayerSpawnPoint());
    game.weapon = entities::MakeWeaponState();
    game.targets = entities::MakeTargets(game.player);
    game.difficultyLevel = 1;
    game.score = 0;
    game.currentScoreStage = 1;
    game.hitCount = 0;
    game.destroyedCount = 0;
    game.survivalTime = 0.0f;
    game.pickupMessage = nullptr;
    game.pickupMessageTimer = 0.0f;
    game.stageAdvanceTimer = tuning::kRun.stageAdvanceFeedbackDuration;
    game.pauseMode = PauseMode::Quick;
    game.pauseMenuSelection = PauseOptionResume;
    game.phase = GamePhase::Gameplay;
    game.difficultyLevel = tuning::GetStageDifficultyFloor(game.currentScoreStage);
    SyncCursorVisibility(game);
}

void UpdateGame(GameState& game, float deltaTime)
{
    if (game.phase == GamePhase::Title)
    {
        if (IsMenuBackKeyPressed())
        {
            game.shouldQuit = true;
        }
        else if (IsTitlePreviousLevelKeyPressed())
        {
            world::CycleCurrentLevel(-1);
        }
        else if (IsTitleNextLevelKeyPressed())
        {
            world::CycleCurrentLevel(1);
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

    if (game.phase == GamePhase::Victory)
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
    game.difficultyLevel = std::max(
        tuning::CalculateDifficultyLevel(game.survivalTime),
        tuning::GetStageDifficultyFloor(game.currentScoreStage));
    game.pickupMessageTimer = std::max(0.0f, game.pickupMessageTimer - deltaTime);
    game.stageAdvanceTimer = std::max(0.0f, game.stageAdvanceTimer - deltaTime);
    if (game.pickupMessageTimer <= 0.0f)
    {
        game.pickupMessage = nullptr;
    }

    const int playerHealthBeforeDamage = game.player.health;
    entities::UpdatePlayer(game.player, deltaTime);
    entities::UpdateWeapon(game.weapon, deltaTime);
    entities::UpdatePickups(game.pickups, deltaTime);

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

    const entities::TargetHitResult hitResult = firedShot ?
        entities::TryHitTargets(
            game.player,
            game.targets,
            game.hitCount,
            game.destroyedCount,
            game.difficultyLevel) :
        entities::TargetHitResult{};
    if (hitResult.hit)
    {
        PlayHitSound(game.audio);
        game.weapon.hitMarker = tuning::kWeapon.hitMarkerStrength;

        if (hitResult.scoreDelta > 0)
        {
            game.score += hitResult.scoreDelta;
            game.bestScore = std::max(game.bestScore, game.score);
            TryAdvanceProgression(game);
        }
    }

    if (game.phase != GamePhase::Gameplay)
    {
        return;
    }

    entities::TryDamagePlayerFromTargets(game.player, game.targets);

    const entities::PickupCollectionResult pickupResult = entities::TryCollectPickups(game.player, game.weapon, game.pickups);
    if (pickupResult.collected)
    {
        entities::AddScreenShake(game.player, 0.16f);
        if (pickupResult.scoreDelta > 0)
        {
            game.score += pickupResult.scoreDelta;
            game.bestScore = std::max(game.bestScore, game.score);
            TryAdvanceProgression(game);
        }

        game.pickupMessage = entities::GetPickupLabel(pickupResult.type);
        game.pickupMessageTimer = tuning::kPickup.feedbackDuration;
    }

    if (game.phase != GamePhase::Gameplay)
    {
        return;
    }

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
    render::DrawPickups(game.player, game.pickups, game.depthBuffer);
    if (game.phase != GamePhase::Title)
    {
        render::DrawWeapon(game.player, game.weapon);
        render::DrawMiniMap(game.player, game.targets, game.pickups);
        render::DrawHud(
            game.player,
            game.weapon,
            game.difficultyLevel,
            game.currentScoreStage,
            tuning::kRun.totalScoreStages,
            tuning::GetScoreStageTarget(std::min(game.currentScoreStage, tuning::kRun.totalScoreStages)),
            (game.phase == GamePhase::Victory) ? 0 : std::max(0, tuning::GetScoreStageTarget(game.currentScoreStage) - game.score),
            game.score,
            game.bestScore,
            game.hitCount,
            game.destroyedCount,
            entities::CountAliveTargets(game.targets),
            game.survivalTime,
            game.bestSurvivalTime,
            game.pickupMessage,
            game.pickupMessageTimer,
            game.stageAdvanceTimer,
            game.phase == GamePhase::GameOver,
            game.phase == GamePhase::Victory);
    }
    EndMode2D();

    if (game.phase == GamePhase::Title)
    {
        render::DrawTitleOverlay(
            world::GetCurrentLevelDisplayName(),
            world::GetCurrentLevelIndex(),
            world::GetLevelCount());
    }
    else if (game.phase == GamePhase::Pause)
    {
        render::DrawPauseOverlay(game.pauseMode, game.pauseMenuSelection);
    }
    else if (game.phase == GamePhase::Victory)
    {
        render::DrawVictoryOverlay(
            game.score,
            game.bestScore,
            game.survivalTime,
            game.bestSurvivalTime,
            tuning::kRun.totalScoreStages);
    }
}
} // namespace core
