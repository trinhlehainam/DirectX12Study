#include "DefaultBuffer.h"
#include "D12Helper.h"
#include <cassert>

DefaultBuffer::~DefaultBuffer()
{
    ResetSubresource();
}

ID3D12Resource* DefaultBuffer::Resource()
{
    return m_buffer.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS DefaultBuffer::GetGPUVirtualAddress()
{
    return m_buffer.Get()->GetGPUVirtualAddress();
}

void DefaultBuffer::SetUpSubresource(const void* pData, LONG_PTR RowPitch, LONG_PTR SlicePitch,
    bool IsShaderResourceBuffer)
{
    ResetSubresource();
    m_subresource = new D3D12_SUBRESOURCE_DATA();

    m_subresource->pData = pData;
    m_subresource->RowPitch = RowPitch;
    m_subresource->SlicePitch = SlicePitch;

    m_isShaderResourceBuffer = IsShaderResourceBuffer;
}

void DefaultBuffer::SetUpSubresource(const void* pData, size_t SizeInBytes)
{
    ResetSubresource();
    m_subresource = new D3D12_SUBRESOURCE_DATA();

    m_subresource->pData = pData;
    m_subresource->RowPitch = SizeInBytes;
    m_subresource->SlicePitch = SizeInBytes;

    m_isShaderResourceBuffer = false;
}

bool DefaultBuffer::UpdateSubresource(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList)
{
    assert(m_subresource);
    if (!m_subresource) return false;

    size_t sizeInBytes = static_cast<size_t>(m_subresource->SlicePitch);

    m_buffer = D12Helper::CreateBuffer(pDevice, sizeInBytes, D3D12_HEAP_TYPE_DEFAULT);
    m_intermedinateBuffer = D12Helper::CreateBuffer(pDevice, sizeInBytes);

    UpdateSubresources(pCmdList, m_buffer.Get(), m_intermedinateBuffer.Get(), 0, 0, 1, m_subresource);

    if(m_isShaderResourceBuffer)
        D12Helper::ChangeResourceState(pCmdList, m_buffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    else
        D12Helper::ChangeResourceState(pCmdList, m_buffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_GENERIC_READ);
        
    return true;
}

bool DefaultBuffer::ClearSubresource()
{
    ResetSubresource();
        
    if (!m_intermedinateBuffer) return false;
    m_intermedinateBuffer.Reset();

    return true;
}

void DefaultBuffer::ResetSubresource()
{
    if (m_subresource != nullptr)
    {
        delete m_subresource;
        m_subresource = nullptr;
    }
}
