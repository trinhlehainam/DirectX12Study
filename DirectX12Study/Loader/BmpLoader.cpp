#include "BmpLoader.h"
#include <cassert>
#include <Windows.h>

BmpLoader::BmpLoader(const char* filePath)
{
	assert(LoadFile(filePath));
}

BmpLoader::~BmpLoader()
{
}

bool BmpLoader::LoadFile(const char* filePath)
{
	FILE* fp = nullptr;
	auto err = fopen_s(&fp, filePath, "rb");
	if (err != 0)
	{
		char cerr[256];
		strerror_s(cerr, _countof(cerr), err);
		OutputDebugStringA(cerr);
	}

	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;

	fread_s(&fileHeader, sizeof(fileHeader), sizeof(fileHeader), 1, fp);
	fread_s(&infoHeader, sizeof(infoHeader), sizeof(infoHeader), 1, fp);

	auto byteSizePerPixel = infoHeader.biBitCount / 8;
	rawData_.resize(byteSizePerPixel * infoHeader.biWidth * infoHeader.biHeight);
	fread_s(rawData_.data(), rawData_.size(), rawData_.size(), 1, fp);

	fclose(fp);

	bmpSize_.width = infoHeader.biWidth;
	bmpSize_.height = infoHeader.biHeight;

	return true;
}

Size BmpLoader::GetBmpSize() const
{
	return bmpSize_;
}

const std::vector<uint8_t>& BmpLoader::GetRawData() const
{
	return rawData_;
}

