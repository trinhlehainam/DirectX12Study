#pragma once
#include <unordered_map>
#include <string>
#include <DirectXMath.h>

// Load VMD file to VMDMotion data
// Use XMMatrixRotationQuadternion to create Rotation Matrix
// Use RecursiveCalculate 

struct VMDData
{

};

class VMDMotion
{
private:
	std::unordered_map <std::string, DirectX::XMFLOAT4> m_vmdDatas;
public:
	bool Load(const char* path);
	const std::unordered_map <std::string, DirectX::XMFLOAT4>& GetVMDMotionData() const;
};

