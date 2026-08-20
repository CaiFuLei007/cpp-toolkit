// CalService 客户端 :
//   1. 用 ChannelManager(注册中心) 登记服务端地址
//   2. 从注册中心获取 channel
//   3. 用 ClosureFactory 把 lambda 包成 closure 发起异步 rpc, 打印结果

#include <future>
#include <iostream>
#include <string>

#include <brpc/channel.h>
#include <brpc/controller.h>

#include "cfl_rpc.h"
#include "cal.pb.h"

int main(int argc, char* argv[])
{
    std::string addr = "127.0.0.1:8080";
    int a = 10, b = 32;
    if (argc >= 2) addr = argv[1];
    if (argc >= 4) { a = std::stoi(argv[2]); b = std::stoi(argv[3]); }

    // (1) 注册中心 : 客户端登记服务端提供的服务地址
    cpp_toolkit::ChannelManager::Ptr registry =
        std::make_shared<cpp_toolkit::ChannelManager>();
    registry->AddService("CalService", addr);
    std::cout << "[client] registered CalService -> " << addr << std::endl;

    // (2) 从注册中心取出一个 channel (内部按 index 轮询负载均衡)
    cpp_toolkit::ChannelPtr channel = registry->GetChannel("CalService");
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
