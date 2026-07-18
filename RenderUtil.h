// 渲染辅助声明：提供透明去黑边绘制、背景等比铺满和 UTF-8 文本输出。
//
// [EasyX] 可以直接画图片和文字，但本项目遇到几个实际问题：
// 1. 有些 PNG 素材外侧不是透明，而是黑色背景，需要去掉黑边。
// 2. 背景图需要等比铺满窗口，不能随便拉伸变形。
// 3. 源码中的中文字符串是 UTF-8，直接用 [EasyX] 普通文本函数可能乱码。
//
// 所以把这些细节集中封装到 RenderUtil.*。
// 这样状态和实体只需要调用“画什么”，不用每次关心底层怎么画。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的渲染辅助函数。
// [EasyX] 表示 EasyX 图形库提供的函数或类型。
// [Windows API] 表示 Windows GDI/API 提供的函数或类型。
#pragma once

#include <graphics.h>

// [自定义函数] 绘制图片并去掉外侧黑色背景。
// drawX/drawY 是图片左上角坐标。
// 参数 img 是 [EasyX类型] IMAGE*。
void DrawSpriteWithoutOuterBlack(int drawX, int drawY, IMAGE *img);

// [自定义函数] 居中绘制去黑边图片。
// cx/cy 是图片中心点，适合用来绘制飞机、子弹、道具。
void DrawCenteredSprite(double cx, double cy, IMAGE *img);

// [自定义函数] 居中旋转 180 度绘制图片。
// Boss 特殊弹使用道具图片反向绘制时会用到。
void DrawCenteredSpriteRot180(double cx, double cy, IMAGE *img);

// [自定义函数] 等比裁剪铺满目标区域。
// 类似网页 CSS 的 background-size: cover。
// 内部使用 [Windows API] StretchBlt。
void DrawImageCover(IMAGE *img, int dstX, int dstY, int dstW, int dstH);

// [自定义函数] 计算 UTF-8 文本宽度。
// 画居中文字、按钮文字时需要先知道文本宽度。
int TextWidthUtf8(const char *text);

// [自定义函数] 计算 UTF-8 文本高度。
int TextHeightUtf8(const char *text);

// [自定义函数] 输出 UTF-8 文本。
// 内部使用 [Windows API] TextOutW。
void OutTextUtf8(int x, int y, const char *text);

// [自定义函数] 输出带阴影的 UTF-8 文本。
// 先用 shadowColor 偏移绘制一次，再用 textColor 在原位置绘制一次。
void OutTextUtf8Shadow(int x, int y, const char *text,
                       COLORREF textColor = RGB(255, 255, 255),
                       COLORREF shadowColor = RGB(0, 0, 0), int offset = 2);

// [自定义函数] 设置黑色文本样式。
// 内部调用 [EasyX] setbkmode/settextcolor/settextstyle。
void SetBlackText(int height, const char *face = "Microsoft YaHei");

// [自定义函数] 设置白色文本样式。
void SetWhiteText(int height, const char *face = "Microsoft YaHei");
