#pragma once

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include "MonoBehavior.h"
#include "WindowSettings.h"
#include "Window.h"
#include "Win.h"

class RetryTransitionComponent :public MonoBehavior 
{
public:
	enum class State
	{
		IDLE,
		CLOSING,
		OPENING
	};

	void Begin()
	{
		if (state != State::IDLE)
		{
			return;
		}

		state = State::CLOSING;
		timer = 0.0f;
	}

	void Update(float deltaTime) override
	{
		if (state == State::IDLE)
		{
			return;
		}

		timer += deltaTime;

		if (timer < transitionDuration)
		{
			return;
		}

		timer = 0.0f;

		if (state == State::CLOSING)
		{
			state = State::OPENING;
			respawnRequested = true;
		}
		else
		{
			state = State::IDLE;
		}
	}
	
	bool IsRunning() const
	{
		return state != State::IDLE;
	}

	bool ConsumeRespawnRequest()
	{
		const bool requested = respawnRequested;
		respawnRequested = false;
		return requested;
	}

	void Draw() const
	{
		if (state == State::IDLE)
		{
			return;
		}

		// ImGuiのメインウィンドウに対応するHWNDを取得する。
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		if (viewport == nullptr || viewport->PlatformHandleRaw == nullptr)
		{
			return;
		}

		const HWND windowHandle =
			static_cast<HWND>(viewport->PlatformHandleRaw);

		// タイトルバーを除いた、ゲームの描画領域を取得する。
		RECT clientRect{};
		if (!GetClientRect(windowHandle, &clientRect))
		{
			return;
		}

		const float width =
			static_cast<float>(clientRect.right - clientRect.left);
		const float height =
			static_cast<float>(clientRect.bottom - clientRect.top);

		if (width <= 0.0f || height <= 0.0f)
		{
			return;
		}

		// 円の中心をクライアント領域の中心にする。
		const ImVec2 topLeft = viewport->Pos;
		const ImVec2 bottomRight{
			topLeft.x + width,
			topLeft.y + height
		};
		const ImVec2 center{
			topLeft.x + width * 0.5f,
			topLeft.y + height * 0.5f
		};

		// 円が画面の四隅を越える半径を計算する。
		const float halfWidth = width * 0.5f;
		const float halfHeight = height * 0.5f;
		const float outerRadius =
			static_cast<float>(std::sqrt(
				static_cast<double>(
					halfWidth * halfWidth + halfHeight * halfHeight)))
			+ 4.0f;

		// アニメーションの進行度を0～1にし、動きをなめらかにする。
		float progress = std::clamp(
			timer / transitionDuration,
			0.0f,
			1.0f);

		progress = progress * progress * (3.0f - 2.0f * progress);

		// 暗転時は円を縮め、復帰時は円を広げる。
		const float openRadius =
			state == State::CLOSING
			? outerRadius * (1.0f - progress)
			: outerRadius * progress;

		ImDrawList* drawList = ImGui::GetForegroundDrawList();

		// トランジションをクライアント領域内に描画する。
		drawList->PushClipRect(topLeft, bottomRight, false);

		constexpr int segmentCount = 128;
		constexpr float pi = 3.14159265f;
		const ImU32 transitionColor = IM_COL32(0, 0, 0, 255);

		// 円周を細かい扇形に分け、円の外側を塗る。
		for (int i = 0; i < segmentCount; ++i)
		{
			const float angleA =
				2.0f * pi * static_cast<float>(i)
				/ static_cast<float>(segmentCount);

			const float angleB =
				2.0f * pi * static_cast<float>(i + 1)
				/ static_cast<float>(segmentCount);

			const float cosA = std::cos(angleA);
			const float sinA = std::sin(angleA);
			const float cosB = std::cos(angleB);
			const float sinB = std::sin(angleB);

			const ImVec2 innerA{
				center.x + cosA * openRadius,
				center.y + sinA * openRadius
			};

			const ImVec2 outerA{
				center.x + cosA * outerRadius,
				center.y + sinA * outerRadius
			};

			const ImVec2 outerB{
				center.x + cosB * outerRadius,
				center.y + sinB * outerRadius
			};

			const ImVec2 innerB{
				center.x + cosB * openRadius,
				center.y + sinB * openRadius
			};

			drawList->AddQuadFilled(
				innerA,
				outerA,
				outerB,
				innerB,
				transitionColor);
		}

		drawList->PopClipRect();
	}

private:
	State state = State::IDLE;
	float timer = 0.0f;
	float transitionDuration = 0.45f;
	bool respawnRequested = false;
};