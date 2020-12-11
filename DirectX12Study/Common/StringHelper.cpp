#include "StringHelper.h"

#include <windows.h>

std::string StringHelper::GetTexturePathFromModelPath(const char* modelPath, const char* texturePath)
{
	std::string ret = modelPath;
	auto idx = ret.rfind("/") + 1;

	return ret.substr(0, idx) + texturePath;
}

std::wstring StringHelper::ConvertStringToWideString(const std::string& str)
{
	std::wstring ret;
	auto wstringSize = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), str.length(), nullptr, 0);
	ret.resize(wstringSize);
	MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), str.length(), &ret[0], ret.length());
	return ret;
}

std::string StringHelper::GetFileExtension(const std::string& path)
{
	auto idx = path.rfind('.') + 1;
	return idx == std::string::npos ? "" : path.substr(idx, path.length() - idx);
}

std::vector<std::string> StringHelper::SplitFilePath(const std::string& path, const char splitter)
{
	std::vector <std::string> ret;
	auto idx = path.rfind(splitter);
	if (idx == std::string::npos)
		ret.push_back(path);
	ret.push_back(path.substr(0, idx));
	++idx;
	ret.push_back(path.substr(idx, path.length() - idx));
	return ret;
}
