#include "PMDModel.h"
#include <stdio.h>
#include <Windows.h>

bool PMDModel::Load(const char* path)
{
	//Ž¯•ÊŽq"pmd"
	FILE* fp = nullptr;
	auto err = fopen_s(&fp, path, "rb");
	if (err != 0)
	{
		char cerr[256];
		strerror_s(cerr, _countof(cerr), err);
		OutputDebugStringA(cerr);
	}
#pragma pack(1)
	struct PMDHeader {
		char id[3];
		// padding
		float version;
		char name[20];
		char comment[256];
	};
#pragma pack()
	PMDHeader header;
	fread_s(&header, sizeof(header), sizeof(header), 1, fp);

	fclose(fp);

	return true;
}
