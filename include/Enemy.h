#pragma once
#include "Math.h"
#include "Renderer.h"
#include <memory>
#include <vector>
#include <algorithm>
#include <functional>

class Renderer;

// 导弹结构体
struct Missile {
  Vec2 pos;
  Vec2 vel;
  Vec2 targetPos;          // 目标位置（用于追踪）
  float life;              // 总生命时间
  float age;               // 已存在时间
  bool isTracking;         // 是否还在追踪阶段
  std::vector<Vec2> trail; // 拖尾轨迹

  Missile(const Vec2 &startPos, const Vec2 &target, const Vec2 &startVel)
      : pos(startPos), vel(startVel), targetPos(target), life(3.0f), age(0.0f),
        isTracking(true) {}

  void update(float dt, const Vec2 &playerPos) {
    // 记录拖尾
    trail.push_back(pos);
    if (trail.size() > 10)
      trail.erase(trail.begin());

    // 更新年龄
    age += dt;

    // 0.5秒后停止追踪
    if (age > 0.8f)
      isTracking = false;

    if (isTracking) {
      Vec2 dir = targetPos - pos;
      float len = dir.length();
      if (len > 0.001f) {
        dir /= len;
        vel = dir * 160.0f;
      }
      // 每帧更新目标位置为玩家当前位置
      targetPos = playerPos;
    } else {
      float speed = 300.0f;
      Vec2 dir = vel;
      float len = dir.length();
      if (len > 0.001f) {
        dir /= len;
        vel = dir * speed;
      }
    }

    pos += vel * dt;
  }

  bool isExpired() const { return age >= life; }
};

class Enemy {
public:
  Enemy(const Vec2 &spawnPos, int maxLife);
  virtual ~Enemy() = default;

  // 更新敌人行为
  virtual void update(float dt, const Vec2 &playerPos) = 0;

  virtual void draw(Renderer &renderer,
                    const std::function<Vec2(Vec2)> &transform) const = 0;

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
  void setAngle(float newAngle) { angle = newAngle; }
  void setAngleTowards(const Vec2 &target) {
    Vec2 dir = target - pos;
    angle = atan2(dir.y, dir.x);
  }
  void setLife(int newLife) { life = newLife; }
  static void setEnemyList(std::vector<std::unique_ptr<Enemy>> *list) {
    s_enemies = list;
  }

  float getColorRatio() const { return 1.0f - (float)life / maxLife; }

  // 受到伤害
  void takeDamage(int dmg) {
    life -= dmg;
    if (life < 0)
      life = 0;
  }

protected:
  Vec2 computeSeparation(float threshold, float strength = 1.0f) const;

  Vec2 pos;
  Vec2 vel;
  float angle;
  int life;
  int maxLife;

private:
  static std::vector<std::unique_ptr<Enemy>> *s_enemies;
};

// 普通敌人
class BasicEnemy : public Enemy {
public:
  BasicEnemy(const Vec2 &spawnPos, int maxLife = 20);
  void update(float dt, const Vec2 &playerPos) override;
  void draw(Renderer &renderer,
            const std::function<Vec2(Vec2)> &transform) const override;

private:
  static constexpr float SPEED = 120.0f;
  static constexpr float SIZE = 10.0f;
  static constexpr float TIP_ANGLE = 120.0f;
};

class GreyEnemy : public Enemy {
public:
  GreyEnemy(const Vec2 &spawnPos, int maxLife = 50,
            std::vector<Missile> *missiles = nullptr);
  void update(float dt, const Vec2 &playerPos) override;
  void draw(Renderer &renderer,
            const std::function<Vec2(Vec2)> &transform) const override;

private:
  static constexpr float SPEED = 100.0f;
  static constexpr float SIZE = 12.0f;
  static constexpr float TIP_ANGLE = 90.0f;     // 顶角90度
  static constexpr float COOLDOWN = 3.0f;       // 攻击冷却3秒
  static constexpr float SAFE_DISTANCE = 80.0f; // 保持距离
  static constexpr float TURN_RATE = 2.0f;      // 转向灵敏度（越小越不灵敏）

  float attackTimer; // 攻击计时器
  int missileCount;
  std::vector<Missile> *MISSILE_VECTOR; // 指向全局导弹列表的指针
};