# How UI Tweaker works

A map of the code and the reasons behind the parts that aren't obvious.

## Nothing while it's off

The mod has no hooks. While the edit mode is off the only thing of it in the game is Geode's own keybind for its key. Turning it on adds four listeners to Geode's input events (`MouseInputEvent`, `MouseMoveEvent`, `ScrollWheelEvent`, `KeyboardInputEvent`) at a priority ahead of everyone, and a node to Geode's `OverlayManager`, which is drawn over every scene. Turning it off destroys both. A node is changed only by an edit or an undo.

## The path of a click

1. **Editor.cpp**, `onMouse`: Geode sends the press before the game's touch dispatch. The edit mode answers `Stop`, so the game never gets the touch. A release whose press it didn't see (a button held from before) goes on to the game, so nothing stays pressed.
2. **Inspect.cpp**, `hitsAt`: the running scene in the order it's drawn (children by z, then by order of arrival, the negative ones before their parent), from the last drawn back. A hit is a visible, not fully transparent node whose content box, through its node-to-world transform, contains the point. Plain containers (layers, menus, batch nodes, bare `CCNode`s) count only if nothing else is there, or a full-screen menu would cover every button. A hit inside a button, a label or a 9-slice is that whole thing, unless `Alt` is held.
3. `pressAt` selects it and keeps its state, the press point and the parent's node-to-world transform. Each frame, `dragTo` turns the mouse's move on the screen into the parent's space (`geom::localDelta`: the inverse of the transform's linear part), so the node follows the mouse under a scaled, turned or flipped parent.
4. `release` records one step in the history: the node, its state before and after. The wheel and the arrows merge their steps on the same node within 0.6 s and 1 s.

## Undo

`core/History.hpp` keeps the steps. The nodes in it are held by `Retained`, a retain/release pair with the moves done right: `geode::Ref`'s move assignment doesn't release what it held, and `geode::WeakRef`'s assignment repoints the shared controller of the object it held. A removed node stays alive while a step needs it, but undo skips a node that isn't in the running scene anymore; a new scene forgets the old one's steps, so their nodes go.

## The code

`inspect::describe` turns a node into a plain `codegen::NodeDesc`: IDs, class, position, scale, anchor, z-order, the frame to measure in and the parent's layout. `core/Codegen.cpp` writes the C++.

- **The frame.** The screen if the parent's node-to-world transform is a translation only and the parent is screen-sized (or has no size, a bare container): then the node's place on the screen is the parent's origin plus the position, and each axis is written as the closest of 0, `winSize / 2` and `winSize`, plus an offset, minus where the parent's origin is (also as the closest of the three). A menu as made has its origin in the middle of the screen, so a node 25 above the bottom in it is `-winSize.height / 2 + 25.f`. Otherwise the parent's content box, with `parentSize`.
- **Layouts.** An `AnchorLayout` moves only children with `AnchorLayoutOptions`, to one of nine anchors on the parent's box plus an offset: the same 3x3 as the expressions in the parent's frame, so the code is that anchor and offset. An `AxisLayout` puts every child in line; what can be told to it per node is the gap before it and whether to scale it. A `SimpleAxisLayout` has no per-node gap: an `AxisGap` node is one.
- **Checking it.** `core/Expr.cpp` reads the generated expressions back (numbers, `winSize.width`, `+ - * /`, parentheses). The self test and `inspect::checkCode` evaluate the code's `setPosition` (or the anchor and offset through `AnchorLayout::getAnchoredPosition`) and compare with where the node is, to a hundredth.

## Files

| File | What it does |
| --- | --- |
| `main.cpp` | The keybind, the self test's start |
| `Editor.cpp` | The edit mode: listeners, selection, drag, wheel, arrows, undo, copying the code, the overlay and its panel |
| `Inspect.cpp` | Real nodes: class names, boxes on the screen, what's under a point, the frame and the layout, `NodeDesc`, checking code |
| `SelfTest.cpp` | `UI_TWEAKER_SELFTEST=1` |
| `Retained.hpp` | A strong reference that's safe in vectors and maps |
| `core/Geometry.cpp` | Points, rects, affine transforms, `nodeToParent` as Cocos builds it, quads, deltas between spaces, keeping a node in place when its anchor changes |
| `core/Expr.cpp` | The closest edge or center, formatting expressions and numbers, reading them back, the arguments of a call |
| `core/Codegen.cpp` | The C++ for one node or many, layout warnings, how the code finds the node |
| `core/History.hpp` | Undo and redo with merging and a limit |
| `core/Text.cpp` | Wrapping the panel's text |

Everything in `core/` is Geode-free and tested in `tests/`, on Linux with GCC and on Windows with MSVC in CI.
