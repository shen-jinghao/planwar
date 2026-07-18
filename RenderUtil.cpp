// 渲染辅助实现：封装 [EasyX] 图片像素绘制、背景缩放和 UTF-8 文本输出。
// 主要解决素材黑边、背景适配窗口、中文文本输出和文本阴影这些底层问题。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的渲染辅助函数。
// [EasyX] 表示 EasyX 图形库提供的类型或函数。
// [Windows API] 表示 Windows GDI/API 提供的函数或类型。
// [C++标准库] 表示 C++ 标准库提供的容器或类型。
#include "RenderUtil.h"

#include <map>
#include <queue>
#include <string>
#include <vector>

#include "Common.h"

namespace {
// [自定义辅助函数] 将 UTF-8 或本地编码文本转成宽字符。
std::wstring ToWideText(const char* text) {
  // 空指针或空字符串直接返回空宽字符串。
  if (!text || !text[0]) {
    return L"";
  }

  // 第一次调用 [Windows API] MultiByteToWideChar 只询问转换后需要多少 wchar_t。
  // CP_UTF8 表示输入字符串按 UTF-8 解释。
  int len =
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, nullptr, 0);
  if (len <= 0) {
    return L"";
  }

  // 分配足够空间后，再真正执行转换。
  // [C++标准库] std::wstring 保存宽字符字符串。
  std::wstring wide(len, L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, &wide[0], len);

  // Windows API 会把字符串结束符 '\0' 也写进结果，
  // [C++标准库] std::wstring 自己已经知道长度，所以这里去掉最后的结束符。
  if (!wide.empty() && wide.back() == L'\0') {
    wide.pop_back();
  }

  return wide;
}
}  // namespace

// [自定义辅助函数] 获取图片外侧连通黑色区域的遮罩。
static const std::vector<unsigned char>& GetOuterBlackMask(IMAGE* img) {
  // 遮罩计算比较耗时，所以按 IMAGE* 缓存。
  // 同一张图片第一次计算，之后直接复用结果。
  // [C++标准库容器] std::map 缓存 [EasyX类型] IMAGE* 到遮罩数组的映射。
  static std::map<IMAGE*, std::vector<unsigned char>> maskCache;

  auto found = maskCache.find(img);
  if (found != maskCache.end()) {
    return found->second;
  }

  int w = img->getwidth();
  int h = img->getheight();

  // mask 与图片像素一一对应：
  // mask[i] == 1 表示这个像素属于外侧黑色背景，绘制时应该跳过。
  std::vector<unsigned char> mask(w * h, 0);

  // [EasyX] GetImageBuffer(img) 得到图片像素数组。
  // 每个 DWORD 可以理解为一个像素的颜色值。
  DWORD* src = GetImageBuffer(img);
  if (!src || w <= 0 || h <= 0) {
    maskCache[img] = mask;
    return maskCache[img];
  }

  // [C++标准库容器] std::queue 用于广度优先搜索。
  std::queue<int> q;

  // 将二维坐标 (x,y) 转成一维数组下标。
  auto idx = [w](int x, int y) { return y * w + x; };

  // 如果某个点在图内、尚未访问、并且颜色接近黑色，就加入搜索队列。
  auto tryPush = [&](int x, int y) {
    if (x < 0 || x >= w || y < 0 || y >= h) {
      return;
    }
    int i = idx(x, y);
    if (mask[i]) {
      return;
    }
    // [自定义函数] IsNearBlack 判断颜色是否接近黑色。
    if (!IsNearBlack((COLORREF)src[i])) {
      return;
    }
    mask[i] = 1;
    q.push(i);
  };

  // 从图片四条边开始搜索。
  // 这样只会去掉“与边缘连通的黑色背景”，不会误删飞机内部的黑色细节。
  for (int x = 0; x < w; ++x) {
    tryPush(x, 0);
    tryPush(x, h - 1);
  }

  for (int y = 0; y < h; ++y) {
    tryPush(0, y);
    tryPush(w - 1, y);
  }

  // 广度优先搜索：从边缘黑色像素向上下左右扩展。
  while (!q.empty()) {
    int cur = q.front();
    q.pop();
    int x = cur % w;
    int y = cur / w;
    tryPush(x + 1, y);
    tryPush(x - 1, y);
    tryPush(x, y + 1);
    tryPush(x, y - 1);
  }

  // 保存到缓存并返回。
  maskCache[img] = mask;
  return maskCache[img];
}

// [自定义辅助函数] 按遮罩绘制图片，rotate180=true 时旋转 180 度。
static void DrawSpriteMasked(int drawX, int drawY, IMAGE* img, bool rotate180) {
  // 没有图片直接不画。
  if (!img) {
    return;
  }

  // 读取源图片尺寸。
  int w = img->getwidth();
  int h = img->getheight();
  if (w <= 0 || h <= 0) {
    return;
  }

  // src 是源图片像素，dst 是当前屏幕缓冲区像素。
  // [EasyX] GetImageBuffer 获取图片或屏幕缓冲区。
  DWORD* src = GetImageBuffer(img);
  DWORD* dst = GetImageBuffer();
  if (!src || !dst) {
    return;
  }

  const std::vector<unsigned char>& mask = GetOuterBlackMask(img);

  // 遍历源图每一个像素。
  for (int sy = 0; sy < h; ++sy) {
    for (int sx = 0; sx < w; ++sx) {
      int si = sy * w + sx;

      // 外侧黑色背景像素不绘制。
      if (mask[si]) {
        continue;
      }

      // rotate180 为 true 时，把源图坐标反过来，实现 180 度旋转。
      int dx = drawX + (rotate180 ? (w - 1 - sx) : sx);
      int dy = drawY + (rotate180 ? (h - 1 - sy) : sy);

      // 目标坐标在屏幕外时跳过，避免访问越界。
      if (dx < 0 || dx >= SCREEN_W || dy < 0 || dy >= SCREEN_H) {
        continue;
      }

      // 把源图像素写到屏幕缓冲区对应位置。
      dst[dy * SCREEN_W + dx] = src[si];
    }
  }
}

// [自定义函数] 绘制图片并跳过外侧黑色区域。
void DrawSpriteWithoutOuterBlack(int drawX, int drawY, IMAGE* img) {
  DrawSpriteMasked(drawX, drawY, img, false);
}

// [自定义函数] 居中绘制去黑边图片。
void DrawCenteredSprite(double cx, double cy, IMAGE* img) {
  if (!img) {
    return;
  }
  // 居中绘制需要把中心点换算成左上角坐标。
  DrawSpriteWithoutOuterBlack((int)cx - img->getwidth() / 2,
                              (int)cy - img->getheight() / 2, img);
}

// [自定义函数] 居中旋转 180 度绘制去黑边图片。
void DrawCenteredSpriteRot180(double cx, double cy, IMAGE* img) {
  if (!img) {
    return;
  }
  // 旋转版本同样先用中心点换算出左上角。
  DrawSpriteMasked((int)cx - img->getwidth() / 2,
                   (int)cy - img->getheight() / 2, img, true);
}

// [自定义函数] 将图片等比裁剪后铺满目标区域。
void DrawImageCover(IMAGE* img, int dstX, int dstY, int dstW, int dstH) {
  if (!img || dstW <= 0 || dstH <= 0) {
    return;
  }

  // 源图尺寸。
  int srcW = img->getwidth();
  int srcH = img->getheight();
  if (srcW <= 0 || srcH <= 0) {
    return;
  }

  int cropX = 0;
  int cropY = 0;
  int cropW = srcW;
  int cropH = srcH;

  // 比较源图宽高比和目标区域宽高比。
  double srcAspect = srcW / (double)srcH;
  double dstAspect = dstW / (double)dstH;

  if (srcAspect > dstAspect) {
    // 源图更宽：裁掉左右多余部分。
    cropW = (int)(srcH * dstAspect + 0.5);
    cropX = (srcW - cropW) / 2;
  } else if (srcAspect < dstAspect) {
    // 源图更高：裁掉上下多余部分。
    cropH = (int)(srcW / dstAspect + 0.5);
    cropY = (srcH - cropH) / 2;
  }

  // 获取目标屏幕和源图片的 [Windows API类型] HDC，交给 Windows GDI 做拉伸绘制。
  // [EasyX] GetImageHDC 返回 EasyX 图像对应的 HDC。
  HDC dst = GetImageHDC();
  HDC src = GetImageHDC(img);
  if (!dst || !src) {
    return;
  }

  // [Windows API] HALFTONE 是质量较好的拉伸模式。
  int oldMode = SetStretchBltMode(dst, HALFTONE);
  POINT oldBrush = {};
  SetBrushOrgEx(dst, 0, 0, &oldBrush);

  // [Windows API] StretchBlt 从源图裁剪区域 cropX/cropY/cropW/cropH
  // 拉伸绘制到目标区域 dstX/dstY/dstW/dstH。
  StretchBlt(dst, dstX, dstY, dstW, dstH, src, cropX, cropY, cropW, cropH,
             SRCCOPY);

  // 恢复原来的绘制设置，避免影响后续绘制。
  SetBrushOrgEx(dst, oldBrush.x, oldBrush.y, nullptr);
  SetStretchBltMode(dst, oldMode);
}

// [自定义函数] 计算 UTF-8 文本宽度。
int TextWidthUtf8(const char* text) {
  std::wstring wide = ToWideText(text);
  if (wide.empty()) {
    return 0;
  }

  // [Windows API] GetTextExtentPoint32W 可以计算当前字体下文字占用的像素尺寸。
  SIZE size = {};
  GetTextExtentPoint32W(GetImageHDC(), wide.c_str(), (int)wide.size(), &size);
  return size.cx;
}

// [自定义函数] 计算 UTF-8 文本高度。
int TextHeightUtf8(const char* text) {
  std::wstring wide = ToWideText(text);
  if (wide.empty()) {
    return 0;
  }

  // 和宽度计算相同，只是返回 cy。
  SIZE size = {};
  GetTextExtentPoint32W(GetImageHDC(), wide.c_str(), (int)wide.size(), &size);
  return size.cy;
}

// [自定义函数] 输出 UTF-8 文本。
void OutTextUtf8(int x, int y, const char* text) {
  std::wstring wide = ToWideText(text);
  if (wide.empty()) {
    return;
  }

  // [Windows API] TextOutW 是宽字符版本输出函数，适合输出中文。
  HDC hdc = GetImageHDC();
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, gettextcolor());
  TextOutW(hdc, x, y, wide.c_str(), (int)wide.size());
}

// [自定义函数] 输出带阴影的 UTF-8 文本。
void OutTextUtf8Shadow(int x, int y, const char* text, COLORREF textColor,
                       COLORREF shadowColor, int offset) {
  // 保存旧颜色，绘制完成后恢复，避免影响调用者后续文本颜色。
  COLORREF oldColor = gettextcolor();

  // 先画阴影，再画正文。
  // [EasyX] settextcolor 设置当前文本颜色。
  settextcolor(shadowColor);
  OutTextUtf8(x + offset, y + offset, text);
  settextcolor(textColor);
  OutTextUtf8(x, y, text);
  settextcolor(oldColor);
}

// [自定义函数] 设置黑色文本样式。
void SetBlackText(int height, const char* face) {
  // [EasyX] setbkmode/settextcolor/settextstyle 设置文字样式。
  setbkmode(TRANSPARENT);
  settextcolor(RGB(0, 0, 0));
  settextstyle(height, 0, face);
}

// [自定义函数] 设置白色文本样式。
void SetWhiteText(int height, const char* face) {
  // [EasyX] setbkmode/settextcolor/settextstyle 设置文字样式。
  setbkmode(TRANSPARENT);
  settextcolor(RGB(255, 255, 255));
  settextstyle(height, 0, face);
}
