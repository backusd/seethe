#include "MainWindow.h"
#include "WindowMessageMap.h"
#include "application/Application.h"
#include "utils/Log.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

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

	LRESULT MainWindow::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		//static seethe::WindowMessageMap mm;
		//LOG_TRACE("{}", mm(msg, wParam, lParam));

		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) 
			return true;

		switch (msg)
		{
		case WM_CREATE:			return m_app->MainWindowOnCreate(hWnd, msg, wParam, lParam);
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
		case WM_SIZE:			
			m_width = LOWORD(lParam);
			m_height = HIWORD(lParam);
			return m_app->MainWindowOnResize(hWnd, msg, wParam, lParam);
		case WM_MOUSEMOVE:		
		{
			const POINTS pt = MAKEPOINTS(lParam); 
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




/*
	LRESULT MainWindow::OnCreate(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		WindowCreateEvent e(cs->y, cs->x, cs->cx, cs->cy);
		m_OnWindowCreateFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-create
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnClose(HWND hWnd, UINT msg, WPARAM wParam , LPARAM lParam ) const
	{
		// we don't want the DefProc to handle this message because
		// we want our destructor to destroy the window, so return 0 instead of break

		// Perform any clean up work here

		WindowCloseEvent e;
		m_OnWindowCloseFn(e);

		PostQuitMessage(0);
		return 0;
	}

	LRESULT MainWindow::OnLButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		const POINTS pt = MAKEPOINTS(lParam);
		MouseButtonPressedEvent e(MOUSE_BUTTON::LBUTTON, pt.x, pt.y);
		m_OnMouseButtonPressedFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-lbuttondown
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnLButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		const POINTS pt = MAKEPOINTS(lParam);
		MouseButtonReleasedEvent e(MOUSE_BUTTON::LBUTTON, pt.x, pt.y);
		m_OnMouseButtonReleasedFn(e);

		if (pt.x < 0 || pt.x >= static_cast<int>(m_width) || pt.y < 0 || pt.y >= static_cast<int>(m_height))
		{
			ReleaseCapture();
			MouseLeaveEvent l;
			m_OnMouseLeaveFn(l);
		}

		BringToForeground();

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-lbuttonup
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnLButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		const POINTS pt = MAKEPOINTS(lParam);
		MouseButtonDoubleClickEvent e(MOUSE_BUTTON::LBUTTON, pt.x, pt.y);
		m_OnMouseButtonDoubleClickFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-lbuttondblclk
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnMButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		const POINTS pt = MAKEPOINTS(lParam);
		MouseButtonPressedEvent e(MOUSE_BUTTON::MBUTTON, pt.x, pt.y);
		m_OnMouseButtonPressedFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mbuttondown
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnMButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		const POINTS pt = MAKEPOINTS(lParam);
		MouseButtonReleasedEvent e(MOUSE_BUTTON::MBUTTON, pt.x, pt.y);
		m_OnMouseButtonReleasedFn(e);

		if (pt.x < 0 || pt.x >= static_cast<int>(m_width) || pt.y < 0 || pt.y >= static_cast<int>(m_height))
		{
			ReleaseCapture();
			MouseLeaveEvent l;
			m_OnMouseLeaveFn(l);
		}

		BringToForeground();

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mbuttonup
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnMButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		const POINTS pt = MAKEPOINTS(lParam);
		MouseButtonDoubleClickEvent e(MOUSE_BUTTON::MBUTTON, pt.x, pt.y);
		m_OnMouseButtonDoubleClickFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mbuttondblclk
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnRButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		const POINTS pt = MAKEPOINTS(lParam);
		MouseButtonPressedEvent e(MOUSE_BUTTON::RBUTTON, pt.x, pt.y);
		m_OnMouseButtonPressedFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-rbuttondown
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnRButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		const POINTS pt = MAKEPOINTS(lParam);
		MouseButtonReleasedEvent e(MOUSE_BUTTON::RBUTTON, pt.x, pt.y);
		m_OnMouseButtonReleasedFn(e);

		if (pt.x < 0 || pt.x >= static_cast<int>(m_width) || pt.y < 0 || pt.y >= static_cast<int>(m_height))
		{
			ReleaseCapture();
			MouseLeaveEvent l;
			m_OnMouseLeaveFn(l);
		}

		BringToForeground();

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-rbuttonup
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnRButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		const POINTS pt = MAKEPOINTS(lParam);
		MouseButtonDoubleClickEvent e(MOUSE_BUTTON::RBUTTON, pt.x, pt.y);
		m_OnMouseButtonDoubleClickFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-rbuttondblclk
		// --> "An application should return zero if it processes this message."
		return 0;
	}

	LRESULT MainWindow::OnResize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		m_width = LOWORD(lParam);
		m_height = HIWORD(lParam);
		WindowResizeEvent e(m_width, m_height);
		m_OnWindowResizeFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-size
		// --> "An application should return zero if it processes this message."
		return 0;
	}

	LRESULT MainWindow::OnMouseMove(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		const POINTS pt = MAKEPOINTS(lParam);
		m_mouseX = pt.x;
		m_mouseY = pt.y;
		// in client region -> trigger move event, and trigger enter event + capture mouse (if not previously in window)
		if (pt.x >= 0 && pt.x < static_cast<int>(m_width) && pt.y >= 0 && pt.y < static_cast<int>(m_height))
		{
			MouseMoveEvent e(pt.x, pt.y);
			m_OnMouseMoveFn(e);

			if (!m_mouseIsInWindow) // Will tell you if the mouse was PREVIOUSLY in the window or not
			{
				m_mouseIsInWindow = true;

				SetCapture(hWnd);

				MouseEnterEvent e;
				m_OnMouseEnterFn(e);
			}
		}
		// not in client -> trigger move event / maintain capture if a button is down
		else
		{
			m_mouseIsInWindow = false;

			if (wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON))
			{
				MouseMoveEvent e(pt.x, pt.y);
				m_OnMouseMoveFn(e);
			}
			// button up -> release capture / log event for leaving
			else
			{
				ReleaseCapture();

				MouseLeaveEvent e;
				m_OnMouseLeaveFn(e);
			}
		}

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousemove
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnMouseLeave(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		// Not handling here because wm_leave messages don't seem to trigger
		// Instead, we are triggering MouseLeaveEvent from OnMouseMove
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	LRESULT MainWindow::OnMouseWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		// Kind of weird behavior, but the mouse location stored in the lParam does not exactly match
		// the actual location of the mouse on the screen. So instead of calling MAKEPOINTS(lParam), we are
		// just going to keep track of mouse location and pass these values instead

		// POINTS pt = MAKEPOINTS(lParam);

		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		MouseScrolledEvent e(m_mouseX, m_mouseY, delta);
		m_OnMouseScrolledVerticalFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousewheel
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnMouseHWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		// Kind of weird behavior, but the mouse location stored in the lParam does not exactly match
		// the actual location of the mouse on the screen. So instead of calling MAKEPOINTS(lParam), we are
		// just going to keep track of mouse location and pass these values instead

		// const POINTS pt = MAKEPOINTS(lParam);

		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		MouseScrolledEvent e(m_mouseX, m_mouseY, delta);
		m_OnMouseScrolledHorizontalFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousehwheel
		// --> "An application should return zero if it processes this message."
		return 0;
	}

	LRESULT MainWindow::OnChar(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		// wParam - Contains the keycode
		// lParam - Bits 0-15 contain the repeat count
		CharEvent e(static_cast<char>(wParam), static_cast<int>(LOWORD(lParam)));
		m_OnCharFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-char
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnKeyUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		// wParam - Contains the keycode
		KeyReleasedEvent e(WParamToKeyCode(wParam));
		m_OnKeyReleasedFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keyup
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnKeyDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		// wParam - Contains the keycode
		// lParam - Bits 0-15 contain the repeat count
		// lParam - Bit 30 indicates whether the key was already down before the WM_KEYDOWN message was triggered
		bool keyWasPreviouslyDown = (lParam & 0x40000000) > 0;
		KeyPressedEvent e(WParamToKeyCode(wParam), static_cast<int>(LOWORD(lParam)), keyWasPreviouslyDown);
		m_OnKeyPressedFn(e);

		// According to: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keydown
		// --> "An application should return zero if it processes this message."
		return 0;
	}
	LRESULT MainWindow::OnSysKeyUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		return this->OnKeyUp(hWnd, msg, wParam, lParam);
	}
	LRESULT MainWindow::OnSysKeyDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		return this->OnKeyDown(hWnd, msg, wParam, lParam);
	}

	LRESULT MainWindow::OnKillFocus(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

*/

//	KEY_CODE MainWindow::WParamToKeyCode(WPARAM wParam) const noexcept
//	{
//		unsigned int w = static_cast<unsigned int>(wParam);
//		switch (w)
//		{
//		case VK_LBUTTON: return KEY_CODE::LBUTTON;
//		case VK_MBUTTON: return KEY_CODE::MBUTTON;
//		case VK_RBUTTON: return KEY_CODE::RBUTTON;
//		case VK_CANCEL: return KEY_CODE::CANCEL;
//		case VK_XBUTTON1: return KEY_CODE::X1BUTTON;
//		case VK_XBUTTON2: return KEY_CODE::X2BUTTON;
//		case VK_BACK: return KEY_CODE::BACKSPACE;
//		case VK_TAB: return KEY_CODE::TAB;
//		case VK_CLEAR: return KEY_CODE::CLEAR;
//		case VK_RETURN: return KEY_CODE::ENTER;
//		case VK_SHIFT: return KEY_CODE::SHIFT;
//		case VK_CONTROL: return KEY_CODE::CTRL;
//		case VK_MENU: return KEY_CODE::ALT;
//		case VK_PAUSE: return KEY_CODE::PAUSE;
//		case VK_CAPITAL: return KEY_CODE::CAPS_LOCK;
//		case VK_ESCAPE: return KEY_CODE::ESC;
//		case VK_SPACE: return KEY_CODE::SPACEBAR;
//		case VK_PRIOR: return KEY_CODE::PAGE_UP;
//		case VK_NEXT: return KEY_CODE::PAGE_DOWN;
//		case VK_END: return KEY_CODE::END;
//		case VK_HOME: return KEY_CODE::HOME;
//		case VK_LEFT: return KEY_CODE::LEFT_ARROW;
//		case VK_RIGHT: return KEY_CODE::RIGHT_ARROW;
//		case VK_UP: return KEY_CODE::UP_ARROW;
//		case VK_DOWN: return KEY_CODE::DOWN_ARROW;
//		case VK_SELECT: return KEY_CODE::SELECT;
//		case VK_PRINT: return KEY_CODE::PRINT;
//		case VK_EXECUTE: return KEY_CODE::EXECUTE;
//		case VK_SNAPSHOT: return KEY_CODE::PRINT_SCREEN;
//		case VK_INSERT: return KEY_CODE::INSERT;
//		case VK_HELP: return KEY_CODE::HELP;
//		case 0x30: return KEY_CODE::_0;
//		case 0x31: return KEY_CODE::_1;
//		case 0x32: return KEY_CODE::_2;
//		case 0x33: return KEY_CODE::_3;
//		case 0x34: return KEY_CODE::_4;
//		case 0x35: return KEY_CODE::_5;
//		case 0x36: return KEY_CODE::_6;
//		case 0x37: return KEY_CODE::_7;
//		case 0x38: return KEY_CODE::_8;
//		case 0x39: return KEY_CODE::_9;
//		case 0x41: return KEY_CODE::A;
//		case 0x42: return KEY_CODE::B;
//		case 0x43: return KEY_CODE::C;
//		case 0x44: return KEY_CODE::D;
//		case 0x45: return KEY_CODE::E;
//		case 0x46: return KEY_CODE::F;
//		case 0x47: return KEY_CODE::G;
//		case 0x48: return KEY_CODE::H;
//		case 0x49: return KEY_CODE::I;
//		case 0x4A: return KEY_CODE::J;
//		case 0x4B: return KEY_CODE::K;
//		case 0x4C: return KEY_CODE::L;
//		case 0x4D: return KEY_CODE::M;
//		case 0x4E: return KEY_CODE::N;
//		case 0x4F: return KEY_CODE::O;
//		case 0x50: return KEY_CODE::P;
//		case 0x51: return KEY_CODE::Q;
//		case 0x52: return KEY_CODE::R;
//		case 0x53: return KEY_CODE::S;
//		case 0x54: return KEY_CODE::T;
//		case 0x55: return KEY_CODE::U;
//		case 0x56: return KEY_CODE::V;
//		case 0x57: return KEY_CODE::W;
//		case 0x58: return KEY_CODE::X;
//		case 0x59: return KEY_CODE::Y;
//		case 0x5A: return KEY_CODE::Z;
//		case VK_LWIN: return KEY_CODE::LEFT_WINDOWS;
//		case VK_RWIN: return KEY_CODE::RIGHT_WINDOWS;
//		case VK_APPS: return KEY_CODE::APPLICATIONS;
//		case VK_SLEEP: return KEY_CODE::SLEEP;
//		case VK_MULTIPLY: return KEY_CODE::MULTIPLY;
//		case VK_ADD: return KEY_CODE::ADD;
//		case VK_SEPARATOR: return KEY_CODE::SEPARATOR;
//		case VK_SUBTRACT: return KEY_CODE::SUBTRACT;
//		case VK_DECIMAL: return KEY_CODE::DECIMAL;
//		case VK_DIVIDE: return KEY_CODE::DIVIDE;
//		case VK_F1: return KEY_CODE::F1;
//		case VK_F2: return KEY_CODE::F2;
//		case VK_F3: return KEY_CODE::F3;
//		case VK_F4: return KEY_CODE::F4;
//		case VK_F5: return KEY_CODE::F5;
//		case VK_F6: return KEY_CODE::F6;
//		case VK_F7: return KEY_CODE::F7;
//		case VK_F8: return KEY_CODE::F8;
//		case VK_F9: return KEY_CODE::F9;
//		case VK_F10: return KEY_CODE::F10;
//		case VK_F11: return KEY_CODE::F11;
//		case VK_F12: return KEY_CODE::F12;
//		case VK_F13: return KEY_CODE::F13;
//		case VK_F14: return KEY_CODE::F14;
//		case VK_F15: return KEY_CODE::F15;
//		case VK_F16: return KEY_CODE::F16;
//		case VK_F17: return KEY_CODE::F17;
//		case VK_F18: return KEY_CODE::F18;
//		case VK_F19: return KEY_CODE::F19;
//		case VK_F20: return KEY_CODE::F20;
//		case VK_F21: return KEY_CODE::F21;
//		case VK_F22: return KEY_CODE::F22;
//		case VK_F23: return KEY_CODE::F23;
//		case VK_F24: return KEY_CODE::F24;
//		case VK_NUMLOCK: return KEY_CODE::NUM_LOCK;
//		case VK_SCROLL: return KEY_CODE::SCROLL_LOCK;
//		}
//
//		return KEY_CODE::INVALID;
//	}

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