#include "boost/system/error_code.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/write.hpp"

#include "boost/optional.hpp"

#include <functional>

class ConnectionWithData {
	boost::asio::ip::tcp::socket socket;
	std::string data;
public:
	explicit ConnectionWithData(boost::asio::io_service& ios)
		: socket(ios) { }

    ConnectionWithData(boost::asio::io_service& ios, const std::string& addr, unsigned short port_num)
        : socket(ios) {
        socket.connect(boost::asio::ip::tcp::endpoint(
            boost::asio::ip::address_v4::from_string(addr), 
            port_num
        ));
    }
	~ConnectionWithData() {
		shutdown();
	}
	void shutdown() {
		if (!socket.is_open()) {
			return;
		}
		boost::system::error_code ignore;
		socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
		socket.close();
	}
    ConnectionWithData(const ConnectionWithData&) = delete;
    ConnectionWithData(ConnectionWithData&&) = delete;
    void set_data(const std::string& new_data) {
        data = new_data;
        }
    const std::string& get_data() const {
        return data;
    }
    boost::asio::ip::tcp::socket& get_socket() {
        return socket;
    }
};
using ConnectionPtr = std::unique_ptr<ConnectionWithData>;

template <typename T>
class TaskWithConnection {
	ConnectionPtr connection;
	T task;
public:
	explicit TaskWithConnection(ConnectionPtr c, const T& f)
		: connection(std::move(c))
		, task(f) { }

	void operator() (const boost::system::error_code& error, std::size_t bytes_count) {
		//connection->data.resize(bytes_count);
		task(std::move(connection), error);
	}
};
template<typename Functor>
void async_write_data(ConnectionPtr connection, Functor&& f) {
    boost::asio::async_write(connection->get_socket(), boost::asio::buffer(connection->get_data()), TaskWithConnection(std::move(connection), f));
}
class ConnectionProcessor {
public:
	using on_accept_func_t = std::function<void(ConnectionPtr, const boost::system::error_code&)>;

    static boost::asio::io_service& get_ios() {
        static boost::asio::io_service ios;
        static boost::asio::io_service::work work(ios);

        return ios;
    }
    static ConnectionPtr create_connection(const std::string& addr, unsigned short port_num) {
        return std::make_unique<ConnectionWithData>(get_ios(), addr, port_num);
    }
	template<typename Functor>
	static void register_message_handler(unsigned short port_num, const Functor& f) {
		start_accepting_connection(std::make_unique<tcp_listener>(get_ios(), port_num, f));
	}
	static void start() {
        get_ios().run();
	}
	static void stop() {
        get_ios().stop();
	}
private:
	class tcp_listener {
		boost::asio::ip::tcp::acceptor acceptor;
		const on_accept_func_t func;
		ConnectionPtr connection;
	public:
		using prepare_result_t = boost::optional<std::pair<boost::asio::ip::tcp::socket&, boost::asio::ip::tcp::acceptor&>>;
		template <typename Functor>
		tcp_listener(boost::asio::io_service& io_service, unsigned short port_num, const Functor& task)
			: acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_num))
			, func(task) { }

		template<typename ConnectionType>
		prepare_result_t prepare_acceptor() {
			if (!acceptor.is_open()) {
				return boost::none;
			}
			connection = std::make_unique<ConnectionType>(acceptor.get_io_service());
            return std::make_pair(std::ref(connection->get_socket()), std::ref(acceptor));
		}
        ConnectionPtr get_connection() {
            return std::move(connection);
        }
        on_accept_func_t get_func() {
            return func;
        }
	};
	using listener_ptr = std::unique_ptr<tcp_listener>;
	class handle_accept {
		listener_ptr listener;
	public:
		explicit handle_accept(listener_ptr l)
			: listener(std::move(l)) { }

		void operator() (const boost::system::error_code& error) {
			TaskWithConnection<on_accept_func_t> task(listener->get_connection(), listener->get_func());
            start_accepting_connection(std::move(listener));
            task(error, 0);
		}
	};
	static void start_accepting_connection(listener_ptr listener) {
		auto result = listener->prepare_acceptor<ConnectionWithData>();
		if (!result) {
			return;
		}
		auto& [socket, acceptor] = result.get();
		acceptor.async_accept(socket, handle_accept(std::move(listener)));
	}
};
