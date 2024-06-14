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

void mqtt_client::start()
{
    client.start_consuming();
    is_running = std::make_shared<bool>(true);
    heartbeat_thread = std::thread([this]()
                                   {
        while (*is_running)
        {
            client.publish("heartbeat", "ba bump", QOS_AT_LEAST_ONCE, false);
            sleep(1);
        } });
    while (*is_running)
    {
        auto msg = client.consume_message();
        if (!msg) // Something has gone very wrong
        {
            this->stop();
            break;
        }

        bool stopping = handleMessage(msg);
        if (stopping)
        {
            this->stop();
            break;
        }

        std::cout << "Received message: " << msg->to_string() << std::endl;
    }
}

bool mqtt_client::handleMessage(mqtt::const_message_ptr msg)
{
    if (msg->get_topic() == "commands")
    {
        std::string payload = msg->to_string();
        if (payload == "stop")
        {
            return true;
        }
    }
    return false;
}

bool mqtt_client::running() const
{
    return *is_running;
}

void mqtt_client::stop()
{
    *is_running = false;
    std::cout << "Shutting down" << std::endl;
    heartbeat_thread.join();
    client.stop_consuming();
    client.disconnect()->wait();
}

mqtt_client::mqtt_client(std::string name, std::string server_address)
    : UNIT_NAME(name), SERVER_ADDRESS(server_address), client(SERVER_ADDRESS, generate_agent_uuid())
{
    std::cout << "Creating client and connecting to server" << std::endl;
    bool connected = client.connect()->wait_until(std::chrono::steady_clock::now() + std::chrono::seconds(5));
    if (!connected)
    {
        throw std::runtime_error("Failed to connect to MQTT server");
    }

    client.subscribe("commands", QOS_AT_LEAST_ONCE)->wait();
}