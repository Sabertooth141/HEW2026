#pragma once
#include "Win.h"
#include <d3d11.h>

class ImGuiLayer
{
public:
	ImGuiLayer() = default;
	~ImGuiLayer();
	ImGuiLayer(const ImGuiLayer&) = delete;
	ImGuiLayer& operator=(const ImGuiLayer&) = delete;

	void Init(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* context);
	void Shutdown();
	void BeginFrame();
	void EndFrame();
	bool IsInitialized() const
	{
		return isInitialized;
	}

private:
	bool isInitialized = false;
};

