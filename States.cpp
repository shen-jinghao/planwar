// 具体状态机实现：包含主菜单、存档页、暂停、游戏结束、死亡演出和局内游玩逻辑。
// 本文件负责 UI 交互、状态切换、局内碰撞/道具/刷怪调度，以及每个状态的绘制。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的状态类、结构体、函数、枚举或常量。
// [EasyX] 表示 EasyX 图形库提供的绘图函数或类型。
// [Windows API] 表示 Windows 系统 API。
// [C++标准库] 表示 C++ 标准库提供的类型或算法。
#include "States.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <string>

#include "Game.h"
#include "RenderUtil.h"

namespace {
// [自定义状态实现文件] States.cpp 是项目中最大的文件。
// 为了让各个状态都能复用按钮绘制、鼠标命中、存档槽计算等小功能，
// 这里先在匿名命名空间中放一批“只给本文件使用”的辅助类型和函数。
//
// 匿名命名空间的作用：里面的名字只在当前 .cpp 文件可见，
// 不会和其它文件里的同名函数/变量冲突。

// [自定义结构体] 矩形按钮区域。
struct ButtonRect {
  // x/y 是按钮左上角坐标，w/h 是按钮宽高。
  int x;
  int y;
  int w;
  int h;
};

// 下面这一大组 constexpr 都是 [自定义布局常量]。
// 它们使用 ScaleX/ScaleY，是为了把 1920x1080 设计尺寸缩放到当前窗口。
// 新手阅读时不必背每个数字，只要知道这些常量决定按钮位置和大小。
constexpr int MAIN_BUTTON_W = ScaleX(320);
constexpr int MAIN_BUTTON_H = ScaleY(64);
constexpr int MAIN_BUTTON_X = SCREEN_W / 2 - MAIN_BUTTON_W / 2;
constexpr int START_BUTTON_Y = ScaleY(330);
constexpr int SAVE_MENU_BUTTON_Y = ScaleY(410);
constexpr int EXIT_BUTTON_Y = ScaleY(490);
constexpr int CREDIT_BUTTON_W = ScaleX(220);
constexpr int CREDIT_BUTTON_H = ScaleY(56);
constexpr int CREDIT_BUTTON_X = SCREEN_W - CREDIT_BUTTON_W - ScaleX(30);
constexpr int CREDIT_BUTTON_Y = SCREEN_H - CREDIT_BUTTON_H - ScaleY(30);
constexpr int BG_BUTTON_X = ScaleX(20);
constexpr int BG_BUTTON_Y = ScaleY(20);
constexpr int BG_BUTTON_W = ScaleX(190);
constexpr int BG_BUTTON_H = ScaleY(48);
constexpr int PLAY_BUTTON_W = ScaleX(150);
constexpr int PLAY_BUTTON_H = ScaleY(48);
constexpr int PLAY_BUTTON_X = SCREEN_W - PLAY_BUTTON_W - ScaleX(30);
constexpr int PLAY_PAUSE_BUTTON_Y = ScaleY(25);
constexpr int PLAY_BOMB_BUTTON_Y = ScaleY(85);
constexpr int PLAY_SAVE_MENU_BUTTON_Y = ScaleY(145);
constexpr int PAUSE_BUTTON_W = ScaleX(320);
constexpr int PAUSE_BUTTON_H = ScaleY(66);
constexpr int PAUSE_BUTTON_X = SCREEN_W / 2 - PAUSE_BUTTON_W / 2;
constexpr int PAUSE_RESUME_BUTTON_Y = SCREEN_H / 2 + ScaleY(10);
constexpr int PAUSE_SAVE_MENU_BUTTON_Y = SCREEN_H / 2 + ScaleY(90);
constexpr int PAUSE_MENU_BUTTON_Y = SCREEN_H / 2 + ScaleY(170);
constexpr int GAMEOVER_BUTTON_W = ScaleX(300);
constexpr int GAMEOVER_BUTTON_H = ScaleY(62);
constexpr int GAMEOVER_BUTTON_X = SCREEN_W / 2 - GAMEOVER_BUTTON_W / 2;
constexpr int GAMEOVER_RETRY_BUTTON_Y = ScaleY(610);
constexpr int GAMEOVER_SAVE_MENU_BUTTON_Y = ScaleY(690);
constexpr int GAMEOVER_MENU_BUTTON_Y = ScaleY(770);
constexpr int SAVE_PAGE_CARD_W = ScaleX(420);
constexpr int SAVE_PAGE_CARD_H = ScaleY(410);
constexpr int SAVE_PAGE_CARD_Y = ScaleY(260);
constexpr int SAVE_PAGE_CARD_GAP = ScaleX(50);
constexpr int SAVE_PAGE_CARD_X =
    SCREEN_W / 2 - (SAVE_PAGE_CARD_W * 3 + SAVE_PAGE_CARD_GAP * 2) / 2;
constexpr int SAVE_PAGE_ACTION_W = ScaleX(150);
constexpr int SAVE_PAGE_ACTION_H = ScaleY(54);
constexpr int SAVE_PAGE_BACK_W = ScaleX(260);
constexpr int SAVE_PAGE_BACK_H = ScaleY(62);
constexpr int SAVE_PAGE_BACK_X = SCREEN_W / 2 - SAVE_PAGE_BACK_W / 2;
constexpr int SAVE_PAGE_BACK_Y = ScaleY(870);
constexpr double CREDITS_SPEED = 130.0 * UI_SCALE;
constexpr double CREDITS_LINE_GAP = 70.0 * UI_SCALE;

const ButtonRect START_BUTTON = {MAIN_BUTTON_X, START_BUTTON_Y, MAIN_BUTTON_W,
                                 MAIN_BUTTON_H};
const ButtonRect SAVE_MENU_BUTTON = {MAIN_BUTTON_X, SAVE_MENU_BUTTON_Y,
                                     MAIN_BUTTON_W, MAIN_BUTTON_H};
const ButtonRect EXIT_BUTTON = {MAIN_BUTTON_X, EXIT_BUTTON_Y, MAIN_BUTTON_W,
                                MAIN_BUTTON_H};
const ButtonRect CREDIT_BUTTON = {CREDIT_BUTTON_X, CREDIT_BUTTON_Y,
                                  CREDIT_BUTTON_W, CREDIT_BUTTON_H};
const ButtonRect PLAY_PAUSE_BUTTON = {PLAY_BUTTON_X, PLAY_PAUSE_BUTTON_Y,
                                      PLAY_BUTTON_W, PLAY_BUTTON_H};
const ButtonRect PLAY_BOMB_BUTTON = {PLAY_BUTTON_X, PLAY_BOMB_BUTTON_Y,
                                     PLAY_BUTTON_W, PLAY_BUTTON_H};
const ButtonRect PLAY_SAVE_MENU_BUTTON = {
    PLAY_BUTTON_X, PLAY_SAVE_MENU_BUTTON_Y, PLAY_BUTTON_W, PLAY_BUTTON_H};
const ButtonRect PAUSE_RESUME_BUTTON = {PAUSE_BUTTON_X, PAUSE_RESUME_BUTTON_Y,
                                        PAUSE_BUTTON_W, PAUSE_BUTTON_H};
const ButtonRect PAUSE_SAVE_MENU_BUTTON = {
    PAUSE_BUTTON_X, PAUSE_SAVE_MENU_BUTTON_Y, PAUSE_BUTTON_W, PAUSE_BUTTON_H};
const ButtonRect PAUSE_MENU_BUTTON = {PAUSE_BUTTON_X, PAUSE_MENU_BUTTON_Y,
                                      PAUSE_BUTTON_W, PAUSE_BUTTON_H};
const ButtonRect GAMEOVER_RETRY_BUTTON = {GAMEOVER_BUTTON_X,
                                          GAMEOVER_RETRY_BUTTON_Y,
                                          GAMEOVER_BUTTON_W, GAMEOVER_BUTTON_H};
const ButtonRect GAMEOVER_SAVE_MENU_BUTTON = {
    GAMEOVER_BUTTON_X, GAMEOVER_SAVE_MENU_BUTTON_Y, GAMEOVER_BUTTON_W,
    GAMEOVER_BUTTON_H};
const ButtonRect GAMEOVER_MENU_BUTTON = {GAMEOVER_BUTTON_X,
                                         GAMEOVER_MENU_BUTTON_Y,
                                         GAMEOVER_BUTTON_W, GAMEOVER_BUTTON_H};
const ButtonRect SAVE_PAGE_BACK_BUTTON = {SAVE_PAGE_BACK_X, SAVE_PAGE_BACK_Y,
                                          SAVE_PAGE_BACK_W, SAVE_PAGE_BACK_H};

// 制作名单文字。
const char* CREDITS_LINES[] = {"2510120222 申璟皓", "2510120224 张思媛",
                               "2510120225 李欣雅"};

// 背景切换按钮显示文本，对应 [自定义结构体] Game::backgroundIndex 0/1/2。
const char* BG_BUTTON_LABELS[] = {"背景一", "背景二", "背景三"};

// [自定义辅助函数] 判断坐标是否落在按钮内。
bool InButton(const ButtonRect& button, int x, int y) {
  // ButtonRect 的 x/y 是左上角，所以判断点是否在 [x, x+w] 和 [y, y+h] 内。
  return x >= button.x && x <= button.x + button.w && y >= button.y &&
         y <= button.y + button.h;
}

// [自定义辅助函数] 命中背景切换按钮时返回背景索引。
int HitBackgroundButton(int x, int y) {
  // 背景按钮是一排三个按钮。
  // 如果 y 不在这一排按钮的高度范围内，直接返回 -1 表示没点中。
  if (y < BG_BUTTON_Y || y > BG_BUTTON_Y + BG_BUTTON_H) {
    return -1;
  }
  for (int i = 0; i < 3; ++i) {
    // 每个按钮宽 BG_BUTTON_W，第 i 个按钮的左边界依次向右偏移。
    int left = BG_BUTTON_X + i * BG_BUTTON_W;
    if (x >= left && x <= left + BG_BUTTON_W) {
      return i;
    }
  }

  return -1;
}

// [自定义辅助函数] 获取鼠标在游戏窗口内的位置。
POINT GetClientMousePos() {
  POINT pt;
  GetCursorPos(&pt);
  ScreenToClient(GetHWnd(), &pt);
  return pt;
}

// [自定义辅助函数] 居中绘制普通文本。
void DrawCenteredText(int y, const char* text) {
  OutTextUtf8((SCREEN_W - TextWidthUtf8(text)) / 2, y, text);
}
// [自定义辅助函数] 居中绘制带阴影文本。
void DrawCenteredTextShadow(int y, const char* text) {
  OutTextUtf8Shadow((SCREEN_W - TextWidthUtf8(text)) / 2, y, text,
                    RGB(255, 255, 255), RGB(0, 0, 0), ScaleLen(2));
}

// [自定义辅助函数] 格式化可变参数文本。
void FormatBuffer(char* buf, size_t size, const char* format, va_list args) {
  // vsnprintf 类似 printf，但结果写入 buf。
  // 这里封装一下，避免每个绘制格式化文本的地方重复写安全结尾逻辑。
  vsnprintf(buf, size, format, args);
  buf[size - 1] = '\0';
}

// [自定义辅助函数] 格式化并绘制文本。
void DrawTextFormat(int x, int y, const char* format, ...) {
  // 这个函数让我们可以像 printf 一样构造文本，然后直接画到屏幕。
  // 例如 [自定义辅助函数] DrawTextFormat(x, y, "得分：%d", score)。
  char buf[256] = {};
  va_list args;
  va_start(args, format);
  FormatBuffer(buf, sizeof(buf), format, args);
  va_end(args);
  OutTextUtf8(x, y, buf);
}

// [自定义辅助函数] 绘制统一样式按钮。
void DrawButton(const ButtonRect& button, const char* text, bool hovered,
                int fontHeight = 30) {
  // [EasyX] setlinecolor/setfillcolor/fillrectangle/rectangle 绘制按钮底板。
  // hovered 为 true 时换一套颜色，让鼠标悬停时有视觉反馈。
  setlinecolor(hovered ? RGB(255, 213, 92) : RGB(220, 235, 255));
  setfillcolor(hovered ? RGB(41, 72, 106) : RGB(18, 31, 48));
  fillrectangle(button.x, button.y, button.x + button.w, button.y + button.h);
  rectangle(button.x, button.y, button.x + button.w, button.y + button.h);

  SetWhiteText(ScaleFont(fontHeight));

  // 居中文本：按钮左上角 + (按钮尺寸 - 文本尺寸) / 2。
  int textX = button.x + (button.w - TextWidthUtf8(text)) / 2;
  int textY = button.y + (button.h - TextHeightUtf8(text)) / 2;
  OutTextUtf8(textX, textY, text);
}

// [自定义辅助函数] 格式化按钮文字并绘制按钮。
void DrawButtonFormat(const ButtonRect& button, bool hovered, int fontHeight,
                      const char* format, ...) {
  // 先把可变参数格式化成普通字符串，再复用 [自定义辅助函数] DrawButton 绘制。
  char buf[128] = {};
  va_list args;
  va_start(args, format);
  FormatBuffer(buf, sizeof(buf), format, args);
  va_end(args);
  DrawButton(button, buf, hovered, fontHeight);
}

// [自定义辅助函数] 格式化并显示临时提示。
void ShowNoticeFormat(Game* game, const char* format, ...) {
  // 临时提示文本也常常需要带数字，例如“2号存档已保存”。
  char buf[128] = {};
  va_list args;
  va_start(args, format);
  FormatBuffer(buf, sizeof(buf), format, args);
  va_end(args);
  game->ShowNotice(buf);
}

// [自定义辅助函数] 绘制临时提示框。
void DrawNotice(Game* game, int y) {
  // 没有提示文本或倒计时已经结束时不绘制。
  if (!game || game->noticeText.empty() || game->noticeTimer <= 0) {
    return;
  }

  const char* text = game->noticeText.c_str();

  // 根据文本宽度动态计算提示框宽度，保证文字可以居中放进去。
  int w = TextWidthUtf8(text) + ScaleX(70);
  int h = ScaleY(48);
  int x = (SCREEN_W - w) / 2;

  setlinecolor(RGB(255, 213, 92));
  setfillcolor(RGB(17, 28, 43));
  fillrectangle(x, y, x + w, y + h);
  rectangle(x, y, x + w, y + h);

  SetWhiteText(ScaleFont(28));
  OutTextUtf8(x + (w - TextWidthUtf8(text)) / 2,
              y + (h - TextHeightUtf8(text)) / 2, text);
}

// [自定义辅助函数] 绘制主菜单背景选择按钮组。
void DrawBackgroundButtons(int selectedIndex, int hoverIndex) {
  SetWhiteText(ScaleFont(22));

  for (int i = 0; i < 3; ++i) {
    // 三个背景按钮水平排布。
    int left = BG_BUTTON_X + i * BG_BUTTON_W;
    int right = left + BG_BUTTON_W;

    // selectedIndex 用金色边框表示当前选中背景。
    // hoverIndex 用深色填充变化表示鼠标悬停。
    setlinecolor(i == selectedIndex ? RGB(255, 213, 92) : RGB(220, 235, 255));
    setfillcolor(i == hoverIndex ? RGB(41, 72, 106) : RGB(18, 31, 48));
    fillrectangle(left, BG_BUTTON_Y, right, BG_BUTTON_Y + BG_BUTTON_H);
    rectangle(left, BG_BUTTON_Y, right, BG_BUTTON_Y + BG_BUTTON_H);

    if (i == hoverIndex || i == selectedIndex) {
      int inset = ScaleLen(3);
      rectangle(left + inset, BG_BUTTON_Y + inset, right - inset,
                BG_BUTTON_Y + BG_BUTTON_H - inset);
    }
    int textX = left + (BG_BUTTON_W - TextWidthUtf8(BG_BUTTON_LABELS[i])) / 2;
    int textY =
        BG_BUTTON_Y + (BG_BUTTON_H - TextHeightUtf8(BG_BUTTON_LABELS[i])) / 2;
    OutTextUtf8(textX, textY, BG_BUTTON_LABELS[i]);
  }
}
// [自定义辅助函数] 判断中心点矩形是否与屏幕矩形重叠。
bool OverlapsRect(double x, double y, int w, int h, const ButtonRect& rect) {
  // ButtonRect 是左上角坐标，RectOverlap 需要中心点坐标。
  // 所以这里把按钮换算成中心点再判断重叠。
  return RectOverlap(x, y, w, h, rect.x + rect.w / 2.0, rect.y + rect.h / 2.0,
                     rect.w, rect.h);
}
// [自定义辅助函数] 将被 HUD 底板盖住的敌机补绘到 HUD 上方。
void RenderEnemiesOverHud(Game* game, int x, int y, int w, int h) {
  ButtonRect hud = {x, y, w, h};
  for (auto& e : game->enemies) {
    if (!e.alive) {
      continue;
    }
    if (OverlapsRect(e.x, e.y, e.w, e.h, hud)) {
      // HUD 底板会盖住实体。这里把与 HUD 区域重叠的敌机本体补画一次，
      // 避免敌机在左上角经过时看起来被 UI 吃掉。
      e.RenderBody(game->resources);
    }
  }
}

// [自定义辅助函数] 将被 HUD 底板盖住的效果补绘到 HUD 上方。
void RenderEffectsOverHud(Game* game, int x, int y, int w, int h) {
  ButtonRect hud = {x, y, w, h};

  for (auto& fx : game->effects) {
    if (!fx.Alive()) {
      continue;
    }

    int fxW = fx.fallbackRadius * 2;
    int fxH = fx.fallbackRadius * 2;

    // 如果动画有图片帧，就用最大帧尺寸估算动画占用区域。
    for (auto* frame : fx.frames) {
      if (!frame) {
        continue;
      }
      fxW = std::max(fxW, frame->getwidth());
      fxH = std::max(fxH, frame->getheight());
    }

    if (OverlapsRect(fx.x, fx.y, fxW, fxH, hud)) {
      // 和敌机类似，被 HUD 底板遮住的爆炸效果也补画一次。
      fx.Render();
    }
  }
}

// [自定义辅助函数] 使用炸弹清理敌机或重创 Boss。
void UseBomb(Game* game) {
  // 没有炸弹就不能使用。
  if (game->player.bombs <= 0) {
    return;
  }

  // 先消耗炸弹，再播放音效。
  game->player.bombs--;
  // [自定义函数] PlaySoundEffect 播放音效。
  PlaySoundEffect(res.sfx_bomb);

  for (auto& e : game->enemies) {
    if (!e.alive) {
      continue;
    }

    if (e.IsBoss()) {
      // 炸弹不会直接秒杀 Boss，而是造成固定 20 点伤害。
      e.TakeDamage(20);
      if (e.hp <= 0) {
        game->KillEnemy(e, true);
      }
    } else {
      // 普通敌机直接击杀并给分。
      game->KillEnemy(e, true);
    }
  }
}

// [自定义辅助函数] 玩家死亡时切换到死亡演出状态。
bool EnterPlayerDyingIfNeeded(Game* game) {
  // 生命仍大于 0，不需要切换。
  if (game->player.hp > 0) {
    return false;
  }

  // 生命归零后不直接进入 [自定义状态类] GameOverState，而是先播放玩家爆炸演出。
  // [自定义成员函数] ChangeState 切换到 [自定义状态类] PlayerDyingState。
  game->ChangeState(new PlayerDyingState(game));
  return true;
}

// [自定义辅助函数] 更新 Boss/普通敌机生成。
void UpdateSpawning(Game* game, double dt) {
  // 先判断场上是否已有 Boss。
  bool bossExists = game->BossExists();

  // 达到击杀门槛且没有 Boss 时，生成 Boss。
  if (!bossExists && game->kills >= game->nextBossKills) {
    // [自定义成员函数] SpawnBoss 生成 Boss。
    game->SpawnBoss();
    bossExists = true;
  }

  // 普通敌机刷怪计时。
  game->spawnTimer += dt;

  // 关卡越高，刷怪间隔越短，但最低不低于 0.40 秒。
  game->spawnInterval = 1.2 - game->level * 0.10;
  if (game->spawnInterval < 0.40) {
    game->spawnInterval = 0.40;
  }

  // Boss 存在时暂停普通刷怪，让玩家专心打 Boss。
  if (!bossExists && game->spawnTimer >= game->spawnInterval) {
    game->spawnTimer = 0;
    // [自定义成员函数] SpawnEnemy 生成普通敌机。
    game->SpawnEnemy();
  }
}

// [自定义辅助函数] 更新全部敌人。
void UpdateEnemies(Game* game, double dt) {
  // 每个敌人自己负责移动、射击、更新子弹。
  for (auto& e : game->enemies) {
    e.Update(dt);
  }
}

// [自定义辅助函数] 处理玩家子弹命中敌机。
void HandlePlayerBulletHits(Game* game) {
  // 双重循环：每个敌人都要检查每颗玩家子弹是否打中它。
  for (auto& e : game->enemies) {
    for (auto& b : game->player.bullets) {
      if (!e.alive || !b.alive) {
        continue;
      }

      // [自定义函数] PointInRect 判断子弹中心点是否进入敌机矩形。
      if (PointInRect(b.x, b.y, e.x, e.y, e.w, e.h)) {
        // 子弹命中后失效，敌人扣 1 点血。
        b.alive = false;
        e.TakeDamage(1);

        if (e.hp <= 0) {
          // 敌人死亡时交给 [自定义成员函数] Game::KillEnemy
          // 统一结算分数、掉落和爆炸。
          game->KillEnemy(e, true);
        }
      }
    }
  }
}

// [自定义辅助函数] 处理敌方子弹命中玩家。
bool HandleEnemyBulletHits(Game* game) {
  // 遍历每个敌人发射的子弹。
  for (auto& e : game->enemies) {
    for (auto& b : e.bullets) {
      // 玩家无敌时不处理敌方子弹伤害。
      if (!b.alive || game->player.invincible) {
        continue;
      }

      if (PointInRect(b.x, b.y, game->player.x, game->player.y, game->player.w,
                      game->player.h)) {
        b.alive = false;

        // BossSpecial 使用特殊伤害规则。
        if (b.kind == BulletKind::BossSpecial) {
          game->player.HurtByBossSpecialBomb();
        } else {
          game->player.Hurt(1);
        }

        // 如果这次伤害导致死亡，立刻切换状态并通知调用方停止后续局内更新。
        if (EnterPlayerDyingIfNeeded(game)) {
          return true;
        }
      }
    }
  }

  return false;
}

// [自定义辅助函数] 处理敌机本体碰撞玩家。
bool HandleEnemyBodyHits(Game* game) {
  // 处理敌机本体和玩家飞机重叠。
  for (auto& e : game->enemies) {
    if (!e.alive || game->player.invincible) {
      continue;
    }

    // [自定义函数] RectOverlap 判断敌机矩形和玩家矩形是否重叠。
    if (RectOverlap(e.x, e.y, e.w, e.h, game->player.x, game->player.y,
                    game->player.w, game->player.h)) {
      if (e.IsBoss()) {
        // 撞到 Boss 扣 2 点血，但 Boss 不会被碰撞击杀。
        game->player.Hurt(2);
      } else {
        // 普通敌机撞到玩家后销毁，但不奖励分数。
        game->KillEnemy(e, false);
        game->player.Hurt(1);
      }

      if (EnterPlayerDyingIfNeeded(game)) {
        return true;
      }
    }
  }

  return false;
}

// [自定义辅助函数] 更新并拾取道具。
void CollectItems(Game* game, double dt) {
  for (auto& i : game->items) {
    // 道具先下落。
    i.Update(dt);

    if (!i.alive) {
      continue;
    }

    if (PointInRect(i.x, i.y, game->player.x, game->player.y, game->player.w,
                    game->player.h)) {
      // 道具被拾取后标记为无效，后面 CleanupWorld 删除。
      i.alive = false;

      if (i.type == ItemType::Health) {
        // 生命 +1，但不能超过 maxHp。
        game->player.hp = clamp_val(game->player.hp + 1, 0, game->player.maxHp);
        PlaySoundEffect(res.sfx_health);
      } else if (i.type == ItemType::Bomb) {
        // 炸弹数量 +1。
        game->player.bombs++;
        PlaySoundEffect(res.sfx_bomb);
      } else {
        // 火力强化持续 10 秒。
        game->player.upgraded = true;
        game->player.upgrade_time = 10;
        PlaySoundEffect(res.sfx_upgrade);
      }
    }
  }
}

// [自定义辅助函数] 清理死亡实体并更新关卡。
void CleanupWorld(Game* game) {
  // 删除已经死亡或飞出屏幕的敌人。
  // [C++标准库算法] std::remove_if 配合 [C++标准库容器] vector::erase
  // 删除元素。
  game->enemies.erase(std::remove_if(game->enemies.begin(), game->enemies.end(),
                                     [](const Enemy& e) { return !e.alive; }),
                      game->enemies.end());

  // 删除已经被拾取或掉出屏幕的道具。
  game->items.erase(std::remove_if(game->items.begin(), game->items.end(),
                                   [](const Item& i) { return !i.alive; }),
                    game->items.end());

  // 普通关卡推进：每 10 击杀提升一级。
  // Boss 存在时不在这里提升，Boss 死亡后的提升在 [自定义成员函数]
  // Game::KillEnemy 中处理。
  if (game->kills >= game->level * 10 && game->level < 10 &&
      !game->BossExists()) {
    game->level++;
  }
}

// [自定义辅助函数] 返回存档槽对应路径。
std::string SaveSlotPath(int slot) {
  // 项目一共有三个存档槽，分别对应三个文本文件。
  if (slot == 1) {
    return "save.dat";
  }
  if (slot == 2) {
    return "save1.dat";
  }
  return "save2.dat";
}

// [自定义辅助函数] 保存指定槽并提示结果。
void SaveGameAndNotify(Game* game, int slot) {
  // 保存成功/失败都通过 noticeText 给玩家一个短暂提示。
  if (game->SaveGame(SaveSlotPath(slot))) {
    ShowNoticeFormat(game, "%d号存档已保存", slot);
  } else {
    game->ShowNotice("存档失败");
  }
}

// [自定义辅助函数] 读取指定槽并提示结果。
bool LoadGameAndNotify(Game* game, int slot) {
  // 读取成功后返回 true，调用方通常会切到 [自定义状态类] PlayState。
  if (game->LoadGame(SaveSlotPath(slot))) {
    ShowNoticeFormat(game, "%d号存档读取成功", slot);
    return true;
  }

  ShowNoticeFormat(game, "%d号存档不存在", slot);
  return false;
}

// [自定义辅助函数] 读取存档槽摘要。
bool ReadSaveSlotSummary(int slot, GameSaveData& summary) {
  // 存档页显示关卡、分数、生命等摘要时，只需要读取存档内容，
  // 不应该真的改动当前 [自定义结构体] Game 状态，所以这里直接读到 summary。
  return ReadGameSaveData(SaveSlotPath(slot), summary);
}

// [自定义辅助函数] 获取存档卡片区域。
ButtonRect SaveSlotCardRect(int index) {
  // index 是 0/1/2。三张卡片横向排布，中间有固定间距。
  return {SAVE_PAGE_CARD_X + index * (SAVE_PAGE_CARD_W + SAVE_PAGE_CARD_GAP),
          SAVE_PAGE_CARD_Y, SAVE_PAGE_CARD_W, SAVE_PAGE_CARD_H};
}

// [自定义辅助函数] 获取存档卡片中的保存按钮区域。
ButtonRect SaveSlotSaveButton(int index) {
  // 保存按钮放在卡片底部左侧。
  ButtonRect card = SaveSlotCardRect(index);
  return {card.x + ScaleX(45), card.y + card.h - ScaleY(82), SAVE_PAGE_ACTION_W,
          SAVE_PAGE_ACTION_H};
}

// [自定义辅助函数] 获取存档卡片中的读取按钮区域。
ButtonRect SaveSlotLoadButton(int index, bool saveAllowed) {
  ButtonRect card = SaveSlotCardRect(index);

  // 如果页面允许保存，读取按钮放右侧；如果只允许读取，读取按钮居中。
  int x = saveAllowed ? card.x + card.w - SAVE_PAGE_ACTION_W - ScaleX(45)
                      : card.x + (card.w - SAVE_PAGE_ACTION_W) / 2;
  return {x, card.y + card.h - ScaleY(82), SAVE_PAGE_ACTION_W,
          SAVE_PAGE_ACTION_H};
}

// [自定义辅助函数] 根据来源返回存档页之前的状态。
void ReturnFromSaveSlot(Game* game, SaveSlotReturn target) {
  // 存档页可能从主菜单、暂停、局内、游戏结束进入。
  // 点击返回时用 returnTarget 回到原来的流程。
  if (target == SaveSlotReturn::Pause) {
    game->ChangeState(new PauseState(game));
  } else if (target == SaveSlotReturn::Play) {
    game->ChangeState(new PlayState(game));
  } else if (target == SaveSlotReturn::GameOver) {
    game->ChangeState(new GameOverState(game));
  } else {
    game->ChangeState(new MainMenuState(game));
  }
}

// [自定义辅助函数] 绘制单个存档卡片。
void DrawSaveSlotCard(int index, bool saveAllowed, POINT mouse) {
  ButtonRect card = SaveSlotCardRect(index);
  GameSaveData summary;

  // hasSave 表示这个槽位是否存在合法存档。
  bool hasSave = ReadSaveSlotSummary(index + 1, summary);

  // 绘制白色卡片底板。
  setlinecolor(RGB(18, 31, 48));
  setfillcolor(RGB(255, 255, 255));
  fillrectangle(card.x, card.y, card.x + card.w, card.y + card.h);
  rectangle(card.x, card.y, card.x + card.w, card.y + card.h);

  SetBlackText(ScaleFont(40));
  DrawTextFormat(card.x + ScaleX(34), card.y + ScaleY(28), "%d号存档",
                 index + 1);

  SetBlackText(ScaleFont(28));
  if (hasSave) {
    // 有存档时显示摘要，方便玩家判断要读哪个档。
    int lineX = card.x + ScaleX(38);
    DrawTextFormat(lineX, card.y + ScaleY(102), "关卡：%d", summary.level);
    DrawTextFormat(lineX, card.y + ScaleY(146), "得分：%d", summary.score);
    DrawTextFormat(lineX, card.y + ScaleY(190), "击毁：%d", summary.kills);
    DrawTextFormat(lineX, card.y + ScaleY(234), "生命：%d/%d",
                   summary.player.hp, summary.player.maxHp);
    DrawTextFormat(lineX, card.y + ScaleY(278), "炸弹：%d",
                   summary.player.bombs);
  } else {
    // 没有存档时显示空档。
    OutTextUtf8(card.x + ScaleX(38), card.y + ScaleY(150), "空档");
  }

  if (saveAllowed) {
    // 管理模式下才绘制保存按钮。
    ButtonRect saveButton = SaveSlotSaveButton(index);
    DrawButtonFormat(saveButton, InButton(saveButton, mouse.x, mouse.y), 26,
                     "保存%d", index + 1);
  }

  // 读取按钮总是存在。
  ButtonRect loadButton = SaveSlotLoadButton(index, saveAllowed);
  DrawButtonFormat(loadButton, InButton(loadButton, mouse.x, mouse.y), 26,
                   "读取%d", index + 1);
}
}  // namespace

// [自定义重写函数] 进入主菜单。
void MainMenuState::Enter() {
  // 记录刚进入状态时的按键状态，避免从上一个状态切过来时误触发。
  enterPrev = IsKeyDown(VK_RETURN);
  escPrev = IsKeyDown(VK_ESCAPE);
  lbuttonPrev = IsKeyDown(VK_LBUTTON);

  // 每次进入主菜单时默认不显示制作名单。
  creditsActive = false;
  creditsY = SCREEN_H + ScaleY(80);

  // 主菜单播放菜单音乐。
  // [自定义函数] PlayBackgroundMusic 播放菜单背景音乐。
  PlayBackgroundMusic(res.menu_bgm);
}

// [自定义重写函数] 更新主菜单交互。
void MainMenuState::Update(double dt) {
  // 所有状态都要更新 noticeTimer，否则提示不会自动消失。
  game->UpdateNotice(dt);

  // [自定义函数] IsKeyDown 内部调用 [Windows API] GetAsyncKeyState。
  bool escNow = IsKeyDown(VK_ESCAPE);

  if (creditsActive) {
    // 制作名单向上滚动。
    creditsY -= CREDITS_SPEED * dt;

    // 计算最后一行文字的 y 坐标，用来判断是否滚完。
    double lastLineY =
        creditsY + (sizeof(CREDITS_LINES) / sizeof(CREDITS_LINES[0]) - 1) *
                       CREDITS_LINE_GAP;
    POINT mouse = GetClientMousePos();
    bool lbuttonNow = IsKeyDown(VK_LBUTTON);

    // 制作名单页面右下角按钮复用 CREDIT_BUTTON 区域作为“返回菜单”。
    bool backClicked =
        lbuttonNow && !lbuttonPrev && InButton(CREDIT_BUTTON, mouse.x, mouse.y);

    // Esc、点击返回或文字滚完，都退出制作名单页面。
    if ((escNow && !escPrev) || backClicked || lastLineY < -90) {
      creditsActive = false;
      creditsY = SCREEN_H + ScaleY(80);
    }

    // 更新上一帧状态，然后直接 return，避免同时处理主菜单按钮。
    enterPrev = IsKeyDown(VK_RETURN);
    escPrev = escNow;
    lbuttonPrev = lbuttonNow;
    return;
  }

  bool enterNow = IsKeyDown(VK_RETURN);
  // Enter 键等价于点击“开始游戏”。
  if (enterNow && !enterPrev) {
    game->ResetGame();
    // [自定义成员函数] ChangeState 切换到 [自定义状态类] PlayState。
    game->ChangeState(new PlayState(game));
    return;
  }
  enterPrev = enterNow;

  // Esc 退出整个游戏。
  if (escNow && !escPrev) {
    game->running = false;
    return;
  }
  escPrev = escNow;

  POINT mouse = GetClientMousePos();
  bool lbuttonNow = IsKeyDown(VK_LBUTTON);
  if (lbuttonNow && !lbuttonPrev) {
    // 鼠标刚点击时，先判断是否点到背景按钮。
    int bgIndex = HitBackgroundButton(mouse.x, mouse.y);
    if (bgIndex >= 0) {
      game->backgroundIndex = bgIndex;
    } else if (InButton(START_BUTTON, mouse.x, mouse.y)) {
      // 开始游戏：重置数据并进入局内。
      game->ResetGame();
      game->ChangeState(new PlayState(game));
      return;
    } else if (InButton(SAVE_MENU_BUTTON, mouse.x, mouse.y)) {
      // 主菜单进入存档页时只允许读取。
      game->ChangeState(new SaveSlotState(game, SaveSlotMode::LoadOnly,
                                          SaveSlotReturn::MainMenu));
      return;
    }

    // 退出按钮。
    if (InButton(EXIT_BUTTON, mouse.x, mouse.y)) {
      game->running = false;
      return;
    }

    // 制作名单按钮。
    if (InButton(CREDIT_BUTTON, mouse.x, mouse.y)) {
      creditsActive = true;
      creditsY = SCREEN_H + ScaleY(80);
    }
  }
  lbuttonPrev = lbuttonNow;
}

// [自定义重写函数] 绘制主菜单。
void MainMenuState::Render() {
  // 主菜单先清屏再画背景。
  // [EasyX] cleardevice 清空画布。
  cleardevice();
  game->RenderBackground();

  if (creditsActive) {
    // 制作名单页面和普通主菜单共用同一个状态。
    SetWhiteText(ScaleFont(34));
    DrawCenteredTextShadow(ScaleY(70), "制作名单");

    SetWhiteText(ScaleFont(44), "Microsoft YaHei");
    for (int i = 0; i < (int)(sizeof(CREDITS_LINES) / sizeof(CREDITS_LINES[0]));
         ++i) {
      DrawCenteredTextShadow((int)(creditsY + i * CREDITS_LINE_GAP),
                             CREDITS_LINES[i]);
    }

    POINT mouse = GetClientMousePos();
    DrawButton(CREDIT_BUTTON, "返回菜单",
               InButton(CREDIT_BUTTON, mouse.x, mouse.y), 28);
    return;
  }

  // 优先使用标题图片；图片缺失时用文字兜底。
  IMAGE* titleImg = game->resources.GetImage(AssetID::UI_Title);
  if (titleImg) {
    DrawCenteredSprite(SCREEN_W / 2.0, ScaleY(250), titleImg);
  } else {
    SetWhiteText(ScaleFont(72));
    DrawCenteredTextShadow(ScaleY(250), "飞机大战");
  }

  POINT mouse = GetClientMousePos();
  // hoverBg 表示鼠标当前悬停在哪个背景按钮上。
  int hoverBg = HitBackgroundButton(mouse.x, mouse.y);
  DrawBackgroundButtons(game->backgroundIndex, hoverBg);

  DrawButton(START_BUTTON, "开始游戏", InButton(START_BUTTON, mouse.x, mouse.y),
             32);
  DrawButton(SAVE_MENU_BUTTON, "读取存档",
             InButton(SAVE_MENU_BUTTON, mouse.x, mouse.y), 32);
  DrawButton(EXIT_BUTTON, "退出游戏", InButton(EXIT_BUTTON, mouse.x, mouse.y),
             32);
  DrawButton(CREDIT_BUTTON, "制作名单",
             InButton(CREDIT_BUTTON, mouse.x, mouse.y), 28);

  SetWhiteText(ScaleFont(32));
  OutTextUtf8Shadow(SCREEN_W / 2 - ScaleX(260), ScaleY(740),
                    "鼠标拖动飞机，也可以用 WASD / 方向键移动");
  OutTextUtf8Shadow(SCREEN_W / 2 - ScaleX(260), ScaleY(800),
                    "点击右上角按钮暂停、保存、读取或使用炸弹");

  SetWhiteText(ScaleFont(22));
  OutTextUtf8Shadow(SCREEN_W / 2 - ScaleX(260), ScaleY(875),
                    "首领会在击毁 20 / 50 / 80 ... 架敌机后出现");
  DrawNotice(game, ScaleY(930));
}

// [自定义重写函数] 进入存档页。
void SaveSlotState::Enter() {
  // 记录进入时按键状态，防止刚切进来就误触发点击或返回。
  lbuttonPrev = IsKeyDown(VK_LBUTTON);
  escPrev = IsKeyDown(VK_ESCAPE);
}

// [自定义重写函数] 更新存档页交互。
void SaveSlotState::Update(double dt) {
  game->UpdateNotice(dt);

  // Esc 返回来源状态。
  bool escNow = IsKeyDown(VK_ESCAPE);
  if (escNow && !escPrev) {
    ReturnFromSaveSlot(game, returnTarget);
    return;
  }
  escPrev = escNow;

  POINT mouse = GetClientMousePos();
  bool lbuttonNow = IsKeyDown(VK_LBUTTON);
  if (lbuttonNow && !lbuttonPrev) {
    // 点击底部返回按钮。
    if (InButton(SAVE_PAGE_BACK_BUTTON, mouse.x, mouse.y)) {
      ReturnFromSaveSlot(game, returnTarget);
      return;
    }

    // Manage 模式允许保存；LoadOnly 模式只允许读取。
    bool saveAllowed = mode == SaveSlotMode::Manage;
    for (int i = 0; i < 3; ++i) {
      if (saveAllowed && InButton(SaveSlotSaveButton(i), mouse.x, mouse.y)) {
        SaveGameAndNotify(game, i + 1);
        break;
      }

      if (InButton(SaveSlotLoadButton(i, saveAllowed), mouse.x, mouse.y) ||
          (!saveAllowed && InButton(SaveSlotCardRect(i), mouse.x, mouse.y))) {
        // 只读模式下，点击整张卡片也可以尝试读取。
        if (LoadGameAndNotify(game, i + 1)) {
          game->ChangeState(new PlayState(game));
          return;
        }
        break;
      }
    }
  }

  lbuttonPrev = lbuttonNow;
}

// [自定义重写函数] 绘制存档页。
void SaveSlotState::Render() {
  // 存档页使用浅色纯背景，不沿用游戏背景。
  // [EasyX] setbkcolor 设置背景色；[EasyX] cleardevice 清屏。
  setbkcolor(RGB(245, 245, 245));
  cleardevice();

  POINT mouse = GetClientMousePos();
  bool saveAllowed = mode == SaveSlotMode::Manage;

  SetBlackText(ScaleFont(70));
  DrawCenteredText(ScaleY(78), "存档管理");

  SetBlackText(ScaleFont(30));
  DrawCenteredText(ScaleY(165),
                   saveAllowed ? "选择档位保存或读取" : "选择档位读取");

  for (int i = 0; i < 3; ++i) {
    // 绘制三个存档卡片。
    DrawSaveSlotCard(i, saveAllowed, mouse);
  }

  DrawButton(SAVE_PAGE_BACK_BUTTON, "返回",
             InButton(SAVE_PAGE_BACK_BUTTON, mouse.x, mouse.y), 32);
  DrawNotice(game, ScaleY(790));
}

// [自定义重写函数] 进入暂停状态。
void PauseState::Enter() {
  // 如果是按 P 进入暂停，刚进入时 P 可能还处于按下状态。
  // waitReleaseP 可以防止暂停界面马上又因为 P 被按下而退出。
  waitReleaseP = IsKeyDown('P');
  lbuttonPrev = IsKeyDown(VK_LBUTTON);
}

// [自定义重写函数] 更新暂停菜单。
void PauseState::Update(double dt) {
  game->UpdateNotice(dt);

  bool pNow = IsKeyDown('P');
  POINT mouse = GetClientMousePos();
  bool lbuttonNow = IsKeyDown(VK_LBUTTON);

  if (waitReleaseP) {
    // 等待 P 松开期间，不处理其它暂停菜单操作。
    if (!pNow) {
      waitReleaseP = false;
    }
    lbuttonPrev = lbuttonNow;
    return;
  }

  if (pNow) {
    // 再次按 P 继续游戏。
    game->ChangeState(new PlayState(game));
    return;
  }

  if (lbuttonNow && !lbuttonPrev) {
    if (InButton(PAUSE_RESUME_BUTTON, mouse.x, mouse.y)) {
      // 继续游戏。
      game->ChangeState(new PlayState(game));
      return;
    }

    if (InButton(PAUSE_SAVE_MENU_BUTTON, mouse.x, mouse.y)) {
      // 暂停进入存档页时允许保存和读取，返回目标是 Pause。
      game->ChangeState(
          new SaveSlotState(game, SaveSlotMode::Manage, SaveSlotReturn::Pause));
      return;
    }

    if (InButton(PAUSE_MENU_BUTTON, mouse.x, mouse.y)) {
      // 返回主菜单，不自动保存当前局面。
      game->ChangeState(new MainMenuState(game));
      return;
    }
  }

  lbuttonPrev = lbuttonNow;
}

// [自定义重写函数] 绘制暂停菜单。
void PauseState::Render() {
  // 暂停界面先绘制当前战场，再在上面覆盖暂停菜单。
  game->RenderWorld(true);
  game->RenderBossHpBar();

  POINT mouse = GetClientMousePos();

  setlinecolor(RGB(0, 0, 0));
  setfillcolor(RGB(255, 255, 255));
  fillrectangle(SCREEN_W / 2 - ScaleX(210), SCREEN_H / 2 - ScaleY(150),
                SCREEN_W / 2 + ScaleX(210), SCREEN_H / 2 - ScaleY(74));
  rectangle(SCREEN_W / 2 - ScaleX(210), SCREEN_H / 2 - ScaleY(150),
            SCREEN_W / 2 + ScaleX(210), SCREEN_H / 2 - ScaleY(74));

  SetBlackText(ScaleFont(54));
  DrawCenteredText(SCREEN_H / 2 - ScaleY(138), "已暂停");

  DrawButton(PAUSE_RESUME_BUTTON, "继续游戏",
             InButton(PAUSE_RESUME_BUTTON, mouse.x, mouse.y), 32);
  DrawButton(PAUSE_SAVE_MENU_BUTTON, "存档管理",
             InButton(PAUSE_SAVE_MENU_BUTTON, mouse.x, mouse.y), 32);
  DrawButton(PAUSE_MENU_BUTTON, "返回主菜单",
             InButton(PAUSE_MENU_BUTTON, mouse.x, mouse.y), 32);
  DrawNotice(game, SCREEN_H / 2 + ScaleY(260));
}

// [自定义重写函数] 进入游戏结束状态。
void GameOverState::Enter() {
  // 初始化输入边沿并播放游戏结束音效。
  enterPrev = IsKeyDown(VK_RETURN);
  lbuttonPrev = IsKeyDown(VK_LBUTTON);
  PlaySoundEffect(res.sfx_gameover);
}

// [自定义重写函数] 更新游戏结束菜单。
void GameOverState::Update(double dt) {
  game->UpdateNotice(dt);

  bool enterNow = IsKeyDown(VK_RETURN);
  if (enterNow && !enterPrev) {
    // Enter 在结束界面中返回主菜单。
    game->ChangeState(new MainMenuState(game));
    return;
  }
  enterPrev = enterNow;

  POINT mouse = GetClientMousePos();
  bool lbuttonNow = IsKeyDown(VK_LBUTTON);
  if (lbuttonNow && !lbuttonPrev) {
    if (InButton(GAMEOVER_RETRY_BUTTON, mouse.x, mouse.y)) {
      // 重新开始一局。
      game->ResetGame();
      game->ChangeState(new PlayState(game));
      return;
    }

    if (InButton(GAMEOVER_SAVE_MENU_BUTTON, mouse.x, mouse.y)) {
      // 游戏结束页只能读取存档。
      game->ChangeState(new SaveSlotState(game, SaveSlotMode::LoadOnly,
                                          SaveSlotReturn::GameOver));
      return;
    }

    if (InButton(GAMEOVER_MENU_BUTTON, mouse.x, mouse.y)) {
      // 返回主菜单。
      game->ChangeState(new MainMenuState(game));
      return;
    }
  }

  lbuttonPrev = lbuttonNow;
}

// [自定义重写函数] 绘制游戏结束界面。
void GameOverState::Render() {
  // [EasyX] setbkcolor 设置背景色；[EasyX] cleardevice 清屏。
  setbkcolor(RGB(245, 245, 245));
  cleardevice();

  SetBlackText(ScaleFont(90));
  DrawCenteredText(ScaleY(300), "游戏结束");

  SetBlackText(ScaleFont(42));
  DrawTextFormat(SCREEN_W / 2 - ScaleX(220), ScaleY(500), "最终得分：%d",
                 game->score);
  DrawTextFormat(SCREEN_W / 2 - ScaleX(220), ScaleY(560),
                 "击毁：%d    关卡：%d", game->kills, game->level);

  POINT mouse = GetClientMousePos();
  DrawButton(GAMEOVER_RETRY_BUTTON, "重新开始",
             InButton(GAMEOVER_RETRY_BUTTON, mouse.x, mouse.y), 32);
  DrawButton(GAMEOVER_SAVE_MENU_BUTTON, "读取存档",
             InButton(GAMEOVER_SAVE_MENU_BUTTON, mouse.x, mouse.y), 32);
  DrawButton(GAMEOVER_MENU_BUTTON, "返回主菜单",
             InButton(GAMEOVER_MENU_BUTTON, mouse.x, mouse.y), 32);
  DrawNotice(game, ScaleY(860));
}

// [自定义重写函数] 进入玩家死亡演出。
void PlayerDyingState::Enter() {
  // 重置演出计时，并添加玩家爆炸动画。
  timer = 0;
  game->AddPlayerDeathExplosion();
}

// [自定义重写函数] 更新玩家死亡演出。
void PlayerDyingState::Update(double dt) {
  game->UpdateNotice(dt);

  // 死亡演出只更新计时和爆炸动画，不再更新玩家、敌人和碰撞。
  timer += dt;
  game->UpdateEffects(dt);

  if (timer >= duration) {
    // 演出结束后进入游戏结束界面。
    game->ChangeState(new GameOverState(game));
    return;
  }
}

// [自定义重写函数] 绘制玩家死亡演出。
void PlayerDyingState::Render() { game->RenderWorld(false); }

// [自定义重写函数] 进入局内状态。
void PlayState::Enter() {
  // 记录进入时输入状态，避免按键从上个状态延续导致误触发。
  pPrev = IsKeyDown('P');
  bPrev = IsKeyDown('B');
  lbuttonPrev = IsKeyDown(VK_LBUTTON);

  // 局内播放战斗背景音乐。
  PlayBackgroundMusic(res.bgm);
}

// [自定义重写函数] 退出局内状态。
// 离开局内时停止背景音乐，避免进入暂停/菜单后战斗音乐继续响。
void PlayState::Exit() { StopBackgroundMusic(); }

// [自定义重写函数] 更新局内玩法。
void PlayState::Update(double dt) {
  game->UpdateNotice(dt);

  // 先读取本帧输入状态。
  POINT mouse = GetClientMousePos();
  bool lbuttonNow = IsKeyDown(VK_LBUTTON);
  bool pNow = IsKeyDown('P');

  // 按钮点击需要“当前按下 && 上一帧没按下”，这样长按不会重复触发。
  bool pauseClicked = lbuttonNow && !lbuttonPrev &&
                      InButton(PLAY_PAUSE_BUTTON, mouse.x, mouse.y);
  bool saveMenuClicked = lbuttonNow && !lbuttonPrev &&
                         InButton(PLAY_SAVE_MENU_BUTTON, mouse.x, mouse.y);

  // 暂停优先级最高，切换状态后本帧不再继续更新局内逻辑。
  if ((pNow && !pPrev) || pauseClicked) {
    game->ChangeState(new PauseState(game));
    return;
  }
  pPrev = pNow;

  if (saveMenuClicked) {
    // 局内进入存档页时允许保存和读取，返回目标是 Play。
    game->ChangeState(
        new SaveSlotState(game, SaveSlotMode::Manage, SaveSlotReturn::Play));
    return;
  }

  // 下面是局内核心更新顺序。
  // 1. 玩家移动和自动射击。
  game->player.Update(dt);

  // 2. 按时间刷普通敌机或 Boss。
  UpdateSpawning(game, dt);

  // 3. 敌人移动、射击、更新敌方子弹。
  UpdateEnemies(game, dt);

  // 4. 玩家子弹打敌人。
  HandlePlayerBulletHits(game);

  // 5. 敌方子弹/敌机本体伤害玩家。
  // 如果玩家死亡，这些函数会切到 [自定义状态类] PlayerDyingState，并返回 true。
  if (HandleEnemyBulletHits(game) || HandleEnemyBodyHits(game)) {
    return;
  }

  // 6. 道具下落和拾取。
  CollectItems(game, dt);

  // 7. 炸弹输入。支持 B 键和右上角炸弹按钮。
  bool bNow = IsKeyDown('B');
  bool bombClicked = lbuttonNow && !lbuttonPrev &&
                     InButton(PLAY_BOMB_BUTTON, mouse.x, mouse.y);
  bool bombPressed = (bNow && !bPrev) || bombClicked;
  if (bombPressed) {
    UseBomb(game);
  }

  // 更新上一帧输入状态，为下一帧的边沿判断做准备。
  bPrev = bNow;
  lbuttonPrev = lbuttonNow;

  // 8. 更新爆炸动画。
  game->UpdateEffects(dt);

  // 9. 清理死亡实体和过期道具。
  CleanupWorld(game);
}

// [自定义重写函数] 绘制局内画面。
void PlayState::Render() {
  // 先绘制世界，再绘制 HUD 和按钮。
  game->RenderWorld(true);
  game->RenderBossHpBar();

  // 左上角 HUD 面板。
  int hudX = ScaleX(12);
  int hudY = ScaleY(12);
  int hudW = ScaleX(370);
  int hudH = game->player.upgraded ? ScaleY(250) : ScaleY(210);
  setlinecolor(RGB(220, 235, 255));
  setfillcolor(RGB(13, 24, 38));
  fillrectangle(hudX, hudY, hudX + hudW, hudY + hudH);
  rectangle(hudX, hudY, hudX + hudW, hudY + hudH);

  // HUD 底板可能遮住敌机/爆炸，这里补画重叠对象。
  RenderEnemiesOverHud(game, hudX, hudY, hudW, hudH);
  RenderEffectsOverHud(game, hudX, hudY, hudW, hudH);

  SetWhiteText(ScaleFont(28));

  int hudTextX = ScaleX(20);
  // HUD 文本显示玩家和关卡状态。
  DrawTextFormat(hudTextX, ScaleY(20), "生命：%d/%d", game->player.hp,
                 game->player.maxHp);
  DrawTextFormat(hudTextX, ScaleY(60), "得分：%d", game->score);
  DrawTextFormat(hudTextX, ScaleY(100), "关卡：%d", game->level);
  DrawTextFormat(hudTextX, ScaleY(140), "炸弹：%d", game->player.bombs);
  DrawTextFormat(hudTextX, ScaleY(180), "下个首领：击毁 %d 架",
                 game->nextBossKills);

  if (game->player.upgraded) {
    // 火力强化时额外显示剩余时间。
    DrawTextFormat(hudTextX, ScaleY(220), "火力强化：%.1f",
                   game->player.upgrade_time);
  }

  // 右上角局内按钮。
  POINT mouse = GetClientMousePos();
  DrawButton(PLAY_PAUSE_BUTTON, "暂停",
             InButton(PLAY_PAUSE_BUTTON, mouse.x, mouse.y), 26);
  DrawButtonFormat(PLAY_BOMB_BUTTON,
                   InButton(PLAY_BOMB_BUTTON, mouse.x, mouse.y), 26, "炸弹 %d",
                   game->player.bombs);
  DrawButton(PLAY_SAVE_MENU_BUTTON, "存档",
             InButton(PLAY_SAVE_MENU_BUTTON, mouse.x, mouse.y), 26);

  // 底部操作提示。
  SetWhiteText(ScaleFont(30));
  const char* hintText = "WASD / 方向键移动    B / 按钮使用炸弹    P 暂停";
  int hintX = ScaleX(12);
  int hintY = SCREEN_H - ScaleY(72);
  int hintW = TextWidthUtf8(hintText) + ScaleX(34);
  setlinecolor(RGB(220, 235, 255));
  setfillcolor(RGB(13, 24, 38));
  fillrectangle(hintX, hintY, hintX + hintW, SCREEN_H - ScaleY(18));
  rectangle(hintX, hintY, hintX + hintW, SCREEN_H - ScaleY(18));
  OutTextUtf8(ScaleX(20), SCREEN_H - ScaleY(56), hintText);
  DrawNotice(game, SCREEN_H - ScaleY(122));
}
