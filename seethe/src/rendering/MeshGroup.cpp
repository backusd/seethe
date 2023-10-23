#include "MeshGroup.h"


namespace seethe
{
//
// MeshGroup ======================================================================================================
//
MeshGroup::MeshGroup(MeshGroup&& rhs) noexcept :
	m_deviceResources(rhs.m_deviceResources),
	m_vertexBufferGPU(rhs.m_vertexBufferGPU),
	m_indexBufferGPU(rhs.m_indexBufferGPU),
	m_vertexBufferView(rhs.m_vertexBufferView),
	m_indexBufferView(rhs.m_indexBufferView),
	m_submeshes(std::move(rhs.m_submeshes))
{
	LOG_WARN("{}", "MeshGroup Move Constructor called, but this method has not been tested. Make sure this call was intentional and, if so, that the constructor works as expected");

	// Set the "moved from" flag on the rhs object so that it knows not to call DelayedDelete on GPU resources
	rhs.m_movedFrom = true;
}
MeshGroup& MeshGroup::operator=(MeshGroup&& rhs) noexcept
{
	LOG_WARN("{}", "MeshGroup Move Assignment operator called, but this method has not been tested. Make sure this call was intentional and, if so, that this function works as expected");

	m_deviceResources = rhs.m_deviceResources;
	m_vertexBufferGPU = rhs.m_vertexBufferGPU;
	m_indexBufferGPU = rhs.m_indexBufferGPU;
	m_vertexBufferView = rhs.m_vertexBufferView;
	m_indexBufferView = rhs.m_indexBufferView;
	m_submeshes = std::move(rhs.m_submeshes);

	// Set the "moved from" flag on the rhs object so that it knows not to call DelayedDelete on GPU resources
	rhs.m_movedFrom = true;

	return *this;
}
Microsoft::WRL::ComPtr<ID3D12Resource> MeshGroup::CreateDefaultBuffer(const void* initData, UINT64 byteSize) const
{
	Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = nullptr;

	// Create the actual default buffer resource.
	auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	GFX_THROW_INFO(
		m_deviceResources->GetDevice()->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(defaultBuffer.GetAddressOf())
		)
	);

	// In order to copy CPU memory data into our default buffer, we need to create an intermediate upload heap. 
	auto props2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto desc2 = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	GFX_THROW_INFO(
		m_deviceResources->GetDevice()->CreateCommittedResource(
			&props2,
			D3D12_HEAP_FLAG_NONE,
			&desc2,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(uploadBuffer.GetAddressOf())
		)
	);

	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource. At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap. Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	auto commandList = m_deviceResources->GetCommandList();

	auto _b = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	commandList->ResourceBarrier(1, &_b);

	UpdateSubresources<1>(commandList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

	auto _b2 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	commandList->ResourceBarrier(1, &_b2);

	// MUST delete the upload buffer AFTER it is done being referenced by the GPU
	m_deviceResources->DelayedDelete(uploadBuffer);

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.
	return defaultBuffer;
}

//
// DynamicMesh ======================================================================================================
//
Microsoft::WRL::ComPtr<ID3D12Resource> DynamicMeshGroup::CreateUploadBuffer(UINT64 totalBufferSize) const
{
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;

	// Need to create the buffer in an upload heap so the CPU can regularly send new data to it
	auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// Create a buffer that will hold an entire buffer per frame resource
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(totalBufferSize * g_numFrameResources);

	GFX_THROW_INFO(
		m_deviceResources->GetDevice()->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)
		)
	);

	return uploadBuffer;
}
}