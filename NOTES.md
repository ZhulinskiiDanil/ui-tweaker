# NOTES — UI Tweaker (zhulis.ui-tweaker), the autonomous run

## Time

- Start: **2026-10-12 15:35:17 +02:00** (the first `date` of the run)
- Hard stop (8 h): **2026-10-12 23:35 +02:00**; stabilization from ~22:35

## Links

- Repository: https://github.com/ZhulinskiiDanil/ui-tweaker — **private for now, see Blocked**
- Releases (pre-releases, `.geode` attached by CI): https://github.com/ZhulinskiiDanil/ui-tweaker/releases

## Blocked — needs you

- **The repository is private, not public.** `gh repo create --public` was refused by Claude Code's auto-mode
  classifier ("Create Public Surface"), so I didn't go around it: I created it private (CI and releases work the same).
  To make it public: `gh repo edit ZhulinskiiDanil/ui-tweaker --visibility public --accept-visibility-change-consequences`
- **Not in Zhulis Mods Manager yet.** The manager reads GitHub releases without a token, so a private repository's
  releases can't be installed from it. The entry is ready to add once the repo is public (same as Crash Doctor's:
  `mods.json` + the built-in list in `src/registry/index.cpp`, description "Beta: ..."). I haven't committed it to
  the manager so it doesn't list a mod nobody can download.

## What's done

| | What | Checked by |
| --- | --- | --- |
| Stage 1 | F8 edit mode, clicks and keys kept from the game; hover box + class/ID/parent; drag, wheel, arrows (Shift: 10), Ctrl+Z/Y; panel (position, scale, anchor, size, parent, z-order); Ctrl+C code relative to the closest edge/center of the screen or parent, by ID; layout warnings and code through layout options; all math in Geode-free `core/` with tests | self test (menu/level/editor), real input, unit tests, compiled-code tests, screenshots |
| Backlog 1 | Snapping + guides (screen, parent, siblings), Ctrl to skip | self test on/off |
| Backlog 2 | Other screens (16:10, 4:3, 21:9), ghosts of the selected node, red boxes for nodes that don't fit, panel warning | screenshot |
| Backlog 3 | Node tree with search by ID/class, click to select | real keys |
| Backlog 4 | Why a node doesn't move/show: layout, actions, hidden parent, opacity/scale 0, no size, ignored anchor, clipping, turned parent, off screen | self test (hidden play button) |
| Backlog 5 | Export of every edited node in one block, a comment per node; then per layer across scenes, as ready-made `$modify` hooks, also in `export.cpp` | self test, compiled hooks in the tests |
| Backlog 6 | Distances with Alt (Figma-like) | self test |
| Backlog 7 | Improved Logger (soft): selection and count pinned, marker on copy | builds/runs with it loaded |
| Backlog 8 | Russian/English (TTF in Russian), settings, about.md, README + README.ru with a GIF and screenshots made by the run | screenshots |
| Backlog 9 | Three code reviews by separate agents; removed nodes, scene changes, popups, nested menus, turned/flipped nodes in the self test and the tests | self test, tests |
| Own | Typed values with expressions (x, y, scale, width, height); anchor point (Ctrl+arrows) and z-order ([ ]); Shift+click multi-select with group undo; resize handles; R to reset; code view (C); panel dodging; remembered views; the generated code compiled and run in the tests | self test, real input, tests |

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

- 16:10 v0.1.0 released (pre-release, `.geode` from CI attached). CI green: tests on Linux and Windows, the build.
- 16:13 backlog 1, snapping and guides (Figma-like), checked in the self test both on and off.
- 16:17 backlog 4, why a node doesn't move/show (layout, actions, hidden parent, opacity/scale 0, no size, ignored
  anchor, clipping, turned parent, off screen), in the panel; the self test checks the hidden play button says so.
- 16:20 backlog 2, other screens (`A`): 16:10, 4:3, 21:9 frames, the selected node's ghost on each, red boxes for
  nodes with IDs that don't fit, a panel line naming the screens. Model: GD keeps 320 points of height and changes
  the width; a node keeps its distance to the closest of left/middle/right (what the generated code does).
- 16:24 **real input check** (`tools/real-input.sh`, SendInput): a drag, the wheel (up = bigger), Shift+Right
  (+10), 3x Ctrl+Z, a click in the edit mode that doesn't open the Geode button's mods list, F8. All work. Found:
  "Copy all (1)" came from MiSide's layout guard moving the bottom menu by half a point; "edited" now means "has
  steps not undone".
- 16:30 backlog 3, the node tree (`T`) with search (`/`, typed without IME, Enter selects the first match), checked
  with real keys. Geode's code for `/` is its own `KEY_Slash` (0x1007), not VK_OEM_2.

- 16:35 backlog 6, distances (Alt held), Figma-like; self test hovers a sibling of the selected node.
  v0.2.0 released (pre-release).
- 16:40 backlog 7, Improved Logger (soft dependency, header copied like mod-profiler): the selection and the
  edited count pinned, a marker on copy. Its console window wasn't open on this machine, so the pins weren't seen
  on screen; the calls are the same API mod-profiler and crash-doctor use.
- 16:47 backlog 8, Russian/English: all words in `core/Words` (tested for completeness and placeholder counts),
  TTF labels (Segoe UI) in Russian since GD's bitmap fonts have no Cyrillic, left-aligned; tree rows (IDs) stay
  bitmap. Windows here is ru-RU, so Auto = Russian. README with a GIF made by real input (`tools/gif.sh`) and
  screenshots in both languages (`UI_TWEAKER_LANG` forces the language for them).
- 16:54 own: `Ctrl+arrows` move the anchor point keeping the node in place, `[` `]` the z-order; found and fixed
  the code for nodes in axis layouts leaving out anchor/z-order.
- 16:55 backlog 9 edge cases in the self test: a node removed while selected/hovered/in the history/in the tree,
  and two scene changes (CCTransitionFade to CreatorLayer and back) with a selection. 68 checks pass.
- 17:00 backlog 9 review by a separate agent (read-only): 1 critical, 7 medium, 6 low. All fixed except as noted
  below; see the commit "Review fixes". The critical one: a tree row's node could be freed mid-frame and drawn.
  Also important: the tree held every node of the scene, so an old scene was destroyed a frame late, inside the
  overlay's update — with GD destructors that clear globals (PlayLayer's) that could break the next scene. The
  tree holds nothing now and checks a row's node against the live scene before using it.
- 17:07 real input 6/6 again. The real drag now targets the title: after the self test's scene changes, MiSide's
  layout guard puts the Geode button back every frame, so its drag left no step (which is what the panel warns).

- 17:13 v0.3.0 released (pre-release), after the stage 1 check on it.
- 17:14 own: the self test in a crowd (5000 more sprites: hover 2.4 ms, tree < 1 ms a frame) and in a level
  (`UI_TWEAKER_SELFTEST=play`: the percentage label, through another mod's node over it).
- 17:16 own: the banner was 3 lines over the top of the screen, where a level's percentage is: 1 line, F1 for all.
- 17:21 own: `Enter` types exact values (x, y, scale; expressions `w/2+40`, `h-25`, `pw`, `ph`), self test and real
  keys.
- 17:24 own: a Geode Popup in the self test, the generated AnchorLayoutOptions put on its button and the layout
  updated: it stays. That found the code switching the button's anchor (BottomRight to Center): a node's own anchor
  is kept now.
- 17:27 own: "Copy all" keeps the edits of the scenes left (by layer) and writes `export.cpp` to the save folder.
- 17:30 own: the level editor in the self test (`UI_TWEAKER_SELFTEST=editor`): build-button in a RowLayout edited,
  the hover 0.43 ms with 3000 objects since big batch nodes aren't looked into.

- 17:36 v0.4.0 released (pre-release) after the checks in the menu, a level and the editor, and real input 7/7.
- 17:38 README.ru.md; ARCHITECTURE as the code is now.
- 17:41 own: `C` shows the selected node's code in the panel (bitmap, wrapped with its indentation).
- 17:44 own: the panel moves to the other side when the selected node is under it (not if placed by hand, not
  for screen-sized nodes, which would make it jump every frame).
- 17:48 second review (separate agent, the changes since v0.3.0): no critical/high; 3 medium, 9 low, all fixed
  (Enter-Enter rounding, duplicate sections on coming back to a layer, removed nodes' code lost when the mode is
  turned off, scenes told apart by a tag instead of an address, typing/selection/drag interplay, value checks,
  anchor read from the options, the self test typing Geode's real key codes). Each with a check in the self test.
- 17:58 own: **the generated code compiled and run in the tests** (tests/compile): eleven kinds of nodes, every
  branch of the generator, compiled against a fake of Geode's API (names and signatures checked against the real
  headers) and run on fake nodes, positions/scales/anchors/z/layout options compared. Found a dangling reference
  in the test itself that only MSVC showed (CI is GCC + MSVC).

- 18:04 own: `Shift`+click selects several nodes (group steps in the history, one Ctrl+Z); real input 8/8.
- 18:09 v0.5.0 released (pre-release).
- 18:17 own: the content size in the typed values; `setContentSize` (+ `updateLayout()` for a node with a layout)
  in the code, compiled and run in the tests. One check run found the game frozen while Geode loaded the mods,
  before UI Tweaker (40 lines of log, no crash log); the next three starts were fine. Not this mod's (it hadn't
  loaded yet), noted in case it comes back.

- 18:23 own: what's open (tree, code view, other screens, help) and a panel placed by hand are kept, also across
  restarts (checked by closing the game normally, which is when Geode writes saved values).
- 18:28 own: resize handles as in Figma (`geom::resize` keeps the opposite side whatever the transform; tested
  with turned, flipped, ignoreAnchor nodes); real input drags one.
- 18:35 own: "Copy all" as ready-made `$modify` hooks for 17 layers mods hook (signatures from the bindings),
  grouped by the closest such parent; the tests compile three hooks against the fake API and call them.

- 18:40 v0.6.0 released (pre-release). 18:45 own: `R` resets nodes; F1's key list wrapped by items, counting
  characters (Cyrillic is 2 bytes).
- 18:58 third review (separate agent, v0.5.0..v0.6.0): no crash paths; 6 important, all fixed: input during a
  resize (and its own focus loss), handles taking clicks meant for the tree/Shift+click or covering small nodes,
  labels/sprites with handles, sizes a node changes by itself being written and put back, ProfilePage's hook
  (`loadPageFromUserInfo`), the hooks' preamble (Geode.hpp + namespace) and their priority after node-ids.

## Checks

| When | Tests | geode build | Self test | Screenshot |
| --- | --- | --- | --- | --- |
| 16:08 | 5/5 | ok | PASS, 39 checks | box + tag + panel at the Geode button |
| 16:32 (v0.2.0) | 9/9 | ok | PASS, 48 checks; real input 6/6 | tree search, other screens, box + tag |
| 17:07 (v0.3.0) | 11/11 | ok | PASS, 68 checks; real input 6/6 | Russian panel, distances, GIF frames |
| 17:33 (v0.4.0) | 12/12 | ok | PASS 91 (menu), 102 (+level), 102 (+editor); real input 7/7 | level, editor, typing |
| 18:09 (v0.5.0) | 13/13 | ok | PASS 104 / 115 / 115; real input 8/8 | several selected, code view |
| 18:35 (v0.6.0) | 14/14 | ok | PASS 113 / 124 / 124; real input 9/9 | handles, hooks |

## Not done / known

- Review items left as they are: right/middle/X button releases are always swallowed in the edit mode (the game only
  uses left touches; another mod that saw a press before the edit mode could miss its release). Edited nodes of
  the running scene stay retained while the edit mode is off (only other scenes' are let go when it's turned off).

- (fixed) "Copy all (1)" after the self test: MiSide's layout guard, see 16:24.

## Proposals

- Make the repository public and add the mod to Zhulis Mods Manager (see Blocked): everything else is ready.
- Publish Improved Logger to the Geode Index first if UI Tweaker should go there (`geode build` warns about the
  soft dependency on a mod that isn't on the Index), as mod-profiler's NOTES say too.
- A "freeze the level while editing" option (pause the running scene's schedulers/actions but not the overlay's):
  not done, since it changes the game's state and other mods (speedhacks) set the time scale too.
- Persisting edits across scene reloads as a live preview (re-applying them by ID when a layer is made again while
  the edit mode is on) would save a rebuild more, but it changes nodes the user didn't touch this time; left out.

