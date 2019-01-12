#include "ConnectionProcessor.hpp"

#include <iostream>

void on_write_complete(ConnectionPtr connection, const boost::system::error_code& error) {
    std::cout << "Done sending. Quitting." << std::endl;
    ConnectionProcessor::stop();
}

int main(int argc, char* argv[]) {
    auto connection = ConnectionProcessor::create_connection("127.0.0.1", 64000);
    connection->set_data("some_message");
    async_write_data(std::move(connection), &on_write_complete);
    ConnectionProcessor::start();
}
