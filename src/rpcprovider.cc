#include <functional>
#include <google/protobuf/stubs/callback.h>
#include <iostream>
#include <muduo/net/TcpConnection.h>
#include <string>

#include "rpcprovider.h"
#include "logger.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"

void RpcProvider::NotifyService(fixbug::UserServiceRpc::Service* service)
{
    ServiceInfo serverInfo;
    //注册Rpc请求
    const google::protobuf::ServiceDescriptor *  pserviceDesc = service->GetDescriptor();
    // 获取服务的名字
    //std::string service_name = pserviceDesc->name();
    std::string service_name(pserviceDesc->name());
    // 获取服务对象service的方法的数量
    int methodCnt = pserviceDesc->method_count();
    for(int i = 0;i < methodCnt;i ++)
    {
        const google::protobuf::MethodDescriptor * it = pserviceDesc->method(i);
        std::string method_name(it->name());
        serverInfo.m_methodMap.insert({method_name,it});
        LOG_INFO("method_name:%s", method_name.c_str());
    }
    serverInfo.m_service = service;
    m_serviceMap.insert({service_name,serverInfo});
}
void RpcProvider::Run()
{
    // 读取配置文件rpcserver的信息
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);

    // 创建TcpServer对象
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");

    //绑定回调函数
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage,this,
        std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection,this,std::placeholders::_1));

    server.setThreadNum(2);

    // rpc服务部署完毕

    // 配置zookeeper

    // 启动muduo网络库
    server.start();
    m_eventLoop.loop(); 

}

// 新的socket连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& tcpConn)
{
    if(!tcpConn->connected())
    {
        tcpConn->shutdown();
    }
}

/*
header_size(4个字节) + header_str + args_str
*/
// 已建立连接用户的读写事件回调
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& tcpConn, muduo::net::Buffer* buffer, muduo::Timestamp times)
{
    //读取到客户端发来的请求
    std::string request_str = buffer->retrieveAllAsString();
    int header_size = 0;
    request_str.copy((char*)&header_size, 4,0);
    // 根据header_size读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = request_str.substr(4, header_size);
    mprpc::RpcHeader rpcheader;
    if(!rpcheader.ParseFromString(rpc_header_str))
    {
        //反序列化失败
        std::cout << "rpc_header_str:" << rpc_header_str << " parse error!" << std::endl;
        return;
    }
    std::string service_name = rpcheader.service_name();
    std::string method_name = rpcheader.method_name();
    int args_size = rpcheader.args_size();
    std::string args_str = request_str.substr(4 + header_size,args_size);

    // 打印调试信息
    std::cout << "============================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl; 
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl; 
    std::cout << "service_name: " << service_name << std::endl; 
    std::cout << "method_name: " << method_name << std::endl; 
    std::cout << "args_str: " << args_str << std::endl; 
    std::cout << "============================================" << std::endl;

    // 获取service对象和method对象
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end())
    {
        std::cout << service_name << " is not exist!" << std::endl;
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end())
    {
        std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
        return;
    }
    google::protobuf::Service *service = it->second.m_service; 
    const google::protobuf::MethodDescriptor *method = mit->second;
    

    // 生成rpc方法调用的请求request和响应response参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str))
    {
        std::cout << "request parse error, content:" << args_str << std::endl;
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();
    google::protobuf::Closure* done = google::protobuf::NewCallback<RpcProvider, 
                                                                    const muduo::net::TcpConnectionPtr&, 
                                                                    google::protobuf::Message*>
                                                                    (this, 
                                                                    &RpcProvider::SendRpcResponse, 
                                                                    tcpConn, response);

    service->CallMethod(method,nullptr,request,response,done);

}
// Closure的回调操作，用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& tcpConn, google::protobuf::Message* response)
{
    std::string response_str;
    if (response->SerializeToString(&response_str)) // response进行序列化
    {
        // 序列化成功后，通过网络把rpc方法执行的结果发送会rpc的调用方
        tcpConn->send(response_str);
    }
    else
    {
        std::cout << "serialize response_str error!" << std::endl; 
    }
    tcpConn->shutdown(); // 模拟http的短链接服务，由rpcprovider主动断开连接
}