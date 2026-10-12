cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The math of the mod, without Geode: spaces, expressions, the generated code, snapping, the undo history
project(ui-tweaker-tests CXX)

enable_testing()

add_executable(geometry-tests GeometryTests.cpp ../src/core/Geometry.cpp)
add_test(NAME geometry COMMAND geometry-tests)

add_executable(expr-tests ExprTests.cpp ../src/core/Expr.cpp)
add_test(NAME expr COMMAND expr-tests)

add_executable(codegen-tests CodegenTests.cpp ../src/core/Codegen.cpp ../src/core/Expr.cpp ../src/core/Geometry.cpp
    ../src/core/Words.cpp ../src/core/Text.cpp)
add_test(NAME codegen COMMAND codegen-tests)

add_executable(history-tests HistoryTests.cpp)
add_test(NAME history COMMAND history-tests)

add_executable(text-tests TextTests.cpp ../src/core/Text.cpp)
add_test(NAME text COMMAND text-tests)

add_executable(snap-tests SnapTests.cpp ../src/core/Snap.cpp)
add_test(NAME snap COMMAND snap-tests)

add_executable(explain-tests ExplainTests.cpp ../src/core/Explain.cpp ../src/core/Words.cpp ../src/core/Text.cpp)
add_test(NAME explain COMMAND explain-tests)

add_executable(aspect-tests AspectTests.cpp ../src/core/Aspect.cpp ../src/core/Expr.cpp)
add_test(NAME aspect COMMAND aspect-tests)

add_executable(tree-tests TreeTests.cpp ../src/core/Tree.cpp)
add_test(NAME tree COMMAND tree-tests)

add_executable(measure-tests MeasureTests.cpp ../src/core/Measure.cpp)
add_test(NAME measure COMMAND measure-tests)

add_executable(words-tests WordsTests.cpp ../src/core/Words.cpp ../src/core/Text.cpp)
add_test(NAME words COMMAND words-tests)

add_executable(fields-tests FieldsTests.cpp ../src/core/Fields.cpp ../src/core/Expr.cpp)
add_test(NAME fields COMMAND fields-tests)

# The generated code itself, compiled against a fake of Geode's API and run on fake nodes
set(CODEGEN_SOURCES ../src/core/Codegen.cpp ../src/core/Expr.cpp ../src/core/Geometry.cpp ../src/core/Words.cpp
    ../src/core/Text.cpp ../src/core/Hooks.cpp)
add_executable(generate-samples compile/GenerateSamples.cpp ${CODEGEN_SOURCES})
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/samples.inc ${CMAKE_CURRENT_BINARY_DIR}/hooks.inc
    COMMAND generate-samples ${CMAKE_CURRENT_BINARY_DIR}/samples.inc ${CMAKE_CURRENT_BINARY_DIR}/hooks.inc
    DEPENDS generate-samples
    COMMENT "Generating the samples' code")
add_executable(compiled-code-tests compile/RunSamples.cpp ${CMAKE_CURRENT_BINARY_DIR}/samples.inc
    ${CMAKE_CURRENT_BINARY_DIR}/hooks.inc ${CODEGEN_SOURCES})
target_include_directories(compiled-code-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR} compile/include)
add_test(NAME compiled-code COMMAND compiled-code-tests)

add_executable(hooks-tests HooksTests.cpp ../src/core/Hooks.cpp)
add_test(NAME hooks COMMAND hooks-tests)
