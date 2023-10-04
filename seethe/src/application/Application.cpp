#include "Application.h"
#include "utils/Log.h"

namespace seethe
{
Application::Application() :
	m_timer(),
	m_viewport{},
	m_scissorRect{ 0, 0, 1, 1 }
{
	m_mainWindow = std::make_unique<MainWindow>(this);
	m_deviceResources = std::make_shared<DeviceResources>(m_mainWindow->GetHWND(), m_mainWindow->GetHeight(), m_mainWindow->GetWidth());
	m_timer.Reset();


	// Initialize all fence values to 0
	std::fill(std::begin(m_fences), std::end(m_fences), 0); 

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

	// Initialize the descriptor vector
	m_descriptorVector = std::make_unique<DescriptorVector>(m_deviceResources, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Initialize viewport
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Height = static_cast<float>(m_mainWindow->GetHeight());
	m_viewport.Width = static_cast<float>(m_mainWindow->GetWidth());
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

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
}
void Application::Render()
{
	auto commandList = m_deviceResources->GetCommandList();

	GFX_THROW_INFO_ONLY(commandList->RSSetViewports(1, &m_viewport));
	GFX_THROW_INFO_ONLY(commandList->RSSetScissorRects(1, &m_scissorRect));

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

	//////
	// RENDER STUFF HERE
	//////

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
LRESULT Application::MainWindowOnResize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
{
	// ... Window Resize event ...

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