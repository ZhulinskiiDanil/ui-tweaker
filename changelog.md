# v0.11.1 (not released yet: on main)

- A node moved back where it was (`R`, by hand, or by another mod that puts it back) isn't in "Copy all" or its count any more: its code would change nothing
- `Alt+C` copies only the way to the selected node (its lookup around an empty block, `node` cast to its class), for code of your own that needs a node without an ID
- The generated code is compiled against the real Geode headers too (`tools/codecheck.sh`): every kind of it builds as it's pasted
- `Ctrl+D` makes Geode's circle buttons too (`CircleButtonSprite` with its icon, its base's color and size, as the Geode button is)
- The comment for a node in a SimpleAxisLayout says what an `AxisGap` does: it replaces the layout's gap before the node and the whole line is laid out again (a centered row moves), so it stays a suggestion

# v0.11.0

- Values typed with several nodes selected: the fields typed into go to each of them (`pw`, `ph` its own parent's), one step to undo; the others stay each node's own. A scale for a whole row of buttons at once
- `Ctrl+D` makes a node like the selected one next to it: a sprite, a button or a label, to drag where it goes; its code makes it (`CCMenuItemSpriteExtra::create` with your callback instead of `nullptr`) and adds it to the same parent. Undo takes it out
- The tree's search finds nodes by what they show too: a sprite's or button's frame, a label's text (`playbtn` finds the play button)
- "Copy all" says a mod's own Geode Popup's code goes in its `init()`, after `Popup::init()`

# v0.10.0

- The edits are kept between runs of the game (`session.txt` in the save folder): after a rebuild and a restart, `Ctrl+Shift+C` still has what wasn't pasted yet. `Ctrl+Delete` forgets them
- *Put edits back* (off by default): while the edit mode is on, a screen made again, or the game started again, gets its kept edits back on the nodes, found as their code finds them, one step to undo; a node already there (its code pasted) is left alone
- The *Grid* setting (0 by default: none): a dragged node's code numbers go in steps of it from the closest edge or center of the screen or its parent (`winSize.width / 2 - 20.f`), the grid drawn while dragging; guides go first, `Ctrl` drags freely
- The panel stays out of the part of a windowed game the taskbar covers (its buttons were under it at the bottom)
- `Ctrl`+wheel turns the selected nodes (15 degrees a notch, `Shift`: 1); `Enter` also types the rotation and the opacity. Written in the code only when the edit mode changed them, so a node the game spins or fades isn't turned in the code
- The self test's tour goes through thirteen scenes: your levels' list, a level's page, a quick popup, the settings and your profile too (947 checks in vanilla)
- An edit of a node without an ID made again in a new instance of its layer replaces the older one in "Copy all" (it was there twice)

# v0.9.0

- `Alt`+`A` `D` `H` / `W` `S` `V` align the selected nodes on the screen as in Figma: several to the box of them all, one to its parent's box; `Alt`+`Shift`+`H` / `V` distribute three or more with the same gaps. One step to undo, the code as for any move
- `L` copies the selected nodes of one parent as a row or a column: a new menu with a `RowLayout`/`ColumnLayout` whose gap, size, position and alignment keep them where they are, and the code that moves them into it (checked with Geode's own layouts in the self test)
- Nodes without an ID are found even when their parent has none either: down from the closest parent with an ID (or the layer itself), each level by a child's ID, the frame a sprite or button shows, or its class and index, in nested `if`s with a comment when that's fragile. An ID another node in the layer has first isn't used. The tour follows every such path in seven scenes (74 nodes in vanilla)
- Undo and redo keep a selection that has every node of the step
- The edit mode's key works while a text field has the keyboard (Geode's keybinds don't by default), and takes the keyboard from the field
- In a level, the edit mode shows the cursor the level hid and lets go of one GD's *Lock cursor* option locked (a locked cursor's position doesn't change, so the mouse did nothing); leaving the edit mode puts them back while the level is played

# v0.8.1

- Russian text in the panel is wrapped as wide as English: lines are counted in letters, not bytes
- The self test's snapping check passes in a vanilla main menu too, where the play button's row is centered a point off the screen's middle
- The other screens' names (16:10, 4:3, 21:9) are on dark plates, readable over a logo of their own color
- README: the GIF and the screenshots made again in a clean, near-vanilla game
- The self test works in a game without a frame limit (it waits for scene transitions to end, not for a number of frames) and with the panel moved out of the way; it is run both with other mods and in a vanilla game (`tools/vanilla.sh`)

# v0.8.0

- Fixes from a fourth review: visibility the game changes (a tab switched) isn't an edit: not in the code, not put back by undo; the panel's look of a node is the node's, a label's text as it is now; the tree keeps hidden and search colors over the edited one; Ctrl+V and Ctrl+R don't hide or reset; K&R never puts a brace after a comment; names cut at a letter, not inside one
- The tree marks edited nodes with `*`, in green
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
