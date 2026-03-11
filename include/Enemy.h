#pragma once
#include "Math.h"
#include <memory>
#include <vector>
#include <algorithm>

class Enemy {
public:
  Enemy(const Vec2 &spawnPos, int maxLife);
  virtual ~Enemy() = default;

  // 更新敌人行为
  virtual void update(float dt, const Vec2 &playerPos) = 0;

  // 公共访问接口
  const Vec2 &getPosition() const { return pos; }
  int getLife() const { return life; }
  int getMaxLife() const { return maxLife; }
  bool isDead() const { return life <= 0; }
  static void removeDeadEnemies(std::vector<std::unique_ptr<Enemy>> &enemies) {
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                                 [](const std::unique_ptr<Enemy> &e) {
                                   return e->isDead();
                                 }),
                  enemies.end());
  }

  void setPosition(const Vec2 &newPos) { pos = newPos; }
  void setVelocity(const Vec2 &newVel) { vel = newVel; }
  void setLife(int newLife) { life = newLife; }

  float getColorRatio() const { return 1.0f - (float)life / maxLife; }

  // 受到伤害
  void takeDamage(int dmg) {
    life -= dmg;
    if (life < 0)
      life = 0;
  }

protected:
  Vec2 pos;
  Vec2 vel;
  int life;
  int maxLife;
};

// 普通敌人
class BasicEnemy : public Enemy {
public:
  BasicEnemy(const Vec2 &spawnPos, int maxLife = 20);
  void update(float dt, const Vec2 &playerPos) override;

private:
  static constexpr float SPEED = 120.0f;
};