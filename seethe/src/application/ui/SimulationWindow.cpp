#include "SimulationWindow.h"
#include "application/Application.h"
#include "application/change-requests/BoxResizeCR.h"
#include "rendering/GeometryGenerator.h"
#include "utils/Constants.h"

using namespace DirectX;

namespace seethe
{
SimulationWindow::SimulationWindow(Application& application,
								   std::shared_ptr<DeviceResources> deviceResources, 
								   Simulation& simulation, std::vector<Material>& materials,
								   float top, float left, float height, float width) :
	m_application(application),
	m_deviceResources(deviceResources),
	m_viewport{ left, top, width, height, 0.0f, 1.0f },
	m_scissorRect{ static_cast<long>(left), static_cast<long>(top), static_cast<long>(left + width), static_cast<long>(top + height) },
	m_renderer(nullptr),
	m_simulation(simulation),
	m_atomMaterials(materials)
{
	m_renderer = std::make_unique<Renderer>(m_deviceResources, m_viewport, m_scissorRect);

	InitializeRenderPasses();

	RegisterEventHandlers();
}

void SimulationWindow::RegisterEventHandlers() noexcept
{
	// Material Changed
	m_application.RegisterMaterialChangedHandler(
		[this]()
		{
			m_oneTimeUpdateFns.push_back(
				[this]()
				{
					m_materialsConstantBuffer->CopyData(m_atomMaterials);
				}
			);
		}
	);

	// Box Size Changed
	m_simulation.RegisterBoxSizeChangedHandler(
		[this]()
		{
			OnBoxSizeChanged();

			if (MouseIsDraggingWall())
				OnBoxFaceHighlightChanged();
		}
	);

	// Atoms Added/Removed/Selected
	m_simulation.RegisterAtomsAddedHandler([this]() { OnAtomsAdded(); });
	m_simulation.RegisterAtomsRemovedHandler([this]() { OnAtomsRemoved(); });
	m_simulation.RegisterSelectedAtomsChangedHandler([this]() { OnSelectedAtomsChanged(); });

	// Simulation Play/Paused
	m_simulation.RegisterSimulationStartedHandler([this]() { OnSimulationPlay(); });
	m_simulation.RegisterSimulationStoppedHandler([this]() { OnSimulationPause(); });
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

	m_passConstantsBuffer = std::make_unique<ConstantBufferMapped<PassConstants>>(m_deviceResources);
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

	// For the materials, we use a static constant buffer. This means we don't use a mapped upload heap and transfer material
	// data to the GPU every frame. Instead, we only upload material data to the GPU when the material data changes.
	// NOTE: This also means we don't need to supply the CBV Update lambda because we don't need to update this every frame
	m_materialsConstantBuffer = std::make_unique<ConstantBufferStatic<Material>>(m_deviceResources, static_cast<unsigned int>(m_atomMaterials.size()));
	m_materialsConstantBuffer->CopyData(m_atomMaterials);
	RootConstantBufferView& materialsCBV = pass1.EmplaceBackRootConstantBufferView(1, m_materialsConstantBuffer.get());


	// Beginning of Layer #1 -----------------------------------------------------------------------
	// Description:		Opaque Items w/ Phong Shading
	// Shading:			Phong
	// Vertex Type:		Vertex
	// Input Layout:	[FLOAT3, FLOAT3] (position, normal)
	// Topology:		Triangle
	// Blending:		None
	// Stenciling:		None

	GeometryGenerator::MeshData sphereMesh = GeometryGenerator::CreateSphere(1.0f, 20, 20);
	std::vector<Vertex> sphereVertices;
	sphereVertices.reserve(sphereMesh.Vertices.size());
	for (GeometryGenerator::Vertex& v : sphereMesh.Vertices)
		sphereVertices.push_back({ v.Position, v.Normal });

	GeometryGenerator::MeshData arrowMesh = GeometryGenerator::CreateArrow(0.25f, 0.25f, 0.5f, 2.0f, 0.8f, 20, 20);
	std::vector<Vertex> arrowVertices;
	arrowVertices.reserve(arrowMesh.Vertices.size());
	for (GeometryGenerator::Vertex& v : arrowMesh.Vertices)
		arrowVertices.push_back({ v.Position, v.Normal });

	std::vector<std::vector<Vertex>> vertices;
	vertices.push_back(std::move(sphereVertices));
	vertices.push_back(std::move(arrowVertices));

	std::vector<std::vector<std::uint16_t>> indices;
	indices.push_back(std::move(sphereMesh.GetIndices16()));
	indices.push_back(std::move(arrowMesh.GetIndices16()));

	m_sphereMeshGroup = std::make_shared<MeshGroup<Vertex>>(m_deviceResources, vertices, indices);

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
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();

	RenderPassLayer& layer1 = pass1.EmplaceBackRenderPassLayer(m_deviceResources, m_sphereMeshGroup, psoDesc, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, "Layer #1");

	m_instanceConstantBuffer = std::make_unique<ConstantBufferMapped<InstanceData>>(m_deviceResources);

	RenderItem& sphereRI = layer1.EmplaceBackRenderItem();
	sphereRI.SetInstanceCount(static_cast<unsigned int>(m_simulation.GetAtoms().size()));

	m_instanceData = std::vector<InstanceData>(10);

	RootConstantBufferView& sphereInstanceCBV = sphereRI.EmplaceBackRootConstantBufferView(0, m_instanceConstantBuffer.get());
	sphereInstanceCBV.Update = [this](const Timer& timer, int frameIndex)
		{
			int iii = 0;

			const std::vector<Atom>& atoms = m_simulation.GetAtoms();
			for (const auto& atom : atoms)
			{
				const DirectX::XMFLOAT3& p = atom.position;
				const float radius = atom.radius;

				DirectX::XMStoreFloat4x4(&m_instanceData[iii].World,
					DirectX::XMMatrixTranspose(
						DirectX::XMMatrixScaling(radius, radius, radius) * DirectX::XMMatrixTranslation(p.x, p.y, p.z)
					)
				);

				m_instanceData[iii].MaterialIndex = static_cast<std::uint32_t>(atom.type) - 1; // Minus one because Hydrogen = 1 but is at index 0, etc

				++iii;
			}

			m_instanceConstantBuffer->CopyData(frameIndex, m_instanceData);
		};



	// Arrow
	RenderItem& arrowRI = layer1.EmplaceBackRenderItem(1);
	arrowRI.SetActive(false);

	m_arrowConstantBuffer = std::make_unique<ConstantBufferStatic<InstanceData>>(m_deviceResources, 1);	
	RootConstantBufferView& arrowInstanceCBV = arrowRI.EmplaceBackRootConstantBufferView(0, m_arrowConstantBuffer.get());



	// Beginning of Layer #2 (Box Layer) -----------------------------------------------------------------------
	// Description:		Solid Lines
	// Shading:			Solid
	// Vertex Type:		SolidColorVertex
	// Input Layout:	[FLOAT3] (position)
	// Topology:		Line
	// Blending:		None
	// Stenciling:		None

	std::vector<SolidColorVertex> boxVertices = {
		{ { 1.0f, 1.0f, 1.0f, 1.0f } },		// 0:  x  y  z
		{ { -1.0f, 1.0f, 1.0f, 1.0f } },	// 1: -x  y  z
		{ { 1.0f, -1.0f, 1.0f, 1.0f } },	// 2:  x -y  z
		{ { 1.0f, 1.0f, -1.0f, 1.0f } },	// 3:  x  y -z
		{ { -1.0f, -1.0f, 1.0f, 1.0f } },	// 4: -x -y  z
		{ { -1.0f, 1.0f, -1.0f, 1.0f } },	// 5: -x  y -z
		{ { 1.0f, -1.0f, -1.0f, 1.0f } },	// 6:  x -y -z
		{ { -1.0f, -1.0f, -1.0f, 1.0f } }	// 7: -x -y -z
	};
	std::vector<std::uint16_t> boxIndices = {
		// Negative z face
		0, 1,
		0, 2,
		1, 4,
		2, 4,
		// Positive z face
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

	std::shared_ptr<MeshGroup<SolidColorVertex>> boxMeshGroup = std::make_shared<MeshGroup<SolidColorVertex>>(m_deviceResources, boxVerticesList, boxIndicesList);

	m_solidVS = std::make_unique<Shader>("src/shaders/output/SolidVS.cso");
	m_solidPS = std::make_unique<Shader>("src/shaders/output/SolidPS.cso");

	m_solidInputLayout = std::make_unique<InputLayout>(
		std::vector<D3D12_INPUT_ELEMENT_DESC>{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
	boxDesc.SampleDesc.Count = 1;
	boxDesc.SampleDesc.Quality = 0;
	boxDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();

	RenderPassLayer& layer2 = pass1.EmplaceBackRenderPassLayer(m_deviceResources, boxMeshGroup, boxDesc, D3D_PRIMITIVE_TOPOLOGY_LINELIST, "Layer #2");

	// Make the box constant buffer a static buffer because it will rarely need updating
	m_boxConstantBuffer = std::make_unique<ConstantBufferStatic<InstanceData>>(m_deviceResources, 1);
	OnBoxSizeChanged();

	RenderItem& boxRI = layer2.EmplaceBackRenderItem();
	RootConstantBufferView& boxCBV = boxRI.EmplaceBackRootConstantBufferView(0, m_boxConstantBuffer.get());



	// Beginning of Layer #3 (Solid Axis Lines Layer) -----------------------------------------------------------------------
	// Description:		Solid Objects
	// Shading:			Solid
	// Vertex Type:		SolidColorVertex
	// Input Layout:	[FLOAT3] (position)
	// Topology:		Triangle
	// Blending:		None
	// Stenciling:		None

	GeometryGenerator::MeshData cylinderMesh = GeometryGenerator::CreateCylinder(0.05f, 0.05f, 1.0f, 20, 1); 
	std::vector<SolidColorVertex> cylinderVertices; 
	cylinderVertices.reserve(cylinderMesh.Vertices.size()); 
	for (GeometryGenerator::Vertex& v : cylinderMesh.Vertices) 
		cylinderVertices.push_back({ { v.Position.x,  v.Position.y,  v.Position.z, 1.0f } }); 

	std::vector<std::vector<SolidColorVertex>> layer3Vertices; 
	layer3Vertices.push_back(std::move(cylinderVertices));

	std::vector<std::vector<std::uint16_t>> layer3Indices;
	layer3Indices.push_back(std::move(cylinderMesh.GetIndices16()));

	std::shared_ptr<MeshGroup<SolidColorVertex>> layer3MeshGroup = std::make_shared<MeshGroup<SolidColorVertex>>(m_deviceResources, layer3Vertices, layer3Indices);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC layer3PSODesc = boxDesc;
	layer3PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	RenderPassLayer& layer3 = pass1.EmplaceBackRenderPassLayer(m_deviceResources, layer3MeshGroup, layer3PSODesc, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, "Layer #3");
	layer3.SetActive(false);

	// Axis Cylinder
	RenderItem& axisRI = layer3.EmplaceBackRenderItem(0);

	m_axisCylinderConstantBuffer = std::make_unique<ConstantBufferStatic<InstanceData>>(m_deviceResources, 1); 
	RootConstantBufferView& axisCylinderInstanceCBV = axisRI.EmplaceBackRootConstantBufferView(0, m_axisCylinderConstantBuffer.get()); 


	// Beginning of Layer #4 (Transparency Layer - Expanding Box Walls) -----------------------------------------------------------------------
	// Description:		Transparent & Solid Items
	// Shading:			Solid
	// Vertex Type:		SolidColorVertex
	// Input Layout:	[FLOAT3] (position)
	// Topology:		Triangle
	// Blending:		Transparency Blending
	// Stenciling:		None

	// In this layer we render two of the six walls. To do so, we create a wall in the yz-place and rotate/scale/translate it depending on
	// which wall is being hovered/dragged
	std::vector<std::vector<SolidColorVertex>> boxFaceVerticesList = {
		{
			{ {  0.0f,  1.0f,  1.0f, 1.0f } },	// x  y  z
			{ {  0.0f, -1.0f,  1.0f, 1.0f } },	// x -y  z
			{ {  0.0f,  1.0f, -1.0f, 1.0f } },	// x  y -z
			{ {  0.0f, -1.0f, -1.0f, 1.0f } },	// x -y -z
		}
	};
	std::vector<std::vector<std::uint16_t>> boxFaceIndicesList = { { 0, 1, 2, 3 } };

	std::shared_ptr<MeshGroup<SolidColorVertex>> boxFacesMeshGroup = std::make_shared<MeshGroup<SolidColorVertex>>(m_deviceResources, boxFaceVerticesList, boxFaceIndicesList);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC boxFaceDesc = boxDesc;
	// Box faces will be triangles
	boxFaceDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// Don't cull back facing triangles because we will want to see the face from either side
	boxFaceDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	// Enable transparency blending
	boxFaceDesc.BlendState.AlphaToCoverageEnable = false;
	boxFaceDesc.BlendState.IndependentBlendEnable = false;
	boxFaceDesc.BlendState.RenderTarget[0].BlendEnable = true;
	boxFaceDesc.BlendState.RenderTarget[0].LogicOpEnable = false;
	boxFaceDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	boxFaceDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	boxFaceDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	boxFaceDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	boxFaceDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	boxFaceDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	boxFaceDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	boxFaceDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	RenderPassLayer& layer4 = pass1.EmplaceBackRenderPassLayer(m_deviceResources, boxFacesMeshGroup, boxFaceDesc, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, "Layer #3");
	layer4.SetActive(false);

	m_boxFaceConstantBuffer = std::make_unique<ConstantBufferStatic<InstanceData>>(m_deviceResources, 2);
	OnBoxFaceHighlightChanged();

	RenderItem& boxFaceRI = layer4.EmplaceBackRenderItem();
	boxFaceRI.SetInstanceCount(2);
	RootConstantBufferView& boxFaceCBV = boxFaceRI.EmplaceBackRootConstantBufferView(0, m_boxFaceConstantBuffer.get());


	// Beginning of Layer #5 (Stencil Layer for selected atoms) -----------------------------------------------------------------------
	// Description:		Stencil Layer
	// Shading:			Solid
	// Vertex Type:		SolidColorVertex
	// Input Layout:	[FLOAT3] (position)
	// Topology:		Triangle
	// Blending:		None
	// Stenciling:		Write 1 to stencil buffer for each pixel


	std::vector<SolidColorVertex> sphereStencilVertices;
	sphereStencilVertices.reserve(sphereMesh.Vertices.size());
	for (GeometryGenerator::Vertex& v : sphereMesh.Vertices)
		sphereStencilVertices.push_back( {{ v.Position.x, v.Position.y, v.Position.z, 1.0f }} );

	std::vector<std::vector<SolidColorVertex>> stencilVertices;
	stencilVertices.push_back(std::move(sphereStencilVertices));

	std::vector<std::vector<std::uint16_t>> sphereStencilIndices;
	sphereStencilIndices.push_back(std::move(sphereMesh.GetIndices16()));

	std::shared_ptr<MeshGroup<SolidColorVertex>> stencilMeshGroup = std::make_shared<MeshGroup<SolidColorVertex>>(m_deviceResources, stencilVertices, sphereStencilIndices);


	D3D12_DEPTH_STENCIL_DESC selectedAtomsStencilDesc = {};
	// Depth
	selectedAtomsStencilDesc.DepthEnable = false;
	selectedAtomsStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;	// <-- these don't matter because depth is disabled
	selectedAtomsStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	// Stencil
	selectedAtomsStencilDesc.StencilEnable = true;
	selectedAtomsStencilDesc.StencilReadMask = 0xff;
	selectedAtomsStencilDesc.StencilWriteMask = 0xff;

	selectedAtomsStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	selectedAtomsStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	selectedAtomsStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	selectedAtomsStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	selectedAtomsStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;		// <-- Don't need to worry about back face for atoms
	selectedAtomsStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	selectedAtomsStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	selectedAtomsStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC stencilPSODesc;
	ZeroMemory(&stencilPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	stencilPSODesc.InputLayout = m_solidInputLayout->GetInputLayoutDesc();
	stencilPSODesc.pRootSignature = rootSig1->Get(); 
	stencilPSODesc.VS = m_solidVS->GetShaderByteCode();
	stencilPSODesc.PS = m_solidPS->GetShaderByteCode();
	stencilPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	stencilPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	stencilPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0;		// IMPORTANT! Disable color writes to the render target. We just want to update stencil values - nothing more
	stencilPSODesc.DepthStencilState = selectedAtomsStencilDesc;
	stencilPSODesc.SampleMask = UINT_MAX;
	stencilPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	stencilPSODesc.NumRenderTargets = 1;
	stencilPSODesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
	stencilPSODesc.SampleDesc.Count = 1;
	stencilPSODesc.SampleDesc.Quality = 0;
	stencilPSODesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();

	RenderPassLayer& layer5 = pass1.EmplaceBackRenderPassLayer(m_deviceResources, stencilMeshGroup, stencilPSODesc, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, "Layer #4");
	layer5.SetStencilRef(1); 

	m_selectedAtomInstanceConstantBuffer = std::make_unique<ConstantBufferMapped<InstanceData>>(m_deviceResources);
	m_selectedAtomsInstanceData = std::vector<InstanceData>(10);

	RenderItem& sphereStencilRI = layer5.EmplaceBackRenderItem();
	sphereStencilRI.SetInstanceCount(static_cast<unsigned int>(m_simulation.GetSelectedAtomIndices().size()));

	RootConstantBufferView& sphereStencilInstanceCBV = sphereStencilRI.EmplaceBackRootConstantBufferView(0, m_selectedAtomInstanceConstantBuffer.get());
	sphereStencilInstanceCBV.Update = [this](const Timer& timer, int frameIndex)
		{
			const std::vector<Atom>& atoms = m_simulation.GetAtoms();
			const std::vector<size_t>& selectedIndices = m_simulation.GetSelectedAtomIndices();

			int iii = 0;
 
			for (size_t index : selectedIndices)
			{
				const Atom& atom = atoms[index];

				const DirectX::XMFLOAT3& p = atom.position; 
				const float radius = atom.radius; 

				DirectX::XMStoreFloat4x4(&m_selectedAtomsInstanceData[iii].World,
					DirectX::XMMatrixTranspose( 
						DirectX::XMMatrixScaling(radius, radius, radius) * DirectX::XMMatrixTranslation(p.x, p.y, p.z) 
					)
				);

				m_selectedAtomsInstanceData[iii].MaterialIndex = 0;

				++iii; 
			}

			m_selectedAtomInstanceConstantBuffer->CopyData(frameIndex, m_selectedAtomsInstanceData);
		};

	
	// Beginning of Layer #6 (Sphere outline for selected atoms) -----------------------------------------------------------------------
	// Description:		Outline Layer
	// Shading:			Solid
	// Vertex Type:		SolidColorVertex
	// Input Layout:	[FLOAT3] (position)
	// Topology:		Triangle
	// Blending:		None
	// Stenciling:		Write to pixels only where stencil buffer != 1


	// re-enable depth testing
	selectedAtomsStencilDesc.DepthEnable = false;
	selectedAtomsStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; 
	selectedAtomsStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; 

	selectedAtomsStencilDesc.FrontFace.StencilFailOp		= D3D12_STENCIL_OP_KEEP;
	selectedAtomsStencilDesc.FrontFace.StencilDepthFailOp	= D3D12_STENCIL_OP_KEEP;
	selectedAtomsStencilDesc.FrontFace.StencilPassOp		= D3D12_STENCIL_OP_KEEP; 
	selectedAtomsStencilDesc.FrontFace.StencilFunc			= D3D12_COMPARISON_FUNC_EQUAL;

	selectedAtomsStencilDesc.BackFace.StencilFailOp = selectedAtomsStencilDesc.FrontFace.StencilFailOp;
	selectedAtomsStencilDesc.BackFace.StencilDepthFailOp = selectedAtomsStencilDesc.FrontFace.StencilDepthFailOp;
	selectedAtomsStencilDesc.BackFace.StencilPassOp = selectedAtomsStencilDesc.FrontFace.StencilPassOp;
	selectedAtomsStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	
	D3D12_GRAPHICS_PIPELINE_STATE_DESC outlinePSODesc;
	ZeroMemory(&outlinePSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	outlinePSODesc.InputLayout = m_solidInputLayout->GetInputLayoutDesc();
	outlinePSODesc.pRootSignature = rootSig1->Get();
	outlinePSODesc.VS = m_solidVS->GetShaderByteCode();
	outlinePSODesc.PS = m_solidPS->GetShaderByteCode();
	outlinePSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	outlinePSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	outlinePSODesc.DepthStencilState = selectedAtomsStencilDesc;
	outlinePSODesc.SampleMask = UINT_MAX;
	outlinePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	outlinePSODesc.NumRenderTargets = 1;
	outlinePSODesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
	outlinePSODesc.SampleDesc.Count = 1;
	outlinePSODesc.SampleDesc.Quality = 0;
	outlinePSODesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();


	RenderPassLayer& layer6 = pass1.EmplaceBackRenderPassLayer(m_deviceResources, stencilMeshGroup, outlinePSODesc, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, "Layer #5");
	layer6.SetStencilRef(0);

	m_selectedAtomInstanceOutlineConstantBuffer = std::make_unique<ConstantBufferMapped<InstanceData>>(m_deviceResources);
	m_selectedAtomsInstanceOutlineData = std::vector<InstanceData>(10);

	RenderItem& sphereOutlineRI = layer6.EmplaceBackRenderItem();
	sphereOutlineRI.SetInstanceCount(static_cast<unsigned int>(m_simulation.GetSelectedAtomIndices().size())); 

	RootConstantBufferView& outlineStencilInstanceCBV = sphereOutlineRI.EmplaceBackRootConstantBufferView(0, m_selectedAtomInstanceOutlineConstantBuffer.get());
	outlineStencilInstanceCBV.Update = [this](const Timer& timer, int frameIndex)
		{
			const std::vector<Atom>& atoms = m_simulation.GetAtoms();
			const std::vector<size_t>& selectedIndices = m_simulation.GetSelectedAtomIndices();

			int iii = 0; 

			XMVECTOR cameraPos = m_renderer->GetCamera().GetPosition();

			for (size_t index : selectedIndices) 
			{
				const Atom& atom = atoms[index]; 
				const DirectX::XMFLOAT3& p = atom.position; 

				float distance = XMVectorGetX(XMVector3Length(cameraPos - XMLoadFloat3(&p)));

				// Increase the radius - this creates the outline effect around the atom
				// NOTE: 0.003 was used because this width seems appealing. It could easily be made smaller or larger
				// NOTE: We use simulation.GetSelectedAtomCenter() and compute an increase to the radius based on the 
				//		 center of the selected atoms and would therefore not need to compute the distance to every
				//		 single atom. However, this leads to undesirable results when some atoms are far from the selected
				//		 atoms center. Atoms close to the camera get a noticably larger outline while atoms further get a
				//		 very thin outline.
				const float radius = atom.radius + (0.003f * distance);

				DirectX::XMStoreFloat4x4(&m_selectedAtomsInstanceOutlineData[iii].World,
					DirectX::XMMatrixTranspose( 
						DirectX::XMMatrixScaling(radius, radius, radius) * DirectX::XMMatrixTranslation(p.x, p.y, p.z) 
					) 
				);

				m_selectedAtomsInstanceOutlineData[iii].MaterialIndex = g_selectedAtomOutlineMaterialIndex;

				++iii;
			}

			m_selectedAtomInstanceOutlineConstantBuffer->CopyData(frameIndex, m_selectedAtomsInstanceOutlineData);
		};
}

void SimulationWindow::Update(const Timer& timer, int frameIndex)
{ 
	m_renderer->Update(timer, frameIndex);

	if (m_oneTimeUpdateFns.size() > 0)
	{
		for (auto& fn : m_oneTimeUpdateFns)
			fn();

		m_oneTimeUpdateFns.clear();
	}
}

void SimulationWindow::StartSelectionMovement(MovementDirection direction) noexcept
{
	m_movementDirection = direction;
	m_selectionBeingMovedStateIsActive = true;
	m_selectionIsBeingDragged = false;
	SelectionMovementDirectionChanged();
}
void SimulationWindow::SelectionMovementDirectionChangedImpl() noexcept
{
	auto& pass1Layers = m_renderer->GetRenderPass(0).GetRenderPassLayers();

	// If nothing is selected, turn off the layers that renders the axis and drag plane
	if (m_simulation.GetSelectedAtomIndices().size() == 0)
	{
		pass1Layers[2].SetActive(false);
		pass1Layers[3].SetActive(false);
		return;
	}

	const XMFLOAT3& center = m_simulation.GetSelectedAtomsCenter();
	XMFLOAT3 dims = m_simulation.GetDimensions();
	float yScale = 10.0f * std::max(dims.x, std::max(dims.y, dims.z));

	switch (m_movementDirection)
	{
	case MovementDirection::X:
	{
		pass1Layers[3].SetActive(false); // Turn off the transparent plane layer
		pass1Layers[2].SetActive(true);

		InstanceData axisData = {};
		axisData.MaterialIndex = g_solidAxisColorMaterialIndex;
		XMStoreFloat4x4(&axisData.World, XMMatrixTranspose(XMMatrixScaling(1.0f, yScale, 1.0f) * XMMatrixRotationZ(XM_PIDIV2) * XMMatrixTranslation(center.x, center.y, center.z)));
		m_axisCylinderConstantBuffer->CopyData(axisData);
		break;
	}
	case MovementDirection::Y:
	{
		pass1Layers[3].SetActive(false); // Turn off the transparent plane layer
		pass1Layers[2].SetActive(true);

		InstanceData axisData = {};
		axisData.MaterialIndex = g_solidAxisColorMaterialIndex;
		XMStoreFloat4x4(&axisData.World, XMMatrixTranspose(XMMatrixScaling(1.0f, yScale, 1.0f) * XMMatrixTranslation(center.x, center.y, center.z)));
		m_axisCylinderConstantBuffer->CopyData(axisData);
		break;
	}
	case MovementDirection::Z:
	{
		pass1Layers[3].SetActive(false); // Turn off the transparent plane layer
		pass1Layers[2].SetActive(true);

		InstanceData axisData = {};
		axisData.MaterialIndex = g_solidAxisColorMaterialIndex;
		XMStoreFloat4x4(&axisData.World, XMMatrixTranspose(XMMatrixScaling(1.0f, yScale, 1.0f) * XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(center.x, center.y, center.z)));
		m_axisCylinderConstantBuffer->CopyData(axisData);
		break;
	}
	default:
	{
		pass1Layers[2].SetActive(false);	// Disable the solid cylinder layer
		pass1Layers[3].SetActive(true);		// Activate the drag plane layer
		pass1Layers[3].GetRenderItems()[0].SetInstanceCount(1);

		SelectionMovementDragPlaneChangedImpl();
		break;
	}
	}
}
void SimulationWindow::SelectionMovementDragPlaneChangedImpl() noexcept
{
	// Box Highlighted Walls -----------------------------------------------
	InstanceData d = {}; 
	d.MaterialIndex = g_boxFaceWhenHoveredMaterialIndex;  

	XMMATRIX rotation = XMMatrixIdentity();
	if (m_movementDirection == MovementDirection::XY) 
		rotation = XMMatrixRotationY(XM_PIDIV2); 
	else if (m_movementDirection == MovementDirection::XZ)
		rotation = XMMatrixRotationZ(XM_PIDIV2);

	const XMFLOAT3& center = m_simulation.GetSelectedAtomsCenter();
	float scale = 10.0f;
	XMMATRIX world = rotation * XMMatrixScaling(scale, scale, scale) * XMMatrixTranslation(center.x, center.y, center.z);
	XMStoreFloat4x4(&d.World, XMMatrixTranspose(world));

	m_boxFaceConstantBuffer->CopyData(d);
}
void SimulationWindow::DragSelectedAtoms(float x, float y) noexcept
{
	// So... How this works is kind of a mystery to me lol
	// I tested out various approaches, including making use of unprojecting 'clickpointFar' and 'prevClickPointFar'
	// but it seems that all I need are the previous and current click point origins

	Camera& camera = m_renderer->GetCamera(); 
	XMMATRIX projection = camera.GetProj(); 
	XMMATRIX view = camera.GetView(); 
	XMMATRIX world = XMMatrixIdentity();

	XMVECTOR clickpointNear = XMVectorSet(x, y, 0.0f, 1.0f); 
	XMVECTOR prevClickpointNear = XMVectorSet(m_mousePrevX, m_mousePrevY, 0.0f, 1.0f);

	XMVECTOR origin = XMVector3Unproject(
		clickpointNear, 
		m_viewport.TopLeftX, 
		m_viewport.TopLeftY, 
		m_viewport.Width, 
		m_viewport.Height, 
		m_viewport.MinDepth,
		m_viewport.MaxDepth,
		projection,
		view, 
		world);

	XMVECTOR prevOrigin = XMVector3Unproject(
		prevClickpointNear,
		m_viewport.TopLeftX,
		m_viewport.TopLeftY,
		m_viewport.Width,
		m_viewport.Height,
		m_viewport.MinDepth,
		m_viewport.MaxDepth,
		projection,
		view,
		world);

	float factor = XMVectorGetX(XMVector3Length(origin));
	XMVECTOR delta = origin - prevOrigin;	

	switch (m_movementDirection)
	{
	case MovementDirection::X:
		m_simulation.MoveSelectedAtomsX(XMVectorGetX(delta) * factor); 
		break;
	case MovementDirection::Y:
		m_simulation.MoveSelectedAtomsY(XMVectorGetY(delta) * factor);
		break;
	case MovementDirection::Z:
		m_simulation.MoveSelectedAtomsZ(XMVectorGetZ(delta) * factor);
		break;
	case MovementDirection::XY:
		m_simulation.MoveSelectedAtomsX(XMVectorGetX(delta) * factor);
		m_simulation.MoveSelectedAtomsY(XMVectorGetY(delta) * factor);
		SelectionMovementDragPlaneChanged();
		break;
	case MovementDirection::XZ:
		m_simulation.MoveSelectedAtomsX(XMVectorGetX(delta) * factor);
		m_simulation.MoveSelectedAtomsZ(XMVectorGetZ(delta) * factor);
		SelectionMovementDragPlaneChanged();
		break;
	case MovementDirection::YZ:
		m_simulation.MoveSelectedAtomsY(XMVectorGetY(delta) * factor);
		m_simulation.MoveSelectedAtomsZ(XMVectorGetZ(delta) * factor);
		SelectionMovementDragPlaneChanged();
		break;
	}
}
void SimulationWindow::EndSelectionMovement() noexcept
{
	m_selectionBeingMovedStateIsActive = false;
	m_selectionIsBeingDragged = false;

	switch (m_movementDirection)
	{
	case MovementDirection::X:
	case MovementDirection::Y:
	case MovementDirection::Z: m_renderer->GetRenderPass(0).GetRenderPassLayers()[2].SetActive(false); break;
	}
}



void SimulationWindow::OnBoxSizeChanged() noexcept
{
	m_oneTimeUpdateFns.push_back(
		[this]()
		{
			InstanceData d = {};
			d.MaterialIndex = g_boxMaterialIndex;

			DirectX::XMFLOAT3 dims = m_simulation.GetDimensionMaxs();
			DirectX::XMStoreFloat4x4(&d.World,
				DirectX::XMMatrixTranspose(
					DirectX::XMMatrixScaling(dims.x, dims.y, dims.z)
				)
			);

			m_boxConstantBuffer->CopyData(d);
		}
	);
}
void SimulationWindow::OnBoxFaceHighlightChanged() noexcept
{
	m_oneTimeUpdateFns.push_back(
		[this]()
		{
			// Box Highlighted Walls -----------------------------------------------
			uint32_t i = MouseIsDraggingWall() ? g_boxFaceWhenClickedMaterialIndex : g_boxFaceWhenHoveredMaterialIndex;
			InstanceData d[2] = {};
			d[0].MaterialIndex = i;
			d[1].MaterialIndex = i;

			XMFLOAT3 dims = m_simulation.GetDimensionMaxs();

			XMMATRIX pos = XMMatrixIdentity();
			XMMATRIX neg = XMMatrixIdentity();

			if (m_mouseHoveringBoxWallPosX || m_mouseHoveringBoxWallNegX || m_mouseDraggingBoxWallPosX || m_mouseDraggingBoxWallNegX)
			{
				pos = XMMatrixScaling(dims.x, dims.y, dims.z) * XMMatrixTranslation(dims.x, 0.0f, 0.0f);
				neg = XMMatrixScaling(dims.x, dims.y, dims.z) * XMMatrixTranslation(-dims.x, 0.0f, 0.0f);
			}
			else if (m_mouseHoveringBoxWallPosY || m_mouseHoveringBoxWallNegY || m_mouseDraggingBoxWallPosY || m_mouseDraggingBoxWallNegY)
			{
				pos = XMMatrixRotationZ(XM_PIDIV2) * XMMatrixScaling(dims.x, dims.y, dims.z) * XMMatrixTranslation(0.0f, dims.y, 0.0f);
				neg = XMMatrixRotationZ(XM_PIDIV2) * XMMatrixScaling(dims.x, dims.y, dims.z) * XMMatrixTranslation(0.0f, -dims.y, 0.0f);
			}
			else if (m_mouseHoveringBoxWallPosZ || m_mouseHoveringBoxWallNegZ || m_mouseDraggingBoxWallPosZ || m_mouseDraggingBoxWallNegZ)
			{
				pos = XMMatrixRotationY(XM_PIDIV2) * XMMatrixScaling(dims.x, dims.y, dims.z) * XMMatrixTranslation(0.0f, 0.0f, dims.z);
				neg = XMMatrixRotationY(XM_PIDIV2) * XMMatrixScaling(dims.x, dims.y, dims.z) * XMMatrixTranslation(0.0f, 0.0f, -dims.z);
			}

			XMStoreFloat4x4(&d[0].World, XMMatrixTranspose(pos));
			XMStoreFloat4x4(&d[1].World, XMMatrixTranspose(neg));

			m_boxFaceConstantBuffer->CopyData(d);


			// Arrow -----------------------------------------------
			InstanceData arrowData = {};

			// We add an extra material at the end of the atom materials vector so we can use it for the arrow
			arrowData.MaterialIndex = g_arrowMaterialIndex;

			pos = XMMatrixIdentity();
			float scale = 1.0f;

			if (m_mouseHoveringBoxWallPosX || m_mouseDraggingBoxWallPosX)
			{
				scale = std::max(dims.y, dims.z) / 10.0f;
				pos = XMMatrixRotationZ(-XM_PIDIV2) * XMMatrixTranslation(dims.x, 0.0f, 0.0f);
			}
			else if (m_mouseHoveringBoxWallNegX || m_mouseDraggingBoxWallNegX)
			{
				scale = std::max(dims.y, dims.z) / 10.0f;
				pos = XMMatrixRotationZ(XM_PIDIV2) * XMMatrixTranslation(-dims.x, 0.0f, 0.0f);
			}
			else if (m_mouseHoveringBoxWallPosY || m_mouseDraggingBoxWallPosY)
			{
				scale = std::max(dims.x, dims.z) / 10.0f;
				pos = XMMatrixTranslation(0.0f, dims.y, 0.0f);
			}
			else if (m_mouseHoveringBoxWallNegY || m_mouseDraggingBoxWallNegY)
			{
				scale = std::max(dims.x, dims.z) / 10.0f;
				pos = XMMatrixRotationZ(XM_PI) * XMMatrixTranslation(0.0f, -dims.y, 0.0f);
			}
			else if (m_mouseHoveringBoxWallPosZ || m_mouseDraggingBoxWallPosZ)
			{
				scale = std::max(dims.x, dims.y) / 10.0f;
				pos = XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(0.0f, 0.0f, dims.z);
			}
			else if (m_mouseHoveringBoxWallNegZ || m_mouseDraggingBoxWallNegZ)
			{
				scale = std::max(dims.x, dims.y) / 10.0f;
				pos = XMMatrixRotationX(-XM_PIDIV2) * XMMatrixTranslation(0.0f, 0.0f, -dims.z);
			}

			XMStoreFloat4x4(&arrowData.World, XMMatrixTranspose(XMMatrixScaling(scale, scale, scale) * pos));

			m_arrowConstantBuffer->CopyData(arrowData);
		}
	);
}
void SimulationWindow::OnSelectedAtomsChanged() noexcept
{
	const std::vector<size_t>& selectedIndices = m_simulation.GetSelectedAtomIndices();
	unsigned int count = static_cast<unsigned int>(selectedIndices.size());
	std::vector<RenderPassLayer>& pass1Layers = m_renderer->GetRenderPass(0).GetRenderPassLayers();

	// Layer at index 4: Stencil layer that writes to the stencil buffer
	// Layer at index 5: Outline layer that draws the solid outline around selected atoms
	if (count == 0)
	{
		pass1Layers[4].SetActive(false);
		pass1Layers[5].SetActive(false);
	}
	else
	{
		pass1Layers[4].SetActive(true);
		pass1Layers[5].SetActive(true);
		pass1Layers[4].GetRenderItems()[0].SetInstanceCount(count);
		pass1Layers[5].GetRenderItems()[0].SetInstanceCount(count);
	}

	// If we are allow the mouse to move the selected atoms, then we need to update the constant buffer
	// that holds data for axis/plane being rendered
	if (m_selectionBeingMovedStateIsActive)
		SelectionMovementDirectionChanged();
}
void SimulationWindow::OnAtomsAdded() noexcept
{
	// Make sure the instance data vector has enough capacity for the new atoms
	const std::vector<Atom>& atoms = m_simulation.GetAtoms();
	if (atoms.size() > m_instanceData.size())
		m_instanceData.resize(atoms.size());

	// Make sure the sphere render item has the appropriate instance count
	m_renderer->GetRenderPass(0).GetRenderPassLayers()[0].GetRenderItems()[0].SetInstanceCount(static_cast<unsigned int>(atoms.size()));
}
void SimulationWindow::OnAtomsRemoved() noexcept
{
	// Make sure the sphere render item has the appropriate instance count
	const std::vector<Atom>& atoms = m_simulation.GetAtoms();
	m_renderer->GetRenderPass(0).GetRenderPassLayers()[0].GetRenderItems()[0].SetInstanceCount(static_cast<unsigned int>(atoms.size()));
}
void SimulationWindow::OnSimulationPlay() noexcept
{
	// If we are in the state where we are allowing the mouse to select and drag atoms, then we will want to turn off
	// the axis cylinder when the simulation plays
	if (m_selectionBeingMovedStateIsActive)
	{
		m_renderer->GetRenderPass(0).GetRenderPassLayers()[2].SetActive(false);
	}
}
void SimulationWindow::OnSimulationPause() noexcept
{
	if (m_selectionBeingMovedStateIsActive)
	{
		m_renderer->GetRenderPass(0).GetRenderPassLayers()[2].SetActive(true);
		SelectionMovementDirectionChanged();
	}
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
	else if (m_allowMouseToResizeBoxDimensions)
	{
		// If we are allowing the box wall to be resized, its possible the transparency layer
		// is active because the mouse could have been hovering over a box wall and then exited
		// the viewport. When this happens, we must make sure the layer is deactivated
		SetBoxWallResizeRenderEffectsActive(false);
	}

	m_mouseLastPos = { x, y }; // Make sure this gets updated AFTER calling HandleMouseMove() so we don't lose the previous position data
	
	// If a keyboard key is down, then return true so that this SimulationWindow can be the one to handle the key up event
	return KeyboardKeyIsPressed(); 
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
bool SimulationWindow::OnKeyDown(unsigned int virtualKeyCode) noexcept
{
	// Only handle keyboard input if we are not currently dragging and the mouse is over the window
	if (!Dragging() && ContainsPoint(m_mouseLastPos.x, m_mouseLastPos.y)) 
	{
		HandleKeyDown(virtualKeyCode);
		return true;
	}
	return false;
}
bool SimulationWindow::OnKeyUp(unsigned int virtualKeyCode) noexcept
{
	// Only handle keyboard input if we are not currently dragging
	if (!Dragging()) 
	{
		HandleKeyUp(virtualKeyCode); 
		return true;
	}
	return false;
}
bool SimulationWindow::OnChar(char c) noexcept
{
	// Only handle keyboard input if we are not currently dragging and the mouse is over the window
	if (!Dragging() && ContainsPoint(m_mouseLastPos.x, m_mouseLastPos.y)) 
	{
		HandleChar(c);
		return true;
	}
	return false;
}



void SimulationWindow::HandleLButtonDown() noexcept
{
	if (m_allowMouseToResizeBoxDimensions)
	{
		m_mouseDraggingBoxWallPosX = m_mouseHoveringBoxWallPosX;
		m_mouseDraggingBoxWallPosY = m_mouseHoveringBoxWallPosY;
		m_mouseDraggingBoxWallPosZ = m_mouseHoveringBoxWallPosZ;
		m_mouseDraggingBoxWallNegX = m_mouseHoveringBoxWallNegX;
		m_mouseDraggingBoxWallNegY = m_mouseHoveringBoxWallNegY;
		m_mouseDraggingBoxWallNegZ = m_mouseHoveringBoxWallNegZ;
		m_mouseDraggingBoxJustStarted = MouseIsDraggingWall();

		if (m_mouseDraggingBoxJustStarted)
		{
			m_boxDimensionsInitial = m_simulation.GetDimensions();
			m_forceSidesToBeEqualInitial = m_application.GetSimulationSettings().forceSidesToBeEqual;

			OnBoxFaceHighlightChanged();
		}
	}
	else if (m_selectionBeingMovedStateIsActive)
	{
		m_selectionIsBeingDragged = m_atomHoveredOverIndex.has_value() && m_simulation.AtomIsSelected(m_atomHoveredOverIndex.value());
	}
}
void SimulationWindow::HandleLButtonUp() noexcept
{
	if (m_allowMouseToResizeBoxDimensions) 
	{
		ClearMouseDraggingWallState(); 

		XMFLOAT3 dims = m_simulation.GetDimensions();
		if (dims.x != m_boxDimensionsInitial.x || dims.y != m_boxDimensionsInitial.y || dims.z != m_boxDimensionsInitial.z)
		{
			m_application.AddUndoCR<BoxResizeCR>(m_boxDimensionsInitial, dims, false, m_forceSidesToBeEqualInitial, false);
			m_application.GetSimulationSettings().forceSidesToBeEqual = false;
		}

		OnBoxFaceHighlightChanged();
	}
	else if (m_selectionBeingMovedStateIsActive)
	{
		if (m_selectionIsBeingDragged)
			m_selectionIsBeingDragged = false;
		else if (m_atomHoveredOverIndex.has_value())
		{
			m_simulation.SelectAtom(m_atomHoveredOverIndex.value());
		}
	}
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
	// Disallow picking when the simulation is actively playing
	if (!m_simulation.IsPlaying())
	{
		// Check if the mouse is allowed to move atoms
		if (m_selectionBeingMovedStateIsActive)
		{
			if (m_selectionIsBeingDragged)
			{
				DragSelectedAtoms(x, y);
			}
			else
			{
				m_atomHoveredOverIndex = PickAtom(x, y); 
			}
		}
		// Check if mouse resizing the box is enabled
		else if (m_allowMouseToResizeBoxDimensions)
		{
			// If we are actively expanding/contracting the box walls, handle that first
			if (MouseIsDraggingWall())
			{
				Camera& camera = m_renderer->GetCamera();
				XMVECTOR _up = XMVector3Normalize(camera.GetUp());
				XMVECTOR _pos = XMVector3Normalize(camera.GetPosition());

				XMFLOAT3 up = {};
				XMFLOAT3 pos = {};
				XMFLOAT3 right = {};

				XMStoreFloat3(&up, _up);
				XMStoreFloat3(&pos, _pos);
				XMStoreFloat3(&right, XMVector3Cross(_pos, _up));

				if (m_mouseDraggingBoxJustStarted)
				{
					if (m_mouseDraggingBoxWallPosX)
					{
						m_mouseDraggingBoxRightIsLarger = right.x > 0.0f;
						m_mouseDraggingBoxUpIsLarger = up.x > 0.0f; 
					}
					else if (m_mouseDraggingBoxWallNegX)
					{
						m_mouseDraggingBoxRightIsLarger = right.x < 0.0f;
						m_mouseDraggingBoxUpIsLarger = up.x < 0.0f;
					}
					else if (m_mouseDraggingBoxWallPosY)
					{
						m_mouseDraggingBoxRightIsLarger = right.y > 0.0f;
						m_mouseDraggingBoxUpIsLarger = up.y > 0.0f;
					}
					else if (m_mouseDraggingBoxWallNegY)
					{
						m_mouseDraggingBoxRightIsLarger = right.y < 0.0f;
						m_mouseDraggingBoxUpIsLarger = up.y < 0.0f;
					}
					else if (m_mouseDraggingBoxWallPosZ)
					{
						m_mouseDraggingBoxRightIsLarger = right.z > 0.0f;
						m_mouseDraggingBoxUpIsLarger = up.z > 0.0f;
					}
					else if (m_mouseDraggingBoxWallNegZ)
					{
						m_mouseDraggingBoxRightIsLarger = right.z < 0.0f;
						m_mouseDraggingBoxUpIsLarger = up.z < 0.0f;
					} 
					
					m_mouseDraggingBoxJustStarted = false;
				}

				float deltaX = (x - m_mousePrevX) * (m_mouseDraggingBoxRightIsLarger ? 1 : -1);
				float deltaY = (y - m_mousePrevY) * (m_mouseDraggingBoxUpIsLarger ? -1 : 1);

				XMFLOAT3 dims = m_simulation.GetDimensions();
				float minimumNewSize = m_simulation.GetMaxAxisAlignedDistanceFromOrigin();

				if (m_mouseDraggingBoxWallPosX || m_mouseDraggingBoxWallNegX)
				{
					deltaX *= std::abs(right.x);
					deltaY *= std::abs(up.x);
					float scaleFactor = 2 * dims.x * ((deltaX / m_viewport.Width) + (deltaY / m_viewport.Height));
					dims.x = std::max(minimumNewSize, dims.x + scaleFactor);					
				}
				else if (m_mouseDraggingBoxWallPosY || m_mouseDraggingBoxWallNegY)
				{
					deltaX *= std::abs(right.y);
					deltaY *= std::abs(up.y);
					float scaleFactor = 2 * dims.x * ((deltaX / m_viewport.Width) + (deltaY / m_viewport.Height));
					dims.y = std::max(minimumNewSize, dims.y + scaleFactor);
				}
				else if (m_mouseDraggingBoxWallPosZ || m_mouseDraggingBoxWallNegZ)
				{
					deltaX *= std::abs(right.z);
					deltaY *= std::abs(up.z);
					float scaleFactor = 2 * dims.x * ((deltaX / m_viewport.Width) + (deltaY / m_viewport.Height));
					dims.z = std::max(minimumNewSize, dims.z + scaleFactor);
				}

				// No need to manually invoke the OnBoxSizeChanged function here. That wll get called in the handler
				// that is invoked by the Application when SetBoxDimensions is called
				m_application.SetBoxDimensions(dims, false, false);

				m_mousePrevX = x;
				m_mousePrevY = y;
				return;
			}
			else
			{
				bool hoveringPosXInitial = m_mouseHoveringBoxWallPosX;
				bool hoveringPosYInitial = m_mouseHoveringBoxWallPosY;
				bool hoveringPosZInitial = m_mouseHoveringBoxWallPosZ;
				bool hoveringNegXInitial = m_mouseHoveringBoxWallNegX;
				bool hoveringNegYInitial = m_mouseHoveringBoxWallNegY;
				bool hoveringNegZInitial = m_mouseHoveringBoxWallNegZ;

				PickBoxWalls(x, y);

				if (hoveringPosXInitial != m_mouseHoveringBoxWallPosX ||
					hoveringPosYInitial != m_mouseHoveringBoxWallPosY ||
					hoveringPosZInitial != m_mouseHoveringBoxWallPosZ ||
					hoveringNegXInitial != m_mouseHoveringBoxWallNegX ||
					hoveringNegYInitial != m_mouseHoveringBoxWallNegY ||
					hoveringNegZInitial != m_mouseHoveringBoxWallNegZ)
				{
					OnBoxFaceHighlightChanged();
				}
				SetBoxWallResizeRenderEffectsActive(MouseIsHoveringWall());
			}
		}		
	}

	Camera& camera = m_renderer->GetCamera();

	if (m_mouseLButtonDown && !m_selectionIsBeingDragged)
	{
		// If the camera is in a constant rotation (because arrow keys are down), disable dragging
		if (camera.IsInConstantRotation())
			return;

		// If the pointer were to move from the middle of the screen to the far right,
		// that should produce one full rotation. Therefore, set a rotationFactor = 2
		const float rotationFactor = 2.0f; 
		const float radiansPerPixelX = (DirectX::XM_2PI / m_viewport.Width) * rotationFactor;
		const float radiansPerPixelY = (DirectX::XM_2PI / m_viewport.Height) * rotationFactor;

		const float thetaX = radiansPerPixelX * (x - m_mouseLastPos.x);
		const float thetaY = radiansPerPixelY * (y - m_mouseLastPos.y);

		camera.RotateAroundLookAtPoint(thetaX, thetaY);
	}

	m_mousePrevX = x;
	m_mousePrevY = y;
}
void SimulationWindow::HandleMouseWheelVertical(int wheelDelta) noexcept
{
	LOG_INFO("Delta Vertical: {}", wheelDelta);

	// NOTE: When using my mouse the wheelDelta values are always multiples of 120. However, when using
	//       the trackpad, they range from 1-10 typically. We could decide to handle those differently,
	//       but for right now, I seem to get pretty decent behavior with ignoring the actual value and
	//       just zooming by percent with the code below.

	constexpr float percent = 0.15f;
	constexpr float duration = 0.1f;

	Camera& camera = m_renderer->GetCamera();
	if (wheelDelta > 0)
		camera.ZoomInPercent(percent, duration);
	else
		camera.ZoomOutPercent(percent, duration);
}
void SimulationWindow::HandleMouseWheelHorizontal(int wheelDelta) noexcept
{
	LOG_INFO("Delta Horizontal: {}", wheelDelta);

}
void SimulationWindow::HandleKeyDown(unsigned int virtualKeyCode) noexcept
{
	Camera& camera = m_renderer->GetCamera();

	switch (virtualKeyCode)
	{
	case VK_LEFT:
		m_arrowLeftIsPressed = true;
		camera.StartConstantLeftRotation();
		break;

	case VK_RIGHT:
		m_arrowRightIsPressed = true;
		camera.StartConstantRightRotation();
		break;

	case VK_UP:
		m_arrowUpIsPressed = true;
		camera.StartConstantUpRotation();
		break;

	case VK_DOWN:
		m_arrowDownIsPressed = true;
		camera.StartConstantDownRotation();
		break;

	case VK_SHIFT:
		m_shiftIsPressed = true;
		break;

	case 0x57: // w
		// Don't do anything if the key is already down
		if (!m_keyWIsPressed && !m_shiftIsPressed)
		{
			m_keyWIsPressed = true;
			camera.StartConstantUpRotation();
		}
		break;

	case 0x41: // a
		// Don't do anything if the key is already down
		if (!m_keyAIsPressed && !m_shiftIsPressed)
		{
			m_keyAIsPressed = true;
			camera.StartConstantLeftRotation();
		}
		break;

	case 0x53: // s
		// Don't do anything if the key is already down
		if (!m_keySIsPressed && !m_shiftIsPressed)
		{
			m_keySIsPressed = true;
			camera.StartConstantDownRotation();
		}
		break;

	case 0x44: // d
		// Don't do anything if the key is already down
		if (!m_keyDIsPressed && !m_shiftIsPressed)
		{
			m_keyDIsPressed = true;
			camera.StartConstantRightRotation();
		}
		break;

	case 0x51: // q
		// Don't do anything if the key is already down
		if (!m_keyQIsPressed && !m_shiftIsPressed)
		{
			m_keyQIsPressed = true;
			camera.StartConstantCounterClockwiseRotation();
		}
		break;

	case 0x45: // e
		// Don't do anything if the key is already down
		if (!m_keyEIsPressed && !m_shiftIsPressed)
		{
			m_keyEIsPressed = true;
			camera.StartConstantClockwiseRotation();
		}
		break;
	}

}
void SimulationWindow::HandleKeyUp(unsigned int virtualKeyCode) noexcept
{
	Camera& camera = m_renderer->GetCamera();

	switch (virtualKeyCode)
	{
	case VK_LEFT:
		m_arrowLeftIsPressed = false;
		camera.StopConstantLeftRotation();
		break;

	case VK_RIGHT:
		m_arrowRightIsPressed = false;
		camera.StopConstantRightRotation();
		break;

	case VK_UP:
		m_arrowUpIsPressed = false;
		camera.StopConstantUpRotation();
		break;

	case VK_DOWN:
		m_arrowDownIsPressed = false;
		camera.StopConstantDownRotation();
		break;

	case VK_SHIFT:
		m_shiftIsPressed = false;
		break;

	case 0x57: // w
		// No harm in calling this even if the 'w' key was hit with SHIFT down
		camera.StopConstantUpRotation();
		m_keyWIsPressed = false;
		break;

	case 0x41: // a
		// No harm in calling this even if the 'a' key was hit with SHIFT down
		camera.StopConstantLeftRotation();
		m_keyAIsPressed = false;
		break;

	case 0x53: // s
		// No harm in calling this even if the 's' key was hit with SHIFT down
		camera.StopConstantDownRotation();
		m_keySIsPressed = false;
		break;

	case 0x44: // d
		// No harm in calling this even if the 'd' key was hit with SHIFT down
		camera.StopConstantRightRotation();
		m_keyDIsPressed = false;
		break;

	case 0x51: // q
		// No harm in calling this even if the 'q' key was hit with SHIFT down
		camera.StopConstantCounterClockwiseRotation();
		m_keyQIsPressed = false;
		break;

	case 0x45: // e
		// No harm in calling this even if the 'e' key was hit with SHIFT down
		camera.StopConstantClockwiseRotation();
		m_keyEIsPressed = false;
		break;

	case VK_TAB:
		if (m_selectionBeingMovedStateIsActive)
		{
			switch (m_movementDirection)
			{
			case MovementDirection::X:
				m_movementDirection = MovementDirection::Y;
				SelectionMovementDirectionChanged();
				break;
			case MovementDirection::Y:
				m_movementDirection = MovementDirection::Z;
				SelectionMovementDirectionChanged();
				break;
			case MovementDirection::Z:
				m_movementDirection = MovementDirection::X;
				SelectionMovementDirectionChanged();
				break;
			case MovementDirection::XY:
				m_movementDirection = MovementDirection::XZ;
				SelectionMovementDirectionChanged();
				break;
			case MovementDirection::XZ:
				m_movementDirection = MovementDirection::YZ;
				SelectionMovementDirectionChanged();
				break;
			case MovementDirection::YZ:
				m_movementDirection = MovementDirection::XZ;
				SelectionMovementDirectionChanged();
				break;
			}
		}
		break;
	}
}
void SimulationWindow::HandleChar(char c) noexcept
{
	Camera& camera = m_renderer->GetCamera();

	switch (c)
	{
	case 'c':
	case 'C':
		camera.CenterOnFace();
		break;

	case 'W':
		// Dont start a 90 degree rotation if an important keyboard key is being held down
		if (!KeyboardKeyIsPressed())
			camera.Start90DegreeRotationUp();
		break;

	case 'A':
		// Dont start a 90 degree rotation if an important keyboard key is being held down
		if (!KeyboardKeyIsPressed()) 
			camera.Start90DegreeRotationLeft();
		break;

	case 'S':
		// Dont start a 90 degree rotation if an important keyboard key is being held down
		if (!KeyboardKeyIsPressed())
			camera.Start90DegreeRotationDown();
		break;

	case 'D':
		// Dont start a 90 degree rotation if an important keyboard key is being held down
		if (!KeyboardKeyIsPressed())
			camera.Start90DegreeRotationRight();
		break;

	case 'Q':
		// Dont start a 90 degree rotation if an important keyboard key is being held down
		if (!KeyboardKeyIsPressed())
			camera.Start90DegreeRotationCounterClockwise();
		break;

	case 'E':
		// Dont start a 90 degree rotation if an important keyboard key is being held down
		if (!KeyboardKeyIsPressed())
			camera.Start90DegreeRotationClockwise();
		break;

	case 'x':
		if (m_selectionBeingMovedStateIsActive && m_movementDirection != MovementDirection::X)
		{
			m_movementDirection = MovementDirection::X;
			SelectionMovementDirectionChanged();
		}
		break;
	case 'y':
		if (m_selectionBeingMovedStateIsActive && m_movementDirection != MovementDirection::Y)
		{
			m_movementDirection = MovementDirection::Y;
			SelectionMovementDirectionChanged();
		}
		break;
	case 'z':
		if (m_selectionBeingMovedStateIsActive && m_movementDirection != MovementDirection::Z)
		{
			m_movementDirection = MovementDirection::Z;
			SelectionMovementDirectionChanged();
		}
		break;

	case 'X':
	{
		if (m_selectionBeingMovedStateIsActive)
		{
			switch (m_movementDirection)
			{
			case MovementDirection::X: break;
			case MovementDirection::Y:
				m_movementDirection = MovementDirection::XY;
				SelectionMovementDirectionChanged();
				break;
			case MovementDirection::Z:
				m_movementDirection = MovementDirection::XZ;
				SelectionMovementDirectionChanged(); 
				break;
			default:
				m_movementDirection = MovementDirection::X;
				SelectionMovementDirectionChanged(); 
				break;
			}
		}
		break;
	}
	case 'Y':
	{
		if (m_selectionBeingMovedStateIsActive)
		{
			switch (m_movementDirection)
			{
			case MovementDirection::Y: break;
			case MovementDirection::X:
				m_movementDirection = MovementDirection::XY;
				SelectionMovementDirectionChanged();
				break;
			case MovementDirection::Z:
				m_movementDirection = MovementDirection::YZ;
				SelectionMovementDirectionChanged();
				break;
			default:
				m_movementDirection = MovementDirection::Y;
				SelectionMovementDirectionChanged();
				break;
			}
		}
		break;
	}
	case 'Z':
	{
		if (m_selectionBeingMovedStateIsActive)
		{
			switch (m_movementDirection)
			{
			case MovementDirection::Z: break;
			case MovementDirection::X:
				m_movementDirection = MovementDirection::XZ;
				SelectionMovementDirectionChanged();
				break;
			case MovementDirection::Y:
				m_movementDirection = MovementDirection::YZ;
				SelectionMovementDirectionChanged();
				break;
			default:
				m_movementDirection = MovementDirection::Z;
				SelectionMovementDirectionChanged();
				break;
			}
		}
		break;
	}

	}
}


std::optional<size_t> SimulationWindow::PickAtom(float x, float y)
{
	std::optional<size_t> pickedAtom = std::nullopt;

	Camera& camera = m_renderer->GetCamera();
	XMMATRIX projection = camera.GetProj();
	XMMATRIX view = camera.GetView();

	BoundingBox bounds = m_sphereMeshGroup->GetSubmesh(0).Bounds; 
	BoundingSphere sphere = m_sphereMeshGroup->GetSubmesh(0).Sphere;

	XMVECTOR clickpointNear = XMVectorSet(x, y, 0.0f, 1.0f);
	XMVECTOR clickpointFar = XMVectorSet(x, y, 1.0f, 1.0f);

	XMVECTOR origin, destination, direction; 

	float minDistance = FLT_MAX; 
	float distance = FLT_MAX;
	for (const Atom& atom : m_simulation.GetAtoms()) 
	{
		// Construct world matrix
		const DirectX::XMFLOAT3& p = atom.position;
		const float radius = atom.radius / 2;
		XMMATRIX world = XMMatrixScaling(radius, radius, radius) * XMMatrixTranslation(p.x, p.y, p.z);

		origin = XMVector3Unproject(
			clickpointNear, 
			m_viewport.TopLeftX,
			m_viewport.TopLeftY,
			m_viewport.Width,
			m_viewport.Height,
			m_viewport.MinDepth, 
			m_viewport.MaxDepth,  
			projection, 
			view, 
			world); 

		destination = XMVector3Unproject( 
			clickpointFar,
			m_viewport.TopLeftX,
			m_viewport.TopLeftY,
			m_viewport.Width,
			m_viewport.Height,
			m_viewport.MinDepth,
			m_viewport.MaxDepth,
			projection, 
			view, 
			world);

		direction = XMVector3Normalize(destination - origin); 

//		if (bounds.Intersects(origin, direction, distance))
//		{
//			LOG_TRACE("INTERSECTION: {}", distance); 
//		}
		if (sphere.Intersects(origin, direction, distance))
		{
			// The distance we get back is relative to the scale that was used when calling XMVector3Unproject
			// In order to get them back to the same scale, we must multiply by the scale factor that was used
			distance *= radius;

			if (distance < minDistance)
			{
				minDistance = distance;
				pickedAtom = m_simulation.IndexOf(atom);
			}
		}
	}

	return pickedAtom;
}
void SimulationWindow::PickBoxWalls(float x, float y)
{
	Camera& camera = m_renderer->GetCamera(); 
	XMMATRIX projection = camera.GetProj(); 
	XMMATRIX view = camera.GetView();

	XMVECTOR clickpointNear = XMVectorSet(x, y, 0.0f, 1.0f);   
	XMVECTOR clickpointFar = XMVectorSet(x, y, 1.0f, 1.0f);  

	DirectX::XMFLOAT3 dims = m_simulation.GetDimensionMaxs();
	XMMATRIX world = XMMatrixScaling(dims.x, dims.y, dims.z);

	XMVECTOR origin = XMVector3Unproject(
		clickpointNear,
		m_viewport.TopLeftX,
		m_viewport.TopLeftY,
		m_viewport.Width,
		m_viewport.Height,
		m_viewport.MinDepth,
		m_viewport.MaxDepth,
		projection,
		view,
		world);

	XMVECTOR destination = XMVector3Unproject(
		clickpointFar,
		m_viewport.TopLeftX,
		m_viewport.TopLeftY,
		m_viewport.Width,
		m_viewport.Height,
		m_viewport.MinDepth,
		m_viewport.MaxDepth,
		projection,
		view,
		world);

	XMVECTOR direction = XMVector3Normalize(destination - origin);

	float distance = FLT_MAX;
	float minDistance = FLT_MAX;
	ClearMouseHoverWallState();

	// X
	if (m_boundingBoxPosX.Intersects(origin, direction, distance))
	{
		if (distance < minDistance)
		{
			minDistance = distance;
			ClearMouseHoverWallState();
			m_mouseHoveringBoxWallPosX = true;
		}
	}
	if (m_boundingBoxNegX.Intersects(origin, direction, distance))
	{
		if (distance < minDistance)
		{
			minDistance = distance;
			ClearMouseHoverWallState();
			m_mouseHoveringBoxWallNegX = true;
		}
	}

	// Y
	if (m_boundingBoxPosY.Intersects(origin, direction, distance))
	{
		if (distance < minDistance)
		{
			minDistance = distance;
			ClearMouseHoverWallState();
			m_mouseHoveringBoxWallPosY = true;
		}
	}
	if (m_boundingBoxNegY.Intersects(origin, direction, distance))
	{
		if (distance < minDistance)
		{
			minDistance = distance;
			ClearMouseHoverWallState();
			m_mouseHoveringBoxWallNegY = true;
		}
	}

	// Z
	if (m_boundingBoxPosZ.Intersects(origin, direction, distance))
	{
		if (distance < minDistance)
		{
			minDistance = distance;
			ClearMouseHoverWallState();
			m_mouseHoveringBoxWallPosZ = true;
		}
	}
	if (m_boundingBoxNegZ.Intersects(origin, direction, distance))
	{
		if (distance < minDistance)
		{
			minDistance = distance;
			ClearMouseHoverWallState();
			m_mouseHoveringBoxWallNegZ = true;
		}
	}
}


}