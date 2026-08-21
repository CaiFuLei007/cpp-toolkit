// CalService 客户端 :
//   1. 用 SvcWatcher(注册中心) 从 etcd 发现服务地址并持续监控上下线
//   2. 用 ChannelManager 缓存已发现的服务, 并从注册中心获取 channel
//   3. 用 ClosureFactory 把 lambda 包成 closure 发起异步 rpc, 打印结果

#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <thread>

#include <brpc/channel.h>
#include <brpc/controller.h>

#include "rpc.h"
#include "etcd.h"
#include "cal.pb.h"

int main(int argc, char* argv[])
{
    std::string etcd_addr = "http://127.0.0.1:2379"; // 注册中心地址
    int a = 10, b = 32;
    if (argc >= 2) etcd_addr = argv[1];
    if (argc >= 4) { a = std::stoi(argv[2]); b = std::stoi(argv[3]); }

    // (1) 注册中心 : 缓存从 etcd 发现的服务地址
    cpp_toolkit::ChannelManager::Ptr registry =
        std::make_shared<cpp_toolkit::ChannelManager>();

    // 用 SvcWatcher 监听 etcd: 服务上线/下线时自动增删 channel
    cpp_toolkit::SvcWatcher::Ptr watcher = std::make_shared<cpp_toolkit::SvcWatcher>(
        etcd_addr,
        [registry](const std::string& key, const std::string& value) {
            registry->AddService(key, value);
            std::cout << "[client] service online : " << key
                      << " -> " << value << std::endl;
        },
        [registry](const std::string& key, const std::string& value) {
            registry->DelService(key, value);
            std::cout << "[client] service offline: " << key
                      << " -> " << value << std::endl;
        });
    if (!watcher->Watch())
    {
        std::cerr << "[client] connect to registry failed, etcd = "
                  << etcd_addr << std::endl;
        return -1;
    }
    std::cout << "[client] watching registry " << etcd_addr << std::endl;

    // (2) 从注册中心获取 channel (等一会儿服务, 最多 10s)
    cpp_toolkit::ChannelPtr channel;
    for (int i = 0; i < 100; ++i)
    {
        channel = registry->GetChannel("CalService");
        if (channel) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!channel)
    {
        std::cerr << "[client] no available channel for CalService" << std::endl;
        return -1;
    }

    cal::AddRequest request;
    request.set_a(a);
    request.set_b(b);

    cal::AddResponse response;
    brpc::Controller controller;
    cal::CalService::Stub stub(channel.get());

    // 用 promise 等待异步回调完成
    auto done_promise = std::make_shared<std::promise<void>>();
    auto done_future = done_promise->get_future();

    // (3) ClosureFactory : 用 lambda 定义 done 回调
    google::protobuf::Closure* done = cpp_toolkit::ClosureFactory::CreateClosure(
        [&controller, &request, &response, done_promise]() {
            if (controller.Failed())
            {
                std::cerr << "[client] rpc failed: "
                          << controller.ErrorText() << std::endl;
            }
            else
            {
                std::cout << "[client] Add(" << request.a() << ", "
                          << request.b() << ") = "
                          << response.result() << std::endl;
            }
            done_promise->set_value();
        });

    // 异步发起请求 (done != nullptr), 等回调通知
    stub.Add(&controller, &request, &response, done);
    done_future.wait();

    return controller.Failed() ? -1 : 0;
}
