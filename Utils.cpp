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
int Utils::searchMapId(const std::map<std::string, int> map, const std::string &str, const std::string &location, bool print_on_fail)
{
	std::string clean = Utils::cleanString(str);
	for (auto const &it : map)
	{
		if (std::regex_match(clean, std::regex(it.first)))
			return (it.second);
	}
	if (!print_on_fail)
		return (0);
	if (!location.size())
		Logger::error("WARNING: searched item '%s' could not be identified", str.c_str());
	else
		Logger::error("WARNING: searched item '%s' could not be identified as %s", str.c_str(), location.c_str());
	return (0);
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

static std::map<int, int> mappedPerks = {
	{ CONST_ITEM_PERK_BOUNTYHUNTER, 0b00001 },
	{ CONST_ITEM_PERK_CLOSECOMBAT, 0b00010 },
	{ CONST_ITEM_PERK_DETERMINATION, 0b00011 },
	{ CONST_ITEM_PERK_EGOCENTRIC, 0b00100 },
	{ CONST_ITEM_PERK_LOOTER, 0b00101 },
	{ CONST_ITEM_PERK_MECHANIC, 0b00110 },
	{ CONST_ITEM_PERK_PILOT, 0b00111 },
	{ CONST_ITEM_PERK_POTENTIALENERGY, 0b01000 },
	{ CONST_ITEM_PERK_QUICKDRAW, 0b01001 },
	{ CONST_ITEM_PERK_REACH, 0b01010 },
	{ CONST_ITEM_PERK_SAFEFALL, 0b01011 },
	{ CONST_ITEM_PERK_SAFETYTHIRD, 0b01100 },
	{ CONST_ITEM_PERK_STEALTHY, 0b01101 },
	{ CONST_ITEM_PERK_SUPERCAPACITOR, 0b01110 },
	{ CONST_ITEM_PERK_SUPERHEAVY, 0b01111 },
	{ CONST_ITEM_PERK_SURVIVALIST, 0b10000 },
	{ CONST_ITEM_PERK_ULTRACAPACITOR, 0b10001 },
	{ CONST_ITEM_PERK_WHEELDEAL, 0b10010 },
	{ CONST_ITEM_PERK_RAGE, 0b10011 },
	{ CONST_ITEM_PERK_SONICPUNCH, 0b10100 },
	{ CONST_ITEM_PERK_LIGHTWEIGHT, 0b10101 },
};

static std::map<int, int> mappedVoices = {
	{ CONST_ITEM_VOICE_LIGHT, 0b00001 },
	{ CONST_ITEM_VOICE_MEDIUM, 0b00010 },
	{ CONST_ITEM_VOICE_HEAVY, 0b00011 },
	{ CONST_ITEM_VOICE_DARK, 0b00100 },
	{ CONST_ITEM_VOICE_FEM1, 0b00101 },
	{ CONST_ITEM_VOICE_FEM2, 0b00110 },
	{ CONST_ITEM_VOICE_AUS, 0b00111 },
	{ CONST_ITEM_VOICE_TOTALBISCUIT, 0b01000 },
	{ CONST_ITEM_VOICE_STOWAWAY, 0b01001 },
	{ CONST_ITEM_VOICE_BASEMENTCHAMPION, 0b01010 },
	{ CONST_ITEM_VOICE_T2FEM01, 0b01011 },
	{ CONST_ITEM_VOICE_T2FEM02, 0b01100 },
	{ CONST_ITEM_VOICE_T2FEM03, 0b01101 },
	{ CONST_ITEM_VOICE_T2FEM04, 0b01110 },
	{ CONST_ITEM_VOICE_T2FEM05, 0b01111 },
	{ CONST_ITEM_VOICE_T2MALE01, 0b10000 },
	{ CONST_ITEM_VOICE_T2MALE02, 0b10001 },
	{ CONST_ITEM_VOICE_T2MALE03, 0b10010 },
	{ CONST_ITEM_VOICE_T2MALE04, 0b10011 },
	{ CONST_ITEM_VOICE_T2MALE05, 0b10100 },
	{ CONST_ITEM_VOICE_T2BDERM01, 0b10101 },
	{ CONST_ITEM_VOICE_T2BDERM02, 0b10110 },
	{ CONST_ITEM_VOICE_T2BDERM03, 0b10111 }
};

int Utils::perksAndVoice_Encode(int voice, int perkA, int perkB) {
	int mappedVoice, mappedPerkA, mappedPerkB;

	auto& it = mappedVoices.find(voice);
	if (it == mappedVoices.end()) return 0;
	mappedVoice = it->second;

	it = mappedPerks.find(perkA);
	if (it == mappedPerks.end()) return 0;
	mappedPerkA = it->second;

	it = mappedPerks.find(perkB);
	if (it == mappedPerks.end()) return 0;
	mappedPerkB = it->second;

	return (mappedVoice << 10) | (mappedPerkA << 5) | (mappedPerkB);
}

int Utils::perksAndVoice_DecodeVoice(int perksAndVoice) {
	int mappedVoice = (perksAndVoice & 0b0111110000000000) >> 10;
	for (auto& voice : mappedVoices) {
		if (voice.second == mappedVoice) return voice.first;
	}

	return 0;
}

int Utils::perksAndVoice_DecodePerkA(int perksAndVoice) {
	int mappedPerkA = (perksAndVoice & 0b0000001111100000) >> 5;
	for (auto& perk : mappedPerks) {
		if (perk.second == mappedPerkA) return perk.first;
	}

	return 0;
}

int Utils::perksAndVoice_DecodePerkB(int perksAndVoice) {
	int mappedPerkB = (perksAndVoice & 0b0000000000011111);
	for (auto& perk : mappedPerks) {
		if (perk.second == mappedPerkB) return perk.first;
	}

	return 0;
}