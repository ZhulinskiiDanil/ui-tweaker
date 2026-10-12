#include "Expr.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace
{
  float round2(float value)
  {
    float rounded = std::round(value * 100.f) / 100.f;
    // No "-0.f"
    return rounded == 0 ? 0.f : rounded;
  }

  // ! --- Parser --- !

  class Parser
  {
  public:
    Parser(std::string_view text, expr::Lookup const& lookup) : m_text(text), m_lookup(lookup) {}

    std::optional<float> parse()
    {
      auto value = sum();
      skipSpaces();
      if (!value || m_at != m_text.size())
        return std::nullopt;
      return value;
    }

  private:
    std::string_view m_text;
    expr::Lookup const& m_lookup;
    size_t m_at = 0;

    void skipSpaces()
    {
      while (m_at < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_at])))
        m_at++;
    }

    bool take(char c)
    {
      skipSpaces();
      if (m_at < m_text.size() && m_text[m_at] == c)
      {
        m_at++;
        return true;
      }
      return false;
    }

    std::optional<float> sum()
    {
      auto value = product();
      while (value)
      {
        if (take('+'))
        {
          auto right = product();
          if (!right)
            return std::nullopt;
          *value += *right;
        }
        else if (take('-'))
        {
          auto right = product();
          if (!right)
            return std::nullopt;
          *value -= *right;
        }
        else
          break;
      }
      return value;
    }

    std::optional<float> product()
    {
      auto value = unary();
      while (value)
      {
        if (take('*'))
        {
          auto right = unary();
          if (!right)
            return std::nullopt;
          *value *= *right;
        }
        else if (take('/'))
        {
          auto right = unary();
          if (!right || *right == 0)
            return std::nullopt;
          *value /= *right;
        }
        else
          break;
      }
      return value;
    }

    std::optional<float> unary()
    {
      if (take('-'))
      {
        auto value = unary();
        if (!value)
          return std::nullopt;
        return -*value;
      }
      if (take('+'))
        return unary();
      return primary();
    }

    std::optional<float> primary()
    {
      skipSpaces();
      if (m_at >= m_text.size())
        return std::nullopt;
      if (take('('))
      {
        auto value = sum();
        if (!value || !take(')'))
          return std::nullopt;
        return value;
      }

      char c = m_text[m_at];
      if (std::isdigit(static_cast<unsigned char>(c)) || c == '.')
        return literal();
      if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        return name();
      return std::nullopt;
    }

    std::optional<float> literal()
    {
      size_t start = m_at;
      bool digits = false;
      while (m_at < m_text.size() && (std::isdigit(static_cast<unsigned char>(m_text[m_at])) || m_text[m_at] == '.'))
      {
        digits = digits || m_text[m_at] != '.';
        m_at++;
      }
      if (!digits)
        return std::nullopt;
      std::string number(m_text.substr(start, m_at - start));
      if (m_at < m_text.size() && (m_text[m_at] == 'f' || m_text[m_at] == 'F'))
        m_at++;
      char* end = nullptr;
      float value = std::strtof(number.c_str(), &end);
      if (end != number.c_str() + number.size())
        return std::nullopt;
      return value;
    }

    std::optional<float> name()
    {
      size_t start = m_at;
      while (m_at < m_text.size() &&
             (std::isalnum(static_cast<unsigned char>(m_text[m_at])) || m_text[m_at] == '_' || m_text[m_at] == '.'))
        m_at++;
      return m_lookup(m_text.substr(start, m_at - start));
    }
  };
} // namespace

expr::Linear expr::nearest(float value, float extent)
{
  if (!(extent > 0))
    return {0, value};
  // The center first, so a tie goes to it
  Linear best{0.5f, value - extent / 2};
  for (float k : {0.f, 1.f})
  {
    float offset = value - k * extent;
    if (std::fabs(offset) < std::fabs(best.offset))
      best = {k, offset};
  }
  return best;
}

expr::Linear expr::relativeTo(Linear value, Linear origin)
{
  return {value.k - origin.k, value.offset - origin.offset};
}

float expr::evaluate(Linear value, float extent)
{
  return value.k * extent + value.offset;
}

std::string expr::number(float value)
{
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%.2f", round2(value));
  std::string text = buffer;
  // "40.00" to "40.", "12.50" to "12.5"
  while (text.back() == '0')
    text.pop_back();
  return text + "f";
}

std::string expr::format(Linear value, std::string_view extent)
{
  float offset = round2(value.offset);
  std::string base;
  if (value.k == 0)
    return number(offset);
  else if (value.k == 1)
    base = std::string(extent);
  else if (value.k == -1)
    base = "-" + std::string(extent);
  else if (value.k == 0.5f)
    base = std::string(extent) + " / 2";
  else if (value.k == -0.5f)
    base = "-" + std::string(extent) + " / 2";
  else
    base = std::string(extent) + " * " + number(value.k);

  if (offset == 0)
    return base;
  if (offset > 0)
    return base + " + " + number(offset);
  return base + " - " + number(-offset);
}

std::optional<float> expr::evaluate(std::string_view text, Lookup const& lookup)
{
  return Parser(text, lookup).parse();
}

std::vector<std::string> expr::callArguments(std::string_view code, std::string_view function)
{
  std::string needle = std::string(function) + "(";
  auto start = code.find(needle);
  if (start == std::string_view::npos)
    return {};
  size_t at = start + needle.size();

  // Up to the matching parenthesis
  int depth = 1;
  size_t end = at;
  for (; end < code.size() && depth > 0; end++)
  {
    if (code[end] == '(' || code[end] == '{')
      depth++;
    else if (code[end] == ')' || code[end] == '}')
      depth--;
  }
  if (depth != 0)
    return {};
  auto inside = code.substr(at, end - 1 - at);

  // One braced argument: its insides are the arguments
  auto trimmed = inside;
  while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front())))
    trimmed.remove_prefix(1);
  while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())))
    trimmed.remove_suffix(1);
  if (trimmed.size() >= 2 && trimmed.front() == '{' && trimmed.back() == '}')
    inside = trimmed.substr(1, trimmed.size() - 2);

  std::vector<std::string> arguments;
  std::string current;
  depth = 0;
  for (char c : inside)
  {
    if (c == '(' || c == '{')
      depth++;
    else if (c == ')' || c == '}')
      depth--;
    if (c == ',' && depth == 0)
    {
      arguments.push_back(current);
      current.clear();
      continue;
    }
    current += c;
  }
  arguments.push_back(current);
  for (auto& argument : arguments)
  {
    while (!argument.empty() && std::isspace(static_cast<unsigned char>(argument.front())))
      argument.erase(argument.begin());
    while (!argument.empty() && std::isspace(static_cast<unsigned char>(argument.back())))
      argument.pop_back();
  }
  return arguments;
}
