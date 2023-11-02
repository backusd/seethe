#pragma once
#include "pch.h"
#include "rendering/Renderer.h"
#include "application/rendering/Materials.h"
#include "simulation/Simulation.h"
#include "utils/Timer.h"



// Max constant buffer size is 4096 float4's
// Our current InstanceData is basically 5 float4's
// 4096 / 5 = 819.2
#define MAX_INSTANCES 819

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = seethe::MathHelper::Identity4x4();
};

struct InstanceData
{
	DirectX::XMFLOAT4X4 World;
	std::uint32_t MaterialIndex;
	std::uint32_t Pad0;
	std::uint32_t Pad1;
	std::uint32_t Pad2;
};

struct InstanceDataArray
{
	InstanceData Data[MAX_INSTANCES];
};

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
};
struct SolidColorVertex
{
	DirectX::XMFLOAT4 Pos;
	DirectX::XMFLOAT4 Color;
};

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<std::uint16_t> indices;
};


static constexpr int MaxLights = 16;

struct Light
{
	DirectX::XMFLOAT3   Strength = { 0.5f, 0.5f, 0.5f };
	float               FalloffStart = 1.0f;                // point/spot light only
	DirectX::XMFLOAT3   Direction = { 0.0f, -1.0f, 0.0f };  // directional/spot light only
	float               FalloffEnd = 10.0f;                 // point/spot light only
	DirectX::XMFLOAT3   Position = { 0.0f, 0.0f, 0.0f };    // point/spot light only
	float               SpotPower = 64.0f;                  // spot light only
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light Lights[MaxLights];
};

namespace seethe
{
class SimulationWindow
{
public:
	SimulationWindow(std::shared_ptr<DeviceResources> deviceResources, 
		Simulation& simulation, Materials& materials,
		float top, float left, float height, float width) noexcept;

	void Update(const Timer& timer, int frameIndex);
	inline void Render(int frameIndex) { m_renderer->Render(m_simulation, frameIndex); }
	inline void SetWindow(float top, float left, float height, float width) noexcept
	{
		m_viewport = { left, top, width, height, 0.0f, 1.0f };
		m_scissorRect = { static_cast<long>(left), static_cast<long>(top), static_cast<long>(left + width), static_cast<long>(top + height) };
	}

	ND bool OnLButtonDown(float x, float y) noexcept { return OnButtonDownImpl(m_mouseLButtonDown, x, y, [this]() { HandleLButtonDown(); }); }
	ND bool OnLButtonUp(float x, float y) noexcept { return OnButtonUpImpl(m_mouseLButtonDown, x, y, [this]() { HandleLButtonUp(); }); }
	ND bool OnLButtonDoubleClick(float x, float y) noexcept { return OnButtonDoubleClickImpl(x, y, [this]() { HandleLButtonDoubleClick(); }); }
	ND bool OnMButtonDown(float x, float y) noexcept { return OnButtonDownImpl(m_mouseMButtonDown, x, y, [this]() { HandleMButtonDown(); }); }
	ND bool OnMButtonUp(float x, float y) noexcept { return OnButtonUpImpl(m_mouseMButtonDown, x, y, [this]() { HandleMButtonUp(); }); }
	ND bool OnMButtonDoubleClick(float x, float y) noexcept { return OnButtonDoubleClickImpl(x, y, [this]() { HandleMButtonDoubleClick(); }); }
	ND bool OnRButtonDown(float x, float y) noexcept { return OnButtonDownImpl(m_mouseRButtonDown, x, y, [this]() { HandleRButtonDown(); }); }
	ND bool OnRButtonUp(float x, float y) noexcept { return OnButtonUpImpl(m_mouseRButtonDown, x, y, [this]() { HandleRButtonUp(); }); }
	ND bool OnRButtonDoubleClick(float x, float y) noexcept { return OnButtonDoubleClickImpl(x, y, [this]() { HandleRButtonDoubleClick(); }); }
	ND bool OnX1ButtonDown(float x, float y) noexcept { return OnButtonDownImpl(m_mouseX1ButtonDown, x, y, [this]() { HandleX1ButtonDown(); }); }
	ND bool OnX1ButtonUp(float x, float y) noexcept { return OnButtonUpImpl(m_mouseX1ButtonDown, x, y, [this]() { HandleX1ButtonUp(); }); }
	ND bool OnX1ButtonDoubleClick(float x, float y) noexcept { return OnButtonDoubleClickImpl(x, y, [this]() { HandleX1ButtonDoubleClick(); }); }
	ND bool OnX2ButtonDown(float x, float y) noexcept { return OnButtonDownImpl(m_mouseX2ButtonDown, x, y, [this]() { HandleX2ButtonDown(); }); }
	ND bool OnX2ButtonUp(float x, float y) noexcept { return OnButtonUpImpl(m_mouseX2ButtonDown, x, y, [this]() { HandleX2ButtonUp(); }); }
	ND bool OnX2ButtonDoubleClick(float x, float y) noexcept { return OnButtonDoubleClickImpl(x, y, [this]() { HandleX2ButtonDoubleClick(); }); }
	ND bool OnMouseMove(float x, float y) noexcept;
	ND bool OnMouseWheelVertical(int wheelDelta) noexcept;
	ND bool OnMouseWheelHorizontal(int wheelDelta) noexcept;

	ND bool OnKeyDown(unsigned int virtualKeyCode) noexcept;
	ND bool OnKeyUp(unsigned int virtualKeyCode) noexcept;
	ND bool OnChar(char c) noexcept;


	ND constexpr inline bool ContainsPoint(float x, float y) const noexcept { return m_viewport.TopLeftX <= x && m_viewport.TopLeftY <= y && m_viewport.TopLeftX + m_viewport.Width >= x && m_viewport.TopLeftY + m_viewport.Height >= y; }
	ND constexpr inline bool Dragging() const noexcept { return m_mouseLButtonDown || m_mouseMButtonDown || m_mouseRButtonDown || m_mouseX1ButtonDown || m_mouseX2ButtonDown; }

	constexpr inline void SetMaterialsDirtyFlag() noexcept { m_materialsDirtyFlag = g_numFrameResources; }

private:
	bool OnButtonDownImpl(bool& buttonFlag, float x, float y, std::function<void()>&& handler);
	bool OnButtonUpImpl(bool& buttonFlag, float x, float y, std::function<void()>&& handler);
	bool OnButtonDoubleClickImpl(float x, float y, std::function<void()>&& handler);

	void HandleLButtonDown() noexcept;
	void HandleLButtonUp() noexcept;
	void HandleLButtonDoubleClick() noexcept;
	void HandleMButtonDown() noexcept;
	void HandleMButtonUp() noexcept;
	void HandleMButtonDoubleClick() noexcept;
	void HandleRButtonDown() noexcept;
	void HandleRButtonUp() noexcept;
	void HandleRButtonDoubleClick() noexcept;
	void HandleX1ButtonDown() noexcept;
	void HandleX1ButtonUp() noexcept;
	void HandleX1ButtonDoubleClick() noexcept;
	void HandleX2ButtonDown() noexcept;
	void HandleX2ButtonUp() noexcept;
	void HandleX2ButtonDoubleClick() noexcept;
	void HandleMouseMove(float x, float y) noexcept;
	void HandleMouseWheelVertical(int wheelDelta) noexcept;
	void HandleMouseWheelHorizontal(int wheelDelta) noexcept;
	void HandleKeyDown(unsigned int virtualKeyCode) noexcept;
	void HandleKeyUp(unsigned int virtualKeyCode) noexcept;
	void HandleChar(char c) noexcept;

	ND constexpr inline bool KeyboardKeyIsPressed() const noexcept 
	{ 
		return m_arrowLeftIsPressed || m_arrowRightIsPressed || m_arrowUpIsPressed || m_arrowDownIsPressed ||
			   m_keyAIsPressed || m_keyDIsPressed || m_keyEIsPressed || m_keyQIsPressed || m_keySIsPressed || m_keyWIsPressed;
	}





	void InitializeRenderPasses();



	std::shared_ptr<DeviceResources> m_deviceResources;
	std::unique_ptr<Renderer> m_renderer;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	Simulation& m_simulation;

	Materials& m_materials;
	unsigned int m_materialsDirtyFlag = g_numFrameResources;


	MeshData SphereMesh(float radius, uint32_t sliceCount, uint32_t stackCount);

	std::unique_ptr<Shader> m_phongVSInstanced = nullptr;
	std::unique_ptr<Shader> m_phongPSInstanced = nullptr;
	std::unique_ptr<InputLayout> m_inputLayoutInstanced = nullptr;
	std::unique_ptr<ConstantBuffer<InstanceData>> m_instanceConstantBuffer;

	std::vector<InstanceData> m_instanceData;

	// Box
	std::unique_ptr<Shader> m_solidVS = nullptr;
	std::unique_ptr<Shader> m_solidPS = nullptr;
	std::unique_ptr<InputLayout> m_solidInputLayout = nullptr;
	std::unique_ptr<ConstantBuffer<DirectX::XMFLOAT4X4>> m_boxConstantBuffer;

	std::unique_ptr<ConstantBuffer<PassConstants>> m_passConstantsBuffer;
	std::unique_ptr<ConstantBuffer<Materials>> m_materialsConstantBuffer;


	// Mouse Tracking
	bool m_mouseLButtonDown = false;
	bool m_mouseMButtonDown = false;
	bool m_mouseRButtonDown = false;
	bool m_mouseX1ButtonDown = false;
	bool m_mouseX2ButtonDown = false;
	DirectX::XMFLOAT2 m_mouseLastPos = DirectX::XMFLOAT2();

	// Keyboard Tracking
	bool m_arrowLeftIsPressed = false;
	bool m_arrowRightIsPressed = false;
	bool m_arrowUpIsPressed = false;
	bool m_arrowDownIsPressed = false;
	bool m_shiftIsPressed = false;
	bool m_keyWIsPressed = false;
	bool m_keyAIsPressed = false;
	bool m_keySIsPressed = false;
	bool m_keyDIsPressed = false;
	bool m_keyQIsPressed = false;
	bool m_keyEIsPressed = false;
};
}