# 项目中用到的 EasyX 和 Windows API

本文只整理本项目源码中直接用到的 EasyX / Windows API 函数、类型和常用宏，不包含 C++ 标准库函数，也不包含项目自己封装的函数。

## EasyX 函数

| 名称 | 主要出现位置 | 简要用法 |
| --- | --- | --- |
| `initgraph(width, height)` | `main.cpp` | 创建 EasyX 图形窗口。项目中用 `initgraph(SCREEN_W, SCREEN_H)` 创建 1600x900 窗口。 |
| `closegraph()` | `main.cpp` | 关闭 EasyX 图形窗口，程序退出前调用。 |
| `BeginBatchDraw()` | `main.cpp` | 开启批量绘图模式，先画到后台缓冲区，减少闪烁。 |
| `FlushBatchDraw()` | `main.cpp` | 把后台缓冲区刷新到屏幕。项目主循环每帧调用一次。 |
| `EndBatchDraw()` | `main.cpp` | 结束批量绘图模式，退出前调用。 |
| `setbkcolor(color)` | `main.cpp`、`Game.cpp`、`States.cpp` | 设置背景色，通常配合 `cleardevice()` 清屏使用。 |
| `cleardevice()` | `main.cpp`、`Game.cpp`、`States.cpp` | 清空当前画布，清成当前背景色。 |
| `setlinecolor(color)` | `Entities.cpp`、`Game.cpp`、`States.cpp` | 设置后续线条、矩形边框、多边形边框的颜色。 |
| `setfillcolor(color)` | `Entities.cpp`、`Game.cpp`、`States.cpp` | 设置后续填充图形的颜色。 |
| `settextcolor(color)` | `RenderUtil.cpp` | 设置 EasyX 当前文字颜色。项目里主要用于文字和阴影文字绘制。 |
| `gettextcolor()` | `RenderUtil.cpp` | 获取当前文字颜色。项目里用于保存旧颜色，绘制完阴影后恢复。 |
| `setbkmode(mode)` | `RenderUtil.cpp` | 设置文字背景模式。项目中用 `TRANSPARENT` 让文字背景透明。 |
| `settextstyle(height, width, face)` | `RenderUtil.cpp` | 设置文字字体样式，例如字号和字体名。 |
| `fillrectangle(left, top, right, bottom)` | `States.cpp` | 画带边框的填充矩形，项目里常用于按钮、提示框、HUD 面板。 |
| `solidrectangle(left, top, right, bottom)` | `Game.cpp` | 画无边框的填充矩形，项目里用于 Boss 血条。 |
| `rectangle(left, top, right, bottom)` | `Game.cpp`、`States.cpp` | 只画矩形边框，不填充。 |
| `solidpolygon(points, count)` | `Entities.cpp` | 画无边框的填充多边形。项目里用于兜底绘制玩家飞机。 |
| `polygon(points, count)` | `Entities.cpp` | 只画多边形边框。项目里用于兜底绘制玩家飞机外框。 |
| `circle(x, y, radius)` | `Entities.cpp` | 画圆形边框。项目里用于兜底爆炸效果和玩家飞机中心圆。 |
| `loadimage(img, path)` | `Resources.cpp` | 从文件加载图片到 `IMAGE` 对象。项目里封装在 `ResourceManager::LoadImageFile()` 里。 |
| `GetImageBuffer(img)` | `RenderUtil.cpp` | 获取图片像素缓冲区，返回 `DWORD*`。传入 `IMAGE*` 读图片像素，不传参数读当前屏幕缓冲区。 |
| `GetImageHDC(img)` | `RenderUtil.cpp` | 获取 EasyX 图像或窗口对应的 GDI 设备句柄 `HDC`，用于调用 Windows GDI。 |
| `GetHWnd()` | `States.cpp` | 获取 EasyX 窗口句柄 `HWND`。项目里配合 `ScreenToClient()` 转换鼠标坐标。 |

## EasyX 类型和方法

| 名称 | 主要出现位置 | 简要用法 |
| --- | --- | --- |
| `IMAGE` / `IMAGE*` | `Resources.*`、`Entities.*`、`Game.*`、`RenderUtil.*` | EasyX 图片对象。项目里图片资源都缓存为 `IMAGE*`。 |
| `IMAGE::getwidth()` | `Resources.cpp`、`RenderUtil.cpp`、`States.cpp` | 获取图片宽度。常用于判断图片是否加载成功、居中绘制、像素遍历。 |
| `IMAGE::getheight()` | `Resources.cpp`、`RenderUtil.cpp`、`States.cpp` | 获取图片高度。用途同 `getwidth()`。 |

## Windows API / GDI 函数

| 名称 | 主要出现位置 | 简要用法 |
| --- | --- | --- |
| `Sleep(ms)` | `main.cpp` | 让当前线程暂停指定毫秒数。项目主循环里 `Sleep(10)` 用来避免 CPU 占用过高。 |
| `GetAsyncKeyState(vk)` | `Common.cpp` | 获取键盘或鼠标按键当前状态。项目封装成 `IsKeyDown()` 使用。 |
| `GetCursorPos(&pt)` | `States.cpp` | 获取鼠标在整个屏幕上的坐标。 |
| `ScreenToClient(hwnd, &pt)` | `States.cpp` | 把屏幕坐标转换成某个窗口内部坐标。项目里把鼠标坐标转换到 EasyX 窗口内。 |
| `mciSendStringA(cmd, ret, retLen, callback)` | `Resources.cpp` | 发送 MCI 多媒体命令。项目用它打开、播放、停止、关闭背景音乐和音效。 |
| `MultiByteToWideChar(codePage, flags, src, srcLen, dst, dstLen)` | `RenderUtil.cpp` | 把多字节字符串转换成宽字符字符串。项目里把 UTF-8 文本转成 `std::wstring`，方便 GDI 输出中文。 |
| `SetStretchBltMode(hdc, mode)` | `RenderUtil.cpp` | 设置图片拉伸模式。项目中用 `HALFTONE` 提高背景图缩放质量。 |
| `SetBrushOrgEx(hdc, x, y, oldPoint)` | `RenderUtil.cpp` | 设置画刷原点。配合 `HALFTONE` 拉伸模式使用，并在绘制后恢复。 |
| `StretchBlt(dstHdc, dstX, dstY, dstW, dstH, srcHdc, srcX, srcY, srcW, srcH, rop)` | `RenderUtil.cpp` | 从源 HDC 裁剪并拉伸绘制到目标 HDC。项目里用于背景图等比例裁剪铺满窗口。 |
| `GetTextExtentPoint32W(hdc, text, len, &size)` | `RenderUtil.cpp` | 计算宽字符文本在当前字体下占用的像素宽高。项目里用于文字居中。 |
| `SetBkMode(hdc, mode)` | `RenderUtil.cpp` | 设置 GDI 文字背景模式。项目中用 `TRANSPARENT` 让 `TextOutW()` 输出文字时不盖背景。 |
| `SetTextColor(hdc, color)` | `RenderUtil.cpp` | 设置 GDI 文字颜色。项目里把 EasyX 当前文字色传给 GDI。 |
| `TextOutW(hdc, x, y, text, len)` | `RenderUtil.cpp` | 输出宽字符文本。项目里用于稳定显示 UTF-8 中文。 |
| `GetRValue(color)` | `Common.cpp` | 从 `COLORREF` 颜色值中取红色通道。 |
| `GetGValue(color)` | `Common.cpp` | 从 `COLORREF` 颜色值中取绿色通道。 |
| `GetBValue(color)` | `Common.cpp` | 从 `COLORREF` 颜色值中取蓝色通道。 |

## Windows 常用类型和宏

| 名称 | 主要出现位置 | 简要用法 |
| --- | --- | --- |
| `RGB(r, g, b)` | 全项目多处 | 生成 `COLORREF` 颜色值，例如 `RGB(255, 255, 255)` 表示白色。 |
| `COLORREF` | `Common.*`、`RenderUtil.*`、`Entities.cpp` | Windows 颜色类型，配合 `RGB()`、`setfillcolor()`、`setlinecolor()` 等使用。 |
| `POINT` | `States.cpp`、`Entities.cpp`、`RenderUtil.cpp` | 保存二维坐标，成员是 `x` 和 `y`。项目里用于鼠标坐标、多边形顶点、画刷原点。 |
| `SIZE` | `RenderUtil.cpp` | 保存文本宽高，成员是 `cx` 和 `cy`。 |
| `HWND` | `States.cpp` | Windows 窗口句柄类型。项目通过 `GetHWnd()` 获得 EasyX 窗口句柄。 |
| `HDC` | `RenderUtil.cpp` | GDI 设备上下文句柄。项目通过 `GetImageHDC()` 获得，然后传给 GDI 绘图函数。 |
| `DWORD` | `RenderUtil.cpp` | 32 位无符号整数。项目里 `DWORD*` 表示像素缓冲区。 |
| `LONG` | `Entities.cpp` | Windows 长整型。项目里构造 `POINT` 顶点时做类型转换。 |
| `MCIERROR` | `Resources.cpp` | MCI 命令返回错误码类型。值为 `0` 通常表示命令成功。 |
| `VK_RETURN` | `States.cpp` | 回车键虚拟键码。 |
| `VK_ESCAPE` | `States.cpp` | Esc 键虚拟键码。 |
| `VK_LBUTTON` | `States.cpp` | 鼠标左键虚拟键码。 |
| `VK_RBUTTON` | `States.cpp` | 鼠标右键虚拟键码。 |
| `VK_UP` / `VK_DOWN` / `VK_LEFT` / `VK_RIGHT` | `Entities.cpp` | 方向键虚拟键码。 |
| `CP_UTF8` | `RenderUtil.cpp` | `MultiByteToWideChar()` 的代码页参数，表示输入是 UTF-8。 |
| `MB_ERR_INVALID_CHARS` | `RenderUtil.cpp` | UTF-8 转换标志，遇到非法字符时返回错误。 |
| `TRANSPARENT` | `RenderUtil.cpp` | 透明背景模式，给 `setbkmode()` 和 `SetBkMode()` 使用。 |
| `HALFTONE` | `RenderUtil.cpp` | 高质量拉伸模式，给 `SetStretchBltMode()` 使用。 |
| `SRCCOPY` | `RenderUtil.cpp` | 光栅操作码，表示直接把源图复制到目标图。 |

## 代码阅读提示

- 画图主流程在 `main.cpp`：创建窗口、开启批量绘图、每帧刷新、退出时关闭窗口。
- 图片加载在 `Resources.cpp`：`loadimage()` 加载，`IMAGE*` 缓存。
- 图片像素处理和中文文字输出在 `RenderUtil.cpp`：这里 EasyX 和 Windows GDI 混用最多。
- 鼠标、键盘输入在 `Common.cpp` 和 `States.cpp`：键盘用 `GetAsyncKeyState()`，鼠标位置用 `GetCursorPos()` 加 `ScreenToClient()`。
- 音乐音效在 `Resources.cpp`：统一通过 `mciSendStringA()` 发送播放命令。
