#pragma once

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include "MonoBehavior.h"

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

		const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
		if (screenSize.x <= 0.0f || screenSize.y <= 0.0f)
		{
			return;
		}

		const ImVec2 center{ screenSize.x * 0.5f,screenSize.y * 0.5f };

		const float outerRadius = 
			std::sqrt(screenSize.x * screenSize.x+screenSize.y * screenSize.y) + 2.0f;

		float progress = std::clamp(timer / transitionDuration, 0.0f, 1.0f);

		//出だしと終わりをなめらかな演出に
		progress = progress * progress * (3.0f - 2.0f * progress);

		const float openRadius = state == State::CLOSING 
										  ? outerRadius * (1.0f - progress) 
										  : outerRadius * progress;

		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		constexpr int segmentCount = 96;
		constexpr float pi = 3.14f;

		for (int segmentIndex = 0;segmentIndex < segmentCount;++segmentIndex)
		{
			const float angleA = 2.0f * pi * static_cast<float>(segmentIndex) / segmentCount;

			const float angleB = 2.0f * pi * static_cast<float>(segmentIndex + 1) / segmentCount;

			const ImVec2 innerA
			{
				center.x + std::cos(angleA) * openRadius,
				center.y + std::sin(angleA) * openRadius
			};
			const ImVec2 outerA
			{
				center.x + std::cos(angleA) * outerRadius,
				center.y + std::sin(angleA) * outerRadius
			};
			const ImVec2 outerB
			{
				center.x + std::cos(angleB) * outerRadius,
				center.y + std::sin(angleB) * outerRadius
			};
			const ImVec2 innerB
			{
				center.x + std::cos(angleB) * openRadius,
				center.y + std::sin(angleB) * openRadius
			};

			drawList->AddQuadFilled
			(
				innerA,
				outerA,
				outerB,
				innerB,
				IM_COL32(0, 0, 0, 255)
			);
		}
	}

private:
	State state = State::IDLE;
	float timer = 0.0f;
	float transitionDuration = 0.45f;
	bool respawnRequested = false;
};