// CalService 服务端 :
//   1. 用 ServerFactory 创建 server 对外提供 add 服务
//   2. 用 SvcProvider 把服务注册到注册中心 (etcd), 客户端可从注册中心发现此服务

#include <iostream>
#include <string>

#include <brpc/server.h>

#include "rpc.h"
#include "etcd.h"
#include "cal.pb.h"

// 业务实现 : 继承 protoc 生成的 CalService 基类
class CalServiceImpl : public cal::CalService
{
public:
    void Add(google::protobuf::RpcController* controller,
            const cal::AddRequest* request,
            cal::AddResponse* response,
            google::protobuf::Closure* done) override
    {
        // ClosureGuard 保证 done 必然被执行 (函数 return 时自动 Run)
        brpc::ClosureGuard done_guard(done);
        response->set_result(request->a() + request->b());
        std::cout << "[server] Add(" << request->a() << ", "
                  << request->b() << ") = " << response->result() << std::endl;
    }
};

int main(int argc, char* argv[])
{
    std::string etcd_addr = "http://127.0.0.1:2379"; // 注册中心地址
    int port = 8080;
    if (argc >= 2) port = std::stoi(argv[1]);
    if (argc >= 3) etcd_addr = argv[2];

    // 业务对象必须比 server 活得久 (ServerFactory 使用 SERVER_DOESNT_OWN_SERVICE)
    CalServiceImpl service;

    // 通过封装的 ServerFactory 创建并启动 brpc server
    std::shared_ptr<brpc::Server> server =
        cpp_toolkit::ServerFactory::CreateServer(port, &service);
    if (!server)
    {
        std::cerr << "[server] start failed on port " << port << std::endl;
        return -1;
    }

    // 必须等 server 启动成功后再注册, 保证客户端发现服务时立即可用
    std::string service_addr = "127.0.0.1:" + std::to_string(port);

    // 用 SvcProvider 把服务注册到 etcd 注册中心 (自带保活, provider 须比 server 活得久)
    cpp_toolkit::SvcProvider::Ptr provider = std::make_shared<cpp_toolkit::SvcProvider>(
        etcd_addr, "CalService", service_addr, "rpc-server-1");
    if (!provider->Registry(10))   // TTL = 10s, KeepAlive 自动续期
    {
        std::cerr << "[server] register CalService to registry failed, etcd = "
                  << etcd_addr << std::endl;
        return -1;
    }

    std::cout << "[server] CalService is running on 0.0.0.0:" << port
              << " and registered to " << etcd_addr
              << " (Ctrl-C to quit)" << std::endl;
    server->RunUntilAskedToQuit();
    std::cout << "[server] stopped." << std::endl;
    return 0;
}
