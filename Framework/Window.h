#pragma once
#include <optional>
#include <string>

#include "Keyboard.h"
#include "Mouse.h"
#include "Win.h"
#include "Renderer.h"

class Window
{
private:
	class WindowClass
	{
	public:
		WindowClass();
		~WindowClass();
		WindowClass(const WindowClass&) = delete;
		WindowClass& operator =(const WindowClass&) = delete;

		static const wchar_t* GetName();
		static HINSTANCE GetInstance();

	private:
		static constexpr auto wndClassName = L"DXPractice";
		static WindowClass wndClass;
		HINSTANCE hInstance;
	};

public:
	Window(int width, int height, const wchar_t* name);
	~Window() = default;
	Window(const Window&) = delete;
	Window& operator =(const Window&) = delete;

	void SetTitle(const std::string& title);
	static std::optional<int> ProcessMessages();

	void EnableCursor();
	void DisableCursor();
	bool CursorEnabled() const;

	Renderer& GetRenderer();

public:
	Keyboard keyboard;
	Mouse mouse;

private:
	static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HandleMsgInterpret(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void ConfineCursor();
	void FreeCursor();
	void ShowCursor();
	void HideCursor();


private:
	bool cursorEnabled = true;
	std::vector<BYTE> rawBuffer;

	HWND hWnd;
	int width;
	int height;
	std::optional<Renderer> renderer;
};
