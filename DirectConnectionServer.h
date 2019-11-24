#pragma once

#include "buildconfig.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <mutex>

#include <nlohmann/json.hpp>

#include "SdkHeaders.h"
#include "Logger.h"
#include "Utils.h"
#include "AccessControl.h"

#include "TCP.h"
#include "DirectConnectionTypes.h"

using json = nlohmann::json;
using boost::asio::ip::tcp;

namespace DCServer {

    typedef std::shared_ptr<TCP::Connection<uint32_t> > ConnectionPtr;

    struct PlayerConnection {
    public:
        ConnectionPtr conn;
        long long playerId = 0;
        bool validated = false;
        std::string role;
    public:
        PlayerConnection(ConnectionPtr conn) : conn(conn), role("default") {}
    };

    int getServerPort(AWorldInfo* worldInfo);

    class Server {
    private:
        boost::asio::io_service ios;
        std::shared_ptr<std::thread> iosThread;

        std::shared_ptr<TCP::Server<uint32_t>> server;

        std::mutex knownPlayerConnectionsMutex;
        std::map<long long, std::shared_ptr<PlayerConnection> > knownPlayerConnections;

        std::mutex pendingConnectionsMutex;
        std::vector<PlayerConnection> pendingConnections;
    private:
        void applyHandlersToConnection(std::shared_ptr<PlayerConnection> pconn);

        void handler_acceptConnection(ConnectionPtr& conn);
        void handler_stopConnection(std::shared_ptr<PlayerConnection> pconn, boost::system::error_code& err);

        bool validatePlayerConn(std::shared_ptr<PlayerConnection> pconn, const PlayerConnectionMessage& connDetails);

        void handler_PlayerConnectionMessage(std::shared_ptr<PlayerConnection> pconn, const json& j);
        void handler_RoleLoginMessage(std::shared_ptr<PlayerConnection> pconn, const json& j);
        void handler_ExecuteCommandMessage(std::shared_ptr<PlayerConnection> pconn, const json& j);
    public:
        Server() {}

        void start(short port);

        void forAllKnownConnections(std::function<void(Server*, std::shared_ptr<PlayerConnection>)> f);

        void sendGameBalanceDetailsMessage(std::shared_ptr<PlayerConnection> pconn,
            const GameBalance::ReplicatedSettings replicatedSettings,
            const GameBalance::Items::ItemsConfig& itemProperties,
            const GameBalance::Items::DeviceValuesConfig& deviceValueProperties,
            const GameBalance::Classes::ClassesConfig& classProperties,
            const GameBalance::Vehicles::VehiclesConfig& vehicleProperties,
            const GameBalance::VehicleWeapons::VehicleWeaponsConfig& vehicleWeaponProperties
        );

        void sendStateUpdateMessage(std::shared_ptr<PlayerConnection> pconn, float playerPing);

        void sendMessageToClient(std::shared_ptr<PlayerConnection> pconn, std::vector<MessageToClientMessage::ConsoleMsgDetails>& consoleMessages, MessageToClientMessage::IngameMsgDetails& ingameMessage);

        bool isPlayerAKnownModdedConnection(long long playerId);
        
        std::shared_ptr<PlayerConnection> getPlayerConnection(long long playerId);
    };

}

extern DCServer::Server g_DCServer;