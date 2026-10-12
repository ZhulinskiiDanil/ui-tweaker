#pragma once

// Typing exact values into the panel: a few fields, one being typed into, each a number or an expression like
// `w / 2 + 40` (w and h the screen's size, pw and ph the parent's)
#include "Expr.hpp"

#include <optional>
#include <string>
#include <vector>

namespace fields
{
  class Fields
  {
  public:
    explicit Fields(std::vector<std::string> values);

    // The first character typed into a field replaces what was there, as in a selected text box
    void type(char c);
    void erase();
    void next();
    void previous();

    size_t current() const { return m_current; }
    std::vector<std::string> const& values() const { return m_values; }

    // Every field read with these names; nullopt for a field that can't be read
    std::vector<std::optional<float>> evaluate(expr::Lookup const& lookup) const;

  private:
    std::vector<std::string> m_values;
    size_t m_current = 0;
    bool m_fresh = true;
  };

  // The character a key types into a field (Windows virtual key codes, Geode's own for some): digits, . - + * /
  // ( ) and the letters of w, h, pw, ph. Nothing for other keys
  std::optional<char> typed(int key, bool shift);
} // namespace fields
