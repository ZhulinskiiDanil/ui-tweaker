#include "Fields.hpp"

fields::Fields::Fields(std::vector<std::string> values) : m_values(std::move(values)) {}

void fields::Fields::type(char c)
{
  if (m_values.empty())
    return;
  auto& value = m_values[m_current];
  if (m_fresh)
    value.clear();
  m_fresh = false;
  if (value.size() < 40)
    value += c;
}

void fields::Fields::erase()
{
  if (m_values.empty())
    return;
  m_fresh = false;
  auto& value = m_values[m_current];
  if (!value.empty())
    value.pop_back();
}

void fields::Fields::next()
{
  if (m_values.empty())
    return;
  m_current = (m_current + 1) % m_values.size();
  m_fresh = true;
}

void fields::Fields::previous()
{
  if (m_values.empty())
    return;
  m_current = (m_current + m_values.size() - 1) % m_values.size();
  m_fresh = true;
}

std::vector<std::optional<float>> fields::Fields::evaluate(expr::Lookup const& lookup) const
{
  std::vector<std::optional<float>> out;
  for (auto const& value : m_values)
    out.push_back(expr::evaluate(value, lookup));
  return out;
}

std::optional<char> fields::typed(int key, bool shift)
{
  if (key >= '0' && key <= '9')
  {
    if (!shift)
      return static_cast<char>(key);
    if (key == '8')
      return '*';
    if (key == '9')
      return '(';
    if (key == '0')
      return ')';
    return std::nullopt;
  }
  // Numpad digits
  if (key >= 0x60 && key <= 0x69)
    return static_cast<char>('0' + key - 0x60);
  switch (key)
  {
  case 'W':
    return 'w';
  case 'H':
    return 'h';
  case 'P':
    return 'p';
  case 0x6A: // numpad *
    return '*';
  case 0x6B: // numpad +
    return '+';
  case 0x6D: // numpad -
    return '-';
  case 0x6E: // numpad .
  case 0xBE: // VK_OEM_PERIOD
    return '.';
  case 0x6F:   // numpad /
  case 0xBF:   // VK_OEM_2
  case 0x1007: // Geode's KEY_Slash
    return '/';
  case 0xBD: // VK_OEM_MINUS
    return '-';
  case 0xBB:   // VK_OEM_PLUS: = and, with Shift, +
  case 0x1001: // Geode's KEY_OEMEqual
    return shift ? std::optional<char>('+') : std::nullopt;
  case 0x20:
    return ' ';
  default:
    return std::nullopt;
  }
}
