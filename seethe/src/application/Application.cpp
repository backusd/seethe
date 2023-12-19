#include "Application.h"
#include "utils/Log.h"
#include "utils/String.h"
#include "application/ui/fonts/Fonts.h"
#include "application/change-requests/AddAtomsCR.h"
#include "application/change-requests/AtomMaterialCR.h"
#include "application/change-requests/AtomsMovedCR.h"
#include "application/change-requests/AtomVelocityCR.h"
#include "application/change-requests/BoxResizeCR.h"
#include "application/change-requests/RemoveAtomsCR.h"
#include "application/change-requests/SimulationPlayCR.h"

#include <windowsx.h> // Included so we can use GET_X_LPARAM/GET_Y_LPARAM

using namespace DirectX;

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function on Simulation
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
Application::Application() :
	m_timer()
{
}
Application::~Application()
{
	ImGui_ImplDX12_Shutdown(); 
	ImGui_ImplWin32_Shutdown(); 
	ImGui::DestroyContext(); 
}

void Application::Initialize()
{
	std::fill(std::begin(m_fences), std::end(m_fences), 0);

	m_mainWindow = std::make_unique<MainWindow>(this);
	m_deviceResources = m_mainWindow->GetDeviceResources();

	m_timer.Reset();

	InitializeMaterials();

	m_mainLighting = std::make_unique<SceneLighting>();
	m_mainLighting->AmbientLight({ 0.25f, 0.25f, 0.25f, 1.0f });
	m_mainLighting->AddDirectionalLight({ 0.9f, 0.9f, 0.9f }, { 0.57735f, -0.57735f, 0.57735f });
	m_mainLighting->AddDirectionalLight({ 0.5f, 0.5f, 0.5f }, { -0.57735f, -0.57735f, 0.57735f });
	m_mainLighting->AddDirectionalLight({ 0.2f, 0.2f, 0.2f }, { 0.0f, -0.707f, -0.707f });

	m_simulation.AddAtom(AtomType::HYDROGEN, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f));
	m_simulation.AddAtom(AtomType::HELIUM, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 0.0f));
	m_simulation.AddAtom(AtomType::LITHIUM, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f));
	m_simulation.AddAtom(AtomType::BERYLLIUM, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 1.0f));
	m_simulation.AddAtom(AtomType::BORON, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_simulation.AddAtom(AtomType::CARBON, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f));
	m_simulation.AddAtom(AtomType::NITROGEN, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f));
	m_simulation.AddAtom(AtomType::OXYGEN, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f));
	m_simulation.AddAtom(AtomType::FLOURINE, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 1.0f, 0.0f));
	m_simulation.AddAtom(AtomType::NEON, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, -1.0f));





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
	m_mainSimulationWindow = std::make_unique<SimulationWindow>(*this, m_deviceResources, m_simulation, m_materials, *(m_mainLighting.get()), 0.0f, 0.0f, m_mainWindow->GetHeight(), m_mainWindow->GetWidth());



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

	float baseFontSize = 18.0f;
	float iconFontSize = baseFontSize * 2.0f / 3.0f;

	ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", baseFontSize);
	ASSERT(font != nullptr, "Could not find font");

	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	icons_config.GlyphMinAdvanceX = iconFontSize;
	icons_config.GlyphOffset = ImVec2(0, 3); // The glyphs tend to be too high, so bring them down 3 pixels

	static const ImWchar icon_ranges[] = { 0xE700, 0xF8B3, 0 };
	font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segmdl2.ttf", 18.0f, &icons_config, icon_ranges);
	ASSERT(font != nullptr, "Could not find font");
}

void Application::InitializeMaterials() noexcept
{
	bool success = false;
	m_materials.clear();

	try
	{
		std::ifstream f("src/application/config/materials.json");
		json data = json::parse(f);

		m_materials.resize(data.size());

		for (auto& [key, value] : data.items())
		{
			auto itr = std::find(AtomNames.begin(), AtomNames.end(), key);

			if (itr == AtomNames.end())
				throw std::runtime_error("Atom name '" + key + "' is not recognized");

			unsigned int materialIndex = static_cast<unsigned int>(itr - AtomNames.begin());

			ASSERT(materialIndex < m_materials.size(), "Something went wrong - index should not be greater than the size here");
			
			for (auto& [key2, value2] : value.items())
			{
				if (key2 == "DiffuseAlbedo")
				{
					if (!value2.is_array())
						throw std::runtime_error("Diffuse Albedo for '" + key + "' must be an array of either 3 or 4 values");

					if (value2.size() != 3 && value2.size() != 4)
						throw std::runtime_error("Diffuse Albedo for '" + key + "' must be an array of either 3 or 4 values");

					float lastValue = value2.size() == 3 ? 1.0f : value2[3].get<float>();
					m_materials[materialIndex].DiffuseAlbedo = { value2[0].get<float>(), value2[1].get<float>(), value2[2].get<float>(), lastValue };
				}
				else if (key2 == "FresnelR0")
				{
					if (!value2.is_array())
						throw std::runtime_error("FresnelR0 for '" + key + "' must be an array of 3 floats");

					if (value2.size() != 3)
						throw std::runtime_error("FresnelR0 for '" + key + "' must be an array of 3 floats");

					m_materials[materialIndex].FresnelR0 = { value2[0].get<float>(), value2[1].get<float>(), value2[2].get<float>() };
				}
				else if (key2 == "Roughness")
				{
					if (!value2.is_number_float())
						throw std::runtime_error("Roughness for '" + key + "' must be a float value");

					m_materials[materialIndex].Roughness = value2.get<float>();
				}
				else
				{
					throw std::runtime_error("Material component '" + key2 + "' is not recognized");
				}
			}
		}

		success = true;
	}
	catch (const json::exception& e)
	{
		LOG_ERROR("Application::InitializeMaterials: {}", "Failed to load materials from file: src/application/config/materials.json");
		LOG_ERROR("Error Message: {}", e.what());
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Application::InitializeMaterials: {}", "Failed to load materials from file: src/application/config/materials.json");
		LOG_ERROR("Error Message: {}", e.what());
	}

	// If unsuccessful, just load some default colors
	if (!success)
	{
		m_materials.clear();
		m_materials.push_back({ XMFLOAT4(Colors::ForestGreen), { 0.02f, 0.02f, 0.02f }, 0.1f });
		m_materials.push_back({ XMFLOAT4(Colors::AliceBlue), { 0.02f, 0.02f, 0.02f }, 0.1f });
		m_materials.push_back({ XMFLOAT4(Colors::Aqua), { 0.02f, 0.02f, 0.02f }, 0.1f });
		m_materials.push_back({ XMFLOAT4(Colors::Azure), { 0.02f, 0.02f, 0.02f }, 0.1f });
		m_materials.push_back({ XMFLOAT4(Colors::BlanchedAlmond), { 0.02f, 0.02f, 0.02f }, 0.1f });
		m_materials.push_back({ XMFLOAT4(Colors::Chartreuse), { 0.02f, 0.02f, 0.02f }, 0.1f });
		m_materials.push_back({ XMFLOAT4(Colors::DarkGoldenrod), { 0.02f, 0.02f, 0.02f }, 0.1f });
		m_materials.push_back({ XMFLOAT4(Colors::Firebrick), { 0.02f, 0.02f, 0.02f }, 0.1f });
		m_materials.push_back({ XMFLOAT4(Colors::Moccasin), { 0.02f, 0.02f, 0.02f }, 0.1f });
		m_materials.push_back({ XMFLOAT4(Colors::Thistle), { 0.02f, 0.02f, 0.02f }, 0.1f });
	}

	// Now that we have loaded the atoms' materials, we need to create the additional materials that
	// will be used in the scene
	
	// Box resizing arrow material
	g_arrowMaterialIndex = static_cast<std::uint32_t>(m_materials.size());
	m_materials.push_back({ XMFLOAT4(Colors::Magenta), { 0.01f, 0.01f, 0.01f }, 0.5f });

	// Box Material
	g_boxMaterialIndex = static_cast<std::uint32_t>(m_materials.size());
	m_materials.push_back({ { 1.0f, 1.0f, 1.0f, 1.0f }, {}, 0.0f });

	// Box face material when highlighted
	g_boxFaceWhenHoveredMaterialIndex = static_cast<std::uint32_t>(m_materials.size());
	g_boxFaceWhenClickedMaterialIndex = g_boxFaceWhenHoveredMaterialIndex + 1;
	m_materials.push_back({ { 0.0f, 1.0f, 0.0f, 0.5f }, {}, 0.0f });
	m_materials.push_back({ { 0.0f, 1.0f, 0.0f, 0.3f }, {}, 0.0f });

	// Selected Atom Outline Material
	g_selectedAtomOutlineMaterialIndex = static_cast<std::uint32_t>(m_materials.size());
	m_materials.push_back({ { 1.0f, 0.647f, 0.0f, 1.0f }, {}, 0.0f });

	// Solid Axis Material
	g_solidAxisColorMaterialIndex = static_cast<std::uint32_t>(m_materials.size());
	m_materials.push_back({ { 1.0f, 0.0f, 0.0f, 1.0f }, {}, 0.0f });
}
void Application::SaveMaterials() noexcept
{
	json j = {};
	for (unsigned int iii = 0; iii < AtomNames.size(); ++iii)
	{
		Material& m = m_materials[iii];

		j[AtomNames[iii]] = {
			{ "DiffuseAlbedo", { m.DiffuseAlbedo.x, m.DiffuseAlbedo.y, m.DiffuseAlbedo.z, m.DiffuseAlbedo.w } },
			{ "FresnelR0", { m.FresnelR0.x, m.FresnelR0.y, m.FresnelR0.z } },
			{ "Roughness", m.Roughness }
		};
	}

	std::ofstream materialsFile; 
	materialsFile.open("src/application/config/materials.json");
	materialsFile << j.dump(4) << '\n';
	materialsFile.close();
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


	// If the simulation is only running for a fixed amount of time, update the time counter
	if (m_simulationSettings.playState == SimulationSettings::PlayState::PLAYING_FOR_FIXED_TIME)
	{
		m_simulationSettings.accumulatedFixedTime += m_timer.DeltaTime();
		if (m_simulationSettings.accumulatedFixedTime > m_simulationSettings.fixedTimePlayDuration)
		{
			m_simulationSettings.playState = SimulationSettings::PlayState::PAUSED;
			m_simulationSettings.accumulatedFixedTime = 0.0f;
			m_simulation.StopPlaying();
		}
	}


	m_simulation.Update(m_timer);
	m_mainSimulationWindow->Update(m_timer, m_currentFrameIndex);
}
void Application::RenderUI()
{
	ImGuiIO& io = ImGui::GetIO();

	static unsigned int fps = 0;
	static unsigned int frameCounter = 0;
	static float startTime = m_timer.TotalTime();

	++frameCounter;
	float currentTime = m_timer.TotalTime();
	if (currentTime - startTime > 0.5f)
	{
		startTime = currentTime;
		fps = frameCounter * 2;
		frameCounter = 0;
	}

	static bool opt_fullscreen = true;
	static bool opt_padding = false;

	static bool editAtoms = true;
	static bool editMaterials = false;
	static bool editLighting = false;
	static bool editSimulationSettings = false;

	static bool allowMouseToResizeBoxDimensions = false;
	static bool allowMouseToMoveAtoms = false;

	static const char* elementNames = "Hydrogen\0Helium\0Lithium\0Beryllium\0Boron\0Carbon\0Nitrogen\0Oxygen\0Flourine\0Neon\0\0";

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
				ImGui::MenuItem("Atoms", NULL, &editAtoms);
				ImGui::MenuItem("Materials", NULL, &editMaterials);
				ImGui::MenuItem("Lighting", NULL, &editLighting);
				ImGui::MenuItem("Simulation Settings", NULL, &editSimulationSettings);
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar(); 
		}

		ImGui::End();
	}

	// Top Panel
	{
		//ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		//window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove; 
		//window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		//window_flags |= ImGuiWindowFlags_NoNavFocus;
		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_NoCollapse;
		window_flags |= ImGuiWindowFlags_NoTitleBar;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));

		ImGui::Begin("Top Panel", nullptr, window_flags);
		float width = ImGui::GetWindowWidth();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 3));
		
		ImGui::Indent();
		// Disable undo/redo buttons when the simulation is playing
		if (m_simulationSettings.playState != SimulationSettings::PlayState::PAUSED)
		{
			ImGui::BeginDisabled(true);
			ImGui::Button(ICON_UNDO);
			ImGui::SameLine();
			ImGui::Button(ICON_REDO);
			ImGui::EndDisabled();
		}
		else
		{
			if (m_undoStack.size() == 0)
			{
				ImGui::BeginDisabled(true);
				ImGui::Button(ICON_UNDO);
				ImGui::EndDisabled();
			}
			else if (ImGui::Button(ICON_UNDO))
			{
				m_redoStack.push(m_undoStack.top());
				m_undoStack.top()->Undo(this);
				m_undoStack.pop();
			}
			ImGui::SetItemTooltip("Undo");
			ImGui::SameLine();
			if (m_redoStack.size() == 0) 
			{
				ImGui::BeginDisabled(true); 
				ImGui::Button(ICON_REDO);
				ImGui::EndDisabled(); 
			}
			else if (ImGui::Button(ICON_REDO)) 
			{
				m_undoStack.push(m_redoStack.top()); 
				m_redoStack.top()->Redo(this);
				m_redoStack.pop();
			}
			ImGui::SetItemTooltip("Redo"); 
		}

		ImGui::SameLine();

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const ImVec2 p = ImGui::GetCursorScreenPos();
		float x = p.x + 10.0f;
		float y = p.y;
		float sz = 24.0f;
		float thickness = 2.0f;
		const ImU32 col = ImColor(1.0f, 0.0f, 0.0f);
		draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + sz), col, thickness);

		ImGui::SameLine(x + 12.0f);

		if (allowMouseToResizeBoxDimensions)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.9f, 0.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
		}

		if (ImGui::Button(ICON_BOX_EDIT))
		{
			if (allowMouseToMoveAtoms)
			{
				allowMouseToMoveAtoms = false;
				m_mainSimulationWindow->EndSelectionMovement();
			}
			allowMouseToResizeBoxDimensions = !allowMouseToResizeBoxDimensions;
			m_simulationSettings.mouseState = allowMouseToResizeBoxDimensions ? SimulationSettings::MouseState::RESIZING_BOX : SimulationSettings::MouseState::NONE;
			m_mainSimulationWindow->SetAllowMouseToResizeBoxDimensions(allowMouseToResizeBoxDimensions);
		}
		ImGui::SetItemTooltip("Allow Mouse to Resize Simulation Box");
		ImGui::PopStyleColor(3);



		if (allowMouseToMoveAtoms)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.9f, 0.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_SPHERES))
		{
			allowMouseToResizeBoxDimensions = false; 
			m_mainSimulationWindow->SetAllowMouseToResizeBoxDimensions(false);

			allowMouseToMoveAtoms = !allowMouseToMoveAtoms;
			if (allowMouseToMoveAtoms)
			{
				m_simulationSettings.mouseState = SimulationSettings::MouseState::MOVING_ATOMS;
				m_mainSimulationWindow->StartSelectionMovement();
			}
			else
			{
				m_simulationSettings.mouseState = SimulationSettings::MouseState::NONE;
				m_mainSimulationWindow->EndSelectionMovement();
			}			
		}
		ImGui::SetItemTooltip("Allow Mouse to Move Atoms");
		ImGui::PopStyleColor(3);


		
		ImGui::SameLine();
		ImGui::Button("Simulation");
		ImGui::SameLine();
		ImGui::Button("Mode");
		ImGui::SameLine();
		ImGui::Button("Buttons");
		ImGui::SameLine();






		ImGui::SameLine(width / 2);

		switch (m_simulationSettings.playState)
		{
		case SimulationSettings::PlayState::PAUSED:
		{
			// Play Button
			if (ImGui::Button(ICON_PLAY_SOLID)) 
			{
				AddUndoCR<SimulationPlayCR>(m_simulation.GetAtoms());
				m_simulationSettings.playState = SimulationSettings::PlayState::PLAYING;
				m_simulation.StartPlaying();
			}
			else
				ImGui::SetItemTooltip("Play");

			// Play while L button is down
			ImGui::SameLine(); 
			ImGui::Button(ICON_PLAY_WHILE_CLICKED);
			if (ImGui::IsItemActive()) // IsItemActive is true when mouse LButton is being held down 
			{
				AddUndoCR<SimulationPlayCR>(m_simulation.GetAtoms());
				m_simulationSettings.playState = SimulationSettings::PlayState::PLAYING_WHILE_LBUTTON_DOWN;
				m_simulation.StartPlaying();
			}
			else
				ImGui::SetItemTooltip("Play the simulation only while this button is being clicked"); 

			// Play for set amount of time button
			ImGui::SameLine(); 
			if (ImGui::Button(ICON_PLAY ICON_STOPWATCH))
			{
				AddUndoCR<SimulationPlayCR>(m_simulation.GetAtoms());
				m_simulationSettings.playState = SimulationSettings::PlayState::PLAYING_FOR_FIXED_TIME;
				m_simulation.StartPlaying();
			}
			else
				ImGui::SetItemTooltip("Play the simulation fixed amount of time. \nSee Edit > Simulation Settings to adjust the duration");

			break;
		}

		case SimulationSettings::PlayState::PLAYING:
		case SimulationSettings::PlayState::PLAYING_FOR_FIXED_TIME:
		{
			// Pause Button
			if (ImGui::Button(ICON_PAUSE))
			{
				m_simulationSettings.playState = SimulationSettings::PlayState::PAUSED;
				m_simulationSettings.accumulatedFixedTime = 0.0f;
				m_simulation.StopPlaying();
			}
			else
				ImGui::SetItemTooltip("Pause");

			break;
		}

		case SimulationSettings::PlayState::PLAYING_WHILE_LBUTTON_DOWN:
		{
			ImGui::BeginDisabled(true);
			ImGui::Button(ICON_PLAY_SOLID);
			ImGui::EndDisabled();

			// Play while L button is down
			ImGui::SameLine();
			ImGui::Button(ICON_PLAY_WHILE_CLICKED);
			if (!ImGui::IsItemActive()) // IsItemActive is true when mouse LButton is being held down 
			{
				// Pause the simulation once we release the LButton
				m_simulationSettings.playState = SimulationSettings::PlayState::PAUSED;
				m_simulationSettings.accumulatedFixedTime = 0.0f;
				m_simulation.StopPlaying();
			}

			ImGui::BeginDisabled(true);
			ImGui::SameLine();
			ImGui::Button(ICON_PLAY ICON_STOPWATCH);
			ImGui::EndDisabled();

			break;
		}			
		}



		ImGui::PopStyleVar(); // ImGuiStyleVar_FramePadding
		ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing

		ImGui::End();

		ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding
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

		ImGui::Begin("Add", nullptr, window_flags);

		if (ImGui::CollapsingHeader("Add Atom", ImGuiTreeNodeFlags_None))
		{
			static int elementIndex = 0;
			ImGui::Combo("##AddAtomElementCombo", &elementIndex, elementNames);

			static XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
			static XMFLOAT3 vel = { 0.0f, 0.0f, 0.0f };

			AtomType type = static_cast<AtomType>(elementIndex + 1);
			XMFLOAT3 boxDims = m_simulation.GetDimensionMaxs();
			float radius = Atom::RadiusOf(type);

			ImGui::Spacing();

			ImGui::Indent();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Position  X");
			ImGui::SameLine(100.0f);
			ImGui::DragFloat("##addAtomPositionX", &pos.x, 0.2f, -boxDims.x + radius, boxDims.x - radius);
			ImGui::Unindent();

			ImGui::Indent(77.0f);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Y");
			ImGui::SameLine(100.0f);
			ImGui::DragFloat("##addAtomPositionY", &pos.y, 0.2f, -boxDims.y + radius, boxDims.y - radius);
			ImGui::Unindent(77.0f);

			ImGui::Indent(76.0f);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Z");
			ImGui::SameLine();
			ImGui::DragFloat("##addAtomPositionZ", &pos.z, 0.2f, -boxDims.z + radius, boxDims.z - radius);
			ImGui::Unindent(76.0f);

			ImGui::Spacing();

			ImGui::Indent();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Velocity  X");
			ImGui::SameLine(100.0f);
			ImGui::DragFloat("##addAtomVelocityX", &vel.x, 0.5f, -10.0, 10.0);
			ImGui::Unindent();

			ImGui::Indent(77.0f);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Y");
			ImGui::SameLine(100.0f);
			ImGui::DragFloat("##addAtomVelocityY", &vel.y, 0.5f, -10.0, 10.0);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Z");
			ImGui::SameLine(100.0f);
			ImGui::DragFloat("##addAtomVelocityZ", &vel.z, 0.5f, -10.0, 10.0);
			ImGui::Unindent(77.0f);

			ImGui::Spacing();

			ImGui::Indent(100.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
			if (ImGui::Button(ICON_ADD " Add Atom##AddAtomButton"))
			{
				// Create the new atom (this will also create the change request)
				const Atom& atom = AddAtom(type, pos, vel);

				// Make the atom the only selected atom
				m_simulation.SelectAtom(atom, true);
			}
			ImGui::PopStyleColor();
			ImGui::Unindent(100.0f);

			ImGui::Spacing();
			ImGui::Spacing();
		}
		if (ImGui::CollapsingHeader("Add Molecule", ImGuiTreeNodeFlags_None))
		{
			ImGui::Text("Add Molecule...");
		}
		if (ImGui::CollapsingHeader("Add From File", ImGuiTreeNodeFlags_None))
		{
			ImGui::Text("Add From File...");
		}
		if (ImGui::CollapsingHeader("Download PDB File", ImGuiTreeNodeFlags_None))
		{
			ImGui::Text("Download PDB File...");
		}

		ImGui::End();
	}

	// Right Panel
	{
		if (editAtoms)
		{
			ImGui::Begin("Atoms");

			std::vector<Atom>& atoms = m_simulation.GetAtoms();
			XMFLOAT3 boxDims = m_simulation.GetDimensionMaxs();
			const std::vector<size_t>& selectedAtomIndices = m_simulation.GetSelectedAtomIndices();

			ImGuiTableFlags tableFlags =
				ImGuiTableFlags_Resizable |						// Ability to drag vertical bar between columns to resize them
				//ImGuiTableFlags_Reorderable |					// Ability to drag column headers to reorder them
				//ImGuiTableFlags_Hideable |					// Ability to right click on the header row and toggle columns on/off
				ImGuiTableFlags_Sortable |						// Ability to click a column header to sort the entire table on that column
				//ImGuiTableFlags_SortMulti |					// Ability to hold SHIFT to sort on multiple columns
				//ImGuiTableFlags_RowBg |						// Alternate row colors
				ImGuiTableFlags_Borders |						// ???
				ImGuiTableFlags_BordersV |						// Include vertical borders on left and right side of the table
				ImGuiTableFlags_BordersInnerV |					// Include vertical borders between columns
				ImGuiTableFlags_BordersOuterV |					// Include vertical borders on left and right side of the table
				ImGuiTableFlags_BordersH |						// Include horizontal borders on top and bottom of the table
				ImGuiTableFlags_BordersInnerH |					// Include horizontal borders between rows
				ImGuiTableFlags_BordersOuterH |					// Include horizontal borders on top and bottom of the table
				// ImGuiTableFlags_NoBordersInBody |			// Omit vertical borders in the table body
				ImGuiTableFlags_NoBordersInBodyUntilResize |	// Omit vertical borders until hovering to resize column width
				//ImGuiTableFlags_ScrollX | 
				ImGuiTableFlags_ScrollY | 
				ImGuiTableFlags_SizingFixedFit
				//ImGuiTableFlags_NoHostExtendX |
				//ImGuiTableFlags_NoHostExtendY
				;

			
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(5.0f, 0.0f));

			// Force the table height to be half of the available window height
			if (ImGui::BeginTable("atoms_table", 4, tableFlags, ImVec2(0.0f, ImGui::GetWindowHeight() / 2.0f)))
			{
				ImGui::TableSetupColumn("ID");
				ImGui::TableSetupColumn("Atom Type");
				ImGui::TableSetupColumn("Position");
				ImGui::TableSetupColumn("Velocity", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupScrollFreeze(1, 1); // Freeze the header row and first column when scrolling

				ImGui::TableHeadersRow(); 


				ImGuiListClipper clipper; 
				clipper.Begin(static_cast<int>(atoms.size()));
				while (clipper.Step())
				{
					for (size_t row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) 
					{
						Atom& atom = atoms[row_n];

						ImGui::TableNextRow(ImGuiTableRowFlags_None);

						ImGui::TableSetColumnIndex(0);
						ImGui::AlignTextToFramePadding();
						bool itemIsSelected = m_simulation.AtomIsSelected(row_n);
						if (ImGui::Selectable(std::format(" {}", row_n).c_str(), itemIsSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
						{
							if (ImGui::GetIO().KeyCtrl) 
							{
								if (itemIsSelected) 
									m_simulation.UnselectAtom(row_n);
								else
									m_simulation.SelectAtom(row_n);
							}
							else
							{
								m_simulation.SelectAtom(row_n, true);
							}
						}

						ImGui::TableSetColumnIndex(1); 
						ImGui::Text("%s", AtomNames[static_cast<size_t>(atom.type) - 1]); 


						ImGui::TableSetColumnIndex(2);
						ImGui::Text("%.2f, %.2f, %.2f", atom.position.x, atom.position.y, atom.position.z);

						ImGui::TableSetColumnIndex(3);
						ImGui::Text("%.2f, %.2f, %.2f", atom.velocity.x, atom.velocity.y, atom.velocity.z);
					}
				}

				ImGui::EndTable();				
			}
			ImGui::PopStyleVar();





			if (selectedAtomIndices.size() == 1)
			{
				static bool positionXSliderIsActive = false;
				static bool positionYSliderIsActive = false;
				static bool positionZSliderIsActive = false;
				static bool velocityXSliderIsActive = false;
				static bool velocityYSliderIsActive = false;
				static bool velocityZSliderIsActive = false;

				Atom& atom = atoms[selectedAtomIndices[0]];

				XMFLOAT3 initialPosition = atom.position;
				XMFLOAT3 initialVelocity = atom.velocity;

				static auto CheckVelocitySlider = [this](const Atom& atom, const XMFLOAT3& initialVelocity, bool& sliderActive)
					{
						if (ImGui::IsItemActive())
						{
							if (!sliderActive)
							{
								sliderActive = true;
								AddUndoCR<AtomVelocityCR>(initialVelocity, atom.velocity, m_simulation.IndexOf(atom));
							}
						}
						else if (sliderActive) 
						{
							sliderActive = false;
							AtomVelocityCR* cr = static_cast<AtomVelocityCR*>(m_undoStack.top().get());
							cr->m_velocityFinal = atom.velocity;
						}
					};
				static auto CheckPositionSlider = [this](const Atom& atom, const XMFLOAT3& initialPosition, bool& sliderActive, MovementDirection direction)
					{
						if (ImGui::IsItemActive()) 
						{
							if (!sliderActive) 
							{
								sliderActive = true; 
								AddUndoCR<AtomsMovedCR>(m_simulation.IndexOf(atom), initialPosition, atom.position);
								m_mainSimulationWindow->StartSelectionMovement(direction);
							}

							// Have to call this because we are manually moving a selected atom
							m_simulation.UpdateSelectedAtomsCenter();
						}
						else if (sliderActive) 
						{
							sliderActive = false; 
							AtomsMovedCR* cr = static_cast<AtomsMovedCR*>(m_undoStack.top().get());
							cr->m_positionFinal = atom.position;  

							if (!(m_simulationSettings.mouseState == SimulationSettings::MouseState::MOVING_ATOMS))
								m_mainSimulationWindow->EndSelectionMovement();
						}
					};

				ImGui::Spacing();
				ImGui::Text("Atom:");
				ImGui::SameLine();
				ImGui::Text("%d - %s", m_simulation.IndexOf(atom), AtomNames[static_cast<size_t>(atom.type) - 1]);

				ImGui::Spacing();
				ImGui::Indent();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Position  X");
				ImGui::SameLine(100.0f);
				ImGui::DragFloat("##atomPositionX", &atom.position.x, 0.2f, -boxDims.x + atom.radius, boxDims.x - atom.radius);
				CheckPositionSlider(atom, initialPosition, positionXSliderIsActive, MovementDirection::X);
				ImGui::Unindent();

				ImGui::Indent(77.0f);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Y");
				ImGui::SameLine(100.0f);
				ImGui::DragFloat("##atomPositionY", &atom.position.y, 0.2f, -boxDims.y + atom.radius, boxDims.y - atom.radius);
				CheckPositionSlider(atom, initialPosition, positionYSliderIsActive, MovementDirection::Y);
				ImGui::Unindent(77.0f);

				ImGui::Indent(76.0f);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Z");
				ImGui::SameLine();
				ImGui::DragFloat("##atomPositionZ", &atom.position.z, 0.2f, -boxDims.z + atom.radius, boxDims.z - atom.radius);
				CheckPositionSlider(atom, initialPosition, positionZSliderIsActive, MovementDirection::Z);
				ImGui::Unindent(76.0f);

				ImGui::Spacing();

				ImGui::Indent();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Velocity  X");
				ImGui::SameLine(100.0f);
				ImGui::DragFloat("##atomVelocityX", &atom.velocity.x, 0.5f, -10.0, 10.0);
				CheckVelocitySlider(atom, initialVelocity, velocityXSliderIsActive);
				ImGui::Unindent();

				ImGui::Indent(77.0f);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Y");
				ImGui::SameLine(100.0f);
				ImGui::DragFloat("##atomVelocityY", &atom.velocity.y, 0.5f, -10.0, 10.0);
				CheckVelocitySlider(atom, initialVelocity, velocityYSliderIsActive);

				ImGui::AlignTextToFramePadding();
				ImGui::Text("Z");
				ImGui::SameLine(100.0f);
				ImGui::DragFloat("##atomVelocityZ", &atom.velocity.z, 0.5f, -10.0, 10.0);
				CheckVelocitySlider(atom, initialVelocity, velocityZSliderIsActive);
				ImGui::Unindent(77.0f);

				
			}
			else if (selectedAtomIndices.size() > 1)
			{
				static bool positionXSliderIsActive = false;
				static bool positionYSliderIsActive = false;
				static bool positionZSliderIsActive = false;

				XMFLOAT3 dims = m_simulation.GetDimensionMaxs();
				XMFLOAT3 centerInitial = m_simulation.GetSelectedAtomsCenter();
				XMFLOAT3 center = centerInitial;
				XMFLOAT3 max = m_simulation.GetSelectedAtomsMaxBounds(); 
				XMFLOAT3 min = m_simulation.GetSelectedAtomsMinBounds(); 
				const float maxX = dims.z - (max.x - centerInitial.x);
				const float minX = -dims.z - (min.x - centerInitial.x);
				const float maxY = max.y - centerInitial.y;
				const float minY = min.y + centerInitial.y;
				const float maxZ = max.z - centerInitial.z;
				const float minZ = min.z + centerInitial.z;

				static auto CheckPositionSlider = [this](float delta, bool& sliderActive, MovementDirection direction)
					{
						if (ImGui::IsItemActive())
						{
							LOG_TRACE("Delta: {}", delta);

							if (!sliderActive)
							{
								sliderActive = true;

								DirectX::XMFLOAT3 center = m_simulation.GetSelectedAtomsCenter();
								DirectX::XMFLOAT3 centerInitial = center;
								switch (direction)
								{
								case MovementDirection::X: centerInitial.x -= delta; break;
								case MovementDirection::Y: centerInitial.y -= delta; break;
								case MovementDirection::Z: centerInitial.z -= delta; break;
								}
								AddUndoCR<AtomsMovedCR>(m_simulation.GetSelectedAtomIndices(), centerInitial, center);

								m_mainSimulationWindow->StartSelectionMovement(direction);
							}

							if (delta != 0.0f)
							{
								switch (direction)
								{
								case MovementDirection::X: m_simulation.MoveSelectedAtomsX(delta); break;
								case MovementDirection::Y: m_simulation.MoveSelectedAtomsY(delta); break;
								case MovementDirection::Z: m_simulation.MoveSelectedAtomsZ(delta); break;
								}
							}
						}
						else if (sliderActive)
						{
							sliderActive = false;

							AtomsMovedCR* cr = static_cast<AtomsMovedCR*>(m_undoStack.top().get()); 
							cr->m_positionFinal = m_simulation.GetSelectedAtomsCenter();

							if (!(m_simulationSettings.mouseState == SimulationSettings::MouseState::MOVING_ATOMS))
								m_mainSimulationWindow->EndSelectionMovement();
						}
					};

				ImGui::Spacing();
				ImGui::Text("Number of Atoms Selected: %d", selectedAtomIndices.size());

				ImGui::Spacing();
				ImGui::Indent();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Position  X");
				ImGui::SameLine(100.0f);
				ImGui::DragFloat("##selectedAtomsPositionX", &center.x, 0.2f, minX, maxX);
				CheckPositionSlider(center.x - centerInitial.x, positionXSliderIsActive, MovementDirection::X);
				ImGui::Unindent();

				ImGui::Indent(77.0f);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Y");
				ImGui::SameLine(100.0f);
				ImGui::DragFloat("##selectedAtomsPositionY", &center.y, 0.2f, minY, maxY);
				CheckPositionSlider(center.y - centerInitial.y, positionYSliderIsActive, MovementDirection::Y);
				ImGui::Unindent(77.0f);

				ImGui::Indent(76.0f);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Z");
				ImGui::SameLine();
				ImGui::DragFloat("##selectedAtomsPositionZ", &center.z, 0.2f, minZ, maxZ);
				CheckPositionSlider(center.z - centerInitial.z, positionZSliderIsActive, MovementDirection::Z);
				ImGui::Unindent(76.0f);
			}
			else
			{
				ImGui::Text("No atoms selected");
			}






			ImGui::End();
		}

		if (editMaterials)
		{
			ImGui::Begin("Materials");

			static int elementIndex = 0;
			ImGui::Combo("##MaterialEditElementCombo", &elementIndex, elementNames);

			static bool materialDiffuseSliderIsActive = false;
			static bool materialFresnelSliderIsActive = false;
			static bool materialRoughnessSliderIsActive = false;

			static auto CheckMaterialSlider = [this](AtomType atomType, const Material& mat, bool& sliderActive)
				{
					if (ImGui::IsItemActive())
					{
						if (!sliderActive)
						{
							sliderActive = true;
							AddUndoCR<AtomMaterialCR>(mat, mat, atomType);
						}
						InvokeHandlers(m_materialChangedHandlers);
						SaveMaterials();
					}
					else if (sliderActive)
					{
						sliderActive = false;
						AtomMaterialCR* cr = static_cast<AtomMaterialCR*>(m_undoStack.top().get()); 
						cr->m_materialFinal = mat;
					}
				};

			Material& material = m_materials[elementIndex];
			AtomType type = static_cast<AtomType>(elementIndex + 1);

			ImGui::ColorEdit4("Diffuse Albedo", (float*)&material.DiffuseAlbedo, ImGuiColorEditFlags_AlphaPreview);
			CheckMaterialSlider(type, material, materialDiffuseSliderIsActive);

			ImGui::ColorEdit3("FresnelR0", (float*)&material.FresnelR0, ImGuiColorEditFlags_AlphaPreview);
			CheckMaterialSlider(type, material, materialFresnelSliderIsActive);

			ImGui::DragFloat("Roughness", &material.Roughness, 0.005f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			CheckMaterialSlider(type, material, materialRoughnessSliderIsActive);

			ImGui::End();
		}

		if (editLighting)
		{
			ImGui::Begin("Lighting");
			ImGui::Text("We are editing lighting now");
			ImGui::End();
		}

		if (editSimulationSettings)
		{
			ImGui::Begin("Simulation Settings");

			// Play / Pause State
			ImGui::SeparatorText("Play/Pause State");
			ImGui::Text("State: ");
			ImGui::SameLine();
			switch (m_simulationSettings.playState)
			{
			case SimulationSettings::PlayState::PAUSED: ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Paused"); break;
			case SimulationSettings::PlayState::PLAYING: ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Playing"); break;
			case SimulationSettings::PlayState::PLAYING_FOR_FIXED_TIME: ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Playing For Fixed Time"); break;
			case SimulationSettings::PlayState::PLAYING_WHILE_LBUTTON_DOWN: ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Playing While LButton Down"); break;
			}
			ImGui::Spacing();

			// Fixed Time Settings
			ImGui::SeparatorText("Play for Fixed Time Settings");
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Play Duration"); ImGui::SameLine();
			ImGui::DragFloat("##Play Duration", &m_simulationSettings.fixedTimePlayDuration, 0.25f, 0.0f, 60.0f, "%.2f");
			ImGui::Text("Time Remaining: ");
			ImGui::SameLine();
			if (m_simulationSettings.playState == SimulationSettings::PlayState::PLAYING_FOR_FIXED_TIME)
				ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%.2f", m_simulationSettings.fixedTimePlayDuration - m_simulationSettings.accumulatedFixedTime);
			else
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "N/A");
			ImGui::Spacing();


			// Simulation Box
			ImGui::SeparatorText("Simulation Box");


			bool disablingBox = !(m_simulationSettings.mouseState == SimulationSettings::MouseState::NONE ||
								  m_simulationSettings.mouseState == SimulationSettings::MouseState::RESIZING_BOX);
			if (disablingBox)
				ImGui::BeginDisabled();

			if (ImGui::Checkbox("Allow Mouse to Resize Box", &allowMouseToResizeBoxDimensions))
			{
				m_simulationSettings.mouseState = allowMouseToResizeBoxDimensions ? SimulationSettings::MouseState::RESIZING_BOX : SimulationSettings::MouseState::NONE;
				m_mainSimulationWindow->SetAllowMouseToResizeBoxDimensions(allowMouseToResizeBoxDimensions); 
			}
			if (disablingBox)
			{
				ImGui::EndDisabled();
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
					ImGui::SetTooltip("Cannot go into box edit mode because a different mouse mode is active");
			}

			ImGui::Checkbox("Allow Atoms to Relocate When Resizing", &m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions);

			float minSideLength = m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions ? 
				5.0f : 2 * m_simulation.GetMaxAxisAlignedDistanceFromOrigin();
			
			static XMFLOAT3 boxDims = m_simulation.GetDimensions(); 
			XMFLOAT3 initialBoxDims = boxDims;

			if (ImGui::Checkbox("Force Simulation Box Sides To Be Equal", &m_simulationSettings.forceSidesToBeEqual))
			{
				if (m_simulationSettings.forceSidesToBeEqual && (initialBoxDims.x != initialBoxDims.y || initialBoxDims.x != initialBoxDims.z))
				{
					boxDims.y = initialBoxDims.x;
					boxDims.z = initialBoxDims.x;

					// Get the initial atom positions before setting the new dimensions
					std::vector<Atom> atomsInitial = {};
					std::vector<Atom> atomsFinal = {};
					if (m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions) 
						atomsInitial = m_simulation.GetAtoms();

					// Update the simulation's dimensions
					SetBoxDimensions(boxDims, true, m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions); 

					// Get final positions and create the change request
					if (m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions) 
						atomsFinal = m_simulation.GetAtoms();

					AddUndoCR<BoxResizeCR>(initialBoxDims, boxDims, m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions, false, true, atomsInitial, atomsFinal);
				}
			}

			if (m_simulationSettings.forceSidesToBeEqual)
			{
				static bool isActive = false;
				if (ImGui::DragFloat("Side Length", &boxDims.x, 0.5f, minSideLength, 1000.f, "%.1f"))
				{
					boxDims.y = boxDims.x;
					boxDims.z = boxDims.x;

					// Update the simulation's dimensions
					SetBoxDimensions(boxDims, m_simulationSettings.forceSidesToBeEqual, m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions);
				}
				if (ImGui::IsItemActive())
				{
					if (!isActive)
					{
						isActive = true;
						// Create the change request for the undo stack
						std::vector<Atom> atomsInitial = {};
						if (m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions)
							atomsInitial = m_simulation.GetAtoms();
						AddUndoCR<BoxResizeCR>(initialBoxDims, boxDims, m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions, m_simulationSettings.forceSidesToBeEqual, m_simulationSettings.forceSidesToBeEqual, atomsInitial);
					}
				}
				else if (isActive)
				{
					isActive = false;
					// Update the final size in the change request
					BoxResizeCR* cr = static_cast<BoxResizeCR*>(m_undoStack.top().get());
					cr->m_final = boxDims;
					if (m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions)
						cr->m_atomsFinal = m_simulation.GetAtoms();
				}
			}
			else
			{
				static bool isActive = false;
				if (ImGui::DragFloat3("Side Lengths", reinterpret_cast<float*>(&boxDims), 0.5, minSideLength, 1000.f, "%.1f"))
				{
					// Update the simulation's dimensions
					SetBoxDimensions(boxDims, m_simulationSettings.forceSidesToBeEqual, m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions);
				}
				if (ImGui::IsItemActive()) 
				{
					if (!isActive) 
					{
						isActive = true; 
						// Create the change request for the undo stack
						std::vector<Atom> atomsInitial = {};
						if (m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions)
							atomsInitial = m_simulation.GetAtoms();
						AddUndoCR<BoxResizeCR>(initialBoxDims, boxDims, m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions, m_simulationSettings.forceSidesToBeEqual, m_simulationSettings.forceSidesToBeEqual, atomsInitial);
					}
				}
				else if (isActive) 
				{
					isActive = false;
					// Update the final size in the change request
					BoxResizeCR* cr = static_cast<BoxResizeCR*>(m_undoStack.top().get()); 
					cr->m_final = boxDims; 
					if (m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions)
						cr->m_atomsFinal = m_simulation.GetAtoms();
				}
			}


			// Atoms
			ImGui::SeparatorText("Atoms");

			bool disablingAtoms = !(m_simulationSettings.mouseState == SimulationSettings::MouseState::NONE ||
									m_simulationSettings.mouseState == SimulationSettings::MouseState::MOVING_ATOMS);
			if (disablingAtoms)
				ImGui::BeginDisabled();

			if (ImGui::Checkbox("Allow Mouse to Move Atoms", &allowMouseToMoveAtoms))
			{
				if (allowMouseToMoveAtoms)
				{
					m_simulationSettings.mouseState = SimulationSettings::MouseState::MOVING_ATOMS;
					m_mainSimulationWindow->StartSelectionMovement();
				}
				else
				{
					m_simulationSettings.mouseState = SimulationSettings::MouseState::NONE;
					m_mainSimulationWindow->EndSelectionMovement();
				}
			}
			if (disablingAtoms)
			{
				ImGui::EndDisabled();
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
					ImGui::SetTooltip("Cannot allow mouse to move atoms because a different mouse mode is active");
			}

			ImGui::End();
		}
	}

	// Bottom Panel
	{
		ImGui::Begin("Bottom Panel"); 

		ImGui::Text("FPS: %d", fps);

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
		m_mainSimulationWindow->SetWindow(pos.y, pos.x, ImGui::GetWindowHeight(), ImGui::GetWindowWidth());

		ImGui::End();
	}

	// Are you sure you want to delete? - Popup
	{
		if (m_openDeletePopup)
		{
			// Only open the popup if atoms are selected
			if (m_simulation.GetSelectedAtomIndices().size())
				ImGui::OpenPopup("Delete Selected Atoms?");
			m_openDeletePopup = false;
		}

		// Always center this window when appearing
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();  
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));  

		if (ImGui::BeginPopupModal("Delete Selected Atoms?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to delete the selected atoms?");
			if (ImGui::Button("Delete", ImVec2(120, 0)))
			{
				RemoveAllSelectedAtoms();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
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
	m_mainSimulationWindow->Render(m_currentFrameIndex);


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

		// Once we have other windows, you will need to attempt to pass the message to other windows 
		// and see if any handle it
		
//		for (SimulationWindow& window : m_simulationWindows)
//		{
//			if (m_simulationWindowSelected.value() != &window && fn(&window))
//			{
//				m_simulationWindowSelected = &window;
//				return;
//			}
//		}
	}
	else
	{
		// Ideally we would just loop here and check if any windows want to handle the message. However, 
		// we only have a single window, so we can just check that for now

		if (fn(m_mainSimulationWindow.get()))
			m_simulationWindowSelected = m_mainSimulationWindow.get();

//		for (SimulationWindow& window : m_simulationWindows)
//		{
//			if (fn(&window))
//			{
//				m_simulationWindowSelected = &window;
//				return;
//			}
//		}
	}
}


void Application::SetMaterial(AtomType atomType, const Material& material) noexcept
{
	m_materials[static_cast<size_t>(atomType) - 1] = material;
	InvokeHandlers(m_materialChangedHandlers);
	SaveMaterials();
}
void Application::SetBoxDimensions(const DirectX::XMFLOAT3& dims, bool forceSidesToBeEqual, bool allowAtomsToRelocate) noexcept
{
	m_simulation.SetDimensions(dims, allowAtomsToRelocate);
	m_simulationSettings.allowAtomsToRelocateWhenUpdatingBoxDimensions = allowAtomsToRelocate;
	m_simulationSettings.forceSidesToBeEqual = forceSidesToBeEqual;
}

void Application::RemoveAllSelectedAtoms() noexcept
{
	// First, create the CR
	const std::vector<size_t>& indices = m_simulation.GetSelectedAtomIndices();
	std::vector<std::tuple<size_t, AtomTPV>> data; 
	data.reserve(indices.size());
	for (size_t index : indices)
	{
		const Atom& atom = m_simulation.GetAtom(index); 
		data.emplace_back(index, AtomTPV(atom.type, atom.position, atom.velocity)); 
	}
	AddUndoCR<RemoveAtomsCR>(std::move(data));

	m_simulation.RemoveAllSelectedAtoms();
}
const Atom& Application::AddAtom(AtomType type, const XMFLOAT3& position, const XMFLOAT3& velocity, bool createCR) noexcept
{
	const Atom& atom = m_simulation.AddAtom(type, position, velocity);

	if (createCR)
		AddUndoCR<AddAtomsCR>(AtomTPV(type, position, velocity));

	return atom;
}
std::vector<Atom*> Application::AddAtoms(const std::vector<AtomTPV>& atomData, bool createCR) noexcept
{
	std::vector<Atom*> atoms = m_simulation.AddAtoms(atomData);

	if (createCR)
		AddUndoCR<AddAtomsCR>(atomData); 

	return atoms;
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

	if (keycode == VK_DELETE)
	{
		m_openDeletePopup = true;
		return 0;
	}

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