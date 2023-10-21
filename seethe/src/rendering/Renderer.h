#pragma once
#include "pch.h"
#include "Camera.h"
#include "ConstantBuffer.h"
#include "DeviceResources.h"
#include "InputLayout.h"
#include "Shader.h"
#include "simulation/Simulation.h"
#include "utils/MathHelper.h"
#include "utils/Timer.h"

#include "RenderPass.h"

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
class Renderer
{
public:
	Renderer(std::shared_ptr<DeviceResources> deviceResources);

	void Update(const Timer& timer, int frameIndex, const D3D12_VIEWPORT& vp);
	void Render(const Simulation& simulation, int frameIndex);
	void OnResize();

private:
	void CreateRootSignature();
	void CreateShadersAndInputLayout();
	void CreateShapeGeometry();
	void CreatePSOs();
	void CreateConstantBuffers();

	MeshData SphereMesh(float radius, uint32_t sliceCount, uint32_t stackCount);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);


	std::shared_ptr<DeviceResources> m_deviceResources;
	Camera m_camera;



	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
	std::unique_ptr<Shader> m_phongVS = nullptr;
	std::unique_ptr<Shader> m_phongPS = nullptr;
	std::unique_ptr<InputLayout> m_inputLayout = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso = nullptr;

	// Instancing
	std::unique_ptr<Shader> m_phongVSInstanced = nullptr;
	std::unique_ptr<Shader> m_phongPSInstanced = nullptr;
	std::unique_ptr<InputLayout> m_inputLayoutInstanced = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_psoInstanced = nullptr;
	std::unique_ptr<ConstantBufferT<InstanceDataArray>> m_instanceConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexUploadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = { 0, 0, 0 };
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = { 0, 0, DXGI_FORMAT_R16_UINT };
	unsigned int m_indexCount = 0;


	MaterialData m_materialData = {};

	std::unique_ptr<ConstantBufferT<PassConstants>> m_passConstantsBuffer;
	std::unique_ptr<ConstantBufferT<MaterialData>> m_materialsConstantBuffer;

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();


	// =======================================================================
	
	void RunComputeLayer(const ComputeLayer& layer, const Timer* timer);


	D3D12_VIEWPORT m_viewport = { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f }; // Dummy values
	D3D12_RECT m_scissorRect = { 0, 0, 1, 1 }; // Dummy values

	std::vector<RenderPass> m_renderPasses;
};
}