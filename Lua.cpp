#include "Lua.h"

Lua::Lua(std::string workingDir) {
    _state = luaL_newstate();
    luaL_openlibs(_state);
    setRequireWorkingDir(workingDir);
    init();
    _initInputEvents();
}

Lua::Lua() : Lua(Utils::getConfigDir()) {}

void Lua::setRequireWorkingDir(std::string workingDir) {
    std::string newWorkingDir(workingDir);
    std::replace(newWorkingDir.begin(), newWorkingDir.end(), '\\', '/');
    std::string path = "package.path = \"" + newWorkingDir + "?.lua;\" .. package.path";
    doString(path);
}

Lua::~Lua()
{
}

void            Lua::_initInputEvents()
{
    // Get global namespace
    lua_getglobal(_state, "_G");
    // Create "Input" namespace
    lua_pushstring(_state, "Input");
    lua_newtable(_state);

    // Push variables
    _pushvar("PRESSED", IE_Pressed);
    _pushvar("RELEASED", IE_Released);
    _pushvar("REPEAT", IE_Repeat);
    _pushvar("DOUBLECLICK", IE_DoubleClick);
    _pushvar("AXIS", IE_Axis);

    lua_settable(_state, -3);
}

void            Lua::_pushvar(const char *name, unsigned char val)
{
    lua_pushstring(_state, name);
    lua_pushnumber(_state, val);
    lua_settable(_state, -3);
}

std::string Lua::_error(int ret)
{
    const char    *str;
    std::string out = "";

    if (ret && (str = lua_tostring(_state, -1)))
    {
        out = std::string(str);
        lua_pop(_state, 1);
    }
    return out;
}

std::string Lua::doFile(const std::string &s)
{
    std::string err = "";
    try
    {
        err = _error(luaL_dofile(_state, s.c_str()));
    }
    catch (const LuaException &e)
    {
        return std::string(e.what());
    }
    catch (const std::exception &e)
    {
        return std::string(e.what());
    }
    return err;
}

void            Lua::_printTop()
{
    switch (lua_type(_state, -1))
    {
    case LUA_TNIL:
        Logger::info("nil\n");
        break;
    case LUA_TSTRING:
    case LUA_TNUMBER:
        Logger::info("%s\n", lua_tostring(_state, -1));
        break;
    case LUA_TBOOLEAN:
        Logger::info("%s\n", lua_toboolean(_state, -1) ? "true" : "false");
        break;
    case LUA_TTABLE:
        Logger::info("table: %p\n", lua_topointer(_state, -1));
        break;
    case LUA_TFUNCTION:
        Logger::info("function: %p\n", lua_topointer(_state, -1));
        break;
    case LUA_TUSERDATA:
        Logger::info("userdata: %p\n", lua_topointer(_state, -1));
        break;
    case LUA_TTHREAD:
        Logger::info("thread: %p\n", lua_topointer(_state, -1));
        break;
    case LUA_TLIGHTUSERDATA:
        Logger::info("lightuserdata: %p\n", lua_topointer(_state, -1));
        break;
    }
}

void            Lua::doString(const std::string &s)
{
    try
    {
        int           old = lua_gettop(_state);
        std::string err = _error(luaL_dostring(_state, s.c_str()));

        if (err.size())
            Logger::error(err.c_str());
        if (lua_gettop(_state) != old)
        {
            _printTop();
            lua_pop(_state, 1);
        }
    }
    catch (const LuaException &e)
    {
        Logger::error("LuaException: %s", e.what());
    }
}

void            Lua::doFunc(const std::string &s)
{
    LuaRef func = getGlobal(_state, s.c_str());

    if (func.isNil())
        return;
    func();
}