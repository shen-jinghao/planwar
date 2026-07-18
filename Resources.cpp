// 资源系统实现：负责图片缓存、资源 ID 映射以及 MCI 音频播放。
// 图片采用按需加载，音效使用多个别名轮换，减少短时间重复播放被截断的问题。
//
// 注释标注规则：
// [自定义] 表示本项目自己定义的结构体、函数或枚举。
// [EasyX] 表示 EasyX 图形库提供的类型或函数。
// [Windows API] 表示 Windows 多媒体 API。
// [C++标准库] 表示 C++ 标准库提供的类型或函数。
#include "Resources.h"

#include <mmsystem.h>

#include <cstdio>

#include "Common.h"

GameRes res;

namespace {
// [自定义函数] 将资源 ID 映射到文件路径。
//
// 其它代码通常只知道 [自定义枚举] AssetID::Player 这种枚举值。
// 这个函数负责把枚举翻译成真正的文件路径。
std::string GetAssetPath(AssetID id) {
  switch (id) {
    case AssetID::Player:
      return res.player_normal;
    case AssetID::Player_Explode:
      return res.player_explode;
    case AssetID::Enemy1:
      return res.enemy1;
    case AssetID::Enemy2:
      return res.enemy2;
    case AssetID::Boss:
      return res.enemy3;
    case AssetID::Enemy2_Hit:
      return res.enemy2_hit;
    case AssetID::Boss_Hit:
      return res.enemy3_hit;
    case AssetID::Bullet_Player:
      return res.bullet_player;
    case AssetID::Bullet_Enemy:
      return res.bullet_enemy;
    case AssetID::Bullet_Boss_Special:
      return res.bullet_boss_special;
    case AssetID::Item_Health:
      return res.supply_health;
    case AssetID::Item_Bomb:
      return res.supply_bomb;
    case AssetID::Item_Upgrade:
      return res.supply_upgrade;
    case AssetID::UI_Title:
      return res.ui_title;
  }
  return "";
}
}  // namespace

// [自定义函数] 播放循环背景音乐。
void PlayBackgroundMusic(const std::string &path) {
  // 背景音乐统一使用 alias bgm。
  // 先 close 掉旧 bgm，避免多个背景音乐同时播放。
  // [Windows API] mciSendStringA 执行 MCI 音频命令。
  mciSendStringA("close bgm", nullptr, 0, nullptr);

  // 文件不存在就直接返回，不让 MCI 报错影响游戏。
  // [自定义函数] FileExists 判断文件是否存在。
  if (!FileExists(path)) {
    return;
  }

  // [Windows API] MCI 使用字符串命令控制音频。
  // open "...path..." type mpegvideo alias bgm 表示打开音频并命名为 bgm。
  char cmd[1024] = {};
  sprintf_s(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias bgm",
            path.c_str());

  MCIERROR err = mciSendStringA(cmd, nullptr, 0, nullptr);
  if (err == 0) {
    // repeat 表示循环播放。
    mciSendStringA("play bgm repeat", nullptr, 0, nullptr);
  }
}

// [自定义函数] 停止并关闭背景音乐。
void StopBackgroundMusic() {
  // [Windows API] mciSendStringA 发送 stop/close 命令。
  // stop 停止播放，close 释放 MCI 对这个 alias 的占用。
  mciSendStringA("stop bgm", nullptr, 0, nullptr);
  mciSendStringA("close bgm", nullptr, 0, nullptr);
}

// [自定义函数] 播放一次性音效。
void PlaySoundEffect(const std::string &path) {
  // 音效文件不存在时直接忽略。
  if (!FileExists(path)) {
    return;
  }

  // 使用多个 alias 轮换播放音效。
  // 如果所有音效都叫同一个 alias，短时间连续播放时可能互相打断。
  static int aliasIndex = 0;
  char alias[32] = {};
  // [C++/安全CRT函数] sprintf_s 格式化 alias 字符串。
  sprintf_s(alias, sizeof(alias), "sfx%d", aliasIndex);
  aliasIndex = (aliasIndex + 1) % 8;

  // 复用 alias 前先关闭它上一次播放的音频。
  char cmd[1024] = {};
  sprintf_s(cmd, sizeof(cmd), "close %s", alias);
  mciSendStringA(cmd, nullptr, 0, nullptr);

  // wav 和 mp3 在 MCI 中使用的 type 不同。
  const bool isWav =
      path.size() >= 4 && (path.substr(path.size() - 4) == ".wav" ||
                           path.substr(path.size() - 4) == ".WAV");

  // 打开音效文件。
  sprintf_s(cmd, sizeof(cmd), "open \"%s\" type %s alias %s", path.c_str(),
            isWav ? "waveaudio" : "mpegvideo", alias);

  if (mciSendStringA(cmd, nullptr, 0, nullptr) == 0) {
    // from 0 表示从开头播放。
    sprintf_s(cmd, sizeof(cmd), "play %s from 0", alias);
    mciSendStringA(cmd, nullptr, 0, nullptr);
  }
}

// [自定义成员函数] 加载图片文件并缓存结果。
IMAGE *ResourceManager::LoadImageFile(const std::string &path) {
  // 如果缓存里已经有这张图，直接返回，不重复加载。
  auto it = fileImages.find(path);
  if (it != fileImages.end()) {
    return it->second;
  }

  // 路径为空或文件不存在，返回空指针。
  // 调用方一般会用简单图形兜底绘制。
  if (path.empty() || !FileExists(path)) {
    return nullptr;
  }

  // [EasyX类型] IMAGE 需要动态创建，然后用 [EasyX] loadimage 读入图片内容。
  IMAGE *img = new IMAGE();
  loadimage(img, path.c_str());

  // 如果加载失败，图片宽高通常无效，需要释放并返回 nullptr。
  if (img->getwidth() <= 0 || img->getheight() <= 0) {
    delete img;
    return nullptr;
  }

  // 保存到缓存，后续同一路径直接复用。
  fileImages[path] = img;
  return img;
}

// [自定义成员函数] 按资源 ID 获取图片。
IMAGE *ResourceManager::GetImage(AssetID id) {
  // [自定义函数] GetAssetPath 负责从枚举找到路径，
  // [自定义成员函数] LoadImageFile 负责缓存加载。
  return LoadImageFile(GetAssetPath(id));
}

// [自定义成员函数] 获取指定敌人类型的爆炸帧。
std::vector<IMAGE *> ResourceManager::GetEnemyDownFrames(int type) {
  // 默认按 Boss 的 6 帧爆炸图处理。
  const char *const *paths = res.enemy3_down;
  int count = 6;

  // 小敌机和中敌机各有 4 帧。
  if (type == 0) {
    paths = res.enemy1_down;
    count = 4;
  } else if (type == 1) {
    paths = res.enemy2_down;
    count = 4;
  }

  std::vector<IMAGE *> frames;
  for (int i = 0; i < count; ++i) {
    // 每一帧也走缓存加载。
    IMAGE *img = LoadImageFile(paths[i]);
    if (img) {
      frames.push_back(img);
    }
  }

  return frames;
}

// [自定义成员函数] 获取玩家死亡动画帧。
std::vector<IMAGE *> ResourceManager::GetPlayerDeathFrames() {
  std::vector<IMAGE *> frames;
  IMAGE *img = GetImage(AssetID::Player_Explode);
  if (img) {
    // 玩家死亡目前只有一张爆炸图，也用 [C++标准库容器] vector 保存，方便和其它动画统一处理。
    frames.push_back(img);
  }
  return frames;
}

// [自定义成员函数] 释放全部图片缓存。
void ResourceManager::UnloadAll() {
  // LoadImageFile 中 new 出来的 IMAGE 都在这里 delete。
  for (auto &p : fileImages) {
    delete p.second;
  }
  fileImages.clear();
}
