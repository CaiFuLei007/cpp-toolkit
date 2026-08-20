# =================================================================
#                         基础配置
# =================================================================

cmake_minimum_required(VERSION 3.18)

project(
    cpp-toolkit
    VERSION 1.0.0
    DESCRIPTION "C++ 工具包静态库"
    LANGUAGES CXX
)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 设置构建类型
set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_BUILD_TYPE "Release")

# 设置安装路径前缀(默认安装到 /usr/local)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "安装路径前缀" FORCE)
endif()

# =================================================================
#                         查找依赖
# =================================================================

find_package(spdlog REQUIRED)
find_package(jsoncpp REQUIRED)

# =================================================================
#                         生成静态库
# =================================================================

file(GLOB
    SRC_LISTS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(cpp_toolkit STATIC
    ${SRC_LISTS}
)

target_include_directories(cpp_toolkit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(cpp_toolkit
    PUBLIC
        spdlog::spdlog
        jsoncpp
)

# =================================================================
#                         安装逻辑
# =================================================================

include(GNUInstallDirs)

# 安装静态库
install(TARGETS cpp_toolkit
    EXPORT cpp_toolkitTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 安装头文件(保留 include 下的目录结构)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
)

# 生成 CMake 配置文件,供下游 find_package(cpp_toolkit) 使用
install(EXPORT cpp_toolkitTargets
    FILE cpp_toolkitTargets.cmake
    NAMESPACE cpp_toolkit::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp_toolkit
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cpp_toolkitConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpp_toolkitConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cpp_toolkitConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp_toolkit
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cpp_toolkitConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cpp_toolkitConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp_toolkit
)
