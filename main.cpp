// 程序入口：初始化 [EasyX] 窗口、创建 [自定义结构体] Game 和主菜单状态，并驱动主循环。
//
// 对刚开始看项目的同学，可以把 main.cpp 理解成“游戏发动机的启动按钮”：
// 1. 先创建窗口。
// 2. 创建一个 [自定义结构体] Game 对象保存全局数据。
// 3. 进入 [自定义状态类] MainMenuState。
// 4. 在 while 循环里一帧一帧地更新逻辑、绘制画面。
// 5. 游戏退出时释放图片、停止音乐、关闭窗口。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的类、结构体、函数或常量。
// [EasyX] 表示 EasyX 图形库提供的函数或类型。
// [Windows API] 表示 Windows 系统 API。
// [C++标准库] 表示 C++ 标准库提供的类型或函数。
#include <graphics.h>
#include <windows.h>

#include <chrono>
#include <cstdlib>
#include <ctime>

#include "Common.h"
#include "Game.h"
#include "Resources.h"
#include "States.h"

// 游戏主入口。
int main() {
  // [C++标准库] srand 用当前时间初始化随机数种子。
  // 后面生成敌机位置、掉落道具概率时会用 [C++标准库] rand()，如果不初始化，
  // 每次启动游戏随机结果会比较固定。
  srand((unsigned)time(nullptr));

  // [EasyX] initgraph 是窗口创建函数。
  // [自定义常量] SCREEN_W 和 SCREEN_H 定义在 Common.h 中，表示当前游戏窗口大小。
  initgraph(SCREEN_W, SCREEN_H);

  // [EasyX] setbkcolor 设置背景色，[EasyX] cleardevice 清空画布。
  // 后续每一帧通常会重新绘制背景图片，
  // 这里主要保证窗口刚创建时不是未初始化画面。
  setbkcolor(RGB(245, 245, 245));
  cleardevice();

  // [EasyX] BeginBatchDraw 开启批量绘图：先把所有绘制操作画到后台缓冲区，
  // 再通过 [EasyX] FlushBatchDraw 一次性显示出来，减少画面闪烁。
  BeginBatchDraw();

  // [自定义结构体] Game 是整个游戏的“上下文”，里面保存玩家、敌人、资源、当前状态等。
  Game game;

  // 游戏启动后先进入主菜单。
  // [自定义状态类] MainMenuState 创建一个主菜单状态，并把 [自定义结构体] Game 指针交给它。
  // 状态内部需要通过这个指针访问资源、切换状态、读取分数等。
  // [自定义函数] ChangeState 负责切换当前状态。
  game.ChangeState(new MainMenuState(&game));

  // 用 [C++标准库] std::chrono::steady_clock 记录上一帧的时间，用来计算 dt。
  // dt 表示“这一帧距离上一帧过了多少秒”。
  auto last = std::chrono::steady_clock::now();

  // 主循环：只要 game.running 仍为 true，游戏就继续运行。
  // 任何状态想退出游戏，只需要把 game.running 改成 false。
  while (game.running) {
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - last).count();
    last = now;

    // 防止某一帧卡顿太久导致 dt 巨大。
    // 如果不限制，玩家/敌机可能会因为“速度 * dt”一下子移动很远。
    if (dt > 0.05) {
      dt = 0.05;
    }

    // [自定义成员] current 是当前状态，例如主菜单、局内游戏、暂停界面等。
    // 每一帧都先调用 [自定义虚函数] Update 更新逻辑，再调用 [自定义虚函数] Render 绘制画面。
    if (game.current) {
      game.current->Update(dt);
      game.current->Render();
    }

    // [EasyX] FlushBatchDraw 把后台缓冲区的画面刷新到屏幕上。
    FlushBatchDraw();

    // [Windows API] Sleep 稍微休眠一下，避免 while 循环占满 CPU。
    Sleep(10);
  }

  // 主循环结束后，释放图片资源并停止音乐。
  // 这里属于程序退出前的清理工作。
  // [自定义函数] UnloadAll 释放图片缓存；[自定义函数] StopBackgroundMusic 停止音乐。
  game.resources.UnloadAll();
  StopBackgroundMusic();

  // [EasyX] EndBatchDraw 关闭批量绘图；[EasyX] closegraph 关闭 EasyX 窗口。
  EndBatchDraw();
  closegraph();

  return 0;
}
