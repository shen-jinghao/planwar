
#pragma once

#include "State.h"

enum class SaveSlotMode { LoadOnly,
                          Manage };
enum class SaveSlotReturn { MainMenu,
                            Pause,
                            Play,
                            GameOver };
struct MainMenuState : State {
  bool enterPrev = false;
  bool escPrev = false;
  bool lbuttonPrev = false;
  bool creditsActive = false;
  double creditsY = 0;
  explicit MainMenuState(Game* g) : State(g) {}
  void Enter() override;
  void Update(double dt) override;
  void Render() override;
};
struct SaveSlotState : State {
  // 当前存档页是“只读档”还是“保存+读取”。
  SaveSlotMode mode = SaveSlotMode::LoadOnly;

  // 按返回键/返回按钮时应该回到哪个状态。
  SaveSlotReturn returnTarget = SaveSlotReturn::MainMenu;

  // 鼠标和 Esc 的上一帧状态，用于边沿触发。
  bool lbuttonPrev = false;
  bool escPrev = false;

  SaveSlotState(Game* g, SaveSlotMode m, SaveSlotReturn r)
      : State(g), mode(m), returnTarget(r) {}

  // [自定义重写函数] 初始化存档页输入状态。
  void Enter() override;
  // [自定义重写函数] 处理保存、读取和返回。
  void Update(double dt) override;
  // [自定义重写函数] 绘制三个存档槽。
  void Render() override;
};

// [自定义状态类] 暂停菜单状态，继承自 [自定义基类] State。
struct PauseState : State {
  // 进入暂停状态时，如果 P 键还没松开，先等待松开。
  // 否则刚按 P 进入暂停后，下一帧又可能因为 P 仍按下立刻继续游戏。
  bool waitReleaseP = true;
  // 鼠标左键上一帧状态。
  bool lbuttonPrev = false;
  explicit PauseState(Game* g) : State(g) {}
  // [自定义重写函数] 初始化暂停输入锁。
  void Enter() override;
  // [自定义重写函数] 处理继续、存档和返回菜单。
  void Update(double dt) override;
  // [自定义重写函数] 绘制暂停覆盖层。
  void Render() override;
};

// [自定义状态类] 游戏结束状态，继承自 [自定义基类] State。
struct GameOverState : State {
  // Enter 和鼠标左键上一帧状态。
  bool enterPrev = false;
  bool lbuttonPrev = false;

  explicit GameOverState(Game* g) : State(g) {}

  // [自定义重写函数] 播放结束音效并初始化输入。
  void Enter() override;
  // [自定义重写函数] 处理重新开始、读档和回主菜单。
  void Update(double dt) override;
  // [自定义重写函数] 绘制游戏结束界面。
  void Render() override;
};

// [自定义状态类] 玩家死亡演出状态，继承自 [自定义基类] State。
struct PlayerDyingState : State {
  // 已经播放了多久。
  double timer = 0;

  // 死亡演出总时长，时间到后进入 [自定义状态类] GameOverState。
  double duration = 0.8;

  explicit PlayerDyingState(Game* g) : State(g) {}

  // [自定义重写函数] 创建玩家爆炸效果。
  void Enter() override;
  // [自定义重写函数] 更新死亡演出计时。
  void Update(double dt) override;
  // [自定义重写函数] 绘制无玩家的世界。
  void Render() override;
};

// [自定义状态类] 局内游玩状态，继承自 [自定义基类] State。
struct PlayState : State {
  // P、B、鼠标左右键的上一帧状态。
  // 用于区分“按住不放”和“刚按下”。
  bool pPrev = false;
  bool bPrev = false;
  bool lbuttonPrev = false;

  explicit PlayState(Game* g) : State(g) {}

  // [自定义重写函数] 进入局内并播放背景音乐。
  void Enter() override;
  // [自定义重写函数] 离开局内时停止背景音乐。
  void Exit() override;
  // [自定义重写函数] 更新输入、刷怪、碰撞、道具和清理。
  void Update(double dt) override;
  // [自定义重写函数] 绘制世界、HUD 和局内按钮。
  void Render() override;
};
