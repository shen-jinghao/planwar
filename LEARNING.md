# 飞机大战代码学习文档

## 一、学习定位

本项目代码主要由 AI 辅助实现，后续不会再新增功能，也不会主动修改现有功能。因此本学习文档的主要目标不是继续开发，而是帮助小组成员熟悉代码结构、理解功能流程，并在期末考核中应对教师随机删除部分代码后要求现场补全的情况。

学习时不建议逐行死记硬背，而应重点理解“这个函数负责什么”“上下文需要哪些变量”“常见代码模式怎么写”“某个功能从哪个入口开始执行”。这样即使考核时删掉一小段代码，也能根据模块职责和前后逻辑补回来。

学习目标如下：

| 学习目标 | 具体要求                                                           |
| -------- | ------------------------------------------------------------------ |
| 看懂结构 | 能说清 `main.cpp`、状态机、实体系统、资源系统和渲染工具之间的关系  |
| 看懂流程 | 能从一次按键、一次射击、一次碰撞或一次存档追踪到对应函数           |
| 看懂分工 | 能按成员负责内容说明每个模块的职责和关键函数                       |
| 能够补全 | 能根据上下文补全常见逻辑，如状态切换、碰撞判断、资源获取、存档读写 |
| 能够解释 | 能向教师说明某段代码为什么这样写，以及它和其它模块如何配合         |

推荐阅读顺序：先读整体流程，再读成员负责模块，最后按功能点反向追代码。复习时应优先掌握函数入口、关键变量和常见代码模板。

## 二、项目结构速览

| 文件或模块     | 负责人 | 学习重点                                             |
| -------------- | ------ | ---------------------------------------------------- |
| `main.cpp`     | 申璟皓 | 程序入口、窗口初始化、主循环、帧时间 `dt`            |
| `State.h`      | 申璟皓 | 状态机基类，理解 `Enter`、`Exit`、`Update`、`Render` |
| `States.*`     | 申璟皓 | 主菜单、存档、暂停、结束、局内游戏流程               |
| `Entities.*`   | 申璟皓 | 玩家、敌机、Boss、子弹、道具、动画效果               |
| `Resources.*`  | 申璟皓 | 图片路径、图片缓存、背景音乐、音效播放               |
| `Common.*`     | 申璟皓 | 屏幕常量、缩放函数、碰撞检测、按键判断               |
| `Game.*`       | 张思媛 | 全局游戏数据、存档、刷怪、击杀结算、通用绘制         |
| `RenderUtil.*` | 李欣雅 | 图片去黑边、背景铺满、中文文本、文字阴影             |
| `bulid.bat`    | 申璟皓 | 编译命令、EasyX 和 MinGW 链接参数                    |
| `.vscode/`     | 申璟皓 | VS Code 编译、运行、调试配置                         |

## 三、整体运行流程

程序从 `main.cpp` 开始执行。主函数完成窗口初始化后创建 `Game` 对象，并把当前状态切换到 `MainMenuState`。

核心流程可以这样理解：

```text
main.cpp
  -> initgraph 创建窗口
  -> BeginBatchDraw 开启批量绘图
  -> Game game
  -> game.ChangeState(new MainMenuState(&game))
  -> while (game.running)
       -> 计算 dt
       -> game.current->Update(dt)
       -> game.current->Render()
       -> FlushBatchDraw()
```

进入局内游戏后，主要流程由 `PlayState::Update()` 驱动：

```text
PlayState::Update(dt)
  -> 处理暂停、存档、鼠标拖动、炸弹输入
  -> game->player.Update(dt)
  -> UpdateSpawning(game, dt)
  -> UpdateEnemies(game, dt)
  -> HandlePlayerBulletHits(game)
  -> HandleEnemyBulletHits(game)
  -> HandleEnemyBodyHits(game)
  -> CollectItems(game, dt)
  -> game->UpdateEffects(dt)
  -> CleanupWorld(game)
```

阅读代码时，建议始终记住：状态机负责“当前处在哪个界面”，实体负责“对象自己怎么动、怎么画”，`Game` 负责“全局数据和跨状态复用逻辑”。

### 期末考核补全思路

如果教师随机删除代码，通常不会要求临场设计新功能，而是考查是否理解已有代码的结构和上下文。补全时可以按以下顺序判断：

| 步骤 | 判断内容                                                                                 |
| ---- | ---------------------------------------------------------------------------------------- |
| 1    | 先看当前函数名，判断它属于状态机、实体、资源、渲染还是全局管理                           |
| 2    | 看缺失代码前后的变量，例如 `game`、`dt`、`player`、`enemies`、`items`、`rm`              |
| 3    | 判断缺失位置是在更新逻辑、绘制逻辑、碰撞逻辑、存档逻辑还是资源逻辑                       |
| 4    | 回忆同类代码模式，例如 `if` 判空、`for` 遍历、`erase/remove_if` 清理、`ChangeState` 切换 |
| 5    | 补全后检查是否保持原有功能，不额外增加新功能或改变游戏规则                               |

最需要熟悉的不是所有细节，而是常见代码模板：状态切换、按键边沿判断、碰撞判断、实体遍历、资源获取、存档读写、绘制调用和无效对象清理。

## 四、张思媛负责内容学习：Game.h 与 Game.cpp

### 1. 模块职责

`Game.*` 是游戏全局上下文模块。它不负责某一个界面的按钮点击，而是保存游戏中所有状态都可能用到的数据，并提供通用操作。

重点理解以下职责：

| 职责      | 对应内容                                                              |
| --------- | --------------------------------------------------------------------- |
| 全局数据  | 玩家、敌人、道具、动画、分数、击杀数、关卡、背景编号                  |
| 状态切换  | `ChangeState(State *s)`                                               |
| 新游戏    | `ResetGame()`                                                         |
| 存档      | `SaveGame()`、`LoadGame()`、`ReadGameSaveData()`                      |
| 刷怪      | `SpawnEnemy()`、`SpawnBoss()`                                         |
| Boss 管理 | `BossExists()`、`GetBoss()`                                           |
| 击杀结算  | `KillEnemy()`                                                         |
| 动画管理  | `AddEnemyExplosion()`、`AddPlayerDeathExplosion()`、`UpdateEffects()` |
| 通用绘制  | `RenderBackground()`、`RenderWorld()`、`RenderBossHpBar()`            |

### 2. 重点数据结构

`GameSaveData` 是存档数据结构。它保存的是“读档后需要恢复”的核心数据，例如玩家状态、分数、击杀数、关卡、背景、下一次 Boss 触发条件和刷怪计时。

`Game` 结构体是整个游戏的共享上下文，状态机、实体系统和绘制系统都通过它访问共享数据。

学习时可以重点回答这些问题：

| 问题                               | 建议追踪位置                            |
| ---------------------------------- | --------------------------------------- |
| 新游戏时哪些数据会被清空？         | `Game::ResetGame()`                     |
| 存档文件里保存了哪些内容？         | `Game::SaveGame()`                      |
| 读档后为什么要清空敌人和道具？     | `Game::LoadGame()`                      |
| Boss 什么时候出现？                | `Game::SpawnBoss()`、`UpdateSpawning()` |
| 击杀 Boss 后为什么会掉落多个道具？ | `Game::KillEnemy()`                     |
| Boss 血条怎么画出来？              | `Game::RenderBossHpBar()`               |

### 3. 学习路线

先读 `Game.h`，弄清 `Game` 里有哪些成员变量。再读 `Game.cpp`，建议按以下顺序阅读：

1. `ResetGame()`：理解一局游戏的初始状态。
2. `SaveGame()` 和 `LoadGame()`：理解存档格式和数据恢复方式。
3. `SpawnEnemy()` 和 `SpawnBoss()`：理解普通敌机和 Boss 如何进入场景。
4. `KillEnemy()`：理解得分、击杀数、关卡和道具掉落。
5. `RenderWorld()` 和 `RenderBossHpBar()`：理解全局绘制如何调度实体绘制。

### 4. 考核补全重点

| 可能被删内容               | 需要回忆的位置和写法                                       |
| -------------------------- | ---------------------------------------------------------- |
| 新游戏重置语句             | `Game::ResetGame()` 中清空实体、分数、关卡和计时           |
| 存档写入顺序               | `Game::SaveGame()` 中按固定顺序写入字段                    |
| 存档读取和校验             | `ReadGameSaveData()` 与 `ClampLoadedData()`                |
| 普通敌机和 Boss 生成       | `SpawnEnemy()`、`SpawnBoss()`                              |
| 敌机死亡后的分数和掉落     | `Game::KillEnemy()`                                        |
| 动画清理逻辑               | `Game::UpdateEffects()` 中 `remove_if` 的写法              |
| 背景、世界和 Boss 血条绘制 | `RenderBackground()`、`RenderWorld()`、`RenderBossHpBar()` |

## 五、李欣雅负责内容学习：RenderUtil.h 与 RenderUtil.cpp

### 1. 模块职责

`RenderUtil.*` 是渲染辅助模块，主要解决 EasyX 使用中的底层绘制问题。其它模块只需要调用封装好的函数，不必每次都直接操作像素缓冲区或 Windows GDI。

重点函数如下：

| 函数                                  | 作用                         |
| ------------------------------------- | ---------------------------- |
| `DrawSpriteWithoutOuterBlack()`       | 绘制图片并去掉外侧黑色背景   |
| `DrawCenteredSprite()`                | 按中心点绘制图片             |
| `DrawCenteredSpriteRot180()`          | 按中心点旋转 180 度绘制图片  |
| `DrawImageCover()`                    | 将背景图片等比裁剪并铺满窗口 |
| `TextWidthUtf8()`、`TextHeightUtf8()` | 计算 UTF-8 中文文本尺寸      |
| `OutTextUtf8()`                       | 输出 UTF-8 中文文本          |
| `OutTextUtf8Shadow()`                 | 输出带阴影的中文文本         |
| `SetBlackText()`、`SetWhiteText()`    | 设置常用文字样式             |

### 2. 图片去黑边逻辑

部分 PNG 素材虽然看起来像透明背景，但实际可能带黑底。直接绘制会出现黑边。该模块通过 `GetOuterBlackMask()` 从图片四周开始搜索外侧连通的近黑色区域，并在绘制时跳过这些像素。

学习路径：

```text
DrawCenteredSprite()
  -> DrawSpriteWithoutOuterBlack()
     -> DrawSpriteMasked()
        -> GetOuterBlackMask()
```

这里最重要的思想是：只去掉“从边缘连通进来的黑色区域”，尽量保留图片内部正常的黑色细节。

### 3. 中文绘制逻辑

项目中的菜单、HUD 和提示信息含有中文。如果直接使用普通输出，可能因为编码问题出现乱码。`RenderUtil` 的做法是：

```text
UTF-8 const char *
  -> MultiByteToWideChar()
  -> std::wstring
  -> TextOutW()
```

这样可以让中文在 EasyX 窗口中稳定显示。

### 4. 考核补全重点

| 可能被删内容       | 需要回忆的位置和写法                                 |
| ------------------ | ---------------------------------------------------- |
| UTF-8 转宽字符     | `ToWideText()` 中 `MultiByteToWideChar` 的调用流程   |
| 图片黑边遮罩缓存   | `GetOuterBlackMask()` 中 `maskCache`、队列和边缘搜索 |
| 像素级图片绘制     | `DrawSpriteMasked()` 中源像素、目标像素和越界判断    |
| 居中绘制           | `DrawCenteredSprite()` 中用图片宽高计算左上角        |
| 180 度旋转绘制     | `DrawCenteredSpriteRot180()` 与 `rotate180` 坐标反转 |
| 背景等比铺满       | `DrawImageCover()` 中宽高比比较、裁剪和 `StretchBlt` |
| 中文文字和阴影输出 | `OutTextUtf8()`、`OutTextUtf8Shadow()`               |

## 六、申璟皓负责内容学习：其余核心模块

申璟皓负责的部分覆盖面最大，建议按“入口 -> 状态 -> 实体 -> 资源 -> 工具”的顺序学习。

### 1. main.cpp：程序入口

重点理解：

| 内容                                         | 说明                                       |
| -------------------------------------------- | ------------------------------------------ |
| `initgraph(SCREEN_W, SCREEN_H)`              | 创建 EasyX 窗口                            |
| `BeginBatchDraw()`                           | 开启双缓冲，减少闪烁                       |
| `Game game`                                  | 创建全局游戏上下文                         |
| `game.ChangeState(new MainMenuState(&game))` | 进入主菜单                                 |
| `dt`                                         | 每帧间隔时间，用于让移动速度不依赖机器性能 |
| `FlushBatchDraw()`                           | 刷新画面                                   |
| `UnloadAll()`、`StopBackgroundMusic()`       | 退出时清理资源                             |

### 2. State.h 与 States.*：状态机和界面流程

状态机是项目最重要的流程组织方式。每个状态代表一个界面或阶段。

| 状态               | 作用                                                 |
| ------------------ | ---------------------------------------------------- |
| `MainMenuState`    | 主菜单、开始游戏、读取存档、退出、制作名单、背景选择 |
| `SaveSlotState`    | 3 个存档槽的保存、读取和返回                         |
| `PauseState`       | 暂停覆盖层、继续游戏、进入存档、返回主菜单           |
| `GameOverState`    | 游戏结束界面、重新开始、读取存档、返回主菜单         |
| `PlayerDyingState` | 玩家死亡后的短暂爆炸演出                             |
| `PlayState`        | 局内玩法主状态                                       |

建议重点阅读 `PlayState::Update()`，因为它串联了局内大部分功能。

局内功能追踪表：

| 功能            | 入口函数                                    |
| --------------- | ------------------------------------------- |
| 暂停            | `PlayState::Update()` 中检测 `P` 或暂停按钮 |
| 鼠标拖动玩家    | `DragPlayerToMouse()`                       |
| 使用炸弹        | `UseBomb()`                                 |
| 刷新敌机和 Boss | `UpdateSpawning()`                          |
| 玩家子弹打敌人  | `HandlePlayerBulletHits()`                  |
| 敌方子弹打玩家  | `HandleEnemyBulletHits()`                   |
| 敌机撞玩家      | `HandleEnemyBodyHits()`                     |
| 拾取道具        | `CollectItems()`                            |
| 清理死亡对象    | `CleanupWorld()`                            |

### 3. Entities.*：实体系统

实体系统负责“游戏对象自身的行为”。

| 实体              | 重点学习内容                                 |
| ----------------- | -------------------------------------------- |
| `AnimationEffect` | 爆炸动画计时、帧切换、兜底绘制               |
| `Bullet`          | 子弹位置更新、出界判断、按类型绘制           |
| `Item`            | 道具下落、出界判断、按类型绘制               |
| `Player`          | 键盘移动、自动射击、无敌时间、火力强化、受伤 |
| `Enemy`           | 普通敌机、Boss、血量、移动、射击、阶段变化   |

重点阅读顺序：

1. `Player::Update()`：玩家如何移动和自动射击。
2. `Player::Hurt()`：玩家受伤和无敌时间。
3. `Enemy::Enemy()`：不同敌机类型如何初始化。
4. `Enemy::Update()`：普通敌机和 Boss 行为差异。
5. `Enemy::Phase()`：Boss 阶段如何由生命值决定。
6. `Bullet::Update()` 和 `Item::Update()`：简单实体如何更新与出界。

### 4. Resources.*：资源与音频

资源系统负责把图片路径、图片缓存和音频播放集中起来。

| 内容                               | 作用                     |
| ---------------------------------- | ------------------------ |
| `GameRes res`                      | 保存图片和音频路径       |
| `AssetID`                          | 用枚举表示图片资源       |
| `GetAssetPath()`                   | 将资源 ID 映射为文件路径 |
| `ResourceManager::LoadImageFile()` | 按需加载图片并缓存       |
| `ResourceManager::GetImage()`      | 按资源 ID 获取图片       |
| `PlayBackgroundMusic()`            | 循环播放背景音乐         |
| `PlaySoundEffect()`                | 播放一次性音效           |
| `UnloadAll()`                      | 退出时释放图片缓存       |

学习重点：为什么图片要缓存？因为如果每帧都从磁盘加载图片，会严重影响性能。

### 5. Common.*：通用工具

`Common.*` 放的是各模块都会用到的小工具。

| 函数或常量                           | 作用                                       |
| ------------------------------------ | ------------------------------------------ |
| `SCREEN_W`、`SCREEN_H`               | 当前窗口尺寸                               |
| `ScaleX()`、`ScaleY()`、`ScaleLen()` | 将设计稿尺寸缩放到实际窗口                 |
| `FileExists()`                       | 判断资源或存档文件是否存在                 |
| `PointInRect()`                      | 判断点是否在矩形内，用于子弹命中和按钮点击 |
| `RectOverlap()`                      | 判断两个矩形是否重叠，用于机体碰撞         |
| `IsKeyDown()`                        | 判断键盘或鼠标按键是否按下                 |
| `IsNearBlack()`                      | 判断像素是否接近黑色，用于去黑边           |

## 七、按功能反向学习代码

如果只按文件读，容易迷路。更推荐按功能追踪。

### 1. 开始游戏

```text
MainMenuState::Update()
  -> 点击“开始游戏”或按 Enter
  -> game->ResetGame()
  -> game->ChangeState(new PlayState(game))
  -> PlayState::Enter()
```

学习重点：状态切换时 `Exit()` 和 `Enter()` 的调用顺序。

### 2. 玩家移动与射击

```text
PlayState::Update()
  -> game->player.Update(dt)
     -> 检测 WASD / 方向键
     -> 限制玩家不出屏幕
     -> shootTimer 达到冷却时间后创建 Bullet
```

学习重点：`dt` 如何影响移动速度，`shootCooldown` 如何控制射速。

### 3. 敌机生成与 Boss 出现

```text
PlayState::Update()
  -> UpdateSpawning(game, dt)
     -> kills >= nextBossKills 时 SpawnBoss()
     -> 否则按 spawnInterval 生成普通敌机
```

学习重点：普通敌机刷新和 Boss 刷新互斥，Boss 存在时不继续刷普通敌机。

### 4. 子弹命中敌机

```text
HandlePlayerBulletHits(game)
  -> 遍历 enemies
  -> 遍历 player.bullets
  -> PointInRect 判断命中
  -> e.TakeDamage(1)
  -> hp <= 0 时 game->KillEnemy(e, true)
```

学习重点：命中检测后子弹会被标记为不存活，敌机死亡后进入统一结算。

### 5. 玩家受伤与死亡

```text
HandleEnemyBulletHits(game) / HandleEnemyBodyHits(game)
  -> player.Hurt()
  -> EnterPlayerDyingIfNeeded()
  -> PlayerDyingState
  -> GameOverState
```

学习重点：玩家死亡不是立刻进入 GameOver，而是先播放死亡演出。

### 6. 道具拾取

```text
CollectItems(game, dt)
  -> i.Update(dt)
  -> PointInRect 判断玩家拾取
  -> Health 增加生命
  -> Bomb 增加炸弹
  -> Upgrade 开启火力强化
```

学习重点：道具效果直接修改 `game->player` 中的数据。

### 7. 存档与读档

```text
SaveSlotState::Update()
  -> SaveGameAndNotify()
     -> game->SaveGame(path)

SaveSlotState::Update()
  -> LoadGameAndNotify()
     -> game->LoadGame(path)
        -> ReadGameSaveData(path, data)
        -> ClampLoadedData(data)
```

学习重点：存档只保存核心进度，不保存当前场上所有敌人、子弹和道具。

### 8. 高频补全模板

以下代码模式在项目中反复出现，考核时如果被删，优先按这些模板恢复。

| 代码模式     | 常见位置                                 | 记忆要点                                                       |
| ------------ | ---------------------------------------- | -------------------------------------------------------------- |
| 状态切换     | `Game::ChangeState()`、各状态 `Update()` | 先 `Exit()`，再 `reset`，最后 `Enter()`                        |
| 按键边沿判断 | `PlayState::Update()`、菜单状态          | 用 `now && !prev` 防止长按重复触发                             |
| 遍历实体     | `UpdateEnemies()`、碰撞处理              | `for (auto &e : game->enemies)`                                |
| 碰撞检测     | 子弹、道具、机体碰撞                     | 点命中用 `PointInRect()`，矩形重叠用 `RectOverlap()`           |
| 死亡清理     | 子弹、敌机、道具、动画                   | `alive = false` 后用 `erase(remove_if(...), end())`            |
| 资源获取     | 实体和界面绘制                           | `rm.GetImage(AssetID::...)` 或 `resources.LoadImageFile(path)` |
| 中文输出     | 菜单、HUD、提示                          | 先 `SetWhiteText()` 或 `SetBlackText()`，再 `OutTextUtf8()`    |
| 音效播放     | 道具、炸弹、结束                         | `PlaySoundEffect(res.xxx)`                                     |
| 存档读写     | `Game.cpp`                               | 保存和读取字段顺序必须一致                                     |

## 八、常见问题与学习时的定位方法

| 问题         | 优先查看位置                            | 定位思路                                         |
| ------------ | --------------------------------------- | ------------------------------------------------ |
| 游戏无法编译 | `bulid.bat`                             | 检查 MinGW 和 EasyX 路径是否存在                 |
| 图片不显示   | `Resources.cpp`、`Resources.h`          | 检查资源路径、文件名和 `AssetID` 映射            |
| 图片有黑边   | `RenderUtil.cpp`                        | 检查是否使用了 `DrawCenteredSprite()` 系列函数   |
| 中文乱码     | `RenderUtil.cpp`                        | 检查是否通过 `OutTextUtf8()` 输出                |
| 音乐不播放   | `Resources.cpp`                         | 检查文件是否存在、MCI 类型是否正确               |
| 存档失败     | `Game.cpp`、存档文件路径                | 检查工作目录和文件写入权限                       |
| Boss 不出现  | `UpdateSpawning()`、`Game::SpawnBoss()` | 检查 `kills` 和 `nextBossKills`                  |
| 玩家碰撞异常 | `Common.cpp`、`States.cpp`              | 检查 `PointInRect()`、`RectOverlap()` 和实体宽高 |
| 炸弹无效果   | `UseBomb()`                             | 检查 `player.bombs` 是否大于 0                   |

## 九、建议的模拟补全任务

可以按由浅入深的顺序进行“遮住代码后口述/手写补全”的练习。练习目的不是实际修改项目功能，而是熟悉已有代码，确保考核时能恢复被删内容。

| 难度 | 模拟补全任务                                  | 目的                               |
| ---- | --------------------------------------------- | ---------------------------------- |
| 简单 | 遮住 `Player::Hurt()`，口述受伤逻辑           | 熟悉生命扣减和无敌时间             |
| 简单 | 遮住 `Bullet::Update()`，补全移动和出界判断   | 熟悉 `dt` 和 `alive` 标记          |
| 简单 | 遮住 `PointInRect()`，补全点与矩形判断        | 熟悉按钮点击和子弹命中基础         |
| 中等 | 遮住 `Game::ResetGame()`，补全重置内容        | 熟悉一局游戏初始状态               |
| 中等 | 遮住 `Game::KillEnemy()`，补全击杀结算        | 熟悉分数、击杀数、掉落和 Boss 奖励 |
| 中等 | 遮住 `HandlePlayerBulletHits()`，补全命中流程 | 熟悉双重遍历、扣血和击杀           |
| 中等 | 遮住 `CollectItems()`，补全三类道具效果       | 熟悉道具类型和玩家状态             |
| 较难 | 遮住 `Enemy::Update()` 中 Boss 弹幕部分       | 熟悉 Boss 阶段和子弹生成           |
| 较难 | 遮住 `ReadGameSaveData()`，补全存档读取流程   | 熟悉文件流、版本标识和字段顺序     |
| 较难 | 遮住 `DrawSpriteMasked()`，补全像素绘制逻辑   | 熟悉图片去黑边和越界判断           |

## 十、学习检查清单

完成学习后，建议能独立回答以下问题：

1. `main.cpp` 的主循环每一帧做了什么？
2. 为什么项目要使用状态机？
3. `Game` 和 `State` 的关系是什么？
4. 玩家自动射击在哪里实现？
5. 普通敌机和 Boss 的区别在哪里初始化？
6. Boss 阶段由什么决定？
7. 子弹命中敌机后，分数在哪里增加？
8. 道具掉落概率在哪里控制？
9. 存档文件保存了哪些数据，哪些数据没有保存？
10. 为什么读档后要调用数据范围修正？
11. 图片黑边是怎么去掉的？
12. 中文文本为什么要转成宽字符？
13. 音效为什么使用多个 alias 播放？
14. 如果按钮点击相关代码被删，应该优先看哪个文件和哪些辅助函数？
15. 如果资源获取相关代码被删，应该优先看 `Resources.h` 还是 `Resources.cpp`？

## 十一、总结

这份代码虽然主要由 AI 辅助实现，但整体结构仍然适合学习 C++ 小型游戏项目的组织方式。学习时应把重点放在模块职责、数据流和功能入口上。

张思媛负责的 `Game.*` 适合学习全局数据管理、存档、刷怪和结算逻辑；李欣雅负责的 `RenderUtil.*` 适合学习底层渲染封装、中文输出和像素级图片处理；申璟皓负责的其余模块适合学习主循环、状态机、实体行为、资源音频和整体功能整合。

建议边运行游戏边阅读代码，每次只追踪一个功能点。复习时可以用“遮住一段代码后口述逻辑”的方式检验自己，而不需要真正改变项目功能。这样比从头到尾逐行阅读更高效，也更容易在考核时补全缺失代码。
