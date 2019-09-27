#include "Utils.h"
#include "NameCryptor.h"
#include <Shlobj.h>

namespace Utils {

    ServerGameStatus serverGameStatus = ServerGameStatus::UNKNOWN;
    ATrGameReplicationInfo* tr_gri = NULL;
    std::mutex tr_gri_mutex;

}

// Converts UE3's FString to std::string
std::string Utils::f2std(FString &fstr)
{
    if (fstr.Count == 0 || fstr.Data == NULL)
        return "";
    wchar_t *wch = fstr.Data;
    std::wstring wstr(wch);
    return (std::string(wstr.begin(), wstr.end()));
}

// Removes whitespace and lowercases the string
std::string Utils::cleanString(const std::string &str)
{
    std::string out = str;
    out.erase(std::remove(out.begin(), out.end(), ' '), out.end());
    out.erase(std::remove(out.begin(), out.end(), '\t'), out.end());
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return (out);
}

// Removes whitespace from both ends of a string
std::string Utils::trim(const std::string &str)
{
    size_t begin = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");

    if (begin == std::string::npos || end == std::string::npos)
        return ("");
    return (str.substr(begin, end - begin + 1));
};

// Regex search in a map of type <string, int> where the string are regexes
// location and print_on_fail are only used for error messages
int Utils::searchMapId(const std::map<std::string, int> map, const std::string &str, const std::string &location, bool print_on_fail, int failure_sentinel_value)
{
    std::string clean = Utils::cleanString(str);
    for (auto const &it : map)
    {
        if (std::regex_match(clean, std::regex(it.first)))
            return (it.second);
    }
    if (!print_on_fail)
        return (failure_sentinel_value);
    if (!location.size())
        Logger::error("WARNING: searched item '%s' could not be identified", str.c_str());
    else
        Logger::error("WARNING: searched item '%s' could not be identified as %s", str.c_str(), location.c_str());
    return (failure_sentinel_value);
}

// Returns the config directory path
std::string Utils::getConfigDir()
{
    wchar_t* localDocuments = 0;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &localDocuments);

    if (FAILED(hr)) {
        return "C:\\";
    }

    std::wstring wstr = localDocuments;

    CoTaskMemFree(static_cast<void*>(localDocuments));

    return std::string(wstr.begin(), wstr.end()) + "\\My Games\\Tribes Ascend\\TribesGame\\config\\";
}

bool Utils::fileExists(const std::string &path, const std::string &mode)
{
    if (FILE *file = fopen(path.c_str(), mode.c_str()))
    {
        fclose(file);
        return true;
    }
    return false;
}

bool Utils::dirExists(const std::string &path)
{
    DWORD ftyp = GetFileAttributesA(path.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false; //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true; // this is a directory!

    return false; // this is not a directory!
}

// Perk encoding

int Utils::perks_Encode(int perkA, int perkB) {
    return (perkA << 16) | perkB;
}

int Utils::perks_DecodeA(int encoded) {
    return encoded >> 16;
}

int Utils::perks_DecodeB(int encoded) {
    return encoded & 0x0000FFFF;
}

// Server Password Hashing
static char hash_xor_constants[8] = { 0x55, 0x93, 0x55, 0x58, 0xBA, 0x6f, 0xe9, 0xf9 };
static char hash_interspersed_constants[8] = { 0x7a, 0x1e, 0x9f, 0x47, 0xf9, 0x17, 0xb0, 0x03 };

std::vector<unsigned char> Utils::passwordHash(std::string password) {
    std::vector<unsigned char> result;
    for (int i = 0; i < password.length(); ++i) {
        result.push_back((password[i] ^ hash_xor_constants[i % 8]));
        result.push_back(hash_interspersed_constants[i % 8]);
    }
    return result;
}

// Player ID encoding
long long Utils::netIdToLong(FUniqueNetId id) {
    long long tmpB = id.Uid.B;
    return (tmpB << 32) | (id.Uid.A);
}

FUniqueNetId Utils::longToNetId(long long id) {
    FUniqueNetId r;
    r.Uid.A = id & 0x00000000FFFFFFFF;
    r.Uid.B = id >> 32;
    return r;
}

// Get player PRI
ATrPlayerReplicationInfo* Utils::getPRIForPlayerId(long long playerId) {
    if (!Utils::tr_gri) return NULL;
    auto arr = Utils::tr_gri->PRIArray;
    for (int i = 0; i < arr.Count; ++i) {
        if (arr.GetStd(i) && netIdToLong(arr.GetStd(i)->UniqueId) == playerId) {
            return (ATrPlayerReplicationInfo*)arr.GetStd(i);
        }
    }

    return NULL;
}

ATrPlayerReplicationInfo* Utils::getPRIForPlayerName(std::string playerName) {
    if (!Utils::tr_gri) return NULL;
    auto arr = Utils::tr_gri->PRIArray;
    for (int i = 0; i < arr.Count; ++i) {
        if (arr.GetStd(i) && Utils::f2std(arr.GetStd(i)->PlayerName) == playerName) {
            return (ATrPlayerReplicationInfo*)arr.GetStd(i);
        }
    }

    return NULL;
}