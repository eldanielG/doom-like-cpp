#pragma once

#include "raylib.h"

namespace core
{
struct AudioState
{
    Sound shoot{};
    Sound hit{};
    Sound playerHurt{};
    Sound gameOver{};
    bool initialized = false;
};

void InitializeAudio(AudioState& audio);
void ShutdownAudio(AudioState& audio);
void PlayShootSound(const AudioState& audio);
void PlayHitSound(const AudioState& audio);
void PlayPlayerHurtSound(const AudioState& audio);
void PlayGameOverSound(const AudioState& audio);
} // namespace core
