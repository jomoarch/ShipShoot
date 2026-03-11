#include "Enemy.h"

Enemy::Enemy(const Vec2 &spawnPos, int maxLife)
    : pos(spawnPos), vel(0, 0), life(maxLife), maxLife(maxLife) {}

// BasicEnemy
BasicEnemy::BasicEnemy(const Vec2 &spawnPos, int maxLife)
    : Enemy(spawnPos, maxLife) {}

void BasicEnemy::update(float dt, const Vec2 &playerPos) {
  Vec2 dir = playerPos - pos;
  float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
  if (len > 0.001f) {
    dir = dir / len;
    pos = pos + dir * SPEED * dt;
  }
}
