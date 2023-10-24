#pragma once
#include "pch.h"
#include "Camera.h"
#include "ConstantBuffer.h"
#include "DeviceResources.h"
#include "InputLayout.h"
#include "Shader.h"
#include "RenderPass.h"
#include "simulation/Simulation.h"
#include "utils/MathHelper.h"
#include "utils/Timer.h"

namespace seethe
{
class Renderer
{
public:
	Renderer(std::shared_ptr<DeviceResources> deviceResources, D3D12_VIEWPORT& viewport, D3D12_RECT& scissorRect);
	Renderer(Renderer&& rhs) noexcept :
		m_deviceResources(rhs.m_deviceResources),
		m_camera(std::move(rhs.m_camera)),
		m_viewport(rhs.m_viewport),
		m_scissorRect(rhs.m_scissorRect),
		m_renderPasses(std::move(rhs.m_renderPasses))
	{}
	Renderer& operator=(Renderer&& rhs) noexcept
	{
		m_deviceResources = rhs.m_deviceResources;
		m_camera = std::move(rhs.m_camera);
		m_viewport = rhs.m_viewport;
		m_scissorRect = rhs.m_scissorRect;
		m_renderPasses = std::move(rhs.m_renderPasses);
		return *this;
	}
	~Renderer() noexcept = default;

	void Update(const Timer& timer, int frameIndex);
	void Render(const Simulation& simulation, int frameIndex);

	ND constexpr inline Camera& GetCamera() noexcept { return m_camera; }
	ND constexpr inline const Camera& GetCamera() const noexcept { return m_camera; }

	constexpr inline void SetViewport(D3D12_VIEWPORT& vp) noexcept { m_viewport = vp; }
	constexpr inline void SetScissorRect(D3D12_RECT& rect) noexcept { m_scissorRect = rect; }

	constexpr void PushBackRenderPass(RenderPass&& pass) noexcept { m_renderPasses.push_back(std::move(pass)); }
	RenderPass& EmplaceBackRenderPass(std::shared_ptr<RootSignature> rootSig, const std::string& name = "Unnamed") noexcept { return m_renderPasses.emplace_back(rootSig, name); }
	RenderPass& EmplaceBackRenderPass(std::shared_ptr<DeviceResources> deviceResources, const D3D12_ROOT_SIGNATURE_DESC& desc, const std::string& name = "Unnamed") noexcept { return m_renderPasses.emplace_back(deviceResources, desc, name); }

private:
	// There is too much state to worry about copying (and expensive ?), so just delete copy operations until we find a good use case
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	void RunComputeLayer(const ComputeLayer& layer, const Timer* timer, int frameIndex);

	std::shared_ptr<DeviceResources> m_deviceResources;

	// Note: we do NOT allow camera to be a reference to a Camera object controlled by
	// the application. The reason being is that we must enforce each Renderer instance to
	// have its own camera (no sharing a camera). The application is responsible for calling
	// GetCamera() and updating its position/orientation when necessary
	Camera m_camera;

	// Have the viewport and scissor rect be controlled by the application. We use references
	// here because neither of these should ever be allowed to be null
	D3D12_VIEWPORT& m_viewport;
	D3D12_RECT& m_scissorRect;

	std::vector<RenderPass> m_renderPasses;
};
}