# PlaneWar 项目开发导览

更新时间：2026-06-11

这份文档面向“学习并熟悉整个项目”的场景。它不会替代 `README.md`、`DEVELOPMENT.md` 和 `LEARNING.md`，而是把源码之间的关系、状态机继承结构、运行流程、阅读顺序和常见修改入口集中放在一起。建议先读本文件，再按末尾的学习路线回到源码中验证。

## 1. 项目一句话概览

PlaneWar 是一个基于 C++17、EasyX for MinGW 和 Windows MCI 音频接口实现的 2D 飞机射击游戏。核心架构可以概括为：

```text
main.cpp 主循环
  -> Game 全局上下文
     -> State 状态机管理当前界面/流程
     -> Entities 实体系统管理玩家、敌机、子弹、道具、动画
     -> ResourceManager 管理图片缓存
     -> RenderUtil 封装 EasyX 绘制细节
```

最重要的设计思想有三个：

1. `main.cpp` 只负责启动窗口和驱动循环，不直接写复杂玩法。
2. `Game` 保存跨状态共享的数据，是整个游戏的上下文。
3. 具体界面和流程用 `State` 派生类拆开，当前只运行一个状态。

## 2. 文件地图

| 文件或目录 | 主要职责 | 学习重点 |
| --- | --- | --- |
| `main.cpp` | 程序入口、EasyX 初始化、主循环、退出清理 | 看懂每帧如何调用 `Update` 和 `Render` |
| `Common.h/.cpp` | 屏幕尺寸、缩放、碰撞、按键、文件检测、颜色判断 | 许多模块依赖的小工具 |
| `State.h` | 状态机基类 | 所有状态继承自这里 |
| `States.h/.cpp` | 主菜单、存档页、暂停、结束、死亡演出、局内玩法 | 项目流程最集中的文件 |
| `Entities.h/.cpp` | 动画、子弹、道具、玩家、敌机/Boss | 游戏对象自身行为 |
| `Game.h/.cpp` | 全局数据、状态切换、存档、刷怪、击杀结算、世界绘制 | 状态和实体之间的共享上下文 |
| `Resources.h/.cpp` | 资源路径、图片缓存、背景音乐、音效 | 图片和音频的集中入口 |
| `RenderUtil.h/.cpp` | 去黑边绘图、背景铺满、UTF-8 中文文本、阴影文字 | EasyX 底层绘制封装 |
| `easyx_ucrt_compat.cpp` | EasyX 与 MinGW/UCRT 链接兼容补丁 | 不参与游戏逻辑 |
| `bulid.bat` | 命令行构建脚本 | 项目实际编译命令 |
| `.vscode/` | VS Code 构建、运行、调试配置 | IDE 中运行和调试 |
| `image/` | 图片素材 | 背景、飞机、子弹、道具、UI |
| `music/` | 音频素材 | BGM 和音效 |
| `save.dat`、`save1.dat`、`save2.dat` | 三个存档槽 | 文本格式存档 |

## 3. 类、结构体和继承关系

### 3.1 唯一的继承体系：状态机

项目里真正使用 C++ 继承的地方只有状态机。

```text
State
  ├─ MainMenuState
  ├─ SaveSlotState
  ├─ PauseState
  ├─ GameOverState
  ├─ PlayerDyingState
  └─ PlayState
```

基类 `State` 定义在 `State.h`：

| 成员/函数 | 作用 |
| --- | --- |
| `Game *game` | 指向全局上下文，状态通过它访问玩家、敌人、资源和状态切换 |
| `Enter()` | 进入状态时调用，通常初始化输入边沿、播放音乐 |
| `Exit()` | 离开状态时调用，通常停止音乐或清理状态内资源 |
| `Update(double dt)` | 每帧更新逻辑，纯虚函数，派生类必须实现 |
| `Render()` | 每帧绘制画面，纯虚函数，派生类必须实现 |

`Game::ChangeState(State *s)` 负责状态切换：

```text
如果 current 存在 -> current->Exit()
current.reset(s)
如果 current 存在 -> current->Enter()
```

因此你读任何状态时，都应该先看它的：

```text
Enter()
Update(double dt)
Render()
Exit()，如果有重写
```

### 3.2 各状态职责

| 状态 | 文件 | 职责 |
| --- | --- | --- |
| `MainMenuState` | `States.h/.cpp` | 主菜单、开始游戏、读档入口、退出、制作名单、背景选择 |
| `SaveSlotState` | `States.h/.cpp` | 三个存档槽的保存、读取、摘要展示和返回来源 |
| `PauseState` | `States.h/.cpp` | 暂停界面、继续游戏、进入存档管理、返回主菜单 |
| `GameOverState` | `States.h/.cpp` | 游戏结束界面、重新开始、读档、返回主菜单 |
| `PlayerDyingState` | `States.h/.cpp` | 玩家死亡后的短暂爆炸演出 |
| `PlayState` | `States.h/.cpp` | 局内核心玩法，输入、刷怪、碰撞、道具、炸弹、HUD |

### 3.3 组合关系：谁拥有谁

除状态机外，其它类型主要是组合关系。

```text
Game
  ├─ ResourceManager resources
  ├─ unique_ptr<State> current
  ├─ Player player
  │   └─ vector<Bullet> bullets
  ├─ vector<Enemy> enemies
  │   └─ 每个 Enemy 有自己的 vector<Bullet> bullets
  ├─ vector<Item> items
  └─ vector<AnimationEffect> effects

ResourceManager
  └─ map<string, IMAGE*> fileImages

AnimationEffect
  └─ vector<IMAGE*> frames
```

注意几点：

1. `Enemy` 同时表示普通敌机和 Boss，通过 `type` 区分，Boss 不是 `Enemy` 的派生类。
2. `Player` 和 `Enemy` 都各自拥有子弹列表，玩家子弹在 `player.bullets`，敌方子弹在每个 `Enemy::bullets`。
3. `AnimationEffect::frames` 里保存的是 `IMAGE*`，图片对象由 `ResourceManager` 统一管理。
4. `Game::current` 用 `std::unique_ptr<State>` 独占当前状态，切换状态时自动释放旧状态。

### 3.4 主要结构体和枚举

| 类型 | 定义位置 | 作用 |
| --- | --- | --- |
| `GameSaveData` | `Game.h` | 存档可恢复数据的临时结构 |
| `Game` | `Game.h` | 游戏全局上下文 |
| `GameRes` | `Resources.h` | 所有图片和音频路径 |
| `ResourceManager` | `Resources.h` | 图片按需加载和缓存 |
| `AnimationEffect` | `Entities.h` | 爆炸等一次性动画 |
| `Bullet` | `Entities.h` | 子弹实体 |
| `Item` | `Entities.h` | 掉落道具 |
| `Player` | `Entities.h` | 玩家飞机 |
| `Enemy` | `Entities.h` | 普通敌机和 Boss |
| `BulletKind` | `Entities.h` | 玩家、普通敌方、Boss、Boss 特殊弹 |
| `ItemType` | `Entities.h` | 生命、炸弹、火力强化 |
| `AssetID` | `Resources.h` | 图片资源 ID |
| `SaveSlotMode` | `States.h` | 存档页是只读档还是可保存/读取 |
| `SaveSlotReturn` | `States.h` | 离开存档页后回到哪里 |

`States.cpp`、`Game.cpp` 和 `Entities.cpp` 中还有一些匿名命名空间里的内部辅助结构：

| 类型 | 文件 | 作用 |
| --- | --- | --- |
| `ButtonRect` | `States.cpp` | UI 按钮矩形 |
| `ExplosionProfile` | `Game.cpp` | 不同敌机爆炸动画参数 |
| `EnemyProfile` | `Entities.cpp` | 不同敌机/Boss 基础属性表 |

这些只在本文件内部使用，外部模块不能直接访问。

## 4. 主运行流程

程序入口在 `main.cpp`。

```text
main()
  -> srand 初始化随机数
  -> initgraph(SCREEN_W, SCREEN_H)
  -> BeginBatchDraw()
  -> Game game
  -> game.ChangeState(new MainMenuState(&game))
  -> while (game.running)
       -> 计算 dt
       -> 限制 dt 最大值为 0.05
       -> game.current->Update(dt)
       -> game.current->Render()
       -> FlushBatchDraw()
       -> Sleep(10)
  -> resources.UnloadAll()
  -> StopBackgroundMusic()
  -> EndBatchDraw()
  -> closegraph()
```

这里的 `dt` 是本帧距离上一帧的秒数。玩家、敌机、子弹等移动都用 `速度 * dt`，这样不同电脑上的移动速度更稳定。

## 5. 状态切换图

```text
MainMenuState
  -> PlayState                 开始游戏
  -> SaveSlotState             读取存档
  -> 退出循环                  退出游戏

PlayState
  -> PauseState                按 P 或点击暂停
  -> SaveSlotState             点击存档
  -> PlayerDyingState          玩家生命归零

PauseState
  -> PlayState                 继续游戏
  -> SaveSlotState             存档管理
  -> MainMenuState             返回主菜单

SaveSlotState
  -> PlayState                 成功读档后进入游戏
  -> MainMenuState/PauseState/PlayState/GameOverState
                               点击返回或 Esc，根据 returnTarget 决定

PlayerDyingState
  -> GameOverState             死亡演出计时结束

GameOverState
  -> PlayState                 重新开始
  -> SaveSlotState             读取存档
  -> MainMenuState             返回主菜单
```

## 6. 局内玩法主流程

局内几乎所有玩法都由 `PlayState::Update(double dt)` 调度。推荐先把这个函数读熟。

```text
PlayState::Update(dt)
  -> 更新临时提示
  -> 读取鼠标和键盘状态
  -> 判断暂停按钮 / P 键
  -> 判断存档按钮
  -> 鼠标左键拖动玩家
  -> game->player.Update(dt)
  -> UpdateSpawning(game, dt)
  -> UpdateEnemies(game, dt)
  -> HandlePlayerBulletHits(game)
  -> HandleEnemyBulletHits(game)
  -> HandleEnemyBodyHits(game)
  -> CollectItems(game, dt)
  -> 判断炸弹按键 / 右键 / 炸弹按钮
  -> game->UpdateEffects(dt)
  -> CleanupWorld(game)
```

读这个流程时要注意顺序：

1. 先处理可能导致状态切换的输入，比如暂停、存档。
2. 再更新玩家和敌人。
3. 再做碰撞和道具拾取。
4. 最后清理 `alive == false` 的对象。

## 7. 实体系统详解

### 7.1 `Player`

`Player` 保存玩家位置、尺寸、生命、炸弹数量、火力强化、无敌时间、移动速度、射击冷却和玩家子弹列表。

核心函数：

| 函数 | 作用 |
| --- | --- |
| `Hurt(int damage)` | 普通受伤，扣血并开启 1.5 秒无敌 |
| `HurtByBossSpecialBomb()` | Boss 特殊弹伤害，扣当前生命约一半但最低保留 1 点 |
| `Update(double dt)` | 键盘移动、边界限制、自动射击、子弹更新、无敌和强化倒计时 |
| `Render(ResourceManager &rm)` | 绘制玩家和玩家子弹，无敌时闪烁 |

火力强化时，玩家一次发射三发子弹；普通状态下一次发射一发。

### 7.2 `Enemy`

`Enemy` 同时表示小敌机、中敌机和 Boss。

类型含义：

| `type` | 含义 | 生命 | 行为 |
| --- | --- | ---: | --- |
| `0` | 小敌机 | 1 | 向下移动，低频射击 |
| `1` | 中敌机 | 3 | 向下移动，稍快射击，分数更高 |
| `2` | Boss | 80 | 入场后水平移动，分阶段弹幕和特殊弹 |

核心函数：

| 函数 | 作用 |
| --- | --- |
| `Enemy(...)` | 根据 `type` 从 `ENEMY_PROFILES` 初始化属性 |
| `IsBoss()` | 判断 `type == 2` |
| `Phase()` | Boss 根据剩余生命进入 1/2/3 阶段 |
| `TakeDamage(int damage)` | 扣血并开启受击贴图计时 |
| `Update(double dt)` | 普通敌机移动和射击，Boss 入场、移动、阶段变化和弹幕 |
| `RenderBody(ResourceManager &rm)` | 只绘制敌机本体 |
| `Render(ResourceManager &rm)` | 绘制敌机本体和它的子弹 |

Boss 阶段规则：

```text
hp > maxHp / 2        -> 阶段 1
hp <= maxHp / 2       -> 阶段 2
hp <= maxHp / 4       -> 阶段 3
```

阶段越高，Boss 移动速度更快、射击间隔更短、弹幕数量更多。

### 7.3 `Bullet`

`Bullet` 是最简单的移动实体：

```text
x += vx * dt
y += vy * dt
如果超出屏幕外 100 像素，alive = false
```

绘制时根据 `BulletKind` 选择不同贴图或兜底颜色。

### 7.4 `Item`

`Item` 表示掉落道具：

| `ItemType` | 效果 |
| --- | --- |
| `Health` | 玩家生命 +1，不超过最大生命 |
| `Bomb` | 玩家炸弹 +1 |
| `Upgrade` | 开启火力强化 10 秒 |

道具由敌机死亡后在 `Game::KillEnemy()` 中随机生成，Boss 死亡时固定掉落三类道具。

### 7.5 `AnimationEffect`

`AnimationEffect` 用于爆炸动画。它持有一组帧图片和计时器：

```text
timer += dt
当前帧 = timer / frameInterval
播放完所有帧后 alive = false
```

如果没有素材帧，会用圆形线条做兜底爆炸效果。

## 8. `Game` 全局上下文

`Game` 是各状态共享的数据中心。它不是单纯的数据结构，也包含跨状态复用的操作。

### 8.1 关键成员

| 成员 | 含义 |
| --- | --- |
| `ResourceManager resources` | 图片资源管理器 |
| `std::unique_ptr<State> current` | 当前状态 |
| `bool running` | 主循环是否继续 |
| `Player player` | 玩家 |
| `std::vector<Enemy> enemies` | 当前敌人和 Boss |
| `std::vector<Item> items` | 当前掉落道具 |
| `std::vector<AnimationEffect> effects` | 当前动画效果 |
| `score`、`kills`、`level` | 分数、击杀数、关卡 |
| `backgroundIndex` | 当前背景编号 |
| `spawnTimer`、`spawnInterval` | 普通敌机刷怪计时 |
| `nextBossKills` | 下一次 Boss 出现所需击杀数 |
| `noticeText`、`noticeTimer` | 临时提示 |

### 8.2 关键函数

| 函数 | 作用 |
| --- | --- |
| `ChangeState(State *s)` | 切换状态 |
| `ResetGame()` | 开始新局，重置玩家、敌人、道具、动画、分数和进度 |
| `SaveGame()` | 保存当前核心进度 |
| `LoadGame()` | 读取进度并清空当前战场临时对象 |
| `ShowNotice()`、`UpdateNotice()` | 管理临时提示 |
| `BossExists()`、`GetBoss()` | 查询当前 Boss |
| `SpawnEnemy()`、`SpawnBoss()` | 生成敌机 |
| `KillEnemy()` | 击杀结算、加分、掉落、Boss 奖励 |
| `UpdateEffects()` | 更新并清理动画 |
| `RenderBackground()` | 绘制当前背景 |
| `RenderWorld()` | 绘制背景、玩家、敌人、道具、动画 |
| `RenderBossHpBar()` | 绘制 Boss 血条 |

## 9. 存档系统

存档文件是文本格式，第一行是版本标记：

```text
PW_SAVE_V1
```

存档槽：

| 槽位 | 文件 |
| --- | --- |
| 1 | `save.dat` |
| 2 | `save1.dat` |
| 3 | `save2.dat` |

保存的数据包括：

| 类别 | 字段 |
| --- | --- |
| 游戏进度 | 背景编号、分数、击杀数、关卡、下一次 Boss 击杀阈值、刷怪计时和间隔 |
| 玩家状态 | 位置、生命、最大生命、炸弹、是否强化、强化剩余时间、是否无敌、无敌剩余时间、射击计时 |

不会保存的数据：

1. 当前场上的敌机。
2. 当前场上的子弹。
3. 当前掉落的道具。
4. 当前爆炸动画。

所以 `Game::LoadGame()` 读档后会清空 `enemies`、`items` 和 `effects`，避免读档时恢复一个混乱的战场瞬间。

读档时会调用 `ClampLoadedData()` 修正异常数据，例如玩家位置越界、生命非法、关卡过大或 Boss 阈值落后于当前击杀数。

## 10. 资源和音频系统

### 10.1 图片资源

`Resources.h` 中的 `GameRes res` 保存所有资源路径，`AssetID` 枚举给常用图片起 ID。其它代码一般不直接写图片路径，而是：

```cpp
IMAGE *img = rm.GetImage(AssetID::Player);
```

`ResourceManager::LoadImageFile()` 使用 `map<string, IMAGE*>` 缓存图片：

```text
如果 path 已加载 -> 直接返回缓存
如果文件不存在 -> 返回 nullptr
否则 new IMAGE + loadimage + 放入缓存
```

退出游戏时，`ResourceManager::UnloadAll()` 释放缓存中的 `IMAGE*`。

### 10.2 音频

音频使用 Windows MCI：

| 函数 | 作用 |
| --- | --- |
| `PlayBackgroundMusic(path)` | 关闭旧 `bgm`，打开新文件并 repeat 播放 |
| `StopBackgroundMusic()` | 停止并关闭 `bgm` |
| `PlaySoundEffect(path)` | 用 `sfx0` 到 `sfx7` 轮换 alias 播放一次性音效 |

轮换 alias 的原因是避免短时间连续播放音效时互相截断。

## 11. 渲染工具系统

`RenderUtil.*` 解决 EasyX 绘制中的几个底层问题。

### 11.1 图片去黑边

素材有些图片带黑底。直接绘制会出现黑边，因此 `DrawSpriteWithoutOuterBlack()` 会跳过图片外侧连通的近黑色区域。

核心思路：

```text
GetOuterBlackMask(img)
  -> 从图片四条边开始
  -> 找到所有与边缘连通的近黑色像素
  -> 缓存成 mask

DrawSpriteMasked(...)
  -> 遍历源图片像素
  -> 如果 mask 中标记为外侧黑色，则跳过
  -> 否则写入 EasyX 目标缓冲区
```

这样只去掉外侧黑色背景，尽量保留飞机或子弹内部真正的黑色细节。

### 11.2 背景铺满

`DrawImageCover()` 按目标区域宽高比裁剪源图，再 `StretchBlt` 等比拉伸，效果类似网页里的 `background-size: cover`。

### 11.3 中文文本

项目源码字符串是 UTF-8。为了在 EasyX/Windows 中稳定显示中文：

```text
const char* UTF-8
  -> MultiByteToWideChar(CP_UTF8)
  -> std::wstring
  -> TextOutW
```

相关函数：

| 函数 | 作用 |
| --- | --- |
| `TextWidthUtf8()` | 计算 UTF-8 文本宽度 |
| `TextHeightUtf8()` | 计算 UTF-8 文本高度 |
| `OutTextUtf8()` | 输出 UTF-8 文本 |
| `OutTextUtf8Shadow()` | 输出带阴影文本 |
| `SetBlackText()`、`SetWhiteText()` | 设置常用文字样式 |

## 12. 构建和运行

命令行构建：

```bat
bulid.bat
```

注意文件名是 `bulid.bat`，不是常见的 `build.bat`。

构建脚本会：

1. 检查 `mingw64/bin/g++.exe` 是否存在。
2. 检查 EasyX 头文件。
3. 检查 EasyX 静态库。
4. 设置 `PATH`。
5. 编译 `main.cpp Common.cpp Entities.cpp Game.cpp Resources.cpp RenderUtil.cpp States.cpp easyx_ucrt_compat.cpp`。

VS Code 中推荐使用 `.vscode/launch.json` 里的 `Run PlaneWar` 配置，它会先运行 `build PlaneWar with EasyX` 任务，再启动 `PlaneWar.exe`。

## 13. 推荐学习顺序

### 第 0 阶段：先运行

先确认自己能编译和运行：

```bat
bulid.bat
PlaneWar.exe
```

运行后实际体验这些操作：

1. 主菜单开始游戏、切换背景、读取存档。
2. 局内用键盘移动和鼠标拖动。
3. 使用炸弹、暂停、存档。
4. 打到 Boss 或观察 Boss 触发规则。
5. 死亡后进入游戏结束界面。

先体验功能，再看代码会轻松很多。

### 第 1 阶段：建立骨架

按这个顺序读：

1. `README.md`
2. `Common.h`
3. `main.cpp`
4. `State.h`
5. `Game.h`
6. `States.h`
7. `Entities.h`

这一阶段不要急着读实现细节，目标是回答：

1. 游戏窗口多大？
2. 主循环在哪里？
3. 状态机基类长什么样？
4. `Game` 里保存了哪些对象？
5. 有哪些具体状态？
6. 有哪些实体？

### 第 2 阶段：看主循环和状态切换

读：

1. `main.cpp`
2. `Game::ChangeState()`
3. `MainMenuState::Enter/Update/Render`
4. `PlayState::Enter/Exit`
5. `PauseState::Enter/Update/Render`
6. `GameOverState::Enter/Update/Render`

目标是能画出状态切换图，明白 `Enter`、`Update`、`Render`、`Exit` 的调用时机。

### 第 3 阶段：看实体行为

读：

1. `Bullet::Update/Render`
2. `Item::Update/Render`
3. `Player::Hurt`
4. `Player::Update`
5. `Enemy::Enemy`
6. `Enemy::Phase`
7. `Enemy::Update`
8. `AnimationEffect::Update/Render`

这一阶段重点理解 `dt`、`alive`、`vector`、`remove_if`、`type` 和 `kind`。

### 第 4 阶段：看全局管理

读：

1. `Game::ResetGame`
2. `Game::SaveGame`
3. `ReadGameSaveData`
4. `Game::LoadGame`
5. `Game::SpawnEnemy`
6. `Game::SpawnBoss`
7. `Game::KillEnemy`
8. `Game::RenderWorld`
9. `Game::RenderBossHpBar`

目标是理解 `Game` 为什么是“上下文”，以及状态为什么通过 `game->xxx` 调用这些功能。

### 第 5 阶段：集中攻克 `States.cpp`

`States.cpp` 最大，建议分块读：

1. 文件顶部的按钮常量和辅助函数。
2. `UseBomb()`。
3. `UpdateSpawning()`。
4. `HandlePlayerBulletHits()`。
5. `HandleEnemyBulletHits()`。
6. `HandleEnemyBodyHits()`。
7. `CollectItems()`。
8. `CleanupWorld()`。
9. `SaveGameAndNotify()`、`LoadGameAndNotify()`、`DrawSaveSlotCard()`。
10. 最后完整读 `PlayState::Update()` 和 `PlayState::Render()`。

这里建议边读边在纸上写流程，不要一次性从第 1 行读到最后一行。

### 第 6 阶段：看资源和渲染

读：

1. `Resources.h`
2. `Resources.cpp`
3. `RenderUtil.h`
4. `RenderUtil.cpp`

目标是理解：

1. 图片为什么按需加载和缓存？
2. 音效为什么要轮换 alias？
3. 图片黑边为什么要做外侧连通区域遮罩？
4. 中文为什么要从 UTF-8 转宽字符？

### 第 7 阶段：看工程配置

读：

1. `bulid.bat`
2. `.vscode/tasks.json`
3. `.vscode/launch.json`
4. `easyx_ucrt_compat.cpp`

目标是知道编译命令包含哪些源文件、头文件路径、库路径和链接库。

## 14. 按功能反向查代码

以后想查某个功能，不要盲目全文搜索，先按下面入口定位。

| 功能 | 阅读入口 | 继续追踪 |
| --- | --- | --- |
| 程序启动 | `main.cpp` | `Game::ChangeState()` |
| 开始游戏 | `MainMenuState::Update()` | `Game::ResetGame()`、`PlayState::Enter()` |
| 暂停/继续 | `PlayState::Update()`、`PauseState::Update()` | `Game::ChangeState()` |
| 玩家移动 | `Player::Update()` | `IsKeyDown()`、`DragPlayerToMouse()` |
| 玩家射击 | `Player::Update()` | `Bullet` 构造和 `Bullet::Update()` |
| 敌机生成 | `UpdateSpawning()` | `Game::SpawnEnemy()`、`Game::SpawnBoss()` |
| Boss 阶段 | `Enemy::Phase()` | `Enemy::Update()` |
| 玩家子弹命中敌机 | `HandlePlayerBulletHits()` | `Enemy::TakeDamage()`、`Game::KillEnemy()` |
| 敌方子弹命中玩家 | `HandleEnemyBulletHits()` | `Player::Hurt()`、`EnterPlayerDyingIfNeeded()` |
| 敌机撞玩家 | `HandleEnemyBodyHits()` | `RectOverlap()`、`Game::KillEnemy()` |
| 道具掉落 | `Game::KillEnemy()` | `ItemType`、`CollectItems()` |
| 道具拾取 | `CollectItems()` | `PlaySoundEffect()` |
| 炸弹 | `UseBomb()` | `Game::KillEnemy()`、`Enemy::TakeDamage()` |
| 死亡演出 | `EnterPlayerDyingIfNeeded()` | `PlayerDyingState`、`GameOverState` |
| 保存 | `SaveSlotState::Update()` | `SaveGameAndNotify()`、`Game::SaveGame()` |
| 读取 | `SaveSlotState::Update()` | `LoadGameAndNotify()`、`Game::LoadGame()` |
| 背景绘制 | `Game::RenderBackground()` | `DrawImageCover()` |
| 图片绘制 | `Player::Render()`、`Enemy::Render()` | `DrawCenteredSprite()` |
| 中文输出 | 菜单/HUD 绘制函数 | `OutTextUtf8()` |

## 15. 常见修改入口

如果以后要做小改动，可以从这里开始。

| 想改什么 | 优先文件 | 注意点 |
| --- | --- | --- |
| 调窗口大小 | `Common.h` | 修改 `SCREEN_W/SCREEN_H` 后检查 UI 缩放 |
| 改玩家速度 | `Entities.h` 中 `Player::speed` | 单位是像素/秒 |
| 改玩家射速 | `Player::shootCooldown` | 越小射速越快 |
| 改敌机属性 | `Entities.cpp` 的 `ENEMY_PROFILES` | 顺序对应 `type` |
| 改 Boss 出现节奏 | `Game.h` 的 `nextBossKills` 初始值和 `Game::KillEnemy()` | 也要看 `UpdateSpawning()` |
| 改掉落概率 | `Game::KillEnemy()` | 普通敌机随机掉落逻辑在这里 |
| 加新图片资源 | `Resources.h` 的 `GameRes` 和 `AssetID`，`Resources.cpp` 的 `GetAssetPath()` | 三处要保持一致 |
| 加新状态 | `States.h/.cpp` | 继承 `State`，实现 `Update/Render`，在合适位置 `ChangeState` |
| 加新存档字段 | `GameSaveData`、`SaveGame()`、`ReadGameSaveData()`、`ClampLoadedData()` | 写入和读取顺序必须一致 |
| 改菜单按钮 | `States.cpp` 顶部按钮常量和对应 `Render/Update` | 点击区域和绘制位置要同步 |
| 改编译选项 | `bulid.bat` 和 `.vscode/tasks.json` | 两边最好保持一致 |

## 16. 代码阅读时的关键模式

### 16.1 按键边沿判断

很多地方不是判断“键是否按下”，而是判断“这一帧刚按下”：

```text
now && !prev
```

这样可以避免长按一次触发很多次。典型变量有：

```text
pPrev
bPrev
lbuttonPrev
rbuttonPrev
enterPrev
escPrev
```

### 16.2 `alive` 标记和统一清理

实体通常不会在遍历中立刻删除，而是：

```text
对象.alive = false
遍历结束后 erase(remove_if(...), end())
```

这样可以减少遍历时删除元素带来的迭代混乱。

### 16.3 点碰撞和矩形碰撞

| 函数 | 用途 |
| --- | --- |
| `PointInRect()` | 子弹命中、道具拾取、按钮点击 |
| `RectOverlap()` | 飞机本体之间的碰撞 |

### 16.4 资源判空

图片加载失败时，很多渲染函数会使用兜底图形：

```text
如果 img 存在 -> 绘制图片
否则 -> 绘制简单几何形状
```

这让资源缺失时游戏仍然能运行到一定程度，便于调试。

## 17. 学习检查清单

学完后建议你能不用看文档回答这些问题：

1. `main.cpp` 每一帧做了哪几件事？
2. `Game` 和 `State` 的关系是什么？
3. 项目里有哪些 `State` 派生类？
4. 为什么 Boss 不是一个单独的派生类？
5. `Player` 和 `Enemy` 的子弹分别存在哪里？
6. 玩家自动射击在哪个函数里？
7. 普通敌机和 Boss 的属性在哪里初始化？
8. Boss 阶段由什么决定？
9. 子弹命中敌机后，分数在哪里增加？
10. 道具在哪里生成，在哪里拾取？
11. 存档保存了哪些数据，没有保存哪些数据？
12. 读档后为什么要清空敌人、道具和动画？
13. 图片为什么不会每帧重新从磁盘读取？
14. 图片黑边是怎么处理的？
15. 中文文本为什么不用普通 `outtextxy` 直接输出？
16. 音效为什么要用多个 alias？
17. 如果要加一个新道具，至少需要改哪些文件？
18. 如果要加一个新界面，至少需要改哪些文件？

## 18. 最推荐的源码阅读路线

如果你只想要一个最短路径，就按这个顺序来：

```text
README.md
Common.h
main.cpp
State.h
Game.h
States.h
Entities.h
main.cpp 再看一遍
Game.cpp: ChangeState / ResetGame
States.cpp: MainMenuState
States.cpp: PlayState::Update
Entities.cpp: Player::Update
Entities.cpp: Enemy::Update
States.cpp: 碰撞和道具辅助函数
Game.cpp: KillEnemy / SaveGame / LoadGame
Resources.cpp
RenderUtil.cpp
bulid.bat
```

这一轮读完后，再按功能反向查代码。项目不算大，但 `States.cpp` 信息密度高，最好分功能读，不要硬从头读到尾。
