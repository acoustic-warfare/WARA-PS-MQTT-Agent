#include <iostream>
#include "mqtt_client.h"
#include <unistd.h>

int main()
{
    mqtt_client client("test", "mqtt://localhost:25565");
    std::cout << "Client created" << std::endl;
    client.start();

    return 0;
}