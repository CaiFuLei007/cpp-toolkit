# =================================================================
#                         基础配置
# =================================================================

cmake_minimum_required(VERSION 3.18)

project(
    cpp-toolkit
    VERSION 1.0.0
    LANGUAGES CXX
)

# 设置C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

 # 设置输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/release)

# 设置可执行程序构建类型
set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_BUILD_TYPE "Release")

# 设置优化级别
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
# 抑制 fmt/spdlog 头文件在 GCC 13 上的 -Wstringop-overflow 误报
# (来自 fmt::v9::detail::bigint::assign_pow10 的模板代码)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-stringop-overflow")

# =================================================================
#                         生成可执行程序
# =================================================================

file(GLOB
    SRC_LISTS
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.cpp"
)

# 生成可执行程序 TODO
# all_test: stdout 模式 (initLogger("stdout", ...))
add_executable(all_test
    ${SRC_LISTS}
    ./all_test.cpp
)

# all_test_file: file 模式 (initLogger(file_path, ...))
add_executable(all_test_file
    ${SRC_LISTS}
    ./all_test_file.cpp
)

# =================================================================
#                         添加依赖
# =================================================================


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

find_package(GTest REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(spdlog REQUIRED)

# 链接库  TODO
target_link_libraries(
    all_test              # 可执行程序（add_executable）或库（add_library）名称
    PRIVATE               # PRIVATE/PUBLIC/INTERFACE
    GTest::GTest          # 链接 GTest 库（all_test.cpp 自带 main，不链接 GTest::Main）
    spdlog::spdlog        # 链接 spdlog 库
    jsoncpp
)

target_link_libraries(
    all_test_file
    PRIVATE
    GTest::GTest
    spdlog::spdlog
    jsoncpp
)