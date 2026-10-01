#include "server.hpp"
#include <iostream>

using namespace std;
using namespace net;
int main() {
    const int PORT = 8086;
    const string doc_root = "./www";

    AdvancedWebServer server(PORT, doc_root);

    if (!server.start()) {
        cerr << "Failed to start server.";
        return 1;
    }

    server.run();
    return 0;
}
