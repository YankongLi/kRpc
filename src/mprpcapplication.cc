#include "mprpcapplication.h"
#include "mprpcconfig.h"
#include <unistd.h>
// main return
/*
    0 OK
    1 ArgsError
    2 otherError
*/

MprpcConfig MprpcApplication::m_config;

void ShowArgsHelp()
{
    std::cout << "./exename -i xxx.conf" << std::endl;
    exit(1);
}

void MprpcApplication::Init(int argc,char ** argv)
{
    // ./exe -i xxx.conf
    if(argc < 2)
    {
        ShowArgsHelp();
    }
        int c = 0;
    std::string config_file;
    while((c = getopt(argc, argv, "i:")) != -1)
    {
        switch (c)
        {
        case 'i':
            config_file = optarg;
            break;
        case '?':
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        case ':':
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }

    // 开始加载配置文件了 rpcserver_ip=  rpcserver_port   zookeeper_ip=  zookepper_port=
    m_config.LoadConfigFile(config_file.c_str());
    
}
MprpcApplication& MprpcApplication::GetInstance()
{
    static MprpcApplication mpa;
    return mpa;
}

MprpcConfig& MprpcApplication::GetConfig()
{
    return m_config;
}