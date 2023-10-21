#pragma once
#include "pch.h"
#include "RootConstantBufferView.h"
#include "RootDescriptorTable.h"
#include "utils/Log.h"
#include "utils/Timer.h"

namespace seethe
{
//
// RenderComputeItemBase
//
class RenderComputeItemBase
{
public:
	void Update(const Timer& timer, int frameIndex)
	{
		// Loop over the constant buffer views and descriptor tables to update them
		for (auto& rcbv : m_constantBufferViews)
			rcbv.Update(timer, frameIndex);

		for (auto& dt : m_descriptorTables)
			dt.Update(&dt, timer, frameIndex);
	}

	constexpr inline void PushBackRootConstantBufferView(RootConstantBufferView&& rcbv) noexcept { m_constantBufferViews.push_back(std::move(rcbv)); }
	constexpr inline void PushBackRootConstantBufferView(const RootConstantBufferView& rcbv) noexcept { m_constantBufferViews.push_back(rcbv); }
	constexpr inline RootConstantBufferView& EmplaceBackRootConstantBufferView(UINT rootParameterIndex, ConstantBuffer* cb) noexcept { return m_constantBufferViews.emplace_back(rootParameterIndex, cb); }

	constexpr inline void PushBackRootDescriptorTable(RootDescriptorTable&& rdt) noexcept { m_descriptorTables.push_back(std::move(rdt)); }
	constexpr inline void PushBackRootDescriptorTable(const RootDescriptorTable& rdt) noexcept { m_descriptorTables.push_back(rdt); }
	constexpr inline RootDescriptorTable& EmplaceBackRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle) noexcept { return m_descriptorTables.emplace_back(rootParameterIndex, descriptorHandle); }

	ND constexpr inline const std::vector<RootConstantBufferView>& GetRootConstantBufferViews() const noexcept { return m_constantBufferViews; }
	ND constexpr inline const std::vector<RootDescriptorTable>& GetRootDescriptorTables() const noexcept { return m_descriptorTables; }

protected:
	// 0+ constant buffer views for per-item constants
	std::vector<RootConstantBufferView> m_constantBufferViews;

	// 0+ descriptor tables for per-item resources
	std::vector<RootDescriptorTable> m_descriptorTables;
};



//
// RenderItem
//
class RenderItem : public RenderComputeItemBase
{
public:
	constexpr RenderItem(unsigned int submeshIndex = 0) noexcept : m_submeshIndex(submeshIndex) {}
	constexpr RenderItem(RenderItem&& rhs) noexcept : 
		RenderComputeItemBase(std::move(rhs)), 
		m_submeshIndex(rhs.m_submeshIndex)
	{
		LOG_WARN("{}", "RenderItem Move Constructor called, but this method has not been tested.");
	}
	constexpr RenderItem& operator=(RenderItem&& rhs) noexcept
	{
		LOG_WARN("{}", "RenderItem Move Assignment operator called, but this method has not been tested.");

		RenderComputeItemBase::operator=(std::move(rhs));
		m_submeshIndex = rhs.m_submeshIndex;
		return *this;
	}

	ND constexpr inline unsigned int GetSubmeshIndex() const noexcept { return m_submeshIndex; }
	constexpr inline void SetSubmeshIndex(unsigned int index) noexcept { m_submeshIndex = index; }

private:
	RenderItem(const RenderItem&) = delete;
	RenderItem& operator=(const RenderItem&) = delete;

	// The PSO will hold and bind the mesh-group for all of the render items it will render.
	// Here, we just need to keep track of which submesh index the render item references
	unsigned int m_submeshIndex;
};



//
// ComputeItem
//
class ComputeItem : public RenderComputeItemBase
{
public:
	constexpr ComputeItem(unsigned int threadGroupCountX = 1, 
						  unsigned int threadGroupCountY = 1, 
						  unsigned int threadGroupCountZ = 1) noexcept :
		m_threadGroupCountX(threadGroupCountX),
		m_threadGroupCountY(threadGroupCountY),
		m_threadGroupCountZ(threadGroupCountZ)
	{}
	constexpr ComputeItem(ComputeItem&& rhs) noexcept :
		RenderComputeItemBase(std::move(rhs)),
		m_threadGroupCountX(rhs.m_threadGroupCountX),
		m_threadGroupCountY(rhs.m_threadGroupCountY),
		m_threadGroupCountZ(rhs.m_threadGroupCountZ)
	{
		LOG_WARN("{}", "ComputeItem Move Constructor called, but this method has not been tested.");
	}
	constexpr ComputeItem& operator=(ComputeItem&& rhs) noexcept
	{
		LOG_WARN("{}", "ComputeItem Move Assignment operator called, but this method has not been tested.");

		RenderComputeItemBase::operator=(std::move(rhs));
		m_threadGroupCountX = rhs.m_threadGroupCountX;
		m_threadGroupCountY = rhs.m_threadGroupCountY;
		m_threadGroupCountZ = rhs.m_threadGroupCountZ;
		return *this;
	}

	ND constexpr inline unsigned int GetThreadGroupCountX() const noexcept { return m_threadGroupCountX; }
	ND constexpr inline unsigned int GetThreadGroupCountY() const noexcept { return m_threadGroupCountY; }
	ND constexpr inline unsigned int GetThreadGroupCountZ() const noexcept { return m_threadGroupCountZ; }

	constexpr inline void SetThreadGroupCountX(unsigned int count) noexcept { m_threadGroupCountX = count; }
	constexpr inline void SetThreadGroupCountY(unsigned int count) noexcept { m_threadGroupCountY = count; }
	constexpr inline void SetThreadGroupCountZ(unsigned int count) noexcept { m_threadGroupCountZ = count; }

private:
	ComputeItem(const ComputeItem&) = delete;
	ComputeItem& operator=(const ComputeItem&) = delete;

	// Thread group counts to use when call Dispatch
	unsigned int m_threadGroupCountX = 1;
	unsigned int m_threadGroupCountY = 1;
	unsigned int m_threadGroupCountZ = 1;
};



}