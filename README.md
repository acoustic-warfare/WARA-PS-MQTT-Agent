# WARA-PS-MQTT-Agent

MQTT Bridge designed to work with ljudkriget beamforming and WARA PS MQTT API for control and sensor output.

## Requirements

The following libraries are needed:

- PahoMqttCpp
- nlohmann/json
- boost/uuid

## Installing the library

To install the library, install the needed dependencies and run:

```bash
mkdir build
cd build
cmake ..
sudo make install
```

## Usage

Create a client object with a given name and MQTT broker address and call the `start()` member function to run the
client, this will occupy the main thread until aborted.

Example:

```cpp
#include <iostream>
#include <WaraPSClient.h>
#include <unistd.h>

int main() {
    WaraPSClient client("test", "mqtt://localhost:25565");
    std::cout << "Client created" << std::endl;
    std::thread client_thread = client.Start();

    auto f = [&](const nlohmann::json &_) {
        client_.publish_message("exec/response", std::string("AAAAAAAAAAA"));
    };

    client.SetCommandCallback("scream", f);
    
    json infoMessage = {
        {"type", "info"},
        {"message", "Hello, world!"}
    };
    
    client.publishMessage("info", infoMessage.dump());
    
    json otherInfo;
     
    auto infoCallback = [&](const json &message) {
        otherInfo = message;
    };
    
    client.SetMessageCallBack("info", infoCallback);
    
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    client.Stop();

    return 0;
}
```

## Tasks

The use of "tasks" in the WARA PS API is not very well-defined and this library needs some specifics regarding how to
use them. The current implementation is a placeholder and will be updated when more information is available.