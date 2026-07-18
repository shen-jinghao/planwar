// 游戏上下文实现：管理全局数据、存档读写、敌人生成、击杀结算和通用绘制。
// 状态机通过 [自定义结构体] Game
// 访问共享世界，本文件尽量只放跨状态复用的核心流程。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的结构体、函数、枚举或常量。
// [EasyX] 表示 EasyX 图形库提供的绘图函数或类型。
// [C++标准库] 表示 C++ 标准库提供的类型或算法。
#include "Game.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include "RenderUtil.h"

namespace {
// 可选背景路径。
// backgroundIndex 会作为数组下标选择当前背景。
const char* BACKGROUND_PATHS[] = {
    "image/background1.png", "image/background2.png", "image/background3.png"};
constexpr int BACKGROUND_COUNT =
    sizeof(BACKGROUND_PATHS) / sizeof(BACKGROUND_PATHS[0]);

// 敌机爆炸参数。
struct ExplosionProfile {
  double interval;
  int radius;
};

// 不同敌人类型对应的爆炸表现。
const ExplosionProfile EXPLOSIONS[] = {{0.07, 35}, {0.08, 55}, {0.09, 95}};

// 规范化读档数据。
//
// 存档文件是普通文本，理论上可能被手动修改或损坏。
// 如果直接相信文件内容，可能出现玩家坐标在屏幕外、生命为负数、
// 关卡特别大等问题。所以读档成功后先把数据夹回合法范围。
void ClampLoadedData(GameSaveData& data) {
  // 玩家位置不能超出屏幕。因为 x/y 是中心点，所以边界要预留半个宽高。
  data.player.x = clamp_val(data.player.x, (double)data.player.w / 2.0,
                            (double)SCREEN_W - data.player.w / 2.0);
  data.player.y = clamp_val(data.player.y, (double)data.player.h / 2.0,
                            (double)SCREEN_H - data.player.h / 2.0);

  // 生命、炸弹、强化时间和无敌时间都要限制在合理范围。
  data.player.maxHp = clamp_val(data.player.maxHp, 1, 20);
  data.player.hp = clamp_val(data.player.hp, 1, data.player.maxHp);
  data.player.bombs = clamp_val(data.player.bombs, 0, 99);
  data.player.upgrade_time = std::max(0.0, data.player.upgrade_time);
  data.player.inv_time = std::max(0.0, data.player.inv_time);

  // 存档只恢复“进度”，不恢复已经飞出去的子弹。
  data.player.bullets.clear();

  // 游戏进度字段也要修正，避免数组越界或难度失控。
  data.backgroundIndex = clamp_val(data.backgroundIndex, 0, 2);
  data.score = std::max(0, data.score);
  data.kills = std::max(0, data.kills);
  data.level = clamp_val(data.level, 1, 10);
  data.spawnTimer = std::max(0.0, data.spawnTimer);
  data.spawnInterval = std::max(0.4, data.spawnInterval);
  data.nextBossKills = std::max(20, data.nextBossKills);

  // 如果存档里的 nextBossKills 已经小于等于当前 kills，
  // 说明这个门槛过期了，需要向后推到未来的 Boss 节点。
  while (data.nextBossKills <= data.kills) {
    data.nextBossKills += 30;
  }
}
}  // namespace

// [自定义函数] 读取并解析存档文件。
bool ReadGameSaveData(const std::string& path, GameSaveData& data) {
  // [C++标准库] ifstream 用于从文件读取文本。
  std::ifstream in(path);
  if (!in) {
    return false;
  }

  // 第一项必须是版本标记。
  // 这样以后如果存档格式改变，可以通过版本号区分。
  std::string tag;
  in >> tag;
  if (!in) {
    return false;
  }

  if (tag != "PW_SAVE_V1") {
    return false;
  }

  // 先读到 [自定义结构体] GameSaveData 临时变量中。
  // 只有全部字段都读取成功，才赋值给输出参数 data。
  GameSaveData parsed;

  // 文本文件中 bool 不直接写 true/false，而是写 0/1，读出来后再转换。
  int upgraded = 0;
  int invincible = 0;

  if (!(in >> parsed.backgroundIndex >> parsed.score >> parsed.kills >>
        parsed.level >> parsed.nextBossKills >> parsed.spawnTimer >>
        parsed.spawnInterval >> parsed.player.x >> parsed.player.y >>
        parsed.player.hp >> parsed.player.maxHp >> parsed.player.bombs >>
        upgraded >> parsed.player.upgrade_time >> invincible >>
        parsed.player.inv_time >> parsed.player.shootTimer)) {
    return false;
  }

  // 把 0/1 转回 bool。
  parsed.player.upgraded = upgraded != 0;
  parsed.player.invincible = invincible != 0;

  // [自定义函数] ClampLoadedData 修正异常存档数据。
  ClampLoadedData(parsed);
  data = parsed;
  return true;
}

// [自定义成员函数] 切换当前状态。
void Game::ChangeState(State* s) {
  // 先通知旧状态退出。
  // 例如 [自定义状态类] PlayState 的 [自定义函数] Exit() 会停止局内背景音乐。
  if (current) {
    current->Exit();
  }

  // [C++标准库] unique_ptr::reset 会释放原来管理的对象，并接管新传入的指针。
  current.reset(s);

  // 再通知新状态进入。
  // 例如 [自定义状态类] MainMenuState 的 [自定义函数] Enter() 会播放菜单音乐。
  if (current) {
    current->Enter();
  }
}

// [自定义成员函数] 重置为新游戏。
void Game::ResetGame() {
  // [自定义结构体] Player()
  // 创建一个默认玩家对象，等于把生命、位置、炸弹等恢复初始值。
  player = Player();

  // 新开一局时，旧场上的敌人、道具和动画全部清空。
  // [C++标准库容器] vector::clear 清空容器。
  enemies.clear();
  items.clear();
  effects.clear();

  // 重置进度和刷怪计时。
  score = 0;
  kills = 0;
  level = 1;
  spawnTimer = 0;
  spawnInterval = 1.2;
  nextBossKills = 20;
}

// [自定义成员函数] 保存当前游戏进度。
bool Game::SaveGame(const std::string& path) const {
  // [C++标准库] ios::trunc 表示覆盖写入。如果文件已存在，会先清空旧内容。
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    return false;
  }

  // 第一行写版本标记。
  out << "PW_SAVE_V1\n";

  // 注意：保存和读取字段顺序必须严格一致。
  // 第一组是游戏进度数据。
  out << backgroundIndex << ' ' << score << ' ' << kills << ' ' << level << ' '
      << nextBossKills << ' ' << spawnTimer << ' ' << spawnInterval << '\n';

  // 第二组是玩家核心状态。
  // bool 转成 0/1 写入，方便用 >> 读取。
  out << player.x << ' ' << player.y << ' ' << player.hp << ' ' << player.maxHp
      << ' ' << player.bombs << ' ' << (player.upgraded ? 1 : 0) << ' '
      << player.upgrade_time << ' ' << (player.invincible ? 1 : 0) << ' '
      << player.inv_time << ' ' << player.shootTimer << '\n';

  return out.good();
}

// [自定义成员函数] 读取游戏进度并清空临时战场实体。
bool Game::LoadGame(const std::string& path) {
  GameSaveData data;

  // 如果读取失败，data.backgroundIndex 至少保留当前背景值。
  data.backgroundIndex = backgroundIndex;
  if (!ReadGameSaveData(path, data)) {
    return false;
  }

  // 恢复 [自定义结构体] Player 和进度数据。
  player = data.player;

  // 读档只恢复“安全的进度状态”，不恢复战场瞬间。
  // 如果连敌机、子弹、道具也保存/恢复，存档格式会复杂很多。
  enemies.clear();
  items.clear();
  effects.clear();

  backgroundIndex = data.backgroundIndex;
  score = data.score;
  kills = data.kills;
  level = data.level;
  spawnTimer = data.spawnTimer;
  spawnInterval = data.spawnInterval;
  nextBossKills = data.nextBossKills;

  return true;
}

// [自定义成员函数] 显示临时提示。
void Game::ShowNotice(const std::string& text, double duration) {
  // noticeText 由绘制函数显示，noticeTimer 负责倒计时。
  noticeText = text;
  noticeTimer = duration;
}

// [自定义成员函数] 更新提示计时。
void Game::UpdateNotice(double dt) {
  // 没有剩余时间时清空文本。
  if (noticeTimer <= 0) {
    noticeText.clear();
    noticeTimer = 0;
    return;
  }

  noticeTimer -= dt;
  // 时间扣完后也要清空文本，避免下一帧继续绘制。
  if (noticeTimer <= 0) {
    noticeText.clear();
    noticeTimer = 0;
  }
}

// [自定义成员函数] 判断 Boss 是否存在。
bool Game::BossExists() const { return GetBoss() != nullptr; }

// [自定义成员函数] 获取 Boss，只读版本。
const Enemy* Game::GetBoss() const {
  // Boss 本质也是 [自定义结构体] Enemy，只是 type == 2。
  for (const auto& e : enemies) {
    if (e.alive && e.type == 2) {
      return &e;
    }
  }
  return nullptr;
}

// [自定义成员函数] 获取 Boss，可修改版本。
Enemy* Game::GetBoss() {
  // 复用 const 版本的查找逻辑，再去掉 const。
  // 这样可以避免写两遍几乎相同的循环。
  return const_cast<Enemy*>(static_cast<const Game*>(this)->GetBoss());
}

// [自定义成员函数] 生成普通敌机。
void Game::SpawnEnemy() {
  // 70% 概率生成小敌机，30% 概率生成中敌机。
  int r = rand() % 100;
  int t = (r < 70) ? 0 : 1;

  // 敌机横向生成位置留出一点边距，避免贴着屏幕边缘出现。
  // [自定义函数] ScaleX/ScaleY 按窗口比例缩放设计尺寸。
  int margin = ScaleX(100);
  // [C++标准库容器] vector::emplace_back 直接构造 [自定义结构体] Enemy。
  enemies.emplace_back(margin + rand() % (SCREEN_W - margin * 2), -ScaleY(100),
                       t);
}

// [自定义成员函数] 生成 Boss。
void Game::SpawnBoss() {
  // 同一时间只允许存在一个 Boss。
  if (BossExists()) {
    return;
  }

  // Boss 从屏幕上方进入。
  // [C++标准库容器] vector::emplace_back 直接构造 [自定义结构体] Enemy。
  enemies.emplace_back(SCREEN_W / 2.0, -ScaleY(180), 2);
}

// [自定义成员函数] 添加敌机爆炸效果。
void Game::AddEnemyExplosion(const Enemy& e) {
  // 根据敌机类型获取对应爆炸帧和爆炸参数。
  // [自定义函数] GetEnemyDownFrames 返回 [EasyX类型] IMAGE* 帧列表。
  std::vector<IMAGE*> frames = resources.GetEnemyDownFrames(e.type);
  const ExplosionProfile& profile = EXPLOSIONS[clamp_val(e.type, 0, 2)];
  effects.emplace_back(e.x, e.y, frames, profile.interval, profile.radius);
}

// [自定义成员函数] 添加玩家死亡爆炸效果。
void Game::AddPlayerDeathExplosion() {
  std::vector<IMAGE*> frames = resources.GetPlayerDeathFrames();
  effects.emplace_back(player.x, player.y, frames, 0.20, 80);
}

// [自定义成员函数] 击杀敌机并结算分数与掉落。
void Game::KillEnemy(Enemy& e, bool giveScore) {
  // 防止同一个敌机被重复结算。
  if (!e.alive) {
    return;
  }

  // 先调用 [自定义函数] AddEnemyExplosion 生成爆炸动画，再把敌机标记为死亡。
  AddEnemyExplosion(e);
  e.alive = false;

  // giveScore=false 常用于敌机撞到玩家后被销毁，但不奖励分数。
  if (giveScore) {
    score += e.scoreValue;
    kills++;
  }

  // Boss 死亡后推进下一次 Boss 门槛、提升关卡，并固定掉落三种道具。
  if (e.type == 2) {
    nextBossKills += 30;
    if (level < 10) {
      level++;
    }

    // [C++标准库容器] vector::emplace_back 直接构造 [自定义结构体] Item。
    items.emplace_back(e.x - 70, e.y, ItemType::Health);
    items.emplace_back(e.x, e.y, ItemType::Bomb);
    items.emplace_back(e.x + 70, e.y, ItemType::Upgrade);
    return;
  }

  // 普通敌机随机掉落。
  // 0-9 掉生命，10-19 掉炸弹，20-39 掉强化，其它不掉。
  int rr = rand() % 100;
  if (rr < 10) {
    items.emplace_back(e.x, e.y, ItemType::Health);
  } else if (rr < 20) {
    items.emplace_back(e.x, e.y, ItemType::Bomb);
  } else if (rr < 40) {
    items.emplace_back(e.x, e.y, ItemType::Upgrade);
  }
}

// [自定义成员函数] 更新并移除结束的动画效果。
void Game::UpdateEffects(double dt) {
  // 先让每个动画自己推进时间。
  for (auto& fx : effects) {
    fx.Update(dt);
  }

  // 再删除已经播放完的动画。
  // [C++标准库算法] std::remove_if 配合 [C++标准库容器] vector::erase
  // 删除元素。
  effects.erase(
      std::remove_if(effects.begin(), effects.end(),
                     [](const AnimationEffect& fx) { return !fx.Alive(); }),
      effects.end());
}

// [自定义成员函数] 绘制当前背景。
void Game::RenderBackground() {
  // 防御式写法：即使 backgroundIndex 异常，也夹回合法范围。
  // [自定义函数] clamp_val 限制数值范围。
  int index = clamp_val(backgroundIndex, 0, BACKGROUND_COUNT - 1);
  IMAGE* bg = resources.LoadImageFile(BACKGROUND_PATHS[index]);

  // 有背景图就铺满窗口；没有图就用纯色背景兜底。
  if (bg) {
    // [自定义函数] DrawImageCover 封装了背景等比铺满绘制。
    DrawImageCover(bg, 0, 0, SCREEN_W, SCREEN_H);
  } else {
    // [EasyX] setbkcolor 设置背景色；[EasyX] cleardevice 清空画布。
    setbkcolor(RGB(245, 245, 245));
    cleardevice();
  }
}

// [自定义成员函数] 绘制背景、玩家、敌人、道具和效果。
void Game::RenderWorld(bool renderPlayer) {
  // 每帧先清屏，再完整重画背景和所有对象。
  // 2D 游戏里这种“每帧重画”非常常见。
  // [EasyX] cleardevice 清空当前帧画面。
  cleardevice();
  RenderBackground();

  // 死亡演出时 renderPlayer=false，画爆炸但不画 [自定义结构体] Player 本体。
  if (renderPlayer) {
    player.Render(resources);
  }

  // 顺序会影响遮挡关系：后画的对象会盖在先画的对象上。
  for (auto& e : enemies) {
    e.Render(resources);
  }

  for (auto& i : items) {
    i.Render(resources);
  }

  for (auto& fx : effects) {
    fx.Render();
  }
}

// [自定义成员函数] 绘制 Boss 血条。
void Game::RenderBossHpBar() {
  Enemy* boss = GetBoss();
  if (!boss) {
    return;
  }

  // 血条宽高和位置。
  int barW = ScaleX(900);
  int barH = ScaleY(24);
  int x0 = SCREEN_W / 2 - barW / 2;
  int y0 = ScaleY(30);

  // ratio 是当前血量百分比，范围限制在 0 到 1。
  double ratio = boss->hp / (double)boss->maxHp;
  ratio = clamp_val(ratio, 0.0, 1.0);

  // [EasyX] setlinecolor/setfillcolor/solidrectangle/rectangle 绘制血条底板。
  setlinecolor(RGB(0, 0, 0));
  setfillcolor(RGB(18, 26, 38));
  solidrectangle(x0, y0, x0 + barW, y0 + barH);
  rectangle(x0, y0, x0 + barW, y0 + barH);

  setfillcolor(RGB(255, 120, 120));
  solidrectangle(x0 + 2, y0 + 2, x0 + 2 + (int)((barW - 4) * ratio),
                 y0 + barH - 2);

  // [自定义函数] SetWhiteText 设置文字样式。
  SetWhiteText(ScaleFont(22));
  char buf[128];
  // [C++/安全CRT函数] sprintf_s 格式化 Boss 血量文本。
  sprintf_s(buf, sizeof(buf), "首领 生命：%d/%d   阶段：%d", boss->hp,
            boss->maxHp, boss->Phase());
  // [自定义函数] OutTextUtf8Shadow 输出 UTF-8 中文阴影文本。
  OutTextUtf8Shadow(x0 + ScaleX(260), y0 + ScaleY(32), buf);
}
