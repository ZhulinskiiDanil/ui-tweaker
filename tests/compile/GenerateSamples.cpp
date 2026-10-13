// Writes the generated code of every sample as C++ to compile: a class per sample whose run() is the code, as it
// would be in a hook of a layer (`this` being the layer), and one with the code of all of them at once
#include "Samples.hpp"

#include "../../src/core/Hooks.hpp"

#include <cstdio>
#include <fstream>

int main(int argc, char** argv)
{
  if (argc < 3)
  {
    std::fprintf(stderr, "usage: generate-samples <samples.inc> <hooks.inc>\n");
    return 1;
  }
  std::ofstream out(argv[1]);
  auto all = samples();
  for (size_t i = 0; i < all.size(); i++)
  {
    out << "// Sample " << i << "\n";
    out << "struct Sample" << i << " : CCNode\n{\n  void run(CCNode* node)\n  {\n    (void)node;\n";
    out << codegen::generate(all[i]);
    out << "  }\n};\n\n";
  }
  out << "// All of them\nstruct AllSamples : CCNode\n{\n  void run(CCNode* node)\n  {\n    (void)node;\n";
  out << codegen::generateAll(all);
  out << "  }\n};\n\n";

  // All of them in K&R style
  out << "struct AllSamplesKAndR : CCNode\n{\n  void run(CCNode* node)\n  {\n    (void)node;\n";
  out << codegen::toKAndR(codegen::generateAll(all));
  out << "  }\n};\n\n";

  // L's code, in both styles: run() in the layer (or the parent's layer), `this` being it
  auto layouts = layoutSamples();
  for (size_t i = 0; i < layouts.size(); i++)
    for (bool kAndR : {false, true})
    {
      auto code = codegen::layoutCode(layouts[i]);
      out << "struct LayoutSample" << i << (kAndR ? "KAndR" : "") << " : CCNode\n{\n  void run()\n  {\n";
      out << (kAndR ? codegen::toKAndR(code) : code);
      out << "  }\n};\n\n";
    }
  out << "inline void runLayoutSamples()\n{\n";
  for (size_t i = 0; i < layouts.size(); i++)
    for (bool kAndR : {false, true})
      out << "  {\n    LayoutSample" << i << (kAndR ? "KAndR" : "") << " layer;\n    buildLayoutScene(layer, " << i
          << ");\n    layer.run();\n    checkLayoutSample(layer, " << i << ");\n  }\n";
  out << "}\n\n";

  out << "inline void runSamples()\n{\n";
  for (size_t i = 0; i < all.size(); i++)
    out << "  {\n    Sample" << i << " layer;\n    buildScene(layer);\n    layer.run(nodeOf(" << i
        << "));\n    checkSample(" << i << ");\n  }\n";
  out << "  {\n    AllSamples layer;\n    buildScene(layer);\n    layer.run(nodeOf(NO_ID));\n";
  out << "    for (int i = 0; i < " << all.size() << "; i++)\n      checkSample(i);\n  }\n";
  out << "  {\n    AllSamplesKAndR layer;\n    buildScene(layer);\n    layer.run(nodeOf(NO_ID));\n";
  out << "    for (int i = 0; i < " << all.size() << "; i++)\n      checkSample(i);\n  }\n}\n";

  // "Copy all" as hooks, at the top level as they're pasted: two nodes in MenuLayer's init, one in PauseLayer's
  // customSetup, one in LevelSelectLayer's init(int)
  std::ofstream hooksOut(argv[2]);
  hooksOut << hooks::wrap("MenuLayer", codegen::generateAll({all[0], all[1]})) << "\n";
  hooksOut << hooks::wrap("PauseLayer", codegen::generateAll({all[2]})) << "\n";
  hooksOut << hooks::wrap("LevelSelectLayer", codegen::generateAll({all[4]})) << "\n";
  // And one in K&R style
  hooksOut << codegen::toKAndR(hooks::wrap("GJGarageLayer", codegen::generateAll({all[3]}))) << "\n";

  // With a third path: everything for the real Geode headers (tests/geode): each sample's code in a node's method,
  // the code of all, the layouts', in both styles, and "Copy all" as it's copied, its preamble and hooks included
  if (argc > 3)
  {
    std::ofstream real(argv[3]);
    real << hooks::preamble() << "\n";
    size_t n = 0;
    auto method = [&](std::string const& code)
    {
      // `node` for the code of a node with no way to find it, which is for where it's made
      real << "struct RealSample" << n++ << " : CCNode\n{\n  void run(CCNode* node)\n  {\n    (void)node;\n" << code
           << "  }\n};\n\n";
    };
    for (bool kAndR : {false, true})
    {
      auto style = [&](std::string const& code) { return kAndR ? codegen::toKAndR(code) : code; };
      for (auto const& sample : all)
      {
        method(style(codegen::generate(sample)));
        // Alt+C's: only the way to it
        if (auto way = codegen::findCode(sample); !way.empty())
          method(style(way));
      }
      method(style(codegen::generateAll(all)));
      for (auto const& layout : layoutSamples())
        method(style(codegen::layoutCode(layout)));
    }
    real << hooks::wrap("MenuLayer", codegen::generateAll({all[0], all[1], all[13], all[17]})) << "\n";
    real << codegen::toKAndR(hooks::wrap("CreatorLayer", codegen::generateAll({all[5], all[14], all[19]}))) << "\n";
  }
  return 0;
}
