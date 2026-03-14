#pragma once

#include "raylib.h"

namespace entities
{
constexpr float kMoveSpeed = 3.4f;

struct Player
{
    Vector2 position;
    Vector2 velocity;
    float angle;
    float turnVelocity;
    float fov;
};

struct WeaponState
{
    float recoil;
    float shotCooldown;
    float muzzleFlash;
    float hitMarker;
};

Player MakePlayer(Vector2 spawnPosition);
WeaponState MakeWeaponState();
void UpdatePlayer(Player& player, float deltaTime);
void UpdateWeapon(WeaponState& weapon, float deltaTime);
bool TryFireWeapon(WeaponState& weapon);
} // namespace entities
