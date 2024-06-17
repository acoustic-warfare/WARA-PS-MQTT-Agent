#include "mqtt_client.hpp"
#include <nlohmann/json.hpp>

#define QOS_AT_MOST_ONCE 0
#define QOS_AT_LEAST_ONCE 1
#define QOS_EXACTLY_ONCE 2

#include <thread>
#include <unistd.h>

using json = nlohmann::json;

const bool RETAIN = false;
const std::string TOPIC_PREFIX = "waraps/unit/ground/real/ljudkriget/";

std::string mqtt_client::generate_agent_uuid()
{
    // TODO: Fixa denna, typ requesta från brokern?
    return "0";
}

std::string mqtt_client::generate_full_topic(std::string topic) const
{
    return TOPIC_PREFIX + topic;
}

std::string mqtt_client::generate_heartbeat_message() const
{
    json j = {
        {"agent-type", "surface"},
        {"agent-uuid", 0},
        {"levels", {"sensor", "direct execution"}},
        {"name", "ljudkriget"},
        {"rate", heartbeat_interval.count() / 1000},
        {"stamp", std::chrono::system_clock::now().time_since_epoch().count()},
        {"type", "heartbeat"}};

    return j.dump(4); // Pretty print with 4 spaces indentation
}

void mqtt_client::start()
{
    client.start_consuming();
    is_running = std::make_shared<bool>(true);
    heartbeat_thread = std::thread([this]()
                                   {
        std::string heartbeat_topic = generate_full_topic("heartbeat");
        std::string heartbeat_message = generate_heartbeat_message();
        while (*is_running)
        {
            client.publish(heartbeat_topic, heartbeat_message, QOS_AT_LEAST_ONCE, RETAIN);
            sleep(1);
        } });
    while (*is_running)
    {
        auto msg = client.consume_message();
        if (!msg) // Something has gone very wrong
        {
            std::cerr << "Failed to consume message" << std::endl;
            this->stop();
            break;
        }

        bool stopping = handle_message(msg);
        if (stopping)
        {
            std::cout << "Stop command recieved" << std::endl;
            this->stop();
            break;
        }

        std::cout << "Received message: " << msg->to_string() << std::endl;
    }
}

bool mqtt_client::handle_message(mqtt::const_message_ptr msg)
{
    if (msg->get_topic() == generate_full_topic("commands"))
    {
        std::string payload = msg->to_string();
        if (payload == "stop")
        {
            return true;
        }
    }
    return false;
}

bool mqtt_client::publish_message_async(std::string topic, std::string payload)
{
    std::string full_topic = generate_full_topic(topic);
    mqtt::delivery_token_ptr token = client.publish(full_topic, payload, QOS_AT_LEAST_ONCE, RETAIN);
    return token->get_message_id() != -1;
}

bool mqtt_client::running() const
{
    return *is_running;
}

void mqtt_client::stop()
{
    std::cout << "Shutting down" << std::endl;
    *is_running = false;
    heartbeat_thread.join();
    client.stop_consuming();
    client.disconnect()->wait();
}

// ctors and dtors

mqtt_client::mqtt_client(std::string name, std::string server_address)
    : UNIT_NAME(name), SERVER_ADDRESS(server_address), client(SERVER_ADDRESS, generate_agent_uuid())
{
    std::cout << "Creating client and connecting to server" << std::endl;
    bool connected = client.connect()->wait_for(std::chrono::seconds(5));
    if (!connected)
    {
        throw std::runtime_error("Failed to connect to MQTT server");
    }

    std::cout << "Connected to server" << std::endl;

    client.subscribe(generate_full_topic("commands"), QOS_AT_LEAST_ONCE)->wait();
}

// no copying, moving or assigning
mqtt_client::mqtt_client(const mqtt_client &other) = delete;
mqtt_client &mqtt_client::operator=(const mqtt_client &other) = delete;
mqtt_client::mqtt_client(mqtt_client &&other) = delete;

mqtt_client::~mqtt_client()
{
    std::cout << "Destroying client" << std::endl;
    if (running())
    {
        stop();
    }
}