// 通用基础函数实现：提供文件检测、矩形碰撞、键盘状态和近黑色判断。
//
// 这些函数都尽量保持“无状态”：
// 函数只根据传入参数和系统当前状态返回结果，不保存自己的长期数据。
// 这样任何模块都可以放心调用，不容易产生隐藏副作用。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的函数或常量。
// [Windows API] 表示 Windows 系统 API 或宏。
// [C++标准库] 表示 C++ 标准库提供的类型或函数。
#include "Common.h"

#include <cmath>
#include <fstream>

// [自定义函数] 判断文件是否存在。
bool FileExists(const std::string &path) {
  // 以二进制方式尝试打开文件。
  // [C++标准库] std::ifstream 是 C++ 的文件输入流。
  // 如果打开成功，good() 通常返回 true，说明文件存在且可访问。
  std::ifstream f(path, std::ios::binary);
  return f.good();
}

// [自定义函数] 判断点是否落在中心点矩形内。
bool PointInRect(double px, double py, double cx, double cy, int w, int h) {
  // 中心点矩形的左边界是 cx - w/2，右边界是 cx + w/2。
  // y 方向同理。只要点的 x/y 都在范围内，就说明点在矩形里。
  return px >= cx - w / 2.0 && px <= cx + w / 2.0 && py >= cy - h / 2.0 &&
         py <= cy + h / 2.0;
}

// [自定义函数] 判断两个中心点矩形是否重叠。
bool RectOverlap(double ax, double ay, int aw, int ah, double bx, double by,
                 int bw, int bh) {
  // 两个中心点的水平距离 * 2 小于两者宽度之和，说明 x 方向重叠。
  // y 方向也重叠时，两个矩形才算真正碰撞。
  return std::abs(ax - bx) * 2.0 < (aw + bw) &&
         std::abs(ay - by) * 2.0 < (ah + bh);
}

// [自定义函数] 判断指定虚拟键是否处于按下状态。
//
// [Windows API] GetAsyncKeyState 用于获取按键当前状态。
// 返回值的最高位为 1 时，表示这个键当前处于按下状态。
bool IsKeyDown(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

// [自定义函数] 判断颜色是否接近黑色。
bool IsNearBlack(COLORREF c, int tolerance) {
  // [Windows 类型] COLORREF 中可以取出红、绿、蓝三个通道。
  // [Windows 宏] GetRValue/GetGValue/GetBValue 分别取 RGB 通道。
  // 三个通道都很小，就认为它接近黑色。
  return GetRValue(c) <= tolerance && GetGValue(c) <= tolerance &&
         GetBValue(c) <= tolerance;
}
