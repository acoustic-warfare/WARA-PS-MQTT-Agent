#include "waraps_client.h"

#include <thread>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <mqtt/async_client.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <utility>

#define QOS_AT_MOST_ONCE 0
#define QOS_AT_LEAST_ONCE 1
#define QOS_EXACTLY_ONCE 2

using json = nlohmann::json;

const bool RETAIN = false;
const std::string TOPIC_PREFIX = "waraps/unit/ground/real/ljudkriget/";

std::string waraps_client::generate_uuid()
{
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return boost::uuids::to_string(uuid);
}

std::string waraps_client::generate_full_topic(const std::string& topic)
{
    return TOPIC_PREFIX + topic;
}

std::string waraps_client::generate_heartbeat_message() const
{
    json j = {
        {"agent-type", "surface"},
        {"agent-uuid", uuid},
        {"levels", {"sensor", "direct execution"}},
        {"name", "ljudkriget"},
        {"rate", heartbeat_interval.count() / 1000},
        {"stamp", (double)std::chrono::system_clock::now().time_since_epoch().count() / 1000.0},
        {"type", "HeartBeat"}};

    return j.dump(4); // Pretty print with 4 spaces indentation
}

void waraps_client::start()
{
    client.start_consuming();
    is_running = std::make_shared<bool>(true);
    heartbeat_thread = std::thread([this]()
                                   {
        std::string heartbeat_topic = generate_full_topic("heartbeat");
        while (*is_running)
        {
            std::string heartbeat_message = generate_heartbeat_message();
            client.publish(heartbeat_topic, heartbeat_message, QOS_AT_LEAST_ONCE, RETAIN);
            sleep(1);
        } });

    consume_thread = std::thread([this]()
                                 {
        while (*is_running)
        {
            auto msg = client.consume_message();
            if (!msg) // Something has gone very wrong
            {
                std::cerr << "Failed to consume message: " << msg <<  std::endl;
                this->stop();
                break;
            }

            handle_message(msg);

            std::cout << "Received message: " << msg->to_string() << std::endl;
        } });
}

bool waraps_client::handle_message(const mqtt::const_message_ptr& msg)
{
    json msg_payload = json::parse(msg->to_string());

    if (msg->get_topic() == generate_full_topic("exec/command"))
    {
        handle_command(msg_payload);
    }
    else
    {
        message_callbacks[msg->get_topic()](this, msg_payload);
    }
    return false;
}

void waraps_client::cmd_stop(nlohmann::json msg_payload)
{
}

void waraps_client::handle_command(nlohmann::json msg_payload)
{
    command_callbacks[msg_payload["command"]](this, msg_payload);
}

void waraps_client::cmd_pong(json msg_payload)
{
    json response = {
        {"agent-uuid", uuid},
        {"com-uuid", generate_uuid()},
        {"response", "pong"},
        {"response-to", msg_payload["com-uuid"]}};
    std::string response_topic = generate_full_topic("exec/response");
    client.publish(response_topic, response.dump(4), QOS_AT_LEAST_ONCE, RETAIN);
}

bool waraps_client::publish_message_async(const std::string& topic, const std::string& payload)
{
    std::string full_topic = generate_full_topic(topic);
    mqtt::delivery_token_ptr token = client.publish(full_topic, payload, QOS_AT_LEAST_ONCE, RETAIN);
    return token->get_message_id() != -1;
}

void waraps_client::set_message_callback(const std::string& topic, std::function<void(waraps_client *, nlohmann::json)> callback)
{
    if (topic == "exec/command")
    {
        throw std::invalid_argument("Cannot set callback for command topic, use set_command_callback instead");
    }
    message_callbacks[topic] = std::move(callback);
}

bool waraps_client::running() const
{
    return *is_running;
}

void waraps_client::stop()
{
    std::cout << "Shutting down" << std::endl;
    *is_running = false;
    heartbeat_thread.join();
    client.stop_consuming();
    client.disconnect()->wait();
}

// ctors and dtors

waraps_client::waraps_client(std::string name, std::string server_address)
    : UNIT_NAME(std::move(name)), SERVER_ADDRESS(std::move(server_address)), client(SERVER_ADDRESS, uuid)
{
    std::cout << "Creating client and connecting to server" << std::endl;
    bool connected = client.connect()->wait_for(std::chrono::seconds(5));
    if (!connected)
    {
        throw std::runtime_error("Failed to connect to MQTT server");
    }

    std::cout << "Connected to server" << std::endl;

    client.subscribe(generate_full_topic("exec/command"), QOS_AT_LEAST_ONCE)->wait();
}

waraps_client::~waraps_client()
{
    if (running())
    {
        stop();
    }
}