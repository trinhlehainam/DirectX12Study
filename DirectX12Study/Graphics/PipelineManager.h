#pragma once

#include <string>

#include <d3d12.h>

class PipelineManager
{
public:
	PipelineManager();
	~PipelineManager();
public:
	bool Create(const std::string& name, ID3D12PipelineState* pPSO);
	ID3D12PipelineState* Get(const std::string& name) const;
private:
	PipelineManager(const PipelineManager&);
	void operator = (const PipelineManager&);
private:
	class Impl;
	Impl* m_impl = nullptr;
};

