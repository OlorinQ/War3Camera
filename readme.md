# wartool

魔兽争霸3 辅助工具（Miles 插件，注入 `war3.exe`）：无边框窗口全屏、无 Win 键、FPS 显示、帧率解锁、字体重叠修复、视距调整、允许本地文件（mod）。

## 功能特性

- **无边框窗口全屏** - 强制窗口化启动并无边框铺满屏幕，Alt+Tab 流畅切换窗口
- **无 Win 键** - 阻止 Win 键在游戏内触发开始菜单，避免误操作
- **FPS 显示** - 在游戏 HUD 中显示当前帧率
- **帧率解锁** - 解除 60FPS 上限，刷新率上限与显示器一致（支持 Direct3D 与 OpenGL）
- **字体重叠修复** - 高分辨率长时间游玩出现字体重叠时，按 `END` 键重置窗口临时修复
- **视距调整** - 调整游戏镜头距离，`+`/`-` 键放大/缩小，`*` 键重置
- **允许本地文件** - 自动写入 `Allow Local Files=1`，配合 `-loadfromarchive` 启用 mod

## 使用方法

> **仓库已附带编译好的成品 `output\wartool.m3d`**，无需自行编译，直接使用即可。

1. 将 `output\wartool.m3d` 复制到魔兽争霸3 `...\Warcraft III\redist\miles` 目录下
2. 正常启动魔兽争霸3即可，**无需手动添加 `-window` 参数**，工具会自动加载并应用所有功能；不依赖任何外部 DLL

> 注：工具会自动向注册表写入 `Software\Blizzard Entertainment\Warcraft III` 下的 `Allow Local Files = 1`（启用 mod）与 `Gfx Fullscreen = 0`（强制窗口化）。

## 支持版本

- 魔兽争霸3 1.24e
- 魔兽争霸3 1.27a

## 编译说明

### 环境要求

- Visual Studio 2022 或更高版本（工程使用 v145 工具集）
- Windows SDK 10.0
- MSBuild

### 编译步骤

```bash
MSBuild wartool.sln /p:Configuration=Release /p:Platform=Win32
```

- 编译完成后成品 `wartool.m3d` 生成在根目录 `output\` 中
- 中间产物统一在根目录 `build\`（与 `output` 同级），从解决方案或直接构建 vcxproj 均一致

## 项目结构

```
wartool/
├── src/                    # 源代码
│   ├── dllmain.cpp         # DLL 入口：附加时加载插件、分离时卸载
│   ├── helper.cpp/hpp      # 插件管理器（统一启停）+ 游戏启动配置（Allow Local Files 等注册表写入）
│   ├── game/               # 游戏抽象层
│   │   ├── warcraft.cpp    # 窗口查找、版本识别、内存补丁、Detours 钩子、版本偏移表
│   │   ├── warcraft.hpp    # Game / System / Offsets 定义（GetOffsets 按版本取偏移表）
│   │   └── version.hpp     # 版本定义（1.24e / 1.27a）
│   └── plugin/             # 插件目录（统一实现 IPlugin 接口：Start/Stop）
│       ├── plugin.hpp      # 插件统一接口
│       ├── borderless.cpp/hpp  # 无边框窗口全屏
│       ├── nowinkey.cpp/hpp    # 禁用 Win 键
│       ├── showfps.cpp/hpp     # FPS 显示
│       ├── unlockfps.cpp/hpp   # 帧率解锁
│       ├── windowfixer.cpp/hpp # 窗口修复（字体重叠，END 键手动触发）
│       └── camera.cpp/hpp      # 视距调整
├── 3rd/                    # 第三方库
│   └── Detours/            # Microsoft Detours
├── output/                 # 编译输出（含成品 wartool.m3d）
├── build/                  # 编译中间产物（临时，可删除）
└── wartool.sln             # 解决方案文件
```

## 键位汇总

| 按键 | 功能 |
|------|------|
| `END` | 重置窗口，修复字体重叠（手动触发，不自动干预） |
| `+`（主键盘或小键盘） | 放大视距（看得更近） |
| `-`（主键盘或小键盘） | 缩小视距（看得更远） |
| `*`（小键盘） | 重置视距为默认 1650 |

## 技术细节（逆向信息）

> 以下偏移均为 **RVA（相对 `Game.dll` 模块基址）**，已通过 IDA 逆向确认。后续逆向可据此直接定位。

### 偏移集中管理

所有逆向外挂偏移统一存放在 `src/game/warcraft.cpp` 的 `kOffsetsV124e` / `kOffsetsV127a` 两张表中，由 `GetOffsets(version)` 按版本返回。各插件只通过 `Offsets` 结构字段取用，**新增/修改偏移只需改动 warcraft.cpp 一处**：

| 字段 | 1.24e | 1.27a | 说明 |
|------|-------|-------|------|
| `getGameOpt` | `+0x5720` | `+0x23E00` | 游戏偏好设置单例获取函数 |
| `setGameOptValue` | `+0x57F0` | `+0x25A70` | 游戏偏好选项写入函数 |
| `fpsLimitPatch` | `+0x62DF9B` | `+0x5FCFB` | 帧率限制标志补丁点 |
| `d3d9Present` | `+0x62E1E0` | `+0xEC6B0` | D3D9 呈现参数获取函数（钩子点） |
| `gxType` | `+0xA9E764` | `+0xB665C8` | 图形渲染后端类型全局变量 |
| `fpsShowFlag` | `+0x34B248` | `+0x397E5A` | FPS 显示标志字节 |

### 无边框窗口全屏

- 启动时写注册表 `Software\Blizzard Entertainment\Warcraft III\Gfx Fullscreen = 0`，强制游戏以窗口模式初始化，避免独占全屏下无法安全切换窗口。
- 监视线程每 500ms 检查游戏窗口：若带标题栏（`WS_CAPTION`），用 `0xFF3BFFFF` 掩码移除标题栏与可调边框并拉伸铺满屏幕。
- **窗口句柄失效重绑定**：过场视频播放/跳过时游戏可能销毁重建主窗口（视频由 DirectShow 在独立全屏窗口 "Blizzard Player" 中播放，创建函数 RVA `0xE9DB0`；按 ESC 由窗口过程 `DestroyWindow` 关闭）。监听线程检测到句柄失效后自动重新查找主窗口，避免"视频后无法恢复无边框"。

### 允许本地文件（mod）

启动时写注册表 `Software\Blizzard Entertainment\Warcraft III\Allow Local Files = 1`，允许游戏加载本地自定义文件；配合启动参数 `-loadfromarchive` 即可启用地图/资源 mod。与插件开关无关，始终生效。

### FPS 显示

将游戏 HUD 帧率显示标志字节置 1（0=关闭，1=开启）：

- 1.24e: `Game.dll + 0x34B248`
- 1.27a: `Game.dll + 0x397E5A`

### 帧率解锁

解锁 60FPS 上限，共四个逆向点位（两版本）：

| 点位 | 1.24e | 1.27a | 说明 |
|------|-------|-------|------|
| 偏好单例获取函数 | `+0x5720` | `+0x23E00` | 获取游戏偏好设置对象（CPreferencesWar3） |
| 偏好选项写入函数 | `+0x57F0` | `+0x25A70` | 按索引写入偏好值（索引 < 79），用于写入刷新率上限 |
| 帧率限制标志补丁点 | `+0x62DF9B` | `+0x5FCFB` | 改写为 `0xFF`，保持帧率限制标志不被清除 |
| D3D9 呈现参数获取函数 | `+0x62E1E0` | `+0xEC6B0` | 钩子点（GetPresentParameters），强制 `PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE` 关闭垂直同步 |

- 图形后端类型全局变量（决定走 D3D 还是 OpenGL 重置路径）：1.24e `+0xA9E764`、1.27a `+0xB665C8`（0=Direct3D，1=OpenGL）。
- 显示器刷新率 >60Hz 时，通过偏好写入把游戏刷新率上限同步为显示器刷新率。
- OpenGL 模式：调用 `wglSwapIntervalEXT(0)` 关闭垂直同步；Direct3D 模式：最小化再还原窗口重建交换链。
- 独占全屏（窗口无标题栏）下跳过交换链重建，仅保留补丁与呈现参数钩子，避免黑屏/卡死。

### 字体重叠修复

高分辨率（>1080p）长时间游玩后，游戏字形缓存/字体槽状态异常导致文字重叠。按 `END` 键对窗口执行一次 `MoveWindow` 宽度 +1 再还原，触发游戏视口尺寸更新（1.27a 更新函数 RVA `0xCF660`）→ 通知链重建字体系统（清空字号缓存槽），临时恢复。**因属于累积性问题且不宜自动干预，保留手动按键方式。**

### 视距调整

通过内存特征扫描定位相机距离参数表：

- 特征值 `0x44CE4000` = `1650.0f`（默认镜头距离）
- 相邻校验值：`0x42C80000` = `100.0f`（调节步长）、`0x461C4000` = `10000.0f`（视距范围上限）、`0x44C80000` = `1600.0f` / `0x44BB8000` = `1500.0f` / `0x44AF0000` = `1400.0f`（相邻预设视距序列）

- **放大视距**（看得更近）：按 `+` 键，距离减 100
- **缩小视距**（看得更远）：按 `-` 键，距离加 100
- **重置视距**：按 `*` 键，恢复 1650
- **调整范围**：250 ~ 3050（默认 1650）
- 写入后向游戏窗口发送 `WM_SYSKEYDOWN`（PageUp/PageDown）让相机立即生效

## 许可证

MIT License

## 致谢

- [WarcraftHelper](https://github.com/LoveBeforT/WarcraftHelper) - FPS显示、帧率解锁、字体重叠修复
- [Microsoft Detours](https://github.com/microsoft/Detours) - 用于函数钩子
- [war3_tool](https://github.com/llxiaoyuan/war3_tool) - 视距调整实现参考
