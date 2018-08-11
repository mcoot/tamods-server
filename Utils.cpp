#include "Utils.h"
#include "NameCryptor.h"
#include <Shlobj.h>

// Converts UE3's FString to std::string
std::string Utils::f2std(FString &fstr)
{
	if (fstr.Count == 0 || fstr.Data == NULL)
		return "";
	wchar_t *wch = fstr.Data;
	std::wstring wstr(wch);
	return (std::string(wstr.begin(), wstr.end()));
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