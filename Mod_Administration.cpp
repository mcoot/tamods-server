#include "Mods.h"

void DCServer::Server::handler_RoleLoginMessage(std::shared_ptr<PlayerConnection> pconn, const json& j) {
	Logger::debug("[Received login message]");

	RoleLoginMessage msg;

	if (!msg.fromJson(j)) {
		Logger::warn("Failed to parse role login message from client %d: %s", pconn->playerId, j.dump().c_str());
		return;
	}

	// Attempt to log the user into this role
	if (g_config.serverAccessControl.roles.find(msg.role) == g_config.serverAccessControl.roles.end()) {
		Logger::info("Login attempt to non-existent role %s by player %d", msg.role.c_str(), pconn->playerId);
		std::vector<MessageToClientMessage::ConsoleMsgDetails> v;
		v.push_back(MessageToClientMessage::ConsoleMsgDetails("Login failed"));
		sendMessageToClient(pconn, v, MessageToClientMessage::IngameMsgDetails());
		return;
	}
	if (msg.password != g_config.serverAccessControl.roles[msg.role].password) {
		Logger::info("Failed login attempt to role %s by player %d", msg.role.c_str(), pconn->playerId);
		std::vector<MessageToClientMessage::ConsoleMsgDetails> v;
		v.push_back(MessageToClientMessage::ConsoleMsgDetails("Login failed"));
		sendMessageToClient(pconn, v, MessageToClientMessage::IngameMsgDetails());
		return;
	}

	// Successful login
	pconn->role = g_config.serverAccessControl.roles[msg.role];
	Logger::info("Successful login attempt to role %s by player %d", msg.role.c_str(), pconn->playerId);
	std::vector<MessageToClientMessage::ConsoleMsgDetails> v;
	v.push_back(MessageToClientMessage::ConsoleMsgDetails("Login succeeded"));
	sendMessageToClient(pconn, v, MessageToClientMessage::IngameMsgDetails());
}

void DCServer::Server::handler_ExecuteCommandMessage(std::shared_ptr<PlayerConnection> pconn, const json& j) {
	Logger::debug("[Attempt to execute server command]");
	ExecuteCommandMessage msg;

	if (!msg.fromJson(j)) {
		Logger::warn("Failed to parse execute command message from client %d: %s", pconn->playerId, j.dump().c_str());
		return;
	}

	//Logger::debug("Attempt to execute server command (%d): %s", msg.rawLua, msg.commandString.c_str());

	// Currently only raw Lua commands are supported

	if (msg.rawLua) {
		Logger::debug("Command is Raw Lua");
		if (!pconn) {
			Logger::warn("Command execution failed due to missing player connection");
			return;
		}

		Logger::debug("Checking user role...");
		// Check the user's role
		if (!pconn->role.canExecuteArbitraryLua) {
			Logger::warn("Player %d attempted to execute arbitrary lua, but their role (%s) does not allow this", pconn->playerId, pconn->role.name.c_str());
			return;
		}
		Logger::debug("User role validated");

		//Logger::debug("Player %d executing command: %s", msg.commandString.c_str());
		g_config.lua.doString(msg.commandString);
		
	}
}

static void addRole(std::string roleName, std::string password, bool canExecuteLua) {
	if (g_config.serverAccessControl.roles.find(roleName) != g_config.serverAccessControl.roles.end()) {
		Logger::warn("Attempted to create role %s which already exists", roleName.c_str());
		return;
	}
	g_config.serverAccessControl.roles[roleName] = ServerRole(roleName, password, canExecuteLua);
}

static void removeRole(std::string roleName) {
	g_config.serverAccessControl.roles.erase(roleName);
}

static void sendGameMessageToAllPlayers(std::string message, float messageTime = 5.0) {
	g_DCServer.forAllKnownConnections([message, messageTime](DCServer::Server* srv, std::shared_ptr<DCServer::PlayerConnection> pconn) {
		DCServer::MessageToClientMessage::IngameMsgDetails details;

		details.doShow = true;
		details.message = message;
		details.priority = 3;
		details.time = messageTime;

		srv->sendMessageToClient(pconn, std::vector< DCServer::MessageToClientMessage::ConsoleMsgDetails>(), details);
	});
}

static void sendConsoleMessageToAllPlayers(std::string message) {
	g_DCServer.forAllKnownConnections([message](DCServer::Server* srv, std::shared_ptr<DCServer::PlayerConnection> pconn) {
		DCServer::MessageToClientMessage::ConsoleMsgDetails details;

		details.message = message;
		details.r = 255;
		details.g = 255;
		details.b = 255;
		details.a = 255;

		std::vector<DCServer::MessageToClientMessage::ConsoleMsgDetails> v;
		v.push_back(details);

		srv->sendMessageToClient(pconn, v, DCServer::MessageToClientMessage::IngameMsgDetails());
	});
}

static void endCurrentMap() {
	std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
	if (!Utils::tr_gri || !Utils::tr_gri->WorldInfo || !Utils::tr_gri->WorldInfo->Game) return;

	if (!Utils::tr_gri->bMatchIsOver) {
		ATrGame* game = (ATrGame*)Utils::tr_gri->WorldInfo->Game;
		game->bForceEndGame = true;
		Utils::tr_gri->RemainingTime = 1;
		Utils::tr_gri->RemainingMinute = 1;
	}
}

static void startCurrentMap() {
	std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
	if (!Utils::tr_gri) return;

	if (!Utils::tr_gri->bWarmupRound) {
		Utils::tr_gri->RemainingTime = 1;
		Utils::tr_gri->RemainingMinute = 1;
	}
}

int bNextMapOverrideValue;
static void setNextMap(int mapId) {
	if (Data::map_id_to_name.find(mapId) == Data::map_id_to_name.end()) {
		Logger::info("Can't set next map to unknown ID %d", mapId);
		return;
	}

	bNextMapOverrideValue = mapId;
	Logger::debug("Next map was set to %d", mapId);
}

namespace LuaAPI {
	void addAdministrationAPI(luabridge::Namespace ns) {
		ns
			.beginNamespace("Admin")
				.beginNamespace("Roles")
					.addFunction("add", &addRole)
					.addFunction("remove", &addRole)
				.endNamespace()
				.addFunction("SendGameMessageToAllPlayers", &sendGameMessageToAllPlayers)
				.addFunction("SendConsoleMessageToAllPlayers", &sendConsoleMessageToAllPlayers)
				.beginNamespace("Game")
					.addFunction("StartMap", &startCurrentMap)
					.addFunction("EndMap", &endCurrentMap)
					.addFunction("NextMap", &setNextMap)
				.endNamespace()
			.endNamespace();
	}
}