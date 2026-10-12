#include "../src/core/Fields.hpp"
#include "Check.hpp"

using namespace fields;

namespace
{
  std::optional<float> names(std::string_view name)
  {
    if (name == "w")
      return 569.f;
    if (name == "h")
      return 320.f;
    if (name == "pw")
      return 200.f;
    if (name == "ph")
      return 100.f;
    return std::nullopt;
  }

  void typeAll(Fields& f, std::string const& text)
  {
    for (char c : text)
      f.type(c);
  }
} // namespace

int main()
{
  Fields f({"284.5", "175", "1"});
  CHECK(f.current() == 0);
  // Typing replaces the field's value, then adds to it
  typeAll(f, "w/2+40");
  CHECK(f.values()[0] == "w/2+40");
  f.next();
  // Back to a field: typed into, it's replaced again
  f.erase();
  CHECK(f.values()[1] == "17");
  typeAll(f, "5");
  CHECK(f.values()[1] == "175");
  f.next();
  typeAll(f, "1.2");
  auto values = f.evaluate(names);
  CHECK(values.size() == 3);
  CHECK(values[0] && near(*values[0], 324.5f));
  CHECK(values[1] && near(*values[1], 175));
  CHECK(values[2] && near(*values[2], 1.2f));

  // Round and round
  f.next();
  CHECK(f.current() == 0);
  f.previous();
  CHECK(f.current() == 2);

  // A field that can't be read
  Fields bad({"h - 25", "pw -", "ph / 2"});
  values = bad.evaluate(names);
  CHECK(values[0] && near(*values[0], 295));
  CHECK(!values[1]);
  CHECK(values[2] && near(*values[2], 50));

  // Keys to characters
  CHECK(typed('5', false) == '5');
  CHECK(typed('8', true) == '*');
  CHECK(typed('9', true) == '(');
  CHECK(typed('0', true) == ')');
  CHECK(!typed('5', true));
  CHECK(typed(0x63, false) == '3');
  CHECK(typed('W', false) == 'w');
  CHECK(typed('P', false) == 'p');
  CHECK(!typed('Q', false));
  CHECK(typed(0x6B, false) == '+');
  CHECK(typed(0xBB, true) == '+');
  CHECK(!typed(0xBB, false));
  CHECK(typed(0x1001, true) == '+');
  CHECK(typed(0xBD, false) == '-');
  CHECK(typed(0x1007, false) == '/');
  CHECK(typed(0xBE, false) == '.');
  return finish("fields");
}
