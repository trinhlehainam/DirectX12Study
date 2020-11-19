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
	DirectX::XMFLOAT4 rotationMatrix;
	size_t frameNO;
	VMDData(const DirectX::XMFLOAT4& mat, const size_t& frameNo):rotationMatrix(mat), frameNO(frameNo){}
};

using VMDData_t = std::unordered_map <std::string,std::vector<VMDData>>;

class VMDMotion
{
private:
	VMDData_t m_vmdDatas;
public:
	bool Load(const char* path);
	const VMDData_t& GetVMDMotionData() const;
};

