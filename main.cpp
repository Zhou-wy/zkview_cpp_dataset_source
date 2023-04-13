/*
 * @description:
 * @version:
 * @Author: zwy
 * @Date: 2023-04-12 16:59:30
 * @LastEditors: zwy
 * @LastEditTime: 2023-04-13 14:18:26
 */
#include <fstream>
#include "websocketServer/zk_webSocket.hpp"
#include "common/ilogger.hpp"
#include "common/json.hpp"

// 全局变量
std::string ip;
int port;
std::string endpoint;
iLogger::LogLevel logLevel;
std::string logSaveDir;

/**
 * @brief :导入配置文件
 *
 * @param : system config file path, 默认在：worlspace/conf/system.json
 * @return true
 * @return false
 */
bool loadSystemConf(const std::string &file_path)
{
    // 打开 JSON 文件
    std::ifstream configFile(file_path);
    if (!configFile.is_open())
    {
        INFOE("Failed to open config file: %s ", file_path.c_str());
        return false;
    }

    // 读取 JSON 数据
    Json::Value root;
    Json::CharReaderBuilder builder;
    JSONCPP_STRING errors;
    if (!parseFromStream(builder, configFile, &root, &errors))
    {
        INFOE("Failed to parse JSON data: %s", errors.c_str());
        return false;
    }

    // 获取数据
    ip = root["ip"].asString();
    port = root["port"].asInt();
    endpoint = root["endpoint"].asString();
    int logLevel_int = root["logLevel"].asInt();
    logSaveDir = root["logSaveDir"].asString();

    switch (logLevel_int)
    {
    case 0:
        logLevel = iLogger::LogLevel::Fatal;
        break;
    case 1:
        logLevel = iLogger::LogLevel::Error;
        break;
    case 2:
        logLevel = iLogger::LogLevel::Warning;
        break;
    case 3:
        logLevel = iLogger::LogLevel::Info;
        break;
    case 4:
        logLevel = iLogger::LogLevel::Verbose;
        break;
    case 5:
        logLevel = iLogger::LogLevel::Debug;
        break;
    default:
        return "unknow";
    }
    return true;
}

int main(int argc, char const *argv[])
{
    if (!loadSystemConf("../workspace/conf/system.json"))
    {
        return -1;
    }
    
    INFO("weServer start-> ip: %s, port: %d,  endpoint: %s", ip.c_str(), port, endpoint.c_str());
    INFO("Log start-> level: %s, save dir: %s", iLogger::level_string(logLevel), logSaveDir.c_str());

    iLogger::set_log_level(logLevel);
    iLogger::set_logger_save_directory(logSaveDir);

    zwy::zkWebSocketServer server(ip, port);

    server.startUp(endpoint);

    INFO("WebSocket server is running.");

    std::thread heart_th([&server]()
                         { server.sendHeartbeat(); });

    if (heart_th.joinable())
        heart_th.join();
    return 0;
}
