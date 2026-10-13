#pragma once

// The node tree's list: which rows a search keeps, and the search's typing, without Cocos in it
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tree
{
  struct Row
  {
    int depth = 0;
    std::string id;
    std::string className;
    // What it shows: a sprite's frame, a label's text (filled while searching)
    std::string look;
  };

  // Whether every word of the query is in the row's ID, class or look, ignoring case
  bool matches(Row const& row, std::string_view query);

  // The rows to show for a query: all for an empty one, else the matches and, for context, their ancestors
  std::vector<size_t> filter(std::vector<Row> const& rows, std::string_view query);

  // The character a key types into the search (Windows virtual key codes, which are Cocos' too): letters
  // lower case, digits, and - . / _ (Shift with -), also Geode's own code for the slash key. Nothing for
  // other keys
  std::optional<char> typed(int key, bool shift);

  // The first row to show so that `row` is visible in a window of `visible` rows starting at `first`
  size_t scrollTo(size_t first, size_t row, size_t visible);
} // namespace tree
