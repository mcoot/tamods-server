#include "DirectConnectionServer.h"

DCServer::Server g_DCServer;

namespace DCServer {

	bool Server::isPlayerAKnownModdedConnection(FUniqueNetId playerId) {
		long long lpid = TAServer::netIdToLong(playerId);
		{
			std::lock_guard<std::mutex> lock(knownPlayerConnectionsMutex);
			auto& it = knownPlayerConnections.find(lpid);
			return it != knownPlayerConnections.end();
		}
	}

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

	void Server::applyHandlersToConnection(std::shared_ptr<PlayerConnection> pconn) {
		pconn->conn->set_stop_handler([this, pconn](boost::system::error_code& err) {
			// Run the server-level stop connection handler
			// `this` will be the server, pconn is the player connection object (captured here)
			// the point being, this runs inside the Connection but has access to the Server and PlayerConnection
			handler_stopConnection(pconn, err);
		});

		pconn->conn->add_handler(DCSRV_MSG_KIND_PLAYER_CONNECTION, [this, pconn](const json& j) {
			handler_PlayerConnectionMessage(pconn, j);
		});
		
		pconn->conn->add_handler(DCSRV_MSG_KIND_ROLE_LOGIN, [this, pconn](const json& j) {
			handler_RoleLoginMessage(pconn, j);
		});
		
		pconn->conn->add_handler(DCSRV_MSG_KIND_EXECUTE_COMMAND, [this, pconn](const json& j) {
			handler_ExecuteCommandMessage(pconn, j);
		});
	}

	void Server::handler_acceptConnection(ConnectionPtr& conn) {
		// Counter used for temporary IDs
		static int uniquePendingId = 0;

		std::shared_ptr<PlayerConnection> pconn = std::make_shared<PlayerConnection>(conn);
		pconn->validated = false;
		applyHandlersToConnection(pconn);

		// Use a temporary ID until we know the ID of this player
		// Mutex to prevent race condition in ID assignment
		std::lock_guard<std::mutex> lock(pendingConnectionsMutex);
		pconn->playerId = uniquePendingId++;
		pendingConnections.push_back(conn);

		Logger::info("Accepted new unvalidated connection, with temporary ID %ld", pconn->playerId);
	}

	void Server::handler_stopConnection(std::shared_ptr<PlayerConnection> pconn, boost::system::error_code& err) {
		Logger::info("[Connection for Pid %ld Stopped with code %d]", pconn->playerId, pconn->conn->get_error_state().value());

		// Delete from pending connections if there
		{
			std::lock_guard<std::mutex> lock(pendingConnectionsMutex);
			for (unsigned int i = 0; i < pendingConnections.size(); ++i) {
				if (pendingConnections[i].playerId == pconn->playerId) {
					// Remove this from pending connections
					pendingConnections.erase(pendingConnections.begin() + i);
					break;
				}
			}
		}

		// Delete from map if there
		{
			std::lock_guard<std::mutex> lock(knownPlayerConnectionsMutex);
			auto& it = knownPlayerConnections.find(pconn->playerId);
			if (it != knownPlayerConnections.end()) {
				knownPlayerConnections.erase(pconn->playerId);
			}
		}
	}

	static bool isVersionCompatible(std::string v, std::string referenceV) {
		return true;
	}

	bool Server::validatePlayerConn(std::shared_ptr<PlayerConnection> pconn, const PlayerConnectionMessage& connDetails) {
		// TODO: Validate protocol version
		if (!isVersionCompatible(connDetails.protocolVersion, TAMODS_PROTOCOL_VERSION)) {

		}

		// Check if player was already validated
		if (pconn->validated) {
			Logger::warn("Duplicate connection message for connection %ld", pconn->playerId);
			return false;
		}

		// Not validated, so the playerId currently refers to a temporary ID
		// Find and remove it in the pending connections
		{
			std::lock_guard<std::mutex> lock(pendingConnectionsMutex);
			for (unsigned int i = 0; i < pendingConnections.size(); ++i) {
				if (pendingConnections[i].playerId == pconn->playerId) {
					// Remove this from pending connections
					pendingConnections.erase(pendingConnections.begin() + i);
					break;
				}
			}
		}

		// Add the player to the map of known players
		pconn->playerId = TAServer::netIdToLong(connDetails.uniquePlayerId);
		{
			std::lock_guard<std::mutex> lock(knownPlayerConnectionsMutex);
			knownPlayerConnections[pconn->playerId] = pconn;
		}
		pconn->validated = true;

		Logger::info("Successfully validated new connection from player %ld", pconn->playerId);

		return true;
	}

	void Server::forAllKnownConnections(std::function<void(Server*, std::shared_ptr<PlayerConnection>)> f) {
		// Need to iterate over all players to send the appropriate update to each
			// Needs to be concurrency safe - since a player could disconnect while this operation is ongoing and be removed from the map

			// Snapshot of the known connections
		std::vector<std::shared_ptr<PlayerConnection> > connList;
		{
			std::lock_guard<std::mutex> lock(knownPlayerConnectionsMutex);

			for (auto& it : knownPlayerConnections) {
				if (it.second) {
					connList.push_back(it.second);
				}
			}
		}

		for (auto& pconn : connList) {
			if (!pconn || !pconn->conn) continue;

			f(this, pconn);
		}
	}

	void Server::sendGameBalanceDetailsMessage(std::shared_ptr<PlayerConnection> pconn,
		const GameBalance::Items::ItemsConfig& itemProperties,
		const GameBalance::Items::DeviceValuesConfig& deviceValueProperties,
		const GameBalance::Classes::ClassesConfig& classProperties,
		const GameBalance::Vehicles::VehiclesConfig& vehicleProperties,
		const GameBalance::VehicleWeapons::VehicleWeaponsConfig& vehicleWeaponProperties
	) 
	{
		GameBalanceDetailsMessage msg;
		msg.itemProperties = itemProperties;
		msg.deviceValueProperties = deviceValueProperties;
		msg.classProperties = classProperties;
		msg.vehicleProperties = vehicleProperties;
		msg.vehicleWeaponProperties = vehicleWeaponProperties;


		json j;
		msg.toJson(j);

		pconn->conn->send(msg.getMessageKind(), j);
	}

	void Server::sendStateUpdateMessage(std::shared_ptr<PlayerConnection> pconn, float playerPing) {
		StateUpdateMessage msg;
		msg.playerPing = playerPing;

		json j;
		msg.toJson(j);

		pconn->conn->send(msg.getMessageKind(), j);
	}

	void Server::sendMessageToClient(std::shared_ptr<PlayerConnection> pconn, std::vector<MessageToClientMessage::ConsoleMsgDetails>& consoleMessages, MessageToClientMessage::IngameMsgDetails& ingameMessage) {
		MessageToClientMessage msg;
		msg.consoleMessages = consoleMessages;
		msg.ingameMessage = ingameMessage;

		json j;
		msg.toJson(j);

		pconn->conn->send(msg.getMessageKind(), j);
	}

}