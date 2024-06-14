#include "mqtt_client.h"

#define QOS_AT_MOST_ONCE 0
#define QOS_AT_LEAST_ONCE 1
#define QOS_EXACTLY_ONCE 2

#include <thread>
#include <unistd.h>

std::string mqtt_client::generate_agent_uuid() const
{
    return "0";
}

mqtt_client::mqtt_client(std::string name, std::string server_address)
    : UNIT_NAME(name), SERVER_ADDRESS(server_address)
{
    std::cout << "Creating client and connecting to server" << std::endl;
    client = std::make_shared<mqtt::async_client>(SERVER_ADDRESS, generate_agent_uuid());
    bool connected = client->connect()->wait_until(std::chrono::steady_clock::now() + std::chrono::seconds(5));
    if (!connected)
    {
        throw std::runtime_error("Failed to connect to MQTT server");
    }

    std::cout << "Connected to server" << std::endl;

    this->is_running = std::make_shared<bool>(true);

    this->heartbeat_thread = std::thread([this]()
                                         {
        while (*this->is_running)
        {
            (*client).publish("heartbeat", "ba bump", 1, false);
            sleep(1);
        } });
}

mqtt_client::~mqtt_client()
{
    *is_running = false;
    heartbeat_thread.join();
}