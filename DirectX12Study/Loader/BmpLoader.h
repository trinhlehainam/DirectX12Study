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
	// �r�b�g�}�b�v�̏c�A���̑傫����Ԃ�
	Size GetBmpSize() const;
	const std::vector<uint8_t>& GetRawData() const;
};

