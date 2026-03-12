#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include "Renderer.h"
#include "Math.h"
#include "Enemy.h"

// 默认窗口大小
const unsigned int SCR_WIDTH = 400;
const unsigned int SCR_HEIGHT = 300;

// 自动调整窗口
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

// 鼠标进入窗口回调
static bool cursorIn = true;
static float viewAngle = 0.0f;

// 摄像机状态
static Vec2 camPos(SCR_WIDTH / 2.0f, SCR_HEIGHT / 2.0f);
static Vec2 camVel(0.0f, 0.0f);
static const float camAccel = 200.0f;
static const float camDamp = 4.0f;

// 鼠标进入/离开窗口时隐藏/显示光标
void cursor_enter_cb(GLFWwindow *w, int entered) {
  cursorIn = entered;
  glfwSetInputMode(w, GLFW_CURSOR,
                   entered ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
}

// 游戏区大小
static float gameWidth = SCR_WIDTH;
static float gameHeight = SCR_HEIGHT;
static Renderer *g_renderer = nullptr;

void scroll_cb(GLFWwindow *w, double xoff, double yoff) {
  // 滚轮缩放游戏区大小
  float scaleFactor = 1.0f + (float)yoff * 0.1f;
  const float MIN_SIZE = 0.5f;
  const float MAX_SIZE = 2.0f;
  if (scaleFactor < MIN_SIZE)
    scaleFactor = MIN_SIZE;
  if (scaleFactor > MAX_SIZE)
    scaleFactor = MAX_SIZE;
  gameWidth *= scaleFactor;
  gameHeight *= scaleFactor;

  // 更新渲染器的投影矩阵
  if (g_renderer) {
    g_renderer->setProjection(gameWidth, gameHeight);
  }
}

// 玩家
struct Player {
  Vec2 pos;
  float angle;
  Player(float x, float y) : pos(x, y), angle(0) {}
};
// 子弹
struct Bullet {
  Vec2 pos, vel;
  float life;
  std::vector<Vec2> trail;
};
// 火花
struct Spark {
  Vec2 pos;
  float life;
  float r, g, b;
  float radius;
};

// 游戏状态
static std::vector<std::unique_ptr<Enemy>> enemies;
static std::vector<Missile> missiles;
static float spawnTimer = 0.0f;
static float greySpawnTimer = 0.0f;
static bool gameOver = false;
// 过热计数器
const int maxHeat = 80;
static int heatCount = 0;

// 随机边框位置选取器
Vec2 getRandomSpawnPosition() {
  int edge = rand() % 4;
  float px, py;
  float offset = 20.0f;
  float left = camPos.x - gameWidth / 2.0f;
  float right = camPos.x + gameWidth / 2.0f;
  float bottom = camPos.y - gameHeight / 2.0f;
  float top = camPos.y + gameHeight / 2.0f;
  if (edge == 0) {
    px = left - offset;
    py = bottom + (float)(rand() % static_cast<int>(gameHeight));
  } else if (edge == 1) {
    px = right + offset;
    py = bottom + (float)(rand() % static_cast<int>(gameHeight));
  } else if (edge == 2) {
    py = bottom - offset;
    px = left + (float)(rand() % static_cast<int>(gameWidth));
  } else {
    py = top + offset;
    px = left + (float)(rand() % static_cast<int>(gameWidth));
  }
  return Vec2{px, py};
}

int main() {
  if (!glfwInit()) {
    std::cerr << "GLFW init failed" << std::endl;
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Pixel Shooter",
                                        nullptr, nullptr);
  if (!window) {
    std::cerr << "window create failed" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorEnterCallback(window, cursor_enter_cb);
  glfwSetScrollCallback(window, scroll_cb);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "GLAD init failed" << std::endl;
    glfwDestroyWindow(window);
    glfwTerminate();
    return -1;
  }

  Renderer renderer;
  g_renderer = &renderer;
  if (!renderer.init()) {
    std::cerr << "renderer init failed" << std::endl;
    return -1;
  }
  renderer.setProjection((float)SCR_WIDTH, (float)SCR_HEIGHT);
  glPointSize(1.0f);

  Player player(SCR_WIDTH / 2.0f, SCR_HEIGHT / 2.0f);
  std::vector<Bullet> bullets;
  std::vector<Spark> sparks;

  Enemy::setEnemyList(&enemies);

  double mouseX = 0, mouseY = 0;

  float lastTime = glfwGetTime();
  // 主循环
  while (!glfwWindowShouldClose(window)) {
    float now = glfwGetTime();
    float dt = now - lastTime;
    lastTime = now;
    bool up = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
              glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    bool down = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
    bool left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
    bool right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
                 glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, true);
    }

    // 处理偏移坐标下的鼠标位置
    glfwGetCursorPos(window, &mouseX, &mouseY);
    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);
    float lx = (float)mouseX * (gameWidth / winW);
    float ly = (float)mouseY * (gameHeight / winH);
    float sx = lx - gameWidth / 2.0f;
    float sy = (gameHeight - ly) - gameHeight / 2.0f;
    float ca = cos(-viewAngle), sa = sin(-viewAngle);
    float wx = sx * ca - sy * sa;
    float wy = sx * sa + sy * ca;
    Vec2 mpos(wx + camPos.x, wy + camPos.y);

    // 调整玩家朝向鼠标
    if (cursorIn) {
      int dx = mpos.x - player.pos.x;
      int dy = mpos.y - player.pos.y;
      player.angle = atan2(dy, dx);
    }
    // 玩家键盘移动
    float speed = 80.0f;
    Vec2 move(0, 0);
    if (up)
      move.y += 1;
    if (down)
      move.y -= 1;
    if (left)
      move.x -= 1;
    if (right)
      move.x += 1;
    if (move.x != 0 || move.y != 0) {
      float len = move.length();
      move /= len;
      player.pos += move * speed * dt;
    }
    // 玩家鼠标移动加速
    bool rightBtn =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (rightBtn) {
      Vec2 dir(mpos - player.pos);
      float len = dir.length();
      if (len > 0.001f) {
        dir /= len;
        player.pos += dir * speed * dt;
      }
    }

    // 玩家射击逻辑和过热机制
    bool leftBtn =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (leftBtn) {
      if (heatCount < maxHeat) {
        Vec2 dir(mpos - player.pos);
        float len = dir.length();
        if (len > 0.001f) {
          dir /= len;
          Bullet b;
          b.pos = player.pos;
          b.vel = Vec2{dir.x * 200.0f, dir.y * 200.0f};
          b.life = 2.0f;
          b.trail.clear();
          bullets.push_back(b);
          heatCount++;
          if (heatCount > maxHeat)
            heatCount = maxHeat;
        }
      }
    } else {
      if (heatCount > 0)
        heatCount -= (int)(80.0f * dt);
      if (heatCount < 0)
        heatCount = 0;
    }

    // 更新子弹位置，处理碰撞和过期
    for (auto it = bullets.begin(); it != bullets.end();) {
      it->trail.push_back(it->pos);
      if (it->trail.size() > 5)
        it->trail.erase(it->trail.begin());
      it->pos += it->vel * dt;
      it->life -= dt;
      bool remove = false;
      // 检测子弹与敌人的碰撞
      for (auto &e : enemies) {
        float dx = it->pos.x - e->getPosition().x;
        float dy = it->pos.y - e->getPosition().y;
        if (dx * dx + dy * dy < 64.0f) {
          e->takeDamage(1);
          float whiteness = 1.0f - (float)e->getLife() / e->getMaxLife();
          Spark s;
          s.pos = it->pos;
          s.life = 0.2f;
          s.r = 1.0f;
          s.g = 1.0f - whiteness;
          s.b = 1.0f - whiteness;
          s.radius = 2.0f;
          sparks.push_back(s);
          remove = true;
          break;
        }
      }
      if (it->life <= 0)
        remove = true;
      if (remove) {
        Spark s;
        s.pos = it->pos;
        s.life = 0.3f;
        s.r = 1.0f;
        s.g = 0.7f;
        s.b = 0.0f;
        s.radius = 2.0f;
        sparks.push_back(s);
        it = bullets.erase(it);
      } else
        ++it;
    }

    // 更新火花状态，移除过期的火花
    for (auto it = sparks.begin(); it != sparks.end();) {
      it->life -= dt;
      if (it->life <= 0)
        it = sparks.erase(it);
      else
        ++it;
    }

    // 更新敌人位置，处理与玩家的碰撞
    for (auto &eit : enemies) {
      eit->update(dt, player.pos);
    }

    // 更新导弹
    for (auto it = missiles.begin(); it != missiles.end();) {
      it->update(dt, player.pos);
      if (it->age < 0.1f)
        continue;
      // 检查导弹与玩家的碰撞
      if ((it->pos - player.pos).lengthSq() < 64.0f) { // 半径8
        gameOver = true;
        break;
      }
      // 检查导弹与所有敌人的碰撞（包括其他灰机）
      bool hitEnemy = false;
      for (auto &enemyPtr : enemies) {
        if ((it->pos - enemyPtr->getPosition()).lengthSq() < 64.0f) {
          // 导弹命中敌人
          enemyPtr->takeDamage(10); // 造成10点伤害
          if (!hitEnemy) {
            Spark s;
            s.pos = it->pos;
            s.life = 0.3f;
            s.r = 1.0f;
            s.g = 0.5f;
            s.b = 0.0f;
            s.radius = 5.0f;
            sparks.push_back(s);
            hitEnemy = true;
          }
        }
      }
      if (hitEnemy)
        missiles.erase(it);
      if (it != missiles.end()) {
        if (it->isExpired()) {
          it = missiles.erase(it);
        } else {
          ++it;
        }
      }
    }

    for (auto &eit : enemies) {
      const Vec2 &ep = eit->getPosition();
      if ((ep - player.pos).lengthSq() < 100.0f) {
        gameOver = true;
        break;
      }
    }
    if (gameOver)
      break;

    // 刷新敌人，随机从四边生成普通敌人
    spawnTimer += dt;
    if (spawnTimer >= 2.0f) {
      spawnTimer = 0.0f;
      enemies.push_back(
          std::make_unique<BasicEnemy>(getRandomSpawnPosition(), 20));
    }
    // 刷新灰色敌人
    greySpawnTimer += dt;
    if (greySpawnTimer >= 10.0f) {
      greySpawnTimer = 0.0f;
      enemies.push_back(
          std::make_unique<GreyEnemy>(getRandomSpawnPosition(), 50, &missiles));
    }
    Enemy::removeDeadEnemies(enemies);

    // 渲染和摄像机控制
    glClearColor(0, 0, 0, 1);
    renderer.clear();
    {
      Vec2 diff(player.pos - camPos);
      Vec2 acc(diff * camAccel);
      camVel += acc * dt;
      camVel *= exp(-camDamp * dt);
      camPos += camVel * dt;
    }
    auto transform = [&](Vec2 p) -> Vec2 {
      float cx = gameWidth / 2.0f, cy = gameHeight / 2.0f;
      float x = p.x - camPos.x;
      float y = p.y - camPos.y;
      float ca = cos(viewAngle), sa = sin(viewAngle);
      float rx = x * ca - y * sa;
      float ry = x * sa + y * ca;
      return Vec2{rx + cx, ry + cy};
    };
    // 下面是绘图逻辑，没有问题，不需要看
    // 圆形火花
    for (auto &s : sparks) {
      float r = s.r;
      float g = s.g + s.life;
      float b = s.b + s.life;
      float radius = s.radius * ((s.life + 1) / 1.3f);
      Vec2 p = transform(s.pos);
      renderer.drawCircle(p.x, p.y, radius, r, g, b, false);
    }
    // 敌机
    for (auto &eit : enemies) {
      eit->draw(renderer, transform);
    }
    // 绘制导弹（亮红色，带拖尾）
    for (auto &m : missiles) {
      // 绘制拖尾
      int idx = 0;
      for (auto &tp : m.trail) {
        float alpha = (float)idx / (float)m.trail.size();
        Vec2 p = transform(tp);
        // 拖尾颜色：亮红到暗红
        renderer.drawCircle(p.x, p.y, 1.5f * (alpha + 0.5f), 1.0f,
                            0.3f * (1 - alpha), 0.0f);
        idx++;
      }
      // 绘制导弹本体
      Vec2 p = transform(m.pos);
      renderer.drawCircle(p.x, p.y, 2.0f, 1.0f, 0.5f, 0.5f, false);
    }
    // 绿机子弹
    for (auto &b : bullets) {
      int idx = 0;
      for (auto &tp : b.trail) {
        float alpha = (float)idx / (float)b.trail.size();
        Vec2 p = transform(tp);
        renderer.drawPixel(p.x, p.y, 1.0f, 0.5f * (1 - alpha), 0.0f);
        idx++;
      }
      Vec2 p = transform(b.pos);
      renderer.drawPixel(p.x, p.y, 1.0f, 0.5f, 0.0f);
    }
    // 玩家绿机
    float size = 8.0f;
    Vec2 tip(player.pos.x + static_cast<float>(cos(player.angle)) * size,
             player.pos.y + static_cast<float>(sin(player.angle)) * size);
    float baseAngle = player.angle + M_PI - (30.0f * M_PI / 180.0f) / 2.0f;
    Vec2 leftp(player.pos.x + static_cast<float>(cos(baseAngle)) * size * 0.5f,
               player.pos.y + static_cast<float>(sin(baseAngle)) * size * 0.5f);
    Vec2 rightp(player.pos.x + static_cast<float>(
                                   cos(baseAngle + (30.0f * M_PI / 180.0f))) *
                                   size * 0.5f,
                player.pos.y + static_cast<float>(
                                   sin(baseAngle + (30.0f * M_PI / 180.0f))) *
                                   size * 0.5f);
    Vec2 tp = transform(tip), lp = transform(leftp), rp = transform(rightp);
    float heatRatio = (float)heatCount / maxHeat;
    float pr, pg;
    if (heatRatio < 0.8f) {
      pr = heatRatio / 0.8f;
      pg = 1.0f;
    } else {
      pr = 1.0f;
      pg = 1.0f - (heatRatio - 0.8f) / 0.2f;
      if (pg < 0.0f)
        pg = 0.0f;
    }
    renderer.drawTriangle(tp.x, tp.y, lp.x, lp.y, rp.x, rp.y, pr, pg, 0.5f);

    // 鼠标位置指示
    if (cursorIn) {
      Vec2 mc = transform(mpos);
      renderer.drawCircle(mc.x, mc.y, 3.0f, 1, 1, 1, true);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
