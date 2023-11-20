#pragma once
#include "pch.h"
#include "DeviceResources.h"
#include "RenderItem.h"
#include "RootSignature.h"
#include "utils/Timer.h"
#include "utils/Log.h"


namespace seethe
{
class ComputeLayer
{
public:
	ComputeLayer(std::shared_ptr<DeviceResources> deviceResources, 
				 std::shared_ptr<RootSignature> rootSig,
				 const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
				 const std::string& name = "Unnamed") :
		m_deviceResources(deviceResources),
		m_rootSignature(rootSig),
		m_pipelineState(nullptr),
		m_name(name)
	{
		ASSERT(m_rootSignature != nullptr, "Root Signature should not be nullptr");
		CreatePSO(desc);
	}
	ComputeLayer(std::shared_ptr<DeviceResources> deviceResources,
				 const D3D12_ROOT_SIGNATURE_DESC& rootSigDesc,
				 const D3D12_COMPUTE_PIPELINE_STATE_DESC& computePSODesc,
				 const std::string& name = "Unnamed") :
		m_deviceResources(deviceResources),
		m_rootSignature(nullptr),
		m_pipelineState(nullptr),
		m_name(name)
	{
		m_rootSignature = std::make_shared<RootSignature>(m_deviceResources, rootSigDesc);
		CreatePSO(computePSODesc);
	}
	ComputeLayer(ComputeLayer&& rhs) noexcept :
		m_deviceResources(rhs.m_deviceResources),
		PreWork(std::move(rhs.PreWork)),
		PostWork(std::move(rhs.PostWork)),
		m_computeItems(std::move(rhs.m_computeItems)),
		m_pipelineState(rhs.m_pipelineState),
		m_name(std::move(rhs.m_name))
	{}
	ComputeLayer& operator=(ComputeLayer&& rhs) noexcept
	{
		m_deviceResources = rhs.m_deviceResources;
		PreWork = std::move(rhs.PreWork);
		PostWork = std::move(rhs.PostWork);
		m_computeItems = std::move(rhs.m_computeItems);
		m_pipelineState = rhs.m_pipelineState;
		m_name = std::move(rhs.m_name);
		return *this;
	}
	~ComputeLayer() noexcept = default;

	inline void CreatePSO(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc)
	{
		GFX_THROW_INFO(m_deviceResources->GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_pipelineState)));
	}

	void Update(const Timer& timer, int frameIndex)
	{
		for (ComputeItem& item : m_computeItems)
			item.Update(timer, frameIndex);
	}

	constexpr void PushBackComputeItem(ComputeItem&& ci) noexcept { m_computeItems.push_back(std::move(ci)); }
	constexpr ComputeItem& EmplaceBackComputeItem(unsigned int threadGroupCountX = 1, unsigned int threadGroupCountY = 1, unsigned int threadGroupCountZ = 1) noexcept 
	{ 
		return m_computeItems.emplace_back(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	ND constexpr inline std::vector<ComputeItem>& GetComputeItems() noexcept { return m_computeItems; }
	ND constexpr inline const std::vector<ComputeItem>& GetComputeItems() const noexcept { return m_computeItems; }
	ND inline ID3D12PipelineState* GetPSO() const noexcept { return m_pipelineState.Get(); }
	ND inline RootSignature* GetRootSignature() const noexcept { return m_rootSignature.get(); }
	ND constexpr inline std::string& GetName() noexcept { return m_name; }
	ND constexpr inline const std::string& GetName() const noexcept { return m_name; }
	ND constexpr inline bool IsActive() const noexcept { return m_active; }

	constexpr inline void SetName(const std::string& name) noexcept { m_name = name; }
	constexpr inline void SetActive(bool active) noexcept { m_active = active; }

	// PreWork needs to return a bool: false -> signals early exit (i.e. do not call Dispatch for this RenderLayer)
	// Also, because a ComputeLayer can be executed during the Update phase, it can get access to the Timer. However, 
	// if the ComputeLayer is execute during a RenderPass, then it will NOT have access to the timer and the timer 
	// parameter will be nullptr
	std::function<bool(const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer*, int)> PreWork = [](const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer*, int) { return true; };
	std::function<void(const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer*, int)> PostWork = [](const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer*, int) {};

private:
	// There is too much state to worry about copying, so just delete copy operations until we find a good use case
	ComputeLayer(const ComputeLayer&) noexcept = delete;
	ComputeLayer& operator=(const ComputeLayer&) noexcept = delete;

	std::shared_ptr<DeviceResources> m_deviceResources;

	// Shared pointer because root signatures may be shared
	std::shared_ptr<RootSignature>				m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	std::vector<ComputeItem>					m_computeItems;
	bool										m_active = true;

	// Name (for debug/profiling purposes)
	std::string m_name;
};
}