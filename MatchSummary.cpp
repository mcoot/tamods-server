#include "Mods.h"
#include "MatchSummary.h"

#include <boost/dynamic_bitset.hpp>
#include <windows.h>

#include "detours.h"

#ifdef _DEBUG
namespace Logger
{
    extern void error(const char *format, ...);
}
#endif

#pragma warning (disable : 4996)


namespace MatchSummary
{
    static const std::map<int,int> cStatValueMap = {
        { CONST_STAT_ACO_KS_FIVE                     , 300 },
        { CONST_STAT_ACO_KS_TEN                      , 500 },
        { CONST_STAT_ACO_KS_FIFTEEN                  , 1000 },
        { CONST_STAT_ACO_KS_TWENTY                   , 2000 },
        { CONST_STAT_ACO_KS_TWENTYFIVE               , 3000 },
        { CONST_STAT_ACO_KS_FIVE_SNIPING             , 300 },
        { CONST_STAT_ACO_KS_TEN_SNIPING              , 500 },
        { CONST_STAT_ACO_KS_FIFTEEN_SNIPING          , 1000 },
        { CONST_STAT_ACO_KS_FIVE_EXPLOSIVE           , 300 },
        { CONST_STAT_ACO_KS_TEN_EXPLOSIVE            , 500 },
        { CONST_STAT_ACO_KS_FIFTEEN_EXPLOSIVE        , 1000 },
        { CONST_STAT_ACO_KS_FIVE_SPINFUSOR           , 300 },
        { CONST_STAT_ACO_KS_TEN_SPINFUSOR            , 500 },
        { CONST_STAT_ACO_KS_FIFTEEN_SPINFUSOR        , 1000 },
        { CONST_STAT_ACO_MK_DOUBLE                   , 500 },
        { CONST_STAT_ACO_MK_TRIPLE                   , 1000 },
        { CONST_STAT_ACO_MK_QUATRA                   , 2000 },
        { CONST_STAT_ACO_MK_ULTRA                    , 3000 },
        { CONST_STAT_ACO_MK_TEAM                     , 4000 },
        { CONST_STAT_ACO_NOJOY                       , 300 },
        { CONST_STAT_ACO_REVENGE                     , 200 },
        { CONST_STAT_ACO_AFTERMATH                   , 200 },
        { CONST_STAT_ACO_FIRSTBLOOD                  , 500 },
        { CONST_STAT_ACO_BLUEPLATESPECIAL            , 200 },
        { CONST_STAT_ACO_STICKYKILL                  , 200 },
        { CONST_STAT_ACO_HEADSHOT                    , 200 },
        { CONST_STAT_ACO_ARTILLERYSTRIKE             , 500 },
        { CONST_STAT_ACO_MELEE                       , 200 },
        { CONST_STAT_ACO_ROADKILL                    , 100 },
        { CONST_STAT_ACO_FLAG_CAPTURE                , 2000 },
        { CONST_STAT_ACO_FLAG_GRAB                   , 200 },
        { CONST_STAT_ACO_BK_GEN                      , 500 },
        { CONST_STAT_ACO_RABBITKILL                  , 200 },
        { CONST_STAT_ACO_KILLASRABBIT                , 300 },
        { CONST_STAT_ACO_FINALBLOW                   , 500 },
        { CONST_STAT_ACO_REPAIR                      , 25 },
        { CONST_STAT_ACO_BK_TURRET                   , 200 },
        { CONST_STAT_ACO_ASSIST                      , 250 },
        { CONST_STAT_ACO_FLAG_RETURN                 , 250 },
        { CONST_STAT_ACO_BK_RADAR                    , 200 },
        { CONST_STAT_ACO_FLAG_ASSIST                 , 500 },
        { CONST_STAT_ACO_AIRMAIL                     , 200 },
        { CONST_STAT_ACO_GAME_COMPLETE               , 50 },
        { CONST_STAT_ACO_GAME_WINNER                 , 1200 },
        { CONST_STAT_ACO_FLAG_GRABDM                 , 250 },
        { CONST_STAT_ACO_FLAG_HOLDER                 , 100 },
        { CONST_STAT_ACO_FLAG_KILLER                 , 250 },
        { CONST_STAT_ACO_FLAG_GRABFAST               , 250 },
        { CONST_STAT_ACO_DEFENSE_GEN                 , 500 },
        { CONST_STAT_ACO_DEFENSE_FLAG                , 300 },
        { CONST_STAT_ACO_VD_BIKE                     , 200 },
        { CONST_STAT_ACO_VD_TANK                     , 500 },
        { CONST_STAT_ACO_VD_SHRIKE                   , 700 },
        { CONST_STAT_ACO_FLAG_GRABE                  , 500 },
        { CONST_STAT_ACO_FLAG_GRABLLAMA              , 150 },
        { CONST_STAT_ACO_ASSIST_VEHICLE              , 250 },
        { CONST_STAT_ACO_FLAG_GRABULTRA              , 300 },
        { CONST_STAT_ACO_BENCHEM                     , 200 },
        { CONST_STAT_ACO_DOUBLEDOWN                  , 2000 },
        { CONST_STAT_ACO_LASTMANSTANDING             , 200 },
        { CONST_STAT_ACO_MIRACLE                     , 10000 },
        { CONST_STAT_ACO_NOTAMONGEQUALS              , 4000 },
        { CONST_STAT_ACO_ONEMANARMY                  , 6000 },
        { CONST_STAT_ACO_TRIBALHONOR                 , 200 },
        { CONST_STAT_ACO_UNITEDWESTAND               , 1000 },
        { CONST_STAT_ACO_HOLDTHELINE                 , 100 },
        { CONST_STAT_ACO_CAPTUREANDHOLD              , 200 },
        { CONST_STAT_ACO_BASEASSIST                  , 99999 },
        { CONST_STAT_ACO_TURRETASSIST                , 99999 },
        { CONST_STAT_ACO_HOTAIR                      , 150 },
        { CONST_STAT_AWD_CREDITS_EARNED              , 99999 },
        { CONST_STAT_AWD_CREDITS_SPENT               , 99999 },
        { CONST_STAT_AWD_DESTRUCTION_DEPLOYABLE      , 99999 },
        { CONST_STAT_AWD_DESTRUCTION_VEHICLE         , 99999 },
        { CONST_STAT_AWD_DISTANCE_HEADSHOT           , 99999 },
        { CONST_STAT_AWD_DISTANCE_KILL               , 99999 },
        { CONST_STAT_AWD_DISTANCE_SKIED              , 99999 },
        { CONST_STAT_AWD_KILLS                       , 99999 },
        { CONST_STAT_AWD_KILLS_DEPLOYABLE            , 99999 },
        { CONST_STAT_AWD_KILLS_MIDAIR                , 99999 },
        { CONST_STAT_AWD_KILLS_VEHICLE               , 99999 },
        { CONST_STAT_AWD_REPAIRS                     , 99999 },
        { CONST_STAT_AWD_SPEED_FLAGCAP               , 99999 },
        { CONST_STAT_AWD_SPEED_FLAGGRAB              , 99999 },
        { CONST_STAT_AWD_SPEED_SKIED                 , 99999 },
        { CONST_STAT_AWD_FLAG_RETURNS                , 99999 },
        { CONST_STAT_AWD_DEATHS                      , 99999 },
    };

    StatsCollector sStatsCollector;

    typedef void(__thiscall* EndGameFunction)(void*, int);
    typedef int(__thiscall* Level6Function)(void*, int, int);
    typedef void(__cdecl* Level8Function)(void*, int, void*, int, int);
    typedef int (__stdcall *SendtoFunction)(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen);

    struct ClientData
    {
        ClientData() :
            seqnr(0),
            previousWriteOffset(0),
            fragmentInserted(false),
            insertionOffset(0),
            insertionCount(0),
            fragmentSent(false),
            maxNumberOfBits(0)
        {
            OutputDebugString(L"tamods: New client data created\n");
        }

        unsigned int seqnr;
        int previousWriteOffset;
        bool fragmentInserted;
        int insertionOffset;
        int insertionCount;
        bool fragmentSent;
        int maxNumberOfBits;
    };

    static EndGameFunction realEndGameFunction = 0;
    static Level6Function realLevel6Function = 0;
    static Level8Function realLevel8Function = 0;
    static SendtoFunction realSendtoFunction = 0;

    static int gameEnded = 0;
    static bool firstLevel8SinceLevel6 = false;
    static std::map<void*, ClientData> clientMap;
    static std::map<char *, unsigned int> bufferToPlayerId;

    void __fastcall fakeEndGameFunction(void* pThis, void*, int a)
    {
        OutputDebugString(L"tamods: fakeEndGameFunction\n");
        gameEnded++;
        realEndGameFunction(pThis, a);
    }

    int __fastcall fakeLevel6Function(void* pThis, void*, int a, int b)
    {
        //OutputDebugString(L"tamods: fakeLevel6Function\n");
        firstLevel8SinceLevel6 = true;
        unsigned int playerId = *((unsigned int*)pThis + 0xb8 / sizeof(unsigned int));
        char *pSendBuffer = *((char **)pThis + 0x2e8 / sizeof(char *));

#ifdef _DEBUG
        auto it = bufferToPlayerId.find(pSendBuffer);
        if(it != bufferToPlayerId.end() && it->second != playerId)
        {
            Logger::error("First player %08X and then player 0x%X was using buffer %08X", it->second, playerId, pSendBuffer);
        }
#endif
        bufferToPlayerId[pSendBuffer] = playerId;

        return realLevel6Function(pThis, a, b);
    }

    void writeBits(FILE* pFile, unsigned char* buffer, unsigned int numberOfBits)
    {
        int bitIndex = 0;
        int byteIndex = 0;
        fprintf(pFile, "bits: ");
        for (unsigned int i = 0; i < numberOfBits; i++)
        {
            char bit = ((buffer[byteIndex] >> bitIndex) & 1) ? '1' : '0';
            fwrite(&bit, 1, 1, pFile);
            bitIndex++;
            if (bitIndex == 8)
            {
                bitIndex = 0;
                byteIndex++;
            }
        }
        fprintf(pFile, "\n");
    }

    void __cdecl fakeLevel8Function(void* pWriteBuffer, int writeOffsetInBits, void* pReadBuffer, int readOffsetInBits, int bitsToCopy)
    {
        //OutputDebugString(L"tamods: fakeLevel8Function\n");
        bool modifyEdi = false;

        ClientData& clientData = clientMap[pWriteBuffer];

        // Each first level8 updates the bit count, each second level8 writes all the bits after the count.
        if (firstLevel8SinceLevel6)
        {
            firstLevel8SinceLevel6 = false;

            // Track sequence number of messages on channel 0
            unsigned int channel0data = *(unsigned int*)pReadBuffer;
            if ((channel0data & 0x00001ff8) == 0)
            {
                clientData.seqnr = (channel0data >> 13) & 0x1f;
            }

            if (gameEnded >= 2 && !clientData.fragmentInserted)
            {
                // Track writes to buffers and insert the fragment when possible

                if (clientData.previousWriteOffset == 0)
                {
                    clientData.previousWriteOffset = writeOffsetInBits;
                }
                else
                {
                    // If the offset for the first level8 changes, it means that this would be the 
                    // first write for a new piece of data. That is when we can insert our own data.
                    if (writeOffsetInBits != clientData.previousWriteOffset)
                    {
                        clientData.fragmentInserted = true;
                        clientData.insertionOffset = writeOffsetInBits;
                    }
                }

            }
        }

        realLevel8Function(pWriteBuffer, writeOffsetInBits, pReadBuffer, readOffsetInBits, bitsToCopy);
        clientData.maxNumberOfBits = max(clientData.maxNumberOfBits, writeOffsetInBits + bitsToCopy);
    }

    void memMoveBits(unsigned char* pWriteBuffer, int fromOffsetInBits, int toOffsetInBits, int bitsToMove)
    {
        int fromByteIndex = fromOffsetInBits / 8;
        int fromBitIndex = fromOffsetInBits - fromByteIndex * 8;
        int lastToByteIndex = (toOffsetInBits + bitsToMove - 1) / 8;

        boost::dynamic_bitset<unsigned char> movedBits;
        movedBits.append(&pWriteBuffer[fromByteIndex], &pWriteBuffer[lastToByteIndex + 1]);

        boost::dynamic_bitset<unsigned char> beginning(movedBits.size());
        for (int i = 0; i < fromBitIndex; i++)
        {
            beginning[i] = movedBits[i];
            movedBits[i] = 0;
        }
        movedBits <<= (toOffsetInBits - fromOffsetInBits);
        movedBits |= beginning;

        std::vector<unsigned char> result(&pWriteBuffer[fromByteIndex], &pWriteBuffer[lastToByteIndex + 1]);
        boost::to_block_range(movedBits, result.begin());
        memcpy(&pWriteBuffer[fromByteIndex], result.data(), result.size());
    }


    int __stdcall fakeSendtoFunction(SOCKET s, char* buf, int len, int flags, const sockaddr* to, int tolen)
    {
        ClientData& clientData = clientMap[(void *)buf];
        if (clientData.fragmentInserted && !clientData.fragmentSent)
        {
            OverallMatchStats overallStats;
            PlayerMatchStats playerStats;

            if(bufferToPlayerId.count(buf) == 0)
            {
                Logger::error("Unable to find player ID for send buffer %08p", buf);
            }
            sStatsCollector.getSummary(bufferToPlayerId.at(buf), playerStats, overallStats);

#ifdef _DEBUG
            Logger::error("logging match summary stuff for player 0x%X", bufferToPlayerId.at(buf));
#endif

            char pBuffer[4096];

            // 0 (flag1 = 0)
            //  01 (flag1a = 2)
            //    0000000000 (channel = 0)
            unsigned int part1 = 0x00000004;
            unsigned int part1size = 13;

            //              01101 (counter = 22)
            unsigned int counter = clientData.seqnr + 1;
            unsigned int countersize = 5;

            //                   00000100
            unsigned int part2 = 0x00000020;
            unsigned int part2size = 8;

            //                                         10111000 (property = interestingproperty)
            unsigned int prop = 0x0000001d;
            unsigned int propsize = 8;

            unsigned int overallStatsSize = overallStats.size() * 8;
            unsigned int playerStatsSize = playerStats.size() * 8;

            //                                      0000011100000000000000001110 (prefix)
            unsigned int prefix1 = (overallStatsSize / 8) | (overallStatsSize / 8) << 19;
            unsigned int prefix1size = 28;

            unsigned int prefix2 = (playerStatsSize / 8) | (playerStatsSize / 8) << 19;
            unsigned int prefix2size = 28;

            //                           0001111100110 (payloadsize = 3320)
            //                                        x (object = FirstServerObject_0)
            unsigned int payloadsize = propsize + prefix1size + overallStatsSize + 
                                       propsize + prefix2size + playerStatsSize;
            unsigned int payloadsizesize = (payloadsize > 3000) ? 13 : 14;

            clientData.insertionCount = part1size + countersize + part2size + payloadsizesize + payloadsize;

            memMoveBits((unsigned char *)buf, clientData.insertionOffset, clientData.insertionOffset + clientData.insertionCount, len * 8 - clientData.insertionOffset);

            len = (clientData.maxNumberOfBits + clientData.insertionCount) / 8 + 1;
            unsigned int writeOffset = clientData.insertionOffset;
            realLevel8Function(buf, writeOffset, &part1, 0, part1size);             writeOffset += part1size;

            realLevel8Function(buf, writeOffset, &counter, 0, countersize);         writeOffset += countersize;
            realLevel8Function(buf, writeOffset, &part2, 0, part2size);             writeOffset += part2size;
            realLevel8Function(buf, writeOffset, &payloadsize, 0, payloadsizesize); writeOffset += payloadsizesize;

            realLevel8Function(buf, writeOffset, &prop, 0, propsize);               writeOffset += propsize;
            realLevel8Function(buf, writeOffset, &prefix1, 0, prefix1size);         writeOffset += prefix1size;
            overallStats.toBytes(pBuffer);
            realLevel8Function(buf, writeOffset, &pBuffer, 0, overallStatsSize);    writeOffset += overallStatsSize;

            realLevel8Function(buf, writeOffset, &prop, 0, propsize);               writeOffset += propsize;
            realLevel8Function(buf, writeOffset, &prefix2, 0, prefix2size);         writeOffset += prefix2size;
            playerStats.toBytes(pBuffer);
            realLevel8Function(buf, writeOffset, &pBuffer, 0, playerStatsSize);     writeOffset += playerStatsSize;

            boost::dynamic_bitset<unsigned char> insertedbits;
            insertedbits.append(&buf[clientData.insertionOffset/8], &buf[writeOffset/8 + 1]);

#ifdef _DEBUG
            static char bitstring[65535] = {0};
            for (unsigned int i = 0; i < insertedbits.size(); i++)
            {
                bitstring[i] = insertedbits[i] ? '1' : '0';
            }
            Logger::error("Insertedbits are more or less: %s", bitstring);
#endif

            clientData.fragmentSent = true;
            clientData.insertionOffset = 0;
            clientData.insertionCount = 0;
        }
        clientData.maxNumberOfBits = 0;

        return realSendtoFunction(s, buf, len, flags, to, tolen);
    }


    void registerHooks()
    {
        realEndGameFunction = (EndGameFunction)DetourFunction((PBYTE)0xcdb110, (PBYTE)fakeEndGameFunction);
        realLevel6Function = (Level6Function)DetourFunction((PBYTE)0xc1b160, (PBYTE)fakeLevel6Function);
        realLevel8Function = (Level8Function)DetourFunction((PBYTE)0x42baf0, (PBYTE)fakeLevel8Function);
        HMODULE handle = GetModuleHandleA("ws2_32.dll");
        void* pSendto = GetProcAddress(handle, "sendto");
        realSendtoFunction = (SendtoFunction)DetourFunction((PBYTE)pSendto, (PBYTE)fakeSendtoFunction);
    }

    void unregisterHooks()
    {
        DetourRemove((PBYTE)realEndGameFunction, (PBYTE)&fakeEndGameFunction);
        DetourRemove((PBYTE)realLevel6Function, (PBYTE)&fakeLevel6Function);
        DetourRemove((PBYTE)realLevel8Function, (PBYTE)&fakeLevel8Function);
        DetourRemove((PBYTE)realSendtoFunction, (PBYTE)&fakeSendtoFunction);
    }


    void StatsCollector::addAccolade(int who, int accoladeId)
    {
        auto key = std::make_pair(who, accoladeId);
        auto it = mAccolades.find(key);
        if(it == mAccolades.end())
        {
            mAccolades[key] = 1;
        }
        else
        {
            it->second += 1;
        }
    }

    void StatsCollector::updateStat(int who, int statId, float newValue)
    {
        mStats[std::make_pair(who, statId)] = newValue;
    }

    void StatsCollector::setField(int who, int fieldId, int value)
    {
        mFields[std::make_pair(who, fieldId)] = value;
    }

    void StatsCollector::getSummary(int thisPlayerId, PlayerMatchStats &playerStats, OverallMatchStats &overallStats)
    {
        int available_overall_stats_slots = 4;
        int available_overall_accolade_slots = 6;
        int available_player_stats_slots = 8;
        int available_player_accolade_slots = 21;

        auto sortFunc = [](const std::pair<std::pair<int, int>, float> &stat1,
                           const std::pair<std::pair<int, int>, float> &stat2)
        {
            auto it = cStatValueMap.find(stat1.first.second);
            int value1 = (it != cStatValueMap.end()) ? it->second : 20000;
            float count1 = stat1.second;

            it = cStatValueMap.find(stat2.first.second);
            int value2 = (it != cStatValueMap.end()) ? it->second : 20000;
            float count2 = stat2.second;

            return value1 * count1 > value2 * count2;
        };

        std::vector<std::pair<std::pair<int, int>, float>> sortedAccolades(mAccolades.begin(), mAccolades.end());
        std::sort(sortedAccolades.begin(), sortedAccolades.end(), sortFunc);

        std::vector<std::pair<std::pair<int, int>, float>> sortedStats(mStats.begin(), mStats.end());
        std::sort(sortedStats.begin(), sortedStats.end(), sortFunc);

        const int overallFields[] =
        {
            CONST_UNKNOWN_04BE,
            CONST_MATCH_ID,
            CONST_MAP_DURATION,
            CONST_UNKNOWN_046C,
            CONST_UNKNOWN_02B4,
            CONST_MAP_ID,
            CONST_BE_SCORE,
            CONST_DS_SCORE
        };
        for(auto& fieldId : overallFields)
        {
            int value = 0;
            auto it = mFields.find(std::make_pair(0, fieldId));
            if(it != mFields.end())
            {
                value = it->second;
            }

            switch(s_typeMap.at(fieldId))
            {
            case FieldType::INT32:
                overallStats.addInt32(fieldId, value);
                break;
            case FieldType::INT64:
                overallStats.addInt64(fieldId, value);
                break;
            case FieldType::FLOAT:
                overallStats.addFloat(fieldId, (float)value);
                break;
            }
        }

        const int playerFields[] =
        {
            CONST_PLAYER_ID,
            CONST_PLAYER_KILLS,
            CONST_PLAYER_DEATHS,
            CONST_PLAYER_ASSISTS,
            CONST_PLAYER_SCORE,
            CONST_UNKNOWN_0095,
            CONST_STARTING_XP,
            CONST_UNKNOWN_04C7,
            CONST_BASE_XP,
            CONST_BONUS_XP,
            CONST_VIP_XP,
            CONST_XP_BAR_LENGTH,
        };
        for(auto& fieldId : playerFields)
        {
            int value = 0;
            auto it = mFields.find(std::make_pair(thisPlayerId, fieldId));
            if(it != mFields.end())
            {
                value = it->second;
            }

            switch(s_typeMap.at(fieldId))
            {
            case FieldType::INT32:
                playerStats.addInt32(fieldId, value);
                break;
            case FieldType::INT64:
                playerStats.addInt64(fieldId, value);
                break;
            case FieldType::FLOAT:
                playerStats.addFloat(fieldId, (float)value);
                break;
            }
        }


        std::set<int> alreadyAddedStats;
        for(const auto &kv: sortedStats)
        {
            int statPlayerId = kv.first.first;
            int stat = kv.first.second;
            float value = kv.second;

            if(value == 0.0)
            {
                continue;
            }

            if(available_overall_stats_slots > 0 && alreadyAddedStats.count(stat) == 0)
            {
                overallStats.addStatistic(statPlayerId, stat, value);
                available_overall_stats_slots--;
                alreadyAddedStats.insert(stat);

#ifdef _DEBUG
                Logger::error("MVP stat player 0x%X, stat %d, value %f", statPlayerId, stat, value);
#endif
            }
            if(thisPlayerId == statPlayerId)
            {
                if(available_player_stats_slots > 0)
                {
                    playerStats.addStatistic(statPlayerId, stat, value);
                    available_player_stats_slots--;
                }
            }
        }

        std::set<int> alreadyAddedAccolades;
        for(const auto &kv: sortedAccolades)
        {
            int accPlayerId = kv.first.first;
            int acc = kv.first.second;
            float value = kv.second;

            if(value == 0.0)
            {
                continue;
            }

            if(available_overall_accolade_slots > 0 && alreadyAddedAccolades.count(acc) == 0)
            {
                overallStats.addAccolade(accPlayerId, acc, value);
                available_overall_accolade_slots--;
                alreadyAddedAccolades.insert(acc);
            }
            if(thisPlayerId == accPlayerId)
            {
                if(available_player_accolade_slots > 0)
                {
                    playerStats.addAccolade(accPlayerId, acc, value);
                    available_player_accolade_slots--;
                }
            }
        }
    }

}
