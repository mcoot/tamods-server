#pragma once

#include <map>
#include <tuple>
#include <vector>

#define CONST_PLAYER_ASSISTS    0x0033
#define CONST_UNKNOWN_0095      0x0095
#define CONST_PLAYER_SCORE      0x00d2
#define CONST_PLAYER_DEATHS     0x0184
#define CONST_PLAYER_KILLS      0x028e
#define CONST_MAP_ID            0x02b2
#define CONST_UNKNOWN_02B4      0x02b4
#define CONST_MAP_DURATION      0x02c2
#define CONST_MATCH_ID          0x02c4
#define CONST_PLAYER_ID         0x0348
#define CONST_UNKNOWN_046C      0x046c
#define CONST_UNKNOWN_04BE      0x04be
#define CONST_BONUS_XP          0x04c0
#define CONST_UNKNOWN_04C7      0x04c7
#define CONST_STARTING_XP       0x04cb
#define CONST_XP_BAR_LENGTH     0x0505
#define CONST_BE_SCORE          0x0596
#define CONST_DS_SCORE          0x0597
#define CONST_BASE_XP           0x0605
#define CONST_VIP_XP            0x0606

namespace MatchSummary
{
    void registerHooks();
    void unregisterHooks();

    enum class FieldType
    {
        INT32,
        INT64,
        FLOAT
    };

    union FieldData
    {
        unsigned int int32;
        unsigned long long int64;
        float float_;
    };

    static const std::map<int, FieldType> s_typeMap = {
        { 0x0033, FieldType::INT32 },
        { 0x0095, FieldType::INT32 },
        { 0x00d2, FieldType::INT32 },
        { 0x0184, FieldType::INT32 },
        { 0x028e, FieldType::INT32 },
        { 0x02b2, FieldType::INT32 },
        { 0x02b4, FieldType::INT32 },
        { 0x02c2, FieldType::INT32 },
        { 0x02c4, FieldType::INT32 },
        { 0x0348, FieldType::INT32 },
        { 0x046c, FieldType::INT64 },
        { 0x049b, FieldType::INT32 },
        { 0x04be, FieldType::INT32 },
        { 0x04c0, FieldType::INT32 },
        { 0x04c7, FieldType::INT32 },
        { 0x04cb, FieldType::INT32 },
        { 0x0505, FieldType::INT32 },
        { 0x0595, FieldType::INT32 },
        { 0x0596, FieldType::INT32 },
        { 0x0597, FieldType::INT32 },
        { 0x0605, FieldType::INT32 },
        { 0x0606, FieldType::INT32 },
    };

    class MatchStats
    {
    public:
        MatchStats(unsigned short id):
            mId(id)
        {
        }

        void addStatistic(int who, int what, float howMuch)
        {
            statistics.push_back(std::make_tuple(who, what, howMuch));
        }

        void addAccolade(int who, int what, float howMuch)
        {
            accolades.push_back(std::make_tuple(who, what, howMuch));
        }

        void setField(int fieldId, int value)
        {
            mFields[fieldId] = value;
        }

        size_t size()
        {
            size_t size = 4;
            for (auto item : dataMap)
            {
                switch (s_typeMap.at(item.first))
                {
                case FieldType::INT32:
                case FieldType::FLOAT:
                    size += 6;
                    break;
                case FieldType::INT64:
                    size += 10;
                    break;
                }
            }

            size += 4 + statistics.size() * (2 + sizeOfStat() * 6);
            size += 4 + accolades.size() * (2 + sizeOfStat() * 6);

            return size;
        }

        void toBytes(void* pBuffer)
        {
            pBuffer = serializeInt16(pBuffer, mId);
            pBuffer = serializeInt16(pBuffer, (unsigned short)dataMap.size() + 2);

            for (auto& item : dataMap)
            {
                pBuffer = serializeInt16(pBuffer, item.first);
                switch (s_typeMap.at(item.first))
                {
                case FieldType::INT32:
                    pBuffer = serializeInt32(pBuffer, item.second.int32);
                    break;
                case FieldType::INT64:
                    pBuffer = serializeInt64(pBuffer, item.second.int64);
                    break;
                case FieldType::FLOAT:
                    pBuffer = serializeFloat(pBuffer, item.second.float_);
                    break;
                }
            }

            pBuffer = serializeInt16(pBuffer, 0x0170);
            pBuffer = serializeInt16(pBuffer, (unsigned short)statistics.size());
            for (auto& item : statistics)
            {
                pBuffer = serializeStat(pBuffer,
                    std::get<0>(item),
                    std::get<1>(item),
                    std::get<2>(item));
            }

            pBuffer = serializeInt16(pBuffer, 0x016f);
            pBuffer = serializeInt16(pBuffer, (unsigned short)accolades.size());
            for (auto& item : accolades)
            {
                pBuffer = serializeStat(pBuffer,
                    std::get<0>(item),
                    std::get<1>(item),
                    std::get<2>(item));
            }
        }

        void addInt32(int id, unsigned int value)
        {
            //assert(s_typeMap.at(id) == FieldType::INT32);
            dataMap[id].int64 = value;
        }

        void addInt64(int id, unsigned int value)
        {
            //assert(s_typeMap.at(id) == FieldType::INT64);
            dataMap[id].int64 = value;
        }

        void addFloat(int id, float value)
        {
            //assert(s_typeMap.at(id) == FieldType::FLOAT);
            dataMap[id].float_ = value;
        }

    protected:
        void* serializeInt16(void* pBuffer, unsigned short value)
        {
            unsigned short* pInt16Buffer = static_cast<unsigned short*>(pBuffer);
            *pInt16Buffer++ = value;
            return static_cast<void*>(pInt16Buffer);
        }

        void* serializeInt32(void* pBuffer, unsigned int value)
        {
            unsigned int* pInt32Buffer = static_cast<unsigned int*>(pBuffer);
            *pInt32Buffer++ = value;
            return static_cast<void*>(pInt32Buffer);
        }

        void* serializeInt64(void* pBuffer, unsigned long long value)
        {
            unsigned long long* pInt64Buffer = static_cast<unsigned long long*>(pBuffer);
            *pInt64Buffer++ = value;
            return static_cast<void*>(pInt64Buffer);
        }

        void* serializeFloat(void* pBuffer, float value)
        {
            float* pFloatBuffer = static_cast<float*>(pBuffer);
            *pFloatBuffer++ = value;
            return static_cast<void*>(pFloatBuffer);
        }

        virtual size_t sizeOfStat() = 0;
        virtual void* serializeStat(void*, int who, int what, float howMuch) = 0;

    private:
        unsigned short mId;
        std::map<int, FieldData> dataMap;
        std::vector<std::tuple<int, int, float>> statistics;
        std::vector<std::tuple<int, int, float>> accolades;
        std::map<int, int> mFields;
    };

    class OverallMatchStats : public MatchStats
    {
    public:
        OverallMatchStats():
            MatchStats(0x008a)
        {
        }

    private:
        size_t sizeOfStat() override
        {
            return 3;
        }

        void* serializeStat(void* pBuffer, int who, int what, float howMuch) override
        {
            pBuffer = serializeInt16(pBuffer, 3);
            pBuffer = serializeInt16(pBuffer, 0x0595);
            pBuffer = serializeInt32(pBuffer, what);
            pBuffer = serializeInt16(pBuffer, 0x049b);
            pBuffer = serializeFloat(pBuffer, howMuch);
            pBuffer = serializeInt16(pBuffer, 0x0348);
            pBuffer = serializeInt32(pBuffer, who);
            return pBuffer;
        }
    };

    class PlayerMatchStats : public MatchStats
    {
    public:
        PlayerMatchStats():
            MatchStats(0x008b)
        {
        }

    private:
        size_t sizeOfStat() override
        {
            return 2;
        }

        void* serializeStat(void* pBuffer, int who, int what, float howMuch) override
        {
            pBuffer = serializeInt16(pBuffer, 2);
            pBuffer = serializeInt16(pBuffer, 0x0595);
            pBuffer = serializeInt32(pBuffer, what);
            pBuffer = serializeInt16(pBuffer, 0x049b);
            pBuffer = serializeFloat(pBuffer, howMuch);
            return pBuffer;
        }
    };

    void memMoveBits(unsigned char* pWriteBuffer, int fromOffsetInBits, int toOffsetInBits, int bitsToMove);

    class StatsCollector
    {
    public:
        void addAccolade(int who, int accoladeId);
        void updateStat(int who, int statId, float newValue);
        void setField(int who, int fieldId, int value);
        void getSummary(int thisPlayerId, PlayerMatchStats &playerStats, OverallMatchStats &overallStats);

    private:
        float GetMinTierValue(int statId);
        float GetTieredWeight(int statId, float val);

        std::map<std::pair<int, int>, float> mAccolades;
        std::map<std::pair<int, int>, float> mStats;
        std::map<std::pair<int, int>, int> mFields;
    };

    extern StatsCollector sStatsCollector;

};
