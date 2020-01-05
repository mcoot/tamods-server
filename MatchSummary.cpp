#include "MatchSummary.h"

#include <boost/assert.hpp>
#include <boost/dynamic_bitset.hpp>
#include <iostream>
#include <map>
#include <vector>
#include <windows.h>

#include "detours.h"

#pragma warning (disable : 4996)


const char logFileName[] = "C:\\projects\\TribesProtocol\\taserver\\gameclient\\testdata\\matchends\\matchend.log";

namespace MatchSummary
{

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


    const unsigned int fragmentSizeInBits = 3359;
    unsigned char fragment[] = {
        0x04, 0x00, 0x80, 0xE0, 0xB3, 0x0E, 0x70, 0x00, 0x80, 0x53, 0x04, 0x50, 0x00, 0xF0, 0x25, 0xF8,
        0xFF, 0xFF, 0xFF, 0x27, 0x16, 0x00, 0x00, 0x00, 0x00, 0x10, 0x16, 0x70, 0x0C, 0x00, 0x00, 0x60,
        0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x15, 0x00, 0x00, 0x00, 0x00, 0x90,
        0x15, 0x08, 0x00, 0x00, 0x00, 0xB0, 0x2C, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x2C, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x0B, 0x28, 0x00, 0x18, 0x00, 0xA8, 0x2C, 0x08, 0xE7, 0x18, 0x00, 0xD8, 0x24, 0x00,
        0x00, 0xD0, 0x2B, 0x42, 0x1A, 0x78, 0x6E, 0x1D, 0x00, 0x18, 0x00, 0xA8, 0x2C, 0x78, 0xE7, 0x18,
        0x00, 0xD8, 0x24, 0x00, 0x00, 0xC0, 0x17, 0x42, 0x1A, 0x78, 0x6E, 0x1D, 0x00, 0x18, 0x00, 0xA8,
        0x2C, 0x60, 0xE7, 0x18, 0x00, 0xD8, 0x24, 0x00, 0x00, 0x70, 0x15, 0x42, 0x1A, 0xC8, 0x01, 0x1D,
        0x01, 0x18, 0x00, 0xA8, 0x2C, 0x38, 0xE7, 0x18, 0x00, 0xD8, 0x24, 0x00, 0x00, 0x70, 0x14, 0x42,
        0x1A, 0x78, 0x6E, 0x1D, 0x00, 0x18, 0x00, 0xA8, 0x2C, 0x70, 0xE7, 0x18, 0x00, 0xD8, 0x24, 0x00,
        0x00, 0x00, 0x00, 0x40, 0x1A, 0x78, 0x6E, 0x1D, 0x00, 0x78, 0x0B, 0x18, 0x00, 0x18, 0x00, 0xA8,
        0x2C, 0xE0, 0xC8, 0x18, 0x00, 0xD8, 0x24, 0x00, 0x00, 0x70, 0x15, 0x42, 0x1A, 0xC8, 0x01, 0x1D,
        0x01, 0x18, 0x00, 0xA8, 0x2C, 0xC0, 0xC8, 0x18, 0x00, 0xD8, 0x24, 0x00, 0x00, 0x00, 0x08, 0x42,
        0x1A, 0x78, 0x6E, 0x1D, 0x00, 0x18, 0x00, 0xA8, 0x2C, 0x20, 0xC9, 0x18, 0x00, 0xD8, 0x24, 0x00,
        0x00, 0x00, 0xFC, 0x41, 0x1A, 0x78, 0x6E, 0x1D, 0x00, 0xE8, 0xB0, 0x05, 0x80, 0xAD, 0x45, 0x00,
        0x07, 0x00, 0xA4, 0x81, 0x1C, 0xD0, 0x11, 0x00, 0x47, 0x01, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x00,
        0x00, 0x00, 0x00, 0x80, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69, 0x80, 0x3F, 0x04, 0x00, 0x80,
        0x4A, 0x80, 0x4E, 0x03, 0x00, 0x80, 0x65, 0x82, 0xDE, 0x38, 0x00, 0x80, 0x63, 0x82, 0x68, 0x00,
        0x00, 0x80, 0x02, 0x83, 0x68, 0x00, 0x00, 0x00, 0x60, 0x02, 0x1A, 0x00, 0x00, 0x00, 0x03, 0x03,
        0x41, 0x00, 0x00, 0x80, 0x82, 0x82, 0xC3, 0x00, 0x00, 0x00, 0xB8, 0x80, 0x02, 0x00, 0x01, 0x80,
        0xCA, 0x82, 0x70, 0x8E, 0x01, 0x80, 0x4D, 0x02, 0x00, 0xF8, 0x83, 0x22, 0x01, 0x80, 0xCA, 0x82,
        0x73, 0x8E, 0x01, 0x80, 0x4D, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0xCA, 0x02, 0x76, 0x8E,
        0x01, 0x80, 0x4D, 0x02, 0x00, 0x00, 0x57, 0x21, 0x01, 0x80, 0xCA, 0x02, 0x77, 0x8E, 0x01, 0x80,
        0x4D, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0xCA, 0x82, 0x77, 0x8E, 0x01, 0x80, 0x4D, 0x02,
        0x00, 0x00, 0x12, 0xA1, 0xB7, 0x00, 0x01, 0x00, 0x01, 0x80, 0xCA, 0x02, 0x8E, 0x8C, 0x01, 0x80,
        0x4D, 0x02, 0x00, 0x00, 0x57, 0x21, 0x01, 0x80, 0xCA, 0x02, 0x92, 0x8C, 0x01, 0x80, 0x4D, 0x02,
        0x00, 0x00, 0xC0, 0x9F
    };


    static EndGameFunction realEndGameFunction = 0;
    static Level6Function realLevel6Function = 0;
    static Level8Function realLevel8Function = 0;
    static SendtoFunction realSendtoFunction = 0;

    static int gameEnded = 0;
    static unsigned int currentPlayer = 0;
    static bool firstLevel8SinceLevel6 = false;
    static std::map<void*, ClientData> clientMap;

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
        currentPlayer = *((unsigned int*)pThis + 0xb8 / sizeof(unsigned int));
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

//#define DOPRINTS

    void __cdecl fakeLevel8Function(void* pWriteBuffer, int writeOffsetInBits, void* pReadBuffer, int readOffsetInBits, int bitsToCopy)
    {
        //OutputDebugString(L"tamods: fakeLevel8Function\n");
        bool modifyEdi = false;

#ifdef DOPRINTS
        FILE* pFile = fopen(logFileName, "at");
#endif

#ifdef DOPRINTS
        char text[1000];

        if (readOffsetInBits != 0)
        {
            fprintf(pFile, "tamods: readoffsetinbits was %d\n", readOffsetInBits);
        }
#endif

        ClientData& clientData = clientMap[pWriteBuffer];

        // Each first level8 updates the bit count, each second level8 writes all the bits after the count.
        if (firstLevel8SinceLevel6)
        {
            firstLevel8SinceLevel6 = false;

#ifdef DOPRINTS
            fprintf(pFile, "tamods: that was the first level8 since level6\n");
#endif

            // Track sequence number of messages on channel 0
            unsigned int channel0data = *(unsigned int*)pReadBuffer;
            if ((channel0data & 0x00001ff8) == 0)
            {
                clientData.seqnr = (channel0data >> 13) & 0x1f;
#ifdef DOPRINTS
                sprintf(text, "tamods: channel 0 seqnr = %lu\n", clientData.seqnr);
                OutputDebugStringA(text);
                fprintf(pFile, text);
#endif
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
#ifdef DOPRINTS
                        sprintf(text, "tamods: Sending fragment to client with buffer %p\n", pWriteBuffer);
                        OutputDebugStringA(text);
                        fprintf(pFile, text);
#endif


#ifdef DOPRINTS
                        sprintf(text, "tamods:   first player %d copy %08X bits from %p+%08X to %p+%08X\n", currentPlayer, fragmentSizeInBits, &fragment[0], 0, pWriteBuffer, writeOffsetInBits);
                        OutputDebugStringA(text);
                        fprintf(pFile, text);

                        writeBits(pFile, fragment, fragmentSizeInBits);
#endif

                        clientData.fragmentInserted = true;
                        clientData.insertionOffset = writeOffsetInBits;
                    }
                }

            }
        }

#ifdef DOPRINTS
        sprintf(text, "tamods:   player %d copy %08X bits from %p+%08X to %p+%08X\n", currentPlayer, bitsToCopy, pReadBuffer, readOffsetInBits, pWriteBuffer, writeOffsetInBits);
        OutputDebugStringA(text);
        fprintf(pFile, text);
        writeBits(pFile, (unsigned char*)pReadBuffer, readOffsetInBits + bitsToCopy);
        fclose(pFile);
#endif
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
#ifdef DOPRINTS
        FILE* pFile = fopen(logFileName, "at");
#endif

        ClientData& clientData = clientMap[(void *)buf];
        if (clientData.fragmentInserted && !clientData.fragmentSent)
        {
            OverallMatchStats overallStats;
            overallStats.addStatistic(0x3adcf, 0x31ce1, 4000.0);
            overallStats.addStatistic(0x3adcf, 0x31cef, 124.0);
            overallStats.addStatistic(0x23a039, 0x31cec, 87.0);
            overallStats.addStatistic(0x3adcf, 0x31ce7, 71.0);
            overallStats.addStatistic(0x3adcf, 0x31cee, 0.0);

            overallStats.addAccolade(0x23a039, 0x3191c, 87.0);
            overallStats.addAccolade(0x3adcf, 0x31918, 8.0);
            overallStats.addAccolade(0x3adcf, 0x31924, 1.0);

            char pBuffer[4096];

            PlayerMatchStats playerStats;
            playerStats.addStatistic(0, 0x31ce1, 2175.0);
            playerStats.addStatistic(0, 0x31ce7, 0.0);
            playerStats.addStatistic(0, 0x31cec, 87.0);
            playerStats.addStatistic(0, 0x31cee, 0.0);
            playerStats.addStatistic(0, 0x31cef, 41.0);

            playerStats.addAccolade(0, 0x3191C, 87.0);
            playerStats.addAccolade(0, 0x31924, 1.0);


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
            //                                      00000111000000000000000011100101000100000000 (prefix)
            unsigned long long prefix1 = 0x008a70000e0;
            unsigned int prefix1size = 44;

            //                                      01101101000000000000110110101101000100000000 (prefix)
            unsigned long long prefix2 = 0x008b5b000b6;
            unsigned int prefix2size = 44;

            unsigned int overallStatsSize = overallStats.size() * 8;
            unsigned int playerStatsSize = playerStats.size() * 8;

            //                           0001111100110 (payloadsize = 3320)
            //                                        x (object = FirstServerObject_0)
            unsigned int payloadsize = propsize + prefix1size + overallStatsSize + 
                                       propsize + prefix2size + playerStatsSize;
            unsigned int payloadsizesize = 13;


//#define FRAGMENT

#ifdef FRAGMENT
            clientData.insertionCount = fragmentSizeInBits;
#else
            clientData.insertionCount = part1size + countersize + part2size + payloadsizesize + payloadsize;
#endif

#ifdef DOPRINTS
            fprintf(pFile, "tamods: sendto before, aftermove and aftercopy\n");
            writeBits(pFile, (unsigned char*)buf, len * 8);
#endif
            *(unsigned int*)fragment |= (clientData.seqnr + 1) << 13;
            memMoveBits((unsigned char *)buf, clientData.insertionOffset, clientData.insertionOffset + clientData.insertionCount, len * 8 - clientData.insertionOffset);

            len = (clientData.maxNumberOfBits + clientData.insertionCount) / 8 + 1;
#ifdef DOPRINTS
            writeBits(pFile, (unsigned char*)buf, len * 8);
#endif
#ifdef FRAGMENT
            realLevel8Function(buf, clientData.insertionOffset, &fragment[0], 0, fragmentSizeInBits);
#else
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
#endif


#ifdef DOPRINTS
            writeBits(pFile, (unsigned char*)buf, len * 8);
#endif

            clientData.fragmentSent = true;
            clientData.insertionOffset = 0;
            clientData.insertionCount = 0;
        }
        clientData.maxNumberOfBits = 0;

#ifdef DOPRINTS
        fprintf(pFile, "tamods:   sendto %08X bytes (%08X bits) from %p:\n", len, len * 8, buf);
        //writeBits(pFile, (unsigned char *)buf, len * 8);
        fclose(pFile);
#endif

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

}
