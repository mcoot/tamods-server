#pragma once

#include "buildconfig.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <nlohmann/json.hpp>

#include "SdkHeaders.h"
#include "Logger.h"
#include "Utils.h"

#include "TCP.h"
#include "DirectConnectionTypes.h"

using json = nlohmann::json;
using boost::asio::ip::tcp;

namespace DCServer {

	class Server {
	private:
		typedef std::shared_ptr<TCP::Connection<uint32_t> > ConnectionPtr;
	private:
		boost::asio::io_service ios;
		std::shared_ptr<std::thread> iosThread;

		std::shared_ptr<TCP::Server<uint32_t>> server;

		std::map<long long, ConnectionPtr> knownPlayerConnections;
		std::vector<ConnectionPtr> pendingConnections;

		void handler_acceptConnection(ConnectionPtr& conn);
	public:
		Server() {}

		void start(short port);
	};

}

extern DCServer::Server g_DCServer;