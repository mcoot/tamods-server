#include "Mods.h"

void ServerRole::addAllowedCommand(std::string commandName) {
	allowedCommands.insert(commandName);
}

void ServerRole::removeAllowedCommand(std::string commandName) {
	allowedCommands.erase(commandName);
}

bool ServerRole::isCommandAllowed(std::string commandName) {
	return allowedCommands.count(commandName) != 0;
}

ServerCommand::ArgValidationResult ServerCommand::validateArguments(std::vector<std::string> receivedParameters) {
	std::vector<ParsedArg> parsedParams;
	std::vector<std::string> errors;
	
	if (receivedParameters.size() != arguments.size()) {
		errors.push_back((boost::format("Expected %1% arguments, got %2%") % arguments.size() % receivedParameters.size()).str());
		return ArgValidationResult(errors);
	}

	// Max of 6 arguments (LuaBridge allows 8, we use two to give playerid + role)
	if (receivedParameters.size() > 6) {
		errors.push_back((boost::format("Commands may only have a maximum of 8 arguments, got %1%") % receivedParameters.size()).str());
		return ArgValidationResult(errors);
	}

	for (int i = 0; i < receivedParameters.size(); ++i) {
		switch (arguments[i].type) {
		case CommandArgType::BOOL:
			if (receivedParameters[i] == "true") {
				parsedParams.push_back(ParsedArg(true));
			}
			else if (receivedParameters[i] == "false") {
				parsedParams.push_back(ParsedArg(false));
			}
			else {
				errors.push_back((boost::format("Parameter %1% should be a boolean; could not parse value \"%2%\"") % i % receivedParameters[i]).str());
			}
			break;
		case CommandArgType::INT:
			int intTest;
			try {
				intTest = std::stoi(receivedParameters[i]);
				parsedParams.push_back(ParsedArg(intTest));

			}
			catch (std::invalid_argument&) {
				errors.push_back((boost::format("Parameter %1% should be an integer; could not parse value \"%2%\"") % i % receivedParameters[i]).str());
			}
			break;
		case CommandArgType::FLOAT:
			double doubleTest;
			try {
				doubleTest = std::stod(receivedParameters[i]);
				parsedParams.push_back(ParsedArg(doubleTest));
			}
			catch (std::invalid_argument&) {
				errors.push_back((boost::format("Parameter %1% should be a floating point number; could not parse value \"%2%\"") % i % receivedParameters[i]).str());
			}
			break;
		case CommandArgType::STRING:
			parsedParams.push_back(ParsedArg(receivedParameters[i]));
			break;
		}
	}

	if (errors.size() > 0) {
		return ArgValidationResult(errors);
	}
	else {
		return ArgValidationResult(parsedParams);
	}
}

std::vector<std::string> ServerCommand::execute(std::string playerName, std::string rolename, std::vector<std::string> receivedParameters) {
	ArgValidationResult v = validateArguments(receivedParameters);
	// Don't execute if params are invalid; also validated that we have at most 6 args
	if (!v.success) return v.validationFailures;

	std::vector<ParsedArg> args = v.parsedArguments;

	std::vector<LuaRef> refArgs;
	// Add the arguments for the playerId and role name
	refArgs.push_back(LuaRef(g_config.lua.getState(), playerName));
	refArgs.push_back(LuaRef(g_config.lua.getState(), rolename));
	// Luabridge functions can only have up to 8 arguments; we always pass 8 arguments, potentially nils
	for (int i = 0; i < 6; ++i) {

		if (i < args.size()) {
			switch (args[i].type) {
			case CommandArgType::BOOL:
				refArgs.push_back(LuaRef(g_config.lua.getState(), args[i].boolValue));
				break;
			case CommandArgType::INT:
				refArgs.push_back(LuaRef(g_config.lua.getState(), args[i].intValue));
				break;
			case CommandArgType::FLOAT:
				refArgs.push_back(LuaRef(g_config.lua.getState(), args[i].floatValue));
				break; 
			case CommandArgType::STRING:
				refArgs.push_back(LuaRef(g_config.lua.getState(), args[i].stringValue));
				break;
			}
			
		}
		else {
			// Nil for this argument
			refArgs.push_back(LuaRef(g_config.lua.getState()));
		}
	}

	LuaRef f = *func;
	// Call the command
	LuaRef result = f(refArgs[0], refArgs[1], refArgs[2], refArgs[3], refArgs[4], refArgs[5], refArgs[6], refArgs[7]);

	if (result.type() == LUA_TBOOLEAN) {
		if (!result.cast<bool>()) {
			std::vector<std::string> err;
			err.push_back("Command returned an unsuccessful result");
			return err;
		}
	}

	return std::vector<std::string>();
}

void DCServer::Server::handler_RoleLoginMessage(std::shared_ptr<PlayerConnection> pconn, const json& j) {
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
	if (msg.password != g_config.serverAccessControl.roles[msg.role]->password) {
		Logger::info("Failed login attempt to role %s by player %d", msg.role.c_str(), pconn->playerId);
		std::vector<MessageToClientMessage::ConsoleMsgDetails> v;
		v.push_back(MessageToClientMessage::ConsoleMsgDetails("Login failed"));
		sendMessageToClient(pconn, v, MessageToClientMessage::IngameMsgDetails());
		return;
	}

	// Successful login
	Logger::info("Successful login attempt to role %s by player %d", msg.role.c_str(), pconn->playerId);
	pconn->role = g_config.serverAccessControl.roles[msg.role]->name;
	std::vector<MessageToClientMessage::ConsoleMsgDetails> v;
	v.push_back(MessageToClientMessage::ConsoleMsgDetails((boost::format("Logged in as %1%") % msg.role).str()));
	sendMessageToClient(pconn, v, MessageToClientMessage::IngameMsgDetails());
}

void DCServer::Server::handler_ExecuteCommandMessage(std::shared_ptr<PlayerConnection> pconn, const json& j) {
	ExecuteCommandMessage msg;

	if (!msg.fromJson(j)) {
		Logger::warn("Failed to parse execute command message from client %d: %s", pconn->playerId, j.dump().c_str());
		return;
	}

	if (msg.rawLua) {
		if (!pconn) {
			Logger::warn("Command execution failed due to missing player connection");
			return;
		}

		// Check the user's role
		if (!g_config.serverAccessControl.roles[pconn->role]->canExecuteArbitraryLua) {
			Logger::warn("Player %d attempted to execute arbitrary lua, but their role (%s) does not allow this", pconn->playerId, pconn->role.c_str());
			std::vector<MessageToClientMessage::ConsoleMsgDetails> v;
			v.push_back(MessageToClientMessage::ConsoleMsgDetails("Access denied", 255, 128, 128, 255));
			sendMessageToClient(pconn, v, MessageToClientMessage::IngameMsgDetails());
			return;
		}

		g_config.lua.doString(msg.commandString);
	}
	else {
		std::vector<std::string> spaceTokens;
		std::vector<std::string> finalTokens;

		// Split the command string on space, respecting double-quotes to group a multi-word string
		size_t start = 0, end = 0;
		std::string curToken;

		// Split on space
		while ((end = msg.commandString.find(" ", start)) != std::string::npos) {
			curToken = msg.commandString.substr(start, end - start);
			if (curToken.length() > 0 && !std::all_of(curToken.begin(), curToken.end(), isspace)) {
				spaceTokens.push_back(curToken);
			}
			start = end + 1;
		}
		// One token after the final space
		curToken = msg.commandString.substr(start, end);
		if (curToken.length() > 0 && !std::all_of(curToken.begin(), curToken.end(), isspace)) {
			spaceTokens.push_back(curToken);
		}

		// We want to handle quotes, so if we have a token starting with ", concat until we find one ending with "
		// If we don't find an end quote, they are mismatched
		for (size_t i = 0; i < spaceTokens.size(); ++i) {
			if (spaceTokens[i].find("\"") == 0) {
				size_t concatStartIdx = i;
				curToken = "";
				std::string spaceDelimiter = "";
				// Find the matching end token
				while (i < spaceTokens.size()) {
					curToken += spaceDelimiter + spaceTokens[i];
					spaceDelimiter = " ";
					if (spaceTokens[i].rfind("\"") == spaceTokens[i].length() - 1 
						&& (i != concatStartIdx || spaceTokens[concatStartIdx].find("\"") != spaceTokens[i].rfind("\""))) break;
					i++;
				}

				// Check that our final token actually ends with a quote
				if (curToken.rfind("\"") != curToken.length() - 1) {
					// Missing end quote, this is an invalid string
					std::vector<MessageToClientMessage::ConsoleMsgDetails> v;
					v.push_back(MessageToClientMessage::ConsoleMsgDetails("Invalid command: mismatched quotes or invalid quote grouping", 255, 128, 128, 255));
					sendMessageToClient(pconn, v, MessageToClientMessage::IngameMsgDetails());
					return;
				}

				// Add the concatenated token, without the quotes
				finalTokens.push_back(curToken.substr(1, curToken.length() - 2));
			}
			else {
				// Add this token by itself
				finalTokens.push_back(spaceTokens[i]);
			}
		}

		if (finalTokens.size() == 0) {
			// Need at least a command
			std::vector<MessageToClientMessage::ConsoleMsgDetails> v;
			v.push_back(MessageToClientMessage::ConsoleMsgDetails("Invalid command: no command name given", 255, 128, 128, 255));
			sendMessageToClient(pconn, v, MessageToClientMessage::IngameMsgDetails());
			return;
		}

		std::string commandName = finalTokens[0];
		std::vector<std::string> arguments(finalTokens.begin() + 1, finalTokens.end());

		// Check that the command exists
		if (g_config.serverAccessControl.commands.find(commandName) == g_config.serverAccessControl.commands.end()) {
			std::vector<MessageToClientMessage::ConsoleMsgDetails> v;
			v.push_back(MessageToClientMessage::ConsoleMsgDetails((boost::format("Invalid command: no command %1% exists") % commandName).str(), 255, 128, 128, 255));
			sendMessageToClient(pconn, v, MessageToClientMessage::IngameMsgDetails());
			return;
		}

		// Access control check
		if (!g_config.serverAccessControl.roles[pconn->role]->isCommandAllowed(commandName)) {
			Logger::warn("Player %d attempted to execute command %s, but their role, %s, does not allow this", pconn->playerId, commandName.c_str(), pconn->role.c_str());
			std::vector<MessageToClientMessage::ConsoleMsgDetails> v;
			v.push_back(MessageToClientMessage::ConsoleMsgDetails("Access denied", 255, 128, 128, 255));
			sendMessageToClient(pconn, v, MessageToClientMessage::IngameMsgDetails());
			return;
		}

		std::string playerName;
		{
			std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
			ATrPlayerReplicationInfo* pri = Utils::getPRIForPlayerId(pconn->playerId);
			if (pri) {
				playerName = Utils::f2std(pri->PlayerName);
			}
		}

		std::vector<std::string> errors = g_config.serverAccessControl.commands[commandName]->execute(playerName, pconn->role, arguments);

		if (errors.size() != 0) {
			std::vector<MessageToClientMessage::ConsoleMsgDetails> v;
			for (auto& error : errors) {
				v.push_back(MessageToClientMessage::ConsoleMsgDetails(error, 255, 128, 128, 255));
			}
			sendMessageToClient(pconn, v, MessageToClientMessage::IngameMsgDetails());
		}
	}
}

static void addRole(std::string roleName, std::string password, bool canExecuteLua) {
	if (g_config.serverAccessControl.roles.find(roleName) != g_config.serverAccessControl.roles.end()) {
		Logger::warn("Attempted to create role %s which already exists", roleName.c_str());
		return;
	}
	g_config.serverAccessControl.roles[roleName] = std::make_shared<ServerRole>(roleName, password, canExecuteLua);
}

static void removeRole(std::string roleName) {
	g_config.serverAccessControl.roles.erase(roleName);
}

static void addAllowedCommandToRole(std::string roleName, std::string commandName) {
	if (g_config.serverAccessControl.roles.find(roleName) == g_config.serverAccessControl.roles.end()) {
		Logger::warn("Failed to add command to role %s; role does not exist", roleName.c_str());
		return;
	}

	g_config.serverAccessControl.roles[roleName]->addAllowedCommand(commandName);
}

static void removeAllowedCommandFromRole(std::string roleName, std::string commandName) {
	if (g_config.serverAccessControl.roles.find(roleName) == g_config.serverAccessControl.roles.end()) {
		Logger::warn("Failed to add command to role %s; role does not exist", roleName.c_str());
		return;
	}

	g_config.serverAccessControl.roles[roleName]->removeAllowedCommand(commandName);
}

static void defineCommand(std::string commandName, LuaRef paramDefinitions, LuaRef func) {
	std::vector<ServerCommand::ServerCommandArg> args;

	if (!paramDefinitions.isTable()) {
		Logger::error("Failed to define command %s: arguments must be given as a table", commandName.c_str());
		return;
	}

	for (int i = 1; i <= paramDefinitions.length(); ++i) {
		LuaRef curParam = LuaRef(paramDefinitions[i]);
		if (!curParam.isTable()) {
			Logger::error("Failed to define command %s: argument %d must be given as a table", commandName.c_str(), (i - 1));
			return;
		}
		LuaRef lParamName = LuaRef(curParam[1]);
		LuaRef lParamType = LuaRef(curParam[2]);
		if (!lParamName.isString() || !lParamType.isNumber()) {
			Logger::error("Failed to define command %s: parameter %d must be given as a table {name, type}", commandName.c_str(), (i - 1));
			return;
		}

		std::string paramName = lParamName;
		int paramType = lParamType;

		if (paramType != (int)ServerCommand::CommandArgType::BOOL 
			&& paramType != (int)ServerCommand::CommandArgType::INT 
			&& paramType != (int)ServerCommand::CommandArgType::FLOAT 
			&& paramType != (int)ServerCommand::CommandArgType::STRING) {
			Logger::error("Failed to define command %s: argument %d was not a valid argument type", commandName.c_str(), (i - 1));
			return;
		}
		args.push_back(ServerCommand::ServerCommandArg(paramName, (ServerCommand::CommandArgType)paramType));
	}

	g_config.serverAccessControl.commands[commandName] = std::make_shared<ServerCommand>(commandName, args, func);
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

static void sendConsoleMessageToPlayer(std::string playerName, std::string message) {
	long long playerId = -1;
	{
		//std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
		ATrPlayerReplicationInfo* pri = Utils::getPRIForPlayerName(playerName);
		if (pri) {
			playerId = Utils::netIdToLong(pri->UniqueId);
		}
	}
	if (playerId == -1) return;

	std::shared_ptr<DCServer::PlayerConnection> pconn = g_DCServer.getPlayerConnection(playerId);
	if (!pconn) return;

	DCServer::MessageToClientMessage::ConsoleMsgDetails details;

	details.message = message;
	details.r = 255;
	details.g = 255;
	details.b = 255;
	details.a = 255;

	std::vector<DCServer::MessageToClientMessage::ConsoleMsgDetails> v;
	v.push_back(details);

	g_DCServer.sendMessageToClient(pconn, v, DCServer::MessageToClientMessage::IngameMsgDetails());
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
	if (!Utils::tr_gri || !Utils::tr_gri->WorldInfo || !Utils::tr_gri->WorldInfo->Game) return;

	if (Utils::tr_gri->bWarmupRound) {
		ATrGame* game = (ATrGame*)Utils::tr_gri->WorldInfo->Game;
		game->bForceRoundStart = true;
		game->m_nRoundCountdownRemainingTime = 0;
	}
}

int bNextMapOverrideValue;
static bool setNextMap(int mapId) {
	if (Data::map_id_to_name.find(mapId) == Data::map_id_to_name.end()) {
		Logger::info("Can't set next map to unknown ID %d", mapId);
		return false;
	}

	bNextMapOverrideValue = mapId;
	return true;
}

static int argTypeBool = (int)ServerCommand::CommandArgType::BOOL;
static int argTypeInt = (int)ServerCommand::CommandArgType::INT;
static int argTypeFloat = (int)ServerCommand::CommandArgType::FLOAT;
static int argTypeString = (int)ServerCommand::CommandArgType::STRING;

namespace LuaAPI {
	void addAdministrationAPI(luabridge::Namespace ns) {
		ns
			.beginNamespace("Admin")
				.beginNamespace("Roles")
					.addFunction("add", &addRole)
					.addFunction("remove", &addRole)
					.addFunction("addAllowedCommand", &addAllowedCommandToRole)
					.addFunction("removeAllowedCommand", &removeAllowedCommandFromRole)
				.endNamespace()
				.beginNamespace("Command")
					.addFunction("define", &defineCommand)
					.beginNamespace("ArgumentType")
						.addVariable("Boolean", &argTypeBool, false)
						.addVariable("Int", &argTypeInt, false)
						.addVariable("Float", &argTypeFloat, false)
						.addVariable("String", &argTypeString, false)
					.endNamespace()
				.endNamespace()
				.addFunction("SendGameMessageToAllPlayers", &sendGameMessageToAllPlayers)
				.addFunction("SendConsoleMessageToAllPlayers", &sendConsoleMessageToAllPlayers)
				.addFunction("SendConsoleMessageToPlayer", &sendConsoleMessageToPlayer)
				.beginNamespace("Game")
					.addFunction("StartMap", &startCurrentMap)
					.addFunction("EndMap", &endCurrentMap)
					.addFunction("NextMap", &setNextMap)
				.endNamespace()
			.endNamespace();
	}
}