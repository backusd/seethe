#pragma once
#include "pch.h"
#include "rendering/Renderer.h"
#include "simulation/Simulation.h"
#include "utils/Timer.h"



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
class SimulationWindow
{
public:
	SimulationWindow(std::shared_ptr<DeviceResources> deviceResources, 
		Simulation& simulation,
		float top, float left, float height, float width) noexcept;

	inline void Update(const Timer& timer, int frameIndex) { m_renderer->Update(timer, frameIndex); }
	inline void Render(int frameIndex) { m_renderer->Render(m_simulation, frameIndex); }
	inline void SetWindow(float top, float left, float height, float width) noexcept
	{
		m_viewport = { left, top, width, height, 0.0f, 1.0f };
		m_scissorRect = { static_cast<long>(left), static_cast<long>(top), static_cast<long>(left + width), static_cast<long>(top + height) };
	}

private:
	void InitializeRenderPasses();



	std::shared_ptr<DeviceResources> m_deviceResources;
	std::unique_ptr<Renderer> m_renderer;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	Simulation& m_simulation;



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