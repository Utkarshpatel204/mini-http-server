#include "server.h"

int main()
{
    const int kPort = 8080;
    const int kMaxConcurrentClients = 64;
    return run_server(kPort, kMaxConcurrentClients);
}
