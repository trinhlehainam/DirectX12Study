#pragma once
#include <string>
#include <vector>

namespace StringHelper
{
	std::string GetTexturePathFromModelPath(const char* modelPath, const char* texturePath);

	std::wstring ConvertStringToWideString(const std::string& str); 

	std::string GetFileExtension(const std::string& path);

	std::wstring GetFileExtensionW(const std::wstring& path);

	std::vector<std::string> SplitFilePath(const std::string& path, const char splitter = ' *');
};

