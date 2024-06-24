#include <iostream>
#include "wara_ps_client.h"
#include <unistd.h>

int main() {
    WaraPSClient client("test", "mqtt://localhost:25565");
    std::cout << "Client created" << std::endl;
    std::thread client_thread = client.Start();

    auto f = [&](const nlohmann::json &_) {
        client.PublishMessage("exec/response", std::string("AAAAAAAAAAA"));
    };

    client.SetCommandCallback("scream", f);

    client_thread.join();

    return 0;
}