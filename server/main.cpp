#include "ConnectionProcessor.hpp"

#include <iostream>

class MessageHandler {
    std::stringstream ss;
public:
    void on_connected(ConnectionPtr connection, const boost::system::error_code& error) {
        if (error) {
            std::cout << "Error: " << error.message() << std::endl;
            ss = std::stringstream();
            return;
        }
        async_read_data(std::move(connection), [this] (ConnectionPtr connection, const boost::system::error_code& error) {
                on_read(std::move(connection), error);
            }, 1024);
    }

    void on_read(ConnectionPtr connection, const boost::system::error_code& error) {
        if (error) {
            if (error == boost::asio::error::eof) {
                ss << connection->get_data();
                std::cout << ss.str() << std::endl;
                ss = std::stringstream();
            } else {
                std::cout << "Error: " << error.message() << std::endl;
                ss = std::stringstream();
                return;
            }
        }
        ss << connection->get_data();
    }
};

int main(int argc, char* argv[]) {
    MessageHandler mh;
    ConnectionProcessor::register_message_handler(64000, [&mh] (ConnectionPtr connection, const boost::system::error_code& error) {
        mh.on_connected(std::move(connection), error);
    });
    ConnectionProcessor::start();
}
