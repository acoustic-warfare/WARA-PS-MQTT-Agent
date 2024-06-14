#pragma once
#include <mqtt/async_client.h>
#include <string>

#define DEFAULT_HEARTBEAT_INTERVAL 1000

class mqtt_client
{
private:
    const std::chrono::milliseconds heartbeat_interval =
        std::chrono::milliseconds(DEFAULT_HEARTBEAT_INTERVAL);
    const std::string UNIT_NAME, SERVER_ADDRESS;
    std::string generate_agent_uuid() const;
    std::thread heartbeat_thread;

    mqtt::async_client client;
    std::shared_ptr<bool> is_running = std::make_shared<bool>(false);

public:
    mqtt_client(std::string name, std::string server_address);
    ~mqtt_client();
    bool running() const;
    void start();
    void stop();
};