#pragma once

#include "buildconfig.h"

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "SdkHeaders.h"
#include "Logger.h"
#include "Utils.h"

using boost::asio::ip::tcp;

namespace TAServer {

	class Client {
	private:
		boost::asio::io_service ios;
		std::shared_ptr<tcp::socket> socket;
		std::array<char, 1024> recvBuffer;

	public:
		void connectToGameServerLauncher(std::string host, int port);
	};
}

extern TAServer::Client g_TAServerClient;