#pragma once
#include <string>
#include <vector>

class StringHelper
{
public:
	static std::string GetTexturePathFromModelPath(const char* modelPath, const char* texturePath);

	static std::wstring ConvertStringToWideString(const std::string& str); 

	static std::string GetFileExtension(const std::string& path);

	static std::vector<std::string> SplitFilePath(const std::string& path, const char splitter = ' *');
};

