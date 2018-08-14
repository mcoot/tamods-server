#pragma once

#include "buildconfig.h"

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <nlohmann/json.hpp>

#include "SdkHeaders.h"
#include "Logger.h"
#include "Utils.h"

#include "TAServerTypes.h"

using json = nlohmann::json;
using boost::asio::ip::tcp;

namespace TAServer {

	class Client {
	private:
		boost::asio::io_service ios;
		std::shared_ptr<tcp::socket> socket;
		std::array<char, 1024> recvBuffer;
	
	private:
		void sendMessage(std::shared_ptr<Message> message, boost::system::error_code& err);
		std::shared_ptr<Message> recvMessage(boost::system::error_code& err);

	public:
		Client() {
			socket = std::make_shared<tcp::socket>(ios);
		}
		bool connect(std::string host, int port);
		bool disconnect();
		bool isConnected();

		bool retrieveLoadout(FUniqueNetId uniquePlayerId, int classId, int slot, std::map<int, int>& resultEquipMap);
	};
}

extern TAServer::Client g_TAServerClient;