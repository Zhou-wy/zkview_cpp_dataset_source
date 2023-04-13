/*
 * @description:
 * @version:
 * @Author: zwy
 * @Date: 2023-04-03 12:44:00
 * @LastEditors: zwy
 * @LastEditTime: 2023-04-13 14:20:27
 */

#include <thread>
#include <chrono>
#include <fstream>
#include "zk_webSocket.hpp"
#include "../common/ilogger.hpp"

namespace zwy
{
    bool zkWebSocketServer::loadTestDataFormJsonFile(const std::string &file_path)
    {
        // 读取JSON文件
        std::ifstream file(file_path);
        if (!file.is_open())
        {
            INFOE("Failed to open data file: %s ", file_path.c_str());
            return false;
        }

        // 解析JSON文件
        Json::Value root;
        file >> root;
        file.close();

        // 判断"data"和"nameMap"节点是否长度一样
        if (root.isMember("data") && root.isMember("nameMap"))
        {
            const Json::Value &data = root["data"];
            const Json::Value &nameMap = root["nameMap"];
            if (data.size() == nameMap.size())
            {
                this->m_data = data;
                this->m_nameMap = nameMap;
                INFO("Successfully import data from %s.", file_path.c_str());
            }
            else
            {
                INFOE("The length of 'data' and 'nameMap' nodes are different.");
                return false;
            }
        }
        else
        {
            INFOE("JSON file is invalid.");
            return false;
        }

        return true;
    }

    zkWebSocketServer::zkWebSocketServer(std::string ip, unsigned short port) : m_ip(ip), m_port(port), m_is_running(false), m_ws_time(std::chrono::high_resolution_clock::now())
    {
    }

    zkWebSocketServer ::~zkWebSocketServer(){};

    void zkWebSocketServer::startUp(const std::string &end_point)
    {
        if (this->m_is_running)
        {
            INFOE("WebSocket server is already running.");
            return;
        }
        this->m_server.config.port = m_port;
        this->m_server.config.address = m_ip;
        auto &echo = this->m_server.endpoint["^/" + end_point + "$"];

        echo.on_message = [this](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::InMessage> in_message)
        {
            std::string input_str = in_message->string();
            INFOD("recv: %s", input_str.c_str());
            Json::CharReaderBuilder builder;
            JSONCPP_STRING errs;
            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            bool success = reader->parse(input_str.c_str(), input_str.c_str() + input_str.size(), &m_recv_message, &errs);
            if (!success)
            {
                INFOE("Failed to parse JSON string: ");
                return 1;
            }
            if (this->m_recv_message["name"] == "HEART")
            {
                this->m_ws_time = std::chrono::system_clock::now();
                Json::Value message;
                message["name"] = "HEART";
                std::string response = Json::FastWriter().write(message);
                this->sendMessage(response);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            else if (this->m_recv_message["name"] == "ORIGINAL")
            {
                if (this->loadTestDataFormJsonFile("../workspace/conf/datasource.json"))
                {
                    this->sendOriginal(this->m_data, this->m_nameMap);
                }
            }
            else if (this->m_recv_message["name"] == "REGISTER")
            {
                INFOD("收到用户注册测点");
            }
            else if (this->m_recv_message["name"] == "USER")
            {
                INFOD("收到用户发送数据");
            }
        };

        this->m_server_thread = std::thread([this]()
                                            { m_server.start(); });

        this->m_is_running = true;
    }

    std::chrono::high_resolution_clock::time_point zkWebSocketServer::getWsTime()
    {
        return this->m_ws_time;
    }

    void zkWebSocketServer::sendOriginal(Json::Value &data, Json::Value &nameMap)
    {
        Json::Value root_data, root_nameMap;
        root_data["name"] = "ORIGINALDATA";
        root_data["data"] = data;

        root_nameMap["name"] = "NAMEMAP";
        root_nameMap["data"] = nameMap;

        Json::StreamWriterBuilder data_builder;
        std::string data_str = Json::writeString(data_builder, root_data);

        Json::StreamWriterBuilder nameMap_builder;
        std::string nameMap_str = Json::writeString(nameMap_builder, root_nameMap);

        INFOD("sendOriginal data: %s", data_str.c_str());
        INFOD("sendOriginal nameMap: %s", nameMap_str.c_str());
        this->sendMessage(data_str);
        this->sendMessage(nameMap_str);
    }

    void zkWebSocketServer::stop()
    {
        if (!this->m_is_running)
        {
            INFOE("WebSocket server is not running.");
            return;
        }
        this->m_server.stop();
        this->m_server_thread.join();
        this->m_is_running = false;
    }

    void zkWebSocketServer::setIP(const std::string &ip)
    {
        this->m_ip = ip;
    }

    void zkWebSocketServer::setPort(unsigned short port)
    {
        this->m_port = port;
    }

    void zkWebSocketServer::sendMessage(const std::string &message)
    {
        for (auto &connection : this->m_server.get_connections())
        {
            connection->send(message);
        }
    }

    Json::Value zkWebSocketServer::receiveMessage()
    {
        return this->m_recv_message;
    }

    void zkWebSocketServer::sendHeartbeat()
    {
        while (this->wsIsRunning())
        {
            auto now = std::chrono::system_clock::now();
            auto lastHeartbeatTime = std::chrono::time_point_cast<std::chrono::milliseconds>(this->getWsTime());
            auto currentTime = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
            auto elapsedTime = currentTime - lastHeartbeatTime;
            // 以下为间隔15S
            if (elapsedTime.count() > (15 * 1000))
            {
                INFOW("connection timed out");
                this->m_is_running = false;
                this->stop();
            }
        }
    }
}
