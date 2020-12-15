#pragma once
#include <unordered_map>
#include <string>
#include <DirectXMath.h>
#include <vector>

// Load VMD file to VMDMotion data
// Use XMMatrixRotationQuadternion to create Rotation Matrix
// Use RecursiveCalculate 

struct VMDData
{
	DirectX::XMFLOAT4 quaternion;
	DirectX::XMFLOAT3 location;
	size_t frameNO;
	DirectX::XMFLOAT2 b1,b2;		// bezier data
	VMDData(const size_t& frameNo,
		const DirectX::XMFLOAT4& quaternion,
		const DirectX::XMFLOAT3& location,
		const DirectX::XMFLOAT2& b1,
		const DirectX::XMFLOAT2& b2):
		quaternion(quaternion), frameNO(frameNo), location(location), b1(b1),b2(b2) {}
};

using VMDData_t = std::unordered_map <std::string,std::vector<VMDData>>;

class VMDMotion
{
public:
	bool Load(const char* path);
	const VMDData_t& GetVMDMotionData() const;
	size_t GetMaxFrame() const;
private:
	VMDData_t m_vmdDatas;
	size_t m_maxFrame = 0;

};

