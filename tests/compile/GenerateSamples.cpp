// Writes the generated code of every sample as C++ to compile: a class per sample whose run() is the code, as it
// would be in a hook of a layer (`this` being the layer), and one with the code of all of them at once
#include "Samples.hpp"

#include <cstdio>
#include <fstream>

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::fprintf(stderr, "usage: generate-samples <out.inc>\n");
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

  out << "inline void runSamples()\n{\n";
  for (size_t i = 0; i < all.size(); i++)
    out << "  {\n    Sample" << i << " layer;\n    buildScene(layer);\n    layer.run(nodeOf(" << i
        << "));\n    checkSample(" << i << ");\n  }\n";
  out << "  {\n    AllSamples layer;\n    buildScene(layer);\n    layer.run(nodeOf(NO_ID));\n";
  out << "    for (int i = 0; i < " << all.size() << "; i++)\n      checkSample(i);\n  }\n}\n";
  return 0;
}
