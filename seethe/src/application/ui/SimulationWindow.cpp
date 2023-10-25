#include "SimulationWindow.h"


namespace seethe
{
SimulationWindow::SimulationWindow(std::shared_ptr<DeviceResources> deviceResources, 
								   Simulation& simulation,
								   float top, float left, float height, float width) noexcept :
	m_deviceResources(deviceResources),
	m_viewport{ left, top, width, height, 0.0f, 1.0f },
	m_scissorRect{ static_cast<long>(left), static_cast<long>(top), static_cast<long>(left + width), static_cast<long>(top + height) },
	m_renderer(nullptr),
	m_simulation(simulation)
{
	m_renderer = std::make_unique<Renderer>(m_deviceResources, m_viewport, m_scissorRect);

	InitializeRenderPasses();
}

void SimulationWindow::InitializeRenderPasses()
{
	// Root parameter can be a table, root descriptor or root constants.
	// *** Perfomance TIP: Order from most frequent to least frequent.
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0].InitAsConstantBufferView(0); // ObjectCB  
	slotRootParameter[1].InitAsConstantBufferView(1); // MaterialCB   
	slotRootParameter[2].InitAsConstantBufferView(2); // PassConstants 

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	std::shared_ptr<RootSignature> rootSig1 = std::make_shared<RootSignature>(m_deviceResources, rootSigDesc);
	RenderPass& pass1 = m_renderer->EmplaceBackRenderPass(rootSig1, "Render Pass #1");

	m_passConstantsBuffer = std::make_unique<ConstantBuffer<PassConstants>>(m_deviceResources);
	RootConstantBufferView& perPassConstantsCBV = pass1.EmplaceBackRootConstantBufferView(2, m_passConstantsBuffer.get());
	perPassConstantsCBV.Update = [this](const Timer& timer, int frameIndex)
		{
			Camera& camera = m_renderer->GetCamera();

			DirectX::XMMATRIX view = camera.GetView();
			DirectX::XMMATRIX proj = camera.GetProj();

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

			passConstants.EyePosW = camera.GetPosition3f();

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


	// Beginning of Layer #1 -----------------------------------------------------------------------

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


	// Beginning of Layer #2 (Box Layer) -----------------------------------------------------------------------

	DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	std::vector<SolidColorVertex> boxVertices = {
		{ { 1.0f, 1.0f, 1.0f, 1.0f }, color },		// 0:  x  y  z
		{ { -1.0f, 1.0f, 1.0f, 1.0f }, color },		// 1: -x  y  z
		{ { 1.0f, -1.0f, 1.0f, 1.0f }, color },		// 2:  x -y  z
		{ { 1.0f, 1.0f, -1.0f, 1.0f }, color },		// 3:  x  y -z
		{ { -1.0f, -1.0f, 1.0f, 1.0f }, color },	// 4: -x -y  z
		{ { -1.0f, 1.0f, -1.0f, 1.0f }, color },	// 5: -x  y -z
		{ { 1.0f, -1.0f, -1.0f, 1.0f }, color },	// 6:  x -y -z
		{ { -1.0f, -1.0f, -1.0f, 1.0f }, color }	// 7: -x -y -z
	};
	std::vector<std::uint16_t> boxIndices = {
		// Positive z face
		0, 1,
		0, 2,
		1, 4,
		2, 4,
		// Negative z face
		3, 5,
		3, 6,
		5, 7,
		6, 7,
		// Connecting postive to negative z
		1, 5,
		0, 3,
		2, 6,
		4, 7
	};

	std::vector<std::vector<SolidColorVertex>> boxVerticesList;
	boxVerticesList.push_back(std::move(boxVertices));

	std::vector<std::vector<std::uint16_t>> boxIndicesList;
	boxIndicesList.push_back(std::move(boxIndices));

	std::shared_ptr<MeshGroupT<SolidColorVertex>> boxMeshGroup = std::make_shared<MeshGroupT<SolidColorVertex>>(m_deviceResources, boxVerticesList, boxIndicesList);

	m_solidVS = std::make_unique<Shader>("src/shaders/output/SolidVS.cso");
	m_solidPS = std::make_unique<Shader>("src/shaders/output/SolidPS.cso");

	m_solidInputLayout = std::make_unique<InputLayout>(
		std::vector<D3D12_INPUT_ELEMENT_DESC>{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	}
	);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC boxDesc;
	ZeroMemory(&boxDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	boxDesc.InputLayout = m_solidInputLayout->GetInputLayoutDesc();
	boxDesc.pRootSignature = rootSig1->Get();
	boxDesc.VS = m_solidVS->GetShaderByteCode();
	boxDesc.PS = m_solidPS->GetShaderByteCode();
	boxDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	boxDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	boxDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	boxDesc.SampleMask = UINT_MAX;
	boxDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	boxDesc.NumRenderTargets = 1;
	boxDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
	boxDesc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1;
	boxDesc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0;
	boxDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();

	RenderPassLayer& layer2 = pass1.EmplaceBackRenderPassLayer(m_deviceResources, boxMeshGroup, boxDesc, D3D_PRIMITIVE_TOPOLOGY_LINELIST, "Layer #2");

	m_boxConstantBuffer = std::make_unique<ConstantBuffer<DirectX::XMFLOAT4X4>>(m_deviceResources);

	RenderItem& boxRI = layer2.EmplaceBackRenderItem();
	RootConstantBufferView& boxCBV = boxRI.EmplaceBackRootConstantBufferView(0, m_boxConstantBuffer.get());
	boxCBV.Update = [this](const Timer& timer, int frameIndex)
		{
			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world,
				DirectX::XMMatrixTranspose(
					DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f)
				)
			);

			m_boxConstantBuffer->CopyData(frameIndex, world);
		};
}
MeshData SimulationWindow::SphereMesh(float radius, uint32_t sliceCount, uint32_t stackCount)
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

void SimulationWindow::Update(const Timer& timer, int frameIndex)
{ 
	m_renderer->Update(timer, frameIndex); 
}



bool SimulationWindow::OnButtonDownImpl(bool& buttonFlag, float x, float y, std::function<void()>&& handler)
{
	// Don't set the buttonDownFlag if any of the other buttons are down
	bool dragging = Dragging();
	if (!dragging)
	{
		if (buttonFlag = ContainsPoint(x, y))
			handler();
	}

	// Return true only if the user is dragging something or the mouse is hovering the window
	return dragging || ContainsPoint(x, y);
}
bool SimulationWindow::OnButtonUpImpl(bool& buttonFlag, float x, float y, std::function<void()>&& handler)
{
	if (buttonFlag)
	{
		buttonFlag = false;
		handler(); 
	}

	// Return true only if the user is dragging something or the mouse is hovering the window
	return Dragging() || ContainsPoint(x, y);
}
bool SimulationWindow::OnButtonDoubleClickImpl(float x, float y, std::function<void()>&& handler)
{
	if (ContainsPoint(x, y)) 
	{
		handler(); 
		return true;
	}
	return false;
}
bool SimulationWindow::OnMouseMove(float x, float y) noexcept
{
	// If we are dragging, then continue to track the mouse
	if (Dragging() || ContainsPoint(x, y))
	{
		HandleMouseMove(x, y);
		m_mouseLastPos = { x, y }; // Make sure this gets updated AFTER calling HandleMouseMove() so we don't lose the previous position data
		return true;
	}

	return false;
}
bool SimulationWindow::OnMouseWheelVertical(int wheelDelta) noexcept
{
	// Only handle wheel input if we are not currently dragging and the mouse is over the window
	if (!Dragging() && ContainsPoint(m_mouseLastPos.x, m_mouseLastPos.y))
	{
		HandleMouseWheelVertical(wheelDelta);
		return true;
	}
	return false;
}
bool SimulationWindow::OnMouseWheelHorizontal(int wheelDelta) noexcept
{
	// Only handle wheel input if we are not currently dragging and the mouse is over the window
	if (!Dragging() && ContainsPoint(m_mouseLastPos.x, m_mouseLastPos.y))
	{
		HandleMouseWheelHorizontal(wheelDelta);
		return true;
	}
	return false;
}



void SimulationWindow::HandleLButtonDown() noexcept
{
	LOG_INFO("{}", "HandleLButtonDown");
}
void SimulationWindow::HandleLButtonUp() noexcept
{
	LOG_INFO("{}", "HandleLButtonUp");
}
void SimulationWindow::HandleLButtonDoubleClick() noexcept
{
	LOG_INFO("{}", "HandleLButtonDoubleClick");
}
void SimulationWindow::HandleMButtonDown() noexcept
{
	LOG_INFO("{}", "HandleMButtonDown");
}
void SimulationWindow::HandleMButtonUp() noexcept
{
	LOG_INFO("{}", "HandleMButtonUp");
}
void SimulationWindow::HandleMButtonDoubleClick() noexcept
{
	LOG_INFO("{}", "HandleMButtonDoubleClick");
}
void SimulationWindow::HandleRButtonDown() noexcept
{
	LOG_INFO("{}", "HandleRButtonDown");
}
void SimulationWindow::HandleRButtonUp() noexcept
{
	LOG_INFO("{}", "HandleRButtonUp");
}
void SimulationWindow::HandleRButtonDoubleClick() noexcept
{
	LOG_INFO("{}", "HandleRButtonDoubleClick");
}
void SimulationWindow::HandleX1ButtonDown() noexcept
{
	LOG_INFO("{}", "HandleX1ButtonDown");
}
void SimulationWindow::HandleX1ButtonUp() noexcept
{
	LOG_INFO("{}", "HandleX1ButtonUp");
}
void SimulationWindow::HandleX1ButtonDoubleClick() noexcept
{
	LOG_INFO("{}", "HandleX1ButtonDoubleClick");
}
void SimulationWindow::HandleX2ButtonDown() noexcept
{
	LOG_INFO("{}", "HandleX2ButtonDown");
}
void SimulationWindow::HandleX2ButtonUp() noexcept
{
	LOG_INFO("{}", "HandleX2ButtonUp");
}
void SimulationWindow::HandleX2ButtonDoubleClick() noexcept
{
	LOG_INFO("{}", "HandleX2ButtonDoubleClick");
}
void SimulationWindow::HandleMouseMove(float x, float y) noexcept
{
	if (m_mouseLButtonDown)
	{
		// If the pointer were to move from the middle of the screen to the far right,
		// that should produce one full rotation. Therefore, set a rotationFactor = 2
		const float rotationFactor = 2.0f; 
		const float radiansPerPixelX = (DirectX::XM_2PI / m_viewport.Width) * rotationFactor;
		const float radiansPerPixelY = (DirectX::XM_2PI / m_viewport.Height) * rotationFactor;

		const float thetaX = radiansPerPixelX * (x - m_mouseLastPos.x);
		const float thetaY = radiansPerPixelY * (y - m_mouseLastPos.y);

		m_renderer->GetCamera().RotateAroundLookAtPoint(thetaX, thetaY);
	}

}
void SimulationWindow::HandleMouseWheelVertical(int wheelDelta) noexcept
{
	LOG_INFO("Delta Vertical: {}", wheelDelta);

	Camera& camera = m_renderer->GetCamera();
	DirectX::XMFLOAT3 pos = camera.GetPosition3f();
	camera.StartAnimatedMove(2.0f, { pos.x * 2, pos.y * 2, pos.z * 2 });
}
void SimulationWindow::HandleMouseWheelHorizontal(int wheelDelta) noexcept
{
	LOG_INFO("Delta Horizontal: {}", wheelDelta);

}

}