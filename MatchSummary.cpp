#include <limits>
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

    struct AwardData
    {
        int tierlimit[3];
        int weight;
    };

    // Get rid of the max macro from some Windows header file
    #ifdef max
    #undef max
    #endif
    #define no_tiers {1, std::numeric_limits<int>::max(), std::numeric_limits<int>::max()}

    const int defaultWeight = 1;

    static const std::map<int,AwardData> cStatValueMap = {
        { CONST_STAT_AWD_CREDITS_EARNED              , {{1, 10000, 20000}, 95} },
        { CONST_STAT_AWD_CREDITS_SPENT               , {{1, 5000, 15000}, 89} },
        { CONST_STAT_AWD_DESTRUCTION_DEPLOYABLE      , {{1, 3, 10}, 84} },
        { CONST_STAT_AWD_DESTRUCTION_VEHICLE         , {{1, 2, 4}, 88} },
        { CONST_STAT_AWD_DISTANCE_HEADSHOT           , {{1, 300, 600}, 87} },
        { CONST_STAT_AWD_DISTANCE_KILL               , {{1, 400, 1000}, 93} },
        { CONST_STAT_AWD_DISTANCE_SKIED              , {{1, 40000, 100000}, 90} },
        { CONST_STAT_AWD_KILLS                       , {{1, 6, 15}, 86} },
        { CONST_STAT_AWD_KILLS_DEPLOYABLE            , {{1, 3, 8}, 92} },
        { CONST_STAT_AWD_KILLS_MIDAIR                , {{1, 3, 8}, 94} },
        { CONST_STAT_AWD_KILLS_VEHICLE               , {{1, 4, 10}, 81} },
        { CONST_STAT_AWD_REPAIRS                     , {{1, 8, 40}, 96} },
        { CONST_STAT_AWD_SPEED_FLAGCAP               , {{-99999, -60, -25}, -100} },
        { CONST_STAT_AWD_SPEED_FLAGGRAB              , {{1, 100, 200}, 97} },
        { CONST_STAT_AWD_SPEED_SKIED                 , {{1, 200, 260}, 95} },
        { CONST_STAT_AWD_FLAG_RETURNS                , {{1, 3, 7}, 85} },
        { CONST_STAT_AWD_DEATHS                      , {{1, 6, 15}, 1} },
        { CONST_STAT_ACO_KS_FIVE                     , {no_tiers, CONST_STAT_ACOW_KS_FIVE               } },
        { CONST_STAT_ACO_KS_TEN                      , {no_tiers, CONST_STAT_ACOW_KS_TEN                } },
        { CONST_STAT_ACO_KS_FIFTEEN                  , {no_tiers, CONST_STAT_ACOW_KS_FIFTEEN            } },
        { CONST_STAT_ACO_KS_TWENTY                   , {no_tiers, CONST_STAT_ACOW_KS_TWENTY             } },
        { CONST_STAT_ACO_KS_TWENTYFIVE               , {no_tiers, CONST_STAT_ACOW_KS_TWENTYFIVE         } },
        { CONST_STAT_ACO_KS_FIVE_SNIPING             , {no_tiers, CONST_STAT_ACOW_KS_FIVE_SNIPING       } },
        { CONST_STAT_ACO_KS_TEN_SNIPING              , {no_tiers, CONST_STAT_ACOW_KS_TEN_SNIPING        } },
        { CONST_STAT_ACO_KS_FIFTEEN_SNIPING          , {no_tiers, CONST_STAT_ACOW_KS_FIFTEEN_SNIPING    } },
        { CONST_STAT_ACO_KS_FIVE_EXPLOSIVE           , {no_tiers, CONST_STAT_ACOW_KS_FIVE_EXPLOSIVE     } },
        { CONST_STAT_ACO_KS_TEN_EXPLOSIVE            , {no_tiers, CONST_STAT_ACOW_KS_TEN_EXPLOSIVE      } },
        { CONST_STAT_ACO_KS_FIFTEEN_EXPLOSIVE        , {no_tiers, CONST_STAT_ACOW_KS_FIFTEEN_EXPLOSIVE  } },
        { CONST_STAT_ACO_KS_FIVE_SPINFUSOR           , {no_tiers, CONST_STAT_ACOW_KS_FIVE_SPINFUSOR     } },
        { CONST_STAT_ACO_KS_TEN_SPINFUSOR            , {no_tiers, CONST_STAT_ACOW_KS_TEN_SPINFUSOR      } },
        { CONST_STAT_ACO_KS_FIFTEEN_SPINFUSOR        , {no_tiers, CONST_STAT_ACOW_KS_FIFTEEN_SPINFUSOR  } },
        { CONST_STAT_ACO_MK_DOUBLE                   , {no_tiers, CONST_STAT_ACOW_MK_DOUBLE             } },
        { CONST_STAT_ACO_MK_TRIPLE                   , {no_tiers, CONST_STAT_ACOW_MK_TRIPLE             } },
        { CONST_STAT_ACO_MK_QUATRA                   , {no_tiers, CONST_STAT_ACOW_MK_QUATRA             } },
        { CONST_STAT_ACO_MK_ULTRA                    , {no_tiers, CONST_STAT_ACOW_MK_ULTRA              } },
        { CONST_STAT_ACO_MK_TEAM                     , {no_tiers, CONST_STAT_ACOW_MK_TEAM               } },
        { CONST_STAT_ACO_NOJOY                       , {no_tiers, CONST_STAT_ACOW_NOJOY                 } },
        { CONST_STAT_ACO_REVENGE                     , {no_tiers, CONST_STAT_ACOW_REVENGE               } },
        { CONST_STAT_ACO_AFTERMATH                   , {no_tiers, CONST_STAT_ACOW_AFTERMATH             } },
        { CONST_STAT_ACO_FIRSTBLOOD                  , {no_tiers, CONST_STAT_ACOW_FIRSTBLOOD            } },
        { CONST_STAT_ACO_BLUEPLATESPECIAL            , {no_tiers, CONST_STAT_ACOW_BLUEPLATESPECIAL      } },
        { CONST_STAT_ACO_STICKYKILL                  , {no_tiers, CONST_STAT_ACOW_STICKYKILL            } },
        { CONST_STAT_ACO_HEADSHOT                    , {no_tiers, CONST_STAT_ACOW_HEADSHOT              } },
        { CONST_STAT_ACO_ARTILLERYSTRIKE             , {no_tiers, CONST_STAT_ACOW_ARTILLERYSTRIKE       } },
        { CONST_STAT_ACO_MELEE                       , {no_tiers, CONST_STAT_ACOW_MELEE                 } },
        { CONST_STAT_ACO_ROADKILL                    , {no_tiers, CONST_STAT_ACOW_ROADKILL              } },
        { CONST_STAT_ACO_FLAG_CAPTURE                , {no_tiers, CONST_STAT_ACOW_FLAG_CAPTURE          } },
        { CONST_STAT_ACO_FLAG_GRAB                   , {no_tiers, CONST_STAT_ACOW_FLAG_GRAB             } },
        { CONST_STAT_ACO_BK_GEN                      , {no_tiers, CONST_STAT_ACOW_BK_GEN                } },
        { CONST_STAT_ACO_RABBITKILL                  , {no_tiers, CONST_STAT_ACOW_RABBITKILL            } },
        { CONST_STAT_ACO_KILLASRABBIT                , {no_tiers, CONST_STAT_ACOW_KILLASRABBIT          } },
        { CONST_STAT_ACO_FINALBLOW                   , {no_tiers, CONST_STAT_ACOW_FINALBLOW             } },
        { CONST_STAT_ACO_REPAIR                      , {no_tiers, CONST_STAT_ACOW_REPAIR                } },
        { CONST_STAT_ACO_BK_TURRET                   , {no_tiers, CONST_STAT_ACOW_BK_TURRET             } },
        { CONST_STAT_ACO_ASSIST                      , {no_tiers, CONST_STAT_ACOW_ASSIST                } },
        { CONST_STAT_ACO_FLAG_RETURN                 , {no_tiers, CONST_STAT_ACOW_FLAG_RETURN           } },
        { CONST_STAT_ACO_BK_RADAR                    , {no_tiers, CONST_STAT_ACOW_BK_RADAR              } },
        { CONST_STAT_ACO_FLAG_ASSIST                 , {no_tiers, CONST_STAT_ACOW_FLAG_ASSIST           } },
        { CONST_STAT_ACO_AIRMAIL                     , {no_tiers, CONST_STAT_ACOW_AIRMAIL               } },
        { CONST_STAT_ACO_GAME_COMPLETE               , {no_tiers, CONST_STAT_ACOW_GAME_COMPLETE         } },
        { CONST_STAT_ACO_GAME_WINNER                 , {no_tiers, CONST_STAT_ACOW_GAME_WINNER           } },
        { CONST_STAT_ACO_FLAG_GRABDM                 , {no_tiers, CONST_STAT_ACOW_FLAG_GRABDM           } },
        { CONST_STAT_ACO_FLAG_HOLDER                 , {no_tiers, CONST_STAT_ACOW_FLAG_HOLDER           } },
        { CONST_STAT_ACO_FLAG_KILLER                 , {no_tiers, CONST_STAT_ACOW_FLAG_KILLER           } },
        { CONST_STAT_ACO_FLAG_GRABFAST               , {no_tiers, CONST_STAT_ACOW_FLAG_GRABFAST         } },
        { CONST_STAT_ACO_DEFENSE_GEN                 , {no_tiers, CONST_STAT_ACOW_DEFENSE_GEN           } },
        { CONST_STAT_ACO_DEFENSE_FLAG                , {no_tiers, CONST_STAT_ACOW_DEFENSE_FLAG          } },
        { CONST_STAT_ACO_VD_BIKE                     , {no_tiers, CONST_STAT_ACOW_VD_BIKE               } },
        { CONST_STAT_ACO_VD_TANK                     , {no_tiers, CONST_STAT_ACOW_VD_TANK               } },
        { CONST_STAT_ACO_VD_SHRIKE                   , {no_tiers, CONST_STAT_ACOW_VD_SHRIKE             } },
        { CONST_STAT_ACO_FLAG_GRABE                  , {no_tiers, CONST_STAT_ACOW_FLAG_GRABE            } },
        { CONST_STAT_ACO_FLAG_GRABLLAMA              , {no_tiers, CONST_STAT_ACOW_FLAG_GRABLLAMA        } },
        { CONST_STAT_ACO_ASSIST_VEHICLE              , {no_tiers, CONST_STAT_ACOW_ASSIST_VEHICLE        } },
        { CONST_STAT_ACO_FLAG_GRABULTRA              , {no_tiers, CONST_STAT_ACOW_FLAG_GRABULTRA        } },
        { CONST_STAT_ACO_BENCHEM                     , {no_tiers, 2                                     } },
        { CONST_STAT_ACO_DOUBLEDOWN                  , {no_tiers, 20                                    } },
        { CONST_STAT_ACO_LASTMANSTANDING             , {no_tiers, 2                                     } },
        { CONST_STAT_ACO_MIRACLE                     , {no_tiers, 100                                   } },
        { CONST_STAT_ACO_NOTAMONGEQUALS              , {no_tiers, 40                                    } },
        { CONST_STAT_ACO_ONEMANARMY                  , {no_tiers, 60                                    } },
        { CONST_STAT_ACO_TRIBALHONOR                 , {no_tiers, 2                                     } },
        { CONST_STAT_ACO_UNITEDWESTAND               , {no_tiers, 10                                    } },
        { CONST_STAT_ACO_HOLDTHELINE                 , {no_tiers, 1                                     } },
        { CONST_STAT_ACO_CAPTUREANDHOLD              , {no_tiers, 2                                     } },
        { CONST_STAT_ACO_BASEASSIST                  , {no_tiers, CONST_STAT_ACOW_BASEASSIST            } },
        { CONST_STAT_ACO_TURRETASSIST                , {no_tiers, CONST_STAT_ACOW_TURRETASSIST          } },
        { CONST_STAT_ACO_HOTAIR                      , {no_tiers, CONST_STAT_ACOW_HOTAIR                } },
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
        clientData.maxNumberOfBits = std::max(clientData.maxNumberOfBits, writeOffsetInBits + bitsToCopy);
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

    void StatsCollector::addToStat(int who, int statId, float value)
    {
        auto key = std::make_pair(who, statId);
        auto it = mStats.find(key);
        if(it == mStats.end())
        {
            mStats[key] = value;
        }
        else
        {
            it->second += value;
        }
    }

    void StatsCollector::updateStat(int who, int statId, float value)
    {
        mStats[std::make_pair(who, statId)] = value;
    }

    void StatsCollector::setField(int who, int fieldId, int value)
    {
        mFields[std::make_pair(who, fieldId)] = value;
    }

    float StatsCollector::GetMinTierValue(int statId)
    {
        auto it = cStatValueMap.find(statId);
        return (it == cStatValueMap.end()) ? 0.0f : it->second.tierlimit[0];
    }

    float StatsCollector::GetTieredWeight(int statId, float val)
    {
        float retVal = 0;

        auto it = cStatValueMap.find(statId);
        if (it != cStatValueMap.end())
        {
            if (val > it->second.tierlimit[2])
            {
                retVal = val * it->second.weight * 10000;
            }
            else if (val > it->second.tierlimit[1])
            {
                retVal = val * it->second.weight * 100;
            }
            else
            {
                retVal = val * it->second.weight;
            }
        }
    
        return retVal;
    }

    void StatsCollector::getSummary(int thisPlayerId, PlayerMatchStats &playerStats, OverallMatchStats &overallStats)
    {
        int available_overall_stats_slots = 4;
        int available_overall_accolade_slots = 6;
        int available_player_stats_slots = 8;
        int available_player_accolade_slots = 21;

        auto sortFunc = [this](const std::pair<std::pair<int, int>, float> &stat1,
                               const std::pair<std::pair<int, int>, float> &stat2)
        {
            return GetTieredWeight(stat1.first.second, stat1.second) > GetTieredWeight(stat2.first.second, stat2.second);
        };

        std::vector<std::pair<std::pair<int, int>, float>> sortedAccolades(mAccolades.begin(), mAccolades.end());
        std::sort(sortedAccolades.begin(), sortedAccolades.end(), sortFunc);

        std::vector<std::pair<std::pair<int, int>, float>> sortedStats(mStats.begin(), mStats.end());
        std::sort(sortedStats.begin(), sortedStats.end(), sortFunc);

#ifdef DEBUG
        Logger::error("Logging ordered accolades:");
        for(auto &stat: sortedAccolades)
        {
            Logger::error("  %d x %d for player %d with value %f", stat.first.second, (int)stat.second, stat.first.first, GetTieredWeight(stat.first.second, stat.second));
        }
        Logger::error("Logging ordered stats:");
        for(auto &stat: sortedStats)
        {
            Logger::error("  %d x %d for player %d with value %f", stat.first.second, (int)stat.second, stat.first.first, GetTieredWeight(stat.first.second, stat.second));
        }
#endif

        std::set<int> alreadyAddedStats;
        for(const auto &kv: sortedStats)
        {
            int statPlayerId = kv.first.first;
            int stat = kv.first.second;
            float value = kv.second;

            static const std::map<int,int> cStatIdToFieldId = {
                { CONST_STAT_AWD_KILLS, CONST_PLAYER_KILLS },
                { CONST_STAT_AWD_DEATHS, CONST_PLAYER_DEATHS },
                { CONST_STAT_CLASS_ASSISTS, CONST_PLAYER_ASSISTS },
                { CONST_STAT_AWD_CREDITS_EARNED, CONST_PLAYER_SCORE },
            };

            if(thisPlayerId == statPlayerId &&
               cStatIdToFieldId.count(stat) != 0)
            {
                setField(thisPlayerId, cStatIdToFieldId.at(stat), (int)value);
            }

            if(value < GetMinTierValue(stat))
            {
                continue;
            }

            if(available_overall_stats_slots > 0 && alreadyAddedStats.count(stat) == 0)
            {
                overallStats.addStatistic(statPlayerId, stat, value);
                available_overall_stats_slots--;
                alreadyAddedStats.insert(stat);
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
    }

}
