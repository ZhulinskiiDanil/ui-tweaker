#include "../src/core/Text.hpp"
#include "../src/core/Words.hpp"
#include "Check.hpp"

#include <string>
#include <vector>

using namespace words;

namespace
{
  // Every field, in the order they're declared
  std::vector<std::string const*> fields(Words const& w)
  {
    return {&w.banner, &w.helpHint, &w.help1, &w.help2, &w.nothingSelected, &w.clickHint, &w.position, &w.scale, &w.rotation,
            &w.anchor, &w.ignored, &w.size, &w.zOrder, &w.none, &w.copy, &w.copyAll, &w.undo, &w.redo, &w.leave,
            &w.offScreen, &w.selectFirst, &w.nothingEdited, &w.copiedFor, &w.copiedNodes, &w.treeTitle,
            &w.searchHint, &w.search, &w.anchorLayout, &w.axisLayout, &w.simpleAxisLayout, &w.otherLayout,
            &w.layoutMoves, &w.action, &w.actions, &w.hidden, &w.itself, &w.transparent, &w.zeroScale, &w.zeroSize,
            &w.ignoresAnchor, &w.clipped, &w.parentTurned, &w.outsideScreen, &w.tag, &w.scaleField, &w.typingHint,
            &w.typingError, &w.selectedCount};
  }
} // namespace

int main()
{
  CHECK(text::format("a {} b {}", {"1", "2"}) == "a 1 b 2");
  CHECK(text::format("{}{}", {"x"}) == "x{}");
  CHECK(text::format("none", {"x"}) == "none");
  CHECK(text::holes("{} and {}") == 2);
  CHECK(text::holes("") == 0);

  // Both languages have every word, with as many places for values
  auto english = fields(get(Lang::English));
  auto russian = fields(get(Lang::Russian));
  CHECK(english.size() == russian.size());
  // The struct has no field the list misses
  CHECK(sizeof(Words) == english.size() * sizeof(std::string));
  for (size_t i = 0; i < english.size() && i < russian.size(); i++)
  {
    if (english[i]->empty() || russian[i]->empty())
      std::printf("empty word #%zu\n", i);
    CHECK(!english[i]->empty() && !russian[i]->empty());
    if (text::holes(*english[i]) != text::holes(*russian[i]))
      std::printf("word #%zu: %s\n", i, english[i]->c_str());
    CHECK(text::holes(*english[i]) == text::holes(*russian[i]));
  }
  CHECK(get(Lang::English).copy == "Copy");
  CHECK(get(Lang::Russian).copy != "Copy");
  return finish("words");
}
