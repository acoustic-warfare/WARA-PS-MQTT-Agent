# MQTT-Communication
MQTT Bridge designed to work with ljudkriget beamforming and WARA PS MQTT API for control and sensor output.

## Requirements
The following libraries are needed:
- PahoMqttCpp
- nlohmann/json
- boost/uuid

## Running the client
Simply `make run` in the root directory will build and run the client.

## Usage
Create a client object with a given name and MQTT broker adress and call the `start()` member function to run the client, this will occupy the main thread until aborted.

Example:
```cpp
#include <iostream>
#include "mqtt_client.hpp"

int main()
{
    mqtt_client client("test", "mqtt://localhost:25565");
    std::cout << "Client created" << std::endl;
    client.start();

    return 0;
}
```