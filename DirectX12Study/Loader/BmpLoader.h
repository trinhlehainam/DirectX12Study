#pragma once
#include <vector>
#include "../common.h"

class BmpLoader
{
private:
	bool LoadFile(const char* filePath);
	std::vector<uint8_t> rawData_;
	Size bmpSize_;
public:
	BmpLoader(const char* filePath);
	~BmpLoader();
	// ビットマップの縦、横の大きさを返す
	Size GetBmpSize() const;
	const std::vector<uint8_t>& GetRawData() const;
};

