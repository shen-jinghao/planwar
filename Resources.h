// 资源系统声明：集中维护图片/音频路径、资源 ID 和懒加载缓存。
//
// 游戏里会频繁绘制同一张图片，例如玩家飞机、子弹、背景。
// 如果每一帧都从磁盘重新读取图片，速度会非常慢。
// 所以本项目使用 ResourceManager：
// - 第一次需要某张图片时才加载。
// - 加载后放入 map 缓存。
// - 下次再用同一路径时直接返回缓存中的 IMAGE*。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的结构体、枚举或函数。
// [EasyX] 表示 EasyX 图形库提供的类型。
// [Windows API] 表示 Windows 系统多媒体 API。
// [C++标准库] 表示 C++ 标准库提供的容器或类型。
#pragma once

#include <graphics.h>

#include <map>
#include <string>
#include <vector>

// [自定义结构体] 资源文件路径表。
struct GameRes {
  // 玩家图片。
  const char *player_normal = "image/me1.png";
  const char *player_explode = "image/me_destroy_1.png";

  // 普通敌机、中型敌机、Boss 的常态图片。
  const char *enemy1 = "image/enemy1.png";
  const char *enemy2 = "image/enemy2.png";
  const char *enemy3 = "image/enemy3_n1.png";

  // 敌机死亡动画帧。数组中的顺序就是动画播放顺序。
  const char *enemy1_down[4] = {
      "image/enemy1_down1.png", "image/enemy1_down2.png",
      "image/enemy1_down3.png", "image/enemy1_down4.png"};

  const char *enemy2_down[4] = {
      "image/enemy2_down1.png", "image/enemy2_down2.png",
      "image/enemy2_down3.png", "image/enemy2_down4.png"};

  const char *enemy3_down[6] = {
      "image/enemy3_down1.png", "image/enemy3_down2.png",
      "image/enemy3_down3.png", "image/enemy3_down4.png",
      "image/enemy3_down5.png", "image/enemy3_down6.png"};

  // 受击贴图。小敌机没有单独受击贴图，所以只配置 enemy2 和 Boss。
  const char *enemy2_hit = "image/enemy2_hit.png";
  const char *enemy3_hit = "image/enemy3_hit.png";

  // 子弹图片。
  const char *bullet_player = "image/bullet2.png";
  const char *bullet_enemy = "image/bullet1.png";
  const char *bullet_boss_special = "image/bomb_supply.png";

  // 道具图片。
  const char *supply_health = "image/life.png";
  const char *supply_bomb = "image/bomb_supply.png";
  const char *supply_upgrade = "image/bullet_supply.png";

  // UI 图片。
  const char *ui_title = "image/title.png";

  // 音乐和音效路径。
  const char *menu_bgm = "music/526d41e94a3a0b6011dc797556be0571.mp3";
  const char *bgm = "music/01.mp3";
  const char *sfx_health = "music/5062a3488d498b773d823a8d04f92d77.mp3";
  const char *sfx_upgrade = "music/80e05ff22058e95b5e69ce37e1ee75f3.mp3";
  const char *sfx_bomb = "music/83c36d806dc92327b9e7049a565c6bff.wav";
  const char *sfx_gameover = "music/gameover.mp3";
};

extern GameRes res;

// [自定义枚举] 可通过资源管理器读取的图片 ID。
//
// 枚举的好处：
// 代码里写 [自定义枚举] AssetID::Player，比直接写 "image/me1.png" 更统一。
// 如果以后换图片路径，只需要改 [自定义结构体] GameRes 和映射函数。
enum class AssetID {
  Player,
  Player_Explode,
  Enemy1,
  Enemy2,
  Boss,
  Enemy2_Hit,
  Boss_Hit,
  Bullet_Player,
  Bullet_Enemy,
  Bullet_Boss_Special,
  Item_Health,
  Item_Bomb,
  Item_Upgrade,
  UI_Title
};

// [自定义函数] 播放循环背景音乐。
// 内部使用 [Windows API] mciSendStringA。
void PlayBackgroundMusic(const std::string &path);
// [自定义函数] 停止背景音乐。
void StopBackgroundMusic();
// [自定义函数] 播放一次性音效。
void PlaySoundEffect(const std::string &path);

// [自定义结构体] 图片缓存管理器。
struct ResourceManager {
  // key 是图片路径，value 是已加载的 IMAGE 指针。
  // 例如 fileImages["image/me1.png"] 指向玩家飞机图片。
  // [C++标准库容器] std::map 保存路径到 [EasyX类型] IMAGE* 的映射。
  std::map<std::string, IMAGE *> fileImages;

  // [自定义函数] 加载图片文件并缓存。
  // 如果已经加载过，直接返回缓存，避免重复读磁盘。
  // 内部调用 [EasyX] loadimage。
  IMAGE *LoadImageFile(const std::string &path);

  // [自定义函数] 按资源 ID 获取图片。
  // 先把 AssetID 转成路径，再调用 LoadImageFile。
  IMAGE *GetImage(AssetID id);

  // [自定义函数] 获取敌机爆炸帧。
  // type=0/1/2 分别对应小敌机、中敌机、Boss。
  std::vector<IMAGE *> GetEnemyDownFrames(int type);

  // [自定义函数] 获取玩家死亡帧。
  std::vector<IMAGE *> GetPlayerDeathFrames();

  // [自定义函数] 释放全部缓存图片。
  // 退出游戏时调用，防止 new 出来的 IMAGE 泄漏。
  void UnloadAll();
};
