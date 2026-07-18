// 游戏实体声明：包含动画效果、子弹、掉落道具、玩家和敌人。
//
// “实体”就是游戏画面中会移动、会被绘制、可能会发生碰撞的对象。
// 本文件只写结构体中有哪些数据、有哪些函数；真正的行为实现在
// Entities.cpp 中。
//
// 读这个文件时建议先看成员变量：
// - x/y 通常表示对象中心点坐标。
// - w/h 通常表示对象碰撞用的宽高。
// - alive 表示对象是否还活着，后续清理时会删掉 alive=false 的对象。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的结构体、枚举或函数。
// [EasyX] 表示 EasyX 图形库提供的类型。
// [C++标准库] 表示 C++ 标准库提供的容器或类型。
#pragma once

#include <graphics.h>

#include <vector>

#include "Common.h"
#include "Resources.h"

// [自定义结构体] 一次性动画效果。
struct AnimationEffect {
  // 动画中心点坐标。爆炸效果会以这个点为中心绘制。
  double x = 0;
  double y = 0;

  // 当前动画已经播放了多久，单位是秒。
  double timer = 0;

  // 每一帧图片持续多久。数值越小，动画播放越快。
  double frameInterval = 0.08;

  // 如果没有图片帧，就用圆圈画一个兜底动画，这里是兜底动画总时长。
  double fallbackDuration = 0.45;

  // 动画是否仍然需要保留。
  bool alive = true;

  // 兜底圆形爆炸效果的最大半径。
  int fallbackRadius = 60;

  // 爆炸动画的图片帧。
  // 注意：这里不拥有图片内存，图片由 ResourceManager 统一管理。
  // [C++标准库容器] std::vector 保存多个 [EasyX类型] IMAGE*。
  std::vector<IMAGE *> frames;

  AnimationEffect() = default;
  AnimationEffect(double _x, double _y, const std::vector<IMAGE *> &_frames,
                  double _frameInterval = 0.08, int _fallbackRadius = 60);

  // [自定义函数] 推进动画时间。
  void Update(double dt);
  // [自定义函数] 返回动画是否仍存在。
  bool Alive() const;
  // [自定义函数] 绘制当前动画帧。
  void Render();
};

// [自定义枚举] 子弹来源/类型。
//
// 枚举可以让代码比直接写数字更清楚。
// 例如 [自定义枚举值] BulletKind::Player 比 0 更容易看懂。
enum class BulletKind { Player,
                        Enemy,
                        Boss,
                        BossSpecial };

// [自定义结构体] 子弹实体。
struct Bullet {
  // 子弹中心点坐标。
  double x = 0;
  double y = 0;

  // 子弹速度。vx 是横向速度，vy 是纵向速度，单位是像素/秒。
  // 玩家子弹默认向上飞，所以 vy 是负数。
  double vx = 0;
  double vy = -1440;

  // 子弹是否还存在。飞出屏幕后会被标记为 false。
  bool alive = true;

  // 子弹类型，决定绘制贴图和伤害处理方式。
  BulletKind kind = BulletKind::Player;

  Bullet(double _x = 0, double _y = 0, double _vx = 0, double _vy = -1440,
         BulletKind _kind = BulletKind::Player);

  // [自定义函数] 按速度移动并检测出界。
  void Update(double dt);
  // [自定义函数] 按类型绘制子弹。
  // 参数 rm 是 [自定义结构体] ResourceManager。
  void Render(ResourceManager &rm);
};

// [自定义枚举] 道具类型。
enum class ItemType { Health,
                      Bomb,
                      Upgrade };

// [自定义结构体] 掉落道具实体。
struct Item {
  // 道具中心点坐标。
  double x = 0;
  double y = 0;

  // 道具下落速度，单位是像素/秒。
  double vy = 180;

  // 道具是否还在场上。
  bool alive = true;

  // 道具类型，决定拾取后的效果。
  ItemType type = ItemType::Health;

  Item(double _x = 0, double _y = 0, ItemType t = ItemType::Health);

  // [自定义函数] 下落并检测出界。
  void Update(double dt);
  // [自定义函数] 按类型绘制道具。
  // 参数 rm 是 [自定义结构体] ResourceManager。
  void Render(ResourceManager &rm);
};

// [自定义结构体] 玩家飞机。
struct Player {
  // 玩家飞机中心点坐标。
  // 默认放在屏幕水平中间、靠近屏幕底部的位置。
  double x = SCREEN_W / 2.0;
  double y = SCREEN_H * 0.83;

  // 碰撞用宽高，不一定等于图片真实宽高。
  int w = 96;
  int h = 96;

  // hp 是当前生命，maxHp 是生命上限。
  int hp = 3;
  int maxHp = 5;

  // 炸弹数量。炸弹可以清理普通敌人或重创 Boss。
  int bombs = 0;

  // 是否处于火力强化状态。
  // 火力强化时一次发射三发子弹。
  bool upgraded = false;

  // 火力强化剩余时间，单位是秒。
  double upgrade_time = 0;

  // 是否处于无敌状态。
  // 受伤后短暂无敌，避免一瞬间被多颗子弹连续扣血。
  bool invincible = false;

  // 无敌剩余时间，单位是秒。
  double inv_time = 0;

  // 玩家移动速度，单位是像素/秒。
  double speed = 720;

  // 射击冷却时间。0.18 表示大约每 0.18 秒发射一次。
  double shootCooldown = 0.18;

  // 距离上一次发射已经过了多久。
  double shootTimer = 0;

  // 玩家发射出去、仍在场上的子弹。
  // [C++标准库容器] std::vector 保存多个 [自定义结构体] Bullet。
  std::vector<Bullet> bullets;

  // [自定义函数] 普通受伤并进入短暂无敌。
  void Hurt(int damage = 1);
  // [自定义函数] 处理 Boss 特殊弹伤害。
  void HurtByBossSpecialBomb();
  // [自定义函数] 更新移动、射击和状态计时。
  void Update(double dt);
  // [自定义函数] 绘制玩家和玩家子弹。
  void Render(ResourceManager &rm);
};

// [自定义结构体] 敌机和 Boss。
struct Enemy {
  // 敌机中心点坐标。
  double x = 0;
  double y = 0;

  // 碰撞用宽高。
  int w = 40;
  int h = 40;

  // 当前生命和最大生命。
  int hp = 1;
  int maxHp = 1;

  // 向下移动速度或 Boss 入场速度，单位是像素/秒。
  double speed = 220;

  // 击杀后给玩家增加的分数。
  int scoreValue = 100;

  // 是否仍在场上。
  bool alive = true;

  // 敌人射击计时和射击冷却。
  double shootTimer = 0;
  double shootCooldown = 2;

  // 敌人类型：
  // 0 = 小敌机，1 = 中敌机，2 = Boss。
  // 本项目没有为 Boss 单独写一个类，而是用 type 区分行为。
  int type = 0;

  // 受击后短暂显示受击贴图。
  // hitTimer > 0 时表示正在受击闪烁。
  double hitTimer = 0;
  double hitDuration = 0.12;

  // Boss 专用字段：横向速度。
  double vx = 0;

  // Boss 是否已经完成入场。
  // Boss 刚生成时从屏幕上方进入，到达 bossTargetY 后才开始左右移动和放弹。
  bool bossEntered = false;
  double bossTargetY = 165;

  // Boss 特殊弹计时和冷却。
  double bossSpecialTimer = 0;
  double bossSpecialCooldown = 1.20;

  // Boss 弹幕中用到的波动计时器。
  double bossPulseTimer = 0;

  // 敌人发射出去、仍在场上的子弹。
  // [C++标准库容器] std::vector 保存多个 [自定义结构体] Bullet。
  std::vector<Bullet> bullets;

  Enemy(double _x = 0, double _y = 0, int t = 0);

  // [自定义函数] 判断是否为 Boss。
  bool IsBoss() const;
  // [自定义函数] 返回 Boss 当前阶段。
  int Phase() const;
  // [自定义函数] 扣血并触发受击闪烁。
  void TakeDamage(int damage);
  // [自定义函数] 更新移动、射击和子弹。
  void Update(double dt);
  // [自定义函数] 获取普通贴图。
  // 返回值是 [EasyX类型] IMAGE*。
  IMAGE *GetNormalImage(ResourceManager &rm);
  // [自定义函数] 获取受击贴图。
  // 返回值是 [EasyX类型] IMAGE*。
  IMAGE *GetHitImage(ResourceManager &rm);
  // [自定义函数] 只绘制敌机本体。
  void RenderBody(ResourceManager &rm);
  // [自定义函数] 绘制敌机和它的子弹。
  void Render(ResourceManager &rm);
};
