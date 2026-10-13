# How UI Tweaker works

A map of the code and the reasons behind the parts that aren't obvious.

## Nothing while it's off

The mod has no hooks. While the edit mode is off the only thing of it in the game is Geode's own keybind for its key. Turning it on adds four listeners to Geode's input events (`MouseInputEvent`, `MouseMoveEvent`, `ScrollWheelEvent`, `KeyboardInputEvent`) at a priority ahead of everyone, and a node to Geode's `OverlayManager`, which is drawn over every scene. Turning it off destroys both and lets go of the nodes of other scenes. A node is changed only by an edit or an undo.

## The path of a click

1. **Editor.cpp**, `onMouse`: Geode sends the press before the game's touch dispatch. The edit mode answers `Stop`, so the game never gets the touch. A release whose press it didn't see (a button held from before) goes on to the game, so nothing stays pressed. A mouse button bound to the toggle goes on to Geode's keybind.
2. **Inspect.cpp**, `hitsAt`: the running scene in the order it's drawn (children by z, then by order of arrival, the negative ones before their parent), from the last drawn back. A hit is a visible, not fully transparent node whose content box, through its node-to-world transform, contains the point. Plain containers (layers, menus, batch nodes, bare `CCNode`s; a `CCLabelBMFont` is a batch node but draws) count only if nothing else is there, or a full-screen menu would cover every button. Batch nodes with over 100 children (a level's objects) aren't looked into. A hit inside a button, a label or a 9-slice is that whole thing, unless `Alt` is held. The hover is looked for when the mouse moves or `Tab` is pressed, otherwise a few times a second, and held for the frame.
3. `pressAt` selects it and keeps its state, the press point and the parent's node-to-world transform. Each frame, `dragTo` turns the mouse's move on the screen into the parent's space (`geom::localDelta`: the inverse of the transform's linear part), so the node follows the mouse under a scaled, turned or flipped parent. Then `core/Snap.cpp` finds the closest guide (the screen's, the parent's and the siblings' edges and centers, gathered when the drag starts) within 3 points on each axis, and the correction, made on the screen, goes through the same inverse.
4. With several nodes selected (`Shift`+click), the drag moves each by the same amount on the screen, through its own parent's transform; the node pressed is the one that snaps. Their steps share a group number in the history and are undone together.
   `Alt` with a letter (`core/Align.cpp`) lines them up the same way: each node's box on the screen, the move it needs to line up with the box of them all (or its parent's, for one node) or to have the same gaps, through its own parent's transform. A node that doesn't move has no change in the step, and undo keeps a selection that has all the step's nodes.
5. A resize handle pressed instead (`handleAt`, before any node) is `startResize`: the mouse's move goes into the node's own space through the inverse of its node-to-world transform, and `geom::resize` gives the new content size and the position that keeps the opposite side's point where it was in the parent.
6. `release` records one step in the history: the node, its state before and after. The wheel and the arrows merge their steps on the same node within 0.6 s and 1 s. Typed values (`core/Fields.cpp`, read by the same reader as the generated code with `w`, `h`, `pw`, `ph`) are one step.

## Keys

Every key press in the edit mode is its own, except the toggle and the modifiers; releases always go on to the game. A focused text field loses its focus when the edit mode starts: characters come through the IME, not key events, and would type into it. The tree's search and the typed values have the keys while they're open, translated by `core/Tree.cpp` and `core/Fields.cpp` (Geode has its own codes for some keys: `/` is `KEY_Slash`, 0x1007).

## Undo and the scenes

`core/History.hpp` keeps the steps. The nodes in it are held by `Retained`, a retain/release pair with the moves done right: `geode::Ref`'s move assignment doesn't release what it held, and `geode::WeakRef`'s assignment repoints the shared controller of the object it held. A removed node stays alive while a step needs it, but undo skips a node that isn't in the running scene anymore; a new scene forgets the old one's steps, so their nodes go.

A scene's nodes can't be described once it's gone (their parents are), so every edited node's `NodeDesc` is kept up to date while its scene runs. When the scene changes they move under the layer's name, and "Copy all" writes them with this scene's.

The node tree (`TreePanel.cpp`) holds no node: holding a scene's nodes would make an old scene die a frame late, inside the overlay's update, with whatever GD's destructors do to its globals. A row's node is looked for in the live scene before it's used.

## Kept between runs

Every edited node's `NodeDesc`, by layer, is written to `session.txt` in the save folder (`core/Session.cpp`: a line a field, strings escaped, unknown keys skipped) when it changes (at most every 2 s), when the edit mode is turned off and when the game exits, and read when the mod loads, into what's kept for "Copy all". A node is the same node (`session::sameNode`) by an ID the layer finds, else by its path, so an edit in a layer made again replaces the older one. With *Put edits back* on (off by default: the edit mode changes nothing by itself), once a scene has settled after a change, or when the edit mode is turned on, `putBack` finds each kept node in it as its code would (`inspect::resolve`, the class checked too) and applies its position (and the scale, anchor, z-order, size or visibility that were changed): one step to undo, recorded only if something changed, so a node whose code was pasted since is left alone. The self test keeps its session in a file of its own.

## The code

`inspect::describe` turns a node into a plain `codegen::NodeDesc`: IDs, class, position, scale, anchor, z-order, the frame to measure in and the parent's layout. `core/Codegen.cpp` writes the C++.

- **The frame.** The screen if the parent's node-to-world transform is a translation only and the parent is screen-sized (or has no size, a bare container): then the node's place on the screen is the parent's origin plus the position, and each axis is written as the closest of 0, `winSize / 2` and `winSize`, plus an offset, minus where the parent's origin is (also as the closest of the three). A menu as made has its origin in the middle of the screen, so a node 25 above the bottom in it is `-winSize.height / 2 + 25.f`. Otherwise the parent's content box, with `parentSize`.
- **Layouts.** An `AnchorLayout` (and a `CopySizeLayout`, which is one) moves only children with `AnchorLayoutOptions`, to one of nine anchors on the parent's box plus an offset: the same 3x3 as the expressions in the parent's frame, so the code is that anchor and offset; a node that has options keeps its anchor. An `AxisLayout` puts every child in line; what can be told to it per node is the gap before it and whether to scale it, set on the node's own options after its scale. A `SimpleAxisLayout` has no per-node gap: an `AxisGap` node is one.
- **Finding the node.** By its ID, if `getChildByIDRecursive` from the layer finds this node with it and not another one first (IDs aren't always unique: LevelSelectLayer has several `background`s). Otherwise a path (`NodeDesc::path`): from the closest parent whose ID is findable that way, or from the layer itself (`this` in its hook; node-ids gives a layer its class name as an ID, which isn't among its children), one nested `if` a level. Each step is a child by its ID (`getChildByID`) if it's the first sibling with it, else by its frame (`geode::cocos::getChildBySpriteFrameName`) if no sibling before it shows the same one, else by `getChildByType`, whose index counts what `typeinfo_cast` would (the MSVC RTTI of the node's class compared with each sibling's bases); a class the code can't name (another mod's, in a namespace) by `getChildByIndex`. The code says when a path goes through parents without IDs, as anything added before a node can change it. The tour follows every such path with the same calls (`inspect::followPath`) and checks it gets to the node.
- **Buttons.** `CCMenuItemSpriteExtra`'s press animation scales it back to `m_baseScale`, so the edit and the code set that too.
- **Rotation and opacity.** `Ctrl`+wheel turns, typed values set both. They're written (`setRotation`, `setOpacity` through `CCRGBAProtocol`, which labels have without being `CCNodeRGBA`) only if a step not undone changed them, and undo puts them back only if its step changed them: a node the game spins or fades with an action isn't edited by it. `setRotation` is called only when the rotation changes, since GD's skew is a rotation on each axis. No color: it isn't layout, and a button's press tints it anyway.
- **Sizes and visibility.** A content size is written (with `updateLayout()` for a node with a layout of its own) only if a resize handle or typed values set it: a label's text or a growing layout changing it isn't an edit, and undo doesn't put such a size back. `V` writes `setVisible`.
- **Hooks.** "Copy all" groups the edits by the layer whose hook they go in: the closest parent that's one of the layers in `core/Hooks.cpp` (17, with the function their interface is made in, from the bindings), else the top one under the scene. A known layer's section is a whole `$modify`, the code after the original, its priority after node-ids' (whose IDs it looks for), under Geode's header and namespace.
- **Layouts from a selection.** `L` (`codegen::layoutCode`): the selected nodes' bounding boxes in their parent's space go through `core/RowFit.cpp` (the axis they're spread along, their order, the mean gap, the closest of start/center/end across), and the code makes a menu (a plain node if none of them is a button) as big as the box of them all, by its middle (`ignoreAnchorPointForPosition(false)`, anchor 0.5: a plain node's is its corner), with the layout's gap and cross line alignment set, `setAutoScale(false)` and, for a column, `setAxisReverse(true)` (Geode's column goes bottom to top). Every node is found before any is moved, since a node found by its index would be another once a sibling goes. The self test does what the code says with Geode's own `RowLayout`/`ColumnLayout` and checks every node stays where it was.
- **Style.** The code is made Allman with 2 spaces; `codegen::toKAndR` turns it into K&R with 4 for the setting: a lone opening brace goes up to the line before unless that's a comment.
- **Checking it.** `core/Expr.cpp` reads the generated expressions back (numbers, `winSize.width`, `+ - * /`, parentheses). The self test and `inspect::checkCode` evaluate the code's `setPosition` (or the anchor and offset through `AnchorLayout::getAnchoredPosition`) and compare with where the node is, to a hundredth; for a popup's button the self test also puts the generated options on it and updates the layout.

## The overlay

`Editor.cpp`'s `Overlay` draws the boxes, guides, distances and other screens' shapes with one `CCDrawNode` (premultiplied colors: it blends with `GL_ONE, GL_ONE_MINUS_SRC_ALPHA`), the tag by the mouse, the banner and the panel. Labels are `UiLabel` (`Ui.cpp`): bitmap `chatFont` in English, TTF (Segoe UI) in Russian since GD's bitmap fonts have no Cyrillic, set again only when their text changes. Every word is in `core/Words.cpp`.

## Files

| File | What it does |
| --- | --- |
| `main.cpp` | The keybind, the self test's start |
| `Editor.cpp` | The edit mode: listeners, selection, drag, snapping, wheel, arrows, typed values, undo, the scenes' code, copying, the overlay and its panel |
| `Inspect.cpp` | Real nodes: class names, boxes on the screen, what's under a point, the frame and the layout, why a node acts oddly, `NodeDesc`, checking code |
| `TreePanel.cpp` | The node tree with its search |
| `Ui.cpp` | The language and the labels |
| `LoggerLink.cpp`, `ImprovedLogger.hpp` | Improved Logger, if it's there: pinned values, markers. The header is a copy of its API |
| `SelfTest.cpp` | `UI_TWEAKER_SELFTEST` |
| `Retained.hpp` | A strong reference that's safe in vectors and maps |
| `core/Geometry.cpp` | Points, rects, affine transforms, `nodeToParent` as Cocos builds it, quads, deltas between spaces, keeping a node in place when its anchor changes |
| `core/Expr.cpp` | The closest edge or center, formatting expressions and numbers, reading them back, the arguments of a call |
| `core/Codegen.cpp` | The C++ for one node or many, layout warnings, how the code finds the node |
| `core/Snap.cpp` | Guides and snapping |
| `core/Measure.cpp` | Distances between two boxes |
| `core/Align.cpp` | Lining boxes up and distributing them |
| `core/Session.cpp` | The kept edits as text and back; whether two descriptions are of the same node |
| `core/RowFit.cpp` | Which axis, order, gap and cross alignment a set of boxes has, for a layout |
| `core/Aspect.cpp` | Other screens' shapes and what doesn't fit on them |
| `core/Explain.cpp` | Why a node doesn't move or show, from facts about it |
| `core/Tree.cpp` | The tree's search |
| `core/Fields.cpp` | Typed values |
| `core/History.hpp` | Undo and redo with merging and a limit |
| `core/Hooks.cpp` | The layers' hooks and wrapping code in them |
| `core/Words.cpp`, `core/Text.cpp` | Every word in English and Russian; wrapping and filling in the panel's text |

Everything in `core/` is Geode-free and tested in `tests/`, on Linux with GCC and on Windows with MSVC in CI. `tests/compile` goes further: a generator writes the code for a node of every kind (and hooks, in both styles), which is compiled against `FakeGeode.hpp` (Geode's names and signatures) and run on fake nodes, their positions, scales, anchors, z-orders, sizes, visibility and layout options compared.

## Checking it in the game

`UI_TWEAKER_SELFTEST` (see the README) runs the same calls the mouse and the keys make, in the main menu and, by its value, in a level, the level editor or seven scenes in turn; `tools/check.sh` runs it and takes a screenshot, `tools/real-input.sh` uses real input through SendInput, `tools/soak.sh` watches the memory over a while, `tools/gif.sh` made the README's GIF.
