# NOTES — UI Tweaker (zhulis.ui-tweaker), the autonomous runs

# Run 2 (2026-10-13)

## Time

- Start: **2026-10-13 06:56:09 +02:00** (the first `date` of the run)
- Hard stop (11 h): **2026-10-13 17:56 +02:00**; stabilization from ~16:56
- You said at ~07:00 you'll be home 16:50–17:20 (Warsaw, +02:00): so everything wrapped up by **16:50**,
  stabilization from **~15:50**

## Summary (for when you're back)

- **Releases today**: v0.8.1, v0.9.0, v0.10.0, v0.11.0, v0.12.0, v0.13.0 (pre-releases, `.geode` from CI, each
  installed and checked both ways; Zhulis Mods Manager in the game fetched v0.13.0).
- **New for modders**: align/distribute (Alt+letters), `L` a row/column layout from a selection, paths to nodes
  without IDs (and IDs that aren't unique avoided), edits kept between runs and *Put edits back*, rotation and
  opacity, a *Grid* for round numbers in the code, `Ctrl+D` new nodes like existing ones (sprites, buttons, text
  and circle buttons, labels), `Alt+C` the way to a node, typed values for several nodes, the tree's search by
  frame or text, the panel kept off the taskbar, F8 in text fields, the cursor in levels; selecting several with
  `Ctrl+A` (siblings) or a right drag's box (outlined while drawn), text buttons made with their font and background.
- **Checked**: every feature in the self test (11 modes, both with your mods and in vanilla: the tour alone is 1200
  checks in vanilla), real mouse and keyboard (16 steps), a restart with an edit put back, the generated code
  compiled against the fake API and against the real Geode headers, and **end to end**: an edit's "Copy all" built
  into a mod and run in the game, the nodes exactly where the edit left them (`tools/e2e.sh`).
- **Reviews**: seven by separate agents, every finding fixed but one minor left as a note (three critical among
  them: a kept edit put on another layer's node; edits falling out after 200 steps; a mod's button background
  losing its folder).
- **Yours to decide**: the Geode Index (`reject-vibecoded`, Improved Logger not on the Index), see "Geode Index".

## What's done

| | What | Checked by |
| --- | --- | --- |
| own | v0.13.0: a right drag's box selects (outlined while drawn, Esc, Shift adds, no level objects), Ctrl+A siblings, Ctrl+D text buttons with their font/background (a mod's resource too), the sixth and seventh reviews fixed | self test both ways (editor: a box over 3000 objects, 2 ms), tour making every text button again, real input 19/19, codecheck, E2E PASS; the release's `.geode` both ways |
| 0 | v0.8.1 released: the self test fixed for vanilla (unlimited fps, the panel), `tools/vanilla.sh`; the CI `.geode` installed and checked both ways; the releases API shows it without a token | self test both ways, tour, real input 9/9 |
| own | Alt+C (the way to a node), Ctrl+D (sprites, buttons, text and circle buttons, labels), typed values for every selected node, the tree's search by frame or text, a popup's code pointed to its `init()`, nodes back where they were left out of "Copy all" | unit tests, compiled samples, self test both ways, real input 16/16 |
| own | The generated code compiled against the real Geode headers (`tools/codecheck.sh`), and the whole promise end to end (`tools/e2e.sh`: an edit, its "Copy all" built into a mod, the nodes where the edit left them) | codecheck against Geode 5.10.1; E2E PASS |
| 8 | Geode Index readiness: metadata checked against the guidelines, `offline` tag, `_spr` IDs on the overlay's parts; what's left for you below | `geode build`, the Index's tag list (API), self test both ways |
| 7 | The tour: 13 scenes (+ LevelBrowserLayer of your levels, EditLevelLayer, a quick popup, OptionsLayer, MoreOptionsLayer, ProfilePage) | tour PASS: 947 checks vanilla, 873 yours; no crash, every path leads to its node |
| 6 | *Grid* setting: the code's offsets in whole steps from the closest edge/center, the grid drawn while dragging (`core/Grid.cpp`) | grid unit tests, self test on the screen and in a turned scaled parent (code read back), screenshot mid-drag (`=grid`): `winSize.width / 2 - 20.f, winSize.height - 40.f` |
| 5 | Rotation (Ctrl+wheel, typed) and opacity (typed), in the code only when the edit mode changed them; no color (not layout; a button's press tints it) | unit test, a compiled sample, self test (an action-turned node not turned in the code, undo leaves the game's turn), real Ctrl+wheel, 13/13 both ways |
| 4 | Edits kept between runs (`core/Session.cpp`, `session.txt`), *Put edits back* (off by default), `Ctrl+Delete`, `inspect::resolve`, nodes without IDs told apart by their path in "Copy all" | session unit tests (every field round trip, odd input), `UI_TWEAKER_SELFTEST=session` both ways, `tools/session-check.sh`: a real drag, a restart, put back where it was, undone (6/6) |
| 2 | `L`: a row/column layout from the selection (`core/RowFit.cpp`, `codegen::layoutCode`) | unit tests, two compiled samples (both styles), the self test applying the code with Geode's real RowLayout/ColumnLayout: every node stays put, both ways |
| 3 | Nodes without IDs: a path from the closest parent with a findable ID or the layer, each step by ID, frame or type; IDs that aren't unique avoided; `inspect::followPath` | unit tests, three new compiled samples (nested ifs, frames, a twin ID), tour both ways: 74 paths in vanilla, 30 through parents without IDs, all lead to their node |
| 1 | Align/distribute (`Alt`+`A` `D` `H` `W` `S` `V`, `Alt`+`Shift`+`H` `V`), `core/Align.cpp` with tests; undo keeps a selection that covers the step | self test (turned/flipped/scaled parent, one node in its parent, code read back) both ways; real Alt+A, 10/10 |

## Decisions

- **Backlog 4, kept edits**: always kept in the save folder (it only changes what "Copy all" has; nothing is done
  to the game), put back only with the setting on and the edit mode on. Putting back is a step like any edit:
  recorded only when a value changes, so after the code is pasted the same values are a no-op; undone with Ctrl+Z.
  The self test writes `session-selftest.txt`, never the user's file; `tools/session-check.sh` puts the user's
  settings and session back after it.

- **Checks both ways**: with your mods as you left them (many, with your MiSide menu) and in a vanilla game
  (`tools/vanilla.sh`: every mod but `geode.loader`, `geode.node-ids` and UI Tweaker off for one check; Geode's
  `saved.json` is copied back byte for byte after it, even if the check fails — its md5 checked after each run).
  `tools/real-input.sh`'s points are the vanilla main menu's, so it runs in vanilla.
- You allowed installing any Geode mods for tests (07:10).
- A "don't turn this computer off" banner (you asked at 07:04): a click-through, focus-less, topmost window
  (PowerShell + WinForms, in the session's scratchpad, not the repository), nearly invisible (6 %) while the game
  runs, hidden for README media through its `mode.txt`.

## Log

- 06:56 start. Before it (06:49–06:55, while TASK.md was being written): the self test on main failed 1 of 118
  with your mods on (the MiSide menu is back): the screenshot's distance check pointed the mouse at a label that
  fills its button exactly, so there was nothing to measure. It now looks for a node the edit mode would hover
  there that has distances to the selected one. With your mods: menu/play/editor/tour/code PASS
  (118/129/129/434/118). `tools/vanilla.sh` written for checks in a vanilla game (you asked for both).
- 06:57 **vanilla found two of the self test's own faults**: without another mod capping the frame rate, 60 frames
  went by inside a 0.5 s `CCTransitionFade` (the creator layer "wasn't there"): steps now last their frames at
  60 fps at least and the steps after a scene change wait for the transition to end. And the distance check
  pointed the mouse at a sibling that the panel then moved over (nothing is hovered under the panel): it now
  points once the panel has moved. Both ways: menu/play/editor/tour/code/aspect PASS; real input 9/9 (vanilla).
- 07:10 a background agent researching known GD/Cocos/Geode bugs that could get in this mod's way (you asked).
- 07:17 v0.8.1 released (pre-release); its own `.geode` from CI installed: menu and tour PASS with your mods
  (129/445) and in vanilla (131/522); the anonymous releases API lists it with the `.geode`.
- 07:20 backlog 1, align/distribute. The first self test run showed undo selecting only the nodes that moved (the
  leftmost one, already in place, has no change in the step), so Alt+A, Ctrl+Z, Alt+W lined up two of three:
  undo now keeps a selection that has all of the step's nodes. Real `Alt`+`A` reaches the mod (Windows sends Alt
  combinations as system keys; Geode's event has them). Self test 144 (vanilla) / 142 (yours), real input 10/10.
- 07:30 the research came back (below, "Known GD/Cocos/Geode quirks"); its findings are now in the backlog.
- 07:34 quirk 7: F8 in a text field (real input 12/12, and checked to fail without the fix). 07:37 quirk 1, the
  game's reload: harmless, now a self test mode (`reload`). A panel cut at the bottom in the check's screenshots is
  the Windows taskbar over a game window that isn't in front (the check starts it in the background), not the mod.
- 07:48 quirk 8, the cursor in a level: shown and let go while editing, put back after. The first version of the
  check passed for nothing (GD locks only with its option on, off here): it now finds out whether the cursor
  locks at all first; and `showCursor(true)` unlocking too hid the relock. Menu/play/editor PASS both ways.
- 08:10 backlog 3, nodes without IDs. Before, a node whose parent had no ID got "no ID to find it by". Now a path
  down from the closest parent with an ID or the layer. The tour checks each with the same calls; its first run
  found the code for a LevelSelectLayer sprite leading nowhere: `getChildByIDRecursive("background")` found
  another `background` first. IDs are now checked for that (the node's own too). The fake scene for the compiled
  samples had two bugs of its own (a CCNode step counts every node; buttons as "others" before a button).
- 08:23 backlog 2, `L`. The self test does what the code says with Geode's real AxisLayout: it found the column
  off by half its size (a plain CCNode's anchor is its corner, so the code sets 0.5 now) and that Geode's column
  goes bottom to top (`setAxisReverse(true)` for nodes added top to bottom: without it, 215 points off).
- 08:23 the second review (separate agent, a15c6f0..HEAD): no critical; 4 important (describe searching the whole
  layer every frame, Alt swallowing every key, a null parent in align, the cursor hidden again on the end screen),
  5 minor. 08:35 all fixed: how a node is found is remembered for half a second per node (and found again for
  copied code): describing an editor node with 3000 objects takes 1.2 ms the first time, 0.001 ms a frame after;
  Alt passes every key but its six letters (Alt+arrows nudge again, checked through Geode's own key event); align
  skips nodes not in the scene or inside another selected one; the cursor is put back only in a level being played
  (not ended, not under a transition); a twin ID without a path says so in the code; followPath can't step past
  the nodes above; vanilla.sh stops if the game doesn't close, the copy is empty or python fails; the keybind's
  description says it works in text fields. Both ways: menu/play/editor/tour PASS, real input 12/12.
- 08:45 v0.9.0 released (pre-release).
- 08:57 backlog 4, kept edits. Found on the way: a self test step's frames are the wait after it, so the check
  came before the put back (0.3 s after a scene settles); your MiSide menu puts the Geode button back every frame,
  so its position is checked only where it stays. Both ways: menu/play/editor/tour/reload/session PASS (vanilla
  tour 622); the restart with real input 6/6.
- 09:17 backlog 5, rotation and opacity. Found on the way: the lists of nodes whose size or visibility the edit
  mode set were never pruned (a node freed later could share an address with a new one): pruned with the originals
  now; and whether a rotation or opacity is an edit comes from the steps not undone, not from a list.
- 09:26 backlog 6, the grid, as a grid for the code's numbers rather than for the screen's points: a modder wants
  `winSize.width / 2 - 20.f`, not a position that's a multiple of 10 on a 569-wide screen. Its lines are drawn per
  anchor (from the edges and the middle out to the quarter points), as the expressions are.
- 09:30 a third review started (separate agent: session, rotation/opacity, grid). It came back at 09:35 with one
  critical (a kept edit of one layer put on another layer's node with the same ID, and the real edit lost) and six
  important (an undone put back losing the kept edit; an unreadable file written over with nothing at exit; rotation
  and opacity counted from any step, a drag of a spinning node included; a skewed node's skew lost by Ctrl+wheel;
  put back by the raw position, not the code's expression from the edge; Ctrl+Delete one stray key from losing
  everything). 09:48 all fixed: only the layers in the scene are looked for and a description of another layer
  finds nothing; the kept description stays kept when it's put back; the file is written only after an edit this
  run, never if it couldn't be read, through a temporary file, the old one backed up (`.bak`; `.forgotten` before
  Ctrl+Delete, which now wants two presses); rotation and opacity come only from turns, typed values and put backs,
  and typing only x doesn't touch them; skewed nodes aren't turned; put back evaluates the code's expressions in
  the frame as it is now; and the minor ones (tracked sizes marked only after a put back, nodes with no way to find
  them told apart, numbers with a point whatever the locale, the grid off under a turned parent and per the guides'
  own lines, the dragged node described once a drag, the opacity field only for nodes with one). Both ways: every
  mode PASS (vanilla tour 951), restart 6/6, real input 13/13. Backlog 7: six more scenes in
  the tour, nothing from the servers (your levels, a local level's page, popups over the main menu): all right.
- 10:00–13:30 (in short; the details are in "What's done", the changelog and the commits): the fourth and fifth
  reviews fixed; v0.10.0, v0.11.0, v0.12.0 released and each checked both ways; Alt+C, Ctrl+D, typed values for
  several nodes, the tree's search by frame/text, the history limit, `tools/codecheck.sh` and its CI job,
  `tools/e2e.sh`, README's settings and GIFs, Ctrl+A.
- 13:45 A right drag draws a box and selects the nodes wholly inside it (`inspect::within`: not plain
  containers, a button's part as the button, nothing inside another chosen node; Shift adds; a right click still
  selects nothing). Self test (three sprites in a box, a box through a sprite leaves it out, Shift keeps the
  selection, a click) both ways; real input in vanilla: a real right drag around the three big buttons, 3 nodes.
  Sixth review (3db95b9..07464da): no critical; fixed: `tools/e2e.sh` could pass on an earlier run's export (now
  deleted first, and step 1 must say PASS), its cleanup waits for the game and warns if the test mod is left; a
  node made with Ctrl+D and hidden with V (an edit) was taken out by the next Ctrl+D or by the edit mode going off
  (now only nodes whose steps are all undone go). E2E PASS again, saved.json md5 unchanged.
- 14:05 Ctrl+D on a text button keeps its font and background: `ButtonSprite`'s numbers (width, min width, scale,
  absolute, height) and its 9-slice's texture file (the texture cache's key without folder and -uhd; not when a
  sprite frame shares the texture, a sheet). Unit tests, a compiled sample (fake API and real Geode headers), the
  self test (a "GJ_button_04"/bigFont button made as big), and the tour making again every text button of 13 real
  scenes: 20 of 21 with their look with your mods, 13 of 14 in vanilla, all as big as the game's.
- 14:25 seventh review (07464da..caafc5d): one critical, fixed: a text button whose background is a mod's
  resource ("geode.loader/white-square.png", `"bg.png"_spr`) lost its folder, so Ctrl+D asked for a file that
  isn't there. Now the name is the file with as many folders as it takes for `textureForKey` to give that very
  texture back, else the default look (self test: Geode's white square as a background). Also: the box's outlined
  nodes let go with the mode and a scene change, looked for again a few times a second, none for a click's few
  points or during a panel drag, no hover looked for meanwhile; the tour fails if no text button of a scene gets
  its look; the whole-screen box with a level's object put wholly in it; Shift adding a node; the mode off/on path
  for a made node hidden with V (kept, no code: made and hidden changes nothing, Ctrl+Z shows it); the fake
  ButtonSprite checks every number; `tools/e2e.sh` exits 1 on a failure. Not done: a ButtonSprite's own scale
  and colors (the code would need statements, not an expression). Both ways: 1, limit, tour, editor PASS; real
  input 19/19; codecheck.
- 14:36 v0.13.0 released (pre-release, the `.geode` CI built from 8c97f2d): installed, self test PASS both ways;
  Zhulis Mods Manager's cache has v0.13.0.

## Checks

| When | Mods | Tests | geode build | Self test | Screenshot |
| --- | --- | --- | --- | --- | --- |
| 14:35 (v0.13.0) | yours | 18/18 | ok | PASS 1 / play / editor / tour / reload / session / code / aspect / grid / ime / limit / export (the release's `.geode`: PASS) | |
| 14:35 (v0.13.0) | vanilla | 18/18 | ok | PASS every mode; real input 19/19; E2E PASS; codecheck against Geode 5.10.1 (the release's `.geode`: PASS) | box.png |
| 12:58 (v0.12.0) | yours | 18/18 | ok | PASS 202 / play 216 / editor 214 / tour 1100 / reload 212 / session 214 / code 202 / aspect 202 / grid 201 / ime 202 / limit 202 | |
| 12:58 (v0.12.0) | vanilla | 18/18 | ok | PASS 204 / 218 / 216 / tour 1200 / 216 / 216 / 204 / 204 / 203 / 204 / 204; real input 16/16; restart 6/6; codecheck against Geode 5.10.1 | |
| 11:41 (v0.11.0) | yours | 18/18 | ok | PASS 189 / play 203 / editor 201 / tour 899 / reload 199 / session 201 / code 189 / aspect 189 / grid 188 / ime 189 | |
| 11:41 (v0.11.0) | vanilla | 18/18 | ok | PASS 191 / 205 / 203 / tour 977 / 203 / 203 / 191 / 191 / 190 / 191; real input 15/15; restart 6/6 | |
| 11:30 soak | vanilla | | | `tools/soak.sh 150` (drags, wheel, undo, Ctrl+D and undo, tree and code open): 880 → 908 → 935 → 959 → 977 → 996 → 1011 → 1031 MB; idle 2 min with the edit mode on: +14 MB, off: +15 MB: the game's own rise, the same rate | |
| 10:10 (v0.10.0) | yours | 18/18 | ok | PASS 164 / play 178 / editor 176 / tour 874 (13 scenes) / reload 174 / session 176 / code 164 / aspect 164 | grid mid-drag, panel above the taskbar |
| 10:10 (v0.10.0) | vanilla | 18/18 | ok | PASS 166 / 180 / 178 / tour 948 / 178 / 178 / 166 / 166; real input 13/13; restart 6/6 | README screenshots made again |
| 07:13 | yours | 14/14 | ok | PASS 129 / play 140 / editor 140 / tour 445 / code 129 / aspect 129 | MiSide menu: label selected, distances |
| 07:13 | vanilla | 14/14 | ok | PASS 131 / 142 / 142 / tour 522 / 131 / 131; real input 9/9 | vanilla menu: Geode button, panel dodged |

## Geode Index: what's ready, what's yours to decide (backlog 8)

Checked against the Index guidelines (geode-sdk/docs `mods/guidelines.md`, read 09:50) and Geode CLI 3.7.3:

- Ready: a proper name, description, icon (logo.png 512x512) and tags (`developer`, `interface`, `offline`,
  `utility`, all on the Index's list, `offline` added: the mod uses no network); about.md, changelog.md; it hooks
  nothing and changes nothing while the edit mode is off (no `reject-tradmod` concern); its own nodes have `_spr`
  IDs (the overlay, and now its boxes, banner, tag, panel and tree too); Windows only (`gd.win`), which the Index
  allows.
- **Yours to decide: `reject-vibecoded`.** The guidelines reject a mod "whose codebase entirely or mostly consists
  of AI-generated code", and also when the developer doesn't seem to understand it. This mod was written by
  Claude in two autonomous runs. Before submitting, you'd need to own the code: read it, be able to explain and
  maintain it, and expect the reviewer to ask. docs/ARCHITECTURE.md is the map.
- `geode build` warns that `zhulis.improved-logger` (the optional dependency) isn't on the Index: publish Improved
  Logger first, or mark it external (`setup_geode_mod(... EXTERNALS zhulis.improved-logger:>=1.5.0)`), or drop
  the integration for the Index build.
- Not done (yours): the submission itself (`geode index` login, the release `.geode`), an index account.

## Known GD/Cocos/Geode quirks (research, 07:30)

From a separate agent's reading of the Geode SDK, the bindings, cocos2d-x 2.2 and issues (VERIFIED = seen in the
source or an issue). What's done about each is marked.

1. **Screen mode / resolution / texture quality reload the whole game** (`GameManager::switchScreenMode` →
   `reloadAll`; Geode sends `GameEvent(TexturesUnloaded)` and purges bitmap fonts). The overlay in OverlayManager
   survives with stale labels; edited nodes go. VERIFIED. **Checked in the game (07:37): harmless.**
   `UI_TWEAKER_SELFTEST=reload` calls `reloadAll` with the edit mode on and a selection: no crash, the overlay
   draws its TTF and bitmap labels (panel, tree, code) fine afterwards (labels keep their own textures), nothing of
   the old scene stays selected, a drag and its undo work in the new menu. GD reopens Video Options over it.
2. **A mouse release can be lost** (raw input queue cleared while GD isn't in front; only keys get fake key-ups):
   a drag stuck after Alt-Tab. VERIFIED. The edit mode already ends a press when the window isn't active.
3. **Transitions**: `getRunningScene()` is the `CCTransitionScene`; the old and new scenes are drawn by hand, not
   its children. VERIFIED; the self test waits for them to end now (06:57).
4. **`setZOrder` gives a node a new arrival order even with the same z**, and GD removed `m_uOrderOfArrival` (sorts
   by its own child index): `[` then `]` or an undo of a z-order moves a node to the end of its row among same-z
   siblings. VERIFIED. Known, left: restoring GD's child index isn't safe to do from outside.
5. **Index paths depend on state** (MenuLayer's IDs by `m_clickedGarage`, controllers; NodeIDs #119 #39 #166 #116
   #133); `getChildBySpriteFrameName` matches a sprite or a button's image by frame. **Done (08:10)**, backlog 3;
   and the tour found an ID that isn't unique (LevelSelectLayer's `background`s) leading a path astray: an ID is
   used only if `getChildByIDRecursive` from the layer finds that very node.
6. **AxisLayout defaults** (gap 5, autoScale resets scales and `m_baseScale`, ignores invisible children, positions
   every child, forces ignoreAnchor off; `respectAnchor` shifts children). → backlog 2: a new container, every option
   set explicitly.
7. **F8 doesn't fire while a text field has the keyboard** (`allow-in-text-inputs` defaults to false). VERIFIED.
   **Done (07:34)**: `allow-in-text-inputs` on the keybind; a real F8 in a focused TextInput turns the edit mode
   on and the field lets go (and without the flag the same F8 did nothing).
8. **Lock cursor freezes `getMousePos()`** in a level with GD's lock-cursor option. VERIFIED. **Done (07:48)**:
   the edit mode shows a hidden cursor and lets go of a locked one every frame (the level does it again when
   resumed), and puts them back when it's left while the level is played. Found on the way: `showCursor(true)`
   also unlocks, so both states are read before either is changed. Self test in a level, both ways.
9. CCMenu clicks the first item in array order: overlapped buttons are clicked differently than drawn. Known.
10. CCMenuItemToggler's two inner buttons: picked as the toggler (the outermost whole thing). Known.
11. TableView cells and the editor's button bar are rebuilt: edits there don't survive; their hooks belong elsewhere.
12. `limitLabelWidth` resets a label's scale on a new text; batched labels in levels.
13. Opacity/color: only CCMenu cascades by default; CCMotionStreak has no opacity. → backlog 5.
14. Re-applying edits on a layer's re-creation: the layer's ID set to its class name ("MenuLayer") by the ID
    provider is the "IDs are ready" marker. → backlog 4.

## Not done / known

- Color isn't edited: it isn't layout, and a button's press tints it anyway (backlog 5, a choice).
- `[` `]` and an undo of a z-order give a node a new arrival order (GD's child index): among same-z siblings in a
  layout it moves to the end of its row (quirk 4). Not fixable from outside without touching GD's sorting.
- Ctrl+D makes a text button with the original's font and background (its image's scale too, not its colors) only when the
  background is a file of its own, a mod's resource too (GD's are: 20 of 21 in the tour's scenes with your mods,
  13 of 14 in vanilla); else `ButtonSprite`'s default look; a button's image a plain sprite or Geode's circle
  button (its top sprite's own relative scale, if not 1, isn't kept).
- `tools/real-input.sh`'s points are the vanilla main menu's: with your MiSide menu it would click other things,
  so real input runs in vanilla; the self test runs both ways.
- The game's memory rises about 7 MB a minute in the vanilla game here, the same with the edit mode on, off, idle
  or used: not the mod's, noted in case it matters.

## Proposals

- Geode Index: decide on `reject-vibecoded` first (see "Geode Index"); publish Improved Logger or mark it external.
- Reorder inside an axis layout by dragging (Figma's auto layout): the code would be `insertBefore` a sibling.
  Needs the order restored on undo, which GD's child index makes delicate (quirk 4).
- A "freeze the level" option while editing (pause the scene's schedulers, not the overlay's): still left out, it
  changes the game's state and fights speedhack mods.

# Run 1 (2026-10-12)

## Time

- Start: **2026-10-12 15:35:17 +02:00** (the first `date` of the run)
- Hard stop (8 h): **2026-10-12 23:35 +02:00**; stabilization from ~22:35
- **End: 2026-10-12 ~20:40 +02:00** (you woke up at ~20:33: the last feature finished, checks, this report). 5 h 05 min

## Links

- Repository: https://github.com/ZhulinskiiDanil/ui-tweaker (public since 20:01)
- Releases (pre-releases, `.geode` attached by CI): https://github.com/ZhulinskiiDanil/ui-tweaker/releases
- Zhulis Mods Manager: listed since 20:03 (`mods.json` 0b75722 and the built-in list 76272e9 in
  https://github.com/ZhulinskiiDanil/zhulis-mods-manager), "Beta: ..." like Crash Doctor; its releases are
  pre-releases, offered with the manager's *Include pre-releases* setting, as the other betas' are

## Blocked — needs you

- Nothing now. (Until 20:01 the repository was private: `gh repo create --public` had been refused by Claude Code's
  auto-mode classifier. At 20:00 you asked to make it public if the manager can't show private repositories; it
  can't, see the log at 20:00, so it's public now and the mod is in Zhulis Mods Manager.)

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

- 18:56 own: `UI_TWEAKER_SELFTEST=tour`: seven scenes, whatever is under a 7x5 grid dragged, its code read back,
  undone; with Alt picks too: 119 real nodes, 480 checks, all right.

- 19:02 own: the code's layout comments wrapped at 100 columns; README "how it goes".
- 19:05 own: the panel says what a node shows (frame name, font and text): `play-button shows GJ_playBtn_001.png`.
- 19:09 soak (`tools/soak.sh 60`): 60 rounds of a real drag, the wheel and two Ctrl+Z with the tree and the code
  view open, in Russian (TTF labels re-rendered as values change): the game's private memory 3173.3 MB at round 20,
  3173.5 at 40, 3173.4 at 60. No leak.

- 19:13 own: the self test edits a button in nested menus, the outer turned by 30, flipped and scaled, the inner
  turned back: picked through them, moved as far on the screen, its code right, undone. 19:14 `V` hides/shows.
- 19:17 own: a code style setting (Allman 2 / K&R 4), the K&R hook compiled in the tests too.
- 19:20 own: edited nodes marked in the tree. One real input run missed Shift+Right (the key's SendInput didn't land
  that time); the rerun was 9/9.

- 19:24 README's GIF and screenshots made again (handles, code view, one-line banner); ARCHITECTURE updated.
- 19:33 fourth review (separate agent, v0.7.0..HEAD): 2 important, fixed — visibility changed by the game being
  written and put back (now only V's counts, like sizes), the panel's cached look surviving to a node at the same
  address; and the smaller ones (tree colors, Ctrl+V/Ctrl+R, K&R after a trailing comment, UTF-8-safe fit,
  every sample compiled in K&R, `/utf-8` for MSVC). A self test check that the game hiding a node isn't an edit.
- 19:44 v0.8.0's own `.geode` from CI (not the local build) installed and checked: tour PASS, 504 checks.
- 19:45 **you turned the other mods off for a clean, near-vanilla game** (19 on: node-ids, devtools,
  texture-loader and the like; MiSide menu and Improved Logger off). Everything again on it: 1/play/editor/tour/
  aspect PASS (131/142/142/504/131); `code` failed once: the snapping check dragged the play button to a point off
  the screen's middle, and in vanilla its menu (the play/icon/editor row) is centered 1.2 points from it, a closer
  guide, which the drag rightly snapped to. MiSide hid that button, so it never came up. The check now drops it on
  the side where the screen's middle is the closest guide (`editor::snappedCenter`, the drag's own guides).
- 19:55 README's GIF and both screenshots made again on the clean game. The vanilla title is a sprite (no handles),
  so `tools/gif.sh` takes the handle as optional and shows a drag that snaps to the screen's center with its guide;
  it waits for the GPU overlay's toast to go (`SETTLE`, and `SHOT_DELAY` in check.sh); the GIF's palette is made
  from frames across all of it, as the menu's background goes through every hue (one frame's palette banded).
- 19:57 own (seen on the Russian screenshot): the panel wrapped Russian text at half the width, counting bytes,
  not letters. `text::wrap` and `fit` count UTF-8 letters now, with tests.
- 20:00 you asked for Zhulis Mods Manager to show private repositories too, or else to make this one public. It
  can't for players: it reads GitHub's releases without a token, as it must, and GitHub answers 404 for a private
  repository to anyone without access. Showing one to everyone would need a token shipped in the mod, a key made
  public. A token setting would work only for you and your collaborators. So the repository is public (anonymous
  API: 200), and the mod is added to the manager like Crash Doctor: pushed, the registry on raw.githubusercontent
  has it, the releases API answers without a token with 8 releases and the `.geode`, the logo 200. The manager
  built (`geode build` put it in the game's mods folder; it stays turned off there, as you left it).
- 20:08 real input on the clean game: its points were the MiSide screen's (the Geode button at the bottom right,
  a title with handles). Now the vanilla main menu's: the Geode button (330.75, 45), the title (284.5, 270) for the
  drag, wheel, arrows, undo and typing, the play button's right handle (339.25, 170) for the resize. 9/9.
- 20:13 the manager checked in the game (you had turned it on again at 19:48): its "Zhulis Mods" list has UI
  Tweaker with its logo and description, installed v0.8.0; its releases cache has all 8 of them with the `.geode`,
  fetched without a token. The list says "No releases yet" while *Include pre-releases* is off (it is here), as
  for the other betas (Crash Doctor, Mod Profiler): their releases are pre-releases too.
- 20:15 own: the other screens' names were colored text alone, lost over the vanilla menu's RobTop logo (green
  and yellow on orange); now on dark plates. `aspect` PASS 131, looked at.
- 20:20 fifth review (separate agent, v0.8.0..HEAD: letters in wrap/fit, `snappedCenter`, the plates, the tools):
  no defects; it also ran wrap/fit on broken UTF-8 (stray and cut bytes, 4-byte letters): no loops, no reads past.
- 20:25 soak on the clean game (`tools/soak.sh`, the vanilla title now): 1472 → 1547 → 1566 → 1581 MB over 60
  rounds, then a longer run: 1542 at the start, 1560 at round 20, 1525 at round 120 — flat, caches warming at the
  start. Stopped at round ~130 when you woke up (it moves the real mouse).
- 20:33 you woke up: stopped. v0.8.1 (Russian wrapping, the plates, clean-game media and checks) is on main and
  checked, not released: v0.8.0 was 19:40, under the ~2 h between releases. To release:
  bump `mod.json`/`CMakeLists.txt` to 0.8.1, tag `v0.8.1`, publish a pre-release; CI attaches the `.geode`.

## Checks

| When | Tests | geode build | Self test | Screenshot |
| --- | --- | --- | --- | --- |
| 16:08 | 5/5 | ok | PASS, 39 checks | box + tag + panel at the Geode button |
| 16:32 (v0.2.0) | 9/9 | ok | PASS, 48 checks; real input 6/6 | tree search, other screens, box + tag |
| 17:07 (v0.3.0) | 11/11 | ok | PASS, 68 checks; real input 6/6 | Russian panel, distances, GIF frames |
| 17:33 (v0.4.0) | 12/12 | ok | PASS 91 (menu), 102 (+level), 102 (+editor); real input 7/7 | level, editor, typing |
| 18:09 (v0.5.0) | 13/13 | ok | PASS 104 / 115 / 115; real input 8/8 | several selected, code view |
| 18:35 (v0.6.0) | 14/14 | ok | PASS 113 / 124 / 124; real input 9/9 | handles, hooks |
| 18:58 (v0.7.0) | 14/14 | ok | PASS 116 / 127 / 127, tour 480 (119 nodes, 7 scenes); real input 9/9 | help wrapped |
| 19:36 (v0.8.0) | 14/14 | ok | PASS 129 / 140 / 140, tour 493; real input 9/9; soak flat | README media |
| 20:08 clean game | 14/14 | ok | PASS 131 / 142 / 142, tour 504, aspect 131, code 131; real input 9/9 | vanilla menu: box, tag, panel; GIF |
| 20:35 final (main) | 14/14 | ok | aspect PASS 131 (with the plates); review: no defects; soak flat | plates looked at |

## Not done / known

- Review items left as they are: right/middle/X button releases are always swallowed in the edit mode (the game only
  uses left touches; another mod that saw a press before the edit mode could miss its release). Edited nodes of
  the running scene stay retained while the edit mode is off (only other scenes' are let go when it's turned off).

- (fixed) "Copy all (1)" after the self test: MiSide's layout guard, see 16:24.

## Proposals

- Publish Improved Logger to the Geode Index first if UI Tweaker should go there (`geode build` warns about the
  soft dependency on a mod that isn't on the Index), as mod-profiler's NOTES say too.
- A "freeze the level while editing" option (pause the running scene's schedulers/actions but not the overlay's):
  not done, since it changes the game's state and other mods (speedhacks) set the time scale too.
- Persisting edits across scene reloads as a live preview (re-applying them by ID when a layer is made again while
  the edit mode is on) would save a rebuild more, but it changes nodes the user didn't touch this time; left out.

