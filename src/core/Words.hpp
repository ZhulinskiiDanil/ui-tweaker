#pragma once

// Every word of the edit mode's interface, in English and Russian. The generated code stays English
#include <string>

namespace words
{
  enum class Lang
  {
    English,
    Russian,
  };

  // {} marks where a value goes, as in fmt
  struct Words
  {
    std::string banner;      // {}: the toggle key
    std::string helpHint;
    std::string help1;
    std::string help2;
    std::string nothingSelected;
    std::string clickHint;
    std::string position;    // {} {}: x, y; {} {}: on the screen
    std::string scale;       // {}: the scale, {}: " x scaleY" or nothing
    std::string rotation;
    std::string anchor;      // {} {}: x, y, {}: " (ignored)" or nothing
    std::string ignored;
    std::string size;        // {} {}
    std::string zOrder;      // {}: z, {}: the parent
    std::string none;
    std::string copy;
    std::string copyAll;     // {}: how many
    std::string undo;
    std::string redo;
    std::string leave;
    std::string offScreen;   // {}: the screens
    std::string selectFirst;
    std::string nothingEdited;
    std::string copiedFor;   // {}: the node
    std::string copiedNodes; // {}: how many
    std::string layoutNeeds;
    std::string copiedLayout; // {} {}: row or column, how many
    std::string row;
    std::string putBack;    // {}: how many
    std::string forgotKept; // {}: how many
    std::string forgetAsk;  // {}: how many
    std::string column;
    std::string treeTitle;   // {} {}: shown, all
    std::string searchHint;
    std::string search;      // {}: the query

    // What a parent's layout does, for the panel (the code's comments stay English). {}: the parent, {}: the layout
    std::string anchorLayout;
    std::string axisLayout;
    std::string simpleAxisLayout;
    std::string otherLayout;

    // Why a node acts oddly. {}: the parent and the layout, the count, what hides it, what clips it
    std::string layoutMoves;
    std::string action;
    std::string actions;
    std::string hidden;
    std::string itself;
    std::string transparent;
    std::string zeroScale;
    std::string zeroSize;
    std::string ignoresAnchor;
    std::string clipped;
    std::string parentTurned;
    std::string outsideScreen;
    // By the mouse: the node, then its parent
    std::string tag;
    // Typing values: the scale's field, the keys, a value that can't be read ({})
    std::string scaleField;
    std::string widthField;
    std::string heightField;
    std::string rotationField;
    std::string opacityField;
    std::string opacity; // {}: 0 to 255
    std::string typingHint;
    std::string typingError;
    // How many nodes are selected, with Shift+click
    std::string selectedCount;
    // What the node shows: {} a frame name, a font and text
    std::string look;
  };

  Words const& get(Lang lang);
} // namespace words
