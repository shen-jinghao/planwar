// 游戏全局状态声明：保存资源、当前状态、世界实体、分数进度和存档接口。
//
// [自定义结构体] Game 是整个项目中最重要的“上下文对象”。
// 你可以把它理解成一个总账本：
// - 当前在哪个状态？
// - 玩家现在有多少血、多少分？
// - 场上有哪些敌人、道具、爆炸动画？
// - 图片资源从哪里拿？
// - 要不要生成 Boss？
//
// 每个 [自定义基类] State 都保存 [自定义结构体] Game*，
// 所以状态想访问全局信息时都会通过 game->xxx。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的类、结构体、函数或常量。
// [EasyX] 表示 EasyX 图形库提供的类型或函数。
// [C++标准库] 表示 C++ 标准库提供的类型。
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Entities.h"
#include "State.h"

// [自定义结构体] 存档文件中的可恢复数据。
struct GameSaveData {
  // 直接保存 [自定义结构体] Player 对象中的核心数据。
  // 注意读档时会清空 player.bullets，所以不会恢复已经飞出去的子弹。
  Player player;

  // 分数、击杀数和关卡进度。
  int score = 0;
  int kills = 0;
  int level = 1;

  // 当前背景编号。
  int backgroundIndex = 0;

  // 下一次 Boss 出现需要达到的击杀数。
  int nextBossKills = 20;

  // 普通敌机刷新计时器和刷新间隔。
  double spawnTimer = 0;
  double spawnInterval = 1.2;
};

// [自定义函数] 从磁盘读取存档数据。
bool ReadGameSaveData(const std::string &path, GameSaveData &data);

// [自定义结构体] 游戏上下文。
struct Game {
  // [自定义结构体] ResourceManager 图片资源管理器。
  // 所有状态和实体绘图时都通过它获取 [EasyX类型] IMAGE*。
  ResourceManager resources;

  // 当前状态。[C++标准库] unique_ptr 表示 [自定义结构体] Game 独占这个状态对象，
  // 切换状态时旧状态会自动释放。
  std::unique_ptr<State> current;

  // 主循环开关。main.cpp 中 while(game.running) 会检查它。
  bool running = true;

  // [自定义结构体] Player 玩家对象。
  Player player;

  // 当前场上的敌人，包括普通敌机和 Boss。
  // [C++标准库容器] std::vector 保存多个 [自定义结构体] Enemy。
  std::vector<Enemy> enemies;

  // 当前场上的道具。
  // [C++标准库容器] std::vector 保存多个 [自定义结构体] Item。
  std::vector<Item> items;

  // 当前正在播放的爆炸等动画效果。
  // [C++标准库容器] std::vector 保存多个 [自定义结构体] AnimationEffect。
  std::vector<AnimationEffect> effects;

  // 游戏进度数据。
  int score = 0;
  int kills = 0;
  int level = 1;

  // 当前选择的背景编号，0/1/2 对应三张背景图。
  int backgroundIndex = 0;

  // 刷怪计时器和刷怪间隔。
  double spawnTimer = 0;
  double spawnInterval = 1.2;

  // 下一次生成 Boss 的击杀数门槛。
  int nextBossKills = 20;

  // 临时提示文本，例如“1号存档已保存”。
  // [C++标准库] std::string 表示可变长度字符串。
  std::string noticeText;

  // 提示文本剩余显示时间。
  double noticeTimer = 0;

  // [自定义函数] 切换当前状态。
  void ChangeState(State *s);
  // [自定义函数] 重置一局新游戏。
  void ResetGame();
  // [自定义函数] 保存游戏进度。
  bool SaveGame(const std::string &path = "save.dat") const;
  // [自定义函数] 读取游戏进度。
  bool LoadGame(const std::string &path = "save.dat");

  // [自定义函数] 显示临时提示文本。
  void ShowNotice(const std::string &text, double duration = 2.0);
  // [自定义函数] 更新提示文本计时。
  void UpdateNotice(double dt);

  // [自定义函数] 判断当前是否存在 Boss。
  bool BossExists() const;
  // [自定义函数] 获取 Boss，只读版本。
  const Enemy *GetBoss() const;
  // [自定义函数] 获取 Boss，可修改版本。
  Enemy *GetBoss();

  // [自定义函数] 生成普通敌机。
  void SpawnEnemy();
  // [自定义函数] 生成 Boss。
  void SpawnBoss();

  // [自定义函数] 添加敌机爆炸动画。
  void AddEnemyExplosion(const Enemy &e);
  // [自定义函数] 添加玩家死亡动画。
  void AddPlayerDeathExplosion();
  // [自定义函数] 击杀敌机并结算分数/掉落。
  void KillEnemy(Enemy &e, bool giveScore = true);

  // [自定义函数] 更新所有动画效果。
  void UpdateEffects(double dt);

  // [自定义函数] 绘制当前背景。
  void RenderBackground();
  // [自定义函数] 绘制世界实体。
  void RenderWorld(bool renderPlayer = true);
  // [自定义函数] 绘制 Boss 血条。
  void RenderBossHpBar();
};
