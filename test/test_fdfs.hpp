#pragma once

// FastDFS 单元测试
// 前置条件: 本机已启动 FastDFS tracker(storage) 服务(host 网络, tracker 监听 127.0.0.1:22122)
//
// 验证 fdfs.h 中 FdfsClient 的接口:
//   1. 服务端上传文件/缓冲区, 并成功下载
//   2. 删除后再读取, 文件不存在
//   3. 多线程同时上传/下载, 无冲突
//
// 注意: FdfsClient::Init 为进程级全局初始化, 整个测试套件只初始化一次
// (SetUpTestSuite), 结束时在 TearDownTestSuite 中销毁。

#include <gtest/gtest.h>
#include "fdfs.h"
#include <fastcommon/logger.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

using cpp_toolkit::FdfsClient;
using cpp_toolkit::FdfsSettings;

// FastDFS tracker 地址(tracker 容器使用 host 网络)
constexpr const char* kTrackerServer = "127.0.0.1:22122";

// 生成一段确定性的随机内容作为测试数据
std::string MakePayload(size_t size, unsigned seed)
{
    std::string buf(size, '\0');
    std::mt19937 rng(seed ? seed : std::random_device{}());
    for (size_t i = 0; i < size; ++i) {
        buf[i] = static_cast<char>(rng() & 0xff);
    }
    return buf;
}

// 生成本地唯一的临时文件路径(用于上传源文件 / 下载目标文件)
std::string MakeTempPath(const std::string& tag)
{
    static std::atomic<uint64_t> s_counter{0};
    return "/tmp/cpp_toolkit_fdfs_" + tag + "_"
         + std::to_string(s_counter++) + ".bin";
}

// 读取本地文件全部内容
std::string ReadBytes(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());
}

} // anonymous namespace

class FdfsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        FdfsSettings settings;
        settings.tracker_servers_.push_back(kTrackerServer);
        settings.connect_timeout = 10;
        settings.network_timeout = 10;
        ASSERT_TRUE(FdfsClient::Init(settings))
            << "FastDFS 客户端初始化失败, 请确认 tracker 服务已启动 ("
            << kTrackerServer << ")";
        // FdfsClient::Init 内部将日志级别设为 LOG_ERR, 测试运行时不希望打印
        // fastcommon 的噪声日志(如下载不存在文件时的 ERROR)。
        // FC_LOG_BY_LEVEL(level) 为 level <= g_log_context.log_level,
        // 调到 LOG_CRIT(<LOG_ERR) 即可隐藏 ERROR 及以下级别, 不影响功能。
        g_log_context.log_level = LOG_CRIT;
    }

    static void TearDownTestSuite()
    {
        FdfsClient::Destroy();
    }
};

// ============================================================
// 上传 =======================================================
// ============================================================

// 上传本地文件, 返回非空 file_id 且可下载回原文
TEST_F(FdfsTest, UploadFromFile)
{
    std::string payload = MakePayload(64 * 1024, 1);
    std::string src = MakeTempPath("upload_src");
    {
        std::ofstream ofs(src, std::ios::binary);
        ASSERT_TRUE(ofs.is_open()) << "无法创建本地源文件: " << src;
        ofs.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    }

    auto file_id = FdfsClient::UploadFromFile(src);
    fs::remove(src);
    ASSERT_TRUE(file_id.has_value()) << "UploadFromFile 失败";
    EXPECT_FALSE(file_id->empty());
    EXPECT_NE(file_id->find('/'), std::string::npos)
        << "file_id 应包含存储组名, 实际: " << *file_id;

    std::string downloaded;
    ASSERT_TRUE(FdfsClient::DownloadToBuffer(*file_id, downloaded));
    EXPECT_EQ(downloaded, payload);
}

// 上传缓冲区, 返回非空 file_id 且内容可完整读回
TEST_F(FdfsTest, UploadFromBuffer)
{
    std::string payload = MakePayload(128 * 1024, 2);
    auto file_id = FdfsClient::UploadFromBuffer(payload);
    ASSERT_TRUE(file_id.has_value()) << "UploadFromBuffer 失败";
    EXPECT_FALSE(file_id->empty());

    std::string downloaded;
    ASSERT_TRUE(FdfsClient::DownloadToBuffer(*file_id, downloaded));
    EXPECT_EQ(downloaded, payload);
}

// ============================================================
// 下载 =======================================================
// ============================================================

// 上传后同时验证两种下载方式: 下载到缓冲区 and 下载到文件
TEST_F(FdfsTest, DownloadToBufferAndFile)
{
    std::string payload = MakePayload(8 * 1024, 3);
    auto file_id = FdfsClient::UploadFromBuffer(payload);
    ASSERT_TRUE(file_id.has_value());

    // 下载到缓冲区
    std::string buf;
    ASSERT_TRUE(FdfsClient::DownloadToBuffer(*file_id, buf));
    EXPECT_EQ(buf, payload);

    // 下载到文件
    std::string dst = MakeTempPath("download_file");
    ASSERT_TRUE(FdfsClient::DownloadToFile(*file_id, dst));
    EXPECT_EQ(ReadBytes(dst), payload);
    fs::remove(dst);
}

// 下载已删除/不存在的文件应失败
TEST_F(FdfsTest, DownloadNonexistent)
{
    std::string buf;
    EXPECT_FALSE(FdfsClient::DownloadToBuffer("group1/M00/00/00/not_exist_xxxx", buf));
    EXPECT_FALSE(FdfsClient::DownloadToFile("group1/M00/00/00/not_exist_xxxx",
                                            MakeTempPath("download_nonexistent")));
}

// ============================================================
// 删除 =======================================================
// ============================================================

// 删除之后再次读取, 文件应不存在
TEST_F(FdfsTest, DeleteThenGone)
{
    std::string payload = MakePayload(1024, 4);
    auto file_id = FdfsClient::UploadFromBuffer(payload);
    ASSERT_TRUE(file_id.has_value());

    EXPECT_TRUE(FdfsClient::DeleteFile(*file_id)) << "首次删除应成功";

    // 删除后下载应失败(文件不存在)
    std::string buf;
    EXPECT_FALSE(FdfsClient::DownloadToBuffer(*file_id, buf))
        << "删除后下载应失败";
    EXPECT_FALSE(FdfsClient::DownloadToFile(*file_id, MakeTempPath("delete_gone")));

    // 重复删除应失败
    EXPECT_FALSE(FdfsClient::DeleteFile(*file_id)) << "重复删除应失败";
}

// ============================================================
// 多线程 =====================================================
// ============================================================

// 多个线程同时上传/下载各自独立的内容, 校验内容完整且互不串扰
TEST_F(FdfsTest, ConcurrentUploadDownload)
{
    constexpr int kThreads = 8;
    constexpr int kRound = 3;
    constexpr size_t kSize = 32 * 1024;

    std::atomic<int> success{0};
    std::atomic<bool> any_fail{false};
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t, &success, &any_fail]() {
            std::string payload = MakePayload(kSize, 100 + t);
            auto file_id = FdfsClient::UploadFromBuffer(payload);
            if (!file_id.has_value()) {
                any_fail = true;
                return;
            }
            // 同一线程反复下载同一文件, 校验内容一致
            for (int r = 0; r < kRound; ++r) {
                std::string buf;
                if (!FdfsClient::DownloadToBuffer(*file_id, buf) || buf != payload) {
                    any_fail = true;
                    return;
                }
            }
            if (!FdfsClient::DeleteFile(*file_id)) {
                any_fail = true;
                return;
            }
            ++success;
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    EXPECT_TRUE(!any_fail) << "并发上传/下载出现失败";
    EXPECT_EQ(success, kThreads) << "并非所有线程均成功完成上传/下载/删除";
}

// 多线程同时上传文件到服务端, 校验全部成功且下载内容正确
TEST_F(FdfsTest, ConcurrentUploadFile)
{
    constexpr int kThreads = 6;
    constexpr size_t kSize = 16 * 1024;

    std::atomic<int> success{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t, &success]() {
            std::string payload = MakePayload(kSize, 200 + t);
            std::string src = MakeTempPath("concurrent_src");
            {
                std::ofstream ofs(src, std::ios::binary);
                ofs.write(payload.data(), static_cast<std::streamsize>(payload.size()));
            }
            auto file_id = FdfsClient::UploadFromFile(src);
            fs::remove(src);
            if (!file_id.has_value()) {
                return;
            }
            std::string buf;
            if (FdfsClient::DownloadToBuffer(*file_id, buf) && buf == payload
                && FdfsClient::DeleteFile(*file_id)) {
                ++success;
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    EXPECT_EQ(success, kThreads);
}