#include "general.h"
#include "network.h"

void send_func()
{
    while (true)
    {
        std::cin.get();

        sNetwork->SendDatagram(1, 1, "aa");
    }
}

int main(int argc, char** argv)
{
    sNetwork->Init();

    send_func();

    std::cin.get();

    return 0;
}
