#include "VMDMotion.h"
#include <stdio.h>
#include <windows.h>

bool VMDMotion::Load(const char* path)
{
	FILE* fp = nullptr;
	auto err = fopen_s(&fp, path, "rb");
	if (fp == nullptr || err != 0)
	{
		char cerr[256];
		strerror_s(cerr, _countof(cerr), err);
		OutputDebugStringA(cerr);
	}
#pragma pack(1)
	// モーションデータ
	struct VMD_MOTION { // 111 Bytes // モーション
		char BoneName[15]; // ボーン名
		DWORD FrameNo; // フレーム番号(読込時は現在のフレーム位置を0とした相対位置)
		DirectX::XMFLOAT3 Location; // 位置
		DirectX::XMFLOAT4 Rotatation; // Quaternion // 回転
		BYTE Interpolation[64]; // [4][4][4] // 補完
	} ;
#pragma pack()
	fseek(fp, sizeof(char) * 50, SEEK_CUR);

	uint32_t motionCount = 0;
	fread_s(&motionCount, sizeof(motionCount), sizeof(motionCount), 1, fp);

	std::vector<VMD_MOTION> motions(motionCount);

	fread_s(motions.data(), sizeof(motions[0]) * motions.size(), sizeof(motions[0]) * motions.size(), 1, fp);

	for (auto& motion : motions)
	{
		m_vmdDatas[motion.BoneName].emplace_back(motion.Rotatation,motion.FrameNo);
	}

	fclose(fp);

	return true;
}

const VMDData_t& VMDMotion::GetVMDMotionData() const
{
	return m_vmdDatas;
}
