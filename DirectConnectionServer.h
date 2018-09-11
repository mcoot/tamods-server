#pragma once

#include "buildconfig.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <mutex>

#include <nlohmann/json.hpp>

#include "SdkHeaders.h"
#include "Logger.h"
#include "Utils.h"

#include "TCP.h"
#include "DirectConnectionTypes.h"

using json = nlohmann::json;
using boost::asio::ip::tcp;

namespace DCServer {

	typedef std::shared_ptr<TCP::Connection<uint32_t> > ConnectionPtr;

	struct PlayerConnection {
	public:
		ConnectionPtr conn;
		long long playerId = 0;
		bool validated = false;
	public:
		PlayerConnection(ConnectionPtr conn) : conn(conn) {}
	};

	class Server {
	private:
		boost::asio::io_service ios;
		std::shared_ptr<std::thread> iosThread;

		std::shared_ptr<TCP::Server<uint32_t>> server;

		std::mutex knownPlayerConnectionsMutex;
		std::map<long long, PlayerConnection> knownPlayerConnections;

		std::mutex pendingConnectionsMutex;
		std::vector<PlayerConnection> pendingConnections;

		void handler_acceptConnection(ConnectionPtr& conn);
		void handler_stopConnection(std::shared_ptr<PlayerConnection> pconn, boost::system::error_code& err);
	public:
		Server() {}

		void start(short port);
	};

}

extern DCServer::Server g_DCServer;