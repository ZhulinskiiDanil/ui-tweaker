#include "../src/core/Tree.hpp"
#include <algorithm>
#include "Check.hpp"

using namespace tree;

int main()
{
  std::vector<Row> rows = {
      {0, "", "MenuLayer"},
      {1, "main-menu", "CCMenu"},
      {2, "play-button", "CCMenuItemSpriteExtra"},
      {2, "editor-button", "CCMenuItemSpriteExtra"},
      {1, "bottom-menu", "CCMenu"},
      {2, "geode.loader/geode-button", "CCMenuItemSpriteExtra"},
      {1, "main-title", "CCSprite"},
  };

  CHECK(matches(rows[2], "play"));
  CHECK(matches(rows[2], "PLAY"));
  CHECK(matches(rows[2], "spriteextra play"));
  CHECK(!matches(rows[2], "play editor"));
  CHECK(matches(rows[2], ""));
  CHECK(matches(rows[5], "loader/geode"));

  // All rows for nothing typed
  CHECK(filter(rows, "").size() == rows.size());
  CHECK(filter(rows, "  ").size() == rows.size());

  // A match with its parents, in order
  auto shown = filter(rows, "geode-button");
  CHECK(shown.size() == 3 && shown[0] == 0 && shown[1] == 4 && shown[2] == 5);

  // Two matches under one parent: the parent once
  shown = filter(rows, "button");
  CHECK(shown.size() == 6);
  // The title's row isn't kept: it doesn't match and isn't a parent of one
  CHECK(std::find(shown.begin(), shown.end(), 6) == shown.end());

  CHECK(filter(rows, "nothing like it").empty());

  CHECK(typed('A', false) == 'a');
  CHECK(typed('Z', true) == 'z');
  CHECK(typed('7', false) == '7');
  CHECK(!typed('7', true));
  CHECK(typed(0x63, false) == '3');
  CHECK(typed(0xBD, false) == '-');
  CHECK(typed(0xBD, true) == '_');
  CHECK(typed(0xBE, false) == '.');
  CHECK(typed(0xBF, false) == '/');
  CHECK(typed(0x1007, false) == '/');
  CHECK(typed(0x20, false) == ' ');
  CHECK(!typed(0x25, false));

  // Scrolling to keep a row in view
  CHECK(scrollTo(0, 5, 10) == 0);
  CHECK(scrollTo(0, 12, 10) == 3);
  CHECK(scrollTo(8, 3, 10) == 3);
  CHECK(scrollTo(8, 17, 10) == 8);
  return finish("tree");
}
