# v0.8.0

- The *Code style* setting: braces on their own lines with 2 spaces, or K&R with 4 spaces as most Geode mods are written; both compiled in the tests
- `V` hides the selected nodes, or shows them again, as a step to undo: the code has `setVisible(false)`
- The self test also edits a button in nested menus, turned, flipped and scaled
- The panel says what the node shows: a sprite's or a button's frame name, a label's font and text, to make one like it
- The layout warnings in the code are wrapped at a hundred columns, as code is
- README: how it goes, in three steps

# v0.7.0

- `UI_TWEAKER_SELFTEST=tour`: seven scenes of the game, whatever is under a grid of points in each dragged, its code read back and undone (119 nodes with the parts of buttons too, all right)
- `R` resets the selected nodes to how they were before their first edit
- Fixes from a third review: nothing else edits a node while it's being resized (the wheel and the arrows waited for nothing and were undone on screen); a resize ends if the window loses focus; handles don't take clicks meant for the tree or Shift+click, and small nodes have no side handles over their middle; labels and sprites have no handles (their size is their text's or texture's); a size is written and put back only when it was set here, not when a label's text or a layout changed it; ProfilePage's hook is `loadPageFromUserInfo`, where its interface is made; "Copy all" starts with Geode's header and namespace, and each hook runs after node-ids' (`setHookPriorityAfter`); right click on the panel's title lets it move out of the way again
- `F1`'s list of keys is wrapped by whole "key: what" items, inside the screen in both languages

# v0.6.0

- "Copy all" is ready-made hooks: grouped by the layer whose hook the code goes in (`MenuLayer`, `PlayLayer`, `EditorUI`, `PauseLayer` and 13 more), each a `$modify` with the right `init` or `customSetup`; compiled in the tests
- A scale an axis layout gave a node isn't written unless it was changed
- Resize handles on the selected node's box, as in Figma: its content size, the opposite side staying in place
- The tree, the code view, the other screens, the help and a panel put somewhere by hand are as they were left, also after a restart
- The content size can be typed too (`Enter`, the fourth and fifth fields): a menu with a layout lays its children out in it; the code has `setContentSize` and `updateLayout()`

# v0.5.0

- `Shift`+click selects several nodes: dragged, nudged and scaled together, one step to undo, their code copied together
- `C` shows the selected node's code in the panel, as it changes
- The panel moves out of the way of the selected node
- The tests compile and run the generated code against a fake of Geode's API (GCC and MSVC)
- Fixes from a second review: exact values kept when Enter is pressed twice, one section per layer in "Copy all", removed nodes' code kept, scenes told apart reliably
- The README in Russian, the architecture as the code is now

# v0.4.0

- `Enter` types exact values for the selected node: x, y and the scale, numbers or expressions like `w / 2 + 40` (`w`, `h`: the screen; `pw`, `ph`: the parent)
- "Copy all" keeps the edits of the scenes left, each under its layer's name, and also writes them to `export.cpp` in the mod's save folder
- A node with AnchorLayoutOptions keeps its anchor in the code, only the offset changes (it was the closest anchor, which could be another one than the mod developer chose)
- The banner is one line at the top, `F1` shows every key
- A level's objects (big batch nodes) aren't looked through for the hover: 0.4 ms in the editor with 3000 objects
- The commented position for a node in an axis layout says where parentSize comes from
- The self test also edits a button in a Geode Popup and puts the generated AnchorLayoutOptions on it: after updateLayout() it stays where it was dragged; it also types values, goes into a level with `UI_TWEAKER_SELFTEST=play` and into the level editor with `editor`, and measures the hover and the tree among 5000 more nodes

# v0.3.0

- Russian and English (the *Language* setting, Auto follows Windows): TTF labels in Russian, since GD's bitmap fonts have no Cyrillic. The generated code stays English
- `Ctrl`+arrows move the anchor point (the node stays in place), `[` and `]` the z-order; both undo and are in the code
- The code for a node in an axis layout has its anchor point and z-order too
- Fixes from a code review: a hovered tree row's node could be freed mid-frame and drawn (a crash); the tree held every node of the scene; the wheel during a drag; CopySizeLayout children; getChildByType indices with subclasses; the scale before AxisLayoutOptions, keeping the node's own options; a button's base scale; text fields keep no focus in the edit mode; a mouse button as the toggle; IDs escaped
- The self test also removes a selected node and changes scenes twice in the edit mode
- With Improved Logger: the selection and the count of edited nodes pinned in its console, a marker when code is copied

# v0.2.0

- Snapping while dragging: the node's edges and center to the screen's, the parent's and the siblings', with guides; `Ctrl` or the setting turns it off
- Why a node doesn't move or show, in the panel: its parent's layout, running actions, an invisible parent, opacity or scale 0, no content size, an ignored anchor point, a clipping parent, a turned parent, off the screen
- `A`: the 16:10, 4:3 and 21:9 screens over this one, the selected node on each, a red box around every node that wouldn't fit; the panel names the screens
- `T`: the node tree with a search by ID and class
- `Alt` held: the distances from the selected node to the hovered one, as in Figma
- Every edit, undo and selection is in the log
- "Copy all" is the nodes with edits not undone

# v0.1.0

- The edit mode (`F8`): hover, select, drag, the wheel, the arrows, undo and redo; the game gets no clicks meanwhile
- The panel of the selected node: position, scale, anchor point, size, parent, z-order, its layout's warning
- Copy as C++: by ID, relative to the closest edge or center, AnchorLayout options, gaps for axis layouts
- `UI_TWEAKER_SELFTEST=1`: edits nodes of the main menu, checks the code and the undo
