#pragma once
#include "pch.h"
#include "window/MainWindow.h"
#include "simulation/Simulation.h"
#include "rendering/DeviceResources.h"
#include "rendering/DescriptorVector.h"
#include "rendering/Renderer.h"
#include "utils/Timer.h"
#include "utils/Constants.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

namespace seethe
{
class Application
{
public:
	Application();
	Application(const Application&) = delete;
	Application(Application&&) = delete;
	Application& operator=(const Application&) = delete;
	Application& operator=(Application&&) = delete;
	~Application();

	int Run();

	ND LRESULT MainWindowOnCreate(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnClose(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnLButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnLButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnLButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnRButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnRButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnRButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnResize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT MainWindowOnMouseMove(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMouseEnter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMouseLeave(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMouseWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnMouseHWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnChar(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnKeyUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnKeyDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT MainWindowOnKillFocus(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;


private:
	void Update();
	void Render();
	void RenderUI();
	void Present();

	std::unique_ptr<MainWindow> m_mainWindow;
	std::shared_ptr<DeviceResources> m_deviceResources;
	Timer m_timer;
	Simulation m_simulation;
	std::unique_ptr<Renderer> m_renderer;

	// Rendering resources
	std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, g_numFrameResources> m_allocators;
	int m_currentFrameIndex = 0;
	std::array<UINT64, g_numFrameResources> m_fences = {};
	std::unique_ptr<DescriptorVector> m_descriptorVector = nullptr;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
};
}