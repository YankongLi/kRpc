# kRpc

`kRpc` 是一个基于 C++17 实现的轻量级 RPC 框架，使用 Protobuf 描述服务接口和序列化请求数据，使用 Muduo 作为服务端网络通信层，并提供配置解析、RPC Channel、服务发布、请求分发和异步日志模块。

> This repository was migrated from Gitee: <https://gitee.com/liyongkui1/krpc>

## 项目特点

- **Protobuf 服务描述**：通过 `.proto` 文件定义 RPC 服务、请求和响应结构。
- **自定义 RPC 协议**：请求格式采用 `header_size + rpc_header + args`，解决 TCP 字节流中的消息边界问题。
- **服务发布机制**：Provider 端注册本地服务对象，将服务名、方法名和 Protobuf MethodDescriptor 建立映射。
- **远程调用封装**：Caller 端通过继承 `google::protobuf::RpcChannel` 的 `MprpcChannel` 发起远程调用。
- **Muduo 网络层**：服务端基于 Muduo `TcpServer` 处理连接、读写事件和请求分发。
- **配置中心**：通过 `-i xxx.conf` 传入配置文件，读取 RPC 服务 IP、端口等参数。
- **异步日志**：使用线程安全队列和后台线程实现异步日志写入。

## 架构设计

```text
Caller Process
  |
  |  UserServiceRpc_Stub
  v
MprpcChannel
  |
  |  header_size + RpcHeader + serialized args
  v
TCP Socket
  |
  v
Muduo TcpServer
  |
  v
RpcProvider
  |
  |  service_name + method_name
  v
Local Service Method
```

## 请求协议

RPC 请求在网络上传输时组织为：

```text
4 bytes header_size + serialized RpcHeader + serialized request args
```

其中 `RpcHeader` 定义在 `src/rpcheader.proto`：

```proto
message RpcHeader
{
    bytes service_name = 1;
    bytes method_name = 2;
    uint32 args_size = 3;
}
```

Provider 收到请求后：

1. 读取前 4 字节得到 `header_size`。
2. 反序列化 `RpcHeader`，获得服务名、方法名和参数长度。
3. 根据服务名和方法名查找本地服务对象与方法描述符。
4. 反序列化业务请求参数。
5. 调用本地业务方法。
6. 将响应序列化后写回调用方。

## 目录结构

```text
.
├── CMakeLists.txt
├── src
│   ├── include
│   │   ├── logger.h
│   │   ├── lockqueue.h
│   │   ├── mprpcapplication.h
│   │   ├── mprpcchannel.h
│   │   ├── mprpcconfig.h
│   │   ├── mprpccontroller.h
│   │   ├── rpcprovider.h
│   │   └── zookeeperutil.h
│   ├── logger.cc
│   ├── mprpcapplication.cc
│   ├── mprpcchannel.cc
│   ├── mprpcconfig.cc
│   ├── mprpccontroller.cc
│   ├── rpcprovider.cc
│   ├── rpcheader.proto
│   └── zookeeperutil.cc
└── example
    ├── user.proto
    ├── callee
    │   └── userservice.cc
    └── caller
        └── calluserservice.cc
```

## 核心模块

| Module | Description |
| --- | --- |
| `RpcProvider` | RPC 服务发布端，负责注册服务、启动 Muduo 服务器、解析请求并调用本地方法 |
| `MprpcChannel` | RPC 调用端通道，封装请求序列化、协议拼接、网络发送和响应解析 |
| `MprpcApplication` | 框架初始化入口，解析命令行参数并加载配置 |
| `MprpcConfig` | 配置文件读取模块，支持 `key=value` 格式 |
| `Logger` | 异步日志模块，基于阻塞队列和后台写日志线程 |
| `LockQueue` | 线程安全队列，用于日志缓冲 |
| `RpcHeader` | RPC 请求头，包含服务名、方法名和参数长度 |

## 环境依赖

该项目面向 Linux 环境，主要依赖：

- C++17
- CMake 3.0+
- Protobuf
- Muduo
- pthread

如果启用 ZooKeeper 相关能力，还需要：

- ZooKeeper C API

> 当前仓库中的 ZooKeeper 封装代码处于注释状态，README 仅按现有代码说明当前框架能力。

## 构建方式

```bash
mkdir -p build
cd build
cmake ..
make
```

构建后默认输出：

```text
bin/      executable files
lib/      generated static library
```

## 使用示例

示例服务定义在 `example/user.proto`：

```proto
service UserServiceRpc
{
    rpc Login(LoginRequest) returns(LoginResponse);
    rpc Register(RegisterRequest) returns(RegisterResponse);
}
```

Provider 端通过继承生成的 Protobuf 服务类实现业务方法：

```cpp
class UserService : public fixbug::UserServiceRpc
{
public:
    void Login(::google::protobuf::RpcController* controller,
               const ::fixbug::LoginRequest* request,
               ::fixbug::LoginResponse* response,
               ::google::protobuf::Closure* done) override;
};
```

然后注册并启动服务：

```cpp
MprpcApplication::Init(argc, argv);

RpcProvider provider;
provider.NotifyService(new UserService());
provider.Run();
```

Caller 端通过 Stub 发起远程调用：

```cpp
MprpcApplication::Init(argc, argv);

fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
fixbug::LoginRequest request;
fixbug::LoginResponse response;

request.set_name("zhang san");
request.set_pwd("123456");

stub.Login(nullptr, &request, &response, nullptr);
```

## 配置文件格式

框架通过 `-i` 参数指定配置文件：

```bash
./provider -i test.conf
./consumer -i test.conf
```

配置文件采用 `key=value` 格式，例如：

```text
rpcserverip=127.0.0.1
rpcserverport=8000
server_ip=127.0.0.1
server_port=8000
zookeeperip=127.0.0.1
zookeeperport=2181
```

## 项目价值

这个项目主要用于实践：

- RPC 框架基本原理
- Protobuf 服务定义与序列化
- TCP 自定义协议设计
- Muduo Reactor 网络模型
- 服务发布与方法分发
- 异步日志系统
- CMake 多模块构建
- Linux 网络编程

## 后续优化方向

- 修复并完善客户端通道中的异常处理和请求头序列化逻辑。
- 补全 ZooKeeper 服务注册与发现流程。
- 增加连接池、超时控制和重试机制。
- 增加更完整的 RPC Controller 错误语义。
- 补充单元测试和集成测试。
- 增加 Docker Compose 示例环境。

