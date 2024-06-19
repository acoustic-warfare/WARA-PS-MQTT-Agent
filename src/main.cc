#include <iostream>
#include "waraps_client.h"
#include <unistd.h>

int main()
{
    waraps_client client("test", "mqtt://localhost:25565");
    std::cout << "Client created" << std::endl;
    client.start();

    while(client.running()) {
        sleep(5);
    }

    return 0;
}