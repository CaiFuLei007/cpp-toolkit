
#include "fdfs.h"
#include <fastcommon/logger.h>
#include <fastdfs/storage_client1.h>
#include <sstream>

namespace cpp_toolkit {

bool FdfsClient::Init(const FdfsSettings& settings)
{
    g_log_context.log_level = LOG_ERR;
    log_init();
    std::ostringstream oss;
    for(const auto& server : settings.tracker_servers_)
    {
        oss << "tracker_server=" << server << "\n";
    }
    oss << "connect_timeout=" << settings.connect_timeout << "\n";
    oss << "network_timeout=" << settings.network_timeout << "\n";  
    oss << "use_connection_pool=" << settings.use_connection_pool << "\n";
    oss << "connection_pool_max_idle_time=" << settings.connection_pool_max_idle_time << "\n";

    int ret = fdfs_client_init_from_buffer(oss.str().c_str());
    if(ret != 0)
    {
        return false;
    }
    return true;
}

void FdfsClient::Destroy()
{
    fdfs_client_destroy();
    log_destroy();
}

std::optional<std::string> FdfsClient::UploadFromFile(const std::string& file_path)
{
    auto tracker_server = tracker_get_connection();
    if(tracker_server == nullptr)
    {
        return nullptr;
    }
    char file_id[128] = {0};
    int ret = storage_upload_by_filename1(tracker_server , nullptr , 0 , file_path.c_str() , nullptr , nullptr , 0 , nullptr, file_id);
    if(ret != 0)
    {
        tracker_close_connection(tracker_server);
        return nullptr;
    }
    tracker_close_connection(tracker_server);
    return std::string(file_id);
}
std::optional<std::string> FdfsClient::UploadFromBuffer(const std::string & buffer)
{
    auto tracker_server = tracker_get_connection();
    if(tracker_server == nullptr)
    {
        return nullptr;
    }
    char file_id[128] = {0};
    int ret = storage_upload_by_filebuff1(tracker_server , nullptr , 0 , buffer.c_str(), buffer.size(), nullptr, nullptr , 0, nullptr, file_id);
    if(ret != 0)
    {
        tracker_close_connection(tracker_server);
        return nullptr;
    }
    tracker_close_connection(tracker_server);
    return std::string(file_id);
}
bool FdfsClient::DownloadToFile(const std::string& file_id, const std::string& file_path)
{
    auto tracker_server = tracker_get_connection();
    if(tracker_server == nullptr)
    {
        return false;
    }
    int64_t file_size = 0;
    int ret = storage_download_file_to_file1(tracker_server, nullptr , file_id.c_str() , file_path.c_str() , &file_size); 
    if(ret != 0)
    {
        tracker_close_connection(tracker_server);
        return false;
    }
    tracker_close_connection(tracker_server);
    return true;
}
bool FdfsClient::DownloadToBuffer(const std::string& file_id, std::string& out_buffer)
{
    auto tracker_server = tracker_get_connection();
    if(tracker_server == nullptr)
    {
        return false;
    }
    int64_t buffer_size = 0;
    char* buffer = nullptr;
    int ret = storage_do_download_file1_ex(tracker_server , nullptr , FDFS_DOWNLOAD_TO_BUFF, file_id.c_str() , 0 , 0 , &buffer, nullptr, &buffer_size);
    if(ret != 0)
    {
        free(buffer);
        tracker_close_connection(tracker_server);
        return false;
    }
    tracker_close_connection(tracker_server);
    if(buffer)
    {
        out_buffer.assign(buffer, buffer_size);
        free(buffer);
    }
    return true;
}
bool FdfsClient::DeleteFile(const std::string& file_id)
{
    auto tracker_server = tracker_get_connection();
    if(tracker_server == nullptr)
    {
        return false;
    }
    int ret = storage_delete_file1(tracker_server , nullptr , file_id.c_str());
    if(ret != 0)
    {        
        tracker_close_connection(tracker_server);
        return false;
    }
    tracker_close_connection(tracker_server);
    return true;
}

} // namespace cpp_toolkit
