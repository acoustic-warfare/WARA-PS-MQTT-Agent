#include "wara_ps_client.h"

#include <thread>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <mqtt/async_client.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <utility>

using namespace std::chrono;
using namespace std::string_view_literals;

constexpr bool QOS_AT_LEAST_ONCE{true};

using json = nlohmann::json;

constexpr bool kRetain = false;

std::string WaraPSClient::GenerateUUID() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return boost::uuids::to_string(uuid);
}

std::string WaraPSClient::GenerateFullTopic(std::string_view topic) {
    std::string str;
    str.reserve(kTopic_Prefix.length() + topic.length());
    str += kTopic_Prefix;
    str += topic;
    return str;
}

std::string WaraPSClient::GenerateHeartBeatMessage() const {
    json j = {
            {"agent-type", "surface"},
            {"agent-uuid", kUUID},
            {"levels",     {"sensor", "direct execution"}},
            {"name",       kUnitName},
            {"rate",       duration_cast<milliseconds>(heartbeat_interval).count()},
            {"stamp",      duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count()},
            {"type",       "HeartBeat"}};

    return j.dump(4); // Pretty print with 4 spaces indentation
}

void WaraPSClient::Start() {
    std::cout << "Creating client and connecting to server" << std::endl;
    try {
        bool connected = client_.connect(conn_opts_)->wait_for(5s);
        if (!connected) {
            throw mqtt::exception(-1);
        }
    } catch (mqtt::exception &e) {
        *is_running_ = false;
        std::cerr << "Failed to connect to server" << std::endl;
        throw std::runtime_error("Failed to connect to MQTT server");
    }

    std::cout << "Connected to server" << std::endl;

    client_.subscribe(GenerateFullTopic("exec/command"), QOS_AT_LEAST_ONCE)->wait();
    client_.start_consuming();
    client_.set_callback(callbackHandler_);
    is_running_ = std::make_shared<bool>(true);
    heartbeat_thread_ = std::thread([this]() {
        std::string heartbeat_topic = GenerateFullTopic("heartbeat");
        while (*is_running_) {
            std::string heartbeat_message = GenerateHeartBeatMessage();
            client_.publish(heartbeat_topic, heartbeat_message, QOS_AT_LEAST_ONCE, kRetain);
            std::this_thread::sleep_for(milliseconds(DEFAULT_HEARTBEAT_INTERVAL));
        }
    });
}

void WaraPSClient::HandleMessage(const mqtt::const_message_ptr &msg) {
    json msg_payload = json::parse(msg->to_string());

    if (msg->get_topic() == GenerateFullTopic("exec/command")) {
        HandleCommand(msg_payload);
    } else {
        if (!message_callbacks.contains(msg->get_topic())) {
            std::cout << "No callback set for topic: " << msg->get_topic() << std::endl;
            return;
        }
        message_callbacks[msg->get_topic()](this, msg_payload);
    }
}

void WaraPSClient::CmdStop(nlohmann::json msg_payload) {
    json response = {
            {"agent-uuid",  kUUID},
            {"com-uuid",    GenerateUUID()},
            {"response",    "stopped"},
            {"response-to", msg_payload["com-uuid"]}};
    std::string response_topic = GenerateFullTopic("exec/response");
    client_.publish(response_topic, response.dump(4), QOS_AT_LEAST_ONCE, kRetain);
    *is_running_ = false;
}

void WaraPSClient::HandleCommand(nlohmann::json msg_payload) {
    if (!command_callbacks.contains(msg_payload["command"])) {
        std::cout << "Bad command received: " << msg_payload["command"] << std::endl;
        return;
    }
    command_callbacks[msg_payload["command"]](this, msg_payload);
}

void WaraPSClient::CmdPong(nlohmann::json msg_payload) {
    json response = {
            {"agent-uuid",  kUUID},
            {"com-uuid",    GenerateUUID()},
            {"response",    "pong"},
            {"response-to", msg_payload["com-uuid"]}};
    std::string response_topic = GenerateFullTopic("exec/response");
    client_.publish(response_topic, response.dump(4), QOS_AT_LEAST_ONCE, kRetain);
}

void WaraPSClient::PublishMessage(std::string_view topic, const std::string &payload) {
    std::string full_topic = GenerateFullTopic(topic);
    mqtt::delivery_token_ptr token = client_.publish(full_topic, payload, QOS_AT_LEAST_ONCE, kRetain);
}

void WaraPSClient::SetMessageCallback(const std::string &topic,
                                      std::function<void(WaraPSClient *, nlohmann::json)> callback) {
    if (topic == "exec/command") {
        throw std::invalid_argument("Cannot set callback for ''command'' topic, use SetCommandCallback instead");
    }
    message_callbacks[topic] = std::move(callback);
}

bool WaraPSClient::running() const {
    return *is_running_;
}

void WaraPSClient::Stop() {
    std::cout << "Shutting down" << std::endl;
    *is_running_ = false;
    heartbeat_thread_.join();
    client_.stop_consuming();
    client_.disconnect()->wait();
}

WaraPSClient::WaraPSClient(std::string name, std::string server_address)
        : kUnitName(std::move(name)), kServerAddress(std::move(server_address)),
          kTopic_Prefix("waraps/unit/ground/real/" + kUnitName + "/"),
          client_(kServerAddress, kUUID) {
    conn_opts_ = mqtt::connect_options_builder()
            .automatic_reconnect(true)
            .finalize();
}

WaraPSClient::~WaraPSClient() {
    if (running()) {
        Stop();
    }
}

void
WaraPSClient::SetCommandCallback(const std::string &command, const std::function<void(nlohmann::json)> &callback) {
    if (command == "stop" || command == "ping") {
        throw std::invalid_argument("Cannot set callback for reserved command: " + command);
    }

    command_callbacks[command] = [callback](WaraPSClient *, nlohmann::json msg_payload) {
        callback(std::move(msg_payload));
    };
}

void WaraPSClient::SetMessageCallback(const std::string &topic, const std::function<void(nlohmann::json)> &callback) {
    SetMessageCallback(topic, [callback](WaraPSClient *, nlohmann::json msg_payload) {
        callback(std::move(msg_payload));
    });
}

WaraPSClient::WaraPSClient(std::string name, std::string server_address, const std::string &username,
                           const std::string &password)
        : WaraPSClient(std::move(name), std::move(server_address)) {
    auto sslOptions = mqtt::ssl_options_builder()
            .enable_server_cert_auth(false)
            .verify(false)
            .finalize();

    conn_opts_ = mqtt::connect_options_builder()
            .password(password)
            .user_name(username)
            .ssl(sslOptions)
            .automatic_reconnect(true)
            .finalize();
}

void WaraPSClient::Callback::message_arrived(mqtt::const_message_ptr msg) {
    try {
        client_.HandleMessage(msg);
    } catch (std::exception &e) {
        std::cerr << "Failed to handle message: " << e.what()
                  << "\nMessage:\n" << msg->to_string() << std::endl;
    }
}
