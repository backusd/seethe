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
	RenderPassLayer(std::shared_ptr<DeviceResources> deviceResources, 
					std::shared_ptr<MeshGroup> meshGroup,
					const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
					D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
					const std::string& name = "Unnamed") :
		m_deviceResources(deviceResources),
		m_pipelineState(nullptr),
		m_topology(topology), 
		m_meshes(meshGroup),
		m_name(name)
	{
		ASSERT(meshGroup != nullptr, "MeshGroup must be set in the constructor");
		CreatePSO(desc); 
	}
	RenderPassLayer(RenderPassLayer&& rhs) noexcept :
		m_deviceResources(rhs.m_deviceResources),
		PreWork(std::move(rhs.PreWork)),
		PostWork(std::move(rhs.PostWork)),
		m_renderItems(std::move(rhs.m_renderItems)),
		m_pipelineState(rhs.m_pipelineState),
		m_topology(rhs.m_topology),
		m_meshes(std::move(rhs.m_meshes)),
		m_name(std::move(rhs.m_name))
	{}
	RenderPassLayer& operator=(RenderPassLayer&& rhs) noexcept
	{
		m_deviceResources = rhs.m_deviceResources;
		PreWork = std::move(rhs.PreWork);
		PostWork = std::move(rhs.PostWork);
		m_renderItems = std::move(rhs.m_renderItems);
		m_pipelineState = rhs.m_pipelineState;
		m_topology = rhs.m_topology;
		m_meshes = std::move(rhs.m_meshes);
		m_name = std::move(rhs.m_name);
		return *this;
	}
	~RenderPassLayer() noexcept = default;


	inline void CreatePSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState))
		);
	}

	void RemoveRenderItem(RenderItem* ri)
	{
		if (ri != nullptr) LIKELY
		{
			for (unsigned int iii = 0; iii < m_renderItems.size(); ++iii)
			{
				if (&m_renderItems[iii] == ri) UNLIKELY
				{
					m_renderItems.erase(m_renderItems.begin() + iii);
					break;
				}
			}
		}
	}

	constexpr void PushBackRenderItem(RenderItem&& ri) noexcept { m_renderItems.push_back(std::move(ri)); }

	ND constexpr inline const std::vector<RenderItem>& GetRenderItems() const noexcept { return m_renderItems; }
	ND inline ID3D12PipelineState* GetPSO() const noexcept { return m_pipelineState.Get(); }
	ND constexpr inline D3D12_PRIMITIVE_TOPOLOGY GetTopology() const noexcept { return m_topology; }
	ND inline std::shared_ptr<MeshGroup> GetMeshGroup() const noexcept { return m_meshes; }
	ND constexpr inline const std::string& GetName() const noexcept { return m_name; }

	constexpr inline void SetName(const std::string& name) noexcept { m_name = name; }

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
	std::shared_ptr<MeshGroup>					m_meshes; // shared_ptr because it is possible (if not likely) that different layers will want to reference the same mesh

	// Name (for debug/profiling purposes)
	std::string m_name;
};
}