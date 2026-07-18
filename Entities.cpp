// 游戏实体实现：处理动画、子弹、道具、玩家和敌人的更新与绘制。
// 本文件不负责全局流程切换，只维护实体自身行为和实体级别的视觉反馈。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的结构体、函数、枚举或常量。
// [EasyX] 表示 EasyX 图形库提供的绘图函数或类型。
// [C++标准库] 表示 C++ 标准库提供的算法、数学函数或容器。
#include "Entities.h"

#include <algorithm>
#include <cmath>

#include "RenderUtil.h"

namespace {
// 敌机基础参数表。
//
// 把不同敌人的初始参数集中放在表里，比在构造函数里写很多 if 更清楚。
// 下标 0/1/2 分别对应小敌机、中敌机、Boss。
struct EnemyProfile {
  int w;
  int h;
  int hp;
  double speed;
  int score;
  double shootCooldown;
};

const EnemyProfile ENEMY_PROFILES[] = {{40, 40, 1, 220, 100, 2.0},
                                       {60, 60, 3, 150, 300, 1.5},
                                       {180, 135, 80, 105, 8000, 0.50}};
}  // namespace

// [自定义构造函数] 创建指定位置和帧序列的动画效果。
AnimationEffect::AnimationEffect(double _x, double _y,
                                 const std::vector<IMAGE*>& _frames,
                                 double _frameInterval, int _fallbackRadius)
    : x(_x),
      y(_y),
      timer(0),
      frameInterval(_frameInterval),
      fallbackDuration(0.45),
      alive(true),
      fallbackRadius(_fallbackRadius),
      frames(_frames) {}

// [自定义成员函数] 更新动画播放时间。
void AnimationEffect::Update(double dt) {
  // 所有计时都用 dt 累加，保证动画速度不依赖电脑帧率。
  timer += dt;

  if (!frames.empty()) {
    // 如果有图片帧，总时长 = 每帧时长 * 帧数。
    double total = frameInterval * frames.size();
    if (timer >= total) {
      alive = false;
    }
  } else {
    // 没有图片帧时，使用兜底圆形动画的总时长。
    if (timer >= fallbackDuration) {
      alive = false;
    }
  }
}

// [自定义成员函数] 判断动画是否仍需保留。
bool AnimationEffect::Alive() const { return alive; }

// [自定义成员函数] 绘制当前动画帧或兜底圆形效果。
void AnimationEffect::Render() {
  if (!frames.empty()) {
    // 根据已经播放的时间计算当前应该显示第几帧。
    int index = (int)(timer / frameInterval);

    // 防止因为浮点误差或最后一帧越界。
    if (index < 0) {
      index = 0;
    }
    if (index >= (int)frames.size()) {
      index = (int)frames.size() - 1;
    }
    // [自定义函数] DrawCenteredSprite 封装 EasyX 图片居中绘制。
    DrawCenteredSprite(x, y, frames[index]);
    return;
  }

  // 兜底动画：没有图片时，用逐渐扩大的圆圈表示爆炸。
  double k = timer / fallbackDuration;
  k = clamp_val(k, 0.0, 1.0);

  int r = (int)(18 + fallbackRadius * k);
  // [EasyX] setlinecolor/circle 绘制兜底圆形爆炸。
  setlinecolor(RGB(0, 0, 0));
  circle((int)x, (int)y, r);
  circle((int)x, (int)y, r / 2);
}

// [自定义构造函数] 创建子弹。
Bullet::Bullet(double _x, double _y, double _vx, double _vy, BulletKind _kind)
    : x(_x), y(_y), vx(_vx), vy(_vy), alive(true), kind(_kind) {}

// [自定义成员函数] 更新子弹位置并处理出界。
void Bullet::Update(double dt) {
  // 物体移动基本公式：新位置 = 旧位置 + 速度 * 时间。
  x += vx * dt;
  y += vy * dt;

  // 子弹飞出屏幕一段距离后就标记为死亡，之后会被清理。
  // 留 100 像素余量是为了避免刚到边缘就突然消失显得突兀。
  if (y < -100 || y > SCREEN_H + 100 || x < -100 || x > SCREEN_W + 100) {
    alive = false;
  }
}

// [自定义成员函数] 按子弹类型绘制贴图或兜底圆点。
void Bullet::Render(ResourceManager& rm) {
  IMAGE* img = nullptr;

  // 根据子弹类型选择贴图。
  if (kind == BulletKind::Player) {
    img = rm.GetImage(AssetID::Bullet_Player);
  } else if (kind == BulletKind::BossSpecial) {
    img = rm.GetImage(AssetID::Bullet_Boss_Special);
  } else {
    img = rm.GetImage(AssetID::Bullet_Enemy);
  }

  if (img) {
    // BossSpecial 用补给图当特殊弹，所以旋转 180 度让方向更像向下飞。
    if (kind == BulletKind::BossSpecial) {
      // [自定义函数] DrawCenteredSpriteRot180 封装 EasyX 像素绘制和旋转。
      DrawCenteredSpriteRot180(x, y, img);
    } else {
      // [自定义函数] DrawCenteredSprite 封装 EasyX 图片居中绘制。
      DrawCenteredSprite(x, y, img);
    }
  } else {
    // 如果图片加载失败，用简单圆形兜底，方便缺资源时仍能看见子弹。
    if (kind == BulletKind::Player) {
      setfillcolor(RGB(120, 220, 255));
    } else if (kind == BulletKind::BossSpecial) {
      setfillcolor(RGB(255, 210, 70));
    } else if (kind == BulletKind::Boss) {
      setfillcolor(RGB(255, 80, 220));
    } else {
      setfillcolor(RGB(255, 110, 110));
    }

    // [EasyX] solidcircle 绘制兜底圆点。
    solidcircle((int)x, (int)y,
                kind == BulletKind::BossSpecial
                    ? 14
                    : (kind == BulletKind::Boss ? 8 : 6));
  }
}

// [自定义构造函数] 创建掉落道具。
Item::Item(double _x, double _y, ItemType t)
    : x(_x), y(_y), vy(180), alive(true), type(t) {}

// [自定义成员函数] 更新道具下落。
void Item::Update(double dt) {
  // 道具只向下移动。
  y += vy * dt;

  // 掉出屏幕后标记为无效，之后统一清理。
  if (y > SCREEN_H + 80) {
    alive = false;
  }
}

// [自定义成员函数] 按道具类型绘制贴图或兜底方块。
void Item::Render(ResourceManager& rm) {
  IMAGE* img = nullptr;

  // 根据道具类型选择贴图。
  if (type == ItemType::Health) {
    img = rm.GetImage(AssetID::Item_Health);
  } else if (type == ItemType::Bomb) {
    img = rm.GetImage(AssetID::Item_Bomb);
  } else {
    img = rm.GetImage(AssetID::Item_Upgrade);
  }

  if (img) {
    // [自定义函数] DrawCenteredSprite 封装 EasyX 图片居中绘制。
    DrawCenteredSprite(x, y, img);
  } else {
    // 图片缺失时用不同颜色的小方块兜底。
    COLORREF c = RGB(120, 255, 160);
    if (type == ItemType::Bomb) {
      c = RGB(255, 210, 90);
    }
    if (type == ItemType::Upgrade) {
      c = RGB(120, 170, 255);
    }

    // [EasyX] setfillcolor/setlinecolor 设置兜底方块颜色。
    setfillcolor(c);
    setlinecolor(RGB(0, 0, 0));
    // [EasyX] solidroundrect/roundrect 绘制兜底圆角方块。
    solidroundrect((int)x - 18, (int)y - 18, (int)x + 18, (int)y + 18, 8, 8);
    roundrect((int)x - 18, (int)y - 18, (int)x + 18, (int)y + 18, 8, 8);
  }
}

// [自定义成员函数] 普通受伤并开启无敌时间。
void Player::Hurt(int damage) {
  // 无敌状态下不再扣血。
  // 这可以避免玩家碰到一串子弹时一帧内连续掉很多血。
  if (invincible) {
    return;
  }

  // 扣血并开启短暂无敌。
  hp -= damage;
  invincible = true;
  inv_time = 1.5;
}

// [自定义成员函数] Boss 特殊弹造成当前生命的一半伤害。
void Player::HurtByBossSpecialBomb() {
  // Boss 特殊弹也受无敌保护。
  if (invincible) {
    return;
  }

  // 生命只剩 1 时，特殊弹不再继续扣到 0。
  // 这样特殊弹偏向“重创”，真正击杀通常由普通子弹或碰撞完成。
  if (hp <= 1) {
    return;
  }

  // 当前生命向上取半作为伤害。
  // 例如 hp=5，则 damage=3；hp=4，则 damage=2。
  int damage = (hp + 1) / 2;
  hp -= damage;

  // 特殊弹最低保留 1 点生命。
  if (hp < 1) {
    hp = 1;
  }

  invincible = true;
  inv_time = 1.5;
}

// [自定义成员函数] 更新玩家移动、自动射击、无敌和强化计时。
void Player::Update(double dt) {
  // 键盘移动：WASD 和方向键都支持。
  // 注意移动量要乘 dt，保证不同帧率下移动速度一致。
  // [自定义函数] IsKeyDown 内部调用 [Windows API] GetAsyncKeyState。
  if (IsKeyDown('W') || IsKeyDown(VK_UP)) {
    y -= speed * dt;
  }
  if (IsKeyDown('S') || IsKeyDown(VK_DOWN)) {
    y += speed * dt;
  }
  if (IsKeyDown('A') || IsKeyDown(VK_LEFT)) {
    x -= speed * dt;
  }
  if (IsKeyDown('D') || IsKeyDown(VK_RIGHT)) {
    x += speed * dt;
  }

  // 限制玩家不跑出屏幕。
  // [自定义函数] clamp_val 把坐标限制在屏幕范围内。
  x = clamp_val(x, (double)w / 2.0, (double)SCREEN_W - w / 2.0);
  y = clamp_val(y, (double)h / 2.0, (double)SCREEN_H - h / 2.0);

  // 自动射击。
  // shootTimer 不断累加，超过 shootCooldown 就生成新子弹。
  shootTimer += dt;
  if (shootTimer >= shootCooldown) {
    if (upgraded) {
      // 强化状态发三发子弹，左右两发稍微错开位置。
      // [C++标准库容器] vector::emplace_back 直接构造 [自定义结构体] Bullet。
      bullets.emplace_back(x - 25, y - 50, 0, -1440, BulletKind::Player);
      bullets.emplace_back(x, y - 58, 0, -1440, BulletKind::Player);
      bullets.emplace_back(x + 25, y - 50, 0, -1440, BulletKind::Player);
    } else {
      // 普通状态发一发子弹。
      bullets.emplace_back(x, y - 50, 0, -1440, BulletKind::Player);
    }
    shootTimer = 0;
  }

  // 更新玩家所有子弹。
  for (auto& b : bullets) {
    b.Update(dt);
  }

  // 删除已经飞出屏幕或命中的子弹。
  // [C++标准库算法] std::remove_if 配合 [C++标准库容器] vector::erase
  // 删除元素。
  bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                               [](const Bullet& b) { return !b.alive; }),
                bullets.end());

  // 无敌倒计时。
  if (invincible) {
    inv_time -= dt;
    if (inv_time <= 0) {
      invincible = false;
      inv_time = 0;
    }
  }

  // 火力强化倒计时。
  if (upgraded) {
    upgrade_time -= dt;
    if (upgrade_time <= 0) {
      upgraded = false;
      upgrade_time = 0;
    }
  }
}

// [自定义成员函数] 绘制玩家和玩家子弹。
void Player::Render(ResourceManager& rm) {
  bool drawPlayer = true;

  // 无敌时闪烁：根据 inv_time 计算当前是否绘制玩家。
  // 视觉上就是“显示/隐藏/显示/隐藏”。
  if (invincible) {
    if (((int)(inv_time * 12)) % 2 == 0) {
      drawPlayer = false;
    }
  }

  if (drawPlayer) {
    IMAGE* img = rm.GetImage(AssetID::Player);

    if (img) {
      // [自定义函数] DrawCenteredSprite 封装 EasyX 图片居中绘制。
      DrawCenteredSprite(x, y, img);
    } else {
      // 图片缺失时，用简单三角形画一个飞机轮廓。
      // [EasyX] setfillcolor/setlinecolor/solidpolygon/polygon 绘制兜底飞机。
      setfillcolor(RGB(80, 180, 255));
      setlinecolor(RGB(0, 0, 0));
      POINT planePts[3] = {{(LONG)x, (LONG)(y - 48)},
                           {(LONG)(x - 42), (LONG)(y + 42)},
                           {(LONG)(x + 42), (LONG)(y + 42)}};
      solidpolygon(planePts, 3);
      polygon(planePts, 3);

      setfillcolor(RGB(220, 240, 255));
      solidcircle((int)x, (int)y, 12);
      circle((int)x, (int)y, 12);
    }
  }

  // 玩家子弹永远绘制，即使无敌闪烁时玩家本体暂时不画。
  for (auto& b : bullets) {
    b.Render(rm);
  }
}

// [自定义构造函数] 创建敌机并按类型初始化属性。
Enemy::Enemy(double _x, double _y, int t) : x(_x), y(_y), type(t) {
  // 防止传入奇怪 type：除了 0、1，其它都按 Boss 参数处理。
  int profileIndex = type == 0 ? 0 : (type == 1 ? 1 : 2);
  const EnemyProfile& profile = ENEMY_PROFILES[profileIndex];

  // 从参数表复制基础属性。
  w = profile.w;
  h = profile.h;
  hp = maxHp = profile.hp;
  speed = profile.speed;
  scoreValue = profile.score;
  shootCooldown = profile.shootCooldown;

  if (profileIndex == 2) {
    // Boss 专属初始参数：左右移动速度、特殊弹冷却、入场目标高度。
    vx = 200;
    bossSpecialCooldown = 1.80;
    bossTargetY = 165;
    bossEntered = false;
  }
}

// [自定义成员函数] 判断是否为 Boss。
bool Enemy::IsBoss() const { return type == 2; }

// [自定义成员函数] 根据 Boss 血量返回阶段。
int Enemy::Phase() const {
  if (!IsBoss()) {
    return 0;
  }
  if (hp <= maxHp / 4) {
    return 3;
  }
  if (hp <= maxHp / 2) {
    return 2;
  }
  return 1;
}

// [自定义成员函数] 敌机受伤并触发受击贴图计时。
void Enemy::TakeDamage(int damage) {
  // 扣血后设置 hitTimer，RenderBody 会在这段时间尝试使用受击贴图。
  hp -= damage;
  hitTimer = hitDuration;
}

// [自定义成员函数] 更新敌机移动、射击和子弹。
void Enemy::Update(double dt) {
  if (type == 2) {
    // Boss 第一阶段先从屏幕上方进入到目标高度。
    if (!bossEntered) {
      y += speed * dt;
      if (y >= bossTargetY) {
        y = bossTargetY;
        bossEntered = true;
      }
    } else {
      // 入场完成后左右移动，碰到屏幕边缘就反向。
      x += vx * dt;
      if (x < w / 2.0) {
        x = w / 2.0;
        // [C++标准库] std::abs 取绝对值，保证反弹后速度方向正确。
        vx = std::abs(vx);
      }
      if (x > SCREEN_W - w / 2.0) {
        x = SCREEN_W - w / 2.0;
        vx = -std::abs(vx);
      }
    }

    // 根据 Boss 当前阶段调整射击速度、特殊弹速度和移动速度。
    int phase = Phase();
    if (phase == 2) {
      shootCooldown = 0.38;
      bossSpecialCooldown = 1.45;
      if (std::abs(vx) < 260) {
        vx = vx < 0 ? -260 : 260;
      }
    } else if (phase == 3) {
      shootCooldown = 0.30;
      bossSpecialCooldown = 1.20;
      if (std::abs(vx) < 310) {
        vx = vx < 0 ? -310 : 310;
      }
    }

    // 用于阶段 3 的波浪弹幕。
    bossPulseTimer += dt;
  } else {
    // 普通敌机只向下移动。
    y += speed * dt;
    if (y > SCREEN_H + 120) {
      alive = false;
    }
  }

  // 受击贴图倒计时。
  if (hitTimer > 0) {
    hitTimer -= dt;
    if (hitTimer < 0) {
      hitTimer = 0;
    }
  }

  // 普通射击计时。
  shootTimer += dt;
  if (shootTimer >= shootCooldown) {
    if (type == 2) {
      if (bossEntered) {
        int phase = Phase();

        // Boss 基础三向弹。
        bullets.emplace_back(x, y + h / 2.0, 0, 370, BulletKind::Boss);
        bullets.emplace_back(x - 45, y + h / 2.0, -95, 350, BulletKind::Boss);
        bullets.emplace_back(x + 45, y + h / 2.0, 95, 350, BulletKind::Boss);

        if (phase >= 2) {
          // 阶段 2 增加两侧更散的子弹。
          bullets.emplace_back(x - 75, y + h / 2.0, -180, 330,
                               BulletKind::Boss);
          bullets.emplace_back(x + 75, y + h / 2.0, 180, 330, BulletKind::Boss);
        }

        if (phase >= 3) {
          // 阶段 3 增加左右摆动速度的波浪弹。
          // [C++标准库] std::sin 生成随时间变化的波浪速度。
          double wave = std::sin(bossPulseTimer * 4.2) * 115.0;
          bullets.emplace_back(x - 30, y + h / 2.0, wave, 370,
                               BulletKind::Boss);
          bullets.emplace_back(x + 30, y + h / 2.0, -wave, 370,
                               BulletKind::Boss);
        }
      }
    } else {
      // 普通敌机只向下发一颗子弹。
      bullets.emplace_back(x, y + h / 2.0, 0, 300, BulletKind::Enemy);
    }
    shootTimer = 0;
  }

  if (type == 2) {
    // Boss 特殊弹单独使用一个计时器，不和普通弹共用 shootTimer。
    bossSpecialTimer += dt;
    if (bossEntered && bossSpecialTimer >= bossSpecialCooldown) {
      int phase = Phase();

      // spread 决定从 -spread 到 +spread 一共发多少颗特殊弹。
      int spread = phase == 1 ? 2 : (phase == 2 ? 3 : 4);
      double baseVy = phase == 3 ? 400 : 360;
      double stepVx = phase == 3 ? 85.0 : 75.0;

      for (int i = -spread; i <= spread; ++i) {
        // i * stepVx 让特殊弹从左到右散开。
        bullets.emplace_back(x, y + h / 2.0, i * stepVx, baseVy,
                             BulletKind::BossSpecial);
      }

      bossSpecialTimer = 0;
    }
  }

  // 更新敌人自己发射的子弹。
  for (auto& b : bullets) {
    b.Update(dt);
  }

  // 删除无效子弹。
  // [C++标准库算法] std::remove_if 配合 [C++标准库容器] vector::erase
  // 删除元素。
  bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                               [](const Bullet& b) { return !b.alive; }),
                bullets.end());
}

// [自定义成员函数] 获取敌机普通贴图。
IMAGE* Enemy::GetNormalImage(ResourceManager& rm) {
  if (type == 0) {
    return rm.GetImage(AssetID::Enemy1);
  }
  if (type == 1) {
    return rm.GetImage(AssetID::Enemy2);
  }
  return rm.GetImage(AssetID::Boss);
}

// [自定义成员函数] 获取敌机受击贴图。
IMAGE* Enemy::GetHitImage(ResourceManager& rm) {
  if (type == 1) {
    return rm.GetImage(AssetID::Enemy2_Hit);
  }
  if (type == 2) {
    return rm.GetImage(AssetID::Boss_Hit);
  }
  return nullptr;
}

// [自定义成员函数] 绘制敌机本体，不绘制子弹。
void Enemy::RenderBody(ResourceManager& rm) {
  IMAGE* img = nullptr;

  // 如果正在受击，并且这个敌人有受击贴图，则优先画受击贴图。
  if (hitTimer > 0) {
    img = GetHitImage(rm);
  }

  // 没有受击贴图或不在受击状态时，画普通贴图。
  if (!img) {
    img = GetNormalImage(rm);
  }

  if (img) {
    // [自定义函数] DrawCenteredSprite 封装 EasyX 图片居中绘制。
    DrawCenteredSprite(x, y, img);
  } else {
    // 图片缺失时，用不同颜色的圆角矩形区分敌人类型。
    if (type == 0) {
      setfillcolor(RGB(255, 120, 120));
    } else if (type == 1) {
      setfillcolor(RGB(255, 160, 90));
    } else {
      setfillcolor(RGB(210, 90, 255));
    }

    setlinecolor(RGB(0, 0, 0));
    solidroundrect((int)x - w / 2, (int)y - h / 2, (int)x + w / 2,
                   (int)y + h / 2, 10, 10);
    roundrect((int)x - w / 2, (int)y - h / 2, (int)x + w / 2, (int)y + h / 2,
              10, 10);
  }
}

// [自定义成员函数] 绘制敌机本体和敌机子弹。
void Enemy::Render(ResourceManager& rm) {
  // 先画敌机本体，再画它发射的子弹。
  RenderBody(rm);
  for (auto& b : bullets) {
    b.Render(rm);
  }
}
