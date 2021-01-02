#pragma once
#include <d3dx12.h>

struct FrameResource
{
	FrameResource(ID3D12Device* pDevice);
	// Each frame resource has their command allocator to store its own command list
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdAlloc = nullptr;
	// Use to check if GPU is currently working on this frame resource
	uint64_t FenceValue = 0;
	
};

