// 通用基础定义：集中放置窗口尺寸、缩放工具、常用数学/输入/颜色判断函数。
//
// 这个文件可以理解成“全项目都会用到的小工具箱”。
// 例如：
// - 窗口到底多大？
// - UI 按钮在不同窗口大小下怎么缩放？
// - 鼠标点是否点中了按钮？
// - 两个矩形是否碰撞？
// - 某个键现在有没有被按下？
//
// 把这些小工具放在 Common.* 中，可以避免每个文件重复写同样的代码。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的类、结构体、函数或常量。
// [Windows API] 表示 Windows 系统 API 或 Windows 类型。
// [C++标准库] 表示 C++ 标准库提供的类型或函数。
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <string>

// [自定义常量] DESIGN_W / DESIGN_H 表示 UI 设计稿的参考尺寸。
// 代码里很多按钮位置先按 1920x1080 写，再通过 ScaleX/ScaleY 缩放到实际窗口。
constexpr int DESIGN_W = 1920;
constexpr int DESIGN_H = 1080;

// [自定义常量] SCREEN_W / SCREEN_H 是当前 EasyX 窗口实际大小。
// 如果以后想调窗口大小，优先改这两个值。
constexpr int SCREEN_W = 1600;
constexpr int SCREEN_H = 900;

// [自定义常量] OUTER_BLACK_TOLERANCE 是判断“接近黑色”时允许的误差。
// 图片素材中黑色背景不一定是纯 RGB(0,0,0)，所以用一个容差值判断近黑色。
constexpr int OUTER_BLACK_TOLERANCE = 32;

// [自定义常量] UI_SCALE 是高度缩放比例。
// 例如设计稿高度 1080，实际窗口高度 900，则比例是 900/1080。
constexpr double UI_SCALE = (double)SCREEN_H / (double)DESIGN_H;

// [自定义函数] 将设计稿 X 坐标缩放到当前窗口。
// constexpr 表示这个函数在参数是常量时可以在编译期计算。
constexpr int ScaleX(int value) {
  return (value * SCREEN_W + DESIGN_W / 2) / DESIGN_W;
}

// [自定义函数] 将设计稿 Y 坐标缩放到当前窗口。
constexpr int ScaleY(int value) {
  return (value * SCREEN_H + DESIGN_H / 2) / DESIGN_H;
}

// [自定义函数] 缩放长度，默认按高度比例处理。
// 字体、按钮高度、圆角等视觉尺寸通常按高度缩放更稳定。
constexpr int ScaleLen(int value) { return ScaleY(value); }

// [自定义函数] 缩放字体高度。
constexpr int ScaleFont(int value) { return ScaleY(value); }

// [自定义模板函数] 将数值限制在指定范围内。
//
// 例子：
//   clamp_val(10, 0, 5)  -> 5
//   clamp_val(-2, 0, 5)  -> 0
//   clamp_val(3, 0, 5)   -> 3
//
// 玩家移动时用它保证飞机不会跑出屏幕。
template <typename T>
inline T clamp_val(T v, T lo, T hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

// 判断文件是否存在。
// [自定义函数] FileExists 使用 [C++标准库] std::ifstream 实现。
bool FileExists(const std::string &path);
// 判断点是否落在中心点矩形内。
//
// 本项目大量使用“中心点 + 宽高”的方式描述物体：
// cx/cy 是矩形中心，w/h 是宽和高。
// 子弹命中敌机、鼠标点中按钮时会用这个函数。
// [自定义函数] PointInRect。
bool PointInRect(double px, double py, double cx, double cy, int w, int h);
// 判断两个中心点矩形是否重叠。
//
// 玩家飞机和敌机本体碰撞时，用两个中心点矩形做粗略碰撞检测。
// [自定义函数] RectOverlap。
bool RectOverlap(double ax, double ay, int aw, int ah, double bx, double by,
                 int bw, int bh);
// 判断指定虚拟键是否按下。
//
// vk 是 Windows 虚拟键码，例如：
// - 'W' 表示 W 键
// - VK_UP 表示方向键上
// - VK_LBUTTON 表示鼠标左键
// [自定义函数] IsKeyDown 内部调用 [Windows API] GetAsyncKeyState。
bool IsKeyDown(int vk);
// 判断颜色是否接近黑色。
// RenderUtil.cpp 的图片去黑边算法会用到它。
// [自定义函数] IsNearBlack 使用 [Windows API/类型] COLORREF 和 RGB 通道宏。
bool IsNearBlack(COLORREF c, int tolerance = OUTER_BLACK_TOLERANCE);
