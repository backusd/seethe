#include "Renderer.h"


namespace seethe
{
Renderer::Renderer(std::shared_ptr<DeviceResources> deviceResources, Simulation& simulation) :
	m_deviceResources(deviceResources),
	m_camera(),
	m_simulation(simulation)
{
	//m_camera.SetPosition(0.0f, 2.0f, -15.0f);
	m_camera.SetLens(0.25f * MathHelper::Pi, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
	m_camera.LookAt(DirectX::XMFLOAT3(0.0f, 0.0f, -10.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));

	InitializeRenderPasses();
}

void Renderer::InitializeRenderPasses()
{
	// Root parameter can be a table, root descriptor or root constants.
	// *** Perfomance TIP: Order from most frequent to least frequent.
	CD3DX12_ROOT_PARAMETER slotRootParameter[3]; 
	slotRootParameter[0].InitAsConstantBufferView(0); // ObjectCB  
	slotRootParameter[1].InitAsConstantBufferView(1); // MaterialCB   
	slotRootParameter[2].InitAsConstantBufferView(2); // PassConstants 

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	std::shared_ptr<RootSignature> rootSig1 = std::make_shared<RootSignature>(m_deviceResources, rootSigDesc);
	RenderPass& pass1 = m_renderPasses.emplace_back(rootSig1, "Render Pass #1");

	m_passConstantsBuffer = std::make_unique<ConstantBuffer<PassConstants>>(m_deviceResources);
	RootConstantBufferView& perPassConstantsCBV = pass1.EmplaceBackRootConstantBufferView(2, m_passConstantsBuffer.get());
	perPassConstantsCBV.Update = [this](const Timer& timer, int frameIndex)
		{
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
		};


	m_materialsConstantBuffer = std::make_unique<ConstantBuffer<MaterialData>>(m_deviceResources);
	RootConstantBufferView& materialsCBV = pass1.EmplaceBackRootConstantBufferView(1, m_materialsConstantBuffer.get());
	materialsCBV.Update = [this](const Timer& timer, int frameIndex)
		{
			MaterialData m_materialData = {};
			m_materialData.MaterialArray[0] = { DirectX::XMFLOAT4(DirectX::Colors::ForestGreen), { 0.02f, 0.02f, 0.02f }, 0.1f };

			m_materialsConstantBuffer->CopyData(frameIndex, m_materialData);
		};



	MeshData sphereMesh = SphereMesh(0.5f, 20, 20); 
	
	std::vector<std::vector<Vertex>> vertices;
	vertices.push_back(std::move(sphereMesh.vertices));

	std::vector<std::vector<std::uint16_t>> indices;
	indices.push_back(std::move(sphereMesh.indices));
	
	std::shared_ptr<MeshGroupT<Vertex>> meshGroup = std::make_shared<MeshGroupT<Vertex>>(m_deviceResources, vertices, indices);



	m_phongVSInstanced = std::make_unique<Shader>("src/shaders/output/PhongInstancedVS.cso");
	m_phongPSInstanced = std::make_unique<Shader>("src/shaders/output/PhongInstancedPS.cso");

	m_inputLayoutInstanced = std::make_unique<InputLayout>(
		std::vector<D3D12_INPUT_ELEMENT_DESC>{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		}
	);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = m_inputLayoutInstanced->GetInputLayoutDesc();  
	psoDesc.pRootSignature = rootSig1->Get();
	psoDesc.VS = m_phongVSInstanced->GetShaderByteCode(); 
	psoDesc.PS = m_phongPSInstanced->GetShaderByteCode();  
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); 
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); 
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); 
	psoDesc.SampleMask = UINT_MAX; 
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; 
	psoDesc.NumRenderTargets = 1; 
	psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat(); 
	psoDesc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1; 
	psoDesc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0; 
	psoDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat(); 


	//RenderPassLayer layer1(m_deviceResources, meshGroup, psoDesc, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, "Layer #1");
	RenderPassLayer& layer1 = pass1.EmplaceBackRenderPassLayer(m_deviceResources, meshGroup, psoDesc, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, "Layer #1");


	m_instanceConstantBuffer = std::make_unique<ConstantBuffer<InstanceDataArray>>(m_deviceResources);


	for (int iii = 0; iii < 1; ++iii)
	{


		RenderItem& sphereRI = layer1.EmplaceBackRenderItem();
		sphereRI.SetInstanceCount(static_cast<unsigned int>(m_simulation.Atoms().size()));
		
		RootConstantBufferView& sphereInstanceCBV = sphereRI.EmplaceBackRootConstantBufferView(0, m_instanceConstantBuffer.get());
		sphereInstanceCBV.Update = [this](const Timer& timer, int frameIndex)
			{
				InstanceDataArray d = {};
				int iii = 0;

				for (const auto& atom : m_simulation.Atoms())
				{
					const DirectX::XMFLOAT3& p = atom.position;
					DirectX::XMStoreFloat4x4(&d.Data[iii].World, DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(p.x, p.y, p.z)));

					d.Data[iii].MaterialIndex = iii % 2;

					++iii;
				}

				m_instanceConstantBuffer->CopyData(frameIndex, d);
			};
	}
}

void Renderer::OnResize()
{
	m_camera.SetLens(0.25f * MathHelper::Pi, m_viewport.Width / m_viewport.Height, 1.0f, 1000.0f);
}

void Renderer::Update(const Timer& timer, int frameIndex, const D3D12_VIEWPORT& vp)
{
	m_camera.UpdateViewMatrix();

	m_camera.SetLens(0.25f * MathHelper::Pi, vp.Width / vp.Height, 1.0f, 1000.0f);


	for (RenderPass& pass : m_renderPasses)
	{
		pass.Update(timer, frameIndex);

		for (RenderPassLayer& layer : pass.GetRenderPassLayers())
			layer.Update(timer, frameIndex);

		for (ComputeLayer& layer : pass.GetComputeLayers())
			layer.Update(timer, frameIndex);
	}
}

void Renderer::Render(const Simulation& simulation, int frameIndex)
{
	ASSERT(m_renderPasses.size() > 0, "No render passes");

	auto commandList = m_deviceResources->GetCommandList();

	GFX_THROW_INFO_ONLY(commandList->RSSetViewports(1, &m_viewport));
	GFX_THROW_INFO_ONLY(commandList->RSSetScissorRects(1, &m_scissorRect));

	for (RenderPass& pass : m_renderPasses)
	{
		ASSERT(pass.GetRenderPassLayers().size() > 0 || pass.GetComputeLayers().size() > 0, "Pass has no render layers nor compute layers. Must have at least 1 type of layer to be valid.");

		// Before attempting to perform any rendering, first perform all compute operations
		for (const ComputeLayer& layer : pass.GetComputeLayers())
		{
			RunComputeLayer(layer, nullptr, frameIndex); // NOTE: Pass nullptr for the timer, because we do not have access to the timer during the Rendering phase
		}

		// Pre-Work method - possibly for transitioning resources or anything necessary
		if (!pass.PreWork(pass, commandList))
			continue;

		// Set only a single root signature per RenderPass
		GFX_THROW_INFO_ONLY(commandList->SetGraphicsRootSignature(pass.GetRootSignature()->Get()));

		// Bind any per-pass constant buffer views
		for (const RootConstantBufferView& cbv : pass.GetRootConstantBufferViews())
		{
			GFX_THROW_INFO_ONLY(
				commandList->SetGraphicsRootConstantBufferView(
					cbv.GetRootParameterIndex(), 
					cbv.GetConstantBuffer()->GetGPUVirtualAddress(frameIndex)
				)
			);
		}

		// Render the render layers for the pass
		for (const RenderPassLayer& layer : pass.GetRenderPassLayers())
		{
			ASSERT(layer.GetRenderItems().size() > 0, "Layer has no render items");
			ASSERT(layer.GetPSO() != nullptr, "Layer has no pipeline state");
			ASSERT(layer.GetMeshGroup() != nullptr, "Layer has no mesh group");

			// PSO / Pre-Work / MeshGroup / Primitive Topology
			GFX_THROW_INFO_ONLY(commandList->SetPipelineState(layer.GetPSO()));

			if (!layer.PreWork(layer, commandList))		// Pre-Work method (example usage: setting stencil value)
				continue;

			MeshGroup* meshGroup = layer.GetMeshGroup().get();
			meshGroup->Bind(commandList);
			GFX_THROW_INFO_ONLY(commandList->IASetPrimitiveTopology(layer.GetTopology()));			

			for (const RenderItem& item : layer.GetRenderItems())
			{
				// Tables and CBV's ARE allowed to be empty
				for (const RootDescriptorTable& table : item.GetRootDescriptorTables())
				{
					GFX_THROW_INFO_ONLY(
						commandList->SetGraphicsRootDescriptorTable(table.GetRootParameterIndex(), table.GetDescriptorHandle())
					);
				}

				for (const RootConstantBufferView& cbv : item.GetRootConstantBufferViews())
				{
					GFX_THROW_INFO_ONLY(
						commandList->SetGraphicsRootConstantBufferView(cbv.GetRootParameterIndex(), cbv.GetConstantBuffer()->GetGPUVirtualAddress(frameIndex))
					);
				}

				SubmeshGeometry mesh = meshGroup->GetSubmesh(item.GetSubmeshIndex());
				GFX_THROW_INFO_ONLY(
					commandList->DrawIndexedInstanced(mesh.IndexCount, item.GetInstanceCount(), mesh.StartIndexLocation, mesh.BaseVertexLocation, 0)
				);
			}
		}

		pass.PostWork(pass, commandList);
	}
}

void Renderer::RunComputeLayer(const ComputeLayer& layer, const Timer* timer, int frameIndex)
{
	auto commandList = m_deviceResources->GetCommandList();

	ASSERT(layer.GetComputeItems().size() > 0, "Compute layer has no compute items");

	// Pre-Work method - possibly for transitioning resources
	//		If it returns false, that means we should quit early and not call Dispatch for this RenderLayer
	if (!layer.PreWork(layer, commandList, timer, frameIndex))
		return;

	// Root Signature / PSO
	GFX_THROW_INFO_ONLY(commandList->SetComputeRootSignature(layer.GetRootSignature()->Get()));
	GFX_THROW_INFO_ONLY(commandList->SetPipelineState(layer.GetPSO()));

	// Iterate over each compute item and call dispatch to submit compute work to the GPU
	for (const ComputeItem& item : layer.GetComputeItems())
	{
		// Tables and CBV's ARE allowed to be empty
		for (const RootDescriptorTable& table : item.GetRootDescriptorTables())
		{
			GFX_THROW_INFO_ONLY(
				commandList->SetComputeRootDescriptorTable(table.GetRootParameterIndex(), table.GetDescriptorHandle())
			);
		}
		for (const RootConstantBufferView& cbv : item.GetRootConstantBufferViews())
		{
			GFX_THROW_INFO_ONLY(
				commandList->SetComputeRootConstantBufferView(cbv.GetRootParameterIndex(), cbv.GetConstantBuffer()->GetGPUVirtualAddress(frameIndex))
			);
		}

		GFX_THROW_INFO_ONLY(commandList->Dispatch(item.GetThreadGroupCountX(), item.GetThreadGroupCountY(), item.GetThreadGroupCountZ()));
	}

	// Post-Work method - possibly for transitioning resources
	layer.PostWork(layer, commandList, timer, frameIndex);
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

			Vertex v = {};

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