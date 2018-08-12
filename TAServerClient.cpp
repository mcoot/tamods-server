#include "TAServerClient.h"

TAServer::Client g_TAServerClient;

namespace TAServer {
	void TAServer::Client::connectToGameServerLauncher(std::string host, int port) {
		tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);
		socket = std::make_shared<tcp::socket>(ios);

		socket->connect(endpoint);

		boost::array<char, 128> buf;
		std::string message = "Test message";
		std::copy(message.begin(), message.end(), buf.begin());
		boost::system::error_code err;

		socket->write_some(boost::asio::buffer(buf, message.size()), err);
		socket->close();
	}
}

