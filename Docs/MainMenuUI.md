# Momentum Main Menu UI

## UI design

The current first-pass main menu is implemented by `UParkourMainMenuWidget`.
It is an in-game UMG widget built from C++ so it runs immediately without requiring manual Blueprint asset setup.

Visual direction:
- Dark grey/black background.
- Left-side title and menu stack.
- Right-side abstract greybox course preview.
- Cyan speed lines and subtle grid motion.
- Button hover scale, brighter fill, left accent bar, hover sound.
- Button press shrink, click sound.
- Settings currently displays `SETTINGS: COMING SOON`.

## Recommended Blueprint asset names

If you want an editable Blueprint version in the UE editor:
- Widget Blueprint: `WBP_MainMenu`
- Level: `MainMenu` or `MainMenuLevel`
- GameMode Blueprint: `BP_MainMenuGameMode`

For the current project, startup already points to:
- `EditorStartupMap=/Game/Maps/MainMenu`
- `GameDefaultMap=/Game/Maps/MainMenu`

## UMG hierarchy

Recommended equivalent Blueprint hierarchy:

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

## Layout and colors

Recommended values:
- Background: `#020304`
- Left panel: `#06080B` with alpha around `0.82`
- Idle button fill: `#101318` with alpha around `0.72`
- Hover button fill: `#1A2933` with alpha around `0.92`
- Accent cyan: `#2ED1FF`
- Primary text: `#E0F2FF`
- Secondary text: `#6B94AD`
- Title size: `86`
- Menu button label size: `23`
- Version size: `15`

## Button events

Test Level:
- `OnHovered`: play hover sound, scale to about `1.035`, brighten fill, show left accent bar.
- `OnUnhovered`: scale back to `1.0`, dim fill, hide accent bar.
- `OnPressed`: shrink briefly to about `0.985`.
- `OnClicked`: play click sound, set input to Game Only, hide cursor, `OpenLevel(TestLevel)`.

Settings:
- same hover and press behavior.
- `OnClicked`: play click sound and show `SETTINGS: COMING SOON`.

Quit:
- same hover and press behavior.
- `OnClicked`: play click sound and call `QuitGame`.

## Animation setup in Blueprint

Create these UMG animations if converting to Blueprint:
- `Button_Hover_In`: 0.10s, button render scale `1.0 -> 1.035`, fill alpha/brightness up, accent alpha `0 -> 1`.
- `Button_Hover_Out`: 0.10s, reverse hover values.
- `Button_Click`: 0.06s down to `0.985`, then 0.08s back to hover or idle scale.
- `Background_Loop`: move speed lines horizontally by 100-240 px over 6-10 seconds, looping.
- `ComingSoon_Show`: fade pill/text in, hold briefly, fade out.

## Level switching

Current C++ flow:
- `AParkourPlayerController::BeginPlay` checks current level name.
- On `MainMenu`, it calls `ShowMainMenu`.
- `ShowMainMenu` creates `UParkourMainMenuWidget`, shows cursor, and uses Game and UI input.
- `StartTestLevel` sets Game Only input and opens `TestLevel`.
- `QuitGame` exits the game.

If using Blueprint:
- In `BP_MainMenuGameMode` or Level Blueprint, create `WBP_MainMenu` on BeginPlay and add it to viewport.
- Set Player Controller `Show Mouse Cursor = true`.
- Set input mode to `UI Only` or `Game and UI`.
- On Test Level click, call `Open Level` with `TestLevel`.

## Acceptance checklist

- Opening game starts on the main menu map.
- `MOMENTUM` title is visible on the left side.
- Buttons exist: `TEST LEVEL`, `SETTINGS`, `QUIT`.
- Hovering a button scales and brightens it, shows a cyan accent bar, and plays hover audio.
- Clicking a button has a press animation and click audio.
- Test Level opens `TestLevel`.
- Settings displays `SETTINGS: COMING SOON`.
- Quit exits the game.
- The menu reads like a designed speed parkour prototype, not default UE buttons.
