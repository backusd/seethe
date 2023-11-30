#pragma once
#include "pch.h"
#include "window/MainWindow.h"
#include "simulation/Simulation.h"
#include "rendering/DeviceResources.h"
#include "rendering/DescriptorVector.h"
#include "rendering/Renderer.h"
#include "utils/Timer.h"
#include "utils/Constants.h"
#include "ui/SimulationWindow.h"
#include "application/rendering/Material.h"
#include "application/change-requests/ChangeRequest.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function on Simulation
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
struct SimulationSettings
{
	enum class PlayState
	{
		PAUSED,
		PLAYING,
		PLAYING_WHILE_LBUTTON_DOWN,
		PLAYING_FOR_FIXED_TIME
	};

	// State of Play / Pause
	PlayState playState = PlayState::PAUSED;

	// Fixed Time Settings
	float fixedTimePlayDuration = 5.0f;
	float accumulatedFixedTime = 0.0f;

	// Box Settings
//	DirectX::XMFLOAT3 boxDimensions = { 20.0f, 20.0f, 20.0f };
	bool allowAtomsToRelocateWhenUpdatingBoxDimensions = false;
	bool forceSidesToBeEqual = true;
	bool allowMouseToResizeBoxDimensions = false;

	// Atom details
	std::vector<unsigned int> selectedAtomUUIDs;
};

class Application
{
public:
	using EventHandler = std::function<void()>;
	using EventHandlers = std::vector<EventHandler>;

public:
	Application();
	Application(const Application&) = delete;
	Application(Application&&) = delete;
	Application& operator=(const Application&) = delete;
	Application& operator=(Application&&) = delete;
	~Application();

	int Run();

	ND LRESULT MainWindowOnCreate(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnClose(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnLButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnLButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnLButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnRButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnMButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnMButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnMButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnRButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnRButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnX1ButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnX2ButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnX1ButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnX2ButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnX1ButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnX2ButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnResize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnMouseMove(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnMouseEnter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnMouseLeave(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnMouseWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnMouseHWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnChar(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnKeyUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnKeyDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnKillFocus(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	ND inline Simulation& GetSimulation() noexcept { return m_simulation; }
	ND inline SimulationSettings& GetSimulationSettings() noexcept { return m_simulationSettings; }

	template<typename T, typename... Args>
	void AddUndoCR(Args&&... args) noexcept
	{
		std::shared_ptr<ChangeRequest> cr = std::make_shared<T>(std::forward<Args>(args)...);
		m_undoStack.push(cr);
		// Clear the Redo Stack
		while (m_redoStack.size() > 0)
			m_redoStack.pop();
	}

	void SetMaterial(AtomType atomType, const Material& material) noexcept;
	void SetBoxDimensions(const DirectX::XMFLOAT3& dims, bool forceSidesToBeEqual, bool allowAtomsToRelocate) noexcept;

	void RemoveAtomByUUID(size_t uuid) noexcept;
	const Atom& AddAtom(AtomType type, const DirectX::XMFLOAT3& position = { 0.0f, 0.0f, 0.0f }, const DirectX::XMFLOAT3& velocity = { 0.0f, 0.0f, 0.0f }) noexcept;
	void SelectAtomByUUID(size_t uuid, bool unselectAllOthersFirst = false) noexcept;
	void SelectAtomByIndex(size_t index, bool unselectAllOthersFirst = false) noexcept;
	void UnselectAtomByUUID(size_t uuid) noexcept;
	void UnselectAtomByIndex(size_t index) noexcept;

	// Handlers
	inline void RegisterMaterialChangedHandler(const EventHandler& handler) noexcept { m_materialChangedHandlers.push_back(handler); }
	inline void RegisterMaterialChangedHandler(EventHandler&& handler) noexcept { m_materialChangedHandlers.push_back(handler); }
	inline void RegisterBoxSizeChangedHandler(const EventHandler& handler) noexcept { m_boxSizeChangedHandlers.push_back(handler); }
	inline void RegisterBoxSizeChangedHandler(EventHandler&& handler) noexcept { m_boxSizeChangedHandlers.push_back(handler); }
	inline void RegisterSelectedAtomsChangedHandler(const EventHandler& handler) noexcept { m_selectedAtomsChangedHandlers.push_back(handler); }
	inline void RegisterSelectedAtomsChangedHandler(EventHandler&& handler) noexcept { m_selectedAtomsChangedHandlers.push_back(handler); }
	inline void RegisterAtomsAddedHandler(const EventHandler& handler) noexcept { m_atomsAddedHandlers.push_back(handler); }
	inline void RegisterAtomsAddedHandler(EventHandler&& handler) noexcept { m_atomsAddedHandlers.push_back(handler); }
	inline void RegisterAtomsRemovedHandler(const EventHandler& handler) noexcept { m_atomsRemovedHandlers.push_back(handler); }
	inline void RegisterAtomsRemovedHandler(EventHandler&& handler) noexcept { m_atomsRemovedHandlers.push_back(handler); }

private:
	void InitializeMaterials() noexcept;
	void SaveMaterials() noexcept;

	void Update();
	void Render();
	void RenderUI();
	void Present();

	void ForwardMessageToWindows(std::function<bool(SimulationWindow*)>&& fn);
	inline void InvokeHandlers(const EventHandlers& handlers) const noexcept
	{
		for (auto& handler : handlers)
			handler();
	}


	std::unique_ptr<MainWindow> m_mainWindow;
	std::shared_ptr<DeviceResources> m_deviceResources;
	Timer m_timer;
	Simulation m_simulation;

	std::vector<SimulationWindow> m_simulationWindows;
	std::optional<SimulationWindow*> m_simulationWindowSelected = std::nullopt;

	// EventHandlers
	EventHandlers m_materialChangedHandlers;
	EventHandlers m_boxSizeChangedHandlers;
	EventHandlers m_selectedAtomsChangedHandlers;
	EventHandlers m_atomsAddedHandlers;
	EventHandlers m_atomsRemovedHandlers;

	// Rendering resources
	std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, g_numFrameResources> m_allocators;
	int m_currentFrameIndex = 0;
	std::array<UINT64, g_numFrameResources> m_fences = {};	
	std::unique_ptr<DescriptorVector> m_descriptorVector = nullptr;

	std::vector<Material> m_materials = {};

	SimulationSettings m_simulationSettings;

	std::stack<std::shared_ptr<ChangeRequest>> m_undoStack;
	std::stack<std::shared_ptr<ChangeRequest>> m_redoStack;


};
}


#pragma pop_macro("AddAtom") // See note above for 'AddAtom'