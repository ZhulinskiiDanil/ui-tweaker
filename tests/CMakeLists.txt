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

add_executable(codegen-tests CodegenTests.cpp ../src/core/Codegen.cpp ../src/core/Expr.cpp ../src/core/Geometry.cpp)
add_test(NAME codegen COMMAND codegen-tests)

add_executable(history-tests HistoryTests.cpp)
add_test(NAME history COMMAND history-tests)

add_executable(text-tests TextTests.cpp ../src/core/Text.cpp)
add_test(NAME text COMMAND text-tests)

add_executable(snap-tests SnapTests.cpp ../src/core/Snap.cpp)
add_test(NAME snap COMMAND snap-tests)

add_executable(explain-tests ExplainTests.cpp ../src/core/Explain.cpp)
add_test(NAME explain COMMAND explain-tests)

add_executable(aspect-tests AspectTests.cpp ../src/core/Aspect.cpp ../src/core/Expr.cpp)
add_test(NAME aspect COMMAND aspect-tests)
