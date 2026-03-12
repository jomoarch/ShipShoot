#include "Enemy.h"

Enemy::Enemy(const Vec2 &spawnPos, int maxLife)
    : pos(spawnPos), vel(0, 0), life(maxLife), maxLife(maxLife) {}

// BasicEnemy
BasicEnemy::BasicEnemy(const Vec2 &spawnPos, int maxLife)
    : Enemy(spawnPos, maxLife) {}

void BasicEnemy::update(float dt, const Vec2 &playerPos) {
  Vec2 dir = playerPos - pos;
  setAngleTowards(playerPos);
  float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
  if (len > 0.001f) {
    dir = dir / len;
    pos = pos + dir * SPEED * dt;
  }
}

void BasicEnemy::draw(Renderer &renderer,
                      const std::function<Vec2(Vec2)> &transform) const {
  // 根据生命值计算颜色
  float whiteness = getColorRatio();
  float r = 1.0f;
  float g = whiteness;
  float b = whiteness;

  // 计算三角形的三个顶点
  Vec2 tip = pos + Vec2(std::cos(angle), std::sin(angle)) * SIZE;
  // 底边两个点：从 angle 向左右各偏移 60 度（即尖端角度 120 度）
  float leftAngle = angle + M_PI - (TIP_ANGLE * M_PI / 180.0f) / 2.0f;
  float rightAngle = angle + M_PI + (TIP_ANGLE * M_PI / 180.0f) / 2.0f;
  Vec2 left =
      pos + Vec2(std::cos(leftAngle), std::sin(leftAngle)) * (SIZE * 0.5f);
  Vec2 right =
      pos + Vec2(std::cos(rightAngle), std::sin(rightAngle)) * (SIZE * 0.5f);

  // 转换为屏幕坐标并绘制
  Vec2 t = transform(tip);
  Vec2 l = transform(left);
  Vec2 rp = transform(right);
  renderer.drawTriangle(t.x, t.y, l.x, l.y, rp.x, rp.y, r, g, b);
}

// GreyEnemy
GreyEnemy::GreyEnemy(const Vec2 &spawnPos, int maxLife,
                     std::vector<Missile> *missiles)
    : Enemy(spawnPos, maxLife), attackTimer(0.0f), MISSILE_VECTOR(missiles) {}

void GreyEnemy::update(float dt, const Vec2 &playerPos) {
  // 更新攻击计时器
  attackTimer += dt;

  // 计算到玩家的距离和方向
  Vec2 toPlayer = playerPos - pos;
  float distToPlayer = toPlayer.length();

  // 单位方向向量
  Vec2 dirToPlayer = toPlayer;
  if (distToPlayer > 0.001f) {
    dirToPlayer /= distToPlayer;
  }

  // 计算垂直方向（切向）的单位向量
  Vec2 tangentDir(-dirToPlayer.y, dirToPlayer.x); // 顺时针旋转90度
  // 随机选择方向（使运动更自然）
  static float direction = 1.0f;
  // 偶尔切换方向（每5秒随机一次）
  if (rand() % 300 == 0) {
    direction = (rand() % 2 == 0) ? 1.0f : -1.0f;
  }
  tangentDir = tangentDir * direction;

  // 初始化合力
  Vec2 totalForce(0, 0);

  // 1. 径向力（控制距离）
  if (distToPlayer < SAFE_DISTANCE * 0.8f) {
    // 太近：强斥力
    float strength = 1.5f * (1.0f - distToPlayer / (SAFE_DISTANCE * 0.8f));
    totalForce += -dirToPlayer * strength * SPEED;
  } else if (distToPlayer > SAFE_DISTANCE * 1.2f) {
    // 太远：弱吸引力
    float strength = 0.5f * (distToPlayer / (SAFE_DISTANCE * 1.2f) - 1.0f);
    totalForce += dirToPlayer * strength * SPEED;
  } else {
    // 在理想距离内：主要是切向力（环绕）
    // 径向力很弱，只做微调
    float radialStrength =
        0.2f * (distToPlayer - SAFE_DISTANCE) / SAFE_DISTANCE;
    totalForce += dirToPlayer * radialStrength * SPEED;
  }

  // 2. 切向力（环绕运动）- 在理想距离内最强
  float tangentStrength = 0.0f;
  if (distToPlayer > SAFE_DISTANCE * 0.5f &&
      distToPlayer < SAFE_DISTANCE * 1.5f) {
    // 在范围内时，切向力随距离变化
    float range = 1.0f - std::abs(distToPlayer - SAFE_DISTANCE) / SAFE_DISTANCE;
    tangentStrength = 0.8f * range;
  }
  totalForce += tangentDir * tangentStrength * SPEED;

  // 3. 添加一些随机性，使运动更自然
  if (rand() % 100 < 5) {                               // 5%的概率添加随机扰动
    float randomAngle = (float)(rand() % 628) / 100.0f; // 0~6.28
    Vec2 randomDir(std::cos(randomAngle), std::sin(randomAngle));
    totalForce += randomDir * SPEED * 0.3f * dt;
  }

  // 更新速度（带转向不灵敏）
  if (totalForce.length() > 0.001f) {
    Vec2 desiredVel = totalForce;
    float desiredLen = desiredVel.length();
    desiredVel = desiredVel / desiredLen;

    // 当前速度方向
    float currentVelAngle = atan2(vel.y, vel.x);
    float desiredAngle = atan2(desiredVel.y, desiredVel.x);

    // 计算角度差，限制转向速度
    float angleDiff = desiredAngle - currentVelAngle;
    while (angleDiff > M_PI)
      angleDiff -= 2 * M_PI;
    while (angleDiff < -M_PI)
      angleDiff += 2 * M_PI;

    // 转向不灵敏：每帧只能转一定角度
    float maxTurn = TURN_RATE * dt;
    if (angleDiff > maxTurn)
      angleDiff = maxTurn;
    if (angleDiff < -maxTurn)
      angleDiff = -maxTurn;

    // 更新速度方向
    float newAngle = currentVelAngle + angleDiff;
    vel = Vec2(std::cos(newAngle), std::sin(newAngle)) * SPEED;
  } else {
    // 如果没有力，减速
    vel = vel * 0.95f;
  }

  // 更新位置
  pos = pos + vel * dt;

  // 更新朝向为速度方向
  if (vel.length() > 0.001f) {
    angle = atan2(vel.y, vel.x);
  } else {
    // 如果速度为0，朝向玩家
    setAngleTowards(playerPos);
  }

  // 攻击逻辑
  if (attackTimer >= COOLDOWN && distToPlayer < 200.0f) {
    attackTimer = 0.0f;

    // 发射3发导弹，间隔0.1秒
    for (int i = 0; i < 3; i++) {
      // 计算发射方向（略微散开）
      float spreadAngle = angle + (i - 1) * 0.2f;
      Vec2 dir(std::cos(spreadAngle), std::sin(spreadAngle));

      Missile m(pos, playerPos, dir * (SPEED * 1.6f));
      if (MISSILE_VECTOR) {
        MISSILE_VECTOR->push_back(m);
      }
    }
  }
}

void GreyEnemy::draw(Renderer &renderer,
                     const std::function<Vec2(Vec2)> &transform) const {
  // 灰色，根据生命值变色
  float whiteness = getColorRatio();
  float r = 0.5f - whiteness * 0.5f;
  float g = 0.5f + whiteness * 0.5f;
  float b = 0.5f + whiteness * 0.5f;

  // 计算三角形的三个顶点（顶角90度）
  Vec2 tip = pos + Vec2(std::cos(angle), std::sin(angle)) * SIZE;
  float leftAngle = angle + M_PI - (TIP_ANGLE * M_PI / 180.0f) / 2.0f;
  float rightAngle = angle + M_PI + (TIP_ANGLE * M_PI / 180.0f) / 2.0f;
  Vec2 left =
      pos + Vec2(std::cos(leftAngle), std::sin(leftAngle)) * (SIZE * 0.6f);
  Vec2 right =
      pos + Vec2(std::cos(rightAngle), std::sin(rightAngle)) * (SIZE * 0.6f);

  // 转换为屏幕坐标并绘制
  Vec2 t = transform(tip);
  Vec2 l = transform(left);
  Vec2 rp = transform(right);
  renderer.drawTriangle(t.x, t.y, l.x, l.y, rp.x, rp.y, r, g, b);
}