// CalService 服务端 : 使用 ServerFactory 创建 server 对外提供 add 服务

#include <iostream>
#include <string>

#include <brpc/server.h>

#include "cfl_rpc.h"
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
    int port = 8080;
    if (argc >= 2) port = std::stoi(argv[1]);

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

    std::cout << "[server] CalService is running on 0.0.0.0:" << port
              << " (Ctrl-C to quit)" << std::endl;
    server->RunUntilAskedToQuit();
    std::cout << "[server] stopped." << std::endl;
    return 0;
}
