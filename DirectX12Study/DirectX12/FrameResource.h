#pragma once
#include <d3dx12.h>

struct FrameResource
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdAlloc = nullptr;
	uint64_t FenceValue = 0;
	FrameResource(ID3D12Device* pDevice);
};

