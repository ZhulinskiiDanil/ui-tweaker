#pragma once

// Text for the panel, whose bitmap font has no wrapping of its own
#include <string>
#include <string_view>
#include <vector>

namespace text
{
  // Lines of at most `columns` characters, broken at spaces where there are any. A word longer than a line is
  // cut. Newlines in the text are kept
  std::vector<std::string> wrap(std::string_view text, size_t columns);

  // At most `columns` characters, the end replaced with ".." if it was longer
  std::string fit(std::string_view text, size_t columns);
} // namespace text
