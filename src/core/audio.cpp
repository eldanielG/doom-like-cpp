#include "core/audio.h"

namespace core
{
void InitializeAudio(AudioState& audio)
{
    if (!IsAudioDeviceReady())
    {
        InitAudioDevice();
    }

    audio.initialized = IsAudioDeviceReady();
    if (!audio.initialized)
    {
        return;
    }

    audio.shoot = LoadSound("assets/audio/sfx/shoot.wav");
    audio.hit = LoadSound("assets/audio/sfx/hit.wav");
    audio.playerHurt = LoadSound("assets/audio/sfx/player_hurt.wav");
    audio.gameOver = LoadSound("assets/audio/sfx/game_over.wav");

    SetSoundVolume(audio.shoot, 0.75f);
    SetSoundVolume(audio.hit, 0.70f);
    SetSoundVolume(audio.playerHurt, 0.85f);
    SetSoundVolume(audio.gameOver, 0.90f);
}

void ShutdownAudio(AudioState& audio)
{
    if (!audio.initialized)
    {
        return;
    }

    UnloadSound(audio.shoot);
    UnloadSound(audio.hit);
    UnloadSound(audio.playerHurt);
    UnloadSound(audio.gameOver);
    audio = AudioState{};

    if (IsAudioDeviceReady())
    {
        CloseAudioDevice();
    }
}

void PlayShootSound(const AudioState& audio)
{
    if (audio.initialized)
    {
        PlaySound(audio.shoot);
    }
}

void PlayHitSound(const AudioState& audio)
{
    if (audio.initialized)
    {
        PlaySound(audio.hit);
    }
}

void PlayPlayerHurtSound(const AudioState& audio)
{
    if (audio.initialized)
    {
        PlaySound(audio.playerHurt);
    }
}

void PlayGameOverSound(const AudioState& audio)
{
    if (audio.initialized)
    {
        PlaySound(audio.gameOver);
    }
}
} // namespace core
