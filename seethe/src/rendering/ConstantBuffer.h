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
		m_mappedData(nullptr)
	{
		ASSERT(m_deviceResources != nullptr, "No device resources");
	}
	ConstantBufferBase(ConstantBufferBase&& rhs) noexcept :
		m_deviceResources(rhs.m_deviceResources),
		m_uploadBuffer(nullptr),
		m_mappedData(nullptr),
		m_elementByteSize(rhs.m_elementByteSize)
	{}
	ConstantBufferBase& operator=(ConstantBufferBase&& rhs) noexcept
	{
		m_deviceResources = rhs.m_deviceResources;
		m_uploadBuffer = nullptr;
		m_mappedData = nullptr;
		m_elementByteSize = rhs.m_elementByteSize;
		return *this;
	}
	~ConstantBufferBase()
	{
		if (m_uploadBuffer != nullptr)
			m_uploadBuffer->Unmap(0, nullptr);

		m_mappedData = nullptr;

		// Upload buffer might still be in use by the GPU, so do a delayed delete
		m_deviceResources->DelayedDelete(m_uploadBuffer);
		m_uploadBuffer = nullptr;
	}

	ND inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(unsigned int frameIndex) const noexcept
	{
		return m_uploadBuffer->GetGPUVirtualAddress() + static_cast<UINT64>(frameIndex) * m_elementByteSize;
	}


protected:
	ConstantBufferBase(const ConstantBufferBase& rhs) = delete;
	ConstantBufferBase& operator=(const ConstantBufferBase& rhs) = delete;


	std::shared_ptr<DeviceResources> m_deviceResources;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;
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
	UINT m_elementByteSize = 0; // default to 0 - will get appropriately assigned in the derived class
};

template<typename T>
class ConstantBuffer : public ConstantBufferBase
{
public:
	// Let the element count be the maximum by default
	// Maximum allowed constant buffer size is 4096 float4's which is 65536 bytes
	ConstantBuffer(std::shared_ptr<DeviceResources> deviceResources, unsigned int elementCount = 65536 / sizeof(T)) :
		ConstantBufferBase(deviceResources),
		m_elementCount(elementCount)
	{
		Initialize();
	}
	ConstantBuffer(ConstantBuffer&& rhs) :
		ConstantBufferBase(std::move(rhs)),
		m_elementCount(rhs.m_elementCount)
	{
		Initialize();
	}
	ConstantBuffer& operator=(ConstantBuffer&& rhs)
	{
		ConstantBufferBase::operator=(std::move(rhs));
		m_elementCount = rhs.m_elementCount;
		Initialize();
		return *this;
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
	ConstantBuffer(const ConstantBuffer& rhs) = delete;
	ConstantBuffer& operator=(const ConstantBuffer& rhs) = delete;

	void Initialize()
	{
		ASSERT(m_elementCount > 0, "Invalid to create a 0 sized constant buffer");
		ASSERT(m_elementCount <= 65536 / sizeof(T), "Element count is too large");

		size_t totalSize = (sizeof(T) * m_elementCount);
		m_elementByteSize = (totalSize + 255) & ~255;

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

	unsigned int m_elementCount;
};


}