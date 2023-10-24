#include "Application.h"
#include "utils/Log.h"

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function on Simulation
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
Application::Application() :
	m_timer(),
	m_viewport{},
	m_scissorRect{}
{
	// Initialize all fence values to 0
	std::fill(std::begin(m_fences), std::end(m_fences), 0);

	m_mainWindow = std::make_unique<MainWindow>(this);

	m_scissorRect = { 0, 0, m_mainWindow->GetWidth(), m_mainWindow->GetHeight() }; 

	m_timer.Reset();

	m_simulation.AddAtom(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	m_simulation.AddAtom(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));




	m_deviceResources = std::make_shared<DeviceResources>(m_mainWindow->GetHWND(), m_mainWindow->GetHeight(), m_mainWindow->GetWidth());

	// Initialize the descriptor vector
	m_descriptorVector = std::make_unique<DescriptorVector>(m_deviceResources, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_descriptorVector->IncrementCount(); // ImGUI will use the first slot in the descriptor vector for its font's SRV, therefore, we need to manually increment the count within the descriptor vector

	// Initialize allocators
	auto device = m_deviceResources->GetDevice(); 
	for (unsigned int iii = 0; iii < g_numFrameResources; ++iii) 
	{
		GFX_THROW_INFO( 
			device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT, 
				IID_PPV_ARGS(m_allocators[iii].GetAddressOf()) 
			)
		);
	}

	// Reset the command list so we can execute commands when initializing the renderer
	GFX_THROW_INFO(m_deviceResources->GetCommandList()->Reset(m_deviceResources->GetCommandAllocator(), nullptr));
	
	m_renderer = std::make_unique<Renderer>(m_deviceResources, m_viewport, m_scissorRect);

	InitializeRenderPasses();

	// Execute the initialization commands.
	GFX_THROW_INFO(m_deviceResources->GetCommandList()->Close()); 
	ID3D12CommandList* cmdsLists[] = { m_deviceResources->GetCommandList() }; 
	GFX_THROW_INFO_ONLY(m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists));

	// Wait until initialization is complete.
	m_deviceResources->FlushCommandQueue();


	// ==================================================================================
	// ImGUI
	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(m_mainWindow->GetHWND());
	ImGui_ImplDX12_Init(
		m_deviceResources->GetDevice(),				// YOUR_D3D_DEVICE
		g_numFrameResources,						// NUM_FRAME_IN_FLIGHT
		m_deviceResources->GetBackBufferFormat(),	// YOUR_RENDER_TARGET_DXGI_FORMAT
		m_descriptorVector->GetRawHeapPointer(),	// YOUR_SRV_DESC_HEAP
		// You'll need to designate a descriptor from your descriptor heap for Dear ImGui to use internally for its font texture's SRV
		m_descriptorVector->GetCPUHandleAt(0),		// YOUR_CPU_DESCRIPTOR_HANDLE_FOR_FONT_SRV,
		m_descriptorVector->GetGPUHandleAt(0)		// YOUR_GPU_DESCRIPTOR_HANDLE_FOR_FONT_SRV);
	);

	ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	ASSERT(font != nullptr, "Could not find font");
}
Application::~Application()
{
	ImGui_ImplDX12_Shutdown(); 
	ImGui_ImplWin32_Shutdown(); 
	ImGui::DestroyContext(); 
}

void Application::InitializeRenderPasses()
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
}
MeshData Application::SphereMesh(float radius, uint32_t sliceCount, uint32_t stackCount)
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

int Application::Run()
{
	while (true)
	{
		// process all messages pending, but to not block for new messages
		if (const auto ecode = m_mainWindow->ProcessMessages())
		{
			// if return optional has value, means we're quitting so return exit code
			return *ecode;
		}

		try
		{
			m_timer.Tick();
			
			// Start the ImGui frame
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			RenderUI();

			// Call Update() AFTER calling RenderUI
			// This is important because running the ImGUI code first may update values (ex. the viewport)
			// that will be important during the update phase
			Update();

			Render();
			Present();
		}
		catch (std::runtime_error& e)
		{
			LOG_ERROR("*** Caught runtime_error ***\n{}\n****************************", e.what());
			return 1;
		}
		catch (std::exception& e)
		{
			LOG_ERROR("*** Caught std::exception ***\n{}\n****************************", e.what());
			return 2;
		}
		catch (...)
		{
			LOG_ERROR("{}", "*** Caught unknown exception ***\nNo information available\n****************************");
			return 3;
		}
	}
}
void Application::Update()
{
	// Cycle through the circular frame resource array.
	m_currentFrameIndex = (m_currentFrameIndex + 1) % g_numFrameResources;

	// Has the GPU finished processing the commands of the current frame?
	// If not, wait until the GPU has completed commands up to this fence point.
	UINT64 currentFence = m_fences[m_currentFrameIndex]; 
	{
		if (currentFence != 0 && m_deviceResources->GetFence()->GetCompletedValue() < currentFence) 
		{
			HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS); 
			GFX_THROW_INFO(
				m_deviceResources->GetFence()->SetEventOnCompletion(currentFence, eventHandle) 
			);

			ASSERT(eventHandle != NULL, "Handle should not be null"); 
			WaitForSingleObject(eventHandle, INFINITE); 
			CloseHandle(eventHandle); 
		}
	}


	auto commandList = m_deviceResources->GetCommandList();

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = m_allocators[m_currentFrameIndex];
	GFX_THROW_INFO(m_allocators[m_currentFrameIndex]->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	// NOTE: When resetting the commandlist, we are allowed to specify the PSO we want the command list to have.
	//       However, this is slightly inconvenient given the way we have structured the loop below. According to
	//		 the documentation for resetting using nullptr: "If NULL, the runtime sets a dummy initial pipeline 
	//		 state so that drivers don't have to deal with undefined state. The overhead for this is low, 
	//		 particularly for a command list, for which the overall cost of recording the command list likely 
	//		 dwarfs the cost of one initial state setting."
	GFX_THROW_INFO(commandList->Reset(commandAllocator.Get(), nullptr));
	
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_descriptorVector->GetRawHeapPointer() };
	GFX_THROW_INFO_ONLY(
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps)
	);



	m_simulation.Update(m_timer);
	m_renderer->Update(m_timer, m_currentFrameIndex);
}
void Application::RenderUI()
{
	static bool opt_fullscreen = true;
	static bool opt_padding = false;

	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		const ImGuiViewport* viewport = ImGui::GetMainViewport(); 
		ImGui::SetNextWindowPos(viewport->WorkPos); 
		ImGui::SetNextWindowSize(viewport->WorkSize); 
		ImGui::SetNextWindowViewport(viewport->ID); 
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("My DockSpace", nullptr, window_flags);
		ImGui::PopStyleVar(3);

		ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(dockspace_id);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Options")) 
			{
				ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen); 
				ImGui::MenuItem("Padding", NULL, &opt_padding); 
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Examples"))
			{
				ImGui::MenuItem("Fullscreen2", NULL, &opt_fullscreen);
				ImGui::MenuItem("Padding2", NULL, &opt_padding);
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar(); 
		}

		ImGui::End();
	}

	// Left Panel
	{
		//ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		//window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove; 
		//window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		//window_flags |= ImGuiWindowFlags_NoNavFocus;
		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_NoCollapse;
		window_flags |= ImGuiWindowFlags_NoTitleBar;

		ImGui::Begin("Left Panel", nullptr, window_flags);

		ImGui::Text("Text on the Left");

		ImGui::End();
	}

	// Right Panel
	{
		ImGui::Begin("Right Panel");

		ImGui::Text("Text on the Right");

		ImGui::End();
	}

	// Bottom Panel
	{
		ImGui::Begin("Bottom Panel");

		ImGui::Text("Text on the Bottom");

		ImGui::End();
	}

	// Viewport
	{
		ImGuiWindowFlags window_flags = 0;
		//window_flags |= ImGuiWindowFlags_NoDocking;
		//window_flags |= ImGuiWindowFlags_NoTitleBar; 
		//window_flags |= ImGuiWindowFlags_NoCollapse;
		//window_flags |= ImGuiWindowFlags_NoResize;
		//window_flags |= ImGuiWindowFlags_NoMove; 
		//window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		//window_flags |= ImGuiWindowFlags_NoNavFocus;
		bool open = ImGui::Begin("Viewport", nullptr, window_flags);
		ASSERT(open, "Viewport window should never be closed");
		ImVec2 pos = ImGui::GetWindowPos();
		m_viewport.TopLeftX = pos.x;
		m_viewport.TopLeftY = pos.y;
		m_viewport.Height = ImGui::GetWindowHeight();
		m_viewport.Width = ImGui::GetWindowWidth();
		ImGui::End();
	}

	// Call ImGUI::Render here - NOTE: It will not actually render to the back buffer, but rather an
	//							 internal buffer. We have to call ImGui_ImplDX12_RenderDrawData below
	//                           to have it actually render to the back buffer
	ImGui::ShowDemoWindow();
	ImGui::Render();
}
void Application::Render()
{
	auto commandList = m_deviceResources->GetCommandList();

	// Indicate a state transition on the resource usage.
	auto _b = CD3DX12_RESOURCE_BARRIER::Transition(
		m_deviceResources->CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	GFX_THROW_INFO_ONLY(commandList->ResourceBarrier(1, &_b));

	// Clear the back buffer and depth buffer.
	FLOAT color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	GFX_THROW_INFO_ONLY(commandList->ClearRenderTargetView(m_deviceResources->CurrentBackBufferView(), color, 0, nullptr));
	GFX_THROW_INFO_ONLY(commandList->ClearDepthStencilView(m_deviceResources->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr));

	// Specify the buffers we are going to render to.
	auto currentBackBufferView = m_deviceResources->CurrentBackBufferView();
	auto depthStencilView = m_deviceResources->DepthStencilView();
	GFX_THROW_INFO_ONLY(
		commandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView)
	);

	// Render ImGui content to back buffer
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	// NOTE: Setting the viewport must come AFTER calling ImGui_ImplDX12_RenderDrawData because it overwrites the viewport
//	GFX_THROW_INFO_ONLY(commandList->RSSetViewports(1, &m_viewport));
//	GFX_THROW_INFO_ONLY(commandList->RSSetScissorRects(1, &m_scissorRect));

	//////
	// RENDER STUFF HERE (on top of UI ??)
	//////
	m_renderer->Render(m_simulation, m_currentFrameIndex);






	// Indicate a state transition on the resource usage.
	auto _b2 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_deviceResources->CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	GFX_THROW_INFO_ONLY(commandList->ResourceBarrier(1, &_b2));

	// Done recording commands.
	GFX_THROW_INFO(commandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { commandList };
	GFX_THROW_INFO_ONLY(
		m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists)
	);
}
void Application::Present()
{
	m_deviceResources->Present(); 

	m_fences[m_currentFrameIndex] = m_deviceResources->GetCurrentFenceValue(); 

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	GFX_THROW_INFO( 
		m_deviceResources->GetCommandQueue()->Signal(m_deviceResources->GetFence(), m_fences[m_currentFrameIndex]) 
	);

	// At the end of each frame, we need to clean up all resources that were previously
	// passed to DeviceResources::DelayedDelete()
	m_deviceResources->CleanupResources();
}

LRESULT Application::MainWindowOnCreate(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	// CREATESTRUCT* cs = (CREATESTRUCT*)lParam;

	// According to: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-create
	// --> "An application should return zero if it processes this message."
	// return 0;

	return DefWindowProc(hWnd, msg, wParam, lParam);
}
LRESULT Application::MainWindowOnClose(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	PostQuitMessage(0);
	return 0;
}
LRESULT Application::MainWindowOnLButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	const POINTS pt = MAKEPOINTS(lParam);

	// ... Mouse LButton Down event ...



	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-lbuttondown
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnLButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	const POINTS pt = MAKEPOINTS(lParam);

	// ... Mouse LButton Up event ...

	if (pt.x < 0 || pt.x >= m_mainWindow->GetWidth() || pt.y < 0 || pt.y >= m_mainWindow->GetHeight())
	{
		ReleaseCapture();

		// ... Mouse Leave event ...
	}

	m_mainWindow->BringToForeground();

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-lbuttonup
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnLButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	const POINTS pt = MAKEPOINTS(lParam);

	// ... Mouse LButton DblClck event ...



	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-lbuttondblclk
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnRButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	const POINTS pt = MAKEPOINTS(lParam);

	// ... Mouse RButton DblClck event ...

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-rbuttondblclk
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	const POINTS pt = MAKEPOINTS(lParam);

	// ... Mouse MButton DblClck event ...

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mbuttondblclk
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	const POINTS pt = MAKEPOINTS(lParam);

	// ... Mouse MButton Down event ...

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mbuttondown
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	const POINTS pt = MAKEPOINTS(lParam);

	// ... Mouse MButton Up event ...

	if (pt.x < 0 || pt.x >= m_mainWindow->GetWidth() || pt.y < 0 || pt.y >= m_mainWindow->GetHeight())
	{
		ReleaseCapture();

		// ... Mouse Leave event ...
	}

	m_mainWindow->BringToForeground();

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mbuttonup
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnRButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	const POINTS pt = MAKEPOINTS(lParam);

	// ... Mouse RButton Down event ...

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-rbuttondown
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnRButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	const POINTS pt = MAKEPOINTS(lParam);

	// ... Mouse RButton Up event ...

	if (pt.x < 0 || pt.x >= m_mainWindow->GetWidth() || pt.y < 0 || pt.y >= m_mainWindow->GetHeight())
	{
		ReleaseCapture();

		// ... Mouse Leave event ...
	}

	m_mainWindow->BringToForeground();

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-rbuttonup
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnResize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// NOTE: Do not update the viewport here. The viewport gets updated every frame in RenderUI() 
	int height = m_mainWindow->GetHeight();
	int width = m_mainWindow->GetWidth();
	m_deviceResources->OnResize(height, width);
	m_scissorRect = { 0, 0, width, height };

	// According to: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-size
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMouseMove(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	// ... Mouse Move event ...
	 
	
	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousemove
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMouseEnter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	// NOTE: MouseEnter is not a real Win32 event. It is really just a WM_MOVE event that we are calling an "enter" event
	//       when the mouse enters a window. Therefore, it should mostly be treated like a WM_MOVE event

	// ... Mouse Enter event ...

	return 0;
}
LRESULT Application::MainWindowOnMouseLeave(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	// NOTE: There is such a thing as a Win32 mouse leave event, however, they don't seem as reliable as just
	//       testing for leaving when the mouse moves. Therefore, this is really just a WM_MOVE event that we are calling a "leave" event
	//       when the mouse leaves a window. Therefore, it should mostly be treated like a WM_MOVE event

	// ... Mouse Leave event ...

	return 0;
}
LRESULT Application::MainWindowOnMouseWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	// Kind of weird behavior, but the mouse location stored in the lParam does not exactly match
	// the actual location of the mouse on the screen. So instead of calling MAKEPOINTS(lParam), we are
	// just going to keep track of mouse location and can make use of those values instead

	const int delta = GET_WHEEL_DELTA_WPARAM(wParam); 

	// ... Mouse Wheel event ...

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousewheel
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMouseHWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	// Kind of weird behavior, but the mouse location stored in the lParam does not exactly match
	// the actual location of the mouse on the screen. So instead of calling MAKEPOINTS(lParam), we are
	// just going to keep track of mouse location and can make use of those values instead

	const int delta = GET_WHEEL_DELTA_WPARAM(wParam);

	// ... Mouse H Wheel event ...

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousehwheel
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnChar(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	// wParam - Contains the keycode
	// lParam - Bits 0-15 contain the repeat count
	char keycode = static_cast<char>(wParam);
	int repeats = static_cast<int>(LOWORD(lParam));

	// ... Char event ...

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-char
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnKeyUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	// wParam - Contains the keycode
	char keycode = static_cast<char>(wParam);

	// ... Key Up event ...

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keyup
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnKeyDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	// wParam - Contains the keycode
	// lParam - Bits 0-15 contain the repeat count
	// lParam - Bit 30 indicates whether the key was already down before the WM_KEYDOWN message was triggered
	char keycode = static_cast<char>(wParam);
	int repeats = static_cast<int>(LOWORD(lParam)); 
	bool keyWasPreviouslyDown = (lParam & 0x40000000) > 0;

	// ... Key Down event ...

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keydown
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnKillFocus(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
}


#pragma pop_macro("AddAtom") // See note above for 'AddAtom'