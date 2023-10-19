#include "Renderer.h"


namespace seethe
{
Renderer::Renderer(std::shared_ptr<DeviceResources> deviceResources) :
	m_deviceResources(deviceResources),
	m_camera()
{
	//m_camera.SetPosition(0.0f, 2.0f, -15.0f);
	m_camera.SetLens(0.25f * MathHelper::Pi, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
	m_camera.LookAt(DirectX::XMFLOAT3(0.0f, 0.0f, -10.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));

	CreateRootSignature();
	CreateShadersAndInputLayout();
	CreateShapeGeometry();
	CreatePSOs();
	CreateConstantBuffers();
}

void Renderer::OnResize()
{
	m_camera.SetLens(0.25f * MathHelper::Pi, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
}

void Renderer::Update(const Timer& timer, int frameIndex, const D3D12_VIEWPORT& vp)
{
	m_camera.UpdateViewMatrix();

	m_camera.SetLens(0.25f * MathHelper::Pi, vp.Width / vp.Height, 1.0f, 1000.0f);

	DirectX::XMMATRIX view = m_camera.GetView();
	DirectX::XMMATRIX proj = m_camera.GetProj();

	DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);

	DirectX::XMVECTOR _det = DirectX::XMMatrixDeterminant(view);
	DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&_det, view);

	_det = DirectX::XMMatrixDeterminant(proj);
	DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&_det, proj);

	_det = DirectX::XMMatrixDeterminant(viewProj);
	DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&_det, viewProj);

	PassConstants passConstants;

	DirectX::XMStoreFloat4x4(&passConstants.View, DirectX::XMMatrixTranspose(view));
	DirectX::XMStoreFloat4x4(&passConstants.InvView, DirectX::XMMatrixTranspose(invView));
	DirectX::XMStoreFloat4x4(&passConstants.Proj, DirectX::XMMatrixTranspose(proj));
	DirectX::XMStoreFloat4x4(&passConstants.InvProj, DirectX::XMMatrixTranspose(invProj));
	DirectX::XMStoreFloat4x4(&passConstants.ViewProj, DirectX::XMMatrixTranspose(viewProj));
	DirectX::XMStoreFloat4x4(&passConstants.InvViewProj, DirectX::XMMatrixTranspose(invViewProj));

	passConstants.EyePosW = m_camera.GetPosition3f(); 

	float height = static_cast<float>(m_deviceResources->GetHeight());
	float width = static_cast<float>(m_deviceResources->GetWidth());

	passConstants.RenderTargetSize = DirectX::XMFLOAT2(width, height); 
	passConstants.InvRenderTargetSize = DirectX::XMFLOAT2(1.0f / width, 1.0f / height); 
	passConstants.NearZ = 1.0f;
	passConstants.FarZ = 1000.0f;
	passConstants.TotalTime = timer.TotalTime();
	passConstants.DeltaTime = timer.DeltaTime();
	passConstants.AmbientLight = { 0.25f, 0.25f, 0.25f, 1.0f };

	passConstants.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	passConstants.Lights[0].Strength = { 0.9f, 0.9f, 0.9f };
	passConstants.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	passConstants.Lights[1].Strength = { 0.5f, 0.5f, 0.5f };
	passConstants.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	passConstants.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	m_passConstantsBuffer->CopyData(frameIndex, passConstants);


	// ======================================================================

	m_materialData.MaterialArray[0] = { DirectX::XMFLOAT4(DirectX::Colors::ForestGreen), { 0.02f, 0.02f, 0.02f }, 0.1f }; 

	//m_materialBuffer->CopyData(frameIndex, m_material);
	m_materialsConstantBuffer->CopyData(frameIndex, m_materialData);
}

void Renderer::Render(const Simulation& simulation, int frameIndex)
{
//	ObjectConstants o = {}; 
//	const DirectX::XMFLOAT3& p = simulation.Atoms()[0].position;
//	DirectX::XMStoreFloat4x4(&o.World, DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(p.x, p.y, p.z))); 
//
//	m_objectConstantsBuffer->CopyData(frameIndex, o); 


	InstanceDataArray d = {};
	int iii = 0;

	for (const auto& atom : simulation.Atoms())
	{
		const DirectX::XMFLOAT3& p = atom.position;
		DirectX::XMStoreFloat4x4(&d.Data[iii].World, DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(p.x, p.y, p.z)));
		
		d.Data[iii].MaterialIndex = iii % 2;

		++iii;
	}

	m_instanceConstantBuffer->CopyData(frameIndex, d);








	// ========================================================================

	auto commandList = m_deviceResources->GetCommandList();

	//commandList->SetPipelineState(m_pso.Get());
	commandList->SetPipelineState(m_psoInstanced.Get());

	commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);
//	commandList->IASetVertexBuffers(1, 1, &m_instanceBufferView);
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//commandList->SetGraphicsRootConstantBufferView(0, m_objectConstantsBuffer->GetGPUVirtualAddress(frameIndex));
	commandList->SetGraphicsRootConstantBufferView(0, m_instanceConstantBuffer->GetGPUVirtualAddress(frameIndex));
	//commandList->SetGraphicsRootConstantBufferView(1, m_materialBuffer->GetGPUVirtualAddress(frameIndex));
	commandList->SetGraphicsRootConstantBufferView(1, m_materialsConstantBuffer->GetGPUVirtualAddress(frameIndex));
	commandList->SetGraphicsRootConstantBufferView(2, m_passConstantsBuffer->GetGPUVirtualAddress(frameIndex));

	GFX_THROW_INFO_ONLY(commandList->DrawIndexedInstanced(m_indexCount, simulation.Atoms().size(), 0, 0, 0));
}

void Renderer::CreateRootSignature()
{
	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0); // ObjectCB 
	slotRootParameter[1].InitAsConstantBufferView(1); // MaterialCB  
	slotRootParameter[2].InitAsConstantBufferView(2); // PassConstants 

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC desc(3, slotRootParameter,
		0, nullptr, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr; 
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr; 
	HRESULT _hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()); 

	if (errorBlob != nullptr) 
	{
		LOG_ERROR("D3D12SerializeRootSignature() failed with message: {}", (char*)errorBlob->GetBufferPointer()); 
	}
	if (FAILED(_hr))
	{
		INFOMAN 
		throw GFX_EXCEPT(_hr);
	}

	GFX_THROW_INFO(
		m_deviceResources->GetDevice()->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSignature)
		)
	);
}
void Renderer::CreateShadersAndInputLayout()
{
	m_phongVS = std::make_unique<Shader>("src/shaders/output/PhongVS.cso");
	m_phongPS = std::make_unique<Shader>("src/shaders/output/PhongPS.cso");

	m_inputLayout = std::make_unique<InputLayout>(
		std::vector<D3D12_INPUT_ELEMENT_DESC>{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },		}
	);

	// Instancing
	m_phongVSInstanced = std::make_unique<Shader>("src/shaders/output/PhongInstancedVS.cso");
	m_phongPSInstanced = std::make_unique<Shader>("src/shaders/output/PhongInstancedPS.cso");

	m_inputLayoutInstanced = std::make_unique<InputLayout>(
		std::vector<D3D12_INPUT_ELEMENT_DESC>{ 
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//			{ "MATERIAL_INDEX", 0, DXGI_FORMAT_R32_UINT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		}
	);
}
void Renderer::CreateShapeGeometry()
{
	MeshData sphereMesh = SphereMesh(0.5f, 20, 20);

	m_indexCount = static_cast<unsigned int>(sphereMesh.indices.size());

	// Compute the vertex/index buffer view data
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = sizeof(Vertex) * static_cast<unsigned int>(sphereMesh.vertices.size());
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT; 
	m_indexBufferView.SizeInBytes = static_cast<unsigned int>(sphereMesh.indices.size()) * sizeof(std::uint16_t);

	// Create the vertex and index buffers with the initial data
	m_vertexBufferGPU = CreateDefaultBuffer(sphereMesh.vertices.data(), m_vertexBufferView.SizeInBytes, m_vertexUploadBuffer); 
	m_indexBufferGPU = CreateDefaultBuffer(sphereMesh.indices.data(), m_indexBufferView.SizeInBytes, m_indexUploadBuffer); 

	// Get the buffer locations
	m_vertexBufferView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress(); 
	m_indexBufferView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress(); 


	//
	// Instance data
	//
//	std::vector<std::uint32_t> instanceData = { 0, 1 };
//
//	m_instanceBufferView.StrideInBytes = sizeof(std::uint32_t);
//	m_instanceBufferView.SizeInBytes = sizeof(std::uint32_t) * static_cast<unsigned int>(instanceData.size());
//
//	m_instanceBufferGPU = CreateDefaultBuffer(instanceData.data(), m_instanceBufferView.SizeInBytes, m_instanceUploadBuffer);
//
//	m_instanceBufferView.BufferLocation = m_instanceBufferGPU->GetGPUVirtualAddress(); 
}
void Renderer::CreatePSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	desc.InputLayout = m_inputLayout->GetInputLayoutDesc();
	desc.pRootSignature = m_rootSignature.Get();
	desc.VS = m_phongVS->GetShaderByteCode();
	desc.PS = m_phongPS->GetShaderByteCode();
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
	desc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1;
	desc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0;
	desc.DSVFormat = m_deviceResources->GetDepthStencilFormat();
	GFX_THROW_INFO(m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pso)));

	//
	// Instancing....
	//
	ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	desc.InputLayout = m_inputLayoutInstanced->GetInputLayoutDesc();
	desc.pRootSignature = m_rootSignature.Get();
	desc.VS = m_phongVSInstanced->GetShaderByteCode();
	desc.PS = m_phongPSInstanced->GetShaderByteCode();
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
	desc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1;
	desc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0;
	desc.DSVFormat = m_deviceResources->GetDepthStencilFormat();
	GFX_THROW_INFO(m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_psoInstanced)));
}
void Renderer::CreateConstantBuffers()
{
	m_passConstantsBuffer = std::make_unique<ConstantBufferT<PassConstants>>(m_deviceResources);
//	m_materialBuffer = std::make_unique<ConstantBufferT<Material>>(m_deviceResources); 
	m_materialsConstantBuffer = std::make_unique<ConstantBufferT<MaterialData>>(m_deviceResources);
//	m_objectConstantsBuffer = std::make_unique<ConstantBufferT<ObjectConstants>>(m_deviceResources);

	// Instancing
	m_instanceConstantBuffer = std::make_unique<ConstantBufferT<InstanceDataArray>>(m_deviceResources);
}

MeshData Renderer::SphereMesh(float radius, uint32_t sliceCount, uint32_t stackCount)
{
	using namespace DirectX;

	MeshData mesh;

	Vertex topVertex({ 0.0f, +radius, 0.0f }, { 0.0f, +1.0f, 0.0f });
	Vertex bottomVertex({ 0.0f, -radius, 0.0f }, { 0.0f, -1.0f, 0.0f });

	mesh.vertices.push_back(topVertex);

	float phiStep = XM_PI / stackCount;
	float thetaStep = 2.0f * XM_PI / sliceCount;

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (uint32_t i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i * phiStep;

		// Vertices of ring.
		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			Vertex v; 

			// spherical to cartesian
			v.Pos.x = radius * sinf(phi) * cosf(theta);
			v.Pos.y = radius * cosf(phi);
			v.Pos.z = radius * sinf(phi) * sinf(theta);

			XMVECTOR p = XMLoadFloat3(&v.Pos);
			XMStoreFloat3(&v.Normal, XMVector3Normalize(p)); 

			mesh.vertices.push_back(v);
		}
	}

	mesh.vertices.push_back(bottomVertex);

	//
	// Compute indices for top stack.  The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.
	//

	for (std::uint16_t i = 1; i <= sliceCount; ++i)
	{
		mesh.indices.push_back(0);
		mesh.indices.push_back(i + 1);
		mesh.indices.push_back(i);
	}

	//
	// Compute indices for inner stacks (not connected to poles).
	//

	// Offset the indices to the index of the first vertex in the first ring.
	// This is just skipping the top pole vertex.
	std::uint16_t baseIndex = 1;
	std::uint16_t ringVertexCount = sliceCount + 1;
	for (std::uint16_t i = 0; i < stackCount - 2; ++i)
	{
		for (std::uint16_t j = 0; j < sliceCount; ++j)
		{
			mesh.indices.push_back(baseIndex + i * ringVertexCount + j);
			mesh.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			mesh.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	//
	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.
	//

	// South pole vertex was added last.
	std::uint16_t southPoleIndex = (std::uint16_t)mesh.vertices.size() - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (std::uint16_t i = 0; i < sliceCount; ++i)
	{
		mesh.indices.push_back(southPoleIndex);
		mesh.indices.push_back(baseIndex + i);
		mesh.indices.push_back(baseIndex + i + 1);
	}

	return mesh;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Renderer::CreateDefaultBuffer(const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer = nullptr;

	// Create the actual default buffer resource.
	auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	GFX_THROW_INFO(
		m_deviceResources->GetDevice()->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(defaultBuffer.GetAddressOf())
		)
	);

	// In order to copy CPU memory data into our default buffer, we need to create an intermediate upload heap. 
	auto props2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto desc2 = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	GFX_THROW_INFO(
		m_deviceResources->GetDevice()->CreateCommittedResource(
			&props2,
			D3D12_HEAP_FLAG_NONE,
			&desc2,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(uploadBuffer.GetAddressOf())
		)
	);

	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource. At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap. Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	auto commandList = m_deviceResources->GetCommandList();

	auto _b = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	GFX_THROW_INFO_ONLY(commandList->ResourceBarrier(1, &_b));

	UpdateSubresources<1>(commandList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

	auto _b2 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	GFX_THROW_INFO_ONLY(commandList->ResourceBarrier(1, &_b2));

	// MUST delete the upload buffer AFTER it is done being referenced by the GPU
//	Engine::DelayedDelete(uploadBuffer);

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.
	return defaultBuffer;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Renderer::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}


}