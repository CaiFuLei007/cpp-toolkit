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
        $<INSTALL_INTERFACE:include/cpp-toolkit>
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
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cpp-toolkit
)

# 安装头文件到 include/cpp-toolkit 下
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cpp-toolkit
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

# 直接生成 Config 文件(无需模板文件)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cpp_toolkitConfig.cmake" [===[
include(CMakeFindDependencyMacro)

# cpp_toolkit 依赖 spdlog 与 jsoncpp,下游使用前需先找到它们
find_dependency(spdlog)
find_dependency(jsoncpp)

include("${CMAKE_CURRENT_LIST_DIR}/cpp_toolkitTargets.cmake")
]===])

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cpp_toolkitConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cpp_toolkitConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp_toolkit
)

# =================================================================
#                         卸载目标
# =================================================================

# 生成卸载脚本(无需模板文件),依据 install_manifest.txt 删除已安装的文件
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake" [===[
get_filename_component(BUILD_DIR "${CMAKE_SCRIPT_MODE_FILE}" DIRECTORY)
set(MANIFEST "${BUILD_DIR}/install_manifest.txt")

if(NOT EXISTS "${MANIFEST}")
    message(FATAL_ERROR "未找到 install_manifest.txt,请先执行安装: cmake --install build")
endif()

file(READ "${MANIFEST}" FILES)
string(STRIP "${FILES}" FILES)
string(REGEX REPLACE "\n" ";" FILES "${FILES}")

# 删除清单中的所有文件
foreach(FILE ${FILES})
    if(IS_SYMLINK "$ENV{DESTDIR}${FILE}" OR EXISTS "$ENV{DESTDIR}${FILE}")
        file(REMOVE "$ENV{DESTDIR}${FILE}")
        if(EXISTS "$ENV{DESTDIR}${FILE}" OR IS_SYMLINK "$ENV{DESTDIR}${FILE}")
            message(SEND_ERROR "删除失败(可能需要 sudo): $ENV{DESTDIR}${FILE}")
        else()
            message(STATUS "已删除: $ENV{DESTDIR}${FILE}")
        endif()
    else()
        message(STATUS "文件不存在,跳过: $ENV{DESTDIR}${FILE}")
    endif()
endforeach()

# 收集文件所在目录并去重
set(DIRS "")
foreach(FILE ${FILES})
    get_filename_component(DIR "${FILE}" DIRECTORY)
    list(APPEND DIRS "${DIR}")
endforeach()
list(REMOVE_DUPLICATES DIRS)

# 由深到浅删除因卸载而变空的目录,避免留下残留
list(SORT DIRS ORDER DESCENDING)
foreach(DIR ${DIRS})
    file(GLOB ENTRIES "$ENV{DESTDIR}${DIR}/*" "$ENV{DESTDIR}${DIR}/.*")
    list(FILTER ENTRIES EXCLUDE REGEX "/[.][.]?$")
    if(NOT ENTRIES)
        file(REMOVE_RECURSE "$ENV{DESTDIR}${DIR}")
        if(EXISTS "$ENV{DESTDIR}${DIR}")
            message(SEND_ERROR "删除失败(可能需要 sudo): $ENV{DESTDIR}${DIR}")
        else()
            message(STATUS "已删除空目录: $ENV{DESTDIR}${DIR}")
        endif()
    endif()
endforeach()

message(STATUS "卸载完成")
]===])

# 执行方式: cmake --build build --target uninstall
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
    COMMENT "正在卸载已安装的文件..."
    VERBATIM
)
