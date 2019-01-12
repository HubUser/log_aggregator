#include "ConnectionProcessor.hpp"

#include <iostream>

void on_connected(ConnectionPtr connection, const boost::system::error_code& error) {
    if (error) {
        std::cout << "Error: " << error.message() << std::endl;
        return;
    }
    std::cout << connection->get_data() << std::endl;
}

int main(int argc, char* argv[]) {
    ConnectionProcessor::register_message_handler(64000, on_connected);
    ConnectionProcessor::start();
}
