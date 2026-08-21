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

# rpc.cpp / etcd.cpp 依赖 brpc/protobuf/etcd, 不编入与 rpc 无关的单元测试, 保持其构建轻量
list(REMOVE_ITEM SRC_LISTS
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/rpc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/etcd.cpp"
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
#                         rpc_test (brpc + protobuf)
# =================================================================

find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(BRPC REQUIRED brpc)

# 用 protoc 生成 cal.pb.cc / cal.pb.h
set(RPC_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rpc_etcd_test)
set(PROTO_FILE ${RPC_TEST_DIR}/cal.proto)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})
set(PROTO_SRCS ${GENERATED_DIR}/cal.pb.cc)
set(PROTO_HDRS ${GENERATED_DIR}/cal.pb.h)
add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            --cpp_out=${GENERATED_DIR}
            -I${RPC_TEST_DIR}
            ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating protobuf C++ from ${PROTO_FILE}"
    VERBATIM
)

# libbrpc.a 是静态库, 链接它时需要带上其私有 (传递) 依赖
set(BRPC_PRIVATE_LIBS gflags leveldb protoc ssl crypto z rt dl pthread)

# rpc/etcd 封装源码 (依赖 brpc/protobuf/etcd)
set(RPC_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/rpc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/etcd.cpp
)

# etcd-cpp-apiv3 头/库 (libetcd-cpp-api.so), 安装在 /usr/local
set(ETCD_INCLUDE_DIR /usr/local/include)
set(ETCD_LIB_DIR /usr/local/lib)

# rpc 服务端 : ServerFactory 创建 server 提供 CalService.Add, 并用 SvcProvider 注册到 etcd
add_executable(rpc_etcd_server
    ${RPC_SRCS}
    ${PROTO_SRCS}
    ${RPC_TEST_DIR}/rpc_etcd_server.cpp
)
target_include_directories(rpc_etcd_server PRIVATE
    ${GENERATED_DIR} ${RPC_TEST_DIR} ${ETCD_INCLUDE_DIR}
)
target_link_directories(rpc_etcd_server PRIVATE ${ETCD_LIB_DIR})
target_link_libraries(rpc_etcd_server PRIVATE
    ${BRPC_LDFLAGS} ${BRPC_PRIVATE_LIBS}
    protobuf::libprotobuf
    etcd-cpp-api
)

# rpc 客户端 : SvcWatcher 从 etcd 发现服务 + ChannelManager 缓存 + ClosureFactory 异步回调
add_executable(rpc_etcd_client
    ${RPC_SRCS}
    ${PROTO_SRCS}
    ${RPC_TEST_DIR}/rpc_etcd_client.cpp
)
target_include_directories(rpc_etcd_client PRIVATE
    ${GENERATED_DIR} ${RPC_TEST_DIR} ${ETCD_INCLUDE_DIR}
)
target_link_directories(rpc_etcd_client PRIVATE ${ETCD_LIB_DIR})
target_link_libraries(rpc_etcd_client PRIVATE
    ${BRPC_LDFLAGS} ${BRPC_PRIVATE_LIBS}
    protobuf::libprotobuf
    etcd-cpp-api
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

# FastDFS 客户端库 (fdfs.cpp 依赖), 位于 /usr/lib
set(FDFS_LIBS
    /usr/lib/libfdfsclient.so
    /usr/lib/libfastcommon.so
)

# 链接库  TODO
target_link_libraries(
    all_test              # 可执行程序（add_executable）或库（add_library）名称
    PRIVATE               # PRIVATE/PUBLIC/INTERFACE
    GTest::GTest          # 链接 GTest 库（all_test.cpp 自带 main，不链接 GTest::Main）
    spdlog::spdlog        # 链接 spdlog 库
    jsoncpp
    ${FDFS_LIBS}
)

target_link_libraries(
    all_test_file
    PRIVATE
    GTest::GTest
    spdlog::spdlog
    jsoncpp
    ${FDFS_LIBS}
)