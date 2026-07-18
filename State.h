
#pragma once

// [自定义结构体] 这里只需要声明“有一个 Game 结构体”，不需要包含完整 Game.h。
// 这种写法叫前向声明，可以减少头文件互相包含。
struct Game;

// [自定义基类] 游戏状态基类。
struct State {
  // 每个状态都保存一个 [自定义结构体] Game 指针。
  // 通过它可以访问玩家、敌人、资源，也可以调用 [自定义函数] game->ChangeState
  // 切换状态。 [自定义结构体] Game。
  Game* game = nullptr;

  // [C++语法] explicit 表示不能用 Game* 隐式转换成 State。
  // 构造状态时必须明确写 [自定义基类] State(g) 或派生状态构造函数。
  explicit State(Game* g) : game(g) {}

  // [C++语法] 基类析构函数写成 virtual，保证通过 State* 删除派生类对象时，
  // 派生类析构逻辑也能被正确调用。
  virtual ~State() = default;

  // [自定义虚函数] 状态进入时调用。
  // 常用于初始化输入状态、播放音乐、重置动画计时等。
  virtual void Enter() {}

  // [自定义虚函数] 状态退出时调用。
  // 常用于停止音乐或保存状态内临时信息。
  virtual void Exit() {}

  // [自定义纯虚函数] 按帧更新状态逻辑。
  // dt 是距离上一帧的秒数，移动和计时都应该乘以 dt。
  // [C++语法] = 0 表示纯虚函数，派生状态必须实现。
  virtual void Update(double dt) = 0;

  // [自定义纯虚函数] 绘制当前状态。
  // 每个状态自己决定画什么界面。
  virtual void Render() = 0;
};
