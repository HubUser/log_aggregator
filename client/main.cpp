#include "ConnectionProcessor.hpp"

#include <iostream>
#include <iterator>
#include <algorithm>
#include <thread>

void on_write_complete(ConnectionPtr connection, const boost::system::error_code& error) {
    //ConnectionProcessor::stop();
}

void send_to_server(const std::string& msg) {
    auto connection = ConnectionProcessor::create_connection("127.0.0.1", 64000);
    connection->set_data(msg);
    async_write_data(std::move(connection), &on_write_complete);
}

int main(int argc, char* argv[]) {
    std::thread t(&ConnectionProcessor::start);
    std::for_each(std::istream_iterator<std::string>(std::cin), 
            std::istream_iterator<std::string>(), &send_to_server);
    ConnectionProcessor::stop();
    if (t.joinable()) {
        t.join();
    }
}
