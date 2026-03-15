#pragma once

#include "raylib.h"

namespace entities
{
struct Player
{
    Vector2 position;
    Vector2 velocity;
    float angle;
    float turnVelocity;
    float fov;
    int health;
    float invulnerabilityTimer;
    float damageFlash;
    float screenShake;
};

struct WeaponState
{
    float recoil;
    float shotCooldown;
    float muzzleFlash;
    float hitMarker;
    float rapidFireTimer;
};

Player MakePlayer(Vector2 spawnPosition);
WeaponState MakeWeaponState();
void UpdatePlayerFeedback(Player& player, float deltaTime);
void UpdatePlayer(Player& player, float deltaTime);
void UpdateWeapon(WeaponState& weapon, float deltaTime);
bool TryFireWeapon(WeaponState& weapon);
bool ApplyDamage(Player& player, int damage);
void AddScreenShake(Player& player, float amount);
} // namespace entities
