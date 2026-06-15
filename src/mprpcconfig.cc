#include "mprpcconfig.h"
#include <fstream>
#include <iostream>


void MprpcConfig::Trim(std::string& s)
{
    //去除前后置空格
    int first_it = s.find_first_not_of(' ');
    int last_it = s.find_last_not_of(' ');
    if(first_it == std::string::npos || last_it == std::string::npos)return;
    s = s.substr(first_it,last_it - first_it + 1);
}

// 负责解析加载配置文件
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    std::ifstream conf_file_stream(config_file);
    if (!conf_file_stream.is_open()) {
        std::cout << "Error: Failed to open config file at " << config_file << std::endl;
        exit(2);
    }
    std::string line;
    while(std::getline(conf_file_stream,line))
    {
        auto pos = line.find('#');
        if(pos != std::string::npos)
        {
            line = line.substr(0,pos);
        }
        Trim(line);
        if(!line.size())continue;
        pos = line.find('=');

        if(pos == std::string::npos)
        {
            std::cout << "error config line str: " << line << std::endl;
            exit(2);
        }
        std::string line1 = line.substr(0,pos);
        std::string line2 = line.substr(pos + 1,line.size() - pos);
        Trim(line1);
        Trim(line2);
        if(m_configMap.count(line1))
        {
            std::cout << "error config line not one str: " << line << std::endl;
            exit(2);
        }
        m_configMap[line1] = line2;
    }

}
// 查询配置项信息
std::string MprpcConfig::Load(const std::string &key)
{
    if(m_configMap.count(key) == 0)
    {
        std::cout << "config not find str: " << key << std::endl;
        return "";
    }
    return m_configMap[key];
}