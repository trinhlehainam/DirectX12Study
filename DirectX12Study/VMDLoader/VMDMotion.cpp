#include "VMDMotion.h"
#include <stdio.h>
#include <windows.h>
#include <vector>

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
	// ���[�V�����f�[�^
	struct VMD_MOTION { // 111 Bytes // ���[�V����
		char BoneName[15]; // �{�[����
		DWORD FrameNo; // �t���[���ԍ�(�Ǎ����͌��݂̃t���[���ʒu��0�Ƃ������Έʒu)
		DirectX::XMFLOAT3 Location; // �ʒu
		DirectX::XMFLOAT4 Rotatation; // Quaternion // ��]
		BYTE Interpolation[64]; // [4][4][4] // �⊮
	} ;
#pragma pack()
	

	fseek(fp, sizeof(char) * 50, SEEK_CUR);

	DWORD motionCount;
	fread_s(&motionCount, sizeof(motionCount), sizeof(motionCount), 1, fp);

	std::vector<VMD_MOTION> motions(motionCount);

	fread_s(motions.data(), sizeof(motions[0]) * motions.size(), sizeof(motions[0]) * motions.size(), 1, fp);

	for (auto& motion : motions)
	{
		m_vmdDatas[motion.BoneName] = motion.Rotatation;
	}

	fclose(fp);

	return true;
}

const std::unordered_map<std::string, DirectX::XMFLOAT4>& VMDMotion::GetVMDMotionData() const
{
	return m_vmdDatas;
}
