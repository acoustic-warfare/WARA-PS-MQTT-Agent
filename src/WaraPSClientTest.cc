#include <iostream>
#include "WaraPSClient.h"
#include <unistd.h>

int main() {
    WaraPSClient client("test", "mqtt://localhost:25565");
    std::cout << "Client created" << std::endl;
    std::thread client_thread = client.start();

    auto f = [&](const nlohmann::json &_) {
        client.publish_message("exec/response", std::string("AAAAAAAAAAA"));
    };

    client.set_command_callback("scream", f);

    client_thread.join();

    return 0;
}