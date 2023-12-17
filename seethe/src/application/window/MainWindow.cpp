#include "MainWindow.h"
#include "WindowMessageMap.h"
#include "application/Application.h"
#include "utils/Log.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

#include <windowsx.h> // Included so we can use GET_X_LPARAM/GET_Y_LPARAM

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

const std::array<HCURSOR, 14> seethe::MainWindow::m_cursors = {
	LoadCursor(nullptr, IDC_ARROW),
	LoadCursor(nullptr, IDC_APPSTARTING),
	LoadCursor(nullptr, IDC_HELP),
	LoadCursor(nullptr, IDC_CROSS),
	LoadCursor(nullptr, IDC_SIZEWE),
	LoadCursor(nullptr, IDC_SIZENS),
	LoadCursor(nullptr, IDC_SIZENESW),
	LoadCursor(nullptr, IDC_SIZENWSE),
	LoadCursor(nullptr, IDC_HAND),
	LoadCursor(nullptr, IDC_WAIT),
	LoadCursor(nullptr, IDC_IBEAM),
	LoadCursor(nullptr, IDC_SIZEALL),
	LoadCursor(nullptr, IDC_NO),
	LoadCursor(nullptr, IDC_UPARROW)
};

namespace seethe
{
	MainWindow::MainWindow(Application* app, const WindowProperties& props) :
		WindowTemplate(props),
		m_app(app)
	{		
		// Perform a simple debug check to make sure all cursors are available
#ifdef DEBUG
		CheckCursors();
#endif
		Init(props);
	}

	MainWindow::~MainWindow()
	{
		Shutdown();
	}
	void MainWindow::Shutdown()
	{
		UnregisterClass(wndBaseClassName, m_hInst);
		DestroyWindow(m_hWnd);
	}

	void MainWindow::Init(const WindowProperties& props) noexcept
	{
		LOG_INFO("Creating window {0} ({1}, {2})", m_title, m_width, m_height);


		// ... Create window ...
	}

	std::optional<int> MainWindow::ProcessMessages() const noexcept
	{
		MSG msg;
		// while queue has messages, remove and dispatch them (but do not block on empty queue)
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// check for quit because peekmessage does not signal this via return val
			if (msg.message == WM_QUIT)
			{
				// return optional wrapping int (arg to PostQuitMessage is in wparam) signals quit
				return static_cast<int>(msg.wParam);
			}

			// TranslateMessage will post auxilliary WM_CHAR messages from key msgs
			TranslateMessage(&msg);

			// Can optionally obtain the LRESULT value that is returned, but from the Microsoft docs:
			// "The return value specifies the value returned by the window procedure. Although its 
			// meaning depends on the message being dispatched, the return value generally is ignored."
			// LRESULT result = DispatchMessage(&msg);
			DispatchMessage(&msg);
		}

		// return empty optional when not quitting app
		return {};
	}

	LRESULT MainWindow::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		//static seethe::WindowMessageMap mm;
		//LOG_TRACE("{}", mm(msg, wParam, lParam));

		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		{
			// In my testing so far, the ImGui_ImplWin32_WndProcHandler only actually returns true for
			// WM_SETCURSOR. When dragging around windows, entering text, clicking, etc., you will still
			// get sent all those messages
			return true;
		}

		switch (msg)
		{
		case WM_CREATE:			
		{	
			CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
			int height = cs->cy;
			int width = cs->cx;
			m_deviceResources = std::make_shared<DeviceResources>(hWnd, height, width);

			// According to: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-create
			// --> "An application should return zero if it processes this message."
			return 0;
		}
		case WM_CLOSE:			return m_app->MainWindowOnClose(hWnd, msg, wParam, lParam);
		case WM_LBUTTONDOWN:	return m_app->MainWindowOnLButtonDown(hWnd, msg, wParam, lParam);
		case WM_LBUTTONUP:		return m_app->MainWindowOnLButtonUp(hWnd, msg, wParam, lParam);
		case WM_LBUTTONDBLCLK:	return m_app->MainWindowOnLButtonDoubleClick(hWnd, msg, wParam, lParam);
		case WM_RBUTTONDBLCLK:  return m_app->MainWindowOnRButtonDoubleClick(hWnd, msg, wParam, lParam);
		case WM_MBUTTONDBLCLK:  return m_app->MainWindowOnMButtonDoubleClick(hWnd, msg, wParam, lParam);
		case WM_MBUTTONDOWN:	return m_app->MainWindowOnMButtonDown(hWnd, msg, wParam, lParam);
		case WM_MBUTTONUP:		return m_app->MainWindowOnMButtonUp(hWnd, msg, wParam, lParam);
		case WM_RBUTTONDOWN:	return m_app->MainWindowOnRButtonDown(hWnd, msg, wParam, lParam);
		case WM_RBUTTONUP:		return m_app->MainWindowOnRButtonUp(hWnd, msg, wParam, lParam);
		case WM_XBUTTONDOWN:	
			return GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ?
				m_app->MainWindowOnX1ButtonDown(hWnd, msg, wParam, lParam) :
				m_app->MainWindowOnX2ButtonDown(hWnd, msg, wParam, lParam);
		case WM_XBUTTONUP:		
			return GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ?
				m_app->MainWindowOnX1ButtonUp(hWnd, msg, wParam, lParam) :
				m_app->MainWindowOnX2ButtonUp(hWnd, msg, wParam, lParam);
		case WM_XBUTTONDBLCLK:	
			return GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ?
				m_app->MainWindowOnX1ButtonDoubleClick(hWnd, msg, wParam, lParam) :
				m_app->MainWindowOnX2ButtonDoubleClick(hWnd, msg, wParam, lParam);
		case WM_SIZE:
		{
			m_width = LOWORD(lParam);
			m_height = HIWORD(lParam);
			m_deviceResources->OnResize(m_height, m_width);

			// According to: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-size
			// --> "An application should return zero if it processes this message."
			return 0;
		}
		case WM_MOUSEMOVE:		
		{
			const POINTS pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			m_mouseX = pt.x; 
			m_mouseY = pt.y; 

			if (pt.x >= 0 && pt.x < m_width && pt.y >= 0 && pt.y < m_height)
			{
				if (!m_mouseIsInWindow) // Will tell you if the mouse was PREVIOUSLY in the window or not
				{
					m_mouseIsInWindow = true;

					SetCapture(hWnd);

					// NOTE: We only call enter OR move, not both. The mouse enter event should also be treated like a move event
					return m_app->MainWindowOnMouseEnter(hWnd, msg, wParam, lParam);
				}

				return m_app->MainWindowOnMouseMove(hWnd, msg, wParam, lParam);
			}
			else
			{
				m_mouseIsInWindow = false;

				if (wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)) 
				{
					return m_app->MainWindowOnMouseMove(hWnd, msg, wParam, lParam); 
				}
			}

			// If we reach here, the mouse is NOT over the window and no mouse buttons are down.
			ReleaseCapture();
			return m_app->MainWindowOnMouseLeave(hWnd, msg, wParam, lParam);
		}
		case WM_MOUSELEAVE:		return m_app->MainWindowOnMouseLeave(hWnd, msg, wParam, lParam);
		case WM_MOUSEWHEEL:		return m_app->MainWindowOnMouseWheel(hWnd, msg, wParam, lParam);
		case WM_MOUSEHWHEEL:	return m_app->MainWindowOnMouseHWheel(hWnd, msg, wParam, lParam);
		case WM_CHAR:			return m_app->MainWindowOnChar(hWnd, msg, wParam, lParam);
		case WM_SYSKEYUP:		return m_app->MainWindowOnKeyUp(hWnd, msg, wParam, lParam);
		case WM_KEYUP:			return m_app->MainWindowOnKeyUp(hWnd, msg, wParam, lParam);
		case WM_SYSKEYDOWN:		return m_app->MainWindowOnKeyDown(hWnd, msg, wParam, lParam);
		case WM_KEYDOWN:		return m_app->MainWindowOnKeyDown(hWnd, msg, wParam, lParam);
		case WM_KILLFOCUS:		return m_app->MainWindowOnKillFocus(hWnd, msg, wParam, lParam);
		case WM_DPICHANGED:
			LOG_WARN("{}", "Received WM_DPICHANGED. Currently not handling this message");
			break;
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	void MainWindow::CheckCursors() noexcept
	{
		for (unsigned int iii = 0; iii < m_cursors.size(); ++iii)
		{
//			if (m_cursors[iii] == NULL)
//				LOG_APP_ERROR("{}:{} cursor at index {} is NULL", __FILE__, __LINE__, iii);
		}
	}
	void MainWindow::SetCursor(Cursor cursor) noexcept
	{
		::SetCursor(m_cursors[static_cast<int>(cursor)]);
	}


}