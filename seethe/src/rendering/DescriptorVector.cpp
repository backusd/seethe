#include "DescriptorVector.h"


namespace seethe
{
DescriptorVector::DescriptorVector(std::shared_ptr<DeviceResources> deviceResources,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    unsigned int initialCapacity) :
    m_count(0),
    m_deviceResources(deviceResources),
    m_capacity(initialCapacity),
    m_type(type),
    m_handleIncrementSize(deviceResources->GetDevice()->GetDescriptorHandleIncrementSize(type))
{
    ASSERT(m_deviceResources != nullptr, "No device resources");
    ASSERT(m_capacity > 0, "Capacity must be greater than 0 so that we can safely double the capacity when necessary");

    auto device = m_deviceResources->GetDevice();

    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = m_capacity;
    desc.Type = m_type;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    // Create the CPU copyable descriptor heap
    GFX_THROW_INFO(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descriptorHeapCopyable)));

    // Create the descriptor heap that will actually be use for retrieving descriptors for rendering
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    GFX_THROW_INFO(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descriptorHeapShaderVisible)));

    m_cpuHeapStart = m_descriptorHeapShaderVisible->GetCPUDescriptorHandleForHeapStart();
    m_gpuHeapStart = m_descriptorHeapShaderVisible->GetGPUDescriptorHandleForHeapStart();
}

void DescriptorVector::DoubleTheCapacity()
{
    auto device = m_deviceResources->GetDevice();

    // Create new descriptor heaps with doubled capacity
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> newDescriptorHeapCopyable;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> newDescriptorHeapShaderVisible;

    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = m_capacity * 2;
    desc.Type = m_type;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;
    GFX_THROW_INFO(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newDescriptorHeapCopyable)));

    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    GFX_THROW_INFO(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newDescriptorHeapShaderVisible)));

    // Copy previous descriptor heap to the new one
    device->CopyDescriptorsSimple(
        m_capacity,
        newDescriptorHeapCopyable->GetCPUDescriptorHandleForHeapStart(),
        m_descriptorHeapCopyable->GetCPUDescriptorHandleForHeapStart(),
        m_type
    );
    device->CopyDescriptorsSimple(
        m_capacity,
        newDescriptorHeapShaderVisible->GetCPUDescriptorHandleForHeapStart(),
        m_descriptorHeapCopyable->GetCPUDescriptorHandleForHeapStart(),
        m_type
    );

    // Reassign the descriptor heap pointers
    m_descriptorHeapCopyable = nullptr;
    m_descriptorHeapShaderVisible = nullptr;

    m_descriptorHeapCopyable = newDescriptorHeapCopyable;
    m_descriptorHeapShaderVisible = newDescriptorHeapShaderVisible;

    // Update the heap start handles
    m_cpuHeapStart = m_descriptorHeapShaderVisible->GetCPUDescriptorHandleForHeapStart();
    m_gpuHeapStart = m_descriptorHeapShaderVisible->GetGPUDescriptorHandleForHeapStart();

    // Update the capacity
    m_capacity *= 2;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorVector::GetCPUHandleAt(UINT index) const noexcept
{
    ASSERT(index < m_capacity, "Index is too large");
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_cpuHeapStart);
    handle.Offset(index, m_handleIncrementSize);
    return handle;
}
D3D12_GPU_DESCRIPTOR_HANDLE DescriptorVector::GetGPUHandleAt(UINT index) const noexcept
{
    ASSERT(index < m_capacity, "Index is too large");
    CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_gpuHeapStart);
    handle.Offset(index, m_handleIncrementSize);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorVector::GetCPUCopyableHandleAt(UINT index) const noexcept
{
    ASSERT(index < m_capacity, "Index is too large");
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_descriptorHeapCopyable->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(index, m_handleIncrementSize);
    return handle;
}
D3D12_GPU_DESCRIPTOR_HANDLE DescriptorVector::GetGPUCopyableHandleAt(UINT index) const noexcept
{
    ASSERT(index < m_capacity, "Index is too large");
    CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_descriptorHeapCopyable->GetGPUDescriptorHandleForHeapStart());
    handle.Offset(index, m_handleIncrementSize);
    return handle;
}

unsigned int DescriptorVector::GetNextIndexAndEnsureCapacity() noexcept
{
    // Get the index that will be used for the next object and increment the count
    unsigned int indexIntoHeap = m_count;

    // If there have been any released descriptors, we can just re-use that memory instead
    if (m_releasedIndices.size() > 0)
    {
        // Get the most recently removed index
        indexIntoHeap = m_releasedIndices.back();
        // Remove the index from the list of released indices
        m_releasedIndices.pop_back();
    }

    // Increment the count (NOTE: This MUST be done AFTER calling GetNextIndex because that function uses m_count to compute the next index)
    ++m_count;

    // If we are over capacity, resize the descriptor heaps
    if (m_count > m_capacity)
        DoubleTheCapacity();

    return indexIntoHeap;
}

unsigned int DescriptorVector::EmplaceBackShaderResourceView(ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
    ASSERT(m_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Invalid to create a Shader Resource View if the type is not D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV");

    // Get the next available index into the heap and increase the capacity if necessary
    unsigned int indexIntoHeap = GetNextIndexAndEnsureCapacity();

    // Create the Shader Resource Views in both heaps
    auto device = m_deviceResources->GetDevice();
    device->CreateShaderResourceView(pResource, desc, GetCPUCopyableHandleAt(indexIntoHeap));
    device->CreateShaderResourceView(pResource, desc, GetCPUHandleAt(indexIntoHeap));

    return indexIntoHeap;
}
unsigned int DescriptorVector::EmplaceBackConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc)
{
    LOG_WARN("{}", "DescriptorVector::EmplaceBackConstantBufferView() has been called, but I've never tested this function.");

    ASSERT(m_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Invalid to create a Constant Buffer View if the type is not D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV");

    // Get the next available index into the heap and increase the capacity if necessary
    unsigned int indexIntoHeap = GetNextIndexAndEnsureCapacity();

    // Create the Constant Buffer Views in both heaps
    auto device = m_deviceResources->GetDevice();
    device->CreateConstantBufferView(desc, GetCPUCopyableHandleAt(indexIntoHeap));
    device->CreateConstantBufferView(desc, GetCPUHandleAt(indexIntoHeap));

    return indexIntoHeap;
}
unsigned int DescriptorVector::EmplaceBackUnorderedAccessView(ID3D12Resource* pResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc)
{
    ASSERT(m_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Invalid to create an Unordered Access View if the type is not D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV");

    // Get the next available index into the heap and increase the capacity if necessary
    unsigned int indexIntoHeap = GetNextIndexAndEnsureCapacity();

    // Create the Unordered Access View in both heaps
    // NOTE: Setting optional second parameter (pCounterResource) to NULL. (I'm not entirely sure what it does exactly)
    //       However, when this parameter is nullptr, the buffer CounterOffsetInBytes value must be 0 (according to the documentation)
    ASSERT(desc->Buffer.CounterOffsetInBytes == 0, "When passing nullptr for pCounterResource, the buffer CounterOffsetInBytes must be 0");
    auto device = m_deviceResources->GetDevice();
    device->CreateUnorderedAccessView(pResource, nullptr, desc, GetCPUCopyableHandleAt(indexIntoHeap));
    device->CreateUnorderedAccessView(pResource, nullptr, desc, GetCPUHandleAt(indexIntoHeap));

    return indexIntoHeap;
}

void DescriptorVector::ReleaseAt(unsigned int index) noexcept
{
    ASSERT(index < m_capacity, "Index is too large");
    ASSERT(std::find(m_releasedIndices.begin(), m_releasedIndices.end(), index) == m_releasedIndices.end(), "Index has already been released");
    m_releasedIndices.push_back(index);
    --m_count;
}

}