#pragma once

#include <string>
#include <map>

struct ServerRole {
	std::string name;
	std::string password;
	bool canExecuteArbitraryLua;

public:
	ServerRole() :
		name("default"),
		password("default"),
		canExecuteArbitraryLua(false)
	{}

	ServerRole(std::string name, std::string password, bool canExecuteArbitraryLua) :
		name(name),
		password(password),
		canExecuteArbitraryLua(canExecuteArbitraryLua)
	{}
};

struct ServerAccessControl {
	std::map<std::string, ServerRole> roles;
};