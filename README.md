# UI Tweaker

[Русский](README.ru.md)

A [Geode](https://geode-sdk.org) mod for Geometry Dash, for mod developers: move and scale any node of the game's interface with the mouse, right in the game, then copy the C++ that puts it there. Your buttons and popups in their places after one rebuild instead of five. Windows.

<img src="logo.png" width="128" alt="logo">

![The main menu's title selected with real mouse and keys, dragged until it snaps to the screen's center with a guide, scaled with the wheel, undone; the code view shows its code as it changes](docs/demo.gif)

![The edit mode in the main menu: the Geode button selected, its panel, the layout's warning](docs/edit-mode.png)

## Install

Download `zhulis.ui-tweaker.geode` from the newest of the [releases](https://github.com/ZhulinskiiDanil/ui-tweaker/releases) and put it into Geode's mods folder (or **Install from file** on Geode's mods page). Or, with [Zhulis Mods Manager](https://github.com/ZhulinskiiDanil/zhulis-mods-manager), turn on its **Include pre-releases** setting: the mod is a beta, its releases are pre-releases.

## How it goes

1. In your mod, add your button or popup anywhere, with an ID (`setID("my-button"_spr)`), and build once
2. In the game, `F8`, drag it where it belongs, scale it with the wheel, type exact values with `Enter` if you like; `A` shows whether it still fits on a 4:3 or a 21:9 screen
3. `Ctrl+Shift+C`: the code for everything you moved, as `$modify` hooks of the layers it's in, also in `export.cpp`. Paste the lines into your hook (or the whole hook), build: it's where you put it, on every screen

## Use

`F8` (a setting) turns the edit mode on and off, in any scene. While it's on, the game gets no clicks and no keys but its own toggle:

| | |
| --- | --- |
| Hover | A box around the topmost node under the mouse, its class (from RTTI), its ID and its parent |
| Click | Selects it: an orange box, its anchor point, and the panel with its position, scale, anchor point, size, parent and z-order, what it shows (a sprite's frame name, a label's font and text), and why it may not move or show: its parent's layout, running actions, an invisible parent, opacity or scale 0, no content size, an ignored anchor point, a clipping parent, being off the screen |
| Drag | Moves it. A turned, scaled or flipped parent is taken into account: the node follows the mouse. Its edges and center snap to the screen's, the parent's and the siblings' within 3 points, with the guides drawn (`Ctrl`: no snapping; a setting). With the *Grid* setting (in points), the code's numbers go in steps of it from the closest edge or center (`winSize.width / 2 - 20.f`), the grid drawn while dragging |
| Handles | The eight small squares on the selected node's box resize it (its content size), the opposite side staying where it is, whatever its anchor point, scale or rotation. A node with a layout of its own lays its children out as it goes |
| Wheel | Scales it by 10% a notch, `Shift`: by 0.01 |
| `Ctrl`+wheel | Turns it by 15 degrees a notch, `Shift`: by 1 |
| Arrows | Moves it by 1 in its parent, `Shift`: by 10 |
| `Enter` | Types exact values: x, y, the scale, the width and the height (the content size; a node with a layout of its own lays its children out in it), the rotation and the opacity (0 to 255), `Tab` to the next, `Enter` to apply (one step to undo), `Esc` to cancel. With several selected, the fields you typed into go to each of them. Numbers or expressions: `w / 2 + 40`, `h - 25` (`w`, `h`: the screen; `pw`, `ph`: the parent) |
| `Ctrl`+arrows | Moves its anchor point to the next of 0, 0.5 and 1 that way, the node staying where it is on the screen (the position changes with it) |
| `[`, `]` | The z-order down, up (in an axis layout that's the order) |
| `R` | Back to how it was before its first edit: one step to undo |
| `V` | Hides it (a vanilla button you don't want), or shows it again: the code has `setVisible(false)`. A hidden node is still in the tree (`T`) |
| `Ctrl+Z`, `Ctrl+Y` | Undo, redo. A turn of the wheel or a run of arrow presses is one step |
| `Ctrl+C` | Copies the C++ for the selected node |
| `C` | Shows that code in the panel, as it is now: it changes as you drag |
| `Ctrl+Shift+C` | Copies the C++ for every node edited, a comment for each: this scene's and the scenes' left before, by the layer whose hook it goes in (the closest parent that's a layer mods hook: `MenuLayer`, `PlayLayer`, `EditorUI`, `PauseLayer`... or the top one, a popup). For those layers it's the whole `$modify` with the right `init` (or `customSetup`), ready to paste. Also written to `export.cpp` in the mod's save folder. The edits are kept between runs of the game (`session.txt` in the save folder), so a rebuild and a restart don't lose what isn't pasted yet |
| `Ctrl+Delete` | Forgets the edits kept from other scenes and earlier runs |
| `Alt` held | The distances from the selected node to the one under the mouse, as in Figma: the gaps between them, or from the inner one's edges to the outer one's |
| `Tab` | The next node under the mouse, deeper each time |
| `Alt`+click | A part of a button or a label, not the whole of it |
| `Shift`+click | Adds a node to the selection or takes it out. Drags, arrows and the wheel move all of them, one step to undo; `Ctrl+C` copies the code for all of them |
| `L` | Copies the selected nodes of one parent as a row or a column: a new menu (or node) with a `RowLayout`/`ColumnLayout` whose gap, size, position and alignment put them where they are now, and the code that moves them into it. Line them up with `Alt` first; the code says if their gaps differ |
| `Alt`+`A` `D` `H` / `W` `S` `V` | Aligns the selected nodes' left edges, right edges, centers / tops, bottoms, middles on the screen, as in Figma: several to the box of them all, one to its parent's box (or the screen's). `Alt`+`Shift`+`H` / `V` puts the same gap between three or more |
| `PgUp`, `PgDn` | The parent, the first child |
| `A` | Other screens: the 16:10, 4:3 and 21:9 screens drawn over this one, the selected node where it would be on each, a red box around every node with an ID that wouldn't fit on one of them. The panel says it too |
| `T` | The node tree of the scene at the right: every node with its ID and class, the hidden ones grey. A click on a row selects its node, the wheel scrolls it. `/` searches by ID and class (words in any order), `Enter` selects the first match |
| `H` | Hides the panel. Its title bar drags it |
| `F1` | Every key in the banner at the top (one line otherwise, out of the way of what's up there) |
| `Esc` | Unselects, then leaves the edit mode |

A button, a label or a 9-slice is picked as a whole: a click on the play button's sprite selects the button.

![The same in Russian](docs/edit-mode-ru.png)

![The main menu's three big buttons selected with a click and Shift+clicks, lined up by their tops with Alt+W, undone, by their bottoms with Alt+S, undone, copied as a layout with L](docs/align.gif)

![The title dragged on a grid of 10: the grid drawn, the code reads winSize.width / 2 - 20.f, winSize.height - 40.f](docs/grid.png)

The edit mode speaks English and Russian (the *Language* setting; Auto follows Windows). GD's bitmap fonts have no Cyrillic, so in Russian its text is drawn with TTF labels, rendered again only when they change. The generated code is English either way.

## The code

```cpp
// geode.loader/geode-button (CCMenuItemSpriteExtra) in bottom-menu (CCMenu)
if (auto node = this->getChildByIDRecursive("geode.loader/geode-button"))
{
  auto winSize = CCDirector::get()->getWinSize();
  node->setPosition({winSize.width / 2 + 40.f, winSize.height - 25.f});
  node->setScale(1.2f);
}
```

- **Found by its ID** (Geode node IDs), if no other node in the layer has it first. Without one, down from the closest parent with one (or from the layer), each step by a child's ID, by the frame a sprite or button shows, or by `getChildByType`; the code says when that's fragile
- **Relative to the closest edge or the center**, on each axis: of the screen when the parent is screen-sized and not scaled or turned (a layer, a menu as made), of the parent's content box otherwise (`parentSize.width - 10.f`). On a wider or narrower screen the node stays by the same edge
- **Layouts**: if the parent has one, the panel says what `updateLayout()` will do to the node. For an `AnchorLayout` the code is the anchor and the offset (`AnchorLayoutOptions`; a node that has them keeps its anchor, only the offset changes), for an `AxisLayout` (`RowLayout`, `ColumnLayout`) the gap before the node and `setAutoScale(false)` if it was scaled, for a `SimpleAxisLayout` an `AxisGap`; the position itself is then a comment
- **Laid out** with braces on their own lines and 2 spaces, or K&R with 4 spaces (the *Code style* setting)
- Only what changed: the scale, the anchor point, the z-order and the content size (with `updateLayout()` for a node with a layout) are written if they differ from before the edits (the scale also when it isn't 1). The z-order is written for nodes in a layout too: there it's their order

## Self test

`UI_TWEAKER_SELFTEST=1` at launch (`play` to go on into the first level afterwards and edit its percentage label there, `editor` to go on into the level editor with 3000 objects and edit its build button, `tour` to go through thirteen scenes of the game (the main menu, the creator menu, the icon kit, the main levels, the search, your levels, a level's page, a quick popup, the settings, more settings, your profile, a level and its pause menu) dragging whatever is under a grid of points in each, reading its code back and undoing it, `aspect` to leave the other screens' shapes on): in the main menu the mod turns the edit mode on and, with the same calls the mouse and the keys make, picks the play button, the Geode button and a label, drags each by (30, -20), turns the wheel, presses the arrows, generates the code and reads it back (it has to give the node's position exactly), then undoes everything and checks that every value is back, redoes and undoes again. Then snapping, the anchor point and z-order, the distances; 5000 more sprites in the scene for what the hover and the tree cost (about 2.4 ms for the hover, under 1 ms a frame for the tree here); a node removed while selected, hovered and in the tree; two scene changes with a node selected; a Geode Popup whose button is dragged and given the generated AnchorLayoutOptions, which must keep it there after `updateLayout()`; values typed with the keys. The log ends with `Self test PASS: N checks` or the failures. `tools/check.sh` runs it, takes a screenshot of the game and prints the log's lines. `tools/real-input.sh` drives the edit mode with real mouse and keys (SendInput) and checks each step in the log; `tools/gif.sh` made the GIF above. `UI_TWEAKER_LANG=en` or `ru` overrides the language for screenshots.

## Build

```
geode build --ninja
```

The math has no Geode in it and is tested on its own:

```
cmake -S tests -B build-tests
cmake --build build-tests
ctest --test-dir build-tests
```

How it works: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).
