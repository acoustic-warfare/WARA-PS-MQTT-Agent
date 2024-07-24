#include "wara_ps_client.h"

#include <thread>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>
#include <utility>

using namespace std::chrono;
using namespace std::string_view_literals;

constexpr bool QOS_AT_LEAST_ONCE{true};

using json = nlohmann::json;

constexpr bool kRetain = false;

std::string WaraPSClient::GenerateUUID() {
    const boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return to_string(uuid);
}

std::string WaraPSClient::GenerateFullTopic(const std::string_view topic) const {
    std::string str;
    str.reserve(topic_Prefix.length() + topic.length());
    str += topic_Prefix;
    str += topic;
    return str;
}

std::string WaraPSClient::GenerateHeartBeatMessage() const {
    const json j = {
            {"agent-type", "surface"},
            {"agent-uuid", kUUID},
            {"levels", {"sensor", "direct execution"}},
            {"name", kUnitName},
            {"rate", duration_cast<milliseconds>(heartbeat_interval).count()},
            {"stamp", duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count()},
            {"type", "HeartBeat"}};

    return j.dump(4); // Pretty print with 4 spaces indentation
}
std::string WaraPSClient::GenerateTaskMessage() {
    json j = {
            {"name", kUnitName},
            {"rate", 1.0},
            {"type", "DirectExecutionInfo"},
            {"stamp", system_clock::now().time_since_epoch().count() / 1000},
            {"tasks-available", availableTasks_},
            {"tasks-executing", executingTasks_},
    };

    return j.dump();
}

void WaraPSClient::Start() {
    std::cout << "Creating client and connecting to server" << std::endl;
    try {
        if (!client_.connect(conn_opts_)->wait_for(5s)) {
            throw mqtt::exception(-1);
        }
    } catch (mqtt::exception &) {
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
        const std::string heartbeat_topic = GenerateFullTopic("heartbeat");
        const std::string task_topic = GenerateFullTopic("direct_execution_info");
        while (*is_running_) {
            if (!availableTasks_.empty()) {
                std::string task_message = GenerateTaskMessage();
                client_.publish(task_topic, task_message, QOS_AT_LEAST_ONCE, kRetain);
            }

            std::string heartbeat_message = GenerateHeartBeatMessage();
            client_.publish(heartbeat_topic, heartbeat_message, QOS_AT_LEAST_ONCE, kRetain);
            std::this_thread::sleep_for(milliseconds(DEFAULT_HEARTBEAT_INTERVAL));
        }
    });
}

void WaraPSClient::HandleMessage(const mqtt::const_message_ptr &msg) {
    const json msg_payload = json::parse(msg->to_string());

    if (msg->get_topic() == GenerateFullTopic("exec/command")) {
        HandleCommand(msg_payload);
    } else {
        if (!message_callbacks_.contains(msg->get_topic())) {
            std::cout << "No callback set for topic: " << msg->get_topic() << std::endl;
            return;
        }
        message_callbacks_[msg->get_topic()](this, msg_payload);
    }
}

void WaraPSClient::CmdStartTask(nlohmann::json msg_payload) {
    /*
     * Denna är lite udda
     * WARA-PS API specificerar aldrig riktigt hur en task-execution ser ut så jag spitballar lite kring responsen och i
     * vilken ordning saker ska göras. För att använda "på riktigt" bör task-systemet ses över konkret. Så länge funkar
     * det...?
     */

    json task = msg_payload["task"];
    task["uuid"] = msg_payload["task-uuid"];

    executingTasks_.push_back(task);

    task_callbacks_[task["name"]](this, task);

    const json response = {{"agent-uuid", kUUID},
                           {"com-uuid", GenerateUUID()},
                           {"response", "ok"},
                           {"response-to", msg_payload["com-uuid"]}};
    client_.publish(GenerateFullTopic("exec/response"), response.dump());
}

void WaraPSClient::HandleCommand(nlohmann::json msg_payload) {
    if (!command_callbacks_.contains(msg_payload["command"])) {
        std::cout << "Bad command received: " << msg_payload["command"] << std::endl;
        return;
    }
    command_callbacks_[msg_payload["command"]](this, msg_payload);
}

void WaraPSClient::CmdPong(nlohmann::json msg_payload) {
    const json response = {{"agent-uuid", kUUID},
                           {"com-uuid", GenerateUUID()},
                           {"response", "pong"},
                           {"response-to", msg_payload["com-uuid"]}};
    const std::string response_topic = GenerateFullTopic("exec/response");
    client_.publish(response_topic, response.dump(4), QOS_AT_LEAST_ONCE, kRetain);
}

void WaraPSClient::PublishMessage(const std::string_view topic, const std::string &payload) {
    const std::string full_topic = GenerateFullTopic(topic);
    mqtt::delivery_token_ptr token = client_.publish(full_topic, payload, QOS_AT_LEAST_ONCE, kRetain);
}

void WaraPSClient::PublishMessageNoPrefix(const std::string_view topic, const std::string &payload) {
    client_.publish(topic.data(), payload, QOS_AT_LEAST_ONCE, kRetain);
}

void WaraPSClient::SetMessageCallback(const std::string &topic,
                                      std::function<void(WaraPSClient *, nlohmann::json)> callback) {
    if (topic == "exec/command") {
        throw std::invalid_argument("Cannot set callback for ''command'' topic, use SetCommandCallback instead");
    }
    message_callbacks_[topic] = std::move(callback);
}

WaraPSClient::~WaraPSClient() {
    if (*is_running_) {
        Stop();
    }
}

bool WaraPSClient::running() const { return *is_running_; }

void WaraPSClient::Stop() {
    std::cout << "Shutting down" << std::endl;
    *is_running_ = false;
    heartbeat_thread_.join();
    client_.stop_consuming();
    client_.disconnect()->wait();
}


void WaraPSClient::SetCommandCallback(const std::string &command, const std::function<void(nlohmann::json)> &callback) {
    if (command == "start-task" || command == "ping") {
        throw std::invalid_argument("Cannot set callback for reserved command: " + command);
    }

    command_callbacks_[command] = [callback](WaraPSClient *, nlohmann::json msg_payload) {
        callback(std::move(msg_payload));
    };
}

void WaraPSClient::CreateTask(json taskJson, const std::function<void(nlohmann::json)> &callback) {
    task_callbacks_[taskJson["name"]] = [callback](WaraPSClient *, nlohmann::json msg_payload) {
        callback(std::move(msg_payload));
    };

    availableTasks_.push_back(std::move(taskJson));
}

void WaraPSClient::SetMessageCallback(const std::string &topic, const std::function<void(nlohmann::json)> &callback) {
    client_.subscribe(topic, QOS_AT_LEAST_ONCE)->wait();
    SetMessageCallback(topic,
                       [callback](WaraPSClient *, nlohmann::json msg_payload) { callback(std::move(msg_payload)); });
}

WaraPSClient::WaraPSClient(const std::string_view name, const std::string_view server_address) :
    kUnitName(name), kServerAddress(server_address), topic_Prefix(DEFAULT_PREFIX.data() + kUnitName + "/"),
    client_(kServerAddress, kUUID) {
    topic_Prefix = DEFAULT_PREFIX.data() + kUnitName + "/";
    conn_opts_ = mqtt::connect_options_builder().automatic_reconnect(true).finalize();
}

WaraPSClient::WaraPSClient(const std::string_view name, const std::string_view server_address,
                           const std::string_view prefix) :
    WaraPSClient(std::string(name), std::string(server_address)) {
    topic_Prefix = prefix;
}

WaraPSClient::WaraPSClient(const std::string_view name, const std::string_view server_address,
                           const std::string_view username, const std::string_view password) :
    WaraPSClient(name, server_address) {
    const auto sslOptions = mqtt::ssl_options_builder().enable_server_cert_auth(false).verify(false).finalize();

    conn_opts_ = mqtt::connect_options_builder()
                         .password(password.data())
                         .user_name(username.data())
                         .ssl(sslOptions)
                         .automatic_reconnect(true)
                         .finalize();
}

WaraPSClient::WaraPSClient(const std::string_view name, const std::string_view server_address,
                           const std::string_view username, const std::string_view password,
                           const std::string_view prefix) : WaraPSClient(name, server_address, username, password) {
    topic_Prefix = prefix;
}

void WaraPSClient::Callback::message_arrived(const mqtt::const_message_ptr msg) {
    try {
        client_.HandleMessage(msg);
    } catch (std::exception &e) {
        std::cerr << "Failed to handle message: " << e.what() << "\nMessage:\n" << msg->to_string() << std::endl;
    }
}
