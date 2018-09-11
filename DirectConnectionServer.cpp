#include "DirectConnectionServer.h"

DCServer::Server g_DCServer;

namespace DCServer {

	void Server::start(short port) {
		TCP::Server<uint32_t>::AcceptHandler acceptHandler = [this](ConnectionPtr& conn) {
			handler_acceptConnection(conn);
		};

		server = std::make_shared<TCP::Server<uint32_t> >(ios, port, 
			acceptHandler);
		iosThread = std::make_shared<std::thread>([&] {
			boost::asio::io_service::work work(ios);
			server->start();

			ios.run();
		});
		
	}

	void Server::handler_acceptConnection(ConnectionPtr& conn) {
		std::shared_ptr<PlayerConnection> pconn = std::make_shared<PlayerConnection>(conn);
		conn->set_stop_handler([&](boost::system::error_code& err) {
			// Run the server-level stop connection handler
			// `this` will be the server, pconn is the player connection object (captured here)
			// the point being, this runs inside the Connection but has access to the Server and PlayerConnection
			handler_stopConnection(pconn, err);
		});

		// Use the index in the pending vector as a temporary ID
		// (until this connection is associated with a player)
		std::lock_guard<std::mutex> lock(pendingConnectionsMutex);
		pconn->playerId = pendingConnections.size();
		pendingConnections.push_back(conn);
	}

	void Server::handler_stopConnection(std::shared_ptr<PlayerConnection> pconn, boost::system::error_code& err) {
		Logger::debug("[Connection Stopped]");
	}

}