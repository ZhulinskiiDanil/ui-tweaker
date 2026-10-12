# UI Tweaker

Place the nodes of the game's interface with the mouse, then copy the C++ that puts them there: one rebuild instead of five.

Press <cy>F8</c> (a setting) in any scene for the edit mode. The game doesn't get the clicks while it's on:

- **Hover** a node: a box around it, its class, its ID and its parent
- **Drag** it to move it, the **wheel** scales it (<cy>Shift</c>: by a hundredth), the **arrows** move it by 1 (<cy>Shift</c>: 10)
- <cy>Ctrl+Z</c> / <cy>Ctrl+Y</c>: undo and redo
- <cy>Ctrl+C</c>: the C++ for the selected node, <cy>Ctrl+Shift+C</c>: for every node edited in the scene
- <cy>Tab</c>: the next node under the mouse, <cy>Alt</c>+click: a part of a button, <cy>PgUp</c>: the parent

The code finds the node by its ID and puts it relative to the closest edge or the center of the screen or of its parent (`winSize.width / 2 + 40.f`, `winSize.height - 25.f`), so it stays in place on other screens. If the parent has a layout that would move the node back, the panel says so, and the code uses the layout's options where there are any.

Nothing of it runs while the edit mode is off: no hooks.
