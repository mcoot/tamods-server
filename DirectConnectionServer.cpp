#include "DirectConnectionServer.h"

DCServer::Server g_DCServer;

namespace DCServer {

	void Server::start(short port) {
		server = std::make_shared<TCP::Server<uint32_t> >(ios, port, 
			[this](std::shared_ptr<TCP::Connection<uint32_t> >& conn) {
				handler_acceptConnection(conn);
			});
		iosThread = std::make_shared<std::thread>([&] {
			boost::asio::io_service::work work(ios);
			server->start();

			ios.run();
		});
		
	}

	void Server::handler_acceptConnection(Server::ConnectionPtr& conn) {
		Logger::debug("[Accepted new connection!]");
		pendingConnections.push_back(conn);
	}

}