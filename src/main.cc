#include <iostream>
#include "mqtt_client.cc"
#include <unistd.h>

int main()
{
    mqtt_client client("test", "mqtt://test.mosquitto.org:1883");
    std::cout << "Client created" << std::endl;
    client.start();

    return 0;
}