#pragma once
#include "pch.h"
#include "window/MainWindow.h"
#include "simulation/Simulation.h"
#include "rendering/DeviceResources.h"
#include "rendering/DescriptorVector.h"
#include "rendering/Renderer.h"
#include "utils/Timer.h"
#include "utils/Constants.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

#define MAX_INSTANCES 100
#define NUM_MATERIALS 10

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

struct Material
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
};

struct MaterialData
{
	Material MaterialArray[NUM_MATERIALS];
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
class Application
{
public:
	Application();
	Application(const Application&) = delete;
	Application(Application&&) = delete;
	Application& operator=(const Application&) = delete;
	Application& operator=(Application&&) = delete;
	~Application();

	int Run();

	ND LRESULT MainWindowOnCreate(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnClose(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnLButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnLButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnLButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnRButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnRButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnRButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnResize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnMouseMove(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMouseEnter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMouseLeave(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMouseWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMouseHWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnChar(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnKeyUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnKeyDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnKillFocus(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;


private:
	void Update();
	void Render();
	void RenderUI();
	void Present();

	std::unique_ptr<MainWindow> m_mainWindow;
	std::shared_ptr<DeviceResources> m_deviceResources;
	Timer m_timer;
	Simulation m_simulation;
	std::unique_ptr<Renderer> m_renderer;

	// Rendering resources
	std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, g_numFrameResources> m_allocators;
	int m_currentFrameIndex = 0;
	std::array<UINT64, g_numFrameResources> m_fences = {};
	std::unique_ptr<DescriptorVector> m_descriptorVector = nullptr;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;






	void InitializeRenderPasses();

	MeshData SphereMesh(float radius, uint32_t sliceCount, uint32_t stackCount);

	std::unique_ptr<Shader> m_phongVSInstanced = nullptr;
	std::unique_ptr<Shader> m_phongPSInstanced = nullptr;
	std::unique_ptr<InputLayout> m_inputLayoutInstanced = nullptr;
	std::unique_ptr<ConstantBuffer<InstanceDataArray>> m_instanceConstantBuffer;

	// Box
	std::unique_ptr<Shader> m_solidVS = nullptr;
	std::unique_ptr<Shader> m_solidPS = nullptr;
	std::unique_ptr<InputLayout> m_solidInputLayout = nullptr;
	std::unique_ptr<ConstantBuffer<DirectX::XMFLOAT4X4>> m_boxConstantBuffer;

	std::unique_ptr<ConstantBuffer<PassConstants>> m_passConstantsBuffer;
	std::unique_ptr<ConstantBuffer<MaterialData>> m_materialsConstantBuffer;
};
}