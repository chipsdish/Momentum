# Momentum 主菜单 UI

## UI 设计说明

当前第一版主菜单由 `UParkourMainMenuWidget` 实现。
它是一个 C++ 构建的游戏内 UMG Widget，因此不需要先手动创建 Widget Blueprint 资产，也能直接运行。

视觉方向：
- 深灰 / 黑色背景。
- 左侧放标题和菜单按钮。
- 右侧放抽象灰盒跑酷关卡预览。
- 青色速度线和轻微网格动态效果。
- 按钮 Hover 时轻微放大、背景变亮、左侧出现发光条，并播放 Hover 音效。
- 按钮点击时轻微按压缩放，并播放 Click 音效。
- Settings 第一版只显示 `SETTINGS: COMING SOON`。

## 推荐蓝图资产命名

如果之后想在 UE 编辑器里做可视化可编辑版本，建议创建：
- Widget Blueprint：`WBP_MainMenu`
- 关卡：`MainMenu` 或 `MainMenuLevel`
- GameMode Blueprint：`BP_MainMenuGameMode`

当前项目已经设置启动地图：
- `EditorStartupMap=/Game/Maps/MainMenu`
- `GameDefaultMap=/Game/Maps/MainMenu`

## UMG 层级结构

如果要在 Blueprint 中复刻，推荐层级：

```text
WBP_MainMenu
CanvasPanel Root
  Border Background
  Border / Image SpeedLine_00..13
  Border / Image GridLine_00..07
  Border LeftPanel
  Border PanelEdge
  VerticalBox MenuBox
    TextBlock Eyebrow
    TextBlock Title: MOMENTUM
    TextBlock Subtitle
    Button TestLevelButton
      Overlay
        Border TestLevelFill
        HorizontalBox
          Border TestLevelAccentBar
          TextBlock TestLevelLabel
          TextBlock Chevron
    Button SettingsButton
      same structure
    Button QuitButton
      same structure
  Border ComingSoonPill
    TextBlock ComingSoonText
  TextBlock VersionText: Prototype Build 0.1
  TextBlock PreviewLabel
  Border PreviewBlock_00..04
  Border PreviewGlow
```

## 布局与颜色参数

推荐值：
- 背景：`#020304`
- 左侧面板：`#06080B`，透明度约 `0.82`
- 按钮默认填充：`#101318`，透明度约 `0.72`
- 按钮 Hover 填充：`#1A2933`，透明度约 `0.92`
- 强调青色：`#2ED1FF`
- 主文字：`#E0F2FF`
- 次级文字：`#6B94AD`
- 标题字号：`86`
- 菜单按钮字号：`23`
- 版本号字号：`15`

## 按钮事件绑定

Test Level：
- `OnHovered`：播放 Hover 音效，按钮缩放到约 `1.035`，填充变亮，显示左侧青色发光条。
- `OnUnhovered`：缩放回 `1.0`，填充变暗，隐藏左侧发光条。
- `OnPressed`：短暂缩小到约 `0.985`。
- `OnClicked`：播放 Click 音效，切换输入模式到 Game Only，隐藏鼠标，执行 `OpenLevel(TestLevel)`。

Settings：
- 使用同样的 Hover 和 Press 行为。
- `OnClicked`：播放 Click 音效，并显示 `SETTINGS: COMING SOON`。

Quit：
- 使用同样的 Hover 和 Press 行为。
- `OnClicked`：播放 Click 音效，并调用 `QuitGame`。

## Blueprint 动画设置方法

如果后续转成 Blueprint Widget，建议创建这些 UMG 动画：
- `Button_Hover_In`：0.10 秒，按钮 Render Scale 从 `1.0 -> 1.035`，填充亮度/透明度上升，Accent Alpha 从 `0 -> 1`。
- `Button_Hover_Out`：0.10 秒，反向恢复 Hover 数值。
- `Button_Click`：0.06 秒缩到 `0.985`，再用 0.08 秒回到 Hover 或 Idle 缩放。
- `Background_Loop`：速度线水平移动 100-240 px，持续 6-10 秒并循环。
- `ComingSoon_Show`：提示条和文字淡入，短暂停留，再淡出。

## MainMenu 和 TestLevel 切换

当前 C++ 流程：
- `AParkourPlayerController::BeginPlay` 会检查当前关卡名。
- 如果在 `MainMenu`，调用 `ShowMainMenu`。
- `ShowMainMenu` 创建 `UParkourMainMenuWidget`，显示鼠标，并使用 Game and UI 输入模式。
- `StartTestLevel` 会切到 Game Only 输入模式，然后打开 `TestLevel`。
- `QuitGame` 会退出游戏。

如果用 Blueprint 实现：
- 在 `BP_MainMenuGameMode` 或 MainMenu 关卡蓝图的 BeginPlay 中创建 `WBP_MainMenu` 并 Add to Viewport。
- 设置 Player Controller 的 `Show Mouse Cursor = true`。
- 设置输入模式为 `UI Only` 或 `Game and UI`。
- Test Level 按钮点击时调用 `Open Level`，关卡名填 `TestLevel`。

## 最终验收标准

- 打开游戏后先进入主菜单地图。
- 左侧能看到 `MOMENTUM` 标题。
- 有三个按钮：`TEST LEVEL`、`SETTINGS`、`QUIT`。
- 鼠标悬停按钮时，按钮会放大、变亮、显示青色发光条，并播放 Hover 音效。
- 点击按钮时有按压动画，并播放 Click 音效。
- 点击 Test Level 能进入 `TestLevel`。
- 点击 Settings 能显示 `SETTINGS: COMING SOON`。
- 点击 Quit 能退出游戏。
- 主菜单整体看起来像有设计感的高速跑酷原型 UI，而不是默认 UE 按钮堆叠。
