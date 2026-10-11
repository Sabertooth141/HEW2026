#include "ImGuiLayer.h"

#include <filesystem>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

ImGuiLayer::~ImGuiLayer()
{
	Shutdown();
}

void ImGuiLayer::Init(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
	if (isInitialized)
	{
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();

	const char* fontPath = "C:/Windows/Fonts/meiryo.ttc";

	if (std::filesystem::exists(fontPath))
	{
		io.Fonts->AddFontFromFileTTF(fontPath, 16.f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	}
	else
	{
		io.Fonts->AddFontDefault();
	}

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device, context);
	isInitialized = true;
}

void ImGuiLayer::Shutdown()
{
	if (!isInitialized)
	{
		return;
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	isInitialized = false;
}

void ImGuiLayer::BeginFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiLayer::EndFrame()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
