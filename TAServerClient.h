#pragma once

#include "buildconfig.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <nlohmann/json.hpp>

#include "SdkHeaders.h"
#include "Logger.h"
#include "Utils.h"

#include "TCP.h"
#include "TAServerTypes.h"

using json = nlohmann::json;
using boost::asio::ip::tcp;

namespace TAServer {

	class Client {
	private:
		boost::asio::io_service ios;
		std::shared_ptr<std::thread> iosThread;
		std::shared_ptr<std::thread> gameInfoPollingThread;
		std::shared_ptr<TCP::Client<short> > tcpClient;
		std::map<long long, Launcher2GameLoadoutMessage> receivedLoadouts;
		std::mutex receivedLoadoutsMutex;
	
	private:

		void attachHandlers();
	public:
		Client() {
		}
		bool connect(std::string host, int port);
		bool disconnect();
		bool isConnected();

		bool retrieveLoadout(FUniqueNetId uniquePlayerId, int classId, int slot, std::map<int, int>& resultEquipMap);

		void sendProtocolVersion();
		void sendTeamInfo(const std::map<long long, int>& playerToTeamId);
		void sendScoreInfo(int beScore, int dsScore);
		void sendMapInfo(int mapId);
		void sendMatchTime(long long matchSecondsLeft, bool counting);
		void sendMatchEnded();

		void handler_OnConnect();
		void handler_Launcher2GameLoadoutMessage(const json& msgBody);
		void handler_Launcher2GameNextMapMessage(const json& msgBody);
		void handler_Launcher2GamePingsMessage(const json& msgBody);
	};
}

void pollForGameInfoChanges();

extern TAServer::Client g_TAServerClient;