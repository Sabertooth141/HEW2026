#pragma once
#include <algorithm>
#include <vector>

#include "AnimatorComponent.h"
#include "MaterialComponent.h"
#include "Renderer.h"
#include "SpriteRendererComponent.h"

class SpriteRenderSystem
{
public:
	void Register(AnimatorComponent* sprite)
	{
		renderers.push_back(sprite);
		isDirty = true;
	}

	void Unregister(GameObject* object)
	{
		std::erase_if(renderers, [object](const AnimatorComponent* a) { return a->owner == object; });
		isDirty = true;
	}

	void Render(const ViewRect2D& view)
	{
		SortIfDirty();

		// 画面端でスプライトが急に現れないよう、少し広めの範囲で判定する
		const ViewRect2D cullRect =
		{
			{ view.min.x - cullMargin, view.min.y - cullMargin },
			{ view.max.x + cullMargin, view.max.y + cullMargin }
		};

		drawnCount = 0;
		culledCount = 0;

		for (auto* sprite : renderers)
		{
			// カメラの外にあるスプライトは描画しない（定数バッファの更新とドローコールを省く）
			// アニメーションの更新は AnimationSystem で別に行うので、画面外でも進み続ける
			if (isCullingEnabled && !sprite->IsInView(cullRect))
			{
				++culledCount;
				continue;
			}

			sprite->Render();
			++drawnCount;
		}
	}

	void SetCullingEnabled(const bool inEnabled)
	{
		isCullingEnabled = inEnabled;
	}

	bool IsCullingEnabled() const
	{
		return isCullingEnabled;
	}

	// 直前のフレームで描画した数・カメラ外で省いた数（デバッグ表示用）
	int GetDrawnCount() const
	{
		return drawnCount;
	}

	int GetCulledCount() const
	{
		return culledCount;
	}

private:
	void SortIfDirty()
	{
		if (!isDirty)
		{
			return;
		}
		std::ranges::stable_sort(renderers,
			[](const AnimatorComponent* a, const AnimatorComponent* b)
			{
				if (a->GetCurrAnimation().spriteRenderer->layer != b->GetCurrAnimation().
					spriteRenderer->layer)
				{
					return static_cast<int>(a->GetCurrAnimation().spriteRenderer->layer) <
						static_cast<int>(b->GetCurrAnimation().spriteRenderer->layer);
				}
				return a->GetCurrAnimation().spriteRenderer->sortOrder < b->GetCurrAnimation().
					spriteRenderer->sortOrder;
			}
		);
		isDirty = false;
	}

private:
	std::vector<AnimatorComponent*> renderers;
	bool isDirty = true;

	bool isCullingEnabled = true;
	float cullMargin = 64.f;
	int drawnCount = 0;
	int culledCount = 0;
};