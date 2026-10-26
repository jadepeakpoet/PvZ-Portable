# PvZ-Portable Toolkit / 工具箱

The toolkit is compiled into this game; it does not attach to other processes.
It reimplements the behavior of PvZ Toolkit against portable game APIs, using
the game's Widget, ButtonWidget, EditWidget, Graphics and MemoryImage classes.
No Windows memory patches or FLTK implementation were copied.

工具箱直接编译进游戏，只操作当前游戏实例。参考 PvZ Toolkit 的功能分类，
通过游戏自身接口实现，不依赖进程注入或原版游戏的内存地址。

## Open and change language / 打开与切换语言

- Click **Toolkit / 工具箱**, or press **F10** on desktop.
- Click **English / 中文** at the upper right to switch immediately. The choice
  persists under `trainer/settings-v1.txt` in the selected save-data directory.
- Desktop: a separate resizable window; closing it leaves the game running.
- Mobile: an in-game modal panel. Opening pauses simulation; closing restores
  the prior speed/pause selection. The panel does not change game resource language.
- `-trainer` opens the toolkit after loading. `-trainer-mobile` also forces the
  mobile layout on desktop for testing.

点击“工具箱”或桌面端按 F10 打开。右上角可即时切换中英文并记住选择。
手机使用游戏内面板，打开时暂停战场，关闭后恢复先前状态；不会改动游戏语言。

## Controls / 操作

The five tabs are Resources, Battlefield, Lineup, Spawning and Utilities. Use
the left list and its page buttons/mouse wheel to select an operation. Switches
apply immediately; numeric actions require **Apply / 应用**. Only relevant input
fields are displayed. Actions requiring a board/profile are unavailable until
that context exists.

五页分别为资源、战场、阵型、出怪、杂项。左侧列表支持翻页和鼠标滚轮；
开关立即生效，数值操作需点击“应用”。按钮灰色表示当前场景不可用。

- **Types / 类型列表** selects named plants, zombies or modes. For spawning,
  clicking zombie names adds/removes IDs in the text editor; click Done afterward.
- Rows and columns are **1-based** in the UI. Type IDs remain **0-based**.
  The spawn-generation form labels its red-eye limit/weight separately.
- **Place on lawn / 点击战场放置** starts continuous placement. Click/tap cells;
  the highlighted cell previews the target. Right-click, Escape or **Done / 完成**
  ends placement. Switching levels cancels it.
- The multiline text editor accepts Enter, selection, copy/paste and arrow keys.
  Copy/Paste buttons also work on touch devices.
- **All effects off / 关闭全部效果** resets switches and speed. It does not undo
  changed sun/coins, created objects, cleared objects or unlocked profile progress.
- Gameplay switches start **off** on each launch. Recording/playback hides the
  tools and disables modifications to preserve the original input stream.

类型列表可直接选择中文或英文名称；出怪页可多选。行列从 1 开始，类型编号从 0 开始。
连续放置模式支持鼠标和触摸，切关自动退出。“关闭全部效果”不会撤销已执行的数值、
布阵、清场和进度修改；每次启动时持续效果默认关闭。录制和回放期间禁用工具箱。

## Feature coverage / 功能范围

| Page / 页面 | Functions / 功能 |
|---|---|
| Resources / 资源 | Sun/cap, coins, collection/loot, garden supplies, tree height, free planting, independent card cooldown, placement rules, conveyor, shovel, level/mode, survival stage, unlock, win / 阳光与上限、金币、自动收集与掉落、花园用品、智慧树、免费种植、独立冷却、任意放置、传送带、连续铲子、模式关卡、生存阶段、解锁与过关 |
| Battlefield / 战场 | Plants/imitaters, zombies, graves, ladders, rake, mowers, clearing, invulnerability/weakness, reload, butter, spawning/movement, awake mushrooms, crater/ice/blast controls / 放植物与模仿者、僵尸、墓碑、梯子、钉耙、小推车、分类清场、无敌与脆弱、装填、黄油、刷怪与移动、蘑菇、弹坑冰道爆炸 |
| Lineup / 阵型 | Seed slots, smart ladders, pool/roof bases, scenes, compressed/text Toolkit codes, YAML list entry selection, import/export, screenshots / 卡槽、智能搭梯、睡莲花盆、场地、压缩及文本阵型码、YAML 条目选择、导入导出、截图 |
| Spawning / 出怪 | Native, uniform or weighted generation, red-eye limits/weights, wave text editing, import/export and chooser preview refresh / 原生、均匀、加权出怪，红眼限制与权重，逐波编辑、导入导出、选卡预览刷新 |
| Utilities / 杂项 | Music, fog/vases, background running, read-only saves, hidden page, save folder, PAK tools, debug, speed/pause, documentation/about / 音乐、雾与罐子、后台、存档只读、隐藏页、存档目录、PAK、调试、速度暂停、文档与关于 |

Terrain-incompatible objects and unsafe counts are rejected. Native wave
generation requires normal zombie ID 0 because the engine's early-wave picker
needs an eligible fallback. Scene/lineup operations apply to ordinary lawns,
not the garden/tree/aquarium. No-ice prevents new trails; existing ice expires
normally. Read-only saves blocks writes/deletes under `userdata/`; toolkit
settings and caches remain writable. Coins use the game's units of ten.

地形不兼容或数量越界会显示错误。原生出怪需包含普通僵尸；场地与阵型操作仅适用于
普通战场。“冰车无痕”阻止新冰道，已有冰道正常消失。存档只读保护 `userdata/`
的写入与删除，不影响缓存和工具箱设置。金币以游戏原有的 10 枚单位保存。

## Files and fonts / 文件与字体

Choose uses native dialogs on Windows/macOS, document pickers on Android/iOS,
or Zenity when installed on Linux. Paths can also be entered manually.
Mobile imports are copied into app-owned `trainer/imports/`. Save-folder access
on mobile exports a ZIP of `userdata/`. Save pickers on
mobile return a new app-local staging path; after Apply, use **Export file**
to save/share outside the app. Directory exports are packaged as standard ZIPs.
For PAK operations, choose a source and an output; the text editor holds the two
paths on separate lines. Existing outputs are not overwritten.

选择按钮调用系统文件选择器；Linux 可选 Zenity，也可手动输入路径。移动端先导入到
应用目录；保存时先生成本地文件，再点“导出文件”调用系统导出。目录导出为 ZIP。
PAK 操作用两行分别表示源路径和目标路径，已有目标不会被覆盖。

The toolkit discovers installed Unicode fonts (CoreText fallback on Apple),
independently of English/Chinese game assets. A trusted `trainer-font.ttf`,
`.otf` or `.ttc` in the resource directory overrides font discovery. If no CJK
font exists, English remains available and the UI explains how to provide one.
No OS fonts or original game assets are shipped in source control.

工具箱使用系统 Unicode 字体，与游戏资源语言无关；可在资源目录提供受信任的
`trainer-font.ttf/.otf/.ttc`。缺少中文字库时保留英文并提示安装字体。

## Build and maintenance / 构建与维护

```sh
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DPVZ_TRAINER=ON
cmake --build build
```

`PVZ_DEBUG` is independent. Use `-DPVZ_TRAINER=OFF` to omit the module.
Most code lives in `src/Trainer/`; upstream-sensitive game-object access is in
TrainerService.cpp, rule hooks are calls to `Trainer::Enabled`, and desktop
event/presentation integration is limited to LawnApp, default/Input and
GLInterface. Keep all game mutations on the main thread and invalidate pending
commands at board replacement. API/profile formats must not be silently changed.

```sh
cmake -S . -B build -DPVZ_TRAINER_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
python3 tests/run-trainer-integration.py --resdir /path/to/GOTY
python3 tests/run-trainer-integration.py --resdir /path/to/GOTY --mobile
```

Integration tests use unique temporary saves under the build directory. Mobile
layout testing on desktop is not a substitute for Android/iOS device testing.
Implementation reference: https://github.com/lmintlcx/pvztoolkit (GPL-3.0).
The font rasterizer attribution is in `src/Trainer/third_party/README.md`.
