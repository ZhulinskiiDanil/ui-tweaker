#pragma once

// Positions as C++ expressions relative to an edge or the center (`winSize.width / 2 + 40.f`), and back:
// the generated code is read again to check that it gives the same position
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace expr
{
  // k * extent + offset. k is 0 (start), 0.5 (center) or 1 (end), or a difference of two of those
  struct Linear
  {
    float k = 0;
    float offset = 0;

    bool operator==(Linear const&) const = default;
  };

  // Of the start, center and end of [0, extent], the one closest to `value`. A tie goes to the center,
  // then the start. An empty extent has only the start
  Linear nearest(float value, float extent);

  // `value` measured from `origin` instead of from 0, both in terms of the same extent
  Linear relativeTo(Linear value, Linear origin);

  float evaluate(Linear value, float extent);

  // A float literal: rounded to hundredths, no trailing zeros. 40 is "40.f", 12.5 is "12.5f"
  std::string number(float value);

  // `extent` is the C++ for the extent, like "winSize.width"
  std::string format(Linear value, std::string_view extent);

  // Reads an expression made of float literals, names (`winSize.width`), + - * / and parentheses.
  // `lookup` gives the value of a name; an unknown name or bad syntax is nullopt
  using Lookup = std::function<std::optional<float>(std::string_view)>;
  std::optional<float> evaluate(std::string_view text, Lookup const& lookup);

  // The arguments of the first call to `function` in `code`, split at top-level commas, with the braces of
  // one braced argument opened: `setPosition({a, b})` gives {"a", "b"}. Empty if there is no such call
  std::vector<std::string> callArguments(std::string_view code, std::string_view function);
} // namespace expr
