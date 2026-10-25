# 由Lostine/wartool项目精简
# 地址：https://github.com/lostine/wartool

# War3Camera

魔兽争霸3 辅助工具（Miles 插件，注入 `war3.exe`）进行“视距调整”。

## 功能特性

- **视距调整** - 调整游戏镜头距离，`+`/`-` 键放大/缩小，`*` 键重置

## 使用方法

> **仓库已附带编译好的成品 `output\War3Camera.m3d`**，无需自行编译，直接使用即可。

1. 将 `output\War3Camera.m3d` 复制到魔兽争霸3 `...\Warcraft III\redist\miles` 目录下
2. 正常启动魔兽争霸3即可，**无需手动添加 `-window` 参数**，工具会自动加载并应用所有功能；不依赖任何外部 DLL

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
MSBuild War3Camera.sln /p:Configuration=Release /p:Platform=Win32
```

- 编译完成后成品 `War3Camera.m3d` 生成在根目录 `output\` 中
- 中间产物统一在根目录 `build\`（与 `output` 同级），从解决方案或直接构建 vcxproj 均一致

## 项目结构

```
War3Camera/
├── src/                    # 源代码
│   ├── dllmain.cpp         # DLL 入口：附加时加载插件、分离时卸载
│   ├── helper.cpp/hpp      # 插件管理器（统一启停）+ 游戏启动配置（Allow Local Files 等注册表写入）
│   ├── game/               # 游戏抽象层
│   │   ├── warcraft.cpp    # 窗口查找、版本识别、内存补丁、Detours 钩子、版本偏移表
│   │   ├── warcraft.hpp    # Game / System / Offsets 定义（GetOffsets 按版本取偏移表）
│   │   └── version.hpp     # 版本定义（1.24e / 1.27a）
│   └── plugin/             # 插件目录（统一实现 IPlugin 接口：Start/Stop）
│       ├── plugin.hpp      # 插件统一接口
│       └── camera.cpp/hpp      # 视距调整
├── 3rd/                    # 第三方库
│   └── Detours/            # Microsoft Detours
├── output/                 # 编译输出（含成品 wartool.m3d）
├── build/                  # 编译中间产物（临时，可删除）
└── War3Camera.sln             # 解决方案文件
```

## 键位汇总

| 按键 | 功能 |
|------|------|
| `+`（主键盘或小键盘） | 放大视距（看得更近） |
| `-`（主键盘或小键盘） | 缩小视距（看得更远） |
| `*`（小键盘） | 重置视距为默认 1650 |

## 技术细节（逆向信息）

> 以下偏移均为 **RVA（相对 `Game.dll` 模块基址）**，已通过 IDA 逆向确认。后续逆向可据此直接定位。

### 偏移集中管理

所有逆向外挂偏移统一存放在 `src/game/warcraft.cpp` 的 `kOffsetsV124e` / `kOffsetsV127a` 两张表中，由 `GetOffsets(version)` 按版本返回。各插件只通过 `Offsets` 结构字段取用，**新增/修改偏移只需改动 warcraft.cpp 一处**：

### 视距调整

通过内存特征扫描定位相机距离参数表：

- 特征值 `0x44CE4000` = `1650.0f`（默认镜头距离）
- 相邻校验值：`0x42C80000` = `100.0f`（调节步长）、`0x461C4000` = `10000.0f`（视距范围上限）、`0x44C80000` = `1600.0f` / `0x44BB8000` = `1500.0f` / `0x44AF0000` = `1400.0f`（相邻预设视距序列）

- **放大视距**（看得更近）：按 `+` 键，距离减 100
- **缩小视距**（看得更远）：按 `-` 键，距离加 100
- **重置视距**：按 `*` 键，恢复 1650
- **调整范围**：1050 ~ 3050（默认 1650）
- 写入后向游戏窗口发送 `WM_SYSKEYDOWN`（PageUp/PageDown）让相机立即生效

## 许可证

MIT License

## 致谢

- [WarcraftHelper](https://github.com/LoveBeforT/WarcraftHelper) - FPS显示、帧率解锁、字体重叠修复
- [Microsoft Detours](https://github.com/microsoft/Detours) - 用于函数钩子
- [war3_tool](https://github.com/llxiaoyuan/war3_tool) - 视距调整实现参考
