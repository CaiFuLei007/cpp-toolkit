
#pragma once

/*
    fdfs 封装
    提供文件/缓冲区上传、下载、删除等功能
*/

#include <string>
#include <vector>
#include <optional>
#include <fastcommon/logger.h>
#include <fastdfs/fdfs_client.h>


namespace cpp_toolkit {

struct FdfsSettings
{
    std::vector<std::string> tracker_servers_;
    int connect_timeout = 30;
    int network_timeout = 30;
    bool use_connection_pool = true;
    int connection_pool_max_idle_time = 3600;
};

class FdfsClient
{
public:
    static bool Init(const FdfsSettings& settings);
    static void Destroy();
    static std::optional<std::string> UploadFromFile(const std::string& file_path);
    static std::optional<std::string> UploadFromBuffer(const std::string & buffer);
    static bool DownloadToFile(const std::string& file_id , const std::string& file_path);
    static bool DownloadToBuffer(const std::string& file_id , std::string& buffer);
    static bool DeleteFile(const std::string& file_id);
};


} // namespace cpp_toolkit
