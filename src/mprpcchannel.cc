#include "mprpcchannel.h"
#include "rpcheader.pb.h"
#include "mprpcapplication.h"

#include <bits/stdint-uintn.h>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                          google::protobuf::Message* response, google::protobuf::Closure* done)
{
    std::string server_name(method->service()->name());
    std::string method_name(method->name());
    std::string args_str;
    if(!request->SerializeToString(&args_str))
    {
        controller->SetFailed("serialize request error!");
        return;
    }
    uint32_t args_size;
    args_size = args_str.size();
    mprpc::RpcHeader rpcheader;
    rpcheader.set_args_size(args_size);
    rpcheader.set_method_name(method_name);
    rpcheader.set_service_name(server_name);
    
    std::string header_str;
    if(!request->SerializeToString(&header_str))
    {
        controller->SetFailed("serialize rpc header error!");
        return;
    }
    uint32_t header_size = header_str.size();
     // 组织待发送的rpc请求的字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size, 4)); // header_size
    send_rpc_str += header_str; // rpcheader
    send_rpc_str += args_str; // args

    // 打印调试信息
    std::cout << "============================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl; 
    std::cout << "rpc_header_str: " << header_str << std::endl; 
    std::cout << "service_name: " << server_name << std::endl; 
    std::cout << "method_name: " << method_name << std::endl; 
    std::cout << "args_str: " << args_str << std::endl; 
    std::cout << "============================================" << std::endl;

    int cfd = socket(AF_INET,SOCK_STERAM,0);
    if(cfd == -1)
    {
        //创建失败
        return;
    }

    auto& config = MprpcApplication::GetInstance().GetConfig();
    //与rpc服务节点通信
    std::string ip = config.Load("server_ip");
    uint16_t port = atoi(config.Load("server_port").c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());


     // 连接rpc服务节点
    if (-1 == connect(cfd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        close(cfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "connect error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 发送rpc请求
    if (-1 == send(cfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        close(cfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "send error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 接收rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(cfd, recv_buf, 1024, 0)))
    {
        close(cfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 反序列化rpc调用的响应数据
    // std::string response_str(recv_buf, 0, recv_size); // bug出现问题，recv_buf中遇到\0后面的数据就存不下来了，导致反序列化失败
    // if (!response->ParseFromString(response_str))
    if (!response->ParseFromArray(recv_buf, recv_size))
    {
        close(cfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "parse error! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }

    close(cfd);
}