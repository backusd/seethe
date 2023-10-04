#pragma once
#include "pch.h"
#include "WindowTemplate.h"


namespace seethe
{
class Application;

enum class Cursor
{
	ARROW					= 0,
	ARROW_AND_HOURGLASS		= 1,
	ARROW_AND_QUESTION_MARK = 2,
	CROSS					= 3,
	DOUBLE_ARROW_EW			= 4,
	DOUBLE_ARROW_NS			= 5,
	DOUBLE_ARROW_NESW		= 6,
	DOUBLE_ARROW_NWSE		= 7,
	HAND					= 8,
	HOURGLASS				= 9,
	I_BEAM					= 10,
	QUAD_ARROW				= 11,
	SLASHED_CIRCLE			= 12,
	UP_ARROW				= 13
};

class MainWindow : public WindowTemplate<MainWindow>
{
public:
	MainWindow(Application* app, const WindowProperties& props = WindowProperties());
	MainWindow(const MainWindow&) = delete;
	MainWindow(MainWindow&&) = delete;
	MainWindow& operator=(const MainWindow&) = delete;
	MainWindow& operator=(MainWindow&&) = delete;
	virtual ~MainWindow() override;

	ND std::optional<int> ProcessMessages() const noexcept;
	ND LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept override;

private:
	Application* m_app;

	virtual void Init(const WindowProperties& props) noexcept;
	void Shutdown();

//	ND KEY_CODE WParamToKeyCode(WPARAM wParam) const noexcept;

	ND LRESULT OnCreate(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnClose(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnLButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnLButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnLButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnRButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnMButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnMButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnMButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnRButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnRButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnResize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT OnMouseMove(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); // cannot be const because it modifies m_mouseIsInWindow
	ND LRESULT OnMouseLeave(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnMouseWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnMouseHWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnChar(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnKeyUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnSysKeyUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnKeyDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnSysKeyDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnKillFocus(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;

public:
	void CheckCursors() noexcept;
	static void SetCursor(Cursor cursor) noexcept;
private:
	static const std::array<HCURSOR, 14> m_cursors;
};

}

template <>
struct std::formatter<seethe::Cursor> : std::formatter<std::string> {
	auto format(seethe::Cursor cursor, std::format_context& ctx) {
		std::string s = "";
		switch (cursor)
		{
		case seethe::Cursor::ARROW:						s = "Cursor::ARROW"; break;
		case seethe::Cursor::ARROW_AND_HOURGLASS:		s = "Cursor::ARROW_AND_HOURGLASS"; break;
		case seethe::Cursor::ARROW_AND_QUESTION_MARK:	s = "Cursor::ARROW_AND_QUESTION_MARK"; break;
		case seethe::Cursor::CROSS:						s = "Cursor::CROSS"; break;
		case seethe::Cursor::DOUBLE_ARROW_EW:			s = "Cursor::DOUBLE_ARROW_EW"; break;
		case seethe::Cursor::DOUBLE_ARROW_NS:			s = "Cursor::DOUBLE_ARROW_NS"; break;
		case seethe::Cursor::DOUBLE_ARROW_NESW:			s = "Cursor::DOUBLE_ARROW_NESW"; break;
		case seethe::Cursor::DOUBLE_ARROW_NWSE:			s = "Cursor::DOUBLE_ARROW_NWSE"; break;
		case seethe::Cursor::HAND:						s = "Cursor::HAND"; break;
		case seethe::Cursor::HOURGLASS:					s = "Cursor::HOURGLASS"; break;
		case seethe::Cursor::I_BEAM:					s = "Cursor::I_BEAM"; break;
		case seethe::Cursor::QUAD_ARROW:				s = "Cursor::QUAD_ARROW"; break;
		case seethe::Cursor::SLASHED_CIRCLE:			s = "Cursor::SLASHED_CIRCLE"; break;
		case seethe::Cursor::UP_ARROW:					s = "Cursor::UP_ARROW"; break;
		default:
			s = "Unrecognized Cursor Type";
			break;
		}
		return formatter<std::string>::format(s, ctx);
	}
};