#pragma once
#include "pch.h"
#include "utils/Log.h"
#include "utils/Timer.h"
#include "RootSignature.h"
#include "RootConstantBufferView.h"
#include "RenderPassLayer.h"
#include "ComputeLayer.h"

namespace seethe
{
class RenderPass
{
public:
	inline RenderPass(std::shared_ptr<RootSignature> rootSig, std::string_view name = "Unnamed") noexcept :
		m_rootSignature(rootSig),
		m_name(name)
	{
		ASSERT(m_rootSignature != nullptr, "Root signature should not be nullptr");
	}
	inline RenderPass(std::shared_ptr<DeviceResources> deviceResources, const D3D12_ROOT_SIGNATURE_DESC& desc, std::string_view name = "Unnamed") :
		m_rootSignature(nullptr),
		m_name(name)
	{
		m_rootSignature = std::make_shared<RootSignature>(deviceResources, desc);
		ASSERT(m_rootSignature != nullptr, "Root signature should not be nullptr");
	}
	inline RenderPass(RenderPass&& rhs) noexcept :
		PreWork(std::move(rhs.PreWork)),
		PostWork(std::move(rhs.PostWork)),
		m_rootSignature(rhs.m_rootSignature),
		m_constantBufferViews(std::move(rhs.m_constantBufferViews)),
		m_renderPassLayers(std::move(rhs.m_renderPassLayers)),
		m_computeLayers(std::move(rhs.m_computeLayers)),
		m_name(std::move(rhs.m_name))
	{}
	inline RenderPass& operator=(RenderPass&& rhs) noexcept
	{
		PreWork = std::move(rhs.PreWork);
		PostWork = std::move(rhs.PostWork);
		m_rootSignature = rhs.m_rootSignature;
		m_constantBufferViews = std::move(rhs.m_constantBufferViews);
		m_renderPassLayers = std::move(rhs.m_renderPassLayers);
		m_computeLayers = std::move(rhs.m_computeLayers);
		m_name = std::move(rhs.m_name);

		return *this;
	}
	inline ~RenderPass() noexcept = default;

	inline void Update(const Timer& timer, int frameIndex)
	{
		// Loop over the constant buffer views to update per-pass constants
		for (auto& rcbv : m_constantBufferViews)
			rcbv.Update(timer, frameIndex);
	}


	ND inline RootSignature* GetRootSignature() const noexcept { return m_rootSignature.get(); }

	// See here for article on 'deducing this' pattern: https://devblogs.microsoft.com/cppblog/cpp23-deducing-this/
	template <class Self>
	ND constexpr auto&& GetRootConstantBufferViews(this Self&& self) noexcept { return std::forward<Self>(self).m_constantBufferViews; }
	template <class Self>
	ND constexpr auto&& GetRenderPassLayers(this Self&& self) noexcept { return std::forward<Self>(self).m_renderPassLayers; }
	template <class Self>
	ND constexpr auto&& GetComputeLayers(this Self&& self) noexcept { return std::forward<Self>(self).m_computeLayers; }

	ND constexpr std::string_view GetName() const noexcept { return m_name; }

	inline void SetRootSignature(std::shared_ptr<RootSignature> rs) noexcept { m_rootSignature = rs; }
	constexpr void SetName(std::string_view name) noexcept { m_name = name; }

	constexpr void PushBackRootConstantBufferView(RootConstantBufferView&& rcbv) noexcept { m_constantBufferViews.push_back(std::move(rcbv)); }
	constexpr void PushBackRenderPassLayer(RenderPassLayer&& rpl) noexcept { m_renderPassLayers.push_back(std::move(rpl)); }
	constexpr void PushBackComputeLayer(ComputeLayer&& cl) noexcept { m_computeLayers.push_back(std::move(cl)); }

	constexpr RootConstantBufferView& EmplaceBackRootConstantBufferView(UINT rootParameterIndex, ConstantBufferBase* cb) noexcept { return m_constantBufferViews.emplace_back(rootParameterIndex, cb); }
	RenderPassLayer& EmplaceBackRenderPassLayer(std::shared_ptr<DeviceResources> deviceResources,
		std::shared_ptr<MeshGroupBase> meshGroup,
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
		D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		std::string_view name = "Unnamed")
	{
		return m_renderPassLayers.emplace_back(deviceResources, meshGroup, desc, topology, name);
	}

	// Function pointers for Pre/Post-Work 
	// PreWork needs to return a bool: false -> signals early exit (i.e. do not make a Draw call for this layer)
	std::function<bool(RenderPass&, ID3D12GraphicsCommandList*)> PreWork = [](RenderPass&, ID3D12GraphicsCommandList*) { return true; };
	std::function<void(RenderPass&, ID3D12GraphicsCommandList*)> PostWork = [](RenderPass&, ID3D12GraphicsCommandList*) {};

private:
	// There is too much state to worry about copying (and expensive ?), so just delete copy operations until we find a good use case
	RenderPass(const RenderPass&) = delete;
	RenderPass& operator=(const RenderPass&) = delete;


	// Shared pointer because root signatures may be shared
	std::shared_ptr<RootSignature> m_rootSignature;

	// 0+ constant buffer views for per-pass constants
	std::vector<RootConstantBufferView> m_constantBufferViews;

	// 0+ render layers
	std::vector<RenderPassLayer> m_renderPassLayers;

	// 0+ render layers
	std::vector<ComputeLayer> m_computeLayers;

	// Name (for debug/profiling purposes)
	std::string m_name;
};
}