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

Create a client object with a given name and MQTT broker adress and call the `start()` member function to run the
client, this will occupy the main thread until aborted.

Example:

```cpp
#include <iostream>
#include <WaraPSClient.h>
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
```
