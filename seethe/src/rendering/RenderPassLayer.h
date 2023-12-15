#pragma once
#include "pch.h"
#include "DeviceResources.h"
#include "RenderItem.h"
#include "MeshGroup.h"
#include "utils/Log.h"

namespace seethe
{
class RenderPassLayer
{
public:
	inline RenderPassLayer(std::shared_ptr<DeviceResources> deviceResources, 
					std::shared_ptr<MeshGroupBase> meshGroup,
					const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
					D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
					std::string_view name = "Unnamed") :
		m_deviceResources(deviceResources),
		m_pipelineState(nullptr),
		m_topology(topology), 
		m_meshes(meshGroup),
		m_name(name),
		m_stencilRef(std::nullopt),
		m_active(true)
	{
		ASSERT(meshGroup != nullptr, "MeshGroup must be set in the constructor");
		CreatePSO(desc); 
	}
	inline RenderPassLayer(RenderPassLayer&& rhs) noexcept :
		m_deviceResources(rhs.m_deviceResources),
		PreWork(std::move(rhs.PreWork)),
		PostWork(std::move(rhs.PostWork)),
		m_renderItems(std::move(rhs.m_renderItems)),
		m_pipelineState(rhs.m_pipelineState),
		m_topology(rhs.m_topology),
		m_meshes(std::move(rhs.m_meshes)),
		m_name(std::move(rhs.m_name)),
		m_stencilRef(rhs.m_stencilRef),
		m_active(rhs.m_active)
	{}
	inline RenderPassLayer& operator=(RenderPassLayer&& rhs) noexcept
	{
		m_deviceResources = rhs.m_deviceResources;
		PreWork = std::move(rhs.PreWork);
		PostWork = std::move(rhs.PostWork);
		m_renderItems = std::move(rhs.m_renderItems);
		m_pipelineState = rhs.m_pipelineState;
		m_topology = rhs.m_topology;
		m_meshes = std::move(rhs.m_meshes);
		m_name = std::move(rhs.m_name);
		m_stencilRef = rhs.m_stencilRef;
		m_active = rhs.m_active;
		return *this;
	}
	~RenderPassLayer() noexcept = default;


	inline void CreatePSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState))
		);
	}

	inline void Update(const Timer& timer, int frameIndex)
	{
		for (RenderItem& item : m_renderItems)
		{
			if (item.IsActive())
				item.Update(timer, frameIndex);
		}

		// This is a virtual method that only actually does anything for DynamicMesh
		m_meshes->Update(frameIndex);
	}

	constexpr void PushBackRenderItem(RenderItem&& ri) noexcept { m_renderItems.push_back(std::move(ri)); }
	constexpr RenderItem& EmplaceBackRenderItem(unsigned int submeshIndex = 0, unsigned int instanceCount = 1) noexcept { return m_renderItems.emplace_back(submeshIndex, instanceCount); }

	ND constexpr std::vector<RenderItem>& GetRenderItems() noexcept { return m_renderItems; }
	ND constexpr const std::vector<RenderItem>& GetRenderItems() const noexcept { return m_renderItems; }
	ND inline ID3D12PipelineState* GetPSO() const noexcept { return m_pipelineState.Get(); }
	ND constexpr D3D12_PRIMITIVE_TOPOLOGY GetTopology() const noexcept { return m_topology; }
	ND inline std::shared_ptr<MeshGroupBase> GetMeshGroup() const noexcept { return m_meshes; }
	ND constexpr std::string_view GetName() const noexcept { return m_name; }
	ND constexpr bool IsActive() const noexcept { return m_active; }
	ND constexpr std::optional<unsigned int> GetStencilRef() const noexcept { return m_stencilRef; }

	constexpr void SetName(std::string_view name) noexcept { m_name = name; }
	constexpr void SetActive(bool active) noexcept { m_active = active; }
	
	constexpr void SetStencilRef(unsigned int value) noexcept { m_stencilRef = value; }
	constexpr void SetStencilRef(std::optional<unsigned int> value) noexcept { m_stencilRef = value; }


	// PreWork needs to return a bool: false -> signals early exit (i.e. do not make a Draw call for this layer)
	std::function<bool(const RenderPassLayer&, ID3D12GraphicsCommandList*)> PreWork = [](const RenderPassLayer&, ID3D12GraphicsCommandList*) { return true; };
	std::function<void(const RenderPassLayer&, ID3D12GraphicsCommandList*)> PostWork = [](const RenderPassLayer&, ID3D12GraphicsCommandList*) {};

private:
	// There is too much state to worry about copying (and expensive ?), so just delete copy operations until we find a good use case
	RenderPassLayer(const RenderPassLayer&) noexcept = delete;
	RenderPassLayer& operator=(const RenderPassLayer&) noexcept = delete;

	std::shared_ptr<DeviceResources> m_deviceResources;

	std::vector<RenderItem>						m_renderItems;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	D3D12_PRIMITIVE_TOPOLOGY					m_topology;
	std::shared_ptr<MeshGroupBase>				m_meshes; // shared_ptr because it is possible (if not likely) that different layers will want to reference the same mesh
	bool										m_active;
	std::optional<unsigned int>					m_stencilRef;

	// Name (for debug/profiling purposes)
	std::string m_name;
};
}