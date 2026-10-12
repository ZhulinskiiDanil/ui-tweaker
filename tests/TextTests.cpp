#include "../src/core/Text.hpp"
#include "Check.hpp"

using namespace text;

int main()
{
  auto lines = wrap("main-menu has a RowLayout: updateLayout() puts the node back", 20);
  CHECK(lines.size() == 4);
  CHECK(lines.size() == 4 && lines[0] == "main-menu has a" && lines[1] == "RowLayout:" &&
        lines[2] == "updateLayout() puts" && lines[3] == "the node back");
  for (auto const& line : lines)
    CHECK(line.size() <= 20);

  // A word longer than a line is cut
  lines = wrap("abcdefghij xy", 4);
  CHECK(lines.size() == 4 && lines[0] == "abcd" && lines[1] == "efgh" && lines[2] == "ij" && lines[3] == "xy");

  // Newlines are kept, an empty line too
  lines = wrap("one\n\ntwo", 10);
  CHECK(lines.size() == 3 && lines[0] == "one" && lines[1].empty() && lines[2] == "two");

  CHECK(wrap("", 10).size() == 1);
  CHECK(wrap("x", 0).empty());

  CHECK(fit("CCMenuItemSpriteExtra", 10) == "CCMenuIt..");
  CHECK(fit("short", 10) == "short");
  CHECK(fit("abc", 2) == "ab");
  // Letters, not bytes: "привет" is 6 letters of 12 bytes
  CHECK(fit("привет", 6) == "привет");
  CHECK(fit("привет", 5) == "при..");
  CHECK(fit("привет", 2) == "пр");

  // A Russian line is as long as an English one: "вернёт ноду в ряд," is 18 letters
  lines = wrap("вернёт ноду в ряд, позиция потеряется", 20);
  CHECK(lines.size() == 2 && lines[0] == "вернёт ноду в ряд," && lines[1] == "позиция потеряется");
  // A long Russian word is cut at letters
  lines = wrap("абвгдеж", 3);
  CHECK(lines.size() == 3 && lines[0] == "абв" && lines[1] == "где" && lines[2] == "ж");
  return finish("text");
}
