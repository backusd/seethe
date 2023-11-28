#pragma once
#include "pch.h"
#include "DeviceResources.h"
#include "utils/Log.h"
#include "utils/Constants.h"

namespace seethe
{
class ConstantBufferBase
{
public:
	ConstantBufferBase(std::shared_ptr<DeviceResources> deviceResources) :
		m_deviceResources(deviceResources),
		m_uploadBuffer(nullptr),
		m_elementByteSizeOffsetter(0)
	{
		ASSERT(m_deviceResources != nullptr, "No device resources");
	}
	ConstantBufferBase(ConstantBufferBase&& rhs) noexcept :
		m_deviceResources(rhs.m_deviceResources),
		m_uploadBuffer(nullptr),
		m_elementByteSizeOffsetter(rhs.m_elementByteSizeOffsetter)
	{}
	ConstantBufferBase& operator=(ConstantBufferBase&& rhs) noexcept
	{
		m_deviceResources = rhs.m_deviceResources;
		m_uploadBuffer = nullptr;
		m_elementByteSizeOffsetter = rhs.m_elementByteSizeOffsetter;
		return *this;
	}
	virtual ~ConstantBufferBase() noexcept
	{
		if (m_uploadBuffer != nullptr)
			m_uploadBuffer->Unmap(0, nullptr);

		// Upload buffer might still be in use by the GPU, so do a delayed delete
		m_deviceResources->DelayedDelete(m_uploadBuffer);
		m_uploadBuffer = nullptr;
	}

	ND inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(unsigned int frameIndex) const noexcept
	{
		return m_uploadBuffer->GetGPUVirtualAddress() + static_cast<UINT64>(frameIndex) * m_elementByteSizeOffsetter;
	}


protected:
	ConstantBufferBase(const ConstantBufferBase& rhs) = delete;
	ConstantBufferBase& operator=(const ConstantBufferBase& rhs) = delete;

	std::shared_ptr<DeviceResources> m_deviceResources;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;

	// When using a ConstantBufferMapped, this value must be set to m_elementByteSize so that we can offset into the buffer
	// However, when using a ConstantBufferStatic, this value must remain 0 because we only retain a single copy of data in
	// the buffer, so there is no need to offset
	UINT m_elementByteSizeOffsetter;
};

template<typename T>
class ConstantBufferMapped : public ConstantBufferBase
{
public:
	// Let the element count be the maximum by default
	// Maximum allowed constant buffer size is 4096 float4's which is 65536 bytes
	ConstantBufferMapped(std::shared_ptr<DeviceResources> deviceResources, unsigned int elementCount = 65536 / sizeof(T)) :
		ConstantBufferBase(deviceResources),
		m_elementCount(elementCount),
		m_mappedData(nullptr)
	{
		Initialize();
	}
	ConstantBufferMapped(ConstantBufferMapped&& rhs) :
		ConstantBufferBase(std::move(rhs)),
		m_elementCount(rhs.m_elementCount),
		m_mappedData(nullptr),
		m_elementByteSize(rhs.m_elementByteSize)
	{
		Initialize();
	}
	ConstantBufferMapped& operator=(ConstantBufferMapped&& rhs)
	{
		ConstantBufferBase::operator=(std::move(rhs));
		m_elementCount = rhs.m_elementCount;
		m_mappedData = nullptr;
		m_elementByteSize = rhs.m_elementByteSize;
		Initialize();
		return *this;
	}
	virtual ~ConstantBufferMapped() noexcept override
	{
		if (m_uploadBuffer != nullptr) 
			m_uploadBuffer->Unmap(0, nullptr); 

		m_mappedData = nullptr; 

		// Upload buffer might still be in use by the GPU, so do a delayed delete
		m_deviceResources->DelayedDelete(m_uploadBuffer); 
		m_uploadBuffer = nullptr; 
	}

	
	inline void CopyData(unsigned int frameIndex, std::span<T> elements) noexcept
	{
		// Ensure that we are not sending more data than the buffer was created for
		ASSERT(elements.size() <= m_elementCount, "More data than expected");
		memcpy(&m_mappedData[frameIndex * m_elementByteSize], elements.data(), elements.size_bytes());
	}
	inline void CopyData(unsigned int frameIndex, const T& singleElement) noexcept
	{
		memcpy(&m_mappedData[frameIndex * m_elementByteSize], &singleElement, sizeof(T));
	}

private:
	ConstantBufferMapped(const ConstantBufferMapped& rhs) = delete;
	ConstantBufferMapped& operator=(const ConstantBufferMapped& rhs) = delete;

	void Initialize()
	{
		ASSERT(sizeof(T) <= 65536, "Size of T is too large"); 
		ASSERT(m_elementCount > 0, "Invalid to create a 0 sized constant buffer");
		ASSERT(m_elementCount * sizeof(T) <= 65536, "Element count is too large");

		size_t totalSize = (sizeof(T) * m_elementCount);
		m_elementByteSize = (totalSize + 255) & ~255;
		m_elementByteSizeOffsetter = m_elementByteSize;

		// Need to create the buffer in an upload heap so the CPU can regularly send new data to it
		auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		// Create a buffer that will hold one element for each frame resource
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(m_elementByteSize) * g_numFrameResources);

		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateCommittedResource(
				&props,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_uploadBuffer)
			)
		);

		GFX_THROW_INFO(
			m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData))
		);

		// We do not need to unmap until we are done with the resource. However, we must not write to
		// the resource while it is in use by the GPU (so we must use synchronization techniques).
	}

	BYTE* m_mappedData;

	// Constant buffers must be a multiple of the minimum hardware
	// allocation size (usually 256 bytes).  So round up to nearest
	// multiple of 256.  We do this by adding 255 and then masking off
	// the lower 2 bytes which store all bits < 256.
	// Example: Suppose byteSize = 300.
	// (300 + 255) & ~255
	// 555 & ~255
	// 0x022B & ~0x00ff
	// 0x022B & 0xff00
	// 0x0200
	// 512
	UINT m_elementByteSize = 0; // default to 0 - will get appropriately assigned in Initialize()
	unsigned int m_elementCount;
};

template<typename T>
class ConstantBufferStatic : public ConstantBufferBase
{
public:
	// Let the element count be the maximum by default
	// Maximum allowed constant buffer size is 4096 float4's which is 65536 bytes
	ConstantBufferStatic(std::shared_ptr<DeviceResources> deviceResources, unsigned int elementCount = 65536 / sizeof(T)) :
		ConstantBufferBase(deviceResources),
		m_elementCount(elementCount),
		m_intermediateBuffer(nullptr)
	{
		Initialize();
	}
	ConstantBufferStatic(ConstantBufferStatic&& rhs) :
		ConstantBufferBase(std::move(rhs)),
		m_elementCount(rhs.m_elementCount),
		m_intermediateBuffer(nullptr),
		m_elementByteSize(rhs.m_elementByteSize)
	{
		Initialize();
	}
	ConstantBufferStatic& operator=(ConstantBufferStatic&& rhs)
	{
		ConstantBufferBase::operator=(std::move(rhs));
		m_elementCount = rhs.m_elementCount;
		m_intermediateBuffer = nullptr;
		m_elementByteSize = rhs.m_elementByteSize;
		Initialize();
		return *this;
	}
	virtual ~ConstantBufferStatic() noexcept override
	{
		// Upload & intermediate buffer might still be in use by the GPU, so do a delayed delete
		m_deviceResources->DelayedDelete(m_uploadBuffer);
		m_uploadBuffer = nullptr;

		m_deviceResources->DelayedDelete(m_intermediateBuffer);
		m_intermediateBuffer = nullptr;
	}

	void CopyData(std::span<T> elements)
	{
		// Ensure that we are not sending more data than the buffer was created for
		ASSERT(elements.size() <= m_elementCount, "More data than expected");
		
		// Describe the data we want to copy into the default buffer.
		D3D12_SUBRESOURCE_DATA subResourceData = {}; 
		subResourceData.pData = static_cast<void*>(elements.data());
		subResourceData.RowPitch = static_cast<UINT>(elements.size()) * sizeof(T); 
		subResourceData.SlicePitch = subResourceData.RowPitch; 

		CopyData(&subResourceData);
	}
	void CopyData(const T& singleElement)
	{
		// Describe the data we want to copy into the default buffer.
		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData = static_cast<const void*>(&singleElement); 
		subResourceData.RowPitch = sizeof(T); 
		subResourceData.SlicePitch = subResourceData.RowPitch; 

		CopyData(&subResourceData);
	}

private:
	ConstantBufferStatic(const ConstantBufferStatic& rhs) = delete;
	ConstantBufferStatic& operator=(const ConstantBufferStatic& rhs) = delete;

	void CopyData(D3D12_SUBRESOURCE_DATA* data)
	{
		// Schedule to copy the data to the constant buffer resource. At a high level, the helper function UpdateSubresources
		// will copy the CPU memory into the intermediate upload heap. Then, using ID3D12CommandList::CopySubresourceRegion,
		// the intermediate upload heap data will be copied to the constant buffer.
		auto commandList = m_deviceResources->GetCommandList(); 

		auto _b = CD3DX12_RESOURCE_BARRIER::Transition(m_uploadBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
		GFX_THROW_INFO_ONLY(commandList->ResourceBarrier(1, &_b));

		GFX_THROW_INFO_ONLY(UpdateSubresources<1>(commandList, m_uploadBuffer.Get(), m_intermediateBuffer.Get(), 0, 0, 1, data));

		auto _b2 = CD3DX12_RESOURCE_BARRIER::Transition(m_uploadBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		GFX_THROW_INFO_ONLY(commandList->ResourceBarrier(1, &_b2));
	}

	void Initialize()
	{
		ASSERT(sizeof(T) <= 65536, "Size of T is too large");
		ASSERT(m_elementCount > 0, "Invalid to create a 0 sized constant buffer");
		ASSERT(m_elementCount * sizeof(T) <= 65536, "Element count is too large");

		size_t totalSize = (sizeof(T) * m_elementCount);
		m_elementByteSize = (totalSize + 255) & ~255;

		auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT); 
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize); 
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateCommittedResource( 
				&props,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_COMMON, // Can go ahead and put in STATE_COMMON - it will warn if you specify STATE_VERTEX_AND_CONSTANT_BUFFER because it said it will effectively put it in STATE_COMMON regardless
				nullptr,
				IID_PPV_ARGS(&m_uploadBuffer)
			)
		);

		// In order to copy CPU memory data into our default buffer, we need to create an intermediate upload heap. 
		auto props2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD); 
		auto desc2 = CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize); 
		GFX_THROW_INFO(  
			m_deviceResources->GetDevice()->CreateCommittedResource( 
				&props2, 
				D3D12_HEAP_FLAG_NONE, 
				&desc2, 
				D3D12_RESOURCE_STATE_GENERIC_READ, 
				nullptr, 
				IID_PPV_ARGS(&m_intermediateBuffer)
			)
		);
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> m_intermediateBuffer;

	// Constant buffers must be a multiple of the minimum hardware
	// allocation size (usually 256 bytes).  So round up to nearest
	// multiple of 256.  We do this by adding 255 and then masking off
	// the lower 2 bytes which store all bits < 256.
	// Example: Suppose byteSize = 300.
	// (300 + 255) & ~255
	// 555 & ~255
	// 0x022B & ~0x00ff
	// 0x022B & 0xff00
	// 0x0200
	// 512
	UINT m_elementByteSize = 0; // default to 0 - will get appropriately assigned in Initialize()
	unsigned int m_elementCount;
};

}