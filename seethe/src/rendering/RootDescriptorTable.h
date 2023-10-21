#pragma once
#include "pch.h"


namespace seethe
{
class RootDescriptorTable
{
public:
	RootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle) noexcept :
		m_rootParameterIndex(rootParameterIndex),
		m_descriptorHandle(descriptorHandle)
	{}
	RootDescriptorTable(const RootDescriptorTable&) noexcept = default;
	RootDescriptorTable(RootDescriptorTable&&) noexcept = default;
	RootDescriptorTable& operator=(const RootDescriptorTable&) noexcept = default;
	RootDescriptorTable& operator=(RootDescriptorTable&&) noexcept = default;
	~RootDescriptorTable() noexcept = default;

	std::function<void(RootDescriptorTable*, const Timer&, int)> Update = [](RootDescriptorTable*, const Timer&, int) {};

	ND constexpr inline UINT RootParameterIndex() const noexcept { return m_rootParameterIndex; }
	ND constexpr inline D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle() const noexcept { return m_descriptorHandle; }

private:
	UINT m_rootParameterIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE m_descriptorHandle;
};

}