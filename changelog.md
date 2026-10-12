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
