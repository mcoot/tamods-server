#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include "../MatchSummary.h"

namespace Logger
{
    void error(char const *, ...)
    {
    }
}

BOOST_AUTO_TEST_CASE(MatchStatsTest)
{
    char buffer[1024];
    MatchSummary::OverallMatchStats overallStats;

    unsigned int initialSize = 4 + 7 * (2 + 4) + 1 * (2 + 8) + 2 * (2 + 2);

    BOOST_CHECK_EQUAL(overallStats.size(), initialSize);

    overallStats.addStatistic(0, 0, 0.0);
    BOOST_CHECK_EQUAL(overallStats.size(), initialSize + 2 + 3 * (2 + 4));

    overallStats.toBytes(buffer);
}


class Buffer
{
public:
    Buffer(const std::string& bits)
    {
        int bitIndex = 0;
        for (auto& bit : bits)
        {
            if (bit == ' ') continue;

            if ((int)mData.size() <= bitIndex / 8)
            {
                mData.push_back(0);
            }
            if (bit == '1')
            {
                mData[bitIndex / 8] |= 1 << (bitIndex % 8);
            }
            bitIndex++;
        }

        BOOST_ASSERT(bitIndex % 8 == 0);
    }

    unsigned char* data()
    {
        return mData.data();
    }

    bool operator==(const Buffer& other) const
    {
        return mData == other.mData;
    }

    const std::string str() const
    {
        std::string repr;
        int byteIndex = 0;
        for (const auto& byte : mData)
        {
            if (byteIndex != 0)
            {
                repr += ' ';
            }
            for (int bitIndex = 0; bitIndex < 8; bitIndex++)
            {
                if (byte & (1 << bitIndex))
                {
                    repr += '1';
                }
                else
                {
                    repr += '0';
                }
            }
            byteIndex++;
        }
        return repr;
    }

private:
    std::vector<unsigned char> mData;
};

std::ostream& operator<<(std::ostream& stream, const Buffer& buffer)
{
    stream << buffer.str();
    return stream;
}

struct MoveTestData
{
    std::string inputBits;
    int shiftFrom;
    int shiftTo;
    int bitCount;
    std::string outputBits;
};

std::ostream& operator<<(std::ostream& stream, const MoveTestData& moveTestData)
{
    stream << "{\"" << moveTestData.inputBits << "\", " << moveTestData.shiftFrom << ", " << moveTestData.shiftTo << ", " << moveTestData.bitCount << ", \"" << moveTestData.outputBits << "\"}";
    return stream;
}

MoveTestData arr[] = {
    {"00101000 00000000 00000000", 3,  8, 3, "00100000 01000000 00000000"},
    {"00101000 00000000 00000000", 3,  9, 3, "00100000 00100000 00000000"},
    {"00101000 00000000 00000000", 3, 11, 3, "00100000 00001000 00000000"},
    {"00101000 00000000 00000000", 3, 13, 3, "00100000 00000010 00000000"},
    {"00101000 00000000 00000000", 3, 14, 3, "00100000 00000001 00000000"},
    {"00000000 01000000 00000000", 9, 17, 1, "00000000 00000000 01000000"},
    {"00000000 11110101 00000000", 8, 16, 8, "00000000 00000000 11110101"},
    {"00001111 01010000 00000000", 4, 12, 8, "00000000 00001111 01010000"},
};

BOOST_DATA_TEST_CASE(MemMoveTest, boost::unit_test::data::make(arr), params)
{
    Buffer buffer(params.inputBits);
    MatchSummary::memMoveBits(buffer.data(), params.shiftFrom, params.shiftTo, params.bitCount);
    BOOST_CHECK_EQUAL(buffer, Buffer(params.outputBits));
}

