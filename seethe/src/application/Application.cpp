#include "Application.h"
#include "utils/Log.h"

#include <windowsx.h> // Included so we can use GET_X_LPARAM/GET_Y_LPARAM

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function on Simulation
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
Application::Application() :
	m_timer()
{
	std::fill(std::begin(m_fences), std::end(m_fences), 0);
	m_mainWindow = std::make_unique<MainWindow>(this);
	m_deviceResources = std::make_shared<DeviceResources>(m_mainWindow->GetHWND(), m_mainWindow->GetHeight(), m_mainWindow->GetWidth());
	m_timer.Reset();

	m_materials.materials[0] = { DirectX::XMFLOAT4(DirectX::Colors::ForestGreen), { 0.02f, 0.02f, 0.02f }, 0.1f };
	m_materials.materials[1] = { DirectX::XMFLOAT4(DirectX::Colors::AliceBlue), { 0.02f, 0.02f, 0.02f }, 0.1f };
	m_materials.materials[2] = { DirectX::XMFLOAT4(DirectX::Colors::Aqua), { 0.02f, 0.02f, 0.02f }, 0.1f };
	m_materials.materials[3] = { DirectX::XMFLOAT4(DirectX::Colors::Azure), { 0.02f, 0.02f, 0.02f }, 0.1f };
	m_materials.materials[4] = { DirectX::XMFLOAT4(DirectX::Colors::BlanchedAlmond), { 0.02f, 0.02f, 0.02f }, 0.1f };
	m_materials.materials[5] = { DirectX::XMFLOAT4(DirectX::Colors::Chartreuse), { 0.02f, 0.02f, 0.02f }, 0.1f };
	m_materials.materials[6] = { DirectX::XMFLOAT4(DirectX::Colors::DarkGoldenrod), { 0.02f, 0.02f, 0.02f }, 0.1f };
	m_materials.materials[7] = { DirectX::XMFLOAT4(DirectX::Colors::Firebrick), { 0.02f, 0.02f, 0.02f }, 0.1f };
	m_materials.materials[8] = { DirectX::XMFLOAT4(DirectX::Colors::Moccasin), { 0.02f, 0.02f, 0.02f }, 0.1f };
	m_materials.materials[9] = { DirectX::XMFLOAT4(DirectX::Colors::Thistle), { 0.02f, 0.02f, 0.02f }, 0.1f };



	m_simulation.AddAtom(AtomType::HYDROGEN, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	m_simulation.AddAtom(AtomType::HELIUM, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f));
	m_simulation.AddAtom(AtomType::LITHIUM, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f));
	m_simulation.AddAtom(AtomType::BERYLLIUM, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f));
	m_simulation.AddAtom(AtomType::BORON, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_simulation.AddAtom(AtomType::CARBON, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f));
	m_simulation.AddAtom(AtomType::NITROGEN, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f));
	m_simulation.AddAtom(AtomType::OXYGEN, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f));
	m_simulation.AddAtom(AtomType::FLOURINE, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(-1.0f, 1.0f, 0.0f));
	m_simulation.AddAtom(AtomType::NEON, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 1.0f, -1.0f));





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
	
	

	// Must intialize the simulation windows AFTER the command list has been reset
	m_simulationWindows.emplace_back(m_deviceResources, m_simulation, m_materials, 0.0f, 0.0f, m_mainWindow->GetHeight(), m_mainWindow->GetWidth());



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

	for (auto& simulationWindow : m_simulationWindows)
		simulationWindow.Update(m_timer, m_currentFrameIndex);
}
void Application::RenderUI()
{
	static bool opt_fullscreen = true;
	static bool opt_padding = false;

	static bool editMaterials = true;
	static bool editLighting = false;

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
			if (ImGui::BeginMenu("Edit"))
			{
				ImGui::MenuItem("Materials", NULL, &editMaterials);
				ImGui::MenuItem("Lighting", NULL, &editLighting);
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
		if (editMaterials)
		{
			ImGui::Begin("Materials");

			static int elementIndex = 0;
			ImGui::Combo("##MaterialEditElementCombo", &elementIndex, "Hydrogen\0Helium\0Lithium\0Beryllium\0Boron\0Carbon\0Nitrogen\0Oxygen\0Flourine\0Neon\0\0");

			Material& material = m_materials.materials[elementIndex];

			auto materialChangedFn = [this]()
				{
					for (auto& window : m_simulationWindows)
						window.SetMaterialsDirtyFlag();
				};

			if (ImGui::ColorEdit4("Diffuse Albedo", (float*)&material.DiffuseAlbedo, ImGuiColorEditFlags_AlphaPreview))
				materialChangedFn();
			if (ImGui::ColorEdit3("FresnelR0", (float*)&material.FresnelR0, ImGuiColorEditFlags_AlphaPreview))
				materialChangedFn();
			if (ImGui::DragFloat("Roughness", &material.Roughness, 0.005f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
				materialChangedFn();


			ImGui::End();
		}

		if (editLighting)
		{
			ImGui::Begin("Lighting");
			ImGui::Text("We are editing lighting now");
			ImGui::End();
		}

	}

	// Bottom Panel
	{
		ImGui::Begin("Bottom Panel");

		ImGui::Text("Text on the Bottom");

		ImGui::End();
	}

	// Viewport
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoBackground;
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
		m_simulationWindows[0].SetWindow(pos.y, pos.x, ImGui::GetWindowHeight(), ImGui::GetWindowWidth());

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
	FLOAT color[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
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


	//////
	// RENDER STUFF HERE (on top of UI ??)
	//////
	for (auto& simulationWindow : m_simulationWindows) 
		simulationWindow.Render(m_currentFrameIndex);


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

void Application::ForwardMessageToWindows(std::function<bool(SimulationWindow*)>&& fn)
{
	// If there is a window that is selected/being interacted with, then pass the event to that window first
	// Otherwise, pass it to all windows, but if one of them returns true, then we know that window wants to be selected
	if (m_simulationWindowSelected.has_value())
	{
		if (fn(m_simulationWindowSelected.value()))
			return;

		for (SimulationWindow& window : m_simulationWindows)
		{
			if (m_simulationWindowSelected.value() != &window && fn(&window))
			{
				m_simulationWindowSelected = &window;
				return;
			}
		}
	}
	else
	{
		for (SimulationWindow& window : m_simulationWindows)
		{
			if (fn(&window))
			{
				m_simulationWindowSelected = &window;
				return;
			}
		}
	}
}

LRESULT Application::MainWindowOnCreate(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// CREATESTRUCT* cs = (CREATESTRUCT*)lParam;

	// According to: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-create
	// --> "An application should return zero if it processes this message."
	// return 0;

	return DefWindowProc(hWnd, msg, wParam, lParam);
}
LRESULT Application::MainWindowOnClose(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PostQuitMessage(0);
	return 0;
}
LRESULT Application::MainWindowOnLButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse LButton Down event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnLButtonDown(pt.x, pt.y); });

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-lbuttondown
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnLButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse LButton Up event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnLButtonUp(pt.x, pt.y); });

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
LRESULT Application::MainWindowOnLButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse LButton DblClck event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnLButtonDoubleClick(pt.x, pt.y); });

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-lbuttondblclk
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnRButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse RButton DblClck event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnRButtonDoubleClick(pt.x, pt.y); });

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-rbuttondblclk
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse MButton DblClck event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnMButtonDoubleClick(pt.x, pt.y); });

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mbuttondblclk
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse MButton Down event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnMButtonDown(pt.x, pt.y); });

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mbuttondown
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse MButton Up event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnMButtonUp(pt.x, pt.y); });

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
LRESULT Application::MainWindowOnRButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse RButton Down event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnRButtonDown(pt.x, pt.y); });

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-rbuttondown
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnRButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse RButton Up event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnRButtonUp(pt.x, pt.y); });

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
LRESULT Application::MainWindowOnX1ButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse XButton1 Down event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnX1ButtonDown(pt.x, pt.y); });

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-xbuttondown
	// --> "If an application processes this message, it should return TRUE."
	return true;
}
LRESULT Application::MainWindowOnX2ButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse XButton2 Down event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnX2ButtonDown(pt.x, pt.y); });

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-xbuttondown
	// --> "If an application processes this message, it should return TRUE."
	return true;
}
LRESULT Application::MainWindowOnX1ButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse XButton1 Up event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnX1ButtonUp(pt.x, pt.y); });

	if (pt.x < 0 || pt.x >= m_mainWindow->GetWidth() || pt.y < 0 || pt.y >= m_mainWindow->GetHeight())
	{
		ReleaseCapture();

		// ... Mouse Leave event ...
	}

	m_mainWindow->BringToForeground();

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-xbuttonup
	// --> "If an application processes this message, it should return TRUE."
	return true;
}
LRESULT Application::MainWindowOnX2ButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse XButton2 Up event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnX2ButtonUp(pt.x, pt.y); });

	if (pt.x < 0 || pt.x >= m_mainWindow->GetWidth() || pt.y < 0 || pt.y >= m_mainWindow->GetHeight())
	{
		ReleaseCapture();

		// ... Mouse Leave event ...
	}

	m_mainWindow->BringToForeground();

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-xbuttonup
	// --> "If an application processes this message, it should return TRUE."
	return true;
}
LRESULT Application::MainWindowOnX1ButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse XButton1 DblClck event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnX1ButtonDoubleClick(pt.x, pt.y); });

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-xbuttondblclk
	// --> "If an application processes this message, it should return TRUE."
	return true;
}
LRESULT Application::MainWindowOnX2ButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse XButton2 DblClck event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnX2ButtonDoubleClick(pt.x, pt.y); }); 

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-xbuttondblclk
	// --> "If an application processes this message, it should return TRUE."
	return true;
}
LRESULT Application::MainWindowOnResize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// NOTE: Do not update the viewport here. The viewport gets updated every frame in RenderUI() 
	int height = m_mainWindow->GetHeight();
	int width = m_mainWindow->GetWidth();
	m_deviceResources->OnResize(height, width);

	// According to: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-size
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMouseMove(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// ... Mouse Move event ...
	ForwardMessageToWindows([&pt](SimulationWindow* window) -> bool { return window->OnMouseMove(pt.x, pt.y); });


	
	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousemove
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMouseEnter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// NOTE: MouseEnter is not a real Win32 event. It is really just a WM_MOVE event that we are calling an "enter" event
	//       when the mouse enters a window. Therefore, it should mostly be treated like a WM_MOVE event

	// ... Mouse Enter event ...

	return 0;
}
LRESULT Application::MainWindowOnMouseLeave(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// NOTE: There is such a thing as a Win32 mouse leave event, however, they don't seem as reliable as just
	//       testing for leaving when the mouse moves. Therefore, this is really just a WM_MOVE event that we are calling a "leave" event
	//       when the mouse leaves a window. Therefore, it should mostly be treated like a WM_MOVE event

	// ... Mouse Leave event ...

	return 0;
}
LRESULT Application::MainWindowOnMouseWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Kind of weird behavior, but the mouse location stored in the lParam does not exactly match
	// the actual location of the mouse on the screen. So instead of calling MAKEPOINTS(lParam), we are
	// just going to keep track of mouse location and can make use of those values instead

	const int delta = GET_WHEEL_DELTA_WPARAM(wParam); 

	// ... Mouse Wheel event ...
	ForwardMessageToWindows([&delta](SimulationWindow* window) -> bool { return window->OnMouseWheelVertical(delta); });


	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousewheel
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnMouseHWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Kind of weird behavior, but the mouse location stored in the lParam does not exactly match
	// the actual location of the mouse on the screen. So instead of calling MAKEPOINTS(lParam), we are
	// just going to keep track of mouse location and can make use of those values instead

	const int delta = GET_WHEEL_DELTA_WPARAM(wParam);

	// ... Mouse H Wheel event ...
	ForwardMessageToWindows([&delta](SimulationWindow* window) -> bool { return window->OnMouseWheelHorizontal(delta); }); 


	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousehwheel
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnChar(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// wParam - Contains the keycode
	// lParam - Bits 0-15 contain the repeat count
	char keycode = static_cast<char>(wParam);
	int repeats = static_cast<int>(LOWORD(lParam));

	// ... Char event ...
	ForwardMessageToWindows([&keycode](SimulationWindow* window) -> bool { return window->OnChar(keycode); });

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-char
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnKeyUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// wParam - Contains the keycode
	unsigned int keycode = static_cast<unsigned int>(wParam);

	// ... Key Up event ...
	ForwardMessageToWindows([&keycode](SimulationWindow* window) -> bool { return window->OnKeyUp(keycode); });
		

	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keyup
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnKeyDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// wParam - Contains the keycode
	// lParam - Bits 0-15 contain the repeat count
	// lParam - Bit 30 indicates whether the key was already down before the WM_KEYDOWN message was triggered
	unsigned int keycode = static_cast<unsigned int>(wParam);
	int repeats = static_cast<int>(LOWORD(lParam)); 
	bool keyWasPreviouslyDown = (lParam & 0x40000000) > 0;

	// ... Key Down event ...
	ForwardMessageToWindows([&keycode](SimulationWindow* window) -> bool { return window->OnKeyDown(keycode); });


	// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keydown
	// --> "An application should return zero if it processes this message."
	return 0;
}
LRESULT Application::MainWindowOnKillFocus(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
}


#pragma pop_macro("AddAtom") // See note above for 'AddAtom'