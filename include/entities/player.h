#pragma once

#include "raylib.h"

namespace entities
{
constexpr float kMoveSpeed = 3.4f;
constexpr int kPlayerMaxHealth = 100;

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
bool ApplyDamage(Player& player, int damage);
} // namespace entities
