#include "VMDMotion.h"
#include <stdio.h>
#include <windows.h>
#include <algorithm>

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

	constexpr size_t bezierNO[] = { 3, 7 ,11 ,15 };
	constexpr size_t offset = 15;
	for (auto& motion : motions)
	{
		DirectX::XMFLOAT2 b1, b2;
		b1.x = (motion.Interpolation[bezierNO[0] + offset])/127.0f;
		b1.y = (motion.Interpolation[bezierNO[1] + offset])/127.0f;
		b2.x = (motion.Interpolation[bezierNO[2] + offset])/127.0f;
		b2.y = (motion.Interpolation[bezierNO[3] + offset])/127.0f;
		m_vmdDatas[motion.BoneName].emplace_back(motion.FrameNo, motion.Rotatation, motion.Location, b1, b2);
	}

	for (auto& vmdData : m_vmdDatas)
	{
		auto& keyframe = vmdData.second;
		std::sort(keyframe.begin(), keyframe.end(),
			[](const VMDData& k1, const VMDData& k2)
			{
				return k1.frameNO < k2.frameNO;
			});
	}

	std::vector<size_t> lastFrameNOList(m_vmdDatas.size());
	int i = 0;
	for (auto& vmdData : m_vmdDatas)
	{
		lastFrameNOList[i] = vmdData.second.rbegin()->frameNO;
		i++;
	}

	std::sort(lastFrameNOList.begin(), lastFrameNOList.end(), [](const size_t& f1, const size_t& f2)
		{return f1 < f2; });

	m_maxFrame = *lastFrameNOList.rbegin();
	
	fclose(fp);

	return true;
}

const VMDData_t& VMDMotion::GetVMDMotionData() const
{
	return m_vmdDatas;
}

size_t VMDMotion::GetMaxFrame() const
{
	return m_maxFrame;
}
