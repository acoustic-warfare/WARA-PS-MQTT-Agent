#ifndef waraps_client_H
#define waraps_client_H

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>

constexpr std::chrono::milliseconds DEFAULT_HEARTBEAT_INTERVAL{1'000};

/**
 * A class to handle MQTT communication with the WARA PS MQTT broker.
 * Built to WARA PS message specifications as a L2 unit.
 * For more info see https://api.docs.waraps.org/
 *
 * Author: Janne Schyffert
 */
class WaraPSClient {
protected:
    class Callback final : public virtual mqtt::callback {
        WaraPSClient &client_;

        void message_arrived(mqtt::const_message_ptr msg) override;

    public:
        explicit Callback(WaraPSClient &client) : client_(client) {}
    };

    Callback callbackHandler_{*this};

    static constexpr std::chrono::milliseconds heartbeat_interval = DEFAULT_HEARTBEAT_INTERVAL;
    static constexpr std::string_view DEFAULT_PREFIX = "waraps/unit/ground/real/";

    const std::string kUUID = GenerateUUID();
    const std::string kUnitName, kServerAddress;
    std::string topic_Prefix;

    static std::string GenerateUUID();

    std::vector<nlohmann::json> availableTasks_;

    std::vector<nlohmann::json> executingTasks_;

    std::string GenerateFullTopic(std::string_view topic) const;

    std::string GenerateHeartBeatMessage() const;

    std::string GenerateTaskMessage();

    void HandleMessage(const mqtt::const_message_ptr &msg);

    void CmdPong(nlohmann::json msg_payload);

    void CmdStartTask(nlohmann::json msg_payload);

    // Commands get a separate function and map to allow different command callbacks as well as user-defined
    // command callbacks
    void HandleCommand(nlohmann::json msg_payload);

    std::thread heartbeat_thread_, publish_thread_;
    std::vector<std::thread> data_threads;

    std::queue<nlohmann::json> message_queue_;

    mqtt::async_client client_;
    mqtt::connect_options conn_opts_;

    std::shared_ptr<bool> is_running_ = std::make_shared<bool>(false);
    std::map<std::string, std::function<void(WaraPSClient *, nlohmann::json)>> message_callbacks_{
            {"exec/command", &WaraPSClient::HandleCommand}};
    std::map<std::string, std::function<void(WaraPSClient *, nlohmann::json)>> command_callbacks_{
            {"start-task", &WaraPSClient::CmdStartTask}, {"ping", &WaraPSClient::CmdPong}};
    std::map<std::string, std::function<void(WaraPSClient *, nlohmann::json)>> task_callbacks_;


public:
    WaraPSClient(std::string_view name, std::string_view server_address);

    WaraPSClient(std::string_view name, std::string_view server_address, std::string_view prefix);

    WaraPSClient(std::string_view name, std::string_view server_address, std::string_view username,
                 std::string_view password);

    WaraPSClient(std::string_view name, std::string_view server_address, std::string_view username,
                 std::string_view password, std::string_view prefix);

    /**
     * Deleted constructors to disallow copying and assignment of the client
     */
    WaraPSClient(const WaraPSClient &) = delete;

    WaraPSClient &operator=(const WaraPSClient &) = delete;
    ~WaraPSClient();

    /**
     * Check if the client is running and consuming messages
     * @return true if the client is running, false otherwise
     */
    bool running() const;

    /**
     * Start main thread and heartbeat thread and begin consuming messages from the MQTT server.
     * Will not block the current thread.
     */
    void Start();

    /**
     * Stop consuming messages and disable the heartbeat thread, allowing the client to be destroyed.
     */
    void Stop();

    /**
     * Publish a message to the MQTT server asynchronously.
     * @param topic The topic to publish the message to, will be added to WARA PS topic prefix
     * @param payload The message to publish as a JSON string
     */
    void PublishMessage(std::string_view topic, const std::string &payload);

    /**
     * Publishes a message "raw" to the MQTT server, skipping all the WARA PS Prefixes usually required, use with
     * caution.
     * @param topic The full topic to publish the message on
     * @param payload The message topic to publish as a JSON string
     */
    void PublishMessageNoPrefix(std::string_view topic, const std::string &payload);

    /**
     * Set an asynchronous callback function to be called when a message is received on the specified topic.
     * Disallows setting the "command" topic callback, see set_command_callback for that.
     * @param topic the topic to listen on, will be added to WARA PS topic prefix.
     * @param callback the function to call when a message is received on the specified topic, takes waraps_client as a
     * parameter to allow for state changes if needed.
     * @throws std::invalid_argument if the topic is "exec/command"
     */
    void SetMessageCallback(const std::string &topic, std::function<void(WaraPSClient *, nlohmann::json)> callback);

    /**
     * Set an asynchronous callback function to be called when a message is received on the specified topic.
     * Disallows setting the "command" topic callback, see set_command_callback for that.
     * @param topic the topic to listen on, will be added to WARA PS topic prefix.
     * @param callback the function to call when a message is received on the specified topic
     * @throws std::invalid_argument if the topic is "exec/command"
     */
    void SetMessageCallback(const std::string &topic, const std::function<void(nlohmann::json)> &callback);

    /**
     * Set an asynchronous callback function to be called when a command is received on the /exec/command topic
     * @param command The type of command to call the callback with
     * @param callback The callback to use
     * @throws std::invalid_argument if the reserved commands "ping" or "stop" are given
     */
    void SetCommandCallback(const std::string &command, const std::function<void(nlohmann::json)> &callback);

    /**
     * Create a task that can be executed via the "start-task" command, the task json
     * will be shown as "direct execution info" in the WARA PS-server, allowing users to start tasks via the GUI.
     * @param taskJson A json containing the following fields:
     *                          name: the name of the task
     *                          params: (optional) array of possible parameters to the task, such as a bearing
     * @param callback The callback to be called when the task is started, takes a json containing start parameters as
     * argument
     */
    void CreateTask(nlohmann::json taskJson, const std::function<void(nlohmann::json)> &callback);

    void SubscribeToTopic(std::string_view topic);
};
#endif
