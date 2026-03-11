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

void scroll_cb(GLFWwindow *w, double xoff, double yoff) {}

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
};

// 游戏状态
static std::vector<std::unique_ptr<Enemy>> enemies;
static float spawnTimer = 0.0f;
static bool gameOver = false;
// 过热计数器
const int maxHeat = 20;
static int heatCount = 0;

// 随机边框位置选取器
Vec2 getRandomSpawnPosition() {
  int edge = rand() % 4;
  float px, py;
  float offset = 20.0f;
  float left = camPos.x - SCR_WIDTH / 2.0f;
  float right = camPos.x + SCR_WIDTH / 2.0f;
  float bottom = camPos.y - SCR_HEIGHT / 2.0f;
  float top = camPos.y + SCR_HEIGHT / 2.0f;
  if (edge == 0) {
    px = left - offset;
    py = bottom + (float)(rand() % SCR_HEIGHT);
  } else if (edge == 1) {
    px = right + offset;
    py = bottom + (float)(rand() % SCR_HEIGHT);
  } else if (edge == 2) {
    py = bottom - offset;
    px = left + (float)(rand() % SCR_WIDTH);
  } else {
    py = top + offset;
    px = left + (float)(rand() % SCR_WIDTH);
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
  if (!renderer.init()) {
    std::cerr << "renderer init failed" << std::endl;
    return -1;
  }
  renderer.setProjection((float)SCR_WIDTH, (float)SCR_HEIGHT);
  glPointSize(1.0f);

  Player player(SCR_WIDTH / 2.0f, SCR_HEIGHT / 2.0f);
  std::vector<Bullet> bullets;
  std::vector<Spark> sparks;

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
    float lx = (float)mouseX * ((float)SCR_WIDTH / winW);
    float ly = (float)mouseY * ((float)SCR_HEIGHT / winH);
    float sx = lx - SCR_WIDTH / 2.0f;
    float sy = (SCR_HEIGHT - ly) - SCR_HEIGHT / 2.0f;
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
          sparks.push_back(s);
          if (e->isDead()) {
            Spark se;
            se.pos = e->getPosition();
            se.life = 0.5f;
            se.r = 1;
            se.g = 1;
            se.b = 1;
            sparks.push_back(se);
          }
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
      float cx = SCR_WIDTH / 2.0f, cy = SCR_HEIGHT / 2.0f;
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
      float t = s.life / 0.3f;
      float r = s.r;
      float g = s.g;
      float b = s.b;
      Vec2 p = transform(s.pos);
      renderer.drawCircle(p.x, p.y, 2.0f, r, g, b, false);
    }
    // 顶角为120度的红色敌机
    for (auto &eit : enemies) {
      Enemy &e = *eit;
      float whiteness = e.getColorRatio();
      float er = 1.0f;
      float eg = whiteness;
      float eb = whiteness;
      float esz = 10.0f;
      float ang = atan2(player.pos.y - e.getPosition().y,
                        player.pos.x - e.getPosition().x);
      Vec2 etip(e.getPosition().x + static_cast<float>(cos(ang)) * esz,
                e.getPosition().y + static_cast<float>(sin(ang)) * esz);
      float baseAng = ang + M_PI - (120.0f * M_PI / 180.0f) / 2.0f;
      Vec2 eleft(
          e.getPosition().x + static_cast<float>(cos(baseAng)) * esz * 0.5f,
          e.getPosition().y + static_cast<float>(sin(baseAng)) * esz * 0.5f);
      Vec2 eright(
          e.getPosition().x +
              static_cast<float>(cos(baseAng + (120.0f * M_PI / 180.0f))) *
                  esz * 0.5f,
          e.getPosition().y +
              static_cast<float>(sin(baseAng + (120.0f * M_PI / 180.0f))) *
                  esz * 0.5f);
      Vec2 etp = transform(etip), elp = transform(eleft),
           erp = transform(eright);
      renderer.drawTriangle(etp.x, etp.y, elp.x, elp.y, erp.x, erp.y, er, eg,
                            eb);
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
