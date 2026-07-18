// [EasyX] 与 MinGW/UCRT 的兼容补丁：补齐旧库期望的 __imp___iob_func 符号。
//
// 这个文件不参与游戏逻辑，不需要在学习玩法时重点研究。
// 它存在的原因是：某些 [EasyX] 静态库是按旧运行时库编译的，
// 链接到当前 MinGW/UCRT 时可能找不到 __imp___iob_func 符号。
// 这里手动提供一个兼容符号，让链接阶段可以通过。
//
// 注释标注规则：
// [自定义] 表示本文件自己补出的兼容函数或符号。
// [C运行库] 表示 C/C++ 运行库提供的函数或类型。
#include <cstdio>

// 返回标准输入 FILE 指针。
// [C运行库] __acrt_iob_func(0) 是 UCRT 中获取标准输入流的方式。
// [自定义兼容函数] easyx_iob_func 用来适配旧 EasyX 库期望的符号。
static FILE *__cdecl easyx_iob_func(void) { return __acrt_iob_func(0); }

// 导出 [EasyX] 旧符号需要的函数指针。
// __asm__("__imp___iob_func") 指定最终导出的符号名。
// [自定义兼容符号] easyx_imp_iob_func。
using EasyXIobFunc = FILE *(__cdecl *)(void);
EasyXIobFunc easyx_imp_iob_func __asm__("__imp___iob_func") = easyx_iob_func;
