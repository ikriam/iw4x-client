#pragma once

namespace Components
{
	class Window : public Component
	{
	public:
		Window();

		static int Width();
		static int Height();
		static int Width(HWND window);
		static int Height(HWND window);
		static void Dimension(RECT* rect);
		static void Dimension(HWND window, RECT* rect);

		static bool IsCursorWithin(HWND window);

		static HWND GetWindow();

	private:
		static BOOL CursorVisible;
		static Dvar::Var NoBorder;
		static Dvar::Var NativeCursor;

		static HWND MainWindow;

		static void ApplyCursor();

		static int IsNoBorder();

		static BOOL WINAPI MessageHandler(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

		static int WINAPI ShowCursorHook(BOOL show);
		static void DrawCursorStub(void *scrPlace, float x, float y, float w, float h, int horzAlign, int vertAlign, const float *color, Game::Material *material);

		static void StyleHookStub();
		static HWND WINAPI CreateMainWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	};
}
