#include "Window.h"

#include <imgui.h>
#include "Window.h"

namespace
{
	constexpr DWORD WINDOW_STYLE = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;

	SIZE FitClientToWorkArea(int width, int height)
	{
		RECT work;
		SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);

		RECT frame = {0, 0, 0, 0};
		AdjustWindowRect(&frame, WINDOW_STYLE, FALSE);
		const int frameW = frame.right - frame.left;
		const int frameH = frame.bottom - frame.top;

		const float maxW = static_cast<float>(work.right - work.left - frameW);
		const float maxH = static_cast<float>(work.bottom - work.top - frameH);
		const float scale = std::min({1.0f, maxW / width, maxH / height});

		return
		{
			std::max(1, static_cast<int>(width * scale)),
			std::max(1, static_cast<int>(height * scale))
		};
	}
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

Window::WindowClass Window::WindowClass::wndClass;

Window::WindowClass::WindowClass() : hInstance(GetModuleHandleW(nullptr))
{
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleMsgSetup;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hCursor = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = GetName();

	RegisterClassExW(&wc);
}

Window::WindowClass::~WindowClass()
{
	UnregisterClassW(wndClassName, GetInstance());
}

const wchar_t* Window::WindowClass::GetName()
{
	return wndClassName;
}

HINSTANCE Window::WindowClass::GetInstance()
{
	return wndClass.hInstance;
}

Window::Window(int width, int height, const wchar_t* name) :
	width(width),
	height(height)
{

	const SIZE fitted = FitClientToWorkArea(width, height);

	RECT wr = {0, 0, fitted.cx, fitted.cy};
	AdjustWindowRect(&wr, WINDOW_STYLE, FALSE);
	const int winW = wr.right - wr.left;
	const int winH = wr.bottom - wr.top;

	RECT work;
	SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
	const int winX = work.left + std::max(0, static_cast<int>(work.right - work.left - winW) / 2);
	const int winY = work.top + std::max(0, static_cast<int>(work.bottom - work.top - winH) / 2);

	WIN_CALL(hWnd = CreateWindowW(
		WindowClass::GetName(),
		name,
                WINDOW_STYLE,
		winX, winY, winW, winH,
		nullptr, nullptr, WindowClass::GetInstance(), this
	));

	ShowWindow(hWnd, SW_SHOWDEFAULT);

	RECT client;
	GetClientRect(hWnd, &client);
	clientW = std::max(1L, client.right - client.left);
	clientH = std::max(1L, client.bottom - client.top);

	renderer.emplace(hWnd, width, height, clientW, clientH);

	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = 0;
	rid.hwndTarget = nullptr;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

Renderer& Window::GetRenderer()
{
	return *renderer;
}

HWND Window::GetHwnd() const
{
	return hWnd;
}

void Window::GetClientSize(int& outWidth, int& outHeight) const
{
	RECT clientRect{};

	//Windows APIからクライアント領域の幅と高さを取得
	if (!GetClientRect(hWnd, &clientRect))
	{
		//取得失敗時はウィンドウ作成時のサイズを返す
		outWidth = width;
		outHeight = height;
		return;
	}

	outWidth = clientRect.right - clientRect.left;
	outHeight = clientRect.bottom - clientRect.top;
}

void Window::SetTitle(const std::string& title)
{
	std::wstring wTitle(title.begin(), title.end());
	WIN_CALL(SetWindowTextW(hWnd, wTitle.c_str()));
}

std::optional<int> Window::ProcessMessages()
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return static_cast<int>(msg.wParam);
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return {};
}

LRESULT Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE)
	{
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		const auto pWnd = static_cast<Window*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMsgInterpret));

		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMsgInterpret(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const auto pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		return true;
	}

	const bool editorMouse = ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse;
	const bool editorKeys = ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard;

	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_KILLFOCUS:
		keyboard.ClearState();
		break;
	case WM_ACTIVATE:
		if (!cursorEnabled)
		{
			if (wParam & WA_ACTIVE)
			{
				ConfineCursor();
				HideCursor();
			}
			else
			{
				FreeCursor();
				ShowCursor();
			}
		}
		break;

	// keyboard msg
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (editorKeys)
		{
			break;
		}

		if (!(lParam & 0x40000000) || keyboard.AutorepeatIsEnabled())
		{
			keyboard.OnKeyPressed(static_cast<unsigned char>(wParam));
		}
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		keyboard.OnKeyReleased(static_cast<unsigned char>(wParam));
		break;
	case WM_CHAR:
		if (editorKeys)
		{
			break;
		}

		keyboard.OnChar(static_cast<unsigned char>(wParam));
		break;

	// mouse msg
	case WM_MOUSEMOVE:
		{
			const POINTS pt = MAKEPOINTS(lParam);

			if (!cursorEnabled)
			{
				if (!mouse.isInWindow)
				{
					SetCapture(hWnd);
					mouse.OnMouseEnter();
					HideCursor();
				}
				break;
			}

			const POINTS lp = ClientToLogic(pt);

			// in client region -> log move first | in client region NOW but not BEFORE -> log enter + focus on window
			if (pt.x >= 0 && pt.x < clientW && pt.y >= 0 && pt.y < clientH)
			{
				mouse.OnMouseMove(lp.x, lp.y);
				if (!mouse.IsInWindow())
				{
					SetCapture(hWnd);
					mouse.OnMouseEnter();
				}
			}
			// for out-of-window tracking - if button down still log move | if not release
			else
			{
				if (wParam & (MK_LBUTTON | MK_RBUTTON))
				{
					mouse.OnMouseMove(lp.x, lp.y);
				}
				else
				{
					ReleaseCapture();
					mouse.OnMouseLeave();
				}
			}
			break;
		}
	case WM_LBUTTONDOWN:
		{
			if (editorMouse)
			{
				break;
			}

			SetForegroundWindow(hWnd);
			if (!cursorEnabled)
			{
				ConfineCursor();
				HideCursor();
			}

			const POINTS pt = MAKEPOINTS(lParam);
			mouse.OnLeftPressed(pt.x, pt.y);
			break;
		}
	case WM_RBUTTONDOWN:
		{
			if (editorMouse)
			{
				break;
			}

			const POINTS pt = MAKEPOINTS(lParam);
			mouse.OnRightPressed(pt.x, pt.y);
			break;
		}
	case WM_LBUTTONUP:
		{
			const POINTS pt = MAKEPOINTS(lParam);
			const POINTS lp = ClientToLogic(pt);

			mouse.OnLeftReleased(pt.x, pt.y);
			// release mouse if outside window
			if (pt.x < 0 || pt.x >= clientW || pt.y < 0 || pt.y >= clientH)
			{
				ReleaseCapture();
				mouse.OnMouseLeave();
			}
			break;
		}
	case WM_RBUTTONUP:
		{
			const POINTS pt = MAKEPOINTS(lParam);
			mouse.OnRightReleased(pt.x, pt.y);
			// release mouse if outside window
			if (pt.x < 0 || pt.x >= width || pt.y < 0 || pt.y >= height)
			{
				ReleaseCapture();
				mouse.OnMouseLeave();
			}
			break;
		}
	case WM_MOUSEWHEEL:
		{
			if (editorMouse)
			{
				break;
			}

			const POINTS pt = MAKEPOINTS(lParam);
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			mouse.OnWheelDelta(pt.x, pt.y, delta);
			break;
		}

	// raw mouse msg
	case WM_INPUT:
		{
			if (editorMouse)
			{
				break;
			}

			if (!mouse.rawEnabled)
			{
				break;
			}

			UINT size;
			// first get size
			if (GetRawInputData(
				reinterpret_cast<HRAWINPUT>(lParam),
				RID_INPUT,
				nullptr,
				&size,
				sizeof(RAWINPUTHEADER)) == -1)
			{
				// bail if err
				break;
			}
			rawBuffer.resize(size);

			// read input data
			if ((GetRawInputData(
				reinterpret_cast<HRAWINPUT>(lParam),
				RID_INPUT,
				rawBuffer.data(),
				&size,
				sizeof(RAWINPUTHEADER)) != size))
			{
				// bail if err
				break;
			}

			const auto& ri = reinterpret_cast<const RAWINPUT&>(*rawBuffer.data());
			if (ri.header.dwType == RIM_TYPEMOUSE && (ri.data.mouse.lLastX != 0 || ri.data.mouse.lLastY != 0))
			{
				mouse.OnRawDelta(ri.data.mouse.lLastX, ri.data.mouse.lLastY);
			}
			break;
		}
	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Window::EnableCursor()
{
	cursorEnabled = true;
}

void Window::DisableCursor()
{
	cursorEnabled = false;
}

bool Window::CursorEnabled() const
{
	return cursorEnabled;
}

void Window::ConfineCursor()
{
	RECT rect;

	GetClientRect(hWnd, &rect);
	MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&rect), 2);
	ClipCursor(&rect);
}

void Window::FreeCursor()
{
	ClipCursor(nullptr);
}

void Window::ShowCursor()
{
	while (::ShowCursor(TRUE) >= 0);
}

void Window::HideCursor()
{
	while (::ShowCursor(FALSE) < 0);
}

POINTS Window::ClientToLogic(POINTS pt) const
{
	return
	{
		static_cast<SHORT>(pt.x * width / clientW),
		static_cast<SHORT>(pt.y * height / clientH)
	};
}
