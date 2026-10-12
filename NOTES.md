# NOTES — UI Tweaker (zhulis.ui-tweaker), the autonomous run

## Time

- Start: **2026-10-12 15:35:17 +02:00** (the first `date` of the run)
- Hard stop (8 h): **2026-10-12 23:35 +02:00**; stabilization from ~22:35

## Links

- Repository: https://github.com/ZhulinskiiDanil/ui-tweaker

## Decisions

- **No hooks at all.** Geode 5 has input events (`MouseInputEvent`, `MouseMoveEvent`, `ScrollWheelEvent`,
  `KeyboardInputEvent`) sent before the game's own dispatch; a listener answering `Stop` keeps the input from the
  game. The listeners exist only while the edit mode is on; the overlay is a child of `OverlayManager`, so it
  survives scene changes and never touches the scene. Off: only Geode's keybind for F8.
- **In the edit mode every key press is the edit mode's**, except the toggle key and modifier keys. Key releases
  and a mouse release whose press the edit mode didn't see go on to the game, so nothing stays held.
- **Picking**: draw order (z, then order of arrival), plain containers (layers, menus, batch nodes, bare CCNode)
  only if nothing else is under the mouse — otherwise a full-screen CCMenu would cover every button. A hit inside a
  button/label/9-slice is the whole of it; `Alt`+click goes inside, `Tab` cycles what's under the mouse.
- **Frames for the code**: the screen (`winSize`) when the parent is screen-sized (or sizeless) and its transform is a
  translation; else the parent's content box (`parentSize`). Each axis is the closest of start/center/end + offset.
  A centered CCMenu origin is subtracted as `winSize / 2` exactly.
- **Layouts**: AnchorLayout → `AnchorLayoutOptions` (anchor + offset, same 3x3); AxisLayout → `setPrevGap` (+
  `setAutoScale(false)` if scaled); SimpleAxisLayout → an `AxisGap` as a comment (it has no per-node gap). The plain
  position is always in the code, as a comment for axis layouts.
- **Retained<T> instead of geode::Ref/WeakRef** in the undo history: Ref's move assignment doesn't release, and
  WeakRef's assignment repoints the shared controller (the SDK itself has a TODO calling it flawed).
- **Generated code style**: 2-space Allman like the user's repos.
- **The self test adapts to other mods.** The user's own MiSide menu mod hides the vanilla play button and covers the
  main menu. The play button is then edited without the mouse (logged as hidden), the Geode button is picked by the
  mouse, the label is a reachable one (a label inside a button is picked with Alt+click, as a user would). Picking
  isn't faked: a visible node that the mouse can't reach is a failure.

## Log

- 15:35 start. Read TASK.md, mod-profiler's and crash-doctor's NOTES (how they ran the game, screenshots, the
  manager), the Geode SDK's input events, layouts, WeakRef.
- 15:50 core (Geode-free): geometry (Cocos' nodeToParent incl. GD's rotationX/Y, skew, ignoreAnchor), expressions
  with a reader, codegen, history, text wrapping. 5 test suites, all passing.
- 15:58 first `geode build`, first game run: picking hit a background sprite, the Geode button's ID is
  `geode.loader/geode-button`, the overlay's fill was white (CCDrawNode wants premultiplied colors), SimpleRowLayout
  got AxisLayoutOptions (wrong). All fixed.
- 16:07 **a real bug the self test found**: `CCLabelBMFont` is a `CCSpriteBatchNode`, so labels were treated as
  containers and couldn't be hovered; and their opacity wasn't checked (not a CCNodeRGBA). Fixed.
- 16:08 **stage 1 check passed**: `Self test PASS: 39 checks`, the screenshot shows the box and the tag in place.

## Checks

| When | Tests | geode build | Self test | Screenshot |
| --- | --- | --- | --- | --- |
| 16:08 | 5/5 | ok | PASS, 39 checks | box + tag + panel at the Geode button |

## Not done / known

- After the self test, "Copy all (1)": the MiSide menu's layout guard moves the bottom menu's buttons after the
  undo, so the Geode button differs from its first state. To look into.
