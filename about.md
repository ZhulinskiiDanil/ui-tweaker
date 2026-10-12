# UI Tweaker

Place the nodes of the game's interface with the mouse, then copy the C++ that puts them there: one rebuild instead of five.

Press <cy>F8</c> (a setting) in any scene for the edit mode. The game doesn't get the clicks while it's on:

- **Hover** a node: a box around it, its class, its ID and its parent
- **Drag** it to move it, snapping to the screen's, its parent's and its siblings' edges and centers (<cy>Ctrl</c>: not). The **wheel** scales it (<cy>Shift</c>: by a hundredth), the **arrows** move it by 1 (<cy>Shift</c>: 10)
- The **handles** on the selected node's box resize it
- <cy>Enter</c>: type exact values: x, y, the scale, the width and the height, numbers or expressions like <cy>w / 2 + 40</c>
- <cy>Ctrl</c>+arrows: the anchor point, the node staying in place; <cy>[</c> <cy>]</c>: the z-order
- <cy>Shift</c>+click: several nodes at once, <cy>V</c>: hide one, <cy>R</c>: back to how it was
- <cy>Ctrl+Z</c> / <cy>Ctrl+Y</c>: undo and redo
- <cy>Ctrl+C</c>: the C++ for the selected node, <cy>Ctrl+Shift+C</c>: for every node edited, in this scene and the ones left (also saved to export.cpp). <cy>C</c> shows the code in the panel as you drag
- <cy>Alt</c> held: the distances to the node under the mouse
- <cy>T</c>: the node tree with a search, <cy>A</c>: how it looks on 16:10, 4:3 and 21:9 screens
- <cy>Tab</c>: the next node under the mouse, <cy>Alt</c>+click: a part of a button, <cy>PgUp</c>: the parent, <cy>F1</c>: every key

The panel says why a node may not move or show: its parent's layout, an animation, a hidden parent, no size...

The code finds the node by its ID and puts it relative to the closest edge or the center of the screen or of its parent (`winSize.width / 2 + 40.f`, `winSize.height - 25.f`), so it stays in place on other screens. If the parent has a layout that would move the node back, the code uses the layout's options where there are any.

In English and Russian. Nothing of it runs while the edit mode is off: no hooks.
