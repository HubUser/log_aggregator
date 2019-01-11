#include "boost/system/error_code.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/asio/ip/tcp.hpp"

#include "boost/optional.hpp"

#include <functional>
#include <iostream>

class ConnectionWithData {
	boost::asio::ip::tcp::socket socket;
public:
	std::string data;
	explicit ConnectionWithData(boost::asio::io_service& ios)
		: socket(ios) { }

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
		connection->data.resize(bytes_count);
		task(std::move(connection), error);
	}
};
class ConnectionProcessor {
public:
	using on_accept_func_t = std::function<void(ConnectionPtr, const boost::system::error_code&)>;

	template<typename Functor>
	void register_message_handler(unsigned short port_num, const Functor& f) {
		start_accepting_connection(std::make_unique<tcp_listener>(ios, port_num, f));
	}
	void start() {
	}
	void stop() {
	}
private:
	class tcp_listener {
		boost::asio::ip::tcp::acceptor acceptor;
		const on_accept_func_t func;
		ConnectionPtr connection;
	public:
		using prepare_result_t = boost::optional<std::pair<boost::asio::ip::tcp::socket, boost::asio::ip::tcp::acceptor>>;
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
			return std::make_pair(&connection->socket, &acceptor);
		}
	};
	using listener_ptr = std::unique_ptr<tcp_listener>;
	class handle_accept {
		listener_ptr listener;
	public:
		explicit handle_accept(listener_ptr l)
			: listener(std::move(l)) { }

		void operator() (const boost::system::error_code& error) {
			TaskWithConnection<on_accept_func_t> task();
		}
	};
	void start_accepting_connection(listener_ptr listener) {
		auto result = listener->prepare_acceptor<ConnectionWithData>();
		if (!result) {
			return;
		}
		auto& [socket, acceptor] = result.get();
		acceptor.async_accept(socket, handle_accept(std::move(listener)));
	}
	boost::asio::io_service ios;
};

int main(int argc, char* argv[]) {
	ConnectionProcessor cp;
	cp.register_message_handler(64000, [](ConnectionPtr&& connection, const boost::system::error_code& error) {
		std::cout << connection->data << std::endl;
	});
	cp.start();
}