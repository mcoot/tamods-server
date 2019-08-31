#pragma once

#include <string>
#include <set>
#include <map>
#include <memory>
//#include <variant>
#include "Lua.h"

struct ServerRole {
	std::string name;
	std::string password;
	bool canExecuteArbitraryLua;
	std::set<std::string> allowedCommands;
public:
	ServerRole() :
		name("default"),
		password("default"),
		canExecuteArbitraryLua(false)
	{}

	ServerRole(std::string name, std::string password, bool canExecuteArbitraryLua) :
		name(name),
		password(password),
		canExecuteArbitraryLua(canExecuteArbitraryLua),
		allowedCommands()
	{}

	void addAllowedCommand(std::string commandName);
	void removeAllowedCommand(std::string commandName);
	bool isCommandAllowed(std::string commandName);
};

struct ServerCommand {
	enum class CommandArgType {
		BOOL = 0,
		INT = 1,
		FLOAT = 2,
		STRING = 3
	};

	struct ServerCommandArg {
		std::string argument;
		CommandArgType type;
	public:
		ServerCommandArg(std::string argument, CommandArgType type) : 
			argument(argument), 
			type(type) 
		{}
	};

	struct ParsedArg {
		CommandArgType type;
		union {
			bool boolValue;
			int intValue;
			double floatValue;
			std::string stringValue;
		};
	public:
		ParsedArg(bool b) : 
			type(CommandArgType::BOOL), 
			boolValue(b) 
		{}
		ParsedArg(int i) :
			type(CommandArgType::INT),
			intValue(i)
		{}
		ParsedArg(double f) :
			type(CommandArgType::FLOAT),
			floatValue(f)
		{}
		ParsedArg(std::string s) :
			type(CommandArgType::STRING),
			stringValue(s)
		{}

		ParsedArg(const ParsedArg& other) : type(other.type) {
			switch (other.type) {
			case CommandArgType::BOOL:
				boolValue = other.boolValue;
				break;
			case CommandArgType::INT:
				intValue = other.intValue;
				break;
			case CommandArgType::FLOAT:
				floatValue = other.floatValue;
				break;
			case CommandArgType::STRING:
				stringValue = other.stringValue;
				break;
			}
		}

		ParsedArg& operator=(const ParsedArg& other) {
			type = other.type;
			switch (other.type) {
			case CommandArgType::BOOL:
				boolValue = other.boolValue;
				break;
			case CommandArgType::INT:
				intValue = other.intValue;
				break;
			case CommandArgType::FLOAT:
				floatValue = other.floatValue;
				break;
			case CommandArgType::STRING:
				stringValue = other.stringValue;
				break;
			}
			return *this;
		}

		~ParsedArg() {}
	};

	struct ArgValidationResult {
		bool success;
		union {
			std::vector<std::string> validationFailures;
			std::vector<ParsedArg> parsedArguments;
		};
	public:
		ArgValidationResult(std::vector<std::string> validationFailures) :
			success(false),
			validationFailures(validationFailures)
		{}
		ArgValidationResult(std::vector<ParsedArg> parsedArguments) :
			success(true),
			parsedArguments(parsedArguments)
		{}
		ArgValidationResult(const ArgValidationResult& other) : success(other.success) {
			if (other.success) {
				parsedArguments = other.parsedArguments;
			}
			else {
				validationFailures = other.validationFailures;
			}
		}

		~ArgValidationResult() {}
	};

	std::string commandName;
	std::vector<ServerCommandArg> arguments;
	LuaRef* func;
public:
	ServerCommand() : 
		commandName(""), 
		arguments(std::vector<ServerCommandArg>()), 
		func(NULL) 
	{}

	ServerCommand(std::string commandName, std::vector<ServerCommandArg> arguments, LuaRef func) : 
		commandName(commandName), 
		arguments(arguments),
		func(new LuaRef(func))
	{}

	~ServerCommand() {
		delete func;
	}

	// Returns a vector of validation errors; empty if successful
	ArgValidationResult validateArguments(std::vector<std::string> receivedParameters);
	// Returns empty vec on success, list of errors on failure
	std::vector<std::string> execute(std::string playerName, std::string rolename, std::vector<std::string> receivedParameters);

	template<typename Tuple, size_t ... I>
	auto callFunc(Tuple t, std::index_sequence<I ...>) {
		return (*func)(std::get<I>(t) ...);
	}

	template<typename Tuple>
	auto callFunc(Tuple t) {
		static constexpr auto size = std::tuple_size<Tuple>::value;
		return callFunc(t, std::make_index_sequence<size>{});
	}
};

struct ServerAccessControl {
	std::map<std::string, std::shared_ptr<ServerRole>> roles;
	std::map<std::string, std::shared_ptr<ServerCommand>> commands;
public:
	ServerAccessControl() {
		// Always add the default role
		auto defaultRole = std::make_shared<ServerRole>();
		roles[defaultRole->name] = defaultRole;
	}
};