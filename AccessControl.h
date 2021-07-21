#pragma once

#include <string>
#include <set>
#include <map>
#include <memory>
#include <boost/variant.hpp>
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

    typedef boost::variant<bool, int, double, std::string> ParsedArg;
    typedef std::vector<std::string> ValidationErrors;

    typedef boost::variant<ValidationErrors, std::vector<ParsedArg>> ArgValidationResult;

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
    ArgValidationResult validateArguments(const std::vector<std::string>& receivedParameters);
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